#include "source/GcLib/pch.h"

#include "DirectGraphics.hpp"
#include "Texture.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//DirectGraphicsConfig
**********************************************************/
DirectGraphicsConfig::DirectGraphicsConfig() {
	bShowWindow_ = true;
	widthScreen_ = 800;
	heightScreen_ = 600;
	bWindowed_ = true;
	bUseRef_ = false;
	colorMode_ = COLOR_MODE_32BIT;
	bUseTripleBuffer_ = true;
	bVSync_ = false;
	bPseudoFullScreen_ = true;
	typeSamples_ = D3DMULTISAMPLE_NONE;
}
DirectGraphicsConfig::~DirectGraphicsConfig() {
}


/**********************************************************
//DirectGraphics
**********************************************************/
DirectGraphics* DirectGraphics::thisBase_ = nullptr;
DirectGraphics::DirectGraphics() {
	pDirect3D_ = nullptr;
	pDevice_ = nullptr;
	pBackSurf_ = nullptr;
	pZBuffer_ = nullptr;

#if defined(DNH_PROJ_EXECUTOR)
	camera_ = new DxCamera();
	camera2D_ = new DxCamera2D();
	
	stateFog_.bEnable = true;
	stateFog_.color = D3DXVECTOR4(0, 0, 0, 0);
	stateFog_.fogDist = D3DXVECTOR2(0, 1);

	bufferManager_ = nullptr;

	bMainRender_ = true;
	previousBlendMode_ = (BlendMode)-999;
	D3DXMatrixIdentity(&matViewPort_);
#endif
}
DirectGraphics::~DirectGraphics() {
	Logger::WriteTop("DirectGraphics: Finalizing.");

	ptr_release(pZBuffer_);
	ptr_release(pBackSurf_);
	ptr_release(pDevice_);
	ptr_release(pDirect3D_);
#if defined(DNH_PROJ_EXECUTOR)
	ptr_delete(bufferManager_);
#endif

	for (auto& itrSample : mapSupportMultisamples_) {
		delete itrSample.second.second;
	}

	thisBase_ = nullptr;
	Logger::WriteTop("DirectGraphics: Finalized.");
}
bool DirectGraphics::Initialize(HWND hWnd) {
	return this->Initialize(hWnd, config_);
}
bool DirectGraphics::Initialize(HWND hWnd, DirectGraphicsConfig& config) {
	if (thisBase_ != nullptr)return false;

	Logger::WriteTop("DirectGraphics: Initialize.");
	pDirect3D_ = Direct3DCreate9(D3D_SDK_VERSION);
	if (pDirect3D_ == nullptr)throw gstd::wexception("Direct3DCreate9 error.");

	config_ = config;
	wndStyleFull_ = WS_POPUP;
	wndStyleWin_ = WS_OVERLAPPEDWINDOW - WS_SIZEBOX;
	hAttachedWindow_ = hWnd;

	//FullScreenModeの設定
	ZeroMemory(&d3dppFull_, sizeof(D3DPRESENT_PARAMETERS));
	d3dppFull_.hDeviceWindow = hWnd;
	d3dppFull_.BackBufferWidth = config.GetScreenWidth();
	d3dppFull_.BackBufferHeight = config.GetScreenHeight();
	d3dppFull_.Windowed = FALSE;
	d3dppFull_.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dppFull_.BackBufferFormat = config.GetColorMode() == DirectGraphicsConfig::COLOR_MODE_16BIT ? 
		D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;
	d3dppFull_.BackBufferCount = 1;
	d3dppFull_.EnableAutoDepthStencil = TRUE;
	d3dppFull_.AutoDepthStencilFormat = D3DFMT_D16;
	d3dppFull_.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dppFull_.PresentationInterval = config.IsVSyncEnable() ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dppFull_.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	//WindowModeの設定
	D3DDISPLAYMODE dmode;
	HRESULT hrAdapt = pDirect3D_->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dmode);
	ZeroMemory(&d3dppWin_, sizeof(D3DPRESENT_PARAMETERS));
	d3dppWin_.BackBufferWidth = config.GetScreenWidth();
	d3dppWin_.BackBufferHeight = config.GetScreenHeight();
	d3dppWin_.Windowed = TRUE;
	d3dppWin_.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dppWin_.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dppWin_.hDeviceWindow = hWnd;
	d3dppWin_.BackBufferCount = 1;
	d3dppWin_.EnableAutoDepthStencil = TRUE;
	d3dppWin_.AutoDepthStencilFormat = D3DFMT_D16;
	d3dppWin_.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dppWin_.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dppWin_.FullScreen_RefreshRateInHz = 0;

	if (!config.IsWindowed()) {//FullScreenMode
		::SetWindowLong(hWnd, GWL_STYLE, wndStyleFull_);
		::ShowWindow(hWnd, SW_SHOW);
	}

	D3DDEVTYPE deviceType = config.IsReferenceEnable() ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL;
	{
		D3DPRESENT_PARAMETERS d3dpp = config.IsWindowed() ? d3dppWin_ : d3dppFull_;
		modeScreen_ = config.IsWindowed() ? SCREENMODE_WINDOW : SCREENMODE_FULLSCREEN;

		constexpr DWORD modeBase = D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;

		HRESULT hrDevice = E_FAIL;

		if (config.IsReferenceEnable()) {
			hrDevice = pDirect3D_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING | modeBase, &d3dpp, &pDevice_);
		}
		else {
			D3DCAPS9 caps;
			pDirect3D_->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

			if (caps.VertexShaderVersion >= D3DVS_VERSION(2, 0)) {
				hrDevice = pDirect3D_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
					D3DCREATE_HARDWARE_VERTEXPROCESSING | modeBase, &d3dpp, &pDevice_);
				if (!FAILED(hrDevice))Logger::WriteTop("DirectGraphics: Created device (D3DCREATE_HARDWARE_VERTEXPROCESSING)");
				if (FAILED(hrDevice)) {
					hrDevice = pDirect3D_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
						D3DCREATE_SOFTWARE_VERTEXPROCESSING | modeBase, &d3dpp, &pDevice_);
					if (!FAILED(hrDevice))Logger::WriteTop("DirectGraphics: Created device (D3DCREATE_SOFTWARE_VERTEXPROCESSING)");
				}
			}
			else {
				hrDevice = pDirect3D_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
					D3DCREATE_SOFTWARE_VERTEXPROCESSING | modeBase, &d3dpp, &pDevice_);
				if (!FAILED(hrDevice))Logger::WriteTop("DirectGraphics: Created device (D3DCREATE_SOFTWARE_VERTEXPROCESSING)");
			}

			if (FAILED(hrDevice)) {
				Logger::WriteTop("DirectGraphics: Cannot create device with HAL.");
				hrDevice = pDirect3D_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd,
					D3DCREATE_SOFTWARE_VERTEXPROCESSING | modeBase, &d3dpp, &pDevice_);
				deviceType = D3DDEVTYPE_REF;
			}
		}

		if (FAILED(hrDevice)) {
			throw gstd::wexception("IDirect3DDevice9::CreateDevice failure.");
		}
	}

	{
		D3DMULTISAMPLE_TYPE chkSamples[] = {
			D3DMULTISAMPLE_NONE,
			D3DMULTISAMPLE_2_SAMPLES,
			D3DMULTISAMPLE_3_SAMPLES,
			D3DMULTISAMPLE_4_SAMPLES,
		};
		for (size_t i = 0; i < sizeof(chkSamples) / sizeof(D3DMULTISAMPLE_TYPE); ++i) {
			DWORD* arrQuality = new DWORD[2];

			HRESULT hrColor = pDirect3D_->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, deviceType,
				config.GetColorMode() == DirectGraphicsConfig::COLOR_MODE_16BIT ? D3DFMT_X4R4G4B4 : D3DFMT_X8R8G8B8,
				FALSE, chkSamples[i], &arrQuality[0]);
			HRESULT hrDepth = pDirect3D_->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, deviceType,
				D3DFMT_D16, FALSE, chkSamples[i], &arrQuality[1]);

			arrQuality[0] -= 1;
			arrQuality[1] -= 1;
			mapSupportMultisamples_.insert(std::make_pair(chkSamples[i], 
				std::make_pair(SUCCEEDED(hrColor) && SUCCEEDED(hrDepth), arrQuality)));
		}

		if (!IsSupportMultiSample(config.GetMultiSampleType())) {
			Logger::WriteTop("DirectGraphics: Selected multisampling is not supported on this device, falling back to D3DMULTISAMPLE_NONE.");
		}
		else if (config.GetMultiSampleType() != D3DMULTISAMPLE_NONE) {
			if (!config.IsPseudoFullScreen()) {
				std::map<D3DMULTISAMPLE_TYPE, std::string> mapSampleIndex = {
					{ D3DMULTISAMPLE_NONE, "D3DMULTISAMPLE_NONE" },
					{ D3DMULTISAMPLE_2_SAMPLES, "D3DMULTISAMPLE_2_SAMPLES" },
					{ D3DMULTISAMPLE_3_SAMPLES, "D3DMULTISAMPLE_3_SAMPLES" },
					{ D3DMULTISAMPLE_4_SAMPLES, "D3DMULTISAMPLE_4_SAMPLES" },
				};

				DWORD* qualities = GetMultiSampleQuality();

				std::string log = StringUtility::Format("DirectGraphics: Anti-aliasing available [%s at Quality (%d, %d)]",
					mapSampleIndex[config.GetMultiSampleType()].c_str(), qualities[0], qualities[1]);
				Logger::WriteTop(log);

				SetMultiSampleType(config.GetMultiSampleType());
				//pDevice_->Reset(config.IsWindowed() ? &d3dppWin_ : &d3dppFull_);
				SetFullscreenAntiAliasing(true);
			}
			else {
				Logger::WriteTop("DirectGraphics: Anti-aliasing is not available for pseudo-fullscreen display.");
			}
		}
	}

	// BackSurface取得
	pDevice_->GetRenderTarget(0, &pBackSurf_);

	// Zバッファ取得
	pDevice_->GetDepthStencilSurface(&pZBuffer_);

#if defined(DNH_PROJ_EXECUTOR)
	bufferManager_ = new VertexBufferManager();
	bufferManager_->Initialize(this);
#endif

	thisBase_ = this;

#if defined(DNH_PROJ_EXECUTOR)
	if (camera2D_ != nullptr)
		camera2D_->Reset();
	_InitializeDeviceState(true);

	BeginScene();
	EndScene();
#endif

	Logger::WriteTop("DirectGraphics: Initialized.");
	return true;
}

#if defined(DNH_PROJ_EXECUTOR)
void DirectGraphics::_ReleaseDxResource() {
	ptr_release(pZBuffer_);
	ptr_release(pBackSurf_);

	std::list<DirectGraphicsListener*>::iterator itr;
	for (itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
		(*itr)->ReleaseDxResource();
	}
}
void DirectGraphics::_RestoreDxResource() {
	pDevice_->GetRenderTarget(0, &pBackSurf_);
	pDevice_->GetDepthStencilSurface(&pZBuffer_);

	std::list<DirectGraphicsListener*>::iterator itr;
	for (itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
		(*itr)->RestoreDxResource();
	}

	_InitializeDeviceState(true);
}
void DirectGraphics::_Restore() {
	Logger::WriteTop("DirectGraphics::_Restore");

	//The device was lost, wait until it's able to be restored
	HRESULT hr = pDevice_->TestCooperativeLevel();
	if (hr == D3DERR_DEVICELOST) {
		while (true) {
			Sleep(1000);
			//The device is now able to be restored, break out of the loop
			if ((hr = pDevice_->TestCooperativeLevel()) == D3DERR_DEVICENOTRESET) break;
		}
	}

	// リストア
	_ReleaseDxResource();

	//デバイスリセット
	if (modeScreen_ == SCREENMODE_FULLSCREEN)
		pDevice_->Reset(&d3dppFull_);
	else
		pDevice_->Reset(&d3dppWin_);

	_RestoreDxResource();

	Logger::WriteTop("DirectGraphics::_Restore finished.");
}
void DirectGraphics::_InitializeDeviceState(bool bResetCamera) {
	if (bResetCamera) {
		if (camera_) {
			camera_->SetWorldViewMatrix();
			camera_->SetProjectionMatrix();
			camera_->UpdateDeviceViewProjectionMatrix();
		}
		else {
			D3DXMATRIX viewMat;
			D3DXMATRIX persMat;

			D3DVECTOR viewFrom = D3DXVECTOR3(100, 300, -500);
			D3DXMatrixLookAtLH(&viewMat, (D3DXVECTOR3*)&viewFrom, &D3DXVECTOR3(0, 0, 0), &D3DXVECTOR3(0, 1, 0));

			D3DXMatrixPerspectiveFovLH(&persMat, D3DXToRadian(45.0),
				config_.GetScreenWidth() / (float)config_.GetScreenHeight(), 10.0f, 2000.0f);

			viewMat = viewMat * persMat;

			pDevice_->SetTransform(D3DTS_VIEW, &viewMat);
		}
	}

	SetCullingMode(D3DCULL_NONE);
	pDevice_->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	pDevice_->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_RGBA(192, 192, 192, 0));
	SetLightingEnable(true);
	SetSpecularEnable(false);

	D3DVECTOR dir;
	dir.x = -1;
	dir.y = -1;
	dir.z = -1;
	SetDirectionalLight(dir);

	SetBlendMode(MODE_BLEND_ALPHA);

	//αテスト
	SetAlphaTest(true, 0, D3DCMP_GREATER);

	//Zテスト
	SetZBufferEnable(false);
	SetZWriteEnable(false);

	//Filtering
	SetTextureFilter(D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);

	//ViewPort
	ResetViewPort();
}
void DirectGraphics::AddDirectGraphicsListener(DirectGraphicsListener* listener) {
	std::list<DirectGraphicsListener*>::iterator itr;
	for (itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
		if ((*itr) == listener)return;
	}
	listListener_.push_back(listener);
}
void DirectGraphics::RemoveDirectGraphicsListener(DirectGraphicsListener* listener) {
	std::list<DirectGraphicsListener*>::iterator itr;
	for (itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
		if ((*itr) != listener)continue;
		listListener_.erase(itr);
		break;
	}
}
void DirectGraphics::BeginScene(bool bMainRender, bool bClear) {
	if (bClear) ClearRenderTarget();
	bMainRender_ = bMainRender;
	pDevice_->BeginScene();

	if (camera_->thisViewChanged_ || camera_->thisProjectionChanged_) {
		if (camera_->thisViewChanged_) 
			camera_->SetWorldViewMatrix();
		if (camera_->thisProjectionChanged_) 
			camera_->SetProjectionMatrix();
		camera_->UpdateDeviceViewProjectionMatrix();
		camera_->thisViewChanged_ = false;
		camera_->thisProjectionChanged_ = false;
	}
}
void DirectGraphics::EndScene(bool bPresent) {
	pDevice_->EndScene();

	if (bPresent) {
		HRESULT hr = pDevice_->Present(nullptr, nullptr, nullptr, nullptr);
		if (FAILED(hr)) {
			_Restore();
			_InitializeDeviceState(true);
		}
	}
}
void DirectGraphics::ClearRenderTarget() {
	/*
	if (textureTarget_ == nullptr) {
		pDevice_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0, 0);
	}
	else {
		pDevice_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0, 0);
	}
	*/
	pDevice_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		textureTarget_  != nullptr ? D3DCOLOR_ARGB(0, 0, 0, 0) : D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
}
void DirectGraphics::ClearRenderTarget(RECT* rect) {
	//D3DRECT rcDest = { rect.left, rect.top, rect.right, rect.bottom };
	/*
	if (textureTarget_ == nullptr) {
		pDevice_->Clear(1, (D3DRECT*)rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0, 0);
	}
	else {
		pDevice_->Clear(1, (D3DRECT*)rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0, 0);
	}
	*/
	pDevice_->Clear(1, (D3DRECT*)rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
		textureTarget_ != nullptr ? D3DCOLOR_ARGB(0, 0, 0, 0) : D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
}
void DirectGraphics::SetRenderTarget(shared_ptr<Texture> texture, bool bResetCameraState) {
	textureTarget_ = texture;
	if (texture == nullptr) {
		pDevice_->SetRenderTarget(0, pBackSurf_);
		pDevice_->SetDepthStencilSurface(pZBuffer_);
	}
	else {
		pDevice_->SetRenderTarget(0, texture->GetD3DSurface());
		pDevice_->SetDepthStencilSurface(texture->GetD3DZBuffer());
	}
	_InitializeDeviceState(bResetCameraState);
}
void DirectGraphics::SetLightingEnable(bool bEnable) {
	pDevice_->SetRenderState(D3DRS_LIGHTING, bEnable);
}
void DirectGraphics::SetSpecularEnable(bool bEnable) {
	pDevice_->SetRenderState(D3DRS_SPECULARENABLE, bEnable);
}
void DirectGraphics::SetCullingMode(DWORD mode) {
	pDevice_->SetRenderState(D3DRS_CULLMODE, mode);
}
void DirectGraphics::SetShadingMode(DWORD mode) {
	pDevice_->SetRenderState(D3DRS_SHADEMODE, mode);
}
void DirectGraphics::SetZBufferEnable(bool bEnable) {
	pDevice_->SetRenderState(D3DRS_ZENABLE, bEnable);
}
void DirectGraphics::SetZWriteEnable(bool bEnable) {
	pDevice_->SetRenderState(D3DRS_ZWRITEENABLE, bEnable);
}
void DirectGraphics::SetAlphaTest(bool bEnable, DWORD ref, D3DCMPFUNC func) {
	pDevice_->SetRenderState(D3DRS_ALPHATESTENABLE, bEnable);
	if (bEnable) {
		pDevice_->SetRenderState(D3DRS_ALPHAFUNC, func);
		pDevice_->SetRenderState(D3DRS_ALPHAREF, ref);
	}
}
void DirectGraphics::SetBlendMode(BlendMode mode, int stage) {
	if (mode == previousBlendMode_) return;
	if (previousBlendMode_ = (BlendMode)-999) {
		pDevice_->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_MODULATE);
		pDevice_->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		pDevice_->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		pDevice_->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
		pDevice_->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
	}
	previousBlendMode_ = mode;

	pDevice_->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	pDevice_->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);

#define SETBLENDOP(op, alp) \
	pDevice_->SetRenderState(D3DRS_BLENDOP, op); \
	pDevice_->SetRenderState(D3DRS_ALPHABLENDENABLE, alp);
#define SETBLENDARGS(sbc, dbc, sba, dba) \
	pDevice_->SetRenderState(D3DRS_SRCBLEND, sbc); \
	pDevice_->SetRenderState(D3DRS_DESTBLEND, dbc); \
	pDevice_->SetRenderState(D3DRS_SRCBLENDALPHA, sba); \
	pDevice_->SetRenderState(D3DRS_DESTBLENDALPHA, dba);

	switch (mode) {
	case MODE_BLEND_NONE://なし
		pDevice_->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		SETBLENDOP(D3DBLENDOP_ADD, FALSE);
		SETBLENDARGS(D3DBLEND_ONE, D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_ZERO);
		break;
	case MODE_BLEND_ALPHA_INV:	//α and Invert
		pDevice_->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT);
	case MODE_BLEND_ALPHA://αで半透明合成
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	case MODE_BLEND_ADD_RGB://RGBで加算合成
		pDevice_->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_ONE);
		break;
	case MODE_BLEND_ADD_ARGB://αで加算合成
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	case MODE_BLEND_MULTIPLY://乗算合成
		pDevice_->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_ZERO, D3DBLEND_SRCCOLOR, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	case MODE_BLEND_SUBTRACT://減算合成
		SETBLENDOP(D3DBLENDOP_REVSUBTRACT, TRUE);
		SETBLENDARGS(D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	case MODE_BLEND_SHADOW://影描画用
		pDevice_->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_ZERO, D3DBLEND_INVSRCCOLOR, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	case MODE_BLEND_INV_DESTRGB://描画先色反転合成
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_INVDESTCOLOR, D3DBLEND_INVSRCCOLOR, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	}

	// 減算半透明合成
	//pDevice_->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
	//pDevice_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	//pDevice_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	// ハイライト(覆い焼き)
	//pDevice_->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	//pDevice_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
	//pDevice_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE); 

	// リバース(反転)
	//pDevice_->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	//pDevice_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_INVDESTCOLOR);
	//pDevice_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
}
void DirectGraphics::SetFillMode(DWORD mode) {
	pDevice_->SetRenderState(D3DRS_FILLMODE, mode);
}
void DirectGraphics::SetFogEnable(bool bEnable) {
	pDevice_->SetRenderState(D3DRS_FOGENABLE, bEnable ? TRUE : FALSE);
}
bool DirectGraphics::IsFogEnable() {
	DWORD fog = FALSE;
	pDevice_->GetRenderState(D3DRS_FOGENABLE, &fog);
	return (fog == TRUE);
}
void DirectGraphics::SetVertexFog(bool bEnable, D3DCOLOR color, float start, float end) {
	SetFogEnable(bEnable);

	pDevice_->SetRenderState(D3DRS_FOGCOLOR, color);
	pDevice_->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
	pDevice_->SetRenderState(D3DRS_FOGSTART, *(DWORD*)(&start));
	pDevice_->SetRenderState(D3DRS_FOGEND, *(DWORD*)(&end));

	stateFog_.bEnable = bEnable;
	stateFog_.color.x = ((color >> 16) & 0xff) / 255.0f;
	stateFog_.color.y = ((color >> 8) & 0xff) / 255.0f;
	stateFog_.color.z = (color & 0xff) / 255.0f;
	stateFog_.color.w = ((color >> 24) & 0xff) / 255.0f;
	stateFog_.fogDist.x = start;
	stateFog_.fogDist.y = end;
}
void DirectGraphics::SetPixelFog(bool bEnable, D3DCOLOR color, float start, float end) {}
void DirectGraphics::SetTextureFilter(D3DTEXTUREFILTERTYPE fMin, D3DTEXTUREFILTERTYPE fMag,
	D3DTEXTUREFILTERTYPE fMip, int stage)
{
	if (fMin >= D3DTEXF_NONE) pDevice_->SetSamplerState(stage, D3DSAMP_MINFILTER, fMin);
	if (fMag >= D3DTEXF_NONE) pDevice_->SetSamplerState(stage, D3DSAMP_MAGFILTER, fMag);
	if (fMip >= D3DTEXF_NONE) pDevice_->SetSamplerState(stage, D3DSAMP_MIPFILTER, fMip);
}
DWORD DirectGraphics::GetTextureFilter(D3DTEXTUREFILTERTYPE* fMin, D3DTEXTUREFILTERTYPE* fMag,
	D3DTEXTUREFILTERTYPE* fMip, int stage)
{
	DWORD res = 0;
	DWORD tmp;
	if (fMin) {
		pDevice_->GetSamplerState(stage, D3DSAMP_MINFILTER, &tmp);
		*fMin = (D3DTEXTUREFILTERTYPE)tmp;
		++res;
	}
	if (fMag) {
		pDevice_->GetSamplerState(stage, D3DSAMP_MAGFILTER, &tmp);
		*fMag = (D3DTEXTUREFILTERTYPE)tmp;
		++res;
	}
	if (fMip) {
		pDevice_->GetSamplerState(stage, D3DSAMP_MIPFILTER, &tmp);
		*fMip = (D3DTEXTUREFILTERTYPE)tmp;
		++res;
	}
	return res;
}
void DirectGraphics::SetDirectionalLight(D3DVECTOR& dir) {
	D3DLIGHT9 light;
	ZeroMemory(&light, sizeof(D3DLIGHT9));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse.r = 0.5f;
	light.Diffuse.g = 0.5f;
	light.Diffuse.b = 0.5f;
	light.Ambient.r = 0.5f;
	light.Ambient.g = 0.5f;
	light.Ambient.b = 0.5f;
	light.Direction = dir;
	pDevice_->SetLight(0, &light);
	pDevice_->LightEnable(0, TRUE);
}
#endif
void DirectGraphics::SetMultiSampleType(D3DMULTISAMPLE_TYPE type) {
	if (IsSupportMultiSample(type)) {
		d3dppWin_.MultiSampleType = type;
		d3dppFull_.MultiSampleType = type;
	}
	else {
		d3dppWin_.MultiSampleType = D3DMULTISAMPLE_NONE;
		d3dppFull_.MultiSampleType = D3DMULTISAMPLE_NONE;
	}
}
D3DMULTISAMPLE_TYPE DirectGraphics::GetMultiSampleType() {
	return d3dppFull_.MultiSampleType;
}
void DirectGraphics::SetMultiSampleQuality(DWORD* quality) {
	if (quality) {
		d3dppWin_.MultiSampleQuality = quality[0];
		d3dppFull_.MultiSampleQuality = quality[0];
	}
	else {
		d3dppWin_.MultiSampleQuality = 0;
		d3dppFull_.MultiSampleQuality = 0;
	}
}
DWORD* DirectGraphics::GetMultiSampleQuality() {
	auto itr = mapSupportMultisamples_.find(GetMultiSampleType());
	if (itr == mapSupportMultisamples_.end()) return nullptr;
	return itr->second.second;
}
HRESULT DirectGraphics::SetFullscreenAntiAliasing(bool bEnable) {
	return pDevice_->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, bEnable ? TRUE : FALSE);
}
bool DirectGraphics::IsSupportMultiSample(D3DMULTISAMPLE_TYPE type) {
	auto itr = mapSupportMultisamples_.find(type);
	if (itr == mapSupportMultisamples_.end()) return false;
	return itr->second.first;
}

#if defined(DNH_PROJ_EXECUTOR)
void DirectGraphics::SetViewPort(int x, int y, int width, int height) {
	D3DVIEWPORT9 viewPort;
	ZeroMemory(&viewPort, sizeof(D3DVIEWPORT9));
	viewPort.X = x;
	viewPort.Y = y;
	viewPort.Width = width;
	viewPort.Height = height;
	viewPort.MinZ = 0.0f;
	viewPort.MaxZ = 1.0f;
	pDevice_->SetViewport(&viewPort);

	{
		matViewPort_._11 = 2.0f / width;
		matViewPort_._22 = -2.0f / height;
		matViewPort_._41 = -(float)(width + x * 2.0f) / width;
		matViewPort_._42 = (float)(height + y * 2.0f) / height;
	}
}
void DirectGraphics::ResetViewPort() {
	SetViewPort(0, 0, GetScreenWidth(), GetScreenHeight());
}
int DirectGraphics::GetScreenWidth() {
	return config_.GetScreenWidth();
}
int DirectGraphics::GetScreenHeight() {
	return config_.GetScreenHeight();
}
double DirectGraphics::GetScreenWidthRatio() {
	RECT rect;
	::GetWindowRect(hAttachedWindow_, &rect);
	double widthWindow = (double)rect.right - (double)rect.left;
	double widthView = config_.GetScreenWidth();

	/*
	DWORD style = ::GetWindowLong(hAttachedWindow_, GWL_STYLE);
	if (modeScreen_ == SCREENMODE_WINDOW && (style & (WS_OVERLAPPEDWINDOW - WS_SIZEBOX)) > 0) {
		widthWindow -= GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXDLGFRAME);
	}
	*/

	return widthWindow / widthView;
}
double DirectGraphics::GetScreenHeightRatio() {
	RECT rect;
	::GetWindowRect(hAttachedWindow_, &rect);
	double heightWindow = (double)rect.bottom - (double)rect.top;
	double heightView = config_.GetScreenHeight();

	/*
	DWORD style = ::GetWindowLong(hAttachedWindow_, GWL_STYLE);
	if (modeScreen_ == SCREENMODE_WINDOW && (style & (WS_OVERLAPPEDWINDOW - WS_SIZEBOX)) > 0) {
		heightWindow -= GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYBORDER) + 
			GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);
	}
	*/

	return heightWindow / heightView;
}
POINT DirectGraphics::GetMousePosition() {
	POINT res = { 0, 0 };
	GetCursorPos(&res);
	ScreenToClient(hAttachedWindow_, &res);

	double ratioWidth = GetScreenWidthRatio();
	double ratioHeight = GetScreenHeightRatio();
	if (ratioWidth != 0) {
		res.x = (int)(res.x / ratioWidth);
	}
	if (ratioHeight != 0) {
		res.y = (int)(res.y / ratioHeight);
	}

	return res;
}

void DirectGraphics::SaveBackSurfaceToFile(std::wstring path) {
	RECT rect = { 0, 0, config_.GetScreenWidth(), config_.GetScreenHeight() };
	LPDIRECT3DSURFACE9 pBackSurface = nullptr;
	pDevice_->GetRenderTarget(0, &pBackSurface);
	D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_BMP,
		pBackSurface, nullptr, &rect);
	pBackSurface->Release();
}
bool DirectGraphics::IsPixelShaderSupported(int major, int minor) {
	D3DCAPS9 caps;
	pDevice_->GetDeviceCaps(&caps);
	bool res = caps.PixelShaderVersion >= D3DPS_VERSION(major, minor);
	return res;
}
#endif

#if defined(DNH_PROJ_EXECUTOR)
/**********************************************************
//DirectGraphicsPrimaryWindow
**********************************************************/
DirectGraphicsPrimaryWindow::DirectGraphicsPrimaryWindow() {
	lpCursor_ = nullptr;
}
DirectGraphicsPrimaryWindow::~DirectGraphicsPrimaryWindow() {

}
void DirectGraphicsPrimaryWindow::_PauseDrawing() {
	//	gstd::Application::GetBase()->SetActive(false);
		// ウインドウのメニューバーを描画する
	::DrawMenuBar(hWnd_);
	// ウインドウのフレームを描画する
	::RedrawWindow(hWnd_, nullptr, nullptr, RDW_FRAME);
}
void DirectGraphicsPrimaryWindow::_RestartDrawing() {
	gstd::Application::GetBase()->SetActive(true);
}
bool DirectGraphicsPrimaryWindow::Initialize() {
	this->Initialize(config_);
	return true;
}
bool DirectGraphicsPrimaryWindow::Initialize(DirectGraphicsConfig& config) {
	HINSTANCE hInst = ::GetModuleHandle(nullptr);
	lpCursor_ = LoadCursor(nullptr, IDC_ARROW);
	{
		std::wstring nameClass = L"DirectGraphicsPrimaryWindow";
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(WNDCLASSEX);
		//		wcex.style=CS_HREDRAW|CS_VREDRAW;
		wcex.lpfnWndProc = (WNDPROC)WindowBase::_StaticWindowProcedure;
		wcex.hInstance = hInst;
		wcex.hIcon = nullptr;
		wcex.hCursor = lpCursor_;
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOWTEXT);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = nameClass.c_str();
		wcex.hIconSm = nullptr;
		::RegisterClassEx(&wcex);

		RECT wr = { 0, 0, config.GetScreenWidth(), config.GetScreenHeight() };
		AdjustWindowRect(&wr, wndStyleWin_, FALSE);

		hWnd_ = ::CreateWindow(wcex.lpszClassName,
			L"",
			WS_OVERLAPPEDWINDOW - WS_SIZEBOX,
			0, 0, wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, hInst, nullptr);
	}


	HWND hWndGraphics = nullptr;
	if (config.IsPseudoFullScreen()) {
		//擬似フルスクリーンの場合は、子ウィンドウにDirectGraphicsを配置する
		std::wstring nameClass = L"DirectGraphicsPrimaryWindow.Child";
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = (WNDPROC)WindowBase::_StaticWindowProcedure;
		wcex.hInstance = hInst;
		wcex.hCursor = lpCursor_;
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOWTEXT);
		wcex.lpszClassName = nameClass.c_str();
		::RegisterClassEx(&wcex);

		int screenWidth = config_.GetScreenWidth(); //+ ::GetSystemMetrics(SM_CXEDGE) + 10;
		int screenHeight = config_.GetScreenHeight(); //+ ::GetSystemMetrics(SM_CYEDGE) + 10;

		HWND hWnd = ::CreateWindow(wcex.lpszClassName,
			L"",
			WS_CHILD | WS_VISIBLE,
			0, 0, screenWidth, screenHeight, hWnd_, nullptr, hInst, nullptr);
		wndGraphics_.Attach(hWnd);

		hWndGraphics = hWnd;
	}
	else {
		if (config.IsShowWindow())
			::ShowWindow(hWnd_, SW_SHOW);
		hWndGraphics = hWnd_;
	}
	::UpdateWindow(hWnd_);
	this->Attach(hWnd_);

	DirectGraphics::Initialize(hWndGraphics, config);

	ShowCursor(config.IsShowCursor() ? TRUE : FALSE);

	if (modeScreen_ == SCREENMODE_WINDOW) {
		::SetWindowLong(hWnd_, GWL_STYLE, wndStyleWin_);
		::ShowWindow(hWnd_, SW_SHOW);

		int screenWidth = config_.GetScreenWidth();
		int screenHeight = config_.GetScreenHeight();

		RECT wr = { 0, 0, screenWidth, screenHeight };
		AdjustWindowRect(&wr, wndStyleWin_, FALSE);

		SetBounds(0, 0, wr.right - wr.left, wr.bottom - wr.top);
		MoveWindowCenter();
	}

	return true;
}

LRESULT DirectGraphicsPrimaryWindow::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
	{
		::DestroyWindow(hWnd);
		return FALSE;
	}
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		return FALSE;
	}
	case WM_ACTIVATEAPP:
	{
		if ((BOOL)wParam)
			_RestartDrawing();
		else
			_PauseDrawing();
		return FALSE;
	}
	case WM_ENTERMENULOOP:
	{
		//メニューが選択されたら動作を停止する
		_PauseDrawing();
		return FALSE;
	}
	case WM_EXITMENULOOP:
	{
		//メニューの選択が解除されたら動作を再開する
		_RestartDrawing();
		return FALSE;
	}
	case WM_SIZE:
	{
		if (wndGraphics_.GetWindowHandle() != nullptr) {
			RECT rect;
			::GetClientRect(hWnd, &rect);
			int width = rect.right;
			int height = rect.bottom;

			int screenWidth = config_.GetScreenWidth();
			int screenHeight = config_.GetScreenHeight();

			double ratioWH = (double)screenWidth / (double)screenHeight;
			if (width > rect.right)width = rect.right;
			height = (double)width / ratioWH;

			double ratioHW = (double)screenHeight / (double)screenWidth;
			if (height > rect.bottom) height = rect.bottom;
			width = (double)height / ratioHW;

			int wX = (rect.right - width) / 2;
			int wY = (rect.bottom - height) / 2;
			wndGraphics_.SetBounds(wX, wY, width, height);
		}

		return FALSE;
	}
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* info = (MINMAXINFO*)lParam;
		int wWidth = ::GetSystemMetrics(SM_CXFULLSCREEN);
		int wHeight = ::GetSystemMetrics(SM_CYFULLSCREEN);

		int screenWidth = config_.GetScreenWidth();
		int screenHeight = config_.GetScreenHeight();

		RECT wr = { 0, 0, screenWidth, screenHeight };
		AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW - WS_SIZEBOX, FALSE);

		int width = wr.right - wr.left;
		int height = wr.bottom - wr.top;

		/*
		double ratioWH = (double)screenWidth / (double)screenHeight;
		if (width > wWidth)width = wWidth;
		height = (double)width / ratioWH;

		double ratioHW = 1.0 / ratioWH;
		if (height > wHeight) height = wHeight;
		width = (double)height / ratioHW;
		*/

		info->ptMaxSize.x = width;
		info->ptMaxSize.y = height;
		return FALSE;
	}
	/*
	case WM_KEYDOWN:
	{
		switch (wParam) {
		case VK_F12:
			::PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
		return FALSE;
	}
	*/
#if defined(DNH_PROJ_EXECUTOR)
	case WM_SYSCHAR:
	{
		if (wParam == VK_RETURN)
			this->ChangeScreenMode();
		return FALSE;
	}
#endif
	}
	return _CallPreviousWindowProcedure(hWnd, uMsg, wParam, lParam);
}

void DirectGraphicsPrimaryWindow::ChangeScreenMode() {
	if (!config_.IsPseudoFullScreen()) {
		if (modeScreen_ == SCREENMODE_WINDOW) {
			int screenWidth = config_.GetScreenWidth(); //+ ::GetSystemMetrics(SM_CXEDGE) + 10;
			int screenHeight = config_.GetScreenHeight(); //+ ::GetSystemMetrics(SM_CYEDGE) + 10;
			int wWidth = ::GetSystemMetrics(SM_CXFULLSCREEN);
			int wHeight = ::GetSystemMetrics(SM_CYFULLSCREEN);
			bool bFullScreenEnable = screenWidth <= wWidth && screenHeight <= wHeight;
			if (!bFullScreenEnable) {
				std::string log = StringUtility::Format(
					"This display does not support fullscreen mode : display[%d-%d] screen[%d-%d]", wWidth, wHeight, screenWidth, screenHeight);
				Logger::WriteTop(log);
				return;
			}
		}

		Application::GetBase()->SetActive(true);

		//テクスチャ解放
		_ReleaseDxResource();

		HRESULT hrReset = E_FAIL;

		if (modeScreen_ == SCREENMODE_FULLSCREEN) {
			hrReset = pDevice_->Reset(&d3dppWin_);
			::SetWindowLong(hAttachedWindow_, GWL_STYLE, wndStyleWin_);
			::ShowWindow(hAttachedWindow_, SW_SHOW);

			int screenWidth = config_.GetScreenWidth();
			int screenHeight = config_.GetScreenHeight();

			RECT wr = { 0, 0, screenWidth, screenHeight };
			AdjustWindowRect(&wr, wndStyleWin_, FALSE);

			SetBounds(0, 0, wr.right - wr.left, wr.bottom - wr.top);
			MoveWindowCenter();

			/*
			RECT drect, mrect;
			HWND hDesk = ::GetDesktopWindow();
			::GetWindowRect(hDesk, &drect);
			::GetWindowRect(hAttachedWindow_, &mrect);

			int wWidth = mrect.right - mrect.left;
			int wHeight = mrect.bottom - mrect.top;
			int left = drect.right / 2 - wWidth / 2;
			int top = drect.bottom / 2 - wHeight / 2;
			::MoveWindow(hAttachedWindow_, left, top, wWidth, wHeight, TRUE);
			*/

			::SetWindowPos(hAttachedWindow_, HWND_NOTOPMOST,
				0, 0, 0, 0,
				SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOSENDCHANGING);

			modeScreen_ = SCREENMODE_WINDOW;
			/*
			if (config_.IsShowCursor()) {
				while (::ShowCursor(TRUE) < 0) {}
			}
			else ShowCursor(FALSE);
			*/
			::ShowCursor(config_.IsShowCursor() ? TRUE : FALSE);
			if (!config_.IsShowCursor()) {
				::SetCursor(nullptr);
				pDevice_->ShowCursor(FALSE);
			}
			else {
				::SetCursor(lpCursor_);
				pDevice_->ShowCursor(TRUE);
			}
		}
		else {
			hrReset = pDevice_->Reset(&d3dppFull_);
			::SetWindowLong(hAttachedWindow_, GWL_STYLE, wndStyleFull_);
			::ShowWindow(hAttachedWindow_, SW_SHOW);

			::ShowCursor(FALSE);
			/*if (!config_.IsShowCursor())*/ {
				::SetCursor(nullptr);
				pDevice_->ShowCursor(FALSE);
			}

			modeScreen_ = SCREENMODE_FULLSCREEN;
		}

		previousBlendMode_ = (BlendMode)-999;
		if (FAILED(hrReset)) {
			std::wstring err = StringUtility::Format(L"IDirect3DDevice9::Reset: \n%s\n  %s",
				DXGetErrorString(hrReset), DXGetErrorDescription(hrReset));
			throw gstd::wexception(err);
		}

		//テクスチャレストア
		_RestoreDxResource();
	}
	else {
		if (modeScreen_ == SCREENMODE_FULLSCREEN) {
			::SetWindowLong(hWnd_, GWL_STYLE, wndStyleWin_);
			::ShowWindow(hWnd_, SW_SHOW);

			int screenWidth = config_.GetScreenWidth();
			int screenHeight = config_.GetScreenHeight();

			RECT wr = { 0, 0, screenWidth, screenHeight };
			AdjustWindowRect(&wr, wndStyleWin_, FALSE);

			SetBounds(0, 0, wr.right - wr.left, wr.bottom - wr.top);
			MoveWindowCenter();

			modeScreen_ = SCREENMODE_WINDOW;
		}
		else {
			RECT rect;
			GetWindowRect(GetDesktopWindow(), &rect);
			::SetWindowLong(hWnd_, GWL_STYLE, wndStyleFull_);
			::ShowWindow(hWnd_, SW_SHOW);
			::MoveWindow(hWnd_, 0, 0, rect.right, rect.bottom, TRUE);

			modeScreen_ = SCREENMODE_FULLSCREEN;
		}
	}

}


/**********************************************************
//DxCamera
**********************************************************/
DxCamera::DxCamera() {
	Reset();
}
DxCamera::~DxCamera() {
}
void DxCamera::Reset() {
	radius_ = 500;
	angleAzimuth_ = D3DXToRadian(15);
	angleElevation_ = D3DXToRadian(45);

	ZeroMemory(&pos_, sizeof(D3DXVECTOR3));
	ZeroMemory(&camPos_, sizeof(D3DXVECTOR3));
	ZeroMemory(&laPosLookAt_, sizeof(D3DXVECTOR3));

	yaw_ = 0;
	pitch_ = 0;
	roll_ = 0;

	projWidth_ = 384.0f;
	projHeight_ = 448.0f;
	clipNear_ = 10.0f;
	clipFar_ = 2000.0f;

	D3DXMatrixIdentity(&matView_);
	D3DXMatrixIdentity(&matProjection_);
	D3DXMatrixIdentity(&matViewProjection_);
	D3DXMatrixIdentity(&matViewInverse_);
	D3DXMatrixIdentity(&matViewTranspose_);
	D3DXMatrixIdentity(&matProjectionInverse_);
	D3DXMatrixIdentity(&matIdentity_);

	thisViewChanged_ = true;
	thisProjectionChanged_ = true;

	modeCamera_ = MODE_NORMAL;
}

D3DXMATRIX DxCamera::GetMatrixLookAtLH() {
	D3DXMATRIX res;

	D3DXVECTOR3 vCameraUp(0, 1, 0);
	{
		D3DXQUATERNION qRot(0, 0, 0, 1.0f);
		D3DXQuaternionRotationYawPitchRoll(&qRot, yaw_, pitch_, roll_);
		D3DXMATRIX matRot;
		D3DXMatrixRotationQuaternion(&matRot, &qRot);
		D3DXVec3TransformCoord((D3DXVECTOR3*)&vCameraUp, (D3DXVECTOR3*)&vCameraUp, &matRot);
	}

	if (modeCamera_ == MODE_LOOKAT) {
		D3DXMatrixLookAtLH(&res, &pos_, &laPosLookAt_, &vCameraUp);
	}
	else {
		camPos_.x = pos_.x + (float)(radius_ * cos(angleElevation_) * cos(angleAzimuth_));
		camPos_.y = pos_.y + (float)(radius_ * sin(angleElevation_));
		camPos_.z = pos_.z + (float)(radius_ * cos(angleElevation_) * sin(angleAzimuth_));

		D3DXVECTOR3 posTo = pos_;

		{
			D3DXMATRIX matTrans1;
			D3DXMatrixTranslation(&matTrans1, -camPos_.x, -camPos_.y, -camPos_.z);
			D3DXMATRIX matTrans2;
			D3DXMatrixTranslation(&matTrans2, camPos_.x, camPos_.y, camPos_.z);

			D3DXQUATERNION qRot(0, 0, 0, 1.0f);
			D3DXQuaternionRotationYawPitchRoll(&qRot, yaw_, pitch_, 0);
			D3DXMATRIX matRot;
			D3DXMatrixRotationQuaternion(&matRot, &qRot);

			D3DXMATRIX mat;
			mat = matTrans1 * matRot * matTrans2;
			D3DXVec3TransformCoord((D3DXVECTOR3*)&posTo, (D3DXVECTOR3*)&posTo, &mat);
		}

		D3DXMatrixLookAtLH(&res, &camPos_, &posTo, &vCameraUp);
	}

	return res;
}
void DxCamera::SetWorldViewMatrix() {
	DirectGraphics* graph = DirectGraphics::GetBase();
	if (graph == nullptr)return;
	IDirect3DDevice9* device = graph->GetDevice();

	matView_ = GetMatrixLookAtLH();
	D3DXMatrixInverse(&matViewInverse_, nullptr, &matView_);

	{
		matViewTranspose_._11 = matView_._11;
		matViewTranspose_._12 = matView_._21;
		matViewTranspose_._13 = matView_._31;
		matViewTranspose_._21 = matView_._12;
		matViewTranspose_._22 = matView_._22;
		matViewTranspose_._23 = matView_._32;
		matViewTranspose_._31 = matView_._13;
		matViewTranspose_._32 = matView_._23;
		matViewTranspose_._33 = matView_._33;
		matViewTranspose_._14 = 0.0f;
		matViewTranspose_._24 = 0.0f;
		matViewTranspose_._34 = 0.0f;
		matViewTranspose_._44 = 1.0f;
	}

	//UpdateDeviceProjectionMatrix();
}
void DxCamera::SetProjectionMatrix() {
	DirectGraphics* graph = DirectGraphics::GetBase();
	if (graph == nullptr)return;
	IDirect3DDevice9* device = graph->GetDevice();

	D3DXMatrixPerspectiveFovLH(&matProjection_, D3DXToRadian(45.0),
		projWidth_ / projHeight_, clipNear_, clipFar_);
	D3DXMatrixInverse(&matProjectionInverse_, nullptr, &matProjection_);

	//UpdateDeviceProjectionMatrix();
}
void DxCamera::UpdateDeviceViewProjectionMatrix() {
	DirectGraphics* graph = DirectGraphics::GetBase();
	if (graph == nullptr)return;
	IDirect3DDevice9* device = graph->GetDevice();

	matViewProjection_ = matView_ * matProjection_;
	device->SetTransform(D3DTS_VIEW, &matViewProjection_);
	device->SetTransform(D3DTS_PROJECTION, &matIdentity_);
}
D3DXVECTOR2 DxCamera::TransformCoordinateTo2D(D3DXVECTOR3 pos) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	int width = graphics->GetConfigData().GetScreenWidth();
	int height = graphics->GetConfigData().GetScreenHeight();

	D3DXVECTOR4 vect;
	D3DXVec3Transform(&vect, &pos, &matViewProjection_);

	if (vect.w > 0) {
		vect.x = width / 2.0f + (vect.x / vect.w) * width / 2.0f;
		vect.y = height / 2.0f - (vect.y / vect.w) * height / 2.0f; // Ｙ方向は上が正となるため
	}

	D3DXVECTOR2 res(vect.x, vect.y);
	return res;
}

/**********************************************************
//DxCamera2D
**********************************************************/
DxCamera2D::DxCamera2D() {
	pos_.x = 400;
	pos_.y = 300;
	ratioX_ = 1.0;
	ratioY_ = 1.0;
	angleZ_ = 0;
	bEnable_ = false;

	posReset_ = nullptr;

	D3DXMatrixIdentity(&matIdentity_);
}
DxCamera2D::~DxCamera2D() {}
void DxCamera2D::Reset() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	int width = graphics->GetScreenWidth();
	int height = graphics->GetScreenHeight();
	if (posReset_ == nullptr) {
		pos_.x = width / 2;
		pos_.y = height / 2;
	}
	else {
		pos_.x = posReset_->x;
		pos_.y = posReset_->y;
	}
	ratioX_ = 1.0;
	ratioY_ = 1.0;
	SetRect(&rcClip_, 0, 0, width, height);

	angleZ_ = 0;
}
D3DXVECTOR2 DxCamera2D::GetLeftTopPosition() {
	return GetLeftTopPosition(pos_, ratioX_, ratioY_, rcClip_);
}
D3DXVECTOR2 DxCamera2D::GetLeftTopPosition(D3DXVECTOR2 focus, double ratio) {
	return GetLeftTopPosition(focus, ratio, ratio);
}
D3DXVECTOR2 DxCamera2D::GetLeftTopPosition(D3DXVECTOR2 focus, double ratioX, double ratioY) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	int width = graphics->GetScreenWidth();
	int height = graphics->GetScreenHeight();
	RECT rcClip;
	ZeroMemory(&rcClip, sizeof(RECT));
	rcClip.right = width;
	rcClip.bottom = height;
	return GetLeftTopPosition(focus, ratioX, ratioY, rcClip);
}
D3DXVECTOR2 DxCamera2D::GetLeftTopPosition(D3DXVECTOR2 focus, double ratioX, double ratioY, RECT rcClip) {
	LONG width = rcClip.right - rcClip.left;
	LONG height = rcClip.bottom - rcClip.top;

	LONG cx = rcClip.left + width / 2; //画面の中心座標x
	LONG cy = rcClip.top + height / 2; //画面の中心座標y

	LONG dx = focus.x - cx; //現フォーカスでの画面左端位置
	LONG dy = focus.y - cy; //現フォーカスでの画面上端位置

	D3DXVECTOR2 res;
	res.x = cx - dx * ratioX; //現フォーカスでの画面中心の位置(x座標変換量)
	res.y = cy - dy * ratioY; //現フォーカスでの画面中心の位置(y座標変換量)

	res.x -= (width / 2) * ratioX; //現フォーカスでの画面左の位置(x座標変換量)
	res.y -= (height / 2) * ratioY; //現フォーカスでの画面中心の位置(x座標変換量)

	return res;
}

void DxCamera2D::UpdateMatrix() {
	D3DXVECTOR2 pos = GetLeftTopPosition();
	/*
	D3DXMATRIX matScale;
	D3DXMatrixScaling(&matScale, ratioX_, ratioY_, 1.0);
	D3DXMATRIX matTrans;
	D3DXMatrixTranslation(&matTrans, pos.x, pos.y, 0);
	*/

	D3DXMatrixIdentity(&matCamera_);

	D3DXMATRIX matAngleZ;
	D3DXMatrixIdentity(&matAngleZ);
	if (angleZ_ != 0) {
		/*
		D3DXMATRIX matTransRot1;
		D3DXMatrixTranslation(&matTransRot1, -GetFocusX() + pos.x, -GetFocusY() + pos.y, 0);
		D3DXMATRIX matRot;
		D3DXMatrixRotationYawPitchRoll(&matRot, 0, 0, angleZ_);
		D3DXMATRIX matTransRot2;
		D3DXMatrixTranslation(&matTransRot2, GetFocusX() - pos.x, GetFocusY() - pos.y, 0);
		matAngleZ = matTransRot1 * matRot * matTransRot2;
		*/

		float c = cosf(angleZ_);
		float s = sinf(angleZ_);
		float x = -GetFocusX() + pos.x;
		float y = -GetFocusY() + pos.y;

		matAngleZ._11 = c;	matAngleZ._12 = s;
		matAngleZ._21 = s;	matAngleZ._22 = -c;
		matAngleZ._41 = c * x + s * y - x;
		matAngleZ._42 = s * x - c * y - y;

		matCamera_ = matAngleZ;
	}

	/*
	mat = mat * matScale;
	mat = mat * matAngleZ;
	mat = mat * matTrans;
	*/ 
	matCamera_._11 *= ratioX_;
	matCamera_._22 *= ratioY_;
	matCamera_._41 += pos.x;
	matCamera_._42 += pos.y;
}
#endif
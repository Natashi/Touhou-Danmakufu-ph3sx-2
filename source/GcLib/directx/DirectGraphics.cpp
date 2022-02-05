#include "source/GcLib/pch.h"

#include "DirectGraphics.hpp"

#include "DxUtility.hpp"

#if defined(DNH_PROJ_EXECUTOR)
#include "Texture.hpp"

#include "../../TouhouDanmakufu/Common/DnhCommon.hpp"
#endif

using namespace gstd;
using namespace directx;

//*******************************************************************
//DirectGraphicsConfig
//*******************************************************************
DirectGraphicsConfig::DirectGraphicsConfig() {
	bShowWindow_ = true;
	bShowCursor_ = true;

	bWindowed_ = true;
	bBorderlessFullscreen_ = true;

	sizeScreen_ = { 640, 480 };
	sizeScreenDisplay_ = { 640, 480 };
	bUseDynamicScaling_ = false;

	colorMode_ = COLOR_MODE_32BIT;
	typeMultiSample_ = D3DMULTISAMPLE_NONE;
	
	bUseRef_ = false;
	bUseTripleBuffer_ = true;
	bVSync_ = false;
	
	bCheckDeviceCaps_ = true;
}

#if defined(DNH_PROJ_EXECUTOR)
//*******************************************************************
//DirectGraphics
//*******************************************************************
DirectGraphics* DirectGraphics::thisBase_ = nullptr;
float DirectGraphics::g_dxCoordsMul_ = 1.0f;
DirectGraphics::DirectGraphics() {
	ZeroMemory(&dxModules_, sizeof(dxModules_));

	pDirect3D_ = nullptr;
	pDevice_ = nullptr;
	pBackSurf_ = nullptr;
	pZBuffer_ = nullptr;

	deviceStatus_ = S_OK;

	camera_ = new DxCamera();
	camera2D_ = new DxCamera2D();

	defaultRenderTargetSize_[0] = 1024;
	defaultRenderTargetSize_[1] = 512;
	
	stateFog_.bEnable = true;
	stateFog_.color = D3DXVECTOR4(0, 0, 0, 0);
	stateFog_.fogDist = D3DXVECTOR2(0, 1);

	bufferManager_ = nullptr;

	bMainRender_ = true;
	previousBlendMode_ = BlendMode::RESET;
	D3DXMatrixIdentity(&matViewPort_);
}
DirectGraphics::~DirectGraphics() {
	Logger::WriteTop("DirectGraphics: Finalizing.");

	ptr_release(pZBuffer_);
	ptr_release(pBackSurf_);
	ptr_release(pDevice_);
	ptr_release(pDirect3D_);
	ptr_delete(bufferManager_);

	for (auto& itrSample : mapSupportMultisamples_) {
		delete itrSample.second.second;
	}

	_FreeModules();

	thisBase_ = nullptr;
	Logger::WriteTop("DirectGraphics: Finalized.");
}

void DirectGraphics::_LoadModules() {
	HANDLE hCurrentProcess = ::GetCurrentProcess();

	auto _LoadModule = [](const std::wstring& name, HMODULE* hDest, bool bThrowErr = true) -> bool {
		*hDest = ::LoadLibraryW(name.c_str());
		if (*hDest == nullptr && bThrowErr)
			throw gstd::wexception(L"Failed to load module: " + name);
		return *hDest != nullptr;
	};

	_LoadModule(L"d3d9.dll", &dxModules_.hLibrary_d3d9);
	_LoadModule(L"d3dx9_43.dll", &dxModules_.hLibrary_d3dx9);
	_LoadModule(L"d3dcompiler_43.dll", &dxModules_.hLibrary_d3dcompiler);
	_LoadModule(L"dsound.dll", &dxModules_.hLibrary_dsound);
	_LoadModule(L"dinput8.dll", &dxModules_.hLibrary_dinput8);
}
void DirectGraphics::_FreeModules() {
	auto _Free = [](HMODULE* pModule) {
		if (*pModule) {
			::FreeLibrary(*pModule);
			*pModule = nullptr;
		}
	};

	_Free(&dxModules_.hLibrary_d3d9);
	_Free(&dxModules_.hLibrary_d3dx9);
	_Free(&dxModules_.hLibrary_d3dcompiler);
	_Free(&dxModules_.hLibrary_dinput8);
	_Free(&dxModules_.hLibrary_dsound);
}

bool DirectGraphics::Initialize(HWND hWnd) {
	return this->Initialize(hWnd, config_);
}
bool DirectGraphics::Initialize(HWND hWnd, const DirectGraphicsConfig& config) {
	if (thisBase_) return false;

	_LoadModules();

	Logger::WriteTop("DirectGraphics: Initialize.");
	pDirect3D_ = Direct3DCreate9(D3D_SDK_VERSION);
	if (pDirect3D_ == nullptr) throw gstd::wexception("Direct3DCreate9 error.");

	config_ = config;
	hAttachedWindow_ = hWnd;

	D3DCAPS9 capsRef;
	D3DCAPS9 capsHal;
	pDirect3D_->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, &capsRef);
	pDirect3D_->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &capsHal);

	D3DDEVTYPE deviceType = config.bUseRef_ ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL;
	deviceCaps_ = deviceType == D3DDEVTYPE_REF ? capsRef : capsHal;
	if (config.bCheckDeviceCaps_ && !config.bUseRef_)
		_VerifyDeviceCaps();

	bool bDeviceVSyncAvailable = (deviceCaps_.PresentationIntervals & D3DPRESENT_INTERVAL_ONE) != 0;

	UINT dxBackBufferW = config.sizeScreen_.x;
	UINT dxBackBufferH = config.sizeScreen_.y;
	if (config.bUseDynamicScaling_) {
		dxBackBufferW = config.sizeScreenDisplay_.x;
		dxBackBufferH = config.sizeScreenDisplay_.y;

		float coordRateX = dxBackBufferW / (float)config.sizeScreen_.x;
		float coordRateY = dxBackBufferH / (float)config.sizeScreen_.y;

		//g_dxCoordsMul_ = std::min(coordRateX, coordRateY);
		g_dxCoordsMul_ = 1.0f;
	}

	{
		//Fullscreen mode settings

		ZeroMemory(&d3dppFull_, sizeof(D3DPRESENT_PARAMETERS));
		d3dppFull_.hDeviceWindow = hWnd;
		d3dppFull_.BackBufferWidth = dxBackBufferW;
		d3dppFull_.BackBufferHeight = dxBackBufferH;
		d3dppFull_.Windowed = FALSE;
		d3dppFull_.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dppFull_.BackBufferFormat = config.colorMode_ == ColorMode::COLOR_MODE_16BIT ?
			D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;
		d3dppFull_.BackBufferCount = 1;
		d3dppFull_.EnableAutoDepthStencil = TRUE;
		d3dppFull_.AutoDepthStencilFormat = D3DFMT_D16;
		d3dppFull_.MultiSampleType = D3DMULTISAMPLE_NONE;
		d3dppFull_.PresentationInterval = (bDeviceVSyncAvailable && config.bVSync_)
			? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
		d3dppFull_.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	}

	{
		//Windowed mode settings
		
		ZeroMemory(&d3dppWin_, sizeof(D3DPRESENT_PARAMETERS));
		d3dppWin_.hDeviceWindow = hWnd;
		d3dppWin_.BackBufferWidth = dxBackBufferW;
		d3dppWin_.BackBufferHeight = dxBackBufferH;
		d3dppWin_.Windowed = TRUE;
		d3dppWin_.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dppWin_.BackBufferFormat = D3DFMT_UNKNOWN;
		d3dppWin_.BackBufferCount = 1;
		d3dppWin_.EnableAutoDepthStencil = TRUE;
		d3dppWin_.AutoDepthStencilFormat = D3DFMT_D16;
		d3dppWin_.MultiSampleType = D3DMULTISAMPLE_NONE;
		d3dppWin_.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		d3dppWin_.FullScreen_RefreshRateInHz = 0;
	}

	if (!config.bWindowed_) {	//Start in fullscreen Mode
		::SetWindowLong(hWnd, GWL_STYLE, wndStyleFull_);
		::ShowWindow(hWnd, SW_SHOW);
	}

	{
		D3DPRESENT_PARAMETERS* d3dpp = config.bWindowed_ ? &d3dppWin_ : &d3dppFull_;
		modeScreen_ = config.bWindowed_ ? SCREENMODE_WINDOW : SCREENMODE_FULLSCREEN;

		HRESULT hrDevice = E_FAIL;
		{
			auto _TryCreateDevice = [&](D3DDEVTYPE type, DWORD addFlag) {
				hrDevice = pDirect3D_->CreateDevice(D3DADAPTER_DEFAULT, type, hWnd, 
					addFlag | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, d3dpp, &pDevice_);
			};
			if (config.bUseRef_) {
				_TryCreateDevice(D3DDEVTYPE_REF, D3DCREATE_SOFTWARE_VERTEXPROCESSING);
			}
			else {
				_TryCreateDevice(D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
				if (SUCCEEDED(hrDevice)) {
					Logger::WriteTop("DirectGraphics: Created device (D3DCREATE_HARDWARE_VERTEXPROCESSING)");
				}
				else {
					_TryCreateDevice(D3DDEVTYPE_HAL, D3DCREATE_SOFTWARE_VERTEXPROCESSING);
					if (SUCCEEDED(hrDevice))
						Logger::WriteTop("DirectGraphics: Created device (D3DCREATE_SOFTWARE_VERTEXPROCESSING)");
				}

				if (FAILED(hrDevice)) {
					std::wstring err = StringUtility::Format(
						L"Cannot create Direct3D device with HAL. [%s]\r\n  %s\r\n%s",
						DXGetErrorString(hrDevice), DXGetErrorDescription(hrDevice),
						L"Restart in reference rasterizer mode.");
					throw wexception(err);
				}
			}
		}

		if (FAILED(hrDevice)) {
			//deviceStatus_ = D3DERR_NOTAVAILABLE;
			std::wstring err = StringUtility::Format(L"Cannot create Direct3D device. [%s]\r\n  %s",
				DXGetErrorString(hrDevice), DXGetErrorDescription(hrDevice));
			if (deviceType == D3DDEVTYPE_HAL)
				err += L"\r\nRestart in reference rasterizer mode.";
			throw wexception(err);
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
				config.colorMode_ == ColorMode::COLOR_MODE_16BIT ? D3DFMT_X4R4G4B4 : D3DFMT_X8R8G8B8,
				FALSE, chkSamples[i], &arrQuality[0]);
			HRESULT hrDepth = pDirect3D_->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, deviceType,
				D3DFMT_D16, FALSE, chkSamples[i], &arrQuality[1]);

			arrQuality[0] -= 1;
			arrQuality[1] -= 1;
			mapSupportMultisamples_.insert(std::make_pair(chkSamples[i], 
				std::make_pair(SUCCEEDED(hrColor) && SUCCEEDED(hrDepth), arrQuality)));
		}

		if (!IsSupportMultiSample(config.typeMultiSample_)) {
			Logger::WriteTop("DirectGraphics: Selected multisampling is not supported on this device, falling back to D3DMULTISAMPLE_NONE.");
		}
		else if (config.typeMultiSample_ != D3DMULTISAMPLE_NONE) {
			if (!config.bBorderlessFullscreen_) {
				std::map<D3DMULTISAMPLE_TYPE, std::string> mapSampleIndex = {
					{ D3DMULTISAMPLE_NONE, "D3DMULTISAMPLE_NONE" },
					{ D3DMULTISAMPLE_2_SAMPLES, "D3DMULTISAMPLE_2_SAMPLES" },
					{ D3DMULTISAMPLE_3_SAMPLES, "D3DMULTISAMPLE_3_SAMPLES" },
					{ D3DMULTISAMPLE_4_SAMPLES, "D3DMULTISAMPLE_4_SAMPLES" },
				};

				DWORD* qualities = GetMultiSampleQuality();

				std::string log = StringUtility::Format("DirectGraphics: Anti-aliasing available [%s at Quality (%d, %d)]",
					mapSampleIndex[config.typeMultiSample_].c_str(), qualities[0], qualities[1]);
				Logger::WriteTop(log);

				SetMultiSampleType(config.typeMultiSample_);
				//pDevice_->Reset(config.bWindowed_ ? &d3dppWin_ : &d3dppFull_);
				SetFullscreenAntiAliasing(true);
			}
			else {
				Logger::WriteTop("DirectGraphics: Anti-aliasing is not available for pseudo-fullscreen display.");
			}
		}
	}

	pDevice_->GetRenderTarget(0, &pBackSurf_);
	pDevice_->GetDepthStencilSurface(&pZBuffer_);

	bufferManager_ = new VertexBufferManager();
	bufferManager_->Initialize(this);

	thisBase_ = this;

	if (camera2D_)
		camera2D_->Reset();
	_InitializeDeviceState(true);

	BeginScene();
	EndScene();

	Logger::WriteTop("DirectGraphics: Initialized.");
	return true;
}

void DirectGraphics::_VerifyDeviceCaps() {
	std::vector<std::string> listError;
	std::vector<std::string> listWarning;

	//listError.push_back("Test");
	//listWarning.push_back("Test");

	if ((deviceCaps_.Caps2 & D3DCAPS2_DYNAMICTEXTURES) == 0)
		listError.push_back("Device doesn't support dynamic textures");

	if ((deviceCaps_.DevCaps & D3DDEVCAPS_DRAWPRIMTLVERTEX) == 0)
		listError.push_back("Device can't draw TLVERTEX primitives");
	if ((deviceCaps_.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0)
		listError.push_back("Device lacks hardware transformation/lighting support");
	//if ((deviceCaps_.DevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY) == 0)
	//	listWarning.push_back("Device can't retrieve textures from system memory");
	if ((deviceCaps_.DevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY) == 0)
		listError.push_back("Device can't retrieve textures from video memory");
	if ((deviceCaps_.DevCaps & D3DDEVCAPS_TLVERTEXSYSTEMMEMORY) == 0)
		listError.push_back("Device can't use buffers in system memory");
	//if ((deviceCaps_.DevCaps & D3DDEVCAPS_TLVERTEXVIDEOMEMORY) == 0)
	//	listError.push_back("Device can't use buffers in video memory");

	if ((deviceCaps_.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE) == 0)
		listError.push_back("D3DPRESENT_INTERVAL_IMMEDIATE is unavailable");
	if ((deviceCaps_.PresentationIntervals & D3DPRESENT_INTERVAL_ONE) == 0)
		listWarning.push_back("V-Sync is unavailable");

	if (!(deviceCaps_.PrimitiveMiscCaps & D3DPMISCCAPS_CULLNONE))
		listWarning.push_back("Device doesn't support culling mode: CULL_NONE");
	if (!(deviceCaps_.PrimitiveMiscCaps & D3DPMISCCAPS_CULLCW))
		listWarning.push_back("Device doesn't support culling mode: CULL_CW");
	if (!(deviceCaps_.PrimitiveMiscCaps & D3DPMISCCAPS_CULLCCW))
		listWarning.push_back("Device doesn't support culling mode: CULL_CCW");
	if ((deviceCaps_.PrimitiveMiscCaps & D3DPMISCCAPS_MASKZ) == 0)
		listWarning.push_back("Device doesn't support depth buffering");
	if ((deviceCaps_.PrimitiveMiscCaps & D3DPMISCCAPS_BLENDOP) == 0)
		listWarning.push_back("Device lacks alpha blending capabilities");
	//if ((deviceCaps_.PrimitiveMiscCaps & D3DPMISCCAPS_PERSTAGECONSTANT) == 0)
	//	listError.push_back("Device doesn't support per-stage blending constants");
	if ((deviceCaps_.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND) == 0)
		listWarning.push_back("Device can't separate blending for the alpha channel");

	if ((deviceCaps_.RasterCaps & D3DPRASTERCAPS_ANISOTROPY) == 0)
		listWarning.push_back("Device doesn't support anisotropic filtering");
	if ((deviceCaps_.RasterCaps & D3DPRASTERCAPS_FOGVERTEX) == 0)
		listWarning.push_back("Device lacks vertex fog support");
	if ((deviceCaps_.RasterCaps & D3DPRASTERCAPS_ZTEST) == 0)
		listWarning.push_back("Device lacks Z-Test support");

	if ((deviceCaps_.ZCmpCaps & D3DPCMPCAPS_LESSEQUAL) == 0)
		listWarning.push_back("Device doesn't support Z-Buffer blending");

	if ((deviceCaps_.SrcBlendCaps & D3DPBLENDCAPS_ONE) == 0
		|| (deviceCaps_.SrcBlendCaps & D3DPBLENDCAPS_ZERO) == 0
		|| (deviceCaps_.SrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) == 0
		|| (deviceCaps_.SrcBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR) == 0)
		listWarning.push_back("Device lacks some blending capabilities (source)");
	if ((deviceCaps_.DestBlendCaps & D3DPBLENDCAPS_ONE) == 0
		|| (deviceCaps_.DestBlendCaps & D3DPBLENDCAPS_ZERO) == 0
		|| (deviceCaps_.DestBlendCaps & D3DPBLENDCAPS_SRCALPHA) == 0
		|| (deviceCaps_.DestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) == 0
		|| (deviceCaps_.DestBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR) == 0)
		listWarning.push_back("Device lacks some blending capabilities (dest)");

	if ((deviceCaps_.AlphaCmpCaps & D3DPCMPCAPS_GREATER) == 0)
		listWarning.push_back("Device might not support alpha-testing");

	if ((deviceCaps_.TextureCaps & D3DPTEXTURECAPS_ALPHA) == 0)
		listError.push_back("Device doesn't support alpha in textures");
	if ((deviceCaps_.TextureCaps & D3DPTEXTURECAPS_POW2) != 0
		&& (deviceCaps_.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) != 0)
		listWarning.push_back("Device might not support textures whose sizes aren't powers of two");
	if ((deviceCaps_.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0)
		listWarning.push_back("Device requires that all textures' sizes must be powers of two");

	if (!(deviceCaps_.TextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT))
		listWarning.push_back("Device doesn't support texture filtering: MAG_POINT");
	if (!(deviceCaps_.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR))
		listWarning.push_back("Device doesn't support texture filtering: MAG_LINEAR");
	if (!(deviceCaps_.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC))
		listWarning.push_back("Device doesn't support texture filtering: MAG_ANISOTROPIC");
	if (!(deviceCaps_.TextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT))
		listWarning.push_back("Device doesn't support texture filtering: MIN_POINT");
	if (!(deviceCaps_.TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR))
		listWarning.push_back("Device doesn't support texture filtering: MIN_LINEAR");
	if (!(deviceCaps_.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC))
		listWarning.push_back("Device doesn't support texture filtering: MIN_ANISOTROPIC");
	if (!(deviceCaps_.TextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT))
		listWarning.push_back("Device doesn't support texture filtering: MIP_POINT");
	if (!(deviceCaps_.TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR))
		listWarning.push_back("Device doesn't support texture filtering: MIP_LINEAR");

	if ((deviceCaps_.TextureAddressCaps & D3DPTADDRESSCAPS_WRAP) == 0)
		listError.push_back("Device doesn't support texture UV wrapping");

	if ((deviceCaps_.TextureOpCaps & D3DTEXOPCAPS_SELECTARG1) == 0
		|| (deviceCaps_.TextureOpCaps & D3DTEXOPCAPS_MODULATE) == 0)
		listWarning.push_back("Device might not support all modes of texture operations");

	if ((deviceCaps_.VertexProcessingCaps & D3DVTXPCAPS_DIRECTIONALLIGHTS) == 0
		|| deviceCaps_.MaxActiveLights < 1)
		listWarning.push_back("Device doesn't support directional lighting");

	if (deviceCaps_.MaxStreams < 2)
		listWarning.push_back("Device might not be able to use hardware instancing");

	if (deviceCaps_.MaxStreamStride < sizeof(VERTEX_TLX))
		listError.push_back("The max stream stride of the device is less than the size of a basic vertex");

	if (deviceCaps_.VertexShaderVersion < D3DVS_VERSION(2, 0)
		|| deviceCaps_.MaxVertexShaderConst < 4)
		listError.push_back("The device's vertex shader support is insufficient (vs_2_0 required)");
	else if (deviceCaps_.VertexShaderVersion < D3DVS_VERSION(3, 0))
		listWarning.push_back("The device's vertex shader support is insufficient (vs_3_0 recommended)");

	if (deviceCaps_.PixelShaderVersion < D3DPS_VERSION(3, 0)
		|| deviceCaps_.PixelShader1xMaxValue < 1.0f)
		listError.push_back("The device's pixel shader support is insufficient (ps_3_0 required)");

	if (deviceCaps_.NumSimultaneousRTs < 1)
		listError.push_back("Device must support at least 1 render target");

	//-------------------------------------------------------------------------------

	if (listError.size() > 0) {
		std::string strAll = "The game cannot start as the\r\n"
			"Direct3D device has the following issue(s):\r\n";
		for (auto& str : listError)
			strAll += "   - " + str + "\r\n";
		strAll += "Try restarting in reference rasterizer mode\r\n";
		throw wexception(strAll);
	}
	else if (listWarning.size() > 0) {
		std::string strAll = "The game's rendering might behave strangely as the\r\n"
			"Direct3D device has the following issue(s):\r\n";
		for (auto& str : listWarning)
			strAll += "   - " + str + "\r\n";
		Logger::WriteTop(strAll);
	}
}

void DirectGraphics::_ReleaseDxResource() {
	ptr_release(pZBuffer_);
	ptr_release(pBackSurf_);

	for (auto itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
		(*itr)->ReleaseDxResource();
	}
}
void DirectGraphics::_RestoreDxResource() {
	pDevice_->GetRenderTarget(0, &pBackSurf_);
	pDevice_->GetDepthStencilSurface(&pZBuffer_);

	for (auto itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
		(*itr)->RestoreDxResource();
	}

	_InitializeDeviceState(true);
}

static int g_restoreFailCount = 0;
bool DirectGraphics::_Restore() {
	//The device was lost, wait until it's able to be restored
	deviceStatus_ = pDevice_->TestCooperativeLevel();
	if (deviceStatus_ == D3D_OK) {
		g_restoreFailCount = 0;
		return true;
	}
	else {
		while ((deviceStatus_ = pDevice_->TestCooperativeLevel()) == D3DERR_DEVICELOST)
			::Sleep(50);

		if (deviceStatus_ == D3DERR_DEVICENOTRESET) {	//The device is now able to be restored
			::InvalidateRect(hAttachedWindow_, nullptr, false);

			_ReleaseDxResource();

			deviceStatus_ = pDevice_->Reset(modeScreen_ == SCREENMODE_FULLSCREEN ? &d3dppFull_ : &d3dppWin_);
			if (SUCCEEDED(deviceStatus_)) {
				_RestoreDxResource();
				Logger::WriteTop("_Restore: IDirect3DDevice restored.");
				g_restoreFailCount = 0;
				return true;
			}
		}
		if (FAILED(deviceStatus_)) {					//Something went wrong
			++g_restoreFailCount;
			if (g_restoreFailCount >= 60) {
				g_restoreFailCount = 0;

				std::wstring err = StringUtility::Format(L"_Restore: Failed to restore the Direct3D device; %s\r\n\t%s",
					DXGetErrorString(deviceStatus_), DXGetErrorDescription(deviceStatus_));
				Logger::WriteTop(err);
				throw gstd::wexception(err);
			}
			else {
				std::wstring err = StringUtility::Format(L"_Restore: Attempt failed; %s\r\n\t%s",
					DXGetErrorString(deviceStatus_), DXGetErrorDescription(deviceStatus_));
				Logger::WriteTop(err);
			}
		}
		return false;
	}
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
				GetRenderScreenWidth() / (float)GetRenderScreenHeight(), 10.0f, 2000.0f);

			viewMat = viewMat * persMat;

			pDevice_->SetTransform(D3DTS_VIEW, &viewMat);
		}
	}

	SetCullingMode(D3DCULL_NONE);
	pDevice_->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	pDevice_->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(192, 192, 192));
	SetLightingEnable(true);
	SetSpecularEnable(false);

	D3DVECTOR dir;
	dir.x = -1;
	dir.y = -1;
	dir.z = -1;
	SetDirectionalLight(dir);

	SetBlendMode(MODE_BLEND_ALPHA);

	SetAlphaTest(true, 0, D3DCMP_GREATER);

	SetZBufferEnable(false);
	SetZWriteEnable(false);

	SetTextureFilter(D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);

	UpdateDefaultRenderTargetSize();
	ResetViewPort();
}
void DirectGraphics::AddDirectGraphicsListener(DirectGraphicsListener* listener) {
	std::list<DirectGraphicsListener*>::iterator itr;
	for (itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
		if ((*itr) == listener) return;
	}
	listListener_.push_back(listener);
}
void DirectGraphics::RemoveDirectGraphicsListener(DirectGraphicsListener* listener) {
	std::list<DirectGraphicsListener*>::iterator itr;
	for (itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
		if ((*itr) != listener) continue;
		listListener_.erase(itr);
		break;
	}
}
void DirectGraphics::BeginScene(bool bMainRender, bool bClear) {
	if (bClear) ClearRenderTarget();
	bMainRender_ = bMainRender;

	if (camera_->thisViewChanged_ || camera_->thisProjectionChanged_) {
		if (camera_->thisViewChanged_) 
			camera_->SetWorldViewMatrix();
		if (camera_->thisProjectionChanged_) 
			camera_->SetProjectionMatrix();
		camera_->UpdateDeviceViewProjectionMatrix();
		camera_->thisViewChanged_ = false;
		camera_->thisProjectionChanged_ = false;
	}

	pDevice_->BeginScene();
}
void DirectGraphics::EndScene(bool bPresent) {
	pDevice_->EndScene();

	if (bPresent) {
		deviceStatus_ = pDevice_->Present(nullptr, nullptr, nullptr, nullptr);
		if (FAILED(deviceStatus_)) {
			if (_Restore()) _InitializeDeviceState(true);
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
void DirectGraphics::ClearRenderTarget(DxRect<LONG>* rect) {
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
	if (previousBlendMode_ == BlendMode::RESET) {
		pDevice_->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_MODULATE);
		pDevice_->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		pDevice_->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
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
	case MODE_BLEND_NONE:		//No blending
		SETBLENDOP(D3DBLENDOP_ADD, FALSE);
		SETBLENDARGS(D3DBLEND_ONE, D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_ZERO);
		break;
	case MODE_BLEND_ALPHA_INV:		//Alpha + Invert
		pDevice_->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT);
	case MODE_BLEND_ALPHA:			//Alpha
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	case MODE_BLEND_ADD_RGB:		//Add - Alpha
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_ONE);
		break;
	case MODE_BLEND_ADD_ARGB:		//Add + Alpha
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	case MODE_BLEND_MULTIPLY:		//Multiply
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_ZERO, D3DBLEND_SRCCOLOR, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	case MODE_BLEND_SUBTRACT:		//Subtract
		SETBLENDOP(D3DBLENDOP_REVSUBTRACT, TRUE);
		SETBLENDARGS(D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	case MODE_BLEND_SHADOW:			//Invert + Multiply
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_ZERO, D3DBLEND_INVSRCCOLOR, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	case MODE_BLEND_INV_DESTRGB:	//Dest invert
		SETBLENDOP(D3DBLENDOP_ADD, TRUE);
		SETBLENDARGS(D3DBLEND_INVDESTCOLOR, D3DBLEND_INVSRCCOLOR, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA);
		break;
	}

	//Reverse Subtract
	//pDevice_->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
	//pDevice_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	//pDevice_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	//Highlight
	//pDevice_->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	//pDevice_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
	//pDevice_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE); 
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
	stateFog_.color = ColorAccess::ToVec4Normalized(color, ColorAccess::PERMUTE_RGBA);
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
	SetViewPort(0, 0, GetRenderScreenWidth(), GetRenderScreenHeight());
}

double DirectGraphics::GetScreenWidthRatio() {
	RECT rect;
	::GetWindowRect(hAttachedWindow_, &rect);

	double widthWindow = rect.right - rect.left;
	double widthView = GetRenderScreenWidth();

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

	double heightWindow = rect.bottom - rect.top;
	double heightView = GetRenderScreenHeight();

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
DxRect<LONG> DirectGraphics::ClientSizeToWindowSize(const DxRect<LONG>& rc, ScreenMode mode) {
	DxRect<LONG> out = rc;
	::AdjustWindowRect((RECT*)&out, mode == SCREENMODE_WINDOW ? wndStyleWin_ : wndStyleFull_, FALSE);
	return out;
}

void DirectGraphics::SaveBackSurfaceToFile(const std::wstring& path) {
	DxRect<LONG> rect(0, 0, GetRenderScreenWidth(), GetRenderScreenHeight());
	LPDIRECT3DSURFACE9 pBackSurface = nullptr;
	pDevice_->GetRenderTarget(0, &pBackSurface);
	D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_BMP,
		pBackSurface, nullptr, (RECT*)&rect);
	pBackSurface->Release();
}

void DirectGraphics::UpdateDefaultRenderTargetSize() {
	size_t baseW = 0;
	size_t baseH = 0;
	if (!config_.bUseDynamicScaling_) {
		baseW = GetScreenWidth();
		baseH = GetScreenHeight();
	}
	else {
		baseW = config_.sizeScreenDisplay_.x;
		baseH = config_.sizeScreenDisplay_.y;
	}

	size_t width = 1U;
	while (width <= baseW)
		width = width << 1;
	size_t height = 1U;
	while (height <= baseH)
		height = height << 1;

	defaultRenderTargetSize_[0] = width;
	defaultRenderTargetSize_[1] = height;
}

//*******************************************************************
//DirectGraphicsPrimaryWindow
//*******************************************************************
DirectGraphicsPrimaryWindow::DirectGraphicsPrimaryWindow() {
	lpCursor_ = nullptr;

	hWndParent_ = nullptr;
	hWndContent_ = nullptr;

	newScreenMode_ = ScreenMode::SCREENMODE_WINDOW;

	bWindowMoveEnable_ = false;
	cPosOffset_ = { 0, 0 };
}
DirectGraphicsPrimaryWindow::~DirectGraphicsPrimaryWindow() {
	SetThreadExecutionState(ES_CONTINUOUS);		//Just in case
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
	bool res =  this->Initialize(config_);
	return res;
}
bool DirectGraphicsPrimaryWindow::Initialize(DirectGraphicsConfig& config) {
	HINSTANCE hInst = ::GetModuleHandle(nullptr);
	lpCursor_ = LoadCursor(nullptr, IDC_ARROW);
	{
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(WNDCLASSEX);
		//		wcex.style=CS_HREDRAW|CS_VREDRAW;
		wcex.lpfnWndProc = (WNDPROC)WindowBase::_StaticWindowProcedure;
		wcex.hInstance = hInst;
		wcex.hIcon = nullptr;
		wcex.hCursor = lpCursor_;
		wcex.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = L"DirectGraphicsPrimaryWindow";
		wcex.hIconSm = nullptr;
		::RegisterClassEx(&wcex);

		DxRect<LONG> wr = ClientSizeToWindowSize(
			{ 0, 0, config.sizeScreen_.x, config.sizeScreen_.y }, SCREENMODE_WINDOW);
		hWnd_ = ::CreateWindowW(wcex.lpszClassName, L"", wndStyleWin_,
			0, 0, wr.GetWidth(), wr.GetHeight(), nullptr, nullptr, hInst, nullptr);

		hWndParent_ = hWnd_;
	}

	if (config.bBorderlessFullscreen_) {
		//Create a child window to handle contents (parent window handles black bars)

		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = (WNDPROC)WindowBase::_StaticWindowProcedure;
		wcex.hInstance = hInst;
		wcex.hCursor = lpCursor_;
		wcex.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
		wcex.lpszClassName = L"DirectGraphicsPrimaryWindow.Child";
		::RegisterClassEx(&wcex);

		LONG screenWidth = config.sizeScreen_.x; //+ ::GetSystemMetrics(SM_CXEDGE) + 10;
		LONG screenHeight = config.sizeScreen_.y; //+ ::GetSystemMetrics(SM_CYEDGE) + 10;

		HWND hWndChild = ::CreateWindowW(wcex.lpszClassName,
			L"",
			WS_CHILD | WS_VISIBLE,
			0, 0, screenWidth, screenHeight, hWnd_, nullptr, hInst, nullptr);
		wndGraphics_.Attach(hWndChild);

		hWndContent_ = hWndChild;
	}
	else {
		if (config.bShowWindow_)
			::ShowWindow(hWnd_, SW_SHOW);
		hWndContent_ = hWnd_;
	}

	::UpdateWindow(hWnd_);
	this->Attach(hWnd_);

	bool res = DirectGraphics::Initialize(hWndContent_, config);
	if (res) {
		ShowCursor(config.bShowCursor_ ? TRUE : FALSE);
		/*
		if (modeScreen_ == SCREENMODE_WINDOW) {
			ChangeScreenMode(SCREENMODE_WINDOW, false);
		}
		*/
	}
	return res;
}

void DirectGraphicsPrimaryWindow::_StartWindowMove(LPARAM lParam) {
	LRESULT region = ::DefWindowProcW(hWnd_, WM_NCHITTEST, 0, lParam);
	if (region == HTCAPTION) {
		bWindowMoveEnable_ = true;

		::GetCursorPos(&cPosOffset_);

		RECT rect;
		GetWindowRect(hWnd_, &rect);
		cPosOffset_.x -= rect.left;
		cPosOffset_.y -= rect.top;

		::SendMessageW(hWnd_, WM_ENTERSIZEMOVE, 0, 0);
	}

	::SetCapture(hWnd_);
}
void DirectGraphicsPrimaryWindow::_StopWindowMove() {
	if (bWindowMoveEnable_) {
		bWindowMoveEnable_ = false;
		::SendMessageW(hWnd_, WM_EXITSIZEMOVE, 0, 0);

		POINT cPos;
		::GetCursorPos(&cPos);
		::ReleaseCapture();

		//If the final pos clips the window into the top of the screen, clamp it down
		if (cPos.y < std::max(0, ::GetSystemMetrics(SM_CYMENUSIZE) - 8)) {
			RECT wRect;
			::GetWindowRect(hWnd_, &wRect);
			::MoveWindow(hWnd_, wRect.left, 0, wRect.right - wRect.left, wRect.bottom - wRect.top, false);
		}
	}
}
void DirectGraphicsPrimaryWindow::_WindowMove() {
	if (bWindowMoveEnable_) {
		POINT cPos;
		::GetCursorPos(&cPos);

		RECT wRect;
		::GetWindowRect(hWnd_, &wRect);

		LONG x = cPos.x - cPosOffset_.x;
		LONG y = cPos.y - cPosOffset_.y;
		::MoveWindow(hWnd_, x, y, wRect.right - wRect.left, wRect.bottom - wRect.top, false);
	}
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
		UINT targetWidth = LOWORD(lParam);
		UINT targetHeight = HIWORD(lParam);

		//Parent window resized with ChangeScreenMode, change the child window's rect
		if (wndGraphics_.GetWindowHandle() != nullptr) {
			RECT rcParent;
			::GetClientRect(hWnd, &rcParent);

			LONG wdParent = rcParent.right - rcParent.left;
			LONG htParent = rcParent.bottom - rcParent.top;

			if (newScreenMode_ == SCREENMODE_WINDOW) {
				//To windowed

				wndGraphics_.SetBounds(0, 0, wdParent, htParent);
			}
			else {
				//To fullscreen

				LONG baseWidth = config_.sizeScreenDisplay_.x;
				LONG baseHeight = config_.sizeScreenDisplay_.y;

				double aspectRatioWH = baseWidth / (double)baseHeight;
				double scalingRatio = std::min(targetWidth / (double)baseWidth, targetHeight / (double)baseHeight);

				LONG newWidth = baseWidth * scalingRatio;
				LONG newHeight = baseHeight * scalingRatio;

				LONG wX = (wdParent - newWidth) / 2L;
				LONG wY = (htParent - newHeight) / 2L;
				wndGraphics_.SetBounds(wX, wY, newWidth, newHeight);
			}
		}

		return FALSE;
	}
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* info = (MINMAXINFO*)lParam;

		int wWidth = ::GetSystemMetrics(SM_CXFULLSCREEN);
		int wHeight = ::GetSystemMetrics(SM_CYFULLSCREEN);

		LONG screenWidth = config_.sizeScreenDisplay_.x;
		LONG screenHeight = config_.sizeScreenDisplay_.y;

		DxRect<LONG> wr = ClientSizeToWindowSize({ 0, 0, screenWidth, screenHeight }, SCREENMODE_WINDOW);

		info->ptMaxSize.x = wr.GetWidth();
		info->ptMaxSize.y = wr.GetHeight();

		return 0;
	}
	case WM_SYSCHAR:
	{
		if (wParam == VK_RETURN)
			this->ChangeScreenMode();
		return FALSE;
	}
	case WM_SYSCOMMAND:
	{
		switch (wParam & 0xfff0) {
		case SC_MAXIMIZE:
			ChangeScreenMode(SCREENMODE_FULLSCREEN);
			return 0;
		}
	}
	}

#if defined(DNH_PROJ_EXECUTOR)
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	if (config->bEnableUnfocusedProcessing_) {
		switch (uMsg) {
		case WM_MOUSEMOVE:
			_WindowMove();
			break;
		case WM_LBUTTONUP:
			_StopWindowMove();
			break;
		case WM_SYSCOMMAND:
		{
			switch (wParam & 0xfff0) {
			case SC_MOVE:
			{
				_StartWindowMove(lParam);
				return 0;
			}
			}
		}
		}
	}
#endif

	return _CallPreviousWindowProcedure(hWnd, uMsg, wParam, lParam);
}

void DirectGraphicsPrimaryWindow::ChangeScreenMode() {
	if (modeScreen_ == SCREENMODE_WINDOW)
		ChangeScreenMode(SCREENMODE_FULLSCREEN);
	else
		ChangeScreenMode(SCREENMODE_WINDOW);
}
void DirectGraphicsPrimaryWindow::ChangeScreenMode(ScreenMode newMode, bool bNoRepeated) {
	if (bNoRepeated && (newMode == modeScreen_)) return;
	newScreenMode_ = newMode;

	//True fullscreen mode
	if (!config_.bBorderlessFullscreen_) {
		Application::GetBase()->SetActive(true);

		_ReleaseDxResource();

		HRESULT hrReset = E_FAIL;

		if (newMode == SCREENMODE_WINDOW) {		//To windowed
			hrReset = pDevice_->Reset(&d3dppWin_);

			::SetWindowLong(hAttachedWindow_, GWL_STYLE, wndStyleWin_);
			::ShowWindow(hAttachedWindow_, SW_SHOW);

			LONG screenWidth = config_.sizeScreenDisplay_.x;
			LONG screenHeight = config_.sizeScreenDisplay_.y;

			DxRect<LONG> wr = ClientSizeToWindowSize({ 0, 0, screenWidth, screenHeight }, SCREENMODE_WINDOW);

			SetBounds(0, 0, wr.GetWidth(), wr.GetHeight());
			MoveWindowCenter(wr.AsRect());

			::SetWindowPos(hAttachedWindow_, HWND_NOTOPMOST, 0, 0, 0, 0,
				SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOSENDCHANGING);
		}
		else {		//To fullscreen
			hrReset = pDevice_->Reset(&d3dppFull_);

			::SetWindowLong(hAttachedWindow_, GWL_STYLE, wndStyleFull_);
			::ShowWindow(hAttachedWindow_, SW_SHOW);
		}

		previousBlendMode_ = BlendMode::RESET;
		if (FAILED(hrReset)) {
			std::wstring err = StringUtility::Format(L"IDirect3DDevice9::Reset: \n%s\n  %s",
				DXGetErrorString(hrReset), DXGetErrorDescription(hrReset));
			throw gstd::wexception(err);
		}

		_RestoreDxResource();

		WindowUtility::SetMouseVisible(config_.bShowCursor_);
		if (!config_.bShowCursor_) {
			::SetCursor(nullptr);
			pDevice_->ShowCursor(false);
		}
		else {
			::SetCursor(lpCursor_);
			pDevice_->ShowCursor(true);
		}
	}
	//Borderless fullscreen mode
	else {
		if (newMode == SCREENMODE_WINDOW) {		//To windowed
			::SetWindowLong(hWnd_, GWL_STYLE, wndStyleWin_);
			::ShowWindow(hWnd_, SW_SHOW);

			LONG screenWidth = config_.sizeScreenDisplay_.x;
			LONG screenHeight = config_.sizeScreenDisplay_.y;

			DxRect<LONG> wr = ClientSizeToWindowSize({ 0, 0, screenWidth, screenHeight }, SCREENMODE_WINDOW);

			SetBounds(0, 0, wr.GetWidth(), wr.GetHeight());
			MoveWindowCenter(wr.AsRect());

			//You can sleep now, 3 hours isn't enough sleep, by the way
			::SetThreadExecutionState(ES_CONTINUOUS);
		}
		else {		//To fullscreen
			RECT rect;
			GetWindowRect(GetDesktopWindow(), &rect);

			::SetWindowLong(hWnd_, GWL_STYLE, wndStyleFull_);
			::ShowWindow(hWnd_, SW_SHOW);

			::MoveWindow(hWnd_, 0, 0, rect.right, rect.bottom, TRUE);

			//Causes fullscreen to prevent Windows drifting off to Dreamland Drama
			::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
		}
	}

	modeScreen_ = newMode;
}

//*******************************************************************
//DxCamera
//*******************************************************************
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
	if (graph == nullptr) return;
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
	if (graph == nullptr) return;
	IDirect3DDevice9* device = graph->GetDevice();

	D3DXMatrixPerspectiveFovLH(&matProjection_, D3DXToRadian(45.0),
		projWidth_ / projHeight_, clipNear_, clipFar_);
	D3DXMatrixInverse(&matProjectionInverse_, nullptr, &matProjection_);

	//UpdateDeviceProjectionMatrix();
}
void DxCamera::UpdateDeviceViewProjectionMatrix() {
	DirectGraphics* graph = DirectGraphics::GetBase();
	if (graph == nullptr) return;
	IDirect3DDevice9* device = graph->GetDevice();

	matViewProjection_ = matView_ * matProjection_;
	device->SetTransform(D3DTS_VIEW, &matViewProjection_);
	device->SetTransform(D3DTS_PROJECTION, &matIdentity_);
}
D3DXVECTOR2 DxCamera::TransformCoordinateTo2D(D3DXVECTOR3 pos) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	LONG width = graphics->GetScreenWidth();
	LONG height = graphics->GetScreenHeight();

	D3DXVECTOR4 vect;
	D3DXVec3Transform(&vect, &pos, &matViewProjection_);

	/*
	if (vect.w > 0) {
		vect.x = width / 2.0f + (vect.x / vect.w) * width / 2.0f;
		vect.y = height / 2.0f - (vect.y / vect.w) * height / 2.0f; // Ｙ方向は上が正となるため
	}
	*/

	return D3DXVECTOR2(vect.x, vect.y);
}
void DxCamera::PushMatrixState() {
	listMatrixState_.push_back(matViewProjection_);
}
void DxCamera::PopMatrixState() {
	if (listMatrixState_.empty()) return;

	DirectGraphics* graph = DirectGraphics::GetBase();
	if (graph == nullptr) return;
	IDirect3DDevice9* device = graph->GetDevice();

	D3DXMATRIX* top = &listMatrixState_.back();

	matViewProjection_ = *top;
	device->SetTransform(D3DTS_VIEW, top);
	device->SetTransform(D3DTS_PROJECTION, &matIdentity_);

	listMatrixState_.pop_back();
}

//*******************************************************************
//DxCamera2D
//*******************************************************************
DxCamera2D::DxCamera2D() {
	pos_.x = 400;
	pos_.y = 300;
	ratioX_ = 1.0f;
	ratioY_ = 1.0f;
	angleZ_ = 0;
	bEnable_ = false;

	posReset_ = nullptr;

	D3DXMatrixIdentity(&matIdentity_);
}
DxCamera2D::~DxCamera2D() {}
void DxCamera2D::Reset() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG width = graphics->GetRenderScreenWidth();
	LONG height = graphics->GetRenderScreenHeight();
	if (posReset_ == nullptr) {
		pos_.x = width / 2;
		pos_.y = height / 2;
	}
	else {
		pos_.x = posReset_->x;
		pos_.y = posReset_->y;
	}
	ratioX_ = 1.0f;
	ratioY_ = 1.0f;

	rcClip_.Set(0, 0, width, height);

	angleZ_ = 0;
}
D3DXVECTOR2 DxCamera2D::GetLeftTopPosition() {
	return GetLeftTopPosition(pos_, ratioX_, ratioY_, rcClip_);
}
D3DXVECTOR2 DxCamera2D::GetLeftTopPosition(const D3DXVECTOR2& focus, float ratio) {
	return GetLeftTopPosition(focus, ratio, ratio);
}
D3DXVECTOR2 DxCamera2D::GetLeftTopPosition(const D3DXVECTOR2& focus, float ratioX, float ratioY) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG width = graphics->GetScreenWidth();
	LONG height = graphics->GetScreenHeight();
	DxRect<LONG> rcClip;
	rcClip.right = width;
	rcClip.bottom = height;
	return GetLeftTopPosition(focus, ratioX, ratioY, rcClip);
}
D3DXVECTOR2 DxCamera2D::GetLeftTopPosition(const D3DXVECTOR2& focus, float ratioX, float ratioY, const DxRect<LONG>& rcClip) {
	LONG width_2 = (rcClip.right - rcClip.left) / 2L;
	LONG height_2 = (rcClip.bottom - rcClip.top) / 2L;

	LONG cen_x = rcClip.left + width_2;
	LONG cen_y = rcClip.top + height_2;

	LONG dx = focus.x - cen_x;
	LONG dy = focus.y - cen_y;

	D3DXVECTOR2 res;
	res.x = cen_x - dx * ratioX;
	res.y = cen_y - dy * ratioY;

	res.x -= width_2 * ratioX;
	res.y -= height_2 * ratioY;

	return res;
}

void DxCamera2D::UpdateMatrix() {
	D3DXVECTOR2 pos = GetLeftTopPosition();

	D3DXMatrixIdentity(&matCamera_);

	if (angleZ_ != 0) {
		float c = cosf(angleZ_);
		float s = sinf(angleZ_);
		float x = GetFocusX() - pos.x;
		float y = GetFocusY() - pos.y;

		__m128 v1 = Vectorize::Mul(
			Vectorize::Set(c, s, s, c),
			Vectorize::Set(x, y, x, y));
		matCamera_._11 = c;
		matCamera_._12 = -s;
		matCamera_._21 = s;
		matCamera_._22 = c;
		matCamera_._41 = -v1.m128_f32[0] - v1.m128_f32[1] + x;
		matCamera_._42 = v1.m128_f32[2] - v1.m128_f32[3] + y;
	}

	matCamera_._11 *= ratioX_;
	matCamera_._22 *= ratioY_;
	matCamera_._41 += pos.x;
	matCamera_._42 += pos.y;
}
#endif
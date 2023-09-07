#include "source/GcLib/pch.h"

#include "GcLibImpl.hpp"
#include "System.hpp"
#include "StgScene.hpp"

#include "../Common/DnhConfiguration.hpp"

//*******************************************************************
//EApplication
//*******************************************************************
EApplication::EApplication() {
	ptrGraphics = nullptr;
}
EApplication::~EApplication() {
}

bool EApplication::_Initialize() {
	ELogger* logger = ELogger::GetInstance();
	Logger::WriteTop("Initializing application.");

	DnhConfiguration* config = DnhConfiguration::CreateInstance();

	EFileManager* fileManager = EFileManager::CreateInstance();
	fileManager->Initialize();

	EFpsController* fpsController = EFpsController::CreateInstance();
	fpsController->SetFastModeRate((size_t)config->fastModeSpeed_ * 60U);
	
	std::wstring appName = L"";

	const std::wstring& configWindowTitle = config->windowTitle_;
	if (configWindowTitle.size() > 0) {
		appName = configWindowTitle;
	}
	else {
		appName = L"東方弾幕風 ph3sx " + DNH_VERSION;
	}
#ifdef _DEBUG
	appName = L"[ph3sx_DEBUG]" + appName;
#endif

	if (!config->bMouseVisible_)
		WindowUtility::SetMouseVisible(false);

	EDirectGraphics* graphics = EDirectGraphics::CreateInstance();
	graphics->Initialize(appName);
	ptrGraphics = graphics;

	//logger->ResetDevice();

	HWND hWndDisplay = graphics->GetParentHWND();
	ErrorDialog::SetParentWindowHandle(hWndDisplay);

	ETextureManager* textureManager = ETextureManager::CreateInstance();
	textureManager->Initialize();

	EShaderManager* shaderManager = EShaderManager::CreateInstance();
	shaderManager->Initialize();

	EMeshManager* meshManager = EMeshManager::CreateInstance();
	meshManager->Initialize();

	EDxTextRenderer* textRenderer = EDxTextRenderer::CreateInstance();
	textRenderer->Initialize();

	EDirectSoundManager* soundManager = EDirectSoundManager::CreateInstance();
	soundManager->Initialize(hWndDisplay);

	EDirectInput* input = EDirectInput::CreateInstance();
	input->Initialize(hWndDisplay);

	ETaskManager* taskManager = ETaskManager::CreateInstance();
	taskManager->Initialize();

	{
		{
			auto infoPanel = logger->GetInfoPanel();
			logger->EAddPanelUpdateData(infoPanel, 50);
		}

		{
			shared_ptr<gstd::TaskInfoPanel> panelTask(new gstd::TaskInfoPanel());
			panelTask->SetDisplayName("Thread");

			// Updated in TaskManager
			if (logger->EAddPanelNoUpdate(panelTask, L"Thread"))
				taskManager->SetInfoPanel(panelTask);
		}

		{
			shared_ptr<directx::TextureInfoPanel> panelTexture(new directx::TextureInfoPanel());
			panelTexture->SetDisplayName("Texture");
			if (logger->EAddPanel(panelTexture, L"Texture", 500))
				textureManager->SetInfoPanel(panelTexture);
		}

		{
			shared_ptr<directx::ShaderInfoPanel> panelShader(new directx::ShaderInfoPanel());
			panelShader->SetDisplayName("Shader");
			if (logger->EAddPanel(panelShader, L"Shader", 1000))
				shaderManager->SetInfoPanel(panelShader);
		}

		{
			shared_ptr<directx::DxMeshInfoPanel> panelMesh(new directx::DxMeshInfoPanel());
			panelMesh->SetDisplayName("Mesh");
			if (logger->EAddPanel(panelMesh, L"Mesh", 1000))
				meshManager->SetInfoPanel(panelMesh);
		}

		{
			shared_ptr<directx::SoundInfoPanel> panelSound(new directx::SoundInfoPanel());
			panelSound->SetDisplayName("Sound");
			// Updated in DirectSoundManager
			if (logger->EAddPanelNoUpdate(panelSound, L"Sound"))
				soundManager->SetInfoPanel(panelSound);
		}

		{
			shared_ptr<gstd::ScriptCommonDataInfoPanel> panelCommonData = logger->GetScriptCommonDataInfoPanel();
			panelCommonData->SetDisplayName("Common Data");

			// Updated in StgSystem
			logger->EAddPanelNoUpdate(panelCommonData, L"Common Data");
		}

		{
			shared_ptr<ScriptInfoPanel> panelScript(new ScriptInfoPanel());
			panelScript->SetDisplayName("Script");
			logger->EAddPanel(panelScript, L"Script", 250);
		}
	}

	logger->LoadState();
	logger->SetWindowVisible(config->bLogWindow_);

	SystemController* systemController = SystemController::CreateInstance();
	systemController->Reset();

	Logger::WriteTop("Application initialized.");

	return true;
}
bool EApplication::_Loop() {
	ELogger* logger = ELogger::GetInstance();
	ETaskManager* taskManager = ETaskManager::GetInstance();
	EFpsController* fpsController = EFpsController::GetInstance();
	EDirectInput* input = EDirectInput::GetInstance();
	EDirectGraphics* graphics = EDirectGraphics::GetInstance();
	DnhConfiguration* config = DnhConfiguration::GetInstance();

	HWND hWndFocused = ::GetForegroundWindow();
	HWND hWndGraphics = graphics->GetWindowHandle();
	HWND hWndLogger = logger->GetWindowHandle();

	bWindowFocused_ = hWndFocused == hWndGraphics || hWndFocused == hWndLogger;
	bool bInputEnable = false;
	if (!config->bEnableUnfocusedProcessing_) {
		if (!bWindowFocused_) {
			//Pause main thread when the window isn't focused
			::Sleep(10);
			return true;
		}
		bInputEnable = true;
	}
	else {
		bInputEnable = bWindowFocused_;
	}

	{
		static uint32_t count = 0;

		auto& [bRenderFrame, bUpdateFrame] = fpsController->Advance();

		if (bUpdateFrame) {
			{
				if (bInputEnable)
					input->Update();
				else input->ClearKeyState();

				if (input->GetKeyState(DIK_LCONTROL) == KEY_HOLD &&
					input->GetKeyState(DIK_LSHIFT) == KEY_HOLD &&
					input->GetKeyState(DIK_R) == KEY_PUSH)
				{
					SystemController* systemController = SystemController::CreateInstance();
					systemController->Reset();
				}
			}

			taskManager->CallWorkFunction();
			taskManager->SetWorkTime(taskManager->GetTimeSpentOnLastFuncCall());

			if (logger->IsWindowVisible()) {
				if (auto infoLog = logger->GetInfoPanel()) {
					std::string fps = StringUtility::Format("Logic: %.2ffps, Render: %.2ffps",
						fpsController->GetCurrentWorkFps(),
						fpsController->GetCurrentRenderFps());
					infoLog->SetInfo(0, "Fps", fps);

					{
						const DirectGraphicsConfig& config = graphics->GetGraphicsConfig();

						//int widthScreen = widthConfig * graphics->GetScreenWidthRatio();
						//int heightScreen = heightConfig * graphics->GetScreenHeightRatio();
						std::string screenInfo = StringUtility::Format("Width: %d/%d, Height: %d/%d",
							graphics->GetRenderScreenWidth(), graphics->GetScreenWidth(),
							graphics->GetRenderScreenHeight(), graphics->GetScreenHeight());
						infoLog->SetInfo(1, "Screen", screenInfo);
					}

					infoLog->SetInfo(2, "Font cache",
						std::to_string(EDxTextRenderer::GetInstance()->GetCacheCount()));
				}

				if (count % 120 == 0) {
					taskManager->ArrangeTask();
				}
				if (count % 10 == 0 && config->fpsType_ == DnhConfiguration::FPS_VARIABLE) {
					fpsController->SetCriticalFrame();
				}
				++count;
			}
		}

		if (bRenderFrame) {
			//graphics->SetAllowRenderTargetChange(false);
			graphics->SetRenderTarget(nullptr);
			graphics->ResetDeviceState();

			graphics->BeginScene(true, true);

			taskManager->CallRenderFunction();
			taskManager->SetRenderTime(taskManager->GetTimeSpentOnLastFuncCall());

			graphics->EndScene(false);

			_RenderDisplay();
		}
	}

	{
		int16_t fastModeKey = fpsController->GetFastModeKey();
		if (input->GetKeyState(fastModeKey) == KEY_HOLD)
			fpsController->SetFastMode(true);
		else if (input->GetKeyState(fastModeKey) == KEY_PULL || input->GetKeyState(fastModeKey) == KEY_FREE)
			fpsController->SetFastMode(false);
	}

	return true;
}
void EApplication::_RenderDisplay() {
	EDirectGraphics* graphics = EDirectGraphics::GetInstance();
	IDirect3DDevice9* device = graphics->GetDevice();

	{
		graphics->SetRenderTargetNull();
		graphics->ResetDeviceState();

		graphics->BeginScene(true, true);

		{
			device->SetFVF(VERTEX_TLX::fvf);

			std::array<VERTEX_TLX, 4> verts;
			auto _Render = [](IDirect3DDevice9* device, VERTEX_TLX* verts, const D3DXMATRIX* mat) {
				constexpr float bias = -0.5f;
				for (size_t iVert = 0; iVert < 4; ++iVert) {
					VERTEX_TLX* vertex = (VERTEX_TLX*)verts + iVert;
					vertex->diffuse_color = 0xffffffff;

					D3DXVECTOR4* vPos = &vertex->position;
					vPos->x += bias;
					vPos->y += bias;

					D3DXVec3TransformCoord((D3DXVECTOR3*)vPos, (D3DXVECTOR3*)vPos, mat);
				}
				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)verts, sizeof(VERTEX_TLX));
			};

			{
				//Render the secondary back buffer first

				float texW = secondaryBackBuffer_->GetWidth();
				float texH = secondaryBackBuffer_->GetHeight();

				verts[0] = VERTEX_TLX(D3DXVECTOR4(0, 0, 0, 1), 0xffffffff,
					D3DXVECTOR2(0, 0));
				verts[1] = VERTEX_TLX(D3DXVECTOR4(texW, 0, 0, 1), 0xffffffff,
					D3DXVECTOR2(1, 0));
				verts[2] = VERTEX_TLX(D3DXVECTOR4(0, texH, 0, 1), 0xffffffff,
					D3DXVECTOR2(0, 1));
				verts[3] = VERTEX_TLX(D3DXVECTOR4(texW, texH, 0, 1), 0xffffffff,
					D3DXVECTOR2(1, 1));

				device->SetTexture(0, secondaryBackBuffer_->GetD3DTexture());
				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)verts.data(), sizeof(VERTEX_TLX));
			}
			{
				//Render the main scene

				DisplaySettings* pDispSettings = graphics->GetDisplaySettings();

				VertexBufferManager* vbManager = VertexBufferManager::GetBase();
				FixedVertexBuffer* vertexBuffer = vbManager->GetVertexBufferTLX();

				UINT scW = graphics->GetScreenWidth(), scH = graphics->GetScreenHeight();
				UINT vpW = graphics->GetRenderScreenWidth(), vpH = graphics->GetRenderScreenHeight();
				/*
				if (graphics->GetScreenMode() == ScreenMode::SCREENMODE_FULLSCREEN) {
					DxRect<LONG> rcWindow = WindowBase::GetActiveMonitorRect(graphics->GetAttachedWindowHandle());
					vpW = rcWindow.GetWidth();
					vpH = rcWindow.GetHeight();
				}
				*/
				//graphics->SetViewPort(0, 0, vpW, vpH);

				shared_ptr<TextureData> mainSceneTexture = graphics->GetDefaultBackBufferRenderTarget();
				D3DXMATRIX matDisplayTransform = pDispSettings->matDisplay;
				shared_ptr<Shader> shader = pDispSettings->shader;

				float texW = mainSceneTexture->GetImageInfo()->Width;
				float texH = mainSceneTexture->GetImageInfo()->Height;

				verts[0] = VERTEX_TLX(D3DXVECTOR4(0, 0, 0, 1), 0xffffffff,
					D3DXVECTOR2(0, 0));
				verts[1] = VERTEX_TLX(D3DXVECTOR4(scW, 0, 0, 1), 0xffffffff,
					D3DXVECTOR2(scW / texW, 0));
				verts[2] = VERTEX_TLX(D3DXVECTOR4(0, scH, 0, 1), 0xffffffff,
					D3DXVECTOR2(0, scH / texH));
				verts[3] = VERTEX_TLX(D3DXVECTOR4(scW, scH, 0, 1), 0xffffffff,
					D3DXVECTOR2(scW / texW, scH / texH));
				{
					constexpr float bias = -0.5f;
					for (size_t iVert = 0; iVert < 4; ++iVert) {
						D3DXVECTOR4* vPos = &verts[iVert].position;
						vPos->x += bias;
						vPos->y += bias;
					}
				}

				device->SetTexture(0, mainSceneTexture->GetD3DTexture());
				if (shader) {
					BufferLockParameter lockParam = BufferLockParameter(D3DLOCK_DISCARD);

					lockParam.SetSource(verts, 4, sizeof(VERTEX_TLX));
					vertexBuffer->UpdateBuffer(&lockParam);

					device->SetStreamSource(0, vertexBuffer->GetBuffer(), 0, sizeof(VERTEX_TLX));
					device->SetVertexDeclaration(
						ShaderManager::GetBase()->GetRenderLib()->GetVertexDeclarationTLX());

					ID3DXEffect* effect = shader->GetEffect();
					if (effect) {
						if (shader->LoadTechnique()) {
							shader->LoadParameter();

							D3DXHANDLE handle = nullptr;
							if (handle = effect->GetParameterBySemantic(nullptr, "WORLD"))
								effect->SetMatrix(handle, &matDisplayTransform);
							if (handle = effect->GetParameterBySemantic(nullptr, "VIEWPROJECTION"))
								effect->SetMatrix(handle, &graphics->GetViewPortMatrix());
							if (handle = effect->GetParameterBySemantic(nullptr, "TEXTURE"))
								effect->SetTexture(handle, mainSceneTexture->GetD3DTexture());
						}

						UINT countPass = 1;
						effect->Begin(&countPass, 0);
						for (UINT iPass = 0; iPass < countPass; ++iPass) {
							effect->BeginPass(iPass);
							device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
							effect->EndPass();
						}
						effect->End();
					}
				}
				else {
					for (size_t iVert = 0; iVert < 4; ++iVert) {
						D3DXVECTOR4* vPos = &verts[iVert].position;
						D3DXVec3TransformCoord((D3DXVECTOR3*)vPos, (D3DXVECTOR3*)vPos, &matDisplayTransform);
					}

					device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)verts.data(), sizeof(VERTEX_TLX));
				}
			}
		}

		graphics->EndScene(true);
		{
			graphics->SetRenderTarget(secondaryBackBuffer_);
			device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
			graphics->SetRenderTarget(nullptr);
		}
	}
}

bool EApplication::_Finalize() {
	Logger::WriteTop("Finalizing application.");

	secondaryBackBuffer_ = nullptr;
	//EDirectGraphics::GetBase()->ResetDisplaySettings();

	SystemController::DeleteInstance();
	ETaskManager::DeleteInstance();
	EFileManager::GetInstance()->EndLoadThread();
	EDirectInput::DeleteInstance();
	EDirectSoundManager::DeleteInstance();
	EDxTextRenderer::DeleteInstance();
	EMeshManager::DeleteInstance();
	EShaderManager::DeleteInstance();
	ETextureManager::DeleteInstance();
	EDirectGraphics::DeleteInstance();
	EFpsController::DeleteInstance();
	EFileManager::DeleteInstance();

	ELogger* logger = ELogger::GetInstance();
	logger->SaveState();
	logger->Close();

	Logger::WriteTop("Application finalized.");
	return true;
}

//*******************************************************************
//EDirectGraphics
//*******************************************************************
EDirectGraphics::EDirectGraphics() {
	defaultWindowTitle_ = L"";
}
EDirectGraphics::~EDirectGraphics() {}
bool EDirectGraphics::Initialize(const std::wstring& windowTitle) {
	defaultWindowTitle_ = windowTitle;

	DnhConfiguration* dnhConfig = DnhConfiguration::GetInstance();
	size_t screenWidth = dnhConfig->screenWidth_;		// From th_dnh.def
	size_t screenHeight = dnhConfig->screenHeight_;		// From th_dnh.def
	ScreenMode screenMode = dnhConfig->modeScreen_;

	size_t windowedWidth = screenWidth;
	size_t windowedHeight = screenHeight;
	{
		std::vector<POINT>& windowSizeList = dnhConfig->windowSizeList_;
		size_t windowSizeIndex = dnhConfig->windowSizeIndex_;
		if (windowSizeIndex < windowSizeList.size()) {
			windowedWidth = windowSizeList[windowSizeIndex].x;
			windowedHeight = windowSizeList[windowSizeIndex].y;
		}
	}

	DirectGraphicsConfig dxConfig;
	dxConfig.sizeScreen = { screenWidth, screenHeight };
	dxConfig.sizeScreenDisplay = { windowedWidth, windowedHeight };
	dxConfig.bShowWindow = true;
	dxConfig.bShowCursor = dnhConfig->bMouseVisible_;
	dxConfig.colorMode = dnhConfig->modeColor_;
	dxConfig.bVSync = dnhConfig->bVSync_;
	dxConfig.bUseRef = dnhConfig->bUseRef_;
	dxConfig.typeMultiSample = dnhConfig->multiSamples_;
	dxConfig.bBorderlessFullscreen = dnhConfig->bPseudoFullscreen_;

	{
		RECT rcMonitor = WindowBase::GetPrimaryMonitorRect();

		LONG monitorWd = rcMonitor.right - rcMonitor.left;
		LONG monitorHt = rcMonitor.bottom - rcMonitor.top;
		/*
		{
			auto rcMonitorClient = ClientSizeToWindowSize(rcMonitor, SCREENMODE_WINDOW);

			//What this does:
			//	1. Imagine a window whose client size is the monitor size
			//	2. Calculate its window size with AdjustWindowRect (or ClientSizeToWindowSize)
			//	3. Subtract the size difference from the original monitor size
			//	4. You have obtained the CLIENT SIZE whose WINDOW SIZE is the monitor size
			monitorWd = monitorWd - (rcMonitorClient.GetWidth() - monitorWd);
			monitorHt = monitorHt - (rcMonitorClient.GetHeight() - monitorHt);
		}
		*/

		if (windowedWidth > monitorWd || windowedHeight > monitorHt) {
			std::wstring msg = StringUtility::Format(L"Your monitor size (usable=%dx%d) is too small "
				"for the game's window size (%dx%d).\n\nAdjust the window size to fit?",
				monitorWd, monitorHt, windowedWidth, windowedHeight);

			int res = ::MessageBoxW(nullptr, msg.c_str(), L"Warning", MB_APPLMODAL | MB_ICONINFORMATION | MB_YESNO);
			if (res == IDYES) {
				//(w1 / h1) = (w2 / h2)
				LONG newWidth, newHeight;
				double aspectRatioWH = windowedWidth / (double)windowedHeight;
				if (windowedWidth > monitorWd) {
					newWidth = monitorWd;
					newHeight = newWidth * (1.0 / aspectRatioWH);
				}
				if (windowedHeight > monitorHt) {	//not else
					newHeight = monitorHt;
					newWidth = newHeight * (aspectRatioWH);
				}
				windowedWidth = newWidth;
				windowedHeight = newHeight;
				
				dxConfig.sizeScreenDisplay = { windowedWidth, windowedHeight };
			}
		}
	}

	bool res = DirectGraphicsPrimaryWindow::Initialize(dxConfig);
	if (res) {
		HWND hWndDisplay = GetParentHWND();
		HICON winIcon = ::LoadIconW(Application::GetApplicationHandle(), MAKEINTRESOURCE(IDI_ICON));

		::SetClassLong(hWndDisplay, GCL_HICON, (LONG)winIcon);
		ELogger::GetInstance()->InsertOpenCommandInSystemMenu(hWndDisplay);

		SetWindowTitle(windowTitle);

		ChangeScreenMode(screenMode, false);
		SetWindowVisible(true);
	}

	return res;
}
void EDirectGraphics::SetRenderStateFor2D(BlendMode type) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetBlendMode(type);
	graphics->SetZBufferEnable(false);
	graphics->SetZWriteEnable(false);
}
LRESULT EDirectGraphics::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_SYSCOMMAND:
	{
		int nId = wParam & 0xffff;
		if (nId == ELogger::MY_SYSCMD_OPEN)
			ELogger::GetInstance()->ShowLogWindow();
		break;
	}
	}
	return DirectGraphicsPrimaryWindow::_WindowProcedure(hWnd, uMsg, wParam, lParam);
}

void EDirectGraphics::SetWindowTitle(const std::wstring& title) {
	HWND hWndDisplay = GetParentHWND();
	::SetWindowTextW(hWndDisplay, (title.size() > 0) 
		? title.c_str() : defaultWindowTitle_.c_str());
}

#include "source/GcLib/pch.h"

#include "GcLibImpl.hpp"
#include "System.hpp"
#include "StgScene.hpp"

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
	fpsController->SetFastModeRate((size_t)config->GetSkipModeSpeedRate() * 60U);
	
	std::wstring appName = L"";

	const std::wstring& configWindowTitle = config->GetWindowTitle();
	if (configWindowTitle.size() > 0) {
		appName = configWindowTitle;
	}
	else {
		appName = L"東方弾幕風 ph3sx " + DNH_VERSION;
	}
#ifdef _DEBUG
	appName = L"[ph3sx_DEBUG]" + appName;
#endif

	if (!config->IsMouseVisible())
		WindowUtility::SetMouseVisible(false);

	EDirectGraphics* graphics = EDirectGraphics::CreateInstance();
	graphics->Initialize(appName);
	ptrGraphics = graphics;

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
		logger->EAddPanel(logger->GetInfoPanel(), L"Info", 500);

		shared_ptr<gstd::TaskInfoPanel> panelTask(new gstd::TaskInfoPanel());
		//Updated in TaskManager
		if (logger->AddPanel(panelTask, L"Thread"))
			taskManager->SetInfoPanel(panelTask);

		shared_ptr<directx::TextureInfoPanel> panelTexture(new directx::TextureInfoPanel());
		if (logger->EAddPanel(panelTexture, L"Texture", 500))
			textureManager->SetInfoPanel(panelTexture);

		shared_ptr<directx::ShaderInfoPanel> panelShader(new directx::ShaderInfoPanel());
		if (logger->EAddPanel(panelShader, L"Shader", 1000))
			shaderManager->SetInfoPanel(panelShader);

		shared_ptr<directx::DxMeshInfoPanel> panelMesh(new directx::DxMeshInfoPanel());
		if (logger->EAddPanel(panelMesh, L"Mesh", 1000))
			meshManager->SetInfoPanel(panelMesh);

		shared_ptr<directx::SoundInfoPanel> panelSound(new directx::SoundInfoPanel());
		//Updated in DirectSoundManager
		if (logger->AddPanel(panelSound, L"Sound"))
			soundManager->SetInfoPanel(panelSound);

		shared_ptr<gstd::ScriptCommonDataInfoPanel> panelCommonData = logger->GetScriptCommonDataInfoPanel();
		//Updated in StgSystem
		logger->AddPanel(panelCommonData, L"Common Data");

		shared_ptr<ScriptInfoPanel> panelScript(new ScriptInfoPanel());
		logger->EAddPanel(panelScript, L"Script", 250);
	}

	logger->LoadState();
	logger->SetWindowVisible(config->IsLogWindow());

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
	if (!config->IsEnableUnfocusedProcessing()) {
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
				std::wstring fps = StringUtility::Format(L"Logic: %.2ffps, Render: %.2ffps",
					fpsController->GetCurrentWorkFps(),
					fpsController->GetCurrentRenderFps());
				logger->SetInfo(0, L"Fps", fps);

				const POINT& screenSize = graphics->GetConfigData().sizeScreen_;
				const POINT& screenSizeWindowed = graphics->GetConfigData().sizeScreenDisplay_;
				//int widthScreen = widthConfig * graphics->GetScreenWidthRatio();
				//int heightScreen = heightConfig * graphics->GetScreenHeightRatio();
				std::wstring screenInfo = StringUtility::Format(L"Width: %d/%d, Height: %d/%d",
					screenSizeWindowed.x, screenSize.x,
					screenSizeWindowed.y, screenSize.y);
				logger->SetInfo(1, L"Screen", screenInfo);

				logger->SetInfo(2, L"Font cache",
					StringUtility::Format(L"%d", EDxTextRenderer::GetInstance()->GetCacheCount()));
			}

			if (count % 120 == 0) {
				taskManager->ArrangeTask();
			}
			if (count % 10 == 0 && config->GetFpsType() == DnhConfiguration::FPS_VARIABLE) {
				fpsController->SetCriticalFrame();
			}
			++count;
		}

		if (bRenderFrame) {
			graphics->BeginScene();

			taskManager->CallRenderFunction();
			taskManager->SetRenderTime(taskManager->GetTimeSpentOnLastFuncCall());

			graphics->EndScene();
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
bool EApplication::_Finalize() {
	Logger::WriteTop("Finalizing application.");
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
	LONG screenWidth = dnhConfig->GetScreenWidth();		//From th_dnh.def
	LONG screenHeight = dnhConfig->GetScreenHeight();	//From th_dnh.def
	ScreenMode screenMode = dnhConfig->GetScreenMode();

	LONG windowedWidth = screenWidth;
	LONG windowedHeight = screenHeight;
	{
		std::vector<POINT>& windowSizeList = dnhConfig->GetWindowSizeList();
		size_t windowSizeIndex = dnhConfig->GetWindowSize();
		if (windowSizeIndex < windowSizeList.size()) {
			windowedWidth = std::max(windowSizeList[windowSizeIndex].x, 320L);
			windowedHeight = std::max(windowSizeList[windowSizeIndex].y, 240L);
		}
	}

	DirectGraphicsConfig dxConfig;
	dxConfig.sizeScreen_ = { screenWidth, screenHeight };
	dxConfig.sizeScreenDisplay_ = { windowedWidth, windowedHeight };
	dxConfig.bShowWindow_ = true;
	dxConfig.bShowCursor_ = dnhConfig->IsMouseVisible();
	dxConfig.colorMode_ = dnhConfig->GetColorMode();
	dxConfig.bVSync_ = dnhConfig->IsEnableVSync();
	dxConfig.bUseRef_ = dnhConfig->IsEnableRef();
	dxConfig.typeMultiSample_ = dnhConfig->GetMultiSampleType();
	dxConfig.bBorderlessFullscreen_ = dnhConfig->bPseudoFullscreen_;
	dxConfig.bUseDynamicScaling_ = dnhConfig->UseDynamicScaling();

	{
		RECT rcMonitor;
		::GetWindowRect(::GetDesktopWindow(), &rcMonitor);

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
				dxConfig.sizeScreenDisplay_ = { windowedWidth, windowedHeight };
			}
		}
	}

	bool res = DirectGraphicsPrimaryWindow::Initialize(dxConfig);
	if (res) {
		HWND hWndDisplay = GetParentHWND();
		HICON winIcon = ::LoadIconW(Application::GetApplicationHandle(), MAKEINTRESOURCE(IDI_ICON));

		::SetClassLong(hWndDisplay, GCL_HICON, (LONG)winIcon);
		WindowLogger::InsertOpenCommandInSystemMenu(hWndDisplay);

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
		if (nId == WindowLogger::MENU_ID_OPEN)
			ELogger::GetInstance()->ShowLogWindow();
		break;
	}
	}
	return DirectGraphicsPrimaryWindow::_WindowProcedure(hWnd, uMsg, wParam, lParam);
}

void EDirectGraphics::SetWindowTitle(const std::wstring& title) {
	HWND hWndDisplay = GetParentHWND();
	::SetWindowText(hWndDisplay, (title.size() > 0) 
		? title.c_str() : defaultWindowTitle_.c_str());
}

#include "source/GcLib/pch.h"

#include "GcLibImpl.hpp"
#include "System.hpp"
#include "StgScene.hpp"

/**********************************************************
//EApplication
**********************************************************/
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

	/*
#if defined(GAME_VERSION_TCL)
	std::wstring appName = L"東方宝天京　～ Treasure Castle Labyrinth ";
#elif defined(GAME_VERSION_SP)
	std::wstring appName = L"東方潮聖書　～ Sapphire Panlogism ";
#else
	std::wstring appName = L"東方弾幕風 ph3sx ";
#endif
	*/
	std::wstring appName = L"東方弾幕風 ph3sx ";
	appName += DNH_VERSION;

	const std::wstring& configWindowTitle = config->GetWindowTitle();
	if (configWindowTitle.size() > 0)
		appName = configWindowTitle;

	//マウス表示
	if (!config->IsMouseVisible())
		WindowUtility::SetMouseVisible(false);

	//DirectX初期化
	EDirectGraphics* graphics = EDirectGraphics::CreateInstance();
	graphics->Initialize();
	HWND hWndMain = graphics->GetWindowHandle();
	WindowLogger::InsertOpenCommandInSystemMenu(hWndMain);
	::SetWindowText(hWndMain, appName.c_str());
	::SetClassLong(hWndMain, GCL_HICON, (LONG)LoadIcon(GetApplicationHandle(), MAKEINTRESOURCE(IDI_ICON)));

	ptrGraphics = graphics;

	ErrorDialog::SetParentWindowHandle(hWndMain);

	ETextureManager* textureManager = ETextureManager::CreateInstance();
	textureManager->Initialize();

	EShaderManager* shaderManager = EShaderManager::CreateInstance();
	shaderManager->Initialize();

	EMeshManager* meshManager = EMeshManager::CreateInstance();
	meshManager->Initialize();

	EDxTextRenderer* textRenderer = EDxTextRenderer::CreateInstance();
	textRenderer->Initialize();

	EDirectSoundManager* soundManager = EDirectSoundManager::CreateInstance();
	soundManager->Initialize(hWndMain);

	EDirectInput* input = EDirectInput::CreateInstance();
	input->Initialize(hWndMain);

	ETaskManager* taskManager = ETaskManager::CreateInstance();
	taskManager->Initialize();

	gstd::ref_count_ptr<gstd::TaskInfoPanel> panelTask = new gstd::TaskInfoPanel();
	bool bAddTaskPanel = logger->AddPanel(panelTask, L"Thread");
	if (bAddTaskPanel) taskManager->SetInfoPanel(panelTask);

	gstd::ref_count_ptr<directx::TextureInfoPanel> panelTexture = new directx::TextureInfoPanel();
	bool bTexturePanel = logger->AddPanel(panelTexture, L"Texture");
	if (bTexturePanel) textureManager->SetInfoPanel(panelTexture);

	gstd::ref_count_ptr<directx::DxMeshInfoPanel> panelMesh = new directx::DxMeshInfoPanel();
	bool bMeshPanel = logger->AddPanel(panelMesh, L"Mesh");
	if (bMeshPanel) meshManager->SetInfoPanel(panelMesh);

	gstd::ref_count_ptr<directx::SoundInfoPanel> panelSound = new directx::SoundInfoPanel();
	bool bSoundPanel = logger->AddPanel(panelSound, L"Sound");
	if (bSoundPanel) soundManager->SetInfoPanel(panelSound);

	gstd::ref_count_ptr<gstd::ScriptCommonDataInfoPanel> panelCommonData = logger->GetScriptCommonDataInfoPanel();
	logger->AddPanel(panelCommonData, L"Common Data");

	gstd::ref_count_ptr<ScriptInfoPanel> panelScript = new ScriptInfoPanel();
	logger->AddPanel(panelScript, L"Script");

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
	EDirectGraphics* graphics = EDirectGraphics::GetInstance();

	HWND hWndFocused = ::GetForegroundWindow();
	HWND hWndGraphics = graphics->GetWindowHandle();
	HWND hWndLogger = ELogger::GetInstance()->GetWindowHandle();
	if (hWndFocused != hWndGraphics && hWndFocused != hWndLogger) {
		//非アクティブ時は動作しない
		::Sleep(10);
		return true;
	}

	EDirectInput* input = EDirectInput::GetInstance();
	input->Update();
	if (input->GetKeyState(DIK_LCONTROL) == KEY_HOLD &&
		input->GetKeyState(DIK_LSHIFT) == KEY_HOLD &&
		input->GetKeyState(DIK_R) == KEY_PUSH) {
		//リセット
		SystemController* systemController = SystemController::CreateInstance();
		systemController->Reset();
	}

	//bool bSaveRT = input->GetKeyState(DIK_P) == KEY_HOLD;

	{
		taskManager->CallWorkFunction();
		if (!fpsController->IsSkip()) {
			graphics->BeginScene();
			taskManager->CallRenderFunction();
			graphics->EndScene();
		}

		fpsController->Wait();
	}

	//ログ関連
	if (logger->IsWindowVisible()) {
		std::wstring fps = StringUtility::Format(L"Work: %.2ffps, Draw: %.2ffps",
			fpsController->GetCurrentWorkFps(),
			fpsController->GetCurrentRenderFps());
		logger->SetInfo(0, L"Fps", fps);

		const POINT& screenSize = graphics->GetConfigData().GetScreenSize();
		const POINT& screenSizeWindowed = graphics->GetConfigData().GetScreenWindowedSize();
		//int widthScreen = widthConfig * graphics->GetScreenWidthRatio();
		//int heightScreen = heightConfig * graphics->GetScreenHeightRatio();
		std::wstring screenInfo = StringUtility::Format(L"Width: %d/%d, Height: %d/%d",
			screenSizeWindowed.x, screenSize.x,
			screenSizeWindowed.y, screenSize.y);
		logger->SetInfo(1, L"Screen", screenInfo);

		logger->SetInfo(2, L"Font cache",
			StringUtility::Format(L"%d", EDxTextRenderer::GetInstance()->GetCacheCount()));
	}

	//高速動作
	int16_t fastModeKey = fpsController->GetFastModeKey();
	if (input->GetKeyState(fastModeKey) == KEY_HOLD)
		fpsController->SetFastMode(true);
	else if (input->GetKeyState(fastModeKey) == KEY_PULL || input->GetKeyState(fastModeKey) == KEY_FREE)
		fpsController->SetFastMode(false);

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




/**********************************************************
//EDirectGraphics
**********************************************************/
EDirectGraphics::EDirectGraphics() {

}
EDirectGraphics::~EDirectGraphics() {}
bool EDirectGraphics::Initialize() {
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
	dxConfig.SetScreenSize({ screenWidth, screenHeight });
	dxConfig.SetScreenWindowedSize({ windowedWidth, windowedHeight });
	dxConfig.SetShowWindow(true);
	dxConfig.SetShowCursor(dnhConfig->IsMouseVisible());
	dxConfig.SetColorMode(dnhConfig->GetColorMode());
	dxConfig.SetVSyncEnable(dnhConfig->IsEnableVSync());
	dxConfig.SetReferenceEnable(dnhConfig->IsEnableRef());
	dxConfig.SetMultiSampleType(dnhConfig->GetMultiSampleType());
	dxConfig.SetbPseudoFullScreen(dnhConfig->bPseudoFullscreen_);

	bool res = DirectGraphicsPrimaryWindow::Initialize(dxConfig);
	if (!res) return res;

	/*
	int monitorWidth = ::GetSystemMetrics(SM_CXFULLSCREEN);
	int monitorHeight = ::GetSystemMetrics(SM_CYFULLSCREEN);
	bool bFullScreenEnable = screenWidth <= monitorWidth && screenHeight <= monitorHeight;

	//コンフィグ反映
	if (screenMode == ScreenMode::SCREENMODE_FULLSCREEN && bFullScreenEnable) {
		ChangeScreenMode();
	}
	else {
		RECT wr = { 0, 0, windowedWidth, windowedHeight };
		AdjustWindowRect(&wr, wndStyleWin_, FALSE);

		SetBounds(0, 0, wr.right - wr.left, wr.bottom - wr.top);
		MoveWindowCenter();
	}
	*/
	if (screenMode == ScreenMode::SCREENMODE_FULLSCREEN) {
		ChangeScreenMode();
	}
	else {
		RECT wr = { 0, 0, windowedWidth, windowedHeight };
		AdjustWindowRect(&wr, wndStyleWin_, FALSE);

		SetBounds(0, 0, wr.right - wr.left, wr.bottom - wr.top);
		MoveWindowCenter();
	}

	SetWindowVisible(true);
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

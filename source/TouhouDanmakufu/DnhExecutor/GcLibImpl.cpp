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

#if defined(GAME_VERSION_TCL)
	std::wstring appName = L"東方宝天京　～ Treasure Castle Labyrinth ";
#elif defined(GAME_VERSION_SP)
	std::wstring appName = L"東方潮聖書　～ Sapphire Panlogism ";
#else
	std::wstring appName = L"東方弾幕風 ph3sx ";
#endif
	appName += DNH_VERSION;

	std::wstring configWindowTitle = config->GetWindowTitle();
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
	if (bAddTaskPanel)taskManager->SetInfoPanel(panelTask);

	gstd::ref_count_ptr<directx::TextureInfoPanel> panelTexture = new directx::TextureInfoPanel();
	bool bTexturePanel = logger->AddPanel(panelTexture, L"Texture");
	if (bTexturePanel)textureManager->SetInfoPanel(panelTexture);

	gstd::ref_count_ptr<directx::DxMeshInfoPanel> panelMesh = new directx::DxMeshInfoPanel();
	bool bMeshPanel = logger->AddPanel(panelMesh, L"Mesh");
	if (bMeshPanel)meshManager->SetInfoPanel(panelMesh);

	gstd::ref_count_ptr<directx::SoundInfoPanel> panelSound = new directx::SoundInfoPanel();
	bool bSoundPanel = logger->AddPanel(panelSound, L"Sound");
	if (bSoundPanel)soundManager->SetInfoPanel(panelSound);

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

		int widthConfig = graphics->GetConfigData().GetScreenWidth();
		int heightConfig = graphics->GetConfigData().GetScreenHeight();
		int widthScreen = widthConfig * graphics->GetScreenWidthRatio();
		int heightScreen = heightConfig * graphics->GetScreenHeightRatio();
		std::wstring screen = StringUtility::Format(L"Width: %d/%d, Height: %d/%d",
			widthScreen, widthConfig,
			heightScreen, heightConfig);
		logger->SetInfo(1, L"Screen", screen);

		logger->SetInfo(2, L"Font cache",
			StringUtility::Format(L"%d", EDxTextRenderer::GetInstance()->GetCacheCount()));
	}

	//高速動作
	int fastModeKey = fpsController->GetFastModeKey();
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
	int screenWidth = dnhConfig->GetScreenWidth();
	int screenHeight = dnhConfig->GetScreenHeight();
	int screenMode = dnhConfig->GetScreenMode();
	int windowSize = dnhConfig->GetWindowSize();

	bool bUserSize = screenWidth != 640 || screenHeight != 480;
	if (!bUserSize) {
		if (windowSize == DnhConfiguration::WINDOW_SIZE_640x480 && screenWidth > 640)
			windowSize = DnhConfiguration::WINDOW_SIZE_800x600;
		if (windowSize == DnhConfiguration::WINDOW_SIZE_800x600 && screenWidth > 800)
			windowSize = DnhConfiguration::WINDOW_SIZE_960x720;
		if (windowSize == DnhConfiguration::WINDOW_SIZE_960x720 && screenWidth > 960)
			windowSize = DnhConfiguration::WINDOW_SIZE_1280x960;
		if (windowSize == DnhConfiguration::WINDOW_SIZE_1280x960 && screenWidth > 1280)
			windowSize = DnhConfiguration::WINDOW_SIZE_1600x1200;
		if (windowSize == DnhConfiguration::WINDOW_SIZE_1600x1200 && screenWidth > 1600)
			windowSize = DnhConfiguration::WINDOW_SIZE_1920x1200;
	}


	bool bShowWindow = screenMode == DirectGraphics::SCREENMODE_FULLSCREEN ||
		windowSize == DnhConfiguration::WINDOW_SIZE_640x480;

	DirectGraphicsConfig dxConfig;
	dxConfig.SetScreenWidth(screenWidth);
	dxConfig.SetScreenHeight(screenHeight);
	dxConfig.SetShowWindow(bShowWindow);
	dxConfig.SetShowCursor(dnhConfig->IsMouseVisible());
	dxConfig.SetColorMode(dnhConfig->GetColorMode());
	dxConfig.SetVSyncEnable(dnhConfig->IsEnableVSync());
	dxConfig.SetReferenceEnable(dnhConfig->IsEnableRef());
	dxConfig.SetMultiSampleType(dnhConfig->GetMultiSampleType());
	dxConfig.SetbPseudoFullScreen(dnhConfig->bPseudoFullscreen_);
	bool res = DirectGraphicsPrimaryWindow::Initialize(dxConfig);

	int monitorWidth = ::GetSystemMetrics(SM_CXFULLSCREEN);
	int monitorHeight = ::GetSystemMetrics(SM_CYFULLSCREEN);
	bool bFullScreenEnable = screenWidth <= monitorWidth && screenHeight <= monitorHeight;

	//コンフィグ反映
	if (screenMode == DirectGraphics::SCREENMODE_FULLSCREEN && bFullScreenEnable) {
		ChangeScreenMode();
	}
	else {
		if (windowSize != DnhConfiguration::WINDOW_SIZE_640x480 || bUserSize) {
			int width = screenWidth;
			int height = screenHeight;
			if (!bUserSize) {
				switch (windowSize) {
				case DnhConfiguration::WINDOW_SIZE_800x600:
					width = 800; height = 600;
					break;
				case DnhConfiguration::WINDOW_SIZE_960x720:
					width = 960; height = 720;
					break;
				case DnhConfiguration::WINDOW_SIZE_1280x960:
					width = 1280; height = 960;
					break;
				case DnhConfiguration::WINDOW_SIZE_1600x1200:
					width = 1600; height = 1200;
					break;
				case DnhConfiguration::WINDOW_SIZE_1920x1200:
					width = 1920; height = 1200;
					break;
				}
			}

			RECT wr = { 0, 0, width, height };
			AdjustWindowRect(&wr, wndStyleWin_, FALSE);

			SetBounds(0, 0, wr.right - wr.left, wr.bottom - wr.top);
			MoveWindowCenter();
		}
	}
	SetWindowVisible(true);

	return res;
}
void EDirectGraphics::SetRenderStateFor2D(int blend) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetBlendMode(blend);
	graphics->SetZBufferEnable(false);
	graphics->SetZWriteEnable(false);
}
LRESULT EDirectGraphics::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_SYSCOMMAND:
	{
		int nId = wParam & 0xffff;
		if (nId == WindowLogger::MENU_ID_OPEN) {
			ELogger::GetInstance()->ShowLogWindow();
		}
		break;
	}
	}
	return DirectGraphicsPrimaryWindow::_WindowProcedure(hWnd, uMsg, wParam, lParam);
}

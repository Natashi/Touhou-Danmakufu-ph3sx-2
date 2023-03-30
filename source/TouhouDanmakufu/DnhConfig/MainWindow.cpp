#include "source/GcLib/pch.h"

#include "MainWindow.hpp"
#include "GcLibImpl.hpp"

#include <shlobj.h>

#pragma push_macro("new")
#undef new
#include <imgui_internal.h>
#pragma pop_macro("new")

static const std::wstring CONFIG_VERSION_STR = WINDOW_TITLE + L" " + DNH_VERSION;

//*******************************************************************
//DxGraphicsConfigurator
//*******************************************************************
DxGraphicsConfigurator::DxGraphicsConfigurator() {
	ZeroMemory(&d3dpp_, sizeof(d3dpp_));
}
DxGraphicsConfigurator::~DxGraphicsConfigurator() {
}

void DxGraphicsConfigurator::_ReleaseDxResource() {
	ImGui_ImplDX9_InvalidateDeviceObjects();
	DirectGraphicsBase::_ReleaseDxResource();
}
void DxGraphicsConfigurator::_RestoreDxResource() {
	ImGui_ImplDX9_CreateDeviceObjects();
	DirectGraphicsBase::_RestoreDxResource();
}
bool DxGraphicsConfigurator::_Restore() {
	deviceStatus_ = pDevice_->TestCooperativeLevel();
	if (deviceStatus_ == D3D_OK) {
		return true;
	}
	else if (deviceStatus_ == D3DERR_DEVICENOTRESET) {
		ResetDevice();
		return SUCCEEDED(deviceStatus_);
	}
	return false;
}

std::vector<std::wstring> DxGraphicsConfigurator::_GetRequiredModules() {
	return std::vector<std::wstring>({
		L"d3d9.dll",
	});
}

bool DxGraphicsConfigurator::Initialize(HWND hWnd) {
	_LoadModules();

	Logger::WriteTop("DirectGraphics: Initialize.");
	pDirect3D_ = Direct3DCreate9(D3D_SDK_VERSION);
	if (pDirect3D_ == nullptr) throw gstd::wexception("Direct3DCreate9 error.");

	hAttachedWindow_ = hWnd;

	pDirect3D_->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &deviceCaps_);
	_VerifyDeviceCaps();

	d3dpp_.hDeviceWindow = hWnd;
	d3dpp_.Windowed = TRUE;
	d3dpp_.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp_.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp_.EnableAutoDepthStencil = TRUE;
	d3dpp_.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp_.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	{
		HRESULT hrDevice = E_FAIL;
		auto _TryCreateDevice = [&](D3DDEVTYPE type, DWORD addFlag) {
			hrDevice = pDirect3D_->CreateDevice(D3DADAPTER_DEFAULT, type, hWnd,
				addFlag | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &d3dpp_, &pDevice_);
		};

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
				L"Cannot create Direct3D device. [%s]\r\n  %s",
				DXGetErrorString(hrDevice), DXGetErrorDescription(hrDevice));
			throw wexception(err);
		}
	}

	pDevice_->GetRenderTarget(0, &pBackSurf_);
	pDevice_->GetDepthStencilSurface(&pZBuffer_);

	Logger::WriteTop("DirectGraphics: Initialized.");
	return true;
}
void DxGraphicsConfigurator::Release() {
	DirectGraphicsBase::Release();
}

bool DxGraphicsConfigurator::BeginScene(bool bClear) {
	pDevice_->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice_->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	pDevice_->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	pDevice_->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	HRESULT hr = pDevice_->BeginScene();
	if (FAILED(hr)) {
		const wchar_t* str = DXGetErrorStringW(hr);
		const wchar_t* desc = DXGetErrorDescriptionW(hr);
	}
	return SUCCEEDED(hr);
}
void DxGraphicsConfigurator::EndScene(bool bPresent) {
	pDevice_->EndScene();

	deviceStatus_ = pDevice_->Present(nullptr, nullptr, nullptr, nullptr);
	if (FAILED(deviceStatus_)) {
		_Restore();
	}
}

void DxGraphicsConfigurator::ResetDevice() {
	_ReleaseDxResource();
	deviceStatus_ = pDevice_->Reset(&d3dpp_);
	if (SUCCEEDED(deviceStatus_)) {
		_RestoreDxResource();
	}
}

//*******************************************************************
//MainWindow
//*******************************************************************
MainWindow::MainWindow() {
	bInitialized_ = false;

	pIo_ = nullptr;
	dpi_ = USER_DEFAULT_SCREEN_DPI;
}
MainWindow::~MainWindow() {
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	dxGraphics_->Release();
	::UnregisterClassW(L"WC_Configurator", ::GetModuleHandleW(nullptr));
}

bool MainWindow::Initialize() {
	dxGraphics_.reset(new DxGraphicsConfigurator());
	dxGraphics_->SetSize(560, 600);

	HINSTANCE hInst = ::GetModuleHandleW(nullptr);

	{
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_CLASSDC;
		wcex.lpfnWndProc = (WNDPROC)WindowBase::_StaticWindowProcedure;
		wcex.hInstance = hInst;
		wcex.lpszClassName = L"WC_Configurator";
		::RegisterClassExW(&wcex);

		hWnd_ = ::CreateWindowW(wcex.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
			0, 0, 0, 0, nullptr, nullptr, hInst, nullptr);
	}

	SetBounds(0, 0, dxGraphics_->GetWidth(), dxGraphics_->GetHeight());
	{
		RECT drect, mrect;
		::GetWindowRect(::GetDesktopWindow(), &drect);
		::GetWindowRect(hWnd_, &mrect);

		MoveWindowCenter(hWnd_, drect, mrect);
	}

	SetWindowTextW(hWnd_, CONFIG_VERSION_STR.c_str());

	dxGraphics_->Initialize(hWnd_);

	this->Attach(hWnd_);
	::ShowWindow(hWnd_, SW_SHOWDEFAULT);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	pIo_ = &ImGui::GetIO();
	pIo_->IniFilename = nullptr;	//Disable default save/load of imgui.ini

	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hWnd_);
	ImGui_ImplDX9_Init(dxGraphics_->GetDevice());

	ImGui_ImplWin32_EnableDpiAwareness();

	_SetImguiStyle(1);
	_ResetFont();

	bInitialized_ = true;
	//Start();

	return true;
}

void MainWindow::InitializePanels() {
	listPanels_.push_back(std::make_unique<DevicePanel>());
	listPanels_.push_back(std::make_unique<KeyPanel>());

	auto panelOption = std::make_unique<OptionPanel>();
	pPanelOption_ = panelOption.get();
	listPanels_.push_back(std::move(panelOption));

	for (auto& iPanel : listPanels_)
		iPanel->Initialize();

	_LoadConfig();

	pCurrentPanel_ = nullptr;
}

void MainWindow::_LoadConfig() {
	for (auto& iPanel : listPanels_)
		iPanel->LoadConfiguration();
}
void MainWindow::_SaveConfig() {
	for (auto& iPanel : listPanels_)
		iPanel->SaveConfiguration();

	DnhConfiguration* config = DnhConfiguration::GetInstance();
	config->SaveConfigFile();
}
void MainWindow::_ClearConfig() {
	for (auto& iPanel : listPanels_) {
		iPanel->DefaultSettings();
		iPanel->SaveConfiguration();
	}

	DnhConfiguration* config = DnhConfiguration::GetInstance();
	config->SaveConfigFile();
}

void MainWindow::_ResetFont() {
	if (mapSystemFontPath_.size() == 0) {
		std::vector<std::wstring> fonts = { L"Arial" };
		for (auto& i : fonts) {
			auto path = SystemUtility::GetSystemFontFilePath(i);
			mapSystemFontPath_[i] = StringUtility::ConvertWideToMulti(path);
		}
	}

	if (pIo_ == nullptr || ImGui::GetCurrentContext() == nullptr) return;

	float scale = SystemUtility::DpiToScalingFactor(dpi_);

	pIo_->Fonts->Clear();
	mapFont_.clear();

	{
		auto pathArial = mapSystemFontPath_[L"Arial"].c_str();

		for (auto fSize : { 8, 14, 16, 20, 24 }) {
			std::string key = StringUtility::Format("Arial%d", fSize);
			mapFont_[key] = pIo_->Fonts->AddFontFromFileTTF(pathArial, fSize * scale);
		}

		{
			static const ImWchar rangesEx[] = {
				0x0020, 0x00FF,		// ASCII
				0x2190, 0x2199,		// Arrows
				0,
			};
			for (auto fSize : { 18, 20 }) {
				std::string key = StringUtility::Format("Arial%d_Ex", fSize);
				mapFont_[key] = pIo_->Fonts->AddFontFromFileTTF(pathArial, fSize * scale, 
					nullptr, rangesEx);
			}
		}
	}

	pIo_->Fonts->Build();
}
void MainWindow::_ResetDevice() {
	_ResetFont();
	dxGraphics_->ResetDevice();
}

void MainWindow::_Resize(float scale) {
	RECT rc;
	::GetWindowRect(hWnd_, &rc);

	UINT wd = rc.right - rc.left;
	UINT ht = rc.bottom - rc.top;
	wd *= scale;
	ht *= scale;

	::MoveWindow(hWnd_, rc.left, rc.left, wd, ht, true);
	MoveWindowCenter();

	dxGraphics_->SetSize(wd, ht);
	_ResetDevice();
}
void MainWindow::_SetImguiStyle(float scale) {
	if (ImGui::GetCurrentContext() == nullptr)
		return;

	ImGuiStyle style;
	ImGui::StyleColorsLight(&style);

	style.ChildBorderSize = style.FrameBorderSize = style.PopupBorderSize
		= style.TabBorderSize = style.WindowBorderSize = 1.0f;
	style.ChildRounding = style.FrameRounding = style.GrabRounding
		= style.PopupRounding = style.ScrollbarRounding
		= style.TabRounding = style.WindowRounding = 0.0f;
	style.ScaleAllSizes(scale);

	ImVec4 lightBlue(0.62f, 0.80f, 1.00f, 1.00f);
	ImVec4 darkBlue(0.06f, 0.53f, 0.98f, 1.00f);

	style.Colors[ImGuiCol_Button] = ImVec4(0.73f, 0.73f, 0.73f, 0.40f);
	style.Colors[ImGuiCol_ButtonHovered] = lightBlue;
	style.Colors[ImGuiCol_ButtonActive] = darkBlue;

	style.Colors[ImGuiCol_TabActive] = ImVec4(0.70f, 0.70f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_TabHovered] = lightBlue;

	style.Colors[ImGuiCol_ScrollbarGrabHovered] = lightBlue;
	style.Colors[ImGuiCol_FrameBgHovered] = lightBlue;
	style.Colors[ImGuiCol_FrameBgActive] = darkBlue;
	//style.Colors[] = ;

	memcpy(defaultStyleColors, style.Colors, sizeof(defaultStyleColors));

	ImGui::GetStyle() = style;
}

bool MainWindow::Loop() {
	MSG msg;
	while (::PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);

		if (msg.message == WM_QUIT)
			return false;
	}
	return bRun_;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT MainWindow::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	switch (uMsg) {
	case WM_CLOSE:
		::DestroyWindow(hWnd);
		bRun_ = false;
		return 0;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		/*
		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(hWnd, &ps);
		::FillRect(hdc, &ps.rcPaint, 0);
		::EndPaint(hWnd, &ps);
		*/
		_Update();
		return 0;
	}

	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* minmax = (MINMAXINFO*)lParam;
		minmax->ptMinTrackSize.x = 16 * 2 + 136 * 3 + ImGui::GetStyle().ItemSpacing.x * 2 + 4;
		minmax->ptMinTrackSize.y = 320;
		minmax->ptMaxTrackSize.x = ::GetSystemMetrics(SM_CXMAXTRACK);
		minmax->ptMaxTrackSize.y = ::GetSystemMetrics(SM_CYMAXTRACK);
		return 0;
	}
	case WM_SIZE:
	{
		if (dxGraphics_->GetDevice() != nullptr && wParam != SIZE_MINIMIZED) {
			dxGraphics_->SetSize(LOWORD(lParam), HIWORD(lParam));
			_ResetDevice();
		}
		return 0;
	}
	case WM_DPICHANGED:
	{
		dpi_ = LOWORD(wParam);
		_Resize(SystemUtility::DpiToScalingFactor(dpi_));
		return 0;
	}
	case WM_SYSCOMMAND:
	{
		if ((wParam & 0xfff0) == SC_KEYMENU)	// Disable default alt menu
			return 0;
		break;
	}
	}
	return _CallPreviousWindowProcedure(hWnd, uMsg, wParam, lParam);
}

void MainWindow::_Update() {
	for (auto& iPanel : listPanels_)
		iPanel->Update();

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();
	_ProcessGui();
	ImGui::EndFrame();

	if (dxGraphics_->BeginScene(true)) {
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}
	dxGraphics_->EndScene(true);
}

void MainWindow::_ProcessGui() {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	static bool bPopup_Help, bPopup_Version;
	bPopup_Help = false;
	bPopup_Version = false;

	ImGui::PushFont(mapFont_["Arial16"]);

	auto _Exit = [&]() {
		::SendMessageW(dxGraphics_->GetAttachedWindowHandle(), WM_CLOSE, 0, 0);
	};

	if (ImGui::Begin("MainWindow", nullptr, flags)) {
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Menu")) {
				if (ImGui::MenuItem("Save and Exit")) {
					_SaveConfig();
					_Exit();
				}
				if (ImGui::MenuItem("Exit without Saving", "Alt+F4")) {
					_Exit();
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Start Game")) {
					if (_StartGame()) {
						_Exit();
					}
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Restore tab default")) {
					if (pCurrentPanel_)
						pCurrentPanel_->DefaultSettings();
				}
				if (ImGui::MenuItem("Restore all defaults")) {
					for (auto& iPanel : listPanels_)
						iPanel->DefaultSettings();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		float ht = ImGui::GetContentRegionAvail().y - 56;

		//ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGuiWindowFlags window_flags = 0;
		if (ImGui::BeginChild("ChildW_Tabs", ImVec2(0, ht), false, window_flags)) {
			ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton;
			if (ImGui::BeginTabBar("TabBar_Tabs", tab_bar_flags)) {
				for (auto& iPanel : listPanels_) {
					if (ImGui::BeginTabItem(iPanel->GetTabName().c_str())) {
						pCurrentPanel_ = iPanel.get();
						iPanel->ProcessGui();

						ImGui::EndTabItem();
					}
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::EndChild();

		{
			ImGui::Dummy(ImVec2(0, 16));

			ImVec2 buttonSize(136, 28);
			if (ImGui::Button("Save and Exit", buttonSize)) {
				_SaveConfig();
				_Exit();
			}
			ImGui::SameLine();
			if (ImGui::Button("Exit without Saving", buttonSize)) {
				_Exit();
			}
			ImGui::SameLine();
			if (ImGui::Button("Start Game", buttonSize)) {
				if (_StartGame()) {
					_Exit();
				}
			}
		}

		ImGui::End();
	}
	
	ImGui::PopFont();
}

bool MainWindow::_StartGame() {
	std::wstring exePath = ((OptionPanel*)pPanelOption_)->GetExecutablePath();

	if (exePath.size() > 0) {
		PROCESS_INFORMATION infoProcess;
		ZeroMemory(&infoProcess, sizeof(infoProcess));

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(STARTUPINFO);

		BOOL res = ::CreateProcessW(nullptr, (wchar_t*)exePath.c_str(),
			nullptr, nullptr, true, 0, nullptr, nullptr,
			&si, &infoProcess);
		/*
		if (res == 0) {
			std::wstring log = StringUtility::Format(L"Could not start the game. [%s]\r\n", ErrorUtility::GetLastErrorMessage().c_str());
			Logger::WriteTop(log);
			return;
		}
		*/

		::CloseHandle(infoProcess.hProcess);
		::CloseHandle(infoProcess.hThread);

		return res != 0;
	}

	return false;
}

//*******************************************************************
//DevicePanel
//*******************************************************************
DevicePanel::DevicePanel() {
	tabName_ = "Display";
}
DevicePanel::~DevicePanel() {
}

bool DevicePanel::Initialize() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();

	for (auto& p : config->windowSizeList_) {
		listWindowSize_.push_back(p);
	}

	{
		DxGraphicsConfigurator* graphics = MainWindow::GetInstance()->GetDxGraphics();
		IDirect3D9* pD3D = graphics->GetDirect3D();

		std::vector<std::pair<D3DMULTISAMPLE_TYPE, const char*>> listMsaaAll = {
			{ D3DMULTISAMPLE_2_SAMPLES, "MSAA 2x"},
			{ D3DMULTISAMPLE_3_SAMPLES, "MSAA 3x"},
			{ D3DMULTISAMPLE_4_SAMPLES, "MSAA 4x"},
			{ D3DMULTISAMPLE_5_SAMPLES, "MSAA 5x"},
			{ D3DMULTISAMPLE_6_SAMPLES, "MSAA 6x"},
			{ D3DMULTISAMPLE_7_SAMPLES, "MSAA 7x"},
			{ D3DMULTISAMPLE_8_SAMPLES, "MSAA 8x"},
		};

		// No multisampling is always supported
		listMSAA_.push_back({ D3DMULTISAMPLE_NONE, "None (Default)",
			true, true });

		for (auto& iMsaaPair : listMsaaAll) {
			auto _Check = [&pD3D](D3DMULTISAMPLE_TYPE msaa, bool windowed) {
				HRESULT hrBack32 = pD3D->CheckDeviceMultiSampleType(
					D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
					windowed, msaa, nullptr);
				HRESULT hrBack16 = pD3D->CheckDeviceMultiSampleType(
					D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_R5G6B5,
					windowed, msaa, nullptr);
				HRESULT hrDepth = pD3D->CheckDeviceMultiSampleType(
					D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_D16,
					windowed, msaa, nullptr);
				return (SUCCEEDED(hrBack32) || SUCCEEDED(hrBack16)) 
					&& SUCCEEDED(hrDepth);
			};

			bool bFullscreen = _Check(iMsaaPair.first, false);
			bool bWindowed = _Check(iMsaaPair.first, true);

			if (bFullscreen || bWindowed) {
				listMSAA_.push_back({ iMsaaPair.first, iMsaaPair.second, 
					bWindowed, bFullscreen });
			}
		}
	}

	DefaultSettings();

	return true;
}
void DevicePanel::DefaultSettings() {
	selectedWindowSize_ = ScreenMode::SCREENMODE_WINDOW;
	selectedMSAA_ = 0;		// No multisampling

	selectedRefreshRate_ = DnhConfiguration::FPS_NORMAL; 
	selectedColorMode_ = ColorMode::COLOR_MODE_32BIT;

	checkEnableVSync_ = true;
	checkBorderlessFullscreen_ = true;
}

void DevicePanel::LoadConfiguration() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();

	selectedScreenMode_ = config->modeScreen_ == ScreenMode::SCREENMODE_FULLSCREEN
		? 0 : 1;

	selectedWindowSize_ = std::min(config->windowSizeIndex_, listWindowSize_.size());

	selectedMSAA_ = 0;
	for (int i = 0; i < listMSAA_.size(); ++i) {
		if (listMSAA_[i].msaa == config->multiSamples_) {
			selectedMSAA_ = i;
			break;
		}
	}

	switch (config->fpsType_) {
	case DnhConfiguration::FPS_1_2:
		selectedRefreshRate_ = 1; break;
	case DnhConfiguration::FPS_1_3:
		selectedRefreshRate_ = 2; break;
	case DnhConfiguration::FPS_VARIABLE:
		selectedRefreshRate_ = 3; break;
	default:
		selectedRefreshRate_ = 0;
	}

	selectedColorMode_ = config->modeColor_ == ColorMode::COLOR_MODE_32BIT ? 0 : 1;

	checkEnableVSync_ = config->bVSync_;
	checkBorderlessFullscreen_ = config->bPseudoFullscreen_;
}
void DevicePanel::SaveConfiguration() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();

	config->modeScreen_ = selectedScreenMode_ == 0 ?
		ScreenMode::SCREENMODE_FULLSCREEN : ScreenMode::SCREENMODE_WINDOW;

	config->windowSizeIndex_ = selectedWindowSize_;

	config->multiSamples_ = D3DMULTISAMPLE_NONE;
	if (selectedMSAA_ < listMSAA_.size())
		config->multiSamples_ = listMSAA_[selectedMSAA_].msaa;

	switch (selectedRefreshRate_) {
	case 1:
		config->fpsType_ = DnhConfiguration::FPS_1_2; break;
	case 2:
		config->fpsType_ = DnhConfiguration::FPS_1_3; break;
	case 3:
		config->fpsType_ = DnhConfiguration::FPS_VARIABLE; break;
	default:
		config->fpsType_ = DnhConfiguration::FPS_NORMAL;
	}

	config->modeColor_ = selectedColorMode_ == 0 ?
		ColorMode::COLOR_MODE_32BIT : ColorMode::COLOR_MODE_16BIT;

	config->bVSync_ = checkEnableVSync_;
	config->bUseRef_ = false;
	config->bPseudoFullscreen_ = checkBorderlessFullscreen_;
}

static ImVector<ImRect> s_GroupPanelLabelStack;
static void ImGuiExt_BeginGroupPanel(const char* name, const ImVec2& size = ImVec2(0.0f, 0.0f));
static void ImGuiExt_EndGroupPanel();

void DevicePanel::ProcessGui() {
	float wd = ImGui::GetContentRegionAvail().x;

	{
		ImGuiExt_BeginGroupPanel("Display", ImVec2(wd - 2, 0.0f));
		ImGui::Dummy(ImVec2(0, 2));

		auto _WinSizeToStr = [&](int idx) -> std::string {
			if (idx >= listWindowSize_.size())
				return "";
			const POINT& pt = listWindowSize_[idx];
			return StringUtility::Format("%dx%d", pt.x, pt.y);
		};

		{
			ImGui::PushItemWidth(300 - ImGui::GetCursorPosX());

			ImGuiComboFlags comboFlags = 0;
			if (ImGui::BeginCombo("Window Size", _WinSizeToStr(selectedWindowSize_).c_str(), comboFlags)) {
				for (int i = 0; i < listWindowSize_.size(); ++i) {
					bool bSelected = i == selectedWindowSize_;
					if (ImGui::Selectable(_WinSizeToStr(i).c_str(), bSelected))
						selectedWindowSize_ = i;

					if (bSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::PopItemWidth();
		}

		ImGui::Dummy(ImVec2(0, 2));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0, 2));

		{
			ImGui::RadioButton("Fullscreen", &selectedScreenMode_, ScreenMode::SCREENMODE_FULLSCREEN);
			ImGui::SameLine();
			ImGui::RadioButton("Windowed", &selectedScreenMode_, ScreenMode::SCREENMODE_WINDOW);
		}

		ImGui::NewLine();

		ImGui::Checkbox("Use Borderless Windowed in Fullscreen Mode", &checkBorderlessFullscreen_);
		ImGui::Checkbox("Enable VSync in Exclusive Fullscreen", &checkEnableVSync_);

		ImGui::Dummy(ImVec2(0, 1));
		ImGuiExt_EndGroupPanel();
	}

	ImGui::NewLine();

	{
		ImGuiExt_BeginGroupPanel("Graphics", ImVec2(wd - 2, 0.0f));
		ImGui::Dummy(ImVec2(0, 1));

		float wd2 = ImGui::GetContentRegionAvail().x - 2;

		{
			ImGuiExt_BeginGroupPanel("Anti-Aliasing", ImVec2(wd2, 0.0f));
			ImGui::Dummy(ImVec2(0, 1));

			ImGui::PushItemWidth(300 - ImGui::GetCursorPosX());

			ImGuiComboFlags comboFlags = 0;
			if (ImGui::BeginCombo("##antialiaslist", listMSAA_[selectedMSAA_].name, comboFlags)) {
				for (int i = 0; i < listMSAA_.size(); ++i) {
					bool bSelected = i == selectedMSAA_;
					if (ImGui::Selectable(listMSAA_[i].name, bSelected))
						selectedMSAA_ = i;

					if (bSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::PopItemWidth();

			ImGui::Dummy(ImVec2(0, 2));
			ImGuiExt_EndGroupPanel();
		}

		//ImGui::NewLine();

		{
			ImGuiExt_BeginGroupPanel("Color Mode", ImVec2(wd2, 0.0f));

			ImGui::RadioButton("32-bit (Recommended)", &selectedColorMode_, ColorMode::COLOR_MODE_32BIT);
			ImGui::SameLine();
			ImGui::RadioButton("16-bit", &selectedColorMode_, ColorMode::COLOR_MODE_16BIT);

			ImGuiExt_EndGroupPanel();
		}

		//ImGui::NewLine();

		{
			ImGuiExt_BeginGroupPanel("Frameskip", ImVec2(wd2, 0.0f));

			ImGui::RadioButton("None (Recommended)", &selectedRefreshRate_, DnhConfiguration::FPS_NORMAL);
			ImGui::SameLine();
			ImGui::RadioButton("1/2", &selectedRefreshRate_, DnhConfiguration::FPS_1_2);
			ImGui::SameLine();
			ImGui::RadioButton("1/3", &selectedRefreshRate_, DnhConfiguration::FPS_1_3);
			ImGui::SameLine();
			ImGui::RadioButton("Automatic", &selectedRefreshRate_, DnhConfiguration::FPS_VARIABLE);

			ImGuiExt_EndGroupPanel();
		}

		ImGui::Dummy(ImVec2(0, 2));
		ImGuiExt_EndGroupPanel();
	}
}

//*******************************************************************
//KeyPanel
//*******************************************************************
KeyPanel::KeyPanel() {
	tabName_ = "Input";
}
KeyPanel::~KeyPanel() {
}

bool KeyPanel::Initialize() {
	mapAction_[EDirectInput::KEY_LEFT] = KeyBindData("Left");
	mapAction_[EDirectInput::KEY_RIGHT] = KeyBindData("Right");
	mapAction_[EDirectInput::KEY_UP] = KeyBindData("Up");
	mapAction_[EDirectInput::KEY_DOWN] = KeyBindData("Down");
	
	mapAction_[EDirectInput::KEY_OK] = KeyBindData("Accept");
	mapAction_[EDirectInput::KEY_CANCEL] = KeyBindData("Cancel");
	
	mapAction_[EDirectInput::KEY_SHOT] = KeyBindData("Shot");
	mapAction_[EDirectInput::KEY_BOMB] = KeyBindData("Bomb");
	mapAction_[EDirectInput::KEY_SLOWMOVE] = KeyBindData("Focus");
	mapAction_[EDirectInput::KEY_USER1] = KeyBindData("User 1");
	mapAction_[EDirectInput::KEY_USER2] = KeyBindData("User 2");

	mapAction_[EDirectInput::KEY_PAUSE] = KeyBindData("Pause");

	ControllerMap::Initialize();

	_RescanDevices();
	DefaultSettings();

	return true;
}
void KeyPanel::DefaultSettings() {
	EDirectInput* input = EDirectInput::GetInstance();

	mapAction_[EDirectInput::KEY_LEFT].SetKeys(DIK_LEFT, DirectInput::PAD_D_LEFT);
	mapAction_[EDirectInput::KEY_RIGHT].SetKeys(DIK_RIGHT, DirectInput::PAD_D_RIGHT);
	mapAction_[EDirectInput::KEY_UP].SetKeys(DIK_UP, DirectInput::PAD_D_UP);
	mapAction_[EDirectInput::KEY_DOWN].SetKeys(DIK_DOWN, DirectInput::PAD_D_DOWN);
	
	mapAction_[EDirectInput::KEY_OK].SetKeys(DIK_Z, DirectInput::PAD_0);
	mapAction_[EDirectInput::KEY_CANCEL].SetKeys(DIK_X, DirectInput::PAD_1);
	
	mapAction_[EDirectInput::KEY_SHOT].SetKeys(DIK_Z, DirectInput::PAD_0);
	mapAction_[EDirectInput::KEY_BOMB].SetKeys(DIK_X, DirectInput::PAD_1);
	mapAction_[EDirectInput::KEY_SLOWMOVE].SetKeys(DIK_LSHIFT, DirectInput::PAD_2);
	mapAction_[EDirectInput::KEY_USER1].SetKeys(DIK_C, DirectInput::PAD_3);
	mapAction_[EDirectInput::KEY_USER2].SetKeys(DIK_V, DirectInput::PAD_4);

	mapAction_[EDirectInput::KEY_PAUSE].SetKeys(DIK_ESCAPE, DirectInput::PAD_5);

	padResponse_ = 500;
	_SetPadResponse();
}

void KeyPanel::LoadConfiguration() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	EDirectInput* input = EDirectInput::GetInstance();

	for (auto& itr : mapAction_) {
		int action = itr.first;

		ref_count_ptr<VirtualKey> vk = config->GetVirtualKey(action);
		if (vk) {
			mapAction_[action].SetKeys(vk->GetKeyCode(), 0, vk->GetPadButton());
		}
	}

	padResponse_ = config->padResponse_;
	_SetPadResponse();
}
void KeyPanel::SaveConfiguration() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();

	for (auto& itr : mapAction_) {
		int action = itr.first;

		ref_count_ptr<VirtualKey> vk = config->GetVirtualKey(action);
		if (vk) {
			vk->SetKeyCode(itr.second.keyboardButton);
			vk->SetPadIndex(itr.second.padDevice);
			vk->SetPadButton(itr.second.padButton);
		}
	}

	config->padResponse_ = padResponse_;
}

void KeyPanel::_SetPadResponse() {
	EDirectInput* input = EDirectInput::GetInstance();
	for (size_t iPad = 0; iPad < input->GetPadDeviceCount(); ++iPad) {
		DirectInput::JoypadInputDevice* pJoypad = const_cast<DirectInput::JoypadInputDevice*>(
			input->GetPadDevice((int16_t)iPad));
		pJoypad->responseThreshold = padResponse_;
	}
}
void KeyPanel::_RescanDevices() {
	EDirectInput* input = EDirectInput::GetInstance();

	input->RefreshInputDevices();

	size_t deviceCount = input->GetPadDeviceCount();

	listPadDevice_.resize(deviceCount);
	listPadDeviceKeyMap_.resize(deviceCount);
	for (size_t iPad = 0; iPad < deviceCount; ++iPad) {
		const DirectInput::JoypadInputDevice* pJoypad = input->GetPadDevice((int16_t)iPad);
		listPadDevice_[iPad] = pJoypad;
		listPadDeviceKeyMap_[iPad] = ControllerMap::GetControllerMap(&pJoypad->idh);
	}

	selectedPadDevice_ = 0;
	bPadRefreshing_ = true;
}

void KeyPanel::Update() {
	EDirectInput* input = EDirectInput::GetInstance();
	input->Update();
}

static stdch::milliseconds GetCurrentMs() {
	auto sysNow = stdch::system_clock::now();
	return stdch::duration_cast<stdch::milliseconds>(
		sysNow.time_since_epoch());
};

void KeyPanel::ProcessGui() {
	MainWindow* pMainWindow = MainWindow::GetInstance();

	float wd = ImGui::GetContentRegionAvail().x;

	{
		ImGuiExt_BeginGroupPanel("Pad Devices", ImVec2(wd - 2, 0.0f));
		ImGui::Dummy(ImVec2(0, 2));

		ImGuiStyle& style = ImGui::GetStyle();

		ImVec2 buttonSize(150, 22);

		{
			float wd2 = ImGui::GetContentRegionAvail().x;

			ImVec2 itemSize(wd2 - buttonSize.x - style.ItemSpacing.x * 2 + 2, buttonSize.y);

			auto _GetPadName = [&](int idx) -> std::string {
				if (idx >= listPadDevice_.size())
					return "(No pad device detected)";
				const DIDEVICEINSTANCE* ddi = &listPadDevice_[idx]->idh.ddi;
				return StringUtility::ConvertWideToMulti(ddi->tszProductName);
			};

			bool bPadAvailable = listPadDevice_.size() > 0;

			if (!bPadAvailable) ImGui::BeginDisabled();
			{
				ImGui::PushItemWidth(itemSize.x);

				ImGuiComboFlags comboFlags = 0;
				if (ImGui::BeginCombo("##paddevicelist", _GetPadName(selectedPadDevice_).c_str(), comboFlags)) {
					for (int i = 0; i < listPadDevice_.size(); ++i) {
						bool bSelected = i == selectedPadDevice_;
						if (ImGui::Selectable(_GetPadName(i).c_str(), bSelected))
							selectedPadDevice_ = i;

						if (bSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::PopItemWidth();
			}
			if (!bPadAvailable) ImGui::EndDisabled();
		}

		ImGui::SameLine();

		if (ImGui::Button("Refresh Input Devices", buttonSize)) {
			_RescanDevices();
		}

		ImGui::Dummy(ImVec2(0, 1));
		ImGuiExt_EndGroupPanel();
	}

	ImGui::Dummy(ImVec2(0, 2));

	{
		EDirectInput* input = EDirectInput::GetInstance();

		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton;
		if (ImGui::BeginTabBar("TabBar_Key_Tabs", tab_bar_flags)) {
			// Do not move this, _RescanDevices may happen mid-way between this function (??)
			bool bPadAvailable = listPadDevice_.size() > 0;

			const ControllerMap* pControllerKeyMap = selectedPadDevice_ < listPadDeviceKeyMap_.size() ?
				listPadDeviceKeyMap_[selectedPadDevice_] : nullptr;

			if (ImGui::BeginTabItem("Key Config", nullptr, bPadRefreshing_ ? ImGuiTabItemFlags_SetSelected : 0)) {
				ImVec2 rgn = ImGui::GetContentRegionAvail();
				if (ImGui::BeginChild("ChildW_Key_Test_Buttons", ImVec2(rgn.x, rgn.y), false, 0)) {
					struct TmpPopupData1 {
						bool bInit = true;
						bool bOpen = false;
						bool bKeyboard;

						int action;
						KeyBindData* pKeyBindData;

						bool bAwaitingInput;
						stdch::milliseconds startTime;
						int16_t pressedKey;
					};
					static TmpPopupData1 popupData1;

					//------------------------------------------------------------------------

					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(16, 16, 192, 255));
					ImGui::Text("Controller button layout: %s", 
						!bPadAvailable ? "(No pad device detected)" : (pControllerKeyMap 
							? pControllerKeyMap->name : "(Unknown controller model)"));
					ImGui::PopStyleColor();

					ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 2.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 4));
					ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, IM_COL32(224, 224, 255, 255));
					ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, IM_COL32(8, 8, 8, 255));
					ImGui::PushStyleColor(ImGuiCol_TableBorderLight, IM_COL32(128, 128, 128, 255));

					ImGui::PushFont(pMainWindow->GetFontMap()["Arial18_Ex"]);

					ImGuiTableFlags table_flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter
						| ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersInnerH;
					if (ImGui::BeginTable("Table_ActionKeys", 3, table_flags)) {
						ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_NoSort
							| ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 96);
						ImGui::TableSetupColumn("Keyboard", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("Pad", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableHeadersRow();

						int iAction = 0;
						for (auto& iAction : mapAction_) {
							int action = iAction.first;
							KeyBindData& keyBindData = iAction.second;
							const std::string& actionName = keyBindData.name;

							ImGui::TableNextRow();

							ImGui::TableSetColumnIndex(0);
							ImGui::Dummy(ImVec2(0, 1));
							ImGui::Text(actionName.c_str());

							ImVec2 buttonSize(-1, 28);
							auto _CalcColor = [&pMainWindow](int64_t timeDiff) {
								const ImVec4& orgColor = pMainWindow->defaultStyleColors[ImGuiCol_Button];
								float rate = std::clamp<float>(
									timeDiff / (float)KeyBindData::FADE_DURATION, 0, 1);
								return IM_COL32(
									Math::Lerp::Smooth<int>(64, orgColor.x * 255, rate),
									Math::Lerp::Smooth<int>(128, orgColor.y * 255, rate),
									Math::Lerp::Smooth<int>(255, orgColor.z * 255, rate),
									Math::Lerp::Smooth<int>(255, orgColor.w * 255, rate));
							};

							ImGui::TableSetColumnIndex(1);
							{
								/*
								// Check duplicated buttons
								bool bDuplicate = false;
								for (auto& [action2, keybindData2] : mapAction_) {
									if (action == action2) continue;
									if (keyBindData.keyboardButton == keybindData2.keyboardButton) {
										bDuplicate = true;

										ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 192, 192, 255));

										break;
									}
								}
								*/

								int64_t timerDiff = (GetCurrentMs() - keyBindData.fadeTimer[0]).count();
								bool bColor = timerDiff < KeyBindData::FADE_DURATION;
								if (bColor) {
									ImU32 color = _CalcColor(timerDiff);
									ImGui::PushStyleColor(ImGuiCol_Button, color);
									ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
								}

								std::string bindKeyboardStr = listKeyCode_.GetCodeText(keyBindData.keyboardButton);
								std::string btnText = StringUtility::Format("%s##%02d",
									bindKeyboardStr.c_str(), action);
								//std::string btnText = StringUtility::Format("%d##%d", cTimer, action);
								if (ImGui::Button(btnText.c_str(), buttonSize)) {
									popupData1.bInit = true;
									popupData1.bOpen = true;
									popupData1.bKeyboard = true;
									popupData1.action = action;
									popupData1.pKeyBindData = &keyBindData;
								}

								if (bColor)
									ImGui::PopStyleColor(2);
								//if (bDuplicate)
								//	ImGui::PopStyleColor();
							}

							ImGui::TableSetColumnIndex(2);
							if (!bPadAvailable) ImGui::BeginDisabled();
							{
								int16_t bindPad = keyBindData.padButton;

								std::string padButtonName = ControllerMap::GetNameFromButton(
									pControllerKeyMap, bindPad);

								if (padButtonName.size() == 0)
									padButtonName = StringUtility::Format("Button%02d", bindPad - 8);

								int64_t timerDiff = (GetCurrentMs() - keyBindData.fadeTimer[1]).count();
								bool bColor = timerDiff < KeyBindData::FADE_DURATION;
								if (bColor) {
									ImU32 color = _CalcColor(timerDiff);
									ImGui::PushStyleColor(ImGuiCol_Button, color);
									ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
								}

								std::string btnText = StringUtility::Format("%s##pad%02d",
									padButtonName.c_str(), action);
								if (ImGui::Button(btnText.c_str(), buttonSize)) {
									popupData1.bInit = true;
									popupData1.bOpen = true;
									popupData1.bKeyboard = false;
									popupData1.action = action;
									popupData1.pKeyBindData = &keyBindData;
								}

								if (bColor) {
									ImGui::PopStyleColor(2);
								}
							}
							if (!bPadAvailable) ImGui::EndDisabled();
						}

						//ImGui::Dummy(ImVec2(0, 0.5f));
						ImGui::EndTable();
					}

					ImGui::PopFont();

					ImGui::PopStyleColor(3);
					ImGui::PopStyleVar(3);

					// Another workaround, popups don't work properly inside tables or something
					{
						if (popupData1.bOpen) {
							std::string popupTitle = StringUtility::Format("Key remap: %s", 
								popupData1.pKeyBindData->name.c_str());
							ImGui::OpenPopup(popupTitle.c_str());

							if (popupData1.bInit) {
								popupData1.bAwaitingInput = true;
								popupData1.startTime = stdch::milliseconds(0);
								popupData1.pressedKey = -1;

								popupData1.bInit = false;
							}

							ImGui::PushFont(pMainWindow->GetFontMap()["Arial20_Ex"]);

							ImGuiViewport* viewport = ImGui::GetMainViewport();

							float popupSizeX = viewport->WorkSize.x * 0.75f;
							//float popupSizeY = 150;
							float popupSizeY = 104;

							ImVec2 center = viewport->GetCenter();
							ImGui::SetNextWindowSize(ImVec2(popupSizeX, popupSizeY), ImGuiCond_Appearing);
							ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

							if (ImGui::BeginPopupModal(popupTitle.c_str(), nullptr, ImGuiWindowFlags_NoResize 
								| ImGuiWindowFlags_NoCollapse))
							{
								auto _CloseButton = [&popupSizeY](float buttonSize) {
									float wd = ImGui::GetContentRegionAvail().x;
									float size = buttonSize + ImGui::GetStyle().FramePadding.x * 2;
									
									ImGui::SetCursorPosY(popupSizeY - 38);
									ImGui::Separator();

									ImGui::SetCursorPosX((wd - size) / 2);

									//ImGui::SetItemDefaultFocus();
									if (ImGui::Button("Cancel", ImVec2(buttonSize, 0)))
										throw 0;	// Jump to popup close
								};

								KeyBindData* pKeybindData = popupData1.pKeyBindData;

								try {
									if (popupData1.bAwaitingInput) {
										//if (input->GetKeyState(DIK_ESCAPE) == KEY_PUSH)
										//	throw 0;	// Jump to popup close

										ImGui::Text("Press any key (%s)...", 
											popupData1.bKeyboard ? "Keyboard" : "Controller");

										_CloseButton(80);

										if (popupData1.bKeyboard) {
											auto& mapValidCode = listKeyCode_.GetCodeMap();
											for (auto& [keyId, _] : mapValidCode) {
												DIKeyState state = input->GetKeyState(keyId);
												if (state == KEY_PUSH) {
													popupData1.pressedKey = keyId;
													break;
												}
											}
										}
										else {
											for (int16_t iButton = 0; iButton < DirectInput::MAX_PAD_STATE; iButton++) {
												DIKeyState state = input->GetPadState(0, iButton);
												if (state == KEY_PUSH) {
													popupData1.pressedKey = iButton;
													break;
												}
											}
										}

										if (popupData1.pressedKey >= 0) {
											popupData1.startTime = GetCurrentMs();
											popupData1.bAwaitingInput = false;
										}
									}
									/*
									else {
										auto nowMs = GetCurrentMs();
										auto timeSinceRemap = (nowMs - popupData1.startTime).count();

										if (timeSinceRemap <= 2000) {		// 2s timer
											if (timeSinceRemap > 200) {		// 0.2s delay
												// Jump to popup close on any key press
												for (int16_t iMouse = 0; iMouse < DirectInput::MAX_MOUSE_BUTTON; ++iMouse) {
													if (input->GetMouseState(iMouse) == KEY_PUSH)
														throw 0;
												}
												for (int16_t iKey = 0; iKey < DirectInput::MAX_KEY; ++iKey) {
													if (input->GetKeyState(iKey) == KEY_PUSH)
														throw 0;
												}
												for (int16_t iPad = 0; iPad < listPadDeviceKeyMap_.size(); ++iPad) {
													for (int16_t iButton = 0; iButton < DirectInput::MAX_PAD_STATE; ++iButton) {
														if (input->GetPadState(iPad, iButton) == KEY_PUSH)
															throw 0;
													}
												}
											}

											std::string keyAsStr;
											if (popupData1.bKeyboard) {
												keyAsStr = listKeyCode_.GetCodeText(popupData1.pressedKey);
											}
											else {
												keyAsStr = ControllerMap::GetNameFromButton(
													pControllerKeyMap, popupData1.pressedKey);
											}

											ImGui::Text("Remapped \"%s\" to %s",
												pKeybindData->name.c_str(), keyAsStr.c_str());
											ImGui::NewLine();
											ImGui::Text("Press any key to close.");

											_CloseButton(80);
										}
										else {
											throw 0;
										}
									}
									*/
									else {
										throw 0;
									}
								}
								catch (int close) {
									if (close == 0) {
										ImGui::CloseCurrentPopup();

										if (popupData1.pressedKey >= 0) {
											if (popupData1.bKeyboard) {
												pKeybindData->keyboardButton = popupData1.pressedKey;
												pKeybindData->fadeTimer[0] = GetCurrentMs();
											}
											else {
												pKeybindData->padDevice = 0;
												pKeybindData->padButton = popupData1.pressedKey;
												pKeybindData->fadeTimer[1] = GetCurrentMs();
											}
										}

										popupData1.bOpen = false;
									}
								}

								ImGui::EndPopup();
							}

							ImGui::PopFont();
						}
					}
				}

				ImGui::EndChild();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Input Devices")) {
				ImVec2 rgn = ImGui::GetContentRegionAvail();
				if (ImGui::BeginChild("ChildW_Key_InputDevices", ImVec2(rgn.x, rgn.y), false, 0)) {
					auto _DisplayDevice = [&](const std::string& title, const DirectInput::InputDeviceHeader* idh) {
						ImGui::Text(title.c_str());
						ImGui::Indent(32);

						auto _AddText = [](const char* text1, const std::string& text2, float offset = 150) {
							ImGui::Text(text1);
							ImGui::SameLine(offset);
							ImGui::Text(text2.c_str());
						};
						auto _AddTextW = [](const char* text1, const std::wstring& text2, float offset = 150) {
							ImGui::Text(text1);
							ImGui::SameLine(offset);
							ImGui::Text(StringUtility::ConvertWideToMulti(text2).c_str());
						};

						_AddTextW("Product name:", idh->ddi.tszProductName);
						_AddTextW("Instance name:", idh->ddi.tszInstanceName);

						_AddText("Product GUID:", StringUtility::FromGuid(&idh->ddi.guidProduct));
						_AddText("Instance GUID:", StringUtility::FromGuid(&idh->ddi.guidInstance));

						ImGui::Text("Vendor ID:");	ImGui::SameLine(32 + 80);
						ImGui::Text(StringUtility::Format("%04x", idh->vendorID).c_str());
						ImGui::SameLine(200);
						ImGui::Text("Product ID:");	ImGui::SameLine(200 + 80);
						ImGui::Text(StringUtility::Format("%04x", idh->productID).c_str());

						ImGui::Unindent(32);
					};

					_DisplayDevice("Keyboard", &input->GetKeyboardDevice()->idh);
					_DisplayDevice("Mouse", &input->GetMouseDevice()->idh);

					for (int i = 0; i < input->GetPadDeviceCount(); ++i) {
						std::string title = StringUtility::Format("Controller %d", i);
						_DisplayDevice(title, &input->GetPadDevice(i)->idh);
					}
				}

				ImGui::EndChild();
				ImGui::EndTabItem();
			}

			if (!bPadAvailable) ImGui::BeginDisabled();
			if (ImGui::BeginTabItem("Controller Calibration")) {
				if (!bPadRefreshing_) {
					if (ImGui::BeginChild("ChildW_Key_ControllerCalibrate", 
						ImVec2(wd, ImGui::GetContentRegionAvail().y), false, 0))
					{
						ImGui::Dummy(ImVec2(0, 2));

						ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 2.0f);
						ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

						const DirectInput::JoypadInputDevice* pJoypad = input->GetPadDevice(selectedPadDevice_);
						const DIJOYSTATE* padState = &pJoypad->state;

						{
							//ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(188, 220, 255, 255));
							ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
							ImGui::PushStyleColor(ImGuiCol_SliderGrab, IM_COL32(178, 178, 255, 255));
							ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL32(60, 60, 255, 255));

							int prevResponse = padResponse_;

							ImGui::Text("Analog stick response threshold (Default: 500)");
							ImGui::PushItemWidth(320);
							ImGui::SliderInt("##pad_response_slider", &padResponse_, 100, 900);
							ImGui::PopItemWidth();
							
							_SetPadResponse();

							ImGui::PopStyleColor(3);
						}

						ImGuiWindowFlags window_flags = ImGuiWindowFlags_None | ImGuiWindowFlags_NoScrollWithMouse;

						// Render XY analog
						{
							if (ImGui::BeginChild("ChildW_Key_Test_Axes_XY", ImVec2(142, 145), true, window_flags)) {
								ImGui::Text(" X/Y Analog Stick");

								ImGuiIO& io = ImGui::GetIO();
								ImDrawList* draw_list = ImGui::GetWindowDrawList();

								ImVec2 canvasSize = ImVec2(112, 100);

								ImVec2 canvasLT = ImGui::GetCursorScreenPos();
								canvasLT.x += 6;
								canvasLT.y += 2;

								ImVec2 canvasRB = ImVec2(canvasLT.x + canvasSize.x, canvasLT.y + canvasSize.y);
								ImVec2 canvasCenter = ImVec2(canvasLT.x + canvasSize.x / 2, canvasLT.y + canvasSize.y / 2);

								ImVec2 crossRange = ImVec2(canvasSize.x - 20, canvasSize.y - 20);

								float padResponseRate = padResponse_ / 1000.0f * 0.5f;
								ImVec2 canvasLT_2 = ImVec2(
									canvasCenter.x - crossRange.x * padResponseRate,
									canvasCenter.y - crossRange.y * padResponseRate);
								ImVec2 canvasRB_2 = ImVec2(
									canvasCenter.x + crossRange.x * padResponseRate,
									canvasCenter.y + crossRange.y * padResponseRate);

								draw_list->AddRectFilled(canvasLT, canvasRB, IM_COL32(200, 200, 224, 255));
								draw_list->AddRectFilled(canvasLT_2, canvasRB_2, IM_COL32(230, 230, 255, 255));
								draw_list->AddRect(canvasLT, canvasRB, IM_COL32(24, 24, 48, 255));

								float crossX = canvasCenter.x + padState->lX * crossRange.x / 1000.0f * 0.5f;
								float crossY = canvasCenter.y + padState->lY * crossRange.y / 1000.0f * 0.5f;

								constexpr float LINE_SPAN = 6.0f;

								draw_list->PushClipRect(canvasLT, canvasRB, true);
								draw_list->AddLine(
									ImVec2(crossX - LINE_SPAN, crossY),
									ImVec2(crossX + LINE_SPAN, crossY),
									IM_COL32(0, 0, 0, 255), 1.2f);
								draw_list->AddLine(
									ImVec2(crossX, crossY - LINE_SPAN),
									ImVec2(crossX, crossY + LINE_SPAN),
									IM_COL32(0, 0, 0, 255), 1.2f);
								draw_list->PopClipRect();
							}
							ImGui::EndChild();
						}

						ImGui::SameLine();

						// Render other axes
						{
							float wd2 = ImGui::GetContentRegionAvail().x - 4;
							if (ImGui::BeginChild("ChildW_Key_Test_Axes_Other", ImVec2(wd2, 145), true, window_flags)) {
								ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(32, 144, 255, 255));

								auto _RenderAxisDisplay = [&](int index, LONG value, const char* text) {
									float progress = 50 + (float)value / 1000 * 50;

									ImGui::ItemSize(ImVec2(48, 0));
									ImGui::Text(text);
									ImGui::SameLine(86);

									ImGui::ProgressBar(0.5f + (float)value / 1000 * 0.5f,
										ImVec2(128, 15), "");
									ImGui::SameLine();

									ImGui::Text("%d", value);
								};

								constexpr int INDENT = 4;

								ImGui::Dummy(ImVec2(0, 10));
								ImGui::Indent(INDENT);
								_RenderAxisDisplay(0, padState->lZ, "Z Axis");
								_RenderAxisDisplay(1, padState->lRx, "X Rotation");
								_RenderAxisDisplay(2, padState->lRy, "Y Rotation");
								_RenderAxisDisplay(3, padState->lRz, "Z Rotation");
								ImGui::Unindent(INDENT);

								ImGui::PopStyleColor();
							}
							ImGui::EndChild();
						}

						// Render POV Hat
						{
							if (ImGui::BeginChild("ChildW_Key_Test_POVHat", ImVec2(142, 164), true, window_flags)) {
								ImGui::Text(" POV Hat #0");

								ImGuiIO& io = ImGui::GetIO();
								ImDrawList* draw_list = ImGui::GetWindowDrawList();

								float wd2 = ImGui::GetContentRegionAvail().x;
								float circleRadius = 50;

								ImVec2 canvasLT = ImGui::GetCursorScreenPos();
								//canvasLT.x += 6;
								canvasLT.y -= 2;

								ImVec2 canvasCenter = ImVec2(canvasLT.x + wd2 / 2, canvasLT.y + wd2 / 2);

								draw_list->AddCircle(canvasCenter, circleRadius, IM_COL32(24, 24, 48, 255));

								draw_list->AddCircleFilled(canvasCenter, 8, IM_COL32(200, 200, 224, 255), 16);
								draw_list->AddCircle(canvasCenter, 8, IM_COL32(24, 24, 48, 255), 16);

								int povDir = (int)padState->rgdwPOV[0];
								if (povDir >= 0) {
									auto _AddTriangle = [&](double angle) {
										angle = Math::DegreeToRadian(angle);
										Math::DVec2 sincos = { cos(angle), sin(angle) };

										ImVec2 verts[3] = {
											ImVec2(0, -circleRadius),
											ImVec2(-12, -circleRadius + 12),
											ImVec2(12, -circleRadius + 12),
										};
										for (int i = 0; i < 3; ++i) {
											Math::DVec2 vert = { verts[i].x, verts[i].y };
											Math::Rotate2D(vert, sincos);
											verts[i] = ImVec2(canvasCenter.x + vert[0], canvasCenter.y + vert[1]);
										}

										draw_list->AddTriangleFilled(verts[0], verts[1], verts[2], IM_COL32(32, 144, 255, 255));
									};

									_AddTriangle(povDir / 100.0f);
								}

								ImGui::Dummy(ImVec2(0, 108));
								//ImGui::Indent(wd2 / 2 - 16);
								ImGui::Text(" %d", povDir);
							}
							ImGui::EndChild();
						}

						ImGui::SameLine();

						// Render buttons
						{
							float wd2 = ImGui::GetContentRegionAvail().x - 4;
							if (ImGui::BeginChild("ChildW_Key_Test_Buttons", ImVec2(wd2, 164), true, window_flags)) {
								ImGui::Text(" Buttons");

								ImVec2 canvasLT = ImGui::GetCursorScreenPos();

								ImGui::Dummy(ImVec2(0, 12));
								ImGui::Indent(8);

								ImGui::PushFont(pMainWindow->GetFontMap()["Arial14"]);

								float cursorXOrg = ImGui::GetCursorPosX();
								float cursorX = cursorXOrg;

								auto _AddButton = [&](int buttonId) {
									float cursorY = ImGui::GetCursorPosY();
									ImGui::Text("%02d", buttonId);

									ImDrawList* draw_list = ImGui::GetWindowDrawList();

									ImU32 colorFill1 = IM_COL32(32, 144, 255, 255);
									ImU32 colorFill2 = IM_COL32(32, 144, 255, 48);
									ImU32 colorEdge = IM_COL32(24, 24, 48, 255);

									ImVec2 circleCenter = ImVec2(
										canvasLT.x + cursorX + 20,
										canvasLT.y + cursorY - 21);

									draw_list->AddCircleFilled(circleCenter, 10,
										padState->rgbButtons[buttonId] ? colorFill1 : colorFill2, 16);
									draw_list->AddCircle(circleCenter, 10, colorEdge, 16);
								};

								for (int i = 0; i < DirectInput::MAX_PAD_BUTTON; ) {
									ImGui::SetCursorPosX(cursorX);
									_AddButton(i);

									cursorX += 54;
									i++;
									if (i % 6 == 0) {
										cursorX = cursorXOrg;
										ImGui::Dummy(ImVec2(0, 18));
									}
									else {
										ImGui::SameLine();
									}
								}

								ImGui::PopFont();
							}

							ImGui::EndChild();
						}

						ImGui::PopStyleVar(2);
					}
					ImGui::EndChild();
				}

				ImGui::EndTabItem();
			}
			if (!bPadAvailable) ImGui::EndDisabled();

			ImGui::EndTabBar();
		}
	}

	bPadRefreshing_ = false;
}

//*******************************************************************
//OptionPanel
//*******************************************************************
OptionPanel::OptionPanel() {
	tabName_ = "Option";
}
OptionPanel::~OptionPanel() {
}

bool OptionPanel::Initialize() {
	DefaultSettings();

	return true;
}
void OptionPanel::DefaultSettings() {
	checkShowLogWindow_ = false;
	checkFileLogger_ = false;
	checkHideCursor_ = false;

	exePath_ = DNH_EXE_NAME;
	_SetTextBuffer();
}

void OptionPanel::LoadConfiguration() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();

	checkShowLogWindow_ = config->bLogWindow_;
	checkFileLogger_ = config->bLogFile_;
	checkHideCursor_ = !config->bMouseVisible_;

	exePath_ = config->pathExeLaunch_;
}
void OptionPanel::SaveConfiguration() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();

	config->bLogWindow_ = checkShowLogWindow_;
	config->bLogFile_ = checkFileLogger_;
	config->bMouseVisible_ = !checkHideCursor_;

	config->pathExeLaunch_ = exePath_.size() > 0U ? exePath_ : DNH_EXE_NAME;
}

void OptionPanel::ProcessGui() {
	float wd = ImGui::GetContentRegionAvail().x;

	{
		ImGuiExt_BeginGroupPanel("Game Executable Path", ImVec2(wd - 2, 0.0f));
		ImGui::Dummy(ImVec2(0, 2));

		ImGuiStyle& style = ImGui::GetStyle();

		ImVec2 buttonSize(56, 22);

		{
			float wd2 = ImGui::GetContentRegionAvail().x;
			//ImGui::ItemSize(ImVec2(wd2 - buttonSize.x - 4, buttonSize.y));

			ImVec2 itemSize(wd2 - buttonSize.x - style.ItemSpacing.x * 2 + 2, buttonSize.y);

			ImGui::InputTextEx("##exepath", StringUtility::ConvertWideToMulti(exePath_).c_str(), 
				bufTextBox_, 255, itemSize, 0, nullptr, nullptr);

			{
				std::string path = bufTextBox_;
				exePath_ = StringUtility::ConvertMultiToWide(path);
			}
		}

		ImGui::SameLine();

		{
			if (ImGui::Button("Browse", buttonSize)) {
				_BrowseExePath();
			}
		}

		ImGui::Dummy(ImVec2(0, 1));
		ImGuiExt_EndGroupPanel();
	}

	ImGui::NewLine();

	{
		ImGuiExt_BeginGroupPanel("Options", ImVec2(wd - 2, 0.0f));
		ImGui::Dummy(ImVec2(0, 2));

		ImGui::Checkbox("Show LogWindow on startup", &checkShowLogWindow_);
		ImGui::Checkbox("Save LogWindow logs to file", &checkFileLogger_);
		ImGui::Checkbox("Hide mouse cursor", &checkHideCursor_);

		ImGui::Dummy(ImVec2(0, 1));
		ImGuiExt_EndGroupPanel();
	}
}

void OptionPanel::_SetTextBuffer() {
	std::string exePathMb = StringUtility::ConvertWideToMulti(exePath_);
	strcpy_s(bufTextBox_, exePathMb.c_str());
}

void OptionPanel::_BrowseExePath() {
	std::wstring path;

	IFileOpenDialog* pFileOpen = nullptr;
	HRESULT hr = ::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, (void**)(&pFileOpen));
	if (SUCCEEDED(hr)) {
		pFileOpen->SetOptions(FOS_FILEMUSTEXIST);

		{
			IShellItem* pDefaultFolder = nullptr;

			std::wstring moduleDir = PathProperty::GetModuleDirectory();
			moduleDir = PathProperty::MakePreferred(moduleDir);	//Path seperator must be \\ for SHCreateItem

			SHCreateItemFromParsingName(moduleDir.c_str(), nullptr,
				IID_IShellItem, (void**)&pDefaultFolder);

			if (pDefaultFolder)
				pFileOpen->SetFolder(pDefaultFolder);
		}

		if (SUCCEEDED(hr = pFileOpen->Show(nullptr))) {
			IShellItem* psiResult;
			pFileOpen->GetResult(&psiResult);

			PWSTR pszFilePath = nullptr;
			hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH,
				&pszFilePath);
			if (SUCCEEDED(hr)) {
				path = (wchar_t*)pszFilePath;
				path = PathProperty::ReplaceYenToSlash(path);

				CoTaskMemFree(pszFilePath);
			}

			psiResult->Release();
		}
		pFileOpen->Release();
	}

	if (path.size() > 0) {
		std::wstring moduleDir = PathProperty::GetModuleDirectory();
		if (path.find(moduleDir) != std::wstring::npos) {
			exePath_ = path.substr(moduleDir.size());
			_SetTextBuffer();
		}
	}
}

// https://github.com/ocornut/imgui/issues/1496#issuecomment-655048353
void ImGuiExt_BeginGroupPanel(const char* name, const ImVec2& size) {
	ImGui::BeginGroup();

	auto cursorPos = ImGui::GetCursorScreenPos();
	auto itemSpacing = ImGui::GetStyle().ItemSpacing;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	auto frameHeight = ImGui::GetFrameHeight();
	ImGui::BeginGroup();

	ImVec2 effectiveSize = size;
	effectiveSize.x = size.x < 0.0f ? ImGui::GetContentRegionAvail().x : size.x;

	ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

	ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::BeginGroup();
	ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::TextUnformatted(name);
	auto labelMin = ImGui::GetItemRectMin();
	auto labelMax = ImGui::GetItemRectMax();
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
	ImGui::BeginGroup();

	//ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

	ImGui::PopStyleVar(2);

	ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;

	ImGui::GetCurrentWindow()->Size.x -= frameHeight;

	auto itemWidth = ImGui::CalcItemWidth();
	ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));

	s_GroupPanelLabelStack.push_back(ImRect(labelMin, labelMax));
}
void ImGuiExt_EndGroupPanel() {
	ImGui::PopItemWidth();

	auto itemSpacing = ImGui::GetStyle().ItemSpacing;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	auto frameHeight = ImGui::GetFrameHeight();

	ImGui::EndGroup();

	//ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 255, 0, 64), 4.0f);

	ImGui::EndGroup();

	ImGui::SameLine(0.0f, 0.0f);
	ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
	ImGui::Dummy(ImVec2(0.0, frameHeight - frameHeight * 0.5f - itemSpacing.y));

	ImGui::EndGroup();

	auto itemMin = ImGui::GetItemRectMin();
	auto itemMax = ImGui::GetItemRectMax();
	//ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

	auto labelRect = s_GroupPanelLabelStack.back();
	s_GroupPanelLabelStack.pop_back();

	auto _ImVec2_Add = [](const ImVec2& v1, const ImVec2& v2) -> ImVec2 {
		return ImVec2(v1.x + v2.x, v1.y + v2.y);
	};
	auto _ImVec2_Sub = [](const ImVec2& v1, const ImVec2& v2) -> ImVec2 {
		return ImVec2(v1.x - v2.x, v1.y - v2.y);
	};

	ImVec2 halfFrame = ImVec2(frameHeight * 0.25f * 0.5f, frameHeight * 0.5f);
	ImRect frameRect = ImRect(
		_ImVec2_Add(itemMin, halfFrame),
		_ImVec2_Sub(itemMax, ImVec2(halfFrame.x, 0.0f)));
	labelRect.Min.x -= itemSpacing.x;
	labelRect.Max.x += itemSpacing.x;

	for (int i = 0; i < 4; ++i) {
		switch (i) {
			// left half-plane
		case 0: ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true); break;
			// right half-plane
		case 1: ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true); break;
			// top
		case 2: ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX), ImVec2(labelRect.Max.x, labelRect.Min.y), true); break;
			// bottom
		case 3: ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true); break;
		}

		ImGui::GetWindowDrawList()->AddRect(
			frameRect.Min, frameRect.Max,
			ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)),
			halfFrame.x);

		ImGui::PopClipRect();
	}

	ImGui::PopStyleVar(2);

	ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;

	ImGui::GetCurrentWindow()->Size.x += frameHeight;

	ImGui::Dummy(ImVec2(0.0f, 0.0f));

	ImGui::EndGroup();
}
#include "source/GcLib/pch.h"

#include <shlobj.h>

#include "MainWindow.hpp"
#include "LibImpl.hpp"

static const std::wstring FILEARCH_VERSION_STR = WINDOW_TITLE + L" " + DNH_VERSION;

//*******************************************************************
//DxGraphicsFileArchiver
//*******************************************************************
DxGraphicsFileArchiver::DxGraphicsFileArchiver() {
	ZeroMemory(&d3dpp_, sizeof(d3dpp_));
}
DxGraphicsFileArchiver::~DxGraphicsFileArchiver() {
}

void DxGraphicsFileArchiver::_ReleaseDxResource() {
	ImGui_ImplDX9_InvalidateDeviceObjects();
	DirectGraphicsBase::_ReleaseDxResource();
}
void DxGraphicsFileArchiver::_RestoreDxResource() {
	ImGui_ImplDX9_CreateDeviceObjects();
	DirectGraphicsBase::_RestoreDxResource();
}
bool DxGraphicsFileArchiver::_Restore() {
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

std::vector<std::wstring> DxGraphicsFileArchiver::_GetRequiredModules() {
	return std::vector<std::wstring>({
		L"d3d9.dll", 
	});
}

bool DxGraphicsFileArchiver::Initialize(HWND hWnd) {
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
void DxGraphicsFileArchiver::Release() {
	DirectGraphicsBase::Release();
}

bool DxGraphicsFileArchiver::BeginScene(bool bClear) {
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
void DxGraphicsFileArchiver::EndScene(bool bPresent) {
	pDevice_->EndScene();

	deviceStatus_ = pDevice_->Present(nullptr, nullptr, nullptr, nullptr);
	if (FAILED(deviceStatus_)) {
		_Restore();
	}
}

void DxGraphicsFileArchiver::ResetDevice() {
	_ReleaseDxResource();
	deviceStatus_ = pDevice_->Reset(&d3dpp_);
	if (SUCCEEDED(deviceStatus_)) {
		_RestoreDxResource();
	}
}

//*******************************************************************
//FileEntryInfo
//*******************************************************************
void FileEntryInfo::Sort() {
	// < operator
	auto _PredLt = [](unique_ptr<FileEntryInfo>& left, unique_ptr<FileEntryInfo>& right) {
		if (left->bDirectory != right->bDirectory)
			return left->bDirectory;
		if (left->bDirectory) {
			//Both are directories
			return left->path < right->path;
		}
		//Both are files
		return left->displayName < right->displayName;
	};
	children.sort(_PredLt);

	for (auto& i : children) {
		if (i->bDirectory)
			i->Sort();
	}
}

//*******************************************************************
//MainWindow
//*******************************************************************
MainWindow::MainWindow() {
	bInitialized_ = false;

	pIo_ = nullptr;
	dpi_ = USER_DEFAULT_SCREEN_DPI;

	bArchiveEnabled_ = false;
}
MainWindow::~MainWindow() {
	//Stop();
	//Join();

	_SaveEnvironment();

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	dxGraphics_->Release();
	::UnregisterClassW(L"WC_FileArchiver", ::GetModuleHandleW(nullptr));
}

bool MainWindow::Initialize() {
	dxGraphics_.reset(new DxGraphicsFileArchiver());
	dxGraphics_->SetSize(800, 600);

	HINSTANCE hInst = ::GetModuleHandleW(nullptr);

	{
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_CLASSDC;
		wcex.lpfnWndProc = (WNDPROC)WindowBase::_StaticWindowProcedure;
		wcex.hInstance = hInst;
		wcex.lpszClassName = L"WC_FileArchiver";
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

	SetWindowTextW(hWnd_, FILEARCH_VERSION_STR.c_str());

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

	_LoadEnvironment();

	bInitialized_ = true;
	//Start();

	return true;
}

void MainWindow::_LoadEnvironment() {
	std::wstring path = PathProperty::GetModuleDirectory() + PATH_ENVIRONMENT;

	File file(path);
	if (file.Open(File::AccessType::READ)) {
		size_t len = file.ReadInteger();
		if (len > 0) {
			pathSaveBaseDir_.resize(len);
			file.Read(pathSaveBaseDir_.data(), len * sizeof(wchar_t));
		}

		len = file.ReadInteger();
		if (len > 0) {
			pathSaveArchive_.resize(len);
			file.Read(pathSaveArchive_.data(), len * sizeof(wchar_t));
		}
	}
}
void MainWindow::_SaveEnvironment() {
	std::wstring path = PathProperty::GetModuleDirectory() + PATH_ENVIRONMENT;

	File file(path);
	if (file.Open(File::AccessType::WRITEONLY)) {
		size_t len = pathSaveBaseDir_.size();
		file.WriteInteger(len);
		if (len > 0)
			file.Write(pathSaveBaseDir_.data(), len * sizeof(wchar_t));

		len = pathSaveArchive_.size();
		file.WriteInteger(len);
		if (len > 0)
			file.Write(pathSaveArchive_.data(), len * sizeof(wchar_t));
	}
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
		mapFont_["Arial8"] = pIo_->Fonts->AddFontFromFileTTF(pathArial, 8.0f * scale);
		mapFont_["Arial16"] = pIo_->Fonts->AddFontFromFileTTF(pathArial, 16.0f * scale);
		mapFont_["Arial24"] = pIo_->Fonts->AddFontFromFileTTF(pathArial, 24.0f * scale);
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

	style.Colors[ImGuiCol_Button] = ImVec4(0.73f, 0.73f, 0.73f, 0.40f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.62f, 0.80f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);

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

void MainWindow::_Update() {
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

	bool bArchiveInProgress = pArchiverWorkThread_ != nullptr;

	if (ImGui::Begin("MainWindow", nullptr, flags)) {
		if (bArchiveInProgress)
			ImGui::BeginDisabled();
		{
			if (ImGui::BeginMainMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("Open Folder", "Ctrl+O")) {
						_AddFileFromDialog();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Exit", "Alt+F4")) {
						::SendMessageW(hWnd_, WM_CLOSE, 0, 0);
					}
					ImGui::EndMenu();
				}
				ImGui::Separator();
				if (ImGui::BeginMenu("Help")) {
					if (ImGui::MenuItem("How to Use", "Ctrl+H")) {
						bPopup_Help = true;
					}
					if (ImGui::MenuItem("Version", "Ctrl+V")) {
						bPopup_Version = true;
					}
					ImGui::EndMenu();
				}
				ImGui::Separator();
				ImGui::EndMainMenuBar();
			}

			//WinAPI MessageBox would've been easier, but this looks way cooler

			//A workaround as BeginPopupModal doesn't work inside BeginMainMenuBar
			auto _CreatePopup = [&](bool* pPopupVar, const char* title, std::function<void(void)> cbDisplay) {
				float popupSize = viewport->WorkSize.x * 0.75f;

				if (*pPopupVar) {
					ImGui::OpenPopup(title);
					*pPopupVar = false;
				}

				ImVec2 center = ImGui::GetMainViewport()->GetCenter();
				ImGui::SetNextWindowSize(ImVec2(popupSize, 0), ImGuiCond_Appearing);
				ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
				if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
					cbDisplay();

					float wd = ImGui::GetContentRegionAvail().x;
					float size = 120 + ImGui::GetStyle().FramePadding.x * 2;
					ImGui::SetCursorPosX((wd - size) / 2);

					ImGui::SetItemDefaultFocus();
					if (ImGui::Button("Close", ImVec2(120, 0)))
						ImGui::CloseCurrentPopup();

					ImGui::EndPopup();
				}
			};

			_CreatePopup(&bPopup_Help, "Help", []() {
				ImGui::TextWrapped(
					"Select the base directory with the \"Add\" button. All files in the directory will be added.\n"
					"   Toggle file/directory inclusion with the checkboxes.\n"
					"To rescan the directory for file changes without having to browse again, use the \"Rescan\" button.\n"
					"To clear all files, use the \"Clear\" button.\n"
					"To start archiving the selected files, use the \"Start Archive\" button.",
					"Help");
			});
			_CreatePopup(&bPopup_Version, "Version", []() {
				ImGui::TextUnformatted(StringUtility::ConvertWideToMulti(FILEARCH_VERSION_STR).c_str());
			});
		}

		{
			float ht = ImGui::GetContentRegionAvail().y - 80;

			{
				ImGuiWindowFlags window_flags = ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar;
				ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

				ImVec2 size(ImGui::GetContentRegionAvail().x - 112, ht);

				if (ImGui::BeginChild("ChildW_FileTreeView", size, true, window_flags)) {
					if (ImGui::BeginMenuBar()) {
						ImGui::Text("Files");
						ImGui::EndMenuBar();
					}
					_ProcessGui_FileTree();
				}
				ImGui::EndChild();
				ImGui::PopStyleVar();
			}

			ImGui::SameLine();

			{
				ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
				ImGui::BeginChild("ChildW_SideButtons", ImVec2(0, ht), false, window_flags);

				float availW = ImGui::GetContentRegionAvail().x;
				ImVec2 size(availW, 0);

				//ImGui::Dummy(ImVec2(0, 6));
				if (ImGui::Button("Open Folder", size)) {
					_AddFileFromDialog();
				}

				bool bDisable = pathBaseDir_.size() == 0;

				if (bDisable) ImGui::BeginDisabled();
				if (ImGui::Button("Rescan", size)) {
					if (pathBaseDir_.size() > 0)
						_AddFilesInDirectory(pathBaseDir_);
				}
				if (ImGui::Button("Clear", size)) {
					pathBaseDir_ = L"";
					_RemoveAllFiles();
				}
				if (bDisable) ImGui::EndDisabled();

				ImGui::EndChild();
			}
		}

		{
			ImGui::Dummy(ImVec2(0, 8));

			ImGuiStyle& style = ImGui::GetStyle();

			//ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 140 + style.WindowPadding.x);
			ImGui::SetCursorPosX(16);
			ImGui::PushFont(mapFont_["Arial24"]);

			std::function<size_t(FileEntryInfo*)> _CountIncluded;
			_CountIncluded = [&](FileEntryInfo* pItem) -> size_t {
				if (!pItem->bDirectory && pItem->bIncluded)
					return 1;
				size_t res = 0;
				for (auto& iChild : pItem->children) {
					res += _CountIncluded(iChild.get());
				}
				return res;
			};
			size_t nIncluded = nodeDirectoryRoot_ 
				? _CountIncluded(nodeDirectoryRoot_.get()) : 0;

			if (nIncluded == 0) ImGui::BeginDisabled();
			if (ImGui::Button("Start Archive", ImVec2(140, 36))) {
				_StartArchive();
			}
			if (nIncluded == 0) ImGui::EndDisabled();

			ImGui::PopFont();
		}
	}
	if (bArchiveInProgress) {
		ImGui::EndDisabled();

		const char* title = pArchiverWorkThread_->GetStatus() == Thread::RUN 
			? "Archiving..." : "Finished";
		float popupSize = viewport->WorkSize.x * 0.75f;

		ImGui::OpenPopup(title);

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowSize(ImVec2(popupSize, 0), ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
			if (pArchiverWorkThread_->GetStatus() == Thread::RUN) {
				const std::wstring& status = pArchiverWorkThread_->GetArchiverStatus();
				float progress = pArchiverWorkThread_->GetArchiverProgress();

				ImGui::TextWrapped(StringUtility::ConvertWideToMulti(status).c_str());
				ImGui::NewLine();

				ImGui::ProgressBar(progress, ImVec2(-1, 0));
			}
			else {
				std::string& err = pArchiverWorkThread_->GetError();
				if (err.size() == 0) {
					//Success

					ImGui::TextWrapped("Archive creation successful.\n\n[%s]",
						StringUtility::ConvertWideToMulti(pathArchive_).c_str());
				}
				else {
					//Failed

					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.15f, 0.15f, 1));
					ImGui::TextWrapped("Failed to create the archive.\n\n");
					ImGui::PopStyleColor();

					ImGui::TextWrapped("  %s", err.c_str());
				}

				float wd = ImGui::GetContentRegionAvail().x;
				float size = 120 + ImGui::GetStyle().FramePadding.x * 2;
				ImGui::SetCursorPosX((wd - size) / 2);

				ImGui::SetItemDefaultFocus();
				if (ImGui::Button("Close", ImVec2(120, 0))) {
					pArchiverWorkThread_->Join(10);
					pArchiverWorkThread_ = nullptr;
					pathArchive_ = L"";

					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}
	}

	ImGui::End();

	ImGui::PopFont();
}
void MainWindow::_ProcessGui_FileTree() {
	if (nodeDirectoryRoot_ == nullptr)
		return;

	static FileEntryInfo* g_pNSelected = nodeDirectoryRoot_.get();

	ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_OpenOnArrow 
		/* | ImGuiTreeNodeFlags_OpenOnDoubleClick*/ | ImGuiTreeNodeFlags_SpanAvailWidth;

	std::function<void(FileEntryInfo*)> _AddItems;
	std::function<void(FileEntryInfo*, bool)> _SetItemIncludedRecursive;

	float availH = ImGui::GetContentRegionAvail().y;

	auto _RenderCheckBox = [&](FileEntryInfo* pItem) {
		ImGui::PushFont(mapFont_["Arial8"]);
		ImGui::Checkbox("", &(pItem->bIncluded));
		if (ImGui::IsItemClicked() && ImGui::IsItemActive()) {
			//Why do I have to do this??
			pItem->bIncluded = !pItem->bIncluded;
			_SetItemIncludedRecursive(pItem, pItem->bIncluded);
		}
		ImGui::PopFont();
	};
	_AddItems = [&](FileEntryInfo* pItem) {
		for (auto& iChild : pItem->children) {
			/*
			if (ImGui::GetCursorScreenPos().y > availH + 100) {
				return;
			}
			*/

			_RenderCheckBox(iChild.get());
			ImGui::SameLine();

			ImGuiTreeNodeFlags nodeFlags = baseFlags;
			if (iChild->bDirectory)
				nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
			else
				nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

			if (iChild.get() == g_pNSelected)
				nodeFlags |= ImGuiTreeNodeFlags_Selected;

			std::string name = StringUtility::ConvertWideToMulti(iChild->displayName);
			bool bOpen = ImGui::TreeNodeEx(iChild.get(), nodeFlags, name.c_str());
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
				g_pNSelected = iChild.get();

			if (bOpen && iChild->bDirectory) {
				//Add children nodes
				_AddItems(iChild.get());
				ImGui::TreePop();
			}
		}
	};
	_SetItemIncludedRecursive = [&](FileEntryInfo* pItem, bool set) {
		pItem->bIncluded = set;
		for (auto& iChild : pItem->children) {
			_SetItemIncludedRecursive(iChild.get(), set);
		}
	};

	{
		ImGuiTreeNodeFlags nodeFlags = baseFlags;
		if (nodeDirectoryRoot_.get() == g_pNSelected)
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

		float orgIndent = ImGui::GetStyle().IndentSpacing;
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, orgIndent + 6);

		std::string name = StringUtility::ConvertWideToMulti(nodeDirectoryRoot_->displayName);
		if (ImGui::TreeNodeEx(nodeDirectoryRoot_.get(), nodeFlags, "")) {
			ImGui::SameLine(35);
			_RenderCheckBox(nodeDirectoryRoot_.get());

			_AddItems(nodeDirectoryRoot_.get());
			ImGui::TreePop();
		}

		ImGui::PopStyleVar();
	}
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
		minmax->ptMinTrackSize.x = 280;
		minmax->ptMinTrackSize.y = 240;
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

void MainWindow::_AddFileFromDialog() {
	std::wstring dirBase;

	IFileOpenDialog* pFileOpen;
	HRESULT hr = ::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, (void**)(&pFileOpen));
	if (SUCCEEDED(hr)) {
		pFileOpen->SetOptions(FOS_PATHMUSTEXIST | FOS_PICKFOLDERS);

		{
			IShellItem* pDefaultFolder = nullptr;

			if (pathSaveBaseDir_.size() == 0) {
				std::wstring moduleDir = PathProperty::GetModuleDirectory();
				moduleDir = PathProperty::MakePreferred(moduleDir);	//Path seperator must be \\ for SHCreateItem

				SHCreateItemFromParsingName(moduleDir.c_str(), nullptr,
					IID_IShellItem, (void**)&pDefaultFolder);
			}
			else {
				std::wstring defDir = PathProperty::MakePreferred(pathSaveBaseDir_);
				SHCreateItemFromParsingName(defDir.c_str(), nullptr,
					IID_IShellItem, (void**)&pDefaultFolder);
			}

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
				dirBase = (wchar_t*)pszFilePath;
				dirBase = PathProperty::ReplaceYenToSlash(dirBase);
				dirBase = PathProperty::AppendSlash(dirBase);

				CoTaskMemFree(pszFilePath);
			}

			psiResult->Release();
		}
		pFileOpen->Release();
	}

	if (dirBase.size() > 0) {
		pathSaveBaseDir_ = PathProperty::GetFileDirectory(dirBase);

		pathBaseDir_ = dirBase;
		_AddFilesInDirectory(dirBase);
	}
}
BOOL MainWindow::_AddFileFromDrop(WPARAM wParam, LPARAM lParam) {
	/*
	wchar_t szFileName[MAX_PATH];
	HDROP hDrop = (HDROP)wParam;
	UINT uFileNo = DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);

	for (UINT iDrop = 0; iDrop < uFileNo; iDrop++) {
		DragQueryFile(hDrop, iDrop, szFileName, sizeof(szFileName));
		std::wstring path = szFileName;

		std::wstring dirBase = PathProperty::GetFileDirectory(path);
		_AddFile(dirBase, path);
	}
	DragFinish(hDrop);

	buttonArchive_.SetWindowEnable(listFile_.size() > 0);
	*/
	return FALSE;
}

void MainWindow::_AddFilesInDirectory(const std::wstring& dirBase) {
	_RemoveAllFiles();

	if (File::IsDirectory(dirBase)) {
		FileEntryInfo* root = new FileEntryInfo;
		root->path = L"";
		root->displayName = L"/";
		root->bDirectory = true;

		std::function<void(FileEntryInfo*, const std::wstring&)> _AddFiles;
		_AddFiles = [&](FileEntryInfo* parent, const std::wstring& dir) {
			for (auto& itr : stdfs::directory_iterator(dir)) {
				if (itr.is_directory()) {
					std::wstring tDir = PathProperty::ReplaceYenToSlash(itr.path());
					tDir = PathProperty::AppendSlash(tDir);

					std::wstring relativePath = tDir.substr(dirBase.size());

					FileEntryInfo* directory = new FileEntryInfo;
					directory->path = relativePath;
					directory->displayName = relativePath;
					directory->bDirectory = true;

					parent->AddChild(directory);
					mapDirectoryNodes_[relativePath] = directory;

					_AddFiles(directory, tDir);
				}
				else {
					std::wstring tPath = PathProperty::ReplaceYenToSlash(itr.path());
					std::wstring relativePath = tPath.substr(dirBase.size());

					FileEntryInfo* file = new FileEntryInfo;
					file->path = relativePath;
					file->displayName = PathProperty::GetFileName(relativePath);
					file->bDirectory = false;

					listFiles_.push_back(file);
					parent->AddChild(file);
				}
			}
		};

		_AddFiles(root, dirBase);

		root->Sort();
		nodeDirectoryRoot_.reset(root);
	}

	bArchiveEnabled_ = listFiles_.size() > 0;
}

void MainWindow::_RemoveAllFiles() {
	mapDirectoryNodes_.clear();
	listFiles_.clear();
	nodeDirectoryRoot_ = nullptr;

	bArchiveEnabled_ = false;
}

std::wstring MainWindow::_CreateRelativeDirectory(const std::wstring& dirBase, const std::wstring& path) {
	std::wstring dirFull = PathProperty::GetFileDirectory(path);
	std::wstring dir = StringUtility::ReplaceAll(dirFull, dirBase, L"");
	return dir;
}

void MainWindow::_StartArchive() {
	std::wstring path;

	IFileSaveDialog* pFileSave;
	HRESULT hr = ::CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
		IID_IFileSaveDialog, (void**)(&pFileSave));
	if (SUCCEEDED(hr)) {
		pFileSave->SetTitle(L"Creating an archive file");
		pFileSave->SetOptions(FOS_OVERWRITEPROMPT | FOS_NOREADONLYRETURN);

		static const COMDLG_FILTERSPEC fileTypes[] = {
			{ L".dat archive", L"*.dat" },
			{ L"All files", L"*.*" }
		};
		pFileSave->SetFileTypes(2, fileTypes);
		pFileSave->SetDefaultExtension(L"dat");

		{
			IShellItem* pDefaultFolder = nullptr;

			if (pathSaveArchive_.size() > 0) {
				std::wstring defDir = PathProperty::MakePreferred(pathSaveArchive_);
				SHCreateItemFromParsingName(defDir.c_str(), nullptr,
					IID_IShellItem, (void**)&pDefaultFolder);
			}

			if (pDefaultFolder)
				pFileSave->SetFolder(pDefaultFolder);
		}

		if (SUCCEEDED(hr = pFileSave->Show(nullptr))) {
			IShellItem* psiResult;
			pFileSave->GetResult(&psiResult);

			PWSTR pszFilePath = nullptr;
			hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
			if (SUCCEEDED(hr)) {
				path = (wchar_t*)pszFilePath;
				path = PathProperty::ReplaceYenToSlash(path);

				CoTaskMemFree(pszFilePath);
			}

			psiResult->Release();
		}
		pFileSave->Release();
	}

	if (path.size() > 0) {
		pathSaveArchive_ = PathProperty::GetFileDirectory(path);
		pathArchive_ = path;

		std::vector<FileEntryInfo*> listFileArchive;
		for (FileEntryInfo* pFile : listFiles_) {
			if (pFile->bDirectory || !pFile->bIncluded) continue;
			listFileArchive.push_back(pFile);
		}

		pArchiverWorkThread_.reset(new ArchiverThread(listFileArchive, pathBaseDir_, pathArchive_));
		pArchiverWorkThread_->Start();
	}
}

//*******************************************************************
//ArchiverThread
//*******************************************************************
ArchiverThread::ArchiverThread(const std::vector<FileEntryInfo*>& listFile, 
	const std::wstring pathBaseDir, const std::wstring& pathArchive)
{
	listFile_ = listFile;
	pathBaseDir_ = pathBaseDir;
	pathArchive_ = pathArchive;
}

std::set<std::wstring> ArchiverThread::listCompressExclude_ = {
	L".png", L".jpg", L".jpeg",
	L".ogg", L".mp3", L".mp4",
	L".zip", L".rar"
};
void ArchiverThread::_Run() {
	FileArchiver archiver;

	for (FileEntryInfo* iFile : listFile_) {
		std::shared_ptr<ArchiveFileEntry> entry = std::make_shared<ArchiveFileEntry>();

		entry->path = iFile->path;
		entry->sizeFull = 0U;
		entry->sizeStored = 0U;
		entry->offsetPos = 0U;

		std::wstring ext = PathProperty::GetFileExtension(entry->path);
		bool bCompress = listCompressExclude_.find(ext) == listCompressExclude_.end();
		entry->compressionType = bCompress ? ArchiveFileEntry::CT_ZLIB : ArchiveFileEntry::CT_NONE;

		archiver.AddEntry(entry);
	}

	::Sleep(100);

	try {
		FileArchiver::CbSetStatus cbSetStatus = [&](const std::wstring& msg) {
			archiverStatus_ = msg;
		};
		FileArchiver::CbSetProgress cbSetProgress = [&](float progress) {
			archiverProgress_ = progress;
		};

		archiver.CreateArchiveFile(pathBaseDir_, pathArchive_, cbSetStatus, cbSetProgress);

		err_ = "";
	}
	catch (const gstd::wexception& e) {
		err_ = StringUtility::ConvertWideToMulti(e.GetErrorMessage());
	}
}
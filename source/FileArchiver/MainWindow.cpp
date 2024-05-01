#include "source/GcLib/pch.h"

#include <shlobj.h>

#include "MainWindow.hpp"
#include "LibImpl.hpp"

using namespace directx::imgui;

static const std::wstring FILEARCH_VERSION_STR = WINDOW_TITLE + L" " + DNH_VERSION;

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
	bArchiveEnabled_ = false;
}
MainWindow::~MainWindow() {
	_SaveEnvironment();
}

bool MainWindow::Initialize() {
	dxGraphics_.reset(new ImGuiDirectGraphics());
	dxGraphics_->SetSize(800, 600);

	if (!InitializeWindow(L"WC_FileArchiver"))
		return false;
	if (!InitializeImGui())
		return false;

	pIo_->IniFilename = nullptr;	// Disable default save/load of imgui.ini

	_SetImguiStyle(1);

	{
		for (auto size : { 8, 14, 16, 20, 24 }) {
			std::string key = StringUtility::Format("Arial%d", size);
			_AddUserFont(ImGuiAddFont(key, L"Arial", size));
		}

		for (auto size : { 18, 20 }) {
			std::string key = StringUtility::Format("Arial%d_Ex", size);
			_AddUserFont(ImGuiAddFont(key, L"Arial", size, {
				{ 0x0020, 0x00FF },		// ASCII
				{ 0x2190, 0x2199 },		// Arrows
			}));
		}

		_ResetFont();
	}

	SetWindowTextW(hWnd_, FILEARCH_VERSION_STR.c_str());
	MoveWindowCenter();

	{
		for (auto size : { 8, 16, 24 }) {
			std::string key = StringUtility::Format("Arial%d", size);
			_AddUserFont(ImGuiAddFont(key, L"Arial", size));
		}

		_ResetFont();
	}

	bInitialized_ = true;
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

void MainWindow::_SetImguiStyle(float scale) {
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

	ImGuiBaseWindow::_SetImguiStyle(style);
}

LRESULT MainWindow::_SubWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* minmax = (MINMAXINFO*)lParam;
		minmax->ptMinTrackSize.x = 280;
		minmax->ptMinTrackSize.y = 240;
		minmax->ptMaxTrackSize.x = ::GetSystemMetrics(SM_CXMAXTRACK);
		minmax->ptMaxTrackSize.y = ::GetSystemMetrics(SM_CYMAXTRACK);
		return 0;
	}
	}
	return 0;
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

void MainWindow::_AddFileFromDialog() {
	std::wstring dirBase;

	IFileOpenDialog* pFileOpen{};
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

	IFileSaveDialog* pFileSave{};
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
		auto entry = make_unique<ArchiveFileEntry>(iFile->path);

		std::wstring ext = PathProperty::GetFileExtension(entry->path);
		bool bCompress = listCompressExclude_.find(ext) == listCompressExclude_.end();
		entry->compressionType = bCompress ? ArchiveFileEntry::CT_ZLIB : ArchiveFileEntry::CT_NONE;

		archiver.AddEntry(MOVE(entry));
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
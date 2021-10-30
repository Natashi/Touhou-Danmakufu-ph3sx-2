#include "source/GcLib/pch.h"

#include "MainWindow.hpp"
#include "LibImpl.hpp"

//*******************************************************************
//MainWindow
//*******************************************************************

MainWindow::MainWindow() {

}
MainWindow::~MainWindow() {
	//コンパイル中なら停止
	Stop();
	Join();
}
bool MainWindow::Initialize() {
	hWnd_ = ::CreateDialog((HINSTANCE)GetWindowLong(NULL, GWL_HINSTANCE),
		MAKEINTRESOURCE(IDD_DIALOG_MAIN),
		NULL, (DLGPROC)_StaticWindowProcedure);

	SetClassLong(hWnd_, GCL_HICON,
		(LONG)(HICON)LoadImage(Application::GetApplicationHandle(), MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 32, 32, 0));

	this->Attach(hWnd_);
	ShowWindow(hWnd_, SW_HIDE);

	//Windowを画面の中央に移動
	SetBounds(0, 0, 640, 480);
	SetWindowText(hWnd_, WINDOW_TITLE.c_str());

	RECT drect, mrect;
	HWND hDesk = ::GetDesktopWindow();
	::GetWindowRect(hDesk, &drect);
	::GetWindowRect(hWnd_, &mrect);
	int left = drect.right / 2 - (mrect.right - mrect.left) / 2;
	int top = drect.bottom / 2 - (mrect.bottom - mrect.top) / 2;
	::MoveWindow(hWnd_, left, top, mrect.right - mrect.left, mrect.bottom - mrect.top, TRUE);

	//逆コンパイルボタン
	buttonDecompile_.Attach(GetDlgItem(hWnd_, IDC_BUTTON_ARCHIVE));
	buttonDecompile_.SetWindowEnable(false);

	//リスト
	HWND hList = GetDlgItem(hWnd_, IDC_LIST_FILE);
	DWORD dwStyle = ListView_GetExtendedListViewStyle(hList);
	dwStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
	ListView_SetExtendedListViewStyle(hList, dwStyle);
	wndListFile_.Attach(hList);
	wndListFile_.AddColumn(160, COL_FILENAME, L"File");
	wndListFile_.AddColumn(160, COL_DIRECTORY, L"Directory");
	wndListFile_.AddColumn(256, COL_FULLPATH, L"Path");

	//ステータスバー
	wndStatus_.Create(hWnd_);
	std::vector<int> sizeStatus;
	sizeStatus.push_back(1600);
	wndStatus_.SetPartsSize(sizeStatus);
	wndStatus_.SetText(0, L"");

	//Progress bar
	wndProgressBar_.Create(hWnd_);
	wndProgressBar_.SetColor(CLR_DEFAULT, CLR_DEFAULT);
	wndProgressBar_.SetWindowVisible(false);

	//設定読み込み
	_LoadEnvironment();

	DragAcceptFiles(hWnd_, TRUE);

	ShowWindow(hWnd_, SW_SHOW);

	return true;
}
//???????? nani????
void MainWindow::_LoadEnvironment() {
	std::wstring path = PathProperty::GetModuleDirectory() + PATH_ENVIRONMENT;
}
void MainWindow::_SaveEnvironment() {
	std::wstring path = PathProperty::GetModuleDirectory() + PATH_ENVIRONMENT;
}
LRESULT MainWindow::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
	{
		::DestroyWindow(hWnd);
		break;
	}
	case WM_DESTROY:
	{
		_SaveEnvironment();
		::PostQuitMessage(0);
		break;
	}

	case WM_COMMAND:
	{
		int id = wParam & 0xffff;
		switch (id) {
		case IDCANCEL:
		case ID_MENUITEM_EXIT:
			::DestroyWindow(hWnd);
			break;

		case IDC_BUTTON_ADD:
		case ID_MENUITEM_ADD:
			_AddFileFromDialog();
			break;

		case IDC_BUTTON_DELETE:
			_RemoveFile();
			break;

		case IDC_BUTTON_CLEAR:
			_RemoveAllFile();
			break;

		case IDC_BUTTON_ARCHIVE:
			_StartArchive();
			break;

		case ID_MENUITEM_VERSION:
		{
			std::wstring version = WINDOW_TITLE + L" " + DNH_VERSION;
			::MessageBox(hWnd_, version.c_str(), L"Version", MB_OK);
			break;
		}
		case ID_MENUITEM_HELP:
		{
			::MessageBox(hWnd_, 
				L"To add entries, use the \"Add\" button to browse or drag them directly to the view panel.\n"
				L"To delete a file entry, use the \"Delete\" button.\n"
				L"To delete all entries, use the \"Clear\" button.\n"
				L"To start archiving the selected files, use the \"Start Archive\" button and wait.", 
				L"Help", MB_OK);
			break;
		}
		}

		break;
	}

	case WM_DROPFILES:
		return _DropFiles(wParam, lParam);

	case WM_SIZE:
	{
		RECT rect;
		::GetClientRect(hWnd_, &rect);
		int wx = rect.left;
		int wy = rect.top;
		int wWidth = rect.right - rect.left;
		int wHeight = rect.bottom - rect.top;

		int leftList = 12;
		int topList = 12;
		int widthList = wWidth - 112;
		int heightList = wHeight - 76;

		::MoveWindow(GetDlgItem(hWnd_, IDC_LIST_FILE),
			leftList, topList, widthList, heightList, TRUE);

		::SetWindowPos(GetDlgItem(hWnd_, IDC_BUTTON_ADD), NULL,
			leftList + widthList + 8,
			topList, 0, 0, SWP_NOSIZE);

		::SetWindowPos(GetDlgItem(hWnd_, IDC_BUTTON_DELETE), NULL,
			leftList + widthList + 8, topList + 32,
			0, 0, SWP_NOSIZE);

		::SetWindowPos(GetDlgItem(hWnd_, IDC_BUTTON_CLEAR), NULL,
			leftList + widthList + 8, topList + 64,
			0, 0, SWP_NOSIZE);

		::SetWindowPos(GetDlgItem(hWnd_, IDC_BUTTON_ARCHIVE), NULL,
			wWidth - 148, wHeight - 54,
			0, 0, SWP_NOSIZE);

		wndStatus_.SetBounds(wParam, lParam);
		wndProgressBar_.SetBounds(leftList, wHeight - 54 + 6,
			std::max(8, wWidth - 148 - leftList - 16), 12);

		break;
	}
	}
	return _CallPreviousWindowProcedure(hWnd, uMsg, wParam, lParam);
}
BOOL MainWindow::_DropFiles(WPARAM wParam, LPARAM lParam) {
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

	buttonDecompile_.SetWindowEnable(listFile_.size() > 0);

	return FALSE;
}
void MainWindow::_AddFileFromDialog() {
	constexpr size_t maxFileCount = 128U;

	wchar_t outFileName[MAX_PATH * maxFileCount];
	ZeroMemory(&outFileName, MAX_PATH * maxFileCount);

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd_;
	ofn.nMaxFile = MAX_PATH * maxFileCount;
	ofn.lpstrFile = outFileName;
	ofn.lpstrTitle = L"Add file";
	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
	if (!GetOpenFileName(&ofn)) return;

	wchar_t* endstr = wcschr(outFileName, '\0');
	wchar_t* nextstr = endstr + 1;

	if (*nextstr == L'\0') {
		//One file selected
		std::wstring path = outFileName;
		std::wstring dirBase = PathProperty::GetFileDirectory(path);
		_AddFile(dirBase, path);
	}
	else {
		//Multiple files selected
		while (*nextstr != L'\0') {
			endstr = wcschr(nextstr, L'\0');
			std::wstring path = outFileName;
			path += L"/";
			path += nextstr;
			nextstr = endstr + 1;

			std::wstring dirBase = PathProperty::GetFileDirectory(path);
			_AddFile(dirBase, path);
		}
	}

	buttonDecompile_.SetWindowEnable(listFile_.size() > 0);
}
void MainWindow::_AddFile(std::wstring dirBase, std::wstring path) {
	dirBase = PathProperty::ReplaceYenToSlash(dirBase);
	path = PathProperty::ReplaceYenToSlash(path);

	File filePath(path);
	if (filePath.IsDirectory()) {
		path += L"/";
		WIN32_FIND_DATA data;
		HANDLE hFind;
		std::wstring findDir = path + L"*.*";
		hFind = FindFirstFile(findDir.c_str(), &data);
		do {
			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (
				wcscmp(data.cFileName, L"..") != 0 && wcscmp(data.cFileName, L".") != 0)) {
				std::wstring tDir = path + data.cFileName;
				_AddFile(dirBase, tDir);
				continue;
			}
			if (wcscmp(data.cFileName, L"..") == 0 || wcscmp(data.cFileName, L".") == 0) {
				continue;
			}

			std::wstring tName = path + data.cFileName;
			if (!_IsValidFilePath(dirBase, tName)) return;

			std::wstring filename = PathProperty::GetFileName(tName);
			std::wstring dir = _CreateRelativeDirectory(dirBase, path);

			int row = wndListFile_.GetRowCount();
			wndListFile_.SetText(row, COL_FILENAME, filename);
			wndListFile_.SetText(row, COL_DIRECTORY, dir);
			wndListFile_.SetText(row, COL_FULLPATH, tName);

			std::wstring key = dir + filename;
			listFile_.insert(key);

		} while (FindNextFile(hFind, &data) == TRUE);
		FindClose(hFind);
	}
	else {
		if (!_IsValidFilePath(dirBase, path)) return;

		std::wstring filename = PathProperty::GetFileName(path);
		std::wstring dir = _CreateRelativeDirectory(dirBase, path);

		int row = wndListFile_.GetRowCount();
		wndListFile_.SetText(row, COL_FILENAME, filename);
		wndListFile_.SetText(row, COL_DIRECTORY, dir);
		wndListFile_.SetText(row, COL_FULLPATH, path);

		std::wstring key = dir + filename;
		listFile_.insert(key);
	}
}
void MainWindow::_RemoveFile() {
	HWND hList = wndListFile_.GetWindowHandle();
	int item = -1;
	while ((item = ListView_GetNextItem(hList, item, LVNI_SELECTED)) != -1) {
		std::wstring name = wndListFile_.GetText(item, COL_FILENAME);
		std::wstring dir = wndListFile_.GetText(item, COL_DIRECTORY);

		wndListFile_.DeleteRow(item);

		std::wstring key = dir + name;
		listFile_.erase(key);
		item = -1;
	}

	buttonDecompile_.SetWindowEnable(listFile_.size() > 0);
}
void MainWindow::_RemoveAllFile() {
	wndListFile_.Clear();
	listFile_.clear();

	buttonDecompile_.SetWindowEnable(false);
}
bool MainWindow::_IsValidFilePath(const std::wstring& dirBase, const std::wstring& path) {
	if (path.size() == 0) return false;
	bool res = true;

	File file(path);
	if (file.IsDirectory()) return true;

	std::wstring key = _CreateKey(dirBase, path);
	res &= listFile_.find(key) == listFile_.end();
	if (!res)
		Logger::WriteTop(StringUtility::Format(L"File already exists. [%s]", key.c_str()));
	return res;
}
std::wstring MainWindow::_CreateKey(const std::wstring& dirBase, const std::wstring& path) {
	std::wstring name = PathProperty::GetFileName(path);
	std::wstring dir = _CreateRelativeDirectory(dirBase, path);
	std::wstring key = dir + name;
	return key;
}
std::wstring MainWindow::_CreateRelativeDirectory(const std::wstring& dirBase, const std::wstring& path) {
	std::wstring dirFull = PathProperty::GetFileDirectory(path);
	std::wstring dir = StringUtility::ReplaceAll(dirFull, dirBase, L"");
	return dir;
}
void MainWindow::_StartArchive() {
	std::wstring path;
	path.resize(MAX_PATH * 4);
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd_;
	ofn.nMaxFile = path.size();
	ofn.lpstrFile = &path[0];
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.nFilterIndex = 2;
	ofn.lpstrDefExt = L".dat";
	ofn.lpstrFilter = L"All files (*.*)\0*.*\0.dat archive(*.dat)\0*.dat\0";
	ofn.lpstrTitle = L"Creating an archive file.";
	if (GetSaveFileName(&ofn)) {
		::EnableWindow(hWnd_, TRUE);
		::EnableWindow(::GetDlgItem(hWnd_, IDC_BUTTON_ADD), FALSE);
		::EnableWindow(::GetDlgItem(hWnd_, IDC_BUTTON_DELETE), FALSE);
		::EnableWindow(::GetDlgItem(hWnd_, IDC_BUTTON_CLEAR), FALSE);
		::EnableWindow(::GetDlgItem(hWnd_, IDC_BUTTON_ARCHIVE), FALSE);
		wndListFile_.SetWindowEnable(false);
		wndProgressBar_.SetWindowVisible(true);

		HMENU hMenu = ::GetMenu(hWnd_);
		HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
		::EnableMenuItem(hSubMenu, ID_MENUITEM_ADD, MF_GRAYED);

		::SetFocus(hWnd_);
		::SetActiveWindow(hWnd_);

		pathArchive_ = path;
		Start();
	}
}

void MainWindow::_Run() {
	//アーカイブ実行
	_Archive();

	::EnableWindow(::GetDlgItem(hWnd_, IDC_EDIT_OPTION), TRUE);
	::EnableWindow(::GetDlgItem(hWnd_, IDC_BUTTON_ADD), TRUE);
	::EnableWindow(::GetDlgItem(hWnd_, IDC_BUTTON_DELETE), TRUE);
	::EnableWindow(::GetDlgItem(hWnd_, IDC_BUTTON_CLEAR), TRUE);
	::EnableWindow(::GetDlgItem(hWnd_, IDC_BUTTON_ARCHIVE), TRUE);
	wndListFile_.SetWindowEnable(true);
	wndStatus_.SetText(0, L"");
	wndProgressBar_.SetWindowVisible(false);

	HMENU hMenu = ::GetMenu(hWnd_);
	HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
	::EnableMenuItem(hSubMenu, ID_MENUITEM_ADD, MF_ENABLED);
}
void MainWindow::_Archive() {
	std::set<std::wstring> listUnArchiveExt;
	listUnArchiveExt.insert(L".png");
	listUnArchiveExt.insert(L".ogg");
	listUnArchiveExt.insert(L".jpg");
	listUnArchiveExt.insert(L".jpeg");
	listUnArchiveExt.insert(L".mp3");
	listUnArchiveExt.insert(L".zip");
	listUnArchiveExt.insert(L".rar");

	gstd::FileArchiver writer;
	int countRow = wndListFile_.GetRowCount();
	for (int iRow = 0; iRow < countRow && GetStatus() == RUN; iRow++) {
		std::wstring path = wndListFile_.GetText(iRow, COL_FULLPATH);
		std::wstring name = wndListFile_.GetText(iRow, COL_FILENAME);
		std::wstring dir = wndListFile_.GetText(iRow, COL_DIRECTORY);
		std::wstring ext = PathProperty::GetFileExtension(path);

		dir = PathProperty::ReplaceYenToSlash(dir);
		if (dir.size() > 0 && dir.back() != L'/')
			dir.push_back(L'/');

		std::shared_ptr<gstd::ArchiveFileEntry> entry = std::make_shared<gstd::ArchiveFileEntry>();
		entry->name = path;
		//entry->nameSize = path.size();
		entry->directory = dir;
		//entry->directorySize = dir.size();
		entry->compressionType = gstd::ArchiveFileEntry::CT_NONE;
		entry->sizeFull = 0U;
		entry->sizeStored = 0U;
		entry->offsetPos = 0U;
		if (listUnArchiveExt.find(ext) == listUnArchiveExt.end())
			entry->compressionType = gstd::ArchiveFileEntry::CT_ZLIB;
		writer.AddEntry(entry);
	}

	try {
		writer.CreateArchiveFile(pathArchive_, &wndStatus_, &wndProgressBar_);

		std::wstring log = StringUtility::Format(L"Archive creation successful.\n [%s]", pathArchive_.c_str());
		Logger::WriteTop(log);
		MessageBox(hWnd_, log.c_str(), L"Success", MB_OK);
	}
	catch (gstd::wexception e) {
		std::wstring log = StringUtility::Format(L"Failed to create the archive.[%s]", e.what());
		Logger::WriteTop(log);
		MessageBox(hWnd_, log.c_str(), L"Error", MB_OK);
	}

}

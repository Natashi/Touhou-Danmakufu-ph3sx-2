#include "source/GcLib/pch.h"

#include "Logger.hpp"

using namespace gstd;

/**********************************************************
//Logger
**********************************************************/
Logger* Logger::top_ = nullptr;
Logger::Logger() {

}
Logger::~Logger() {
	listLogger_.clear();
	if (top_ == this) top_ = nullptr;
}
void Logger::_WriteChild(SYSTEMTIME& time, const std::wstring& str) {
	_Write(time, str);
	for (auto itr = listLogger_.begin(); itr != listLogger_.end(); itr++) {
		(*itr)->_Write(time, str);
	}
}

void Logger::Write(const std::string& str) {
#if defined(DNH_PROJ_EXECUTOR)
	if (WindowLogger::GetParent()) {
		if (WindowLogger::GetParent()->GetState() != WindowLogger::STATE_RUNNING)
			return;
	}

	SYSTEMTIME systemTime;
	GetLocalTime(&systemTime);
	this->_WriteChild(systemTime, StringUtility::ConvertMultiToWide(str));
#endif
}
void Logger::Write(const std::wstring& str) {
#if defined(DNH_PROJ_EXECUTOR)
	if (WindowLogger::GetParent()) {
		if (WindowLogger::GetParent()->GetState() != WindowLogger::STATE_RUNNING)
			return;
	}

	SYSTEMTIME systemTime;
	GetLocalTime(&systemTime);
	this->_WriteChild(systemTime, str);
#endif
}

#if defined(DNH_PROJ_EXECUTOR)
/**********************************************************
//FileLogger
**********************************************************/
FileLogger::FileLogger() {
	sizeMax_ = 10 * 1024 * 1024;//10MB
}

FileLogger::~FileLogger() {

}
void FileLogger::Clear() {
	if (!bEnable_) return;

	File file1(path_);
	file1.Delete();
	File file2(path2_);
	file2.Delete();

	_CreateFile(file1);
}
bool FileLogger::Initialize(bool bEnable) {
	return this->Initialize(L"", bEnable);
}
bool FileLogger::Initialize(std::wstring path, bool bEnable) {
	bEnable_ = bEnable;
	if (path.size() == 0) {
		path = PathProperty::GetModuleDirectory() +
			PathProperty::GetModuleName() +
			std::wstring(L".log");
	}
	return this->SetPath(path);
}
bool FileLogger::SetPath(const std::wstring& path) {
	if (!bEnable_) return false;

	path_ = path;
	File file(path);
	if (file.IsExists() == false) {
		File::CreateFileDirectory(path);
		_CreateFile(file);
	}

	path2_ = path_ + L"_";
	return true;
}
void FileLogger::_CreateFile(File& file) {
	file.Open(File::AccessType::WRITEONLY);

	//BOM（Byte Order Mark）
	file.WriteCharacter((unsigned char)0xFF);
	file.WriteCharacter((unsigned char)0xFE);
}
void FileLogger::_Write(SYSTEMTIME& time, const std::wstring& str) {
	if (!bEnable_) return;

	{
		Lock lock(lock_);
		std::wstring strTime = StringUtility::Format(
			//L"%.4d/%.2d/%.2d "
			L"%.2d:%.2d:%.2d.%.3d ", 
			//time.wYear, time.wMonth, time.wDay, 
			time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

		File file(path_);
		if (!file.Open(File::WRITEONLY)) return;

		std::wstring out = strTime;
		out += str;
		out += L"\n";

		size_t pos = file.GetSize();
		file.SeekWrite(pos, std::ios::beg);
		file.Write(&out[0], StringUtility::GetByteSize(out));

		bool bOverSize = file.GetSize() > sizeMax_;
		file.Close();

		if (bOverSize) {
			::MoveFileEx(path_.c_str(), path2_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
			File file1(path_);
			_CreateFile(file1);
		}
	}
}


/**********************************************************
//WindowLogger
**********************************************************/
WindowLogger* WindowLogger::loggerParentGlobal_ = nullptr;
WindowLogger::WindowLogger() {
	windowState_ = STATE_INITIALIZING;
}
WindowLogger::~WindowLogger() {
	windowState_ = STATE_CLOSED;

	wndInfoPanel_ = nullptr;
	wndLogPanel_ = nullptr;
	wndStatus_ = nullptr;
	wndTab_ = nullptr;
	if (hWnd_)
		SendMessage(hWnd_, WM_ENDLOGGER, 0, 0);
	if (threadWindow_) {
		threadWindow_->Stop();
		threadWindow_->Join(2000);//念のためにタイムアウト設定
	}
}
bool WindowLogger::Initialize(bool bEnable) {
	bEnable_ = bEnable;
	//if (!bEnable) return true;

	loggerParentGlobal_ = this;

	threadWindow_ = new WindowThread(this);
	threadWindow_->Start();

	while (GetWindowHandle() == nullptr) {
		Sleep(10);//ウィンドウが作成完了するまで待機
	}

	//LogPanel
	wndLogPanel_ = new LogPanel();
	this->AddPanel(wndLogPanel_, L"Log");

	//InfoPanel
	wndInfoPanel_ = new InfoPanel();
	this->AddPanel(wndInfoPanel_, L"Info");

	windowState_ = bEnable ? STATE_RUNNING : STATE_CLOSED;

	return true;
}
void WindowLogger::SaveState() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"LogWindow.dat";
	RecordBuffer recordMain;
	bool bRecordExists = recordMain.ReadFromFile(path);

	if (wndTab_) {
		int panelIndex = wndTab_->GetCurrentPage();
		recordMain.SetRecordAsInteger("panelIndex", panelIndex);
	}

	RECT rcWnd;
	ZeroMemory(&rcWnd, sizeof(RECT));
	if (bRecordExists)
		recordMain.GetRecord("windowRect", rcWnd);
	if (IsWindowVisible())
		GetWindowRect(hWnd_, &rcWnd);
	recordMain.SetRecord("windowRect", rcWnd);

	if (wndTab_) {
		RecordBuffer recordPanel;
		int panelCount = wndTab_->GetPageCount();
		for (int iPanel = 0; iPanel < panelCount; iPanel++) {
			ref_count_ptr<WindowLogger::Panel> panel =
				ref_count_ptr<WindowLogger::Panel>::DownCast(wndTab_->GetPanel(iPanel));
			if (panel == nullptr) continue;

			panel->_WriteRecord(recordPanel);
		}
		recordMain.SetRecordAsRecordBuffer("panel", recordPanel);
	}

	recordMain.WriteToFile(path);
}
void WindowLogger::LoadState() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"LogWindow.dat";
	RecordBuffer recordMain;
	if (!recordMain.ReadFromFile(path)) return;

	int panelIndex = recordMain.GetRecordAsInteger("panelIndex");
	if (panelIndex >= 0 && panelIndex < wndTab_->GetPageCount())
		wndTab_->SetCurrentPage(panelIndex);

	RECT rcWnd;
	recordMain.GetRecord("windowRect", rcWnd);
	if (rcWnd.left >= 0 && rcWnd.right > rcWnd.left &&
		rcWnd.top >= 0 && rcWnd.bottom > rcWnd.top) {
		SetBounds(rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top);
	}

	RecordBuffer recordPanel;
	recordMain.GetRecordAsRecordBuffer("panel", recordPanel);

	int panelCount = wndTab_->GetPageCount();
	for (int iPanel = 0; iPanel < panelCount; iPanel++) {
		ref_count_ptr<WindowLogger::Panel> panel =
			ref_count_ptr<WindowLogger::Panel>::DownCast(wndTab_->GetPanel(iPanel));
		if (panel == nullptr) continue;

		panel->_ReadRecord(recordPanel);
	}
}
void WindowLogger::_CreateWindow() {
	HINSTANCE hInst = ::GetModuleHandle(nullptr);
	std::wstring wName = L"LogWindow";

	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = (WNDPROC)WindowBase::_StaticWindowProcedure;
	wcex.hInstance = hInst;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = wName.c_str();
	wcex.hIconSm = nullptr;
	RegisterClassEx(&wcex);

	hWnd_ = ::CreateWindow(wcex.lpszClassName,
		wName.c_str(),
		WS_OVERLAPPEDWINDOW,
		0, 0, 640, 480, nullptr, (HMENU)nullptr, hInst, nullptr);
	::ShowWindow(hWnd_, SW_HIDE);
	this->Attach(hWnd_);

	//タブ
	wndTab_ = new WTabControll();
	wndTab_->Create(hWnd_);
	HWND hTab = wndTab_->GetWindowHandle();

	//ステータスバー
	wndStatus_ = new WStatusBar();
	wndStatus_->Create(hWnd_);
	std::vector<int> sizeStatus;
	sizeStatus.push_back(180);
	sizeStatus.push_back(sizeStatus[0] + 560);
	wndStatus_->SetPartsSize(sizeStatus);

	//初期化完了
	this->SetBounds(32, 32, 280, 480);
	wndTab_->ShowPage();

	::UpdateWindow(hWnd_);
}
void WindowLogger::_Run() {
	_CreateWindow();
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {	//メッセージループ
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
void WindowLogger::_Write(SYSTEMTIME& systemTime, const std::wstring& str) {
	if (hWnd_ == nullptr) return;

	wchar_t timeStr[128];
	swprintf(timeStr, 
		//L"%.4d/%.2d/%.2d " 
		L"%.2d:%.2d:%.2d.%.3d ",
		//systemTime.wYear, systemTime.wMonth, systemTime.wDay,
		systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);

	std::wstring out = timeStr;
	out += str;
	out += L"\n";
	{
		Lock lock(lock_);
		wndLogPanel_->AddText(out);
	}
}
LRESULT WindowLogger::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		windowState_ = STATE_CLOSED;
		//wndLogPanel_->ClearText();
		return FALSE;
	}
	case WM_CLOSE:
	{
		SaveState();
		::ShowWindow(hWnd, SW_HIDE);
		windowState_ = STATE_CLOSED;
		wndLogPanel_->ClearText();
		return FALSE;
	}
	case WM_SIZE:
	{
		RECT rect;
		::GetClientRect(hWnd_, &rect);
		int wx = rect.left;
		int wy = rect.top;
		int wWidth = rect.right - rect.left;
		int wHeight = rect.bottom - rect.top;

		wndStatus_->SetBounds(wParam, lParam);
		wndTab_->SetBounds(wx + 8, wy + 4, wWidth - 16, wHeight - 32);
		::InvalidateRect(wndTab_->GetWindowHandle(), nullptr, TRUE);

		return FALSE;
	}
	case WM_NOTIFY:
	{
		switch (((NMHDR*)lParam)->code) {
		case TCN_SELCHANGE:
			this->ClearStatusBar();
			wndTab_->ShowPage();
			return FALSE;
		}
		break;
	}
	case WM_ENDLOGGER:
	{
		::DestroyWindow(hWnd);
		windowState_ = STATE_CLOSED;
		break;
	}
	case WM_ADDPANEL:
	{
		{
			Lock lock(lock_);
			
			for (auto itr = listEventAddPanel_.begin(); itr != listEventAddPanel_.end(); itr++) {
				AddPanelEvent& event = *itr;
				const std::wstring& name = event.name;
				ref_count_ptr<Panel> panel = event.panel;

				HWND hTab = wndTab_->GetWindowHandle();
				panel->_AddedLogger(hTab);
				wndTab_->AddTab(name, panel);

				RECT rect;
				::GetClientRect(hWnd_, &rect);
				int wx = rect.left;
				int wy = rect.top;
				int wWidth = rect.right - rect.left;
				int wHeight = rect.bottom - rect.top;
				wndTab_->SetBounds(wx + 8, wy + 4, wWidth - 16, wHeight - 32);
				wndTab_->LocateParts();
			}
			listEventAddPanel_.clear();
		}
		break;
	}
	}
	return _CallPreviousWindowProcedure(hWnd, uMsg, wParam, lParam);

}


void WindowLogger::SetInfo(int row, const std::wstring& textInfo, const std::wstring& textData) {
	if (hWnd_ == nullptr) return;
	if (!IsWindowVisible()) return;

	wndInfoPanel_->SetInfo(row, textInfo, textData);
}

bool WindowLogger::AddPanel(ref_count_ptr<Panel> panel, const std::wstring& name) {
	if (hWnd_ == nullptr) return false;

	AddPanelEvent event;
	event.name = name;
	event.panel = panel;
	{
		Lock lock(lock_);
		listEventAddPanel_.push_back(event);
	}

	::SendMessage(hWnd_, WM_ADDPANEL, 0, 0);

	while (panel->GetWindowHandle() == nullptr) {
		Sleep(10);//ウィンドウが作成完了するまで待機
	}
	return true;
}
void WindowLogger::ShowLogWindow() {
	//if (!bEnable_) return;
	windowState_ = STATE_RUNNING;
	ShowWindow(hWnd_, SW_SHOW);
}
void WindowLogger::InsertOpenCommandInSystemMenu(HWND hWnd) {
	HMENU hMenu = GetSystemMenu(hWnd, FALSE);

	MENUITEMINFO mii;
	ZeroMemory(&mii, sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);
	InsertMenuItem(hMenu, 0, 1, &mii);

	mii.fMask = MIIM_ID | MIIM_TYPE;
	mii.fType = MFT_STRING;
	mii.wID = MENU_ID_OPEN;
	mii.dwTypeData = L"Show LogWindow";

	InsertMenuItem(hMenu, 0, 1, &mii);
}
void WindowLogger::ClearStatusBar() {
	wndStatus_->SetText(0, L"");
	wndStatus_->SetText(1, L"");
}

//WindowLogger::WindowThread
WindowLogger::WindowThread::WindowThread(WindowLogger* logger) {
	_SetOuter(logger);
}
void WindowLogger::WindowThread::_Run() {
	WindowLogger* logger = _GetOuter();
	logger->_Run();
}

//WindowLogger::LogPanel
WindowLogger::LogPanel::LogPanel() {

}
WindowLogger::LogPanel::~LogPanel() {

}
bool WindowLogger::LogPanel::_AddedLogger(HWND hTab) {
	Create(hTab);

	gstd::WEditBox::Style styleEdit;
	styleEdit.SetStyle(WS_CHILD | WS_VISIBLE |
		ES_MULTILINE | ES_READONLY | ES_AUTOHSCROLL | ES_AUTOVSCROLL |
		WS_HSCROLL | WS_VSCROLL);
	styleEdit.SetStyleEx(WS_EX_CLIENTEDGE);
	wndEdit_.Create(hWnd_, styleEdit);
	return true;
}
void WindowLogger::LogPanel::LocateParts() {
	int wx = GetClientX();
	int wy = GetClientY();
	int wWidth = GetClientWidth();
	int wHeight = GetClientHeight();
	wndEdit_.SetBounds(wx, wy, wWidth, wHeight);
}
void WindowLogger::LogPanel::AddText(const std::wstring& text) {
	HWND hEdit = wndEdit_.GetWindowHandle();
	int pos = GetWindowTextLength(hEdit);
	if (pos + wndEdit_.GetTextLength() >= wndEdit_.GetMaxTextLength()) {
		std::wstring textNew = wndEdit_.GetText();
		textNew = textNew.substr(textNew.size() / 2) + L"\r\n";
		wndEdit_.SetText(textNew);

		pos = GetWindowTextLength(hEdit);
	}
	::SendMessage(hEdit, EM_SETSEL, pos, pos);
	::SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
}
void WindowLogger::LogPanel::ClearText() {
	HWND hEdit = wndEdit_.GetWindowHandle();
	::SendMessage(hEdit, EM_SETSEL, 0, -1);
	::SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)(L""));
}
//WindowLogger::InfoPanel
WindowLogger::InfoPanel::InfoPanel() {
	infoCollector_ = new InfoCollector(WindowLogger::GetParent()->GetStatusBar(), this);
	infoCollector_->Initialize();
	Start();
}
WindowLogger::InfoPanel::~InfoPanel() {
	Stop();
	Join(500);
	ptr_delete(infoCollector_);
}
bool WindowLogger::InfoPanel::_AddedLogger(HWND hTab) {
	Create(hTab);

	gstd::WListView::Style styleListView;
	styleListView.SetStyle(WS_CHILD | WS_VISIBLE |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER);
	styleListView.SetStyleEx(WS_EX_CLIENTEDGE);
	styleListView.SetListViewStyleEx(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	wndListView_.Create(hWnd_, styleListView);

	wndListView_.AddColumn(120, ROW_INFO, L"Info");
	wndListView_.AddColumn(400, ROW_DATA, L"Data");
	return true;
}
void WindowLogger::InfoPanel::LocateParts() {
	int wx = GetClientX();
	int wy = GetClientY();
	int wWidth = GetClientWidth();
	int wHeight = GetClientHeight();
	wndListView_.SetBounds(wx, wy, wWidth, wHeight);
}
void WindowLogger::InfoPanel::SetInfo(int row, const std::wstring& textInfo, const std::wstring& textData) {
	wndListView_.SetText(row, ROW_INFO, textInfo);
	wndListView_.SetText(row, ROW_DATA, textData);
}
void WindowLogger::InfoPanel::_Run() {
	while (this->GetStatus() == RUN) {
		infoCollector_->Update();
		::Sleep(500);
	}
}

//WindowLogger::InfoCollectThread
WindowLogger::InfoPanel::InfoCollector::InfoCollector(ref_count_ptr<WStatusBar> wndStatus, InfoPanel* wndInfo) {
	wndStatus_ = wndStatus;
	wndInfo_ = wndInfo;
	hProcess_ = INVALID_HANDLE_VALUE;
	hQuery_ = INVALID_HANDLE_VALUE;
	hCounter_ = INVALID_HANDLE_VALUE;
}
WindowLogger::InfoPanel::InfoCollector::~InfoCollector() {
	if (hQuery_ != INVALID_HANDLE_VALUE)
		PdhCloseQuery(&hQuery_);
	//if (hProcess_ != INVALID_HANDLE_VALUE)
	//	CloseHandle(&hProcess_);
}
void WindowLogger::InfoPanel::InfoCollector::Initialize() {
	hProcess_ = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	PdhOpenQuery(nullptr, 0, &hQuery_);
	PdhAddEnglishCounter(hQuery_, L"\\Processor(_Total)\\% Processor Time", 0, &hCounter_);
	PdhCollectQueryData(hQuery_);
}
/*
void WindowLogger::InfoPanel::InfoCollector::_Run() {
	//TODO 無効なステータスバーにメッセージを
	//     送るとき、固まる可能性あり。
	//infoCpu_ = this->_GetCpuInformation();

	while (this->GetStatus() == RUN) {
		Update();
		::Sleep(500);
	}
	
	PROCESS_MEMORY_COUNTERS memoryCounter;
	ZeroMemory(&memoryCounter, sizeof(PROCESS_MEMORY_COUNTERS));
	DWORD dwProcessID = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE ,dwProcessID);
	while(this->GetStatus() == RUN)
	{
		GetProcessMemoryInfo(hProcess, &memoryCounter, sizeof(PROCESS_MEMORY_COUNTERS));
		double pageFileUsage = memoryCounter.PagefileUsage / 1024. / 1024.;
		std::string strMemory = StringUtility::Format("Memory [ %.2fMB ]", pageFileUsage);
		if (this->GetStatus() == RUN) wndStatus_->SetText(STATUS_MEMORY, strMemory);

		double cpuPerformance=this->_GetCpuPerformance();
		CpuInfo &ci=this->infoCpu_;
		std::string strCpu = StringUtility::Format("%s %s [ %4.2fMHz (%3d %%) type:%d family:%d model:%d stepping:%d ]",ci.venderID,ci.cpuName.c_str(),ci.clock/1000/1000,(int)cpuPerformance,ci.type,ci.family,ci.model,ci.stepping);
		if (this->GetStatus() == RUN) wndStatus_->SetText(STATUS_CPU, strCpu);

		::Sleep(500);
	}
	CloseHandle(hProcess);
}
*/
void WindowLogger::InfoPanel::InfoCollector::Update() {
	if (!wndInfo_->IsWindowVisible()) return;

	{
		Lock lock(wndInfo_->lock_);

		//Get RAM info
		PROCESS_MEMORY_COUNTERS pmc;
		GetProcessMemoryInfo(hProcess_, &pmc, sizeof(pmc));
		SIZE_T ramUsageAsMB = pmc.WorkingSetSize / (1024U * 1024U);

		MEMORYSTATUSEX mse;
		mse.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&mse);
		SIZE_T ramTotalAsMB = mse.ullAvailPhys / (1024U * 1024U);

		//Get CPU info
		double cpuUsage = _GetCpuPerformance();

		//Set text
		wndStatus_->SetText(0, StringUtility::Format(L"Process RAM: %u/%u MB", ramUsageAsMB, ramTotalAsMB));
		wndStatus_->SetText(1, StringUtility::Format(L"Process CPU: %.2f%%", cpuUsage));
	}
}

double WindowLogger::InfoPanel::InfoCollector::_GetCpuPerformance() {
	PDH_FMT_COUNTERVALUE fmtValue;

	PdhCollectQueryData(hQuery_);
	PdhGetFormattedCounterValue(hCounter_, PDH_FMT_DOUBLE, nullptr, &fmtValue);

	return fmtValue.doubleValue;
}
WindowLogger::InfoPanel::InfoCollector::CpuInfo WindowLogger::InfoPanel::InfoCollector::_GetCpuInformation() {
	int cpuid_supported;
	char VenderID[13];
	char name[17];
	int eax1cpuid_supported;
	int	eax_flags;
	int	edx_flags;
	unsigned long CPUClock;
	unsigned long start;

	CpuInfo ci;
	ZeroMemory(&ci, sizeof(CpuInfo));

	try {
		/*CPUID命令をサポートしているか調べる。
		（Pentium以降のCPUならばサポートしているらしい…）
		フラグレジスタの２１ビット目を変えることができればサポートしている。*/
		_asm
		{
			pushfd; フラグレジスタの内容をスタックに保存
			pushfd; フラグレジスタの内容をスタックに入れる
			pop eax; スタックの内容をeaxレジスタに取り出す
			xor eax, 00200000h; eaxレジスタの21ビット目をビット反転、残りはそのまま
			push eax; eaxレジスタの内容をスタックに入れる
			popfd; スタックの内容をフラグレジスタに取り出す
			pushfd; フラグレジスタの内容をスタックに入れる
			pop ebx; スタックの内容をebxレジスタに取り出す
			popfd; フラグレジスタの内容を復元
			cmp eax, ebx; 比較
			je supported; 等しいならばジャンプ
			mov cpuid_supported, 0; 変数代入
			jmp exitasm; 無条件ジャンプ

			supported :
			mov cpuid_supported, 1; 変数代入
				exitasm :
		};

		if (!cpuid_supported) {//CPUID 命令をサポートしてない
			throw gstd::wexception();
		}

		/*eax=0でCPUIDを呼び出し、
		・ベンダーIDの取得（ベンダーIDはebx -> edx -> ecxの順にコピーされます）
		・ベンダーIDがAuthenticAMDならCPU名を取得
		・eax=1のときCPUID命令がサポートされているかの調査
		をします。*/
		_asm
		{
			mov	eax, 0
			cpuid
			mov DWORD PTR[VenderID + 0], ebx; 各値をバッファにコピー
			mov DWORD PTR[VenderID + 4], edx
			mov DWORD PTR[VenderID + 8], ecx
			mov BYTE PTR[VenderID + 12], 0; 最後にnullptrを入れる
			mov	eax1cpuid_supported, eax
		};

		//ベンダーIDをコピー
		strcpy(ci.venderID, VenderID);

		//VenderIDがAuthenticAMDならCPU名を取得できます。
		if (0 == strcmp(ci.venderID, "AuthenticAMD")) {
			//CPU名を取得するためにeax=08000002hでCPUIDを呼び出します。
			_asm
			{
				mov	eax, 08000002h
				cpuid
				mov DWORD PTR[name + 0], eax; 各値をバッファにコピー
				mov DWORD PTR[name + 4], ebx
				mov DWORD PTR[name + 8], ecx
				mov DWORD PTR[name + 12], edx
				mov BYTE PTR[name + 16], 0; 最後にnullptrを入れる
			};
			strcpy(ci.name, name);

			_asm
			{
				mov	eax, 08000003h
				cpuid
				mov DWORD PTR[name + 0], eax; 各値をバッファにコピー
				mov DWORD PTR[name + 4], ebx
				mov DWORD PTR[name + 8], ecx
				mov DWORD PTR[name + 12], edx
				mov BYTE PTR[name + 16], 0; 最後にnullptrを入れる
			};
			strcat(ci.name, name);

			_asm
			{
				mov	eax, 08000004h
				cpuid
				mov DWORD PTR[name + 0], eax; 各値をバッファにコピー
				mov DWORD PTR[name + 4], ebx
				mov DWORD PTR[name + 8], ecx
				mov DWORD PTR[name + 12], edx
				mov BYTE PTR[name + 16], 0; 最後にnullptrを入れる
			};
			strcat(ci.name, name);
		}

		//ax=1のときCPUID命令がサポートされているかの調査
		if (1 > eax1cpuid_supported) {
			//eaxレジスタが１の時にCPUID命令がサポートされていない
			throw gstd::wexception();
		}

		/*eaxレジスタに1を入れてCPUIDを呼び出し、
		・CPUのtype,family,Mode,stepping
		・MMX命令のサポート
		・SIMD命令のサポート
		・3DNow!のサポート
		・RDTSC命令のサポート（サポートしていればクロック周波数を測定）
		を調べます。*/
		_asm
		{
			mov	eax, 1
			cpuid
			mov eax_flags, eax
			mov	edx_flags, edx
		};

		//CPUのtype,family,Mode,steppingを調べる
		ci.type = (eax_flags >> 12) & 0xF;
		ci.family = (eax_flags >> 8) & 0xF;
		ci.model = (eax_flags >> 4) & 0xF;
		ci.stepping = eax_flags & 0xF;

		//MMX命令をサポートしているか
		ci.bMMXEnabled = (edx_flags & 0x00800000) ? true : false;

		//SIMD命令をサポートしているか
		ci.bSIMDEnabled = (edx_flags & 0x02000000) ? true : false;

		//AMD 3DNow!をサポートしているか
		ci.bAMD3DNowEnabled = (edx_flags & 0x80000000) ? true : false;

		//RDTSC命令をサポートしているか調べる
		if (edx_flags & 0x00000010) {
			//サポートしていればクロック周波数を測定
			//RDTSR命令はCPUコアのTime Stamp Counterをeaxレジスタに格納する命令です。
			//Time Stamp CounterはCPU1クロックごとにカウントアップしています。
			_asm rdtsc;
			_asm mov CPUClock, eax;
			start = timeGetTime();
			while (timeGetTime() - start < 1000);//1000ms待つ
//			Sleep(1000);
			_asm rdtsc;
			_asm sub eax, CPUClock;/*引き算*/
			_asm mov CPUClock, eax;
			ci.clock = (double)(CPUClock);
		}
		else {
			//RDTSC命令をサポートしていない。
			throw gstd::wexception();
		}

		ci.cpuName = ci.name;
		//AMD
		if (strcmp(ci.venderID, "AuthenticAMD") == 0) {
			/*
			if (ci.family == 5 && ci.model == 8) ci.cpuName = "K6-2";
			else if (ci.family == 5 && ci.model == 9) ci.cpuName = "K6-Ⅲ";
			else if (ci.family == 6 && ci.model == 1) ci.cpuName = "Athlon";
			else if (ci.family == 6 && ci.model == 2) ci.cpuName = "Athlon";
			else if (ci.family == 6 && ci.model == 3) ci.cpuName = "Athlon";
			else if (ci.family == 6 && ci.model == 4) ci.cpuName = "Athlon";
			else if (ci.family == 6 && ci.model == 6) ci.cpuName = "AthlonXp";
			*/
		}
		//Intel
		else if (strcmp(ci.venderID, "GenuineIntel") == 0) {
			/*
			if (ci.family == 5 && ci.model == 1) ci.cpuName = "Pentium 60 / 66";
			else if (ci.family == 5 && ci.model == 2) ci.cpuName = "Pentium";
			else if (ci.family == 5 && ci.model == 3) ci.cpuName = "Pentium OverDrive for 486";
			else if (ci.family == 5 && ci.model == 4) ci.cpuName = "MMX Pentium";
			else if (ci.family == 6 && ci.model == 1) ci.cpuName = "Pentium Pro";
			else if (ci.family == 6 && ci.model == 3) ci.cpuName = "PentiumⅡ";
			else if (ci.family == 6 && ci.model == 5) ci.cpuName = "PentiumⅡ/PentiumⅡXeon/Celeron";
			else if (ci.family == 6 && ci.model == 6) ci.cpuName = "Celeron";
			else if (ci.family == 6 && ci.model == 7) ci.cpuName = "PentiumⅢ/PetiumⅢXeon";
			else if (ci.family == 6 && ci.model == 8) ci.cpuName = "PentiumⅢ/PetiumⅢXeon/Celeron";
			else if (ci.family == 6 && ci.model == 9) ci.cpuName = "PetiumⅢXeon";
			else if (ci.family == 6 && ci.model == 10) ci.cpuName = "PentiumⅡOverDrive";
			else if (ci.family == 15 && ci.model == 0) ci.cpuName = "Pentium4";
			*/
		}
	}
	catch (...) {

	}
	return ci;
}

#endif

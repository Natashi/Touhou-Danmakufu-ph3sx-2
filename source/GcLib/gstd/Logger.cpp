#include "source/GcLib/pch.h"

#include "Logger.hpp"

using namespace gstd;
using namespace directx;

#define ENABLE_RENDER 1

//*******************************************************************
//Logger
//*******************************************************************
Logger* Logger::top_ = nullptr;
Logger::Logger() {
}
Logger::~Logger() {
	listLogger_.clear();
	if (top_ == this) top_ = nullptr;
}

bool Logger::Initialize() {
	return Initialize(true);
}
bool Logger::Initialize(bool bEnable) {
#if ENABLE_RENDER
	dxGraphics_.reset(new imgui::ImGuiDirectGraphics());
	dxGraphics_->SetSize(560, 600);
#endif

	if (!InitializeWindow(L"WC_LogWindow"))
		return false;

#if ENABLE_RENDER
	if (!InitializeImGui())
		return false;

	_SetImguiStyle(1);

	SetWindowTextW(hWnd_, L"LogWindow");
	MoveWindowCenter();

	SetWindowVisible(bEnable);
	bWindowActive_ = bEnable;

	{
		for (auto size : { 8, 14, 16, 20, 24 }) {
			std::string key = StringUtility::Format("Arial%d", size);
			_AddUserFont(imgui::ImGuiAddFont(key, L"Arial", size));
		}

		for (auto size : { 18, 20 }) {
			std::string key = StringUtility::Format("Arial%d_Ex", size);
			_AddUserFont(imgui::ImGuiAddFont(key, L"Arial", size, {
				{ 0x0020, 0x00FF },		// ASCII
				{ 0x2190, 0x2199 },		// Arrows
			}));
		}

		_ResetFont();
	}
#endif

	bInitialized_ = true;
	return true;
}

void Logger::_SetImguiStyle(float scale) {
#if ENABLE_RENDER
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

	ImGuiBaseWindow::_SetImguiStyle(style);
#endif
}

LRESULT Logger::_SubWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
		SetWindowVisible(false);
		return 1;

	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* minmax = (MINMAXINFO*)lParam;
		minmax->ptMinTrackSize.x = 250;
		minmax->ptMinTrackSize.y = 200;
		minmax->ptMaxTrackSize.x = ::GetSystemMetrics(SM_CXMAXTRACK);
		minmax->ptMaxTrackSize.y = ::GetSystemMetrics(SM_CYMAXTRACK);
		return 0;
	}
	case WM_SETFOCUS:
		bWindowActive_ = true;
		break;
	case WM_KILLFOCUS:
		bWindowActive_ = false;
		break;
	}

	return 0;
}

void Logger::_Update() {
	for (auto& logger : listLogger_)
		logger->Update();

	ImGuiBaseWindow::_Update();
}
void Logger::_ProcessGui() {
#if ENABLE_RENDER
	for (auto& logger : listLogger_)
		logger->ProcessGui();
#endif
}

void Logger::Write(ILogger::LogType type, const std::string& str) {
	if (!IsWindowVisible()) return;

	SYSTEMTIME systemTime;
	GetLocalTime(&systemTime);

	ILogger::LogData data = { systemTime, type, str };

	for (auto& logger : listLogger_)
		logger->Write(data);
}
void Logger::Write(ILogger::LogType type, const std::wstring& str) {
	Write(type, StringUtility::ConvertWideToMulti(str));
}

void Logger::Flush() {
	for (auto& logger : listLogger_)
		logger->Flush();
}

void Logger::ShowLogWindow() {
	SetWindowVisible(true);
}

#if defined(DNH_PROJ_EXECUTOR)
//*******************************************************************
//FileLogger
//*******************************************************************
FileLogger::FileLogger() {
	sizeMax_ = 10 * 1024 * 1024;	// 10MB
}
FileLogger::~FileLogger() {
	file_->Close();
}

bool FileLogger::Initialize(const std::string& name) {
	return Initialize(name, L"", true);
}
bool FileLogger::Initialize(const std::string& name, bool bEnable) {
	return Initialize(name, L"", bEnable);
}
bool FileLogger::Initialize(const std::string& name, const std::wstring& path, bool bEnable) {
	name_ = name;
	bEnable_ = bEnable;

	{
		auto npath = path;
		if (npath.size() == 0) {
			npath = PathProperty::GetModuleDirectory() +
				PathProperty::GetModuleName() +
				std::wstring(L".log");
		}
		if (!SetPath(npath))
			return false;
	}

	return true;
}

bool FileLogger::SetPath(const std::wstring& path) {
	if (!bEnable_) return false;

	path_ = path;
	File::CreateFileDirectory(path_);

	_CreateFile();

	return file_->IsOpen();
}
void FileLogger::_CreateFile() {
	file_ = shared_ptr<File>(new File(path_));
	if (file_->Open(File::WRITEONLY)) {
		// BOM for UTF-16 LE
		file_->WriteCharacter((byte)0xFF);
		file_->WriteCharacter((byte)0xFE);
	}
}

void FileLogger::Write(const LogData& data) {
	if (!bEnable_) return;

	if (file_->IsOpen()) {
		Lock lock(Logger::GetTop()->GetLock());

		std::wstring strTime = StringUtility::Format(
			L"%.2d:%.2d:%.2d.%.3d ",
			data.time.wHour, data.time.wMinute, 
			data.time.wSecond, data.time.wMilliseconds);

		file_->WriteString(strTime);
		file_->WriteString(data.text);
		file_->WriteString(std::wstring(L"\n"));
	}
}

void FileLogger::Flush() {
	if (!bEnable_) return;

	if (file_->IsOpen()) {
		std::fstream& hFile = file_->GetFileHandle();
		hFile.flush();
	}
}

//*******************************************************************
//WindowLogger
//*******************************************************************
WindowLogger::WindowLogger() {
}
WindowLogger::~WindowLogger() {
}

bool WindowLogger::Initialize(const std::string& name) {
	return Initialize(name, true);
}
bool WindowLogger::Initialize(const std::string& name, bool bEnable) {
	name_ = name;
	bEnable_ = bEnable;

	panelEventLog_.reset(new PanelEventLog());
	panelEventLog_->SetDisplayName("Log");
	this->AddPanel(panelEventLog_, "Log");

	panelInfo_.reset(new PanelInfo());
	panelInfo_->SetDisplayName("Info");
	this->AddPanel(panelInfo_, "Info");

	return true;
}

void WindowLogger::SaveState() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"LogWindow.dat";

	RecordBuffer recordMain;
	bool bRecordExists = recordMain.ReadFromFile(path, 0, HEADER_RECORDFILE);

	/*
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
			WindowLogger::Panel* panel = (WindowLogger::Panel*)(wndTab_->GetPanel(iPanel).get());
			if (panel == nullptr) continue;

			panel->_WriteRecord(recordPanel);
		}
		recordMain.SetRecordAsRecordBuffer("panel", recordPanel);
	}
	*/

	recordMain.WriteToFile(path, 0, HEADER_RECORDFILE);
}
void WindowLogger::LoadState() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"LogWindow.dat";

	RecordBuffer recordMain;
	if (!recordMain.ReadFromFile(path, 0, HEADER_RECORDFILE)) return;

	/*
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
		WindowLogger::Panel* panel = (WindowLogger::Panel*)(wndTab_->GetPanel(iPanel).get());
		if (panel == nullptr) continue;

		panel->_ReadRecord(recordPanel);
	}
	*/
}

void WindowLogger::ProcessGui() {
	Logger* parent = Logger::GetTop();
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	ImGui::PushFont(parent->GetFont("Arial16"));

	if (ImGui::Begin("MainWindow", nullptr, flags)) {
		float ht = ImGui::GetContentRegionAvail().y - 16;

		//ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGuiWindowFlags window_flags = 0;
		if (ImGui::BeginChild("ChildW_Tabs", ImVec2(0, ht), false, window_flags)) {
			ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton;
			if (ImGui::BeginTabBar("TabBar_Tabs", tab_bar_flags)) {
				currentPanel_ = nullptr;
				for (auto& iPanel : panels_) {
					iPanel->SetActive(false);

					if (ImGui::BeginTabItem(iPanel->GetDisplayName().c_str())) {
						iPanel->SetActive(true);
						currentPanel_ = iPanel;

						iPanel->ProcessGui();
						ImGui::EndTabItem();
					}
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::EndChild();

		ImGui::End();
	}

	ImGui::PopFont();
}

void WindowLogger::Write(const LogData& data) {
	{
		Lock lock(Logger::GetTop()->GetLock());

		panelEventLog_->AddEvent(data);
	}
}

bool WindowLogger::AddPanel(shared_ptr<ILoggerPanel> panel, const std::string& name) {
	panel->Initialize(name);

	{
		Lock lock(Logger::GetTop()->GetLock());

		panels_.push_back(panel);
	}

	return true;
}

//WindowLogger::LogPanel
WindowLogger::PanelEventLog::PanelEventLog() {
}
WindowLogger::PanelEventLog::~PanelEventLog() {
}

void WindowLogger::PanelEventLog::Initialize(const std::string& name) {
	ILoggerPanel::Initialize(name);
}

void WindowLogger::PanelEventLog::ProcessGui() {

}

void WindowLogger::PanelEventLog::AddEvent(const LogData& data) {
	static const wchar_t* strInfo = L"";
	static const wchar_t* strWarning = L"[WARNING] ";
	static const wchar_t* strError = L"[ERROR]   ";

	const wchar_t* initial = strInfo;
	switch (data.type) {
	case LogType::Warning:	initial = strWarning; break;
	case LogType::Error:	initial = strError; break;
	}

	std::string strLog = StringUtility::Format(
		"%s%.2d:%.2d:%.2d.%.3d %s",
		initial,
		data.time.wHour, data.time.wMinute, data.time.wSecond, data.time.wMilliseconds,
		data.text.c_str());

	events_.push_back({ data.type, strLog });
}
void WindowLogger::PanelEventLog::ClearEvents() {
	events_.clear();
}

//WindowLogger::PanelInfo
WindowLogger::PanelInfo::PanelInfo() {
	hProcess_ = INVALID_HANDLE_VALUE;
	hQuery_ = INVALID_HANDLE_VALUE;
	hCounter_ = INVALID_HANDLE_VALUE;

	_InitializeHandle();
}
WindowLogger::PanelInfo::~PanelInfo() {
	if (hQuery_ != INVALID_HANDLE_VALUE)
		PdhCloseQuery(&hQuery_);
	//if (hProcess_ != INVALID_HANDLE_VALUE)
	//	CloseHandle(&hProcess_);
}

void WindowLogger::PanelInfo::Initialize(const std::string& name) {
	ILoggerPanel::Initialize(name);
}

void WindowLogger::PanelInfo::Update() {
	_SetRamInfo();

	Logger::GetTop()->Flush();
}
void WindowLogger::PanelInfo::ProcessGui() {
	
}

void WindowLogger::PanelInfo::SetInfo(int row, const std::string& textInfo, const std::string& textData) {
	infoLines_[row] = { textInfo, textData };
}

void WindowLogger::PanelInfo::_SetRamInfo() {
	if (!bActive_) return;
	{
		Lock lock(Logger::GetTop()->GetLock());

		// Get RAM info
		MEMORYSTATUSEX mse;
		mse.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&mse);

		ramMemAvail_ = mse.ullAvailVirtual / (1024ui64 * 1024ui64);
		ramMemMax_ = mse.ullTotalVirtual / (1024ui64 * 1024ui64);
		
		// Get CPU info
		rateCpuUsage_ = _GetCpuPerformance();;
	}
}

void WindowLogger::PanelInfo::_InitializeHandle() {
	hProcess_ = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	PdhOpenQueryW(nullptr, 0, &hQuery_);
	PdhAddEnglishCounterW(hQuery_, L"\\Processor(_Total)\\% Processor Time", 0, &hCounter_);
	PdhCollectQueryData(hQuery_);
}
double WindowLogger::PanelInfo::_GetCpuPerformance() {
	PDH_FMT_COUNTERVALUE fmtValue;

	PdhCollectQueryData(hQuery_);
	PdhGetFormattedCounterValue(hCounter_, PDH_FMT_DOUBLE, nullptr, &fmtValue);

	return fmtValue.doubleValue;
}

#if 0
WindowLogger::PanelInfo::CpuInfo WindowLogger::PanelInfo::_GetCpuInformation() {
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

#endif
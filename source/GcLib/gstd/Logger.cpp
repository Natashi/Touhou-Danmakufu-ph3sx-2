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
		for (auto size : { 8, 10, 14, 20, 24 }) {
			auto key = STR_FMT("Arial%d", size);
			_AddUserFont(imgui::ImGuiAddFont(key, L"Arial", size));
		}
		for (auto size : { 15, 16 }) {
			auto key = STR_FMT("Arial%d", size);
			_AddUserFont(imgui::ImGuiAddFont(key, L"Arial", size, {
				{ 0x0020, 0x00FF },		// ASCII
				{ 0x0370, 0x03FF },		// Greek and Coptic
			}));
		}

		for (auto size : { 18, 20 }) {
			auto key = STR_FMT("Arial%d_Ex", size);
			_AddUserFont(imgui::ImGuiAddFont(key, L"Arial", size, {
				{ 0x0020, 0x00FF },		// ASCII
				{ 0x2190, 0x2199 },		// Arrows
			}));
		}

		for (auto size : { 16 }) {
			auto key = STR_FMT("Arial%d_U", size);
			_AddUserFont(imgui::ImGuiAddFont(key, L"Arial", size, {
				{ 0x0020, 0x00FF },		// ASCII
				{ 0x0100, 0x024F },		// Latin Extended A+B
				{ 0x0250, 0x02AF },		// IPA Extensions
				{ 0x0370, 0x03FF },		// Greek and Coptic
				{ 0x0400, 0x04FF },		// Cyrillic
				{ 0x0590, 0x06FF },		// Hebrew + Arabic
				{ 0x0900, 0x109F },		// Various South Asian Scripts
				{ 0x16A0, 0x16FF },		// Runic
				{ 0x1780, 0x17FF },		// Khmer
				{ 0x0400, 0x04FF },		// Cyrillic
				{ 0x2000, 0x28FF },		// Special Symbols
				{ 0x2000, 0x28FF },		// Punctuations
				{ 0x2E80, 0x2FFF },		// CJK Radicals
				{ 0x3000, 0x312F },		// CJK Symbols, Japanese Kana
				{ 0x4E00, 0x9FFF },		// CJK Unified
				{ 0xFF00, 0xFFEF },		// Half-Width
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
	//if (!IsWindowVisible()) return;

	SYSTEMTIME systemTime;
	GetLocalTime(&systemTime);

	ILogger::LogData data = { systemTime, type, str };

	for (auto& logger : listLogger_)
		logger->Write(data);
}
void Logger::Write(ILogger::LogType type, const std::wstring& str) {
	//if (!IsWindowVisible()) return;

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
		bool res;

		if (path.size() == 0) {
			auto nPath = PathProperty::GetModuleName() + std::wstring(L".log");
			res = SetPath(nPath);
		}
		else {
			res = SetPath(path);
		}

		if (!res)
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
	bTabInit_ = false;

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
	recordMain.ReadFromFile(path, 0, RecordBuffer::HEADER);

	if (currentPanel_) {
		recordMain.SetRecordAsStringA("panelIndex", currentPanel_->GetName());
	}

	{
		Logger* parent = Logger::GetTop();
		HWND hWnd = parent->GetWindowHandle();

		RECT rc{};
		::GetWindowRect(hWnd, &rc);

		recordMain.SetRecord("windowRect", rc);
	}

	/*
	{
		RecordBuffer recordPanel;

		for (auto& iPanel : panels_) {
			iPanel->WriteRecord(recordPanel);
		}

		recordMain.SetRecordAsRecordBuffer("panel", recordPanel);
	}
	*/

	recordMain.WriteToFile(path, 0, RecordBuffer::HEADER);
}
void WindowLogger::LoadState() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"LogWindow.dat";

	RecordBuffer recordMain;
	if (!recordMain.ReadFromFile(path, 0, RecordBuffer::HEADER))
		return;

	if (auto lastPanel = recordMain.GetRecordAsStringA("panelIndex")) {
		iniPanel_ = *lastPanel;
	}

	{
		if (auto oRect = recordMain.GetRecordAs<RECT>("windowRect")) {
			auto& rc = *oRect;

			Logger* parent = Logger::GetTop();
			HWND hWnd = parent->GetWindowHandle();

			::MoveWindow(hWnd, rc.left, rc.top,
				rc.right - rc.left, rc.bottom - rc.top, true);
		}
	}
}

void WindowLogger::ProcessGui() {
	Logger* parent = Logger::GetTop();
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration 
		| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	ImGui::PushFont(parent->GetFont("Arial16"));

	if (ImGui::Begin("MainWindow", nullptr, flags)) {
		//ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGuiWindowFlags window_flags = 0;
		if (ImGui::BeginChild("ChildW_Tabs", ImVec2(0, 0), false, window_flags)) {
			constexpr ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton;
			constexpr ImGuiTabItemFlags tab_item_flags = ImGuiTabItemFlags_NoCloseWithMiddleMouseButton
				| ImGuiTabItemFlags_NoTooltip | ImGuiTabItemFlags_NoReorder;

			if (ImGui::BeginTabBar("TabBar_Tabs", tab_bar_flags)) {
				currentPanel_ = nullptr;
				for (auto& iPanel : panels_) {
					ImGuiTabItemFlags _tab_item_flags = tab_item_flags;
					if (!bTabInit_) {
						if (iniPanel_ == iPanel->GetName()) {
							_tab_item_flags |= ImGuiTabItemFlags_SetSelected;
							bTabInit_ = true;
						}
					}

					if (ImGui::BeginTabItem(iPanel->GetDisplayName().c_str(), nullptr, _tab_item_flags)) {
						iPanel->SetActive(true);

						currentPanel_ = iPanel;

						ImGui::Dummy(ImVec2(0, 2));

						ImGui::Separator();
						iPanel->ProcessGui();

						ImGui::EndTabItem();
					}
					else {
						iPanel->SetActive(false);
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
	bLogChanged_ = false;
}
WindowLogger::PanelEventLog::~PanelEventLog() {
}

void WindowLogger::PanelEventLog::Initialize(const std::string& name) {
	ILoggerPanel::Initialize(name);

	filter_.resize(256);

	timeStart_ = stdch::high_resolution_clock::now();
}

void WindowLogger::PanelEventLog::Update() {
	if (bLogChanged_) {
		// Copy logs for rendering, can't happen at the same time as AddEvent 
		Lock lock(Logger::GetTop()->GetLock());

		eventsCopy_ = std::vector<LogEntry>(events_.begin(), events_.end());

		bLogChanged_ = false;
	}
}
void WindowLogger::PanelEventLog::ProcessGui() {
	Logger* parent = Logger::GetTop();

	bool bClear = ImGui::Button("Clear");
	ImGui::SameLine();

	bool bCopy = ImGui::Button("Copy");
	ImGui::SameLine();

	// TODO: Maybe implement filtering
	ImGui::SetNextItemWidth(-100);
	bool bFilterTextChanged = ImGui::InputText("Filter", filter_.data(), filter_.size() - 1);

	ImGui::Separator();

	float ht = ImGui::GetContentRegionAvail().y - 32;

	if (ImGui::BeginChild("plog_child_logscr", ImVec2(0, ht), false, ImGuiWindowFlags_HorizontalScrollbar)) {
		static size_t prevCount = 0;

		bool bScrollToBottom = false;
		if (events_.size() > prevCount) {
			bScrollToBottom = true;
			prevCount = events_.size();
		}

		if (bClear) {
			ClearEvents();
		}
		if (bCopy) {
			CopyEventsToClipboard();
		}

		ImGui::PushFont(parent->GetFont("Arial16_U"));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		float htItem = ImGui::GetTextLineHeightWithSpacing();

		{
			const ImVec4 colorInfo  = ImColor(0, 0, 64);
			const ImVec4 colorWarn  = ImColor(255, 128, 0);
			const ImVec4 colorError = ImColor(255, 24, 24);
			const ImVec4 colorUser1 = ImColor(0, 32, 192);
			const char* strWarning  = "[warn]";
			const char* strError    = "[error]";

			size_t count = eventsCopy_.size();

			// Clip display to only visible items
			// TODO: CalcListClipping doesn't work properly due to some entries spanning multiple lines

			int dispStart = 0, dispEnd = count;
			constexpr bool bClipping = false;

			if constexpr (bClipping) {
				ImGui::CalcListClipping(count, htItem, &dispStart, &dispEnd);

				dispStart = std::max(0, dispStart - 3);
				dispEnd = std::min<int>(count, dispEnd + 1);

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + dispStart * htItem);
			}

			for (size_t i = dispStart; i < dispEnd; ++i) {
				const LogEntry& entry = eventsCopy_[i];

				ImVec4 color = ImColor(0, 0, 64);
				const char* label = "";
				bool addIndent = false;

				switch (entry.type) {
				case LogType::Warning:
					color = colorWarn;
					label = strWarning;
					addIndent = true;
					break;
				case LogType::Error:
					color = colorError;
					label = strError;
					addIndent = true;
					break;
				case LogType::User1:
					color = colorUser1;
					break;
				}

				ImGui::PushStyleColor(ImGuiCol_Text, color);

				ImGui::TextUnformatted(entry.time.c_str());
				ImGui::SameLine(92);

				if (addIndent) {
					ImGui::TextUnformatted(label);
					ImGui::SameLine(92 + 48);
				}
				ImGui::TextUnformatted(entry.text.c_str());

				ImGui::PopStyleColor();
			}

			if constexpr (bClipping) {
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (count - dispEnd) * htItem);
			}
		};

		ImGui::PopStyleVar();
		ImGui::PopFont();

		// Scroll to the bottom when getting new events
		if (bScrollToBottom)
			ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();

	ImGui::Dummy(ImVec2(0, 4));
	ImGui::Indent(6);

	{
		auto elapsedMs = stdch::duration_cast<stdch::milliseconds>(
			stdch::high_resolution_clock::now() - timeStart_).count();

		size_t s = elapsedMs / 1000;
		size_t m = s / 60;
		size_t h = m / 60;

		ImGui::Text("Time elapsed: %u:%u:%u.%04u", h, m % 60, s % 60, elapsedMs % 1000);
	}
}

void WindowLogger::PanelEventLog::AddEvent(const LogData& data) {
	LogEntry entry;
	entry.type = data.type;
	entry.time = StringUtility::Format(
		"%.2d:%.2d:%.2d.%.3d",
		data.time.wHour, data.time.wMinute, data.time.wSecond, data.time.wMilliseconds);
	entry.text = data.text;

	if (events_.size() >= 1000) {
		events_.pop_front();
	}
	events_.push_back(entry);

	bLogChanged_ = true;
}
void WindowLogger::PanelEventLog::ClearEvents() {
	events_.clear();
	bLogChanged_ = true;
}

void WindowLogger::PanelEventLog::CopyEventsToClipboard() {
	std::wstring strAll;
	for (auto& event : events_) {
		strAll += STR_WIDE(event.time + " " + event.text + "\r\n");
	}
	strAll.push_back(0);

	using StrChType = decltype(strAll)::traits_type::char_type;
	size_t strSizeInBytes = strAll.size() * sizeof(StrChType);

	if (::OpenClipboard(NULL)) {
		HGLOBAL hAlloc = ::GlobalAlloc(GMEM_MOVEABLE, strSizeInBytes);
		if (hAlloc) {
			// Lock memory and copy string data
			{
				StrChType* dataLock = reinterpret_cast<StrChType*>(::GlobalLock(hAlloc));
				memcpy(dataLock, strAll.data(), strSizeInBytes);
				::GlobalUnlock(hAlloc);
			}

			::SetClipboardData(CF_UNICODETEXT, hAlloc);
		}

		::CloseClipboard();
	}
}

//WindowLogger::PanelInfo
WindowLogger::PanelInfo::PanelInfo() {
	hProcess_ = INVALID_HANDLE_VALUE;
	hQuery_ = INVALID_HANDLE_VALUE;
	hCounter_ = INVALID_HANDLE_VALUE;

	rateCpuUsage_ = 0;
}
WindowLogger::PanelInfo::~PanelInfo() {
	if (hQuery_ != INVALID_HANDLE_VALUE)
		PdhCloseQuery(&hQuery_);
	//if (hProcess_ != INVALID_HANDLE_VALUE)
	//	CloseHandle(&hProcess_);
}

void WindowLogger::PanelInfo::Initialize(const std::string& name) {
	ILoggerPanel::Initialize(name);

	_InitializeHandle();
}

void WindowLogger::PanelInfo::Update() {
	Logger::GetTop()->Flush();
}
void WindowLogger::PanelInfo::ProcessGui() {
	float ht = ImGui::GetContentRegionAvail().y - 32;

	if (ImGui::BeginChild("pinfo_child_table", ImVec2(0, ht), false, ImGuiWindowFlags_HorizontalScrollbar)) {
		ImGuiTableFlags flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable
			| ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingStretchProp;

		if (ImGui::BeginTable("pinfo_table", 2, flags)) {
			ImGui::TableSetupScrollFreeze(1, 1);
			ImGui::TableSetupColumn("Info", 
				ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed, 128);
			ImGui::TableSetupColumn("Data");
			ImGui::TableHeadersRow();

			auto itr = infoLines_.begin();
			for (size_t iRow = 0; itr != infoLines_.end(); ++iRow) {
				ImGui::TableNextRow();

				auto _SetRow = [](const char* s0, const char* s1) {
					ImGui::TableSetColumnIndex(0);
					ImGui::Text(s0);

					if (ImGui::TableSetColumnIndex(1))
						ImGui::Text(s1);
				};

				if (iRow == itr->first) {
					_SetRow(itr->second[0].c_str(), itr->second[1].c_str());
					++itr;
				}
				else {
					_SetRow("", "");
				}
			}

			ImGui::EndTable();
		}
	}
	ImGui::EndChild();

	ImGui::Dummy(ImVec2(0, 4));
	ImGui::Indent(6);
}

void WindowLogger::PanelInfo::SetInfo(int row, const std::string& textInfo, const std::string& textData) {
	infoLines_[row] = { textInfo, textData };
}

void WindowLogger::PanelInfo::_GetRamInfo() {
	if (!bActive_) return;
	{
		// Get RAM info
		MEMORYSTATUSEX mse = {};
		mse.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&mse);

		ramMemAvail_ = mse.ullAvailVirtual / (1024ui64 * 1024ui64);
		ramMemMax_ = mse.ullTotalVirtual / (1024ui64 * 1024ui64);
		
		// Get CPU info
		rateCpuUsage_ = std::clamp<double>(_GetCpuPerformance(), 0, 100);
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
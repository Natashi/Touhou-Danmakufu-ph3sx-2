#include "source/GcLib/pch.h"

#include "SystemPanel.hpp"

#include "DirectGraphics.hpp"
#include "ImGuiWindow.hpp"

#include "../../TouhouDanmakufu/Common/DnhCommon.hpp"

using namespace gstd;
using namespace directx;

//****************************************************************************
//SystemInfoPanel
//****************************************************************************
SystemInfoPanel::SystemInfoPanel() {
	d3dQueryCollecting = false;
}
SystemInfoPanel::~SystemInfoPanel() {
	for (auto& iQuery : d3dQueries)
		ptr_release(iQuery);
}

void SystemInfoPanel::Initialize(const std::string& name) {
	ILoggerPanel::Initialize(name);

	_InitializeD3D();

	{
		hOwnProcess = ::GetCurrentProcess();
		::GetSystemInfo(&sysInfo);

		osInfo.dwOSVersionInfoSize = sizeof(osInfo);
		::GetVersionExA((LPOSVERSIONINFOA)&osInfo);

		{
#if _WIN64
			is64bit = true;
#elif _WIN32
			using LPFN_ISWOW64PROCESS = BOOL(WINAPI*)(HANDLE, PBOOL);

			HMODULE hModule = ::GetModuleHandleW(L"kernel32");
			if (hModule) {
				auto fnIsWow64Process = (LPFN_ISWOW64PROCESS)
					::GetProcAddress(hModule, "IsWow64Process");
				if (fnIsWow64Process) {
					fnIsWow64Process(hOwnProcess, &is64bit);
				}
			}
#else
			is64bit = false;
#endif
		}
	}

	_InitializeWMI();

	_InitializeCounters();
}

void SystemInfoPanel::_InitializeD3D() {
	IDirect3D9* pD3d = EDirect3D9::GetInstance()->GetD3D();

	DirectGraphics* graphics = DirectGraphics::GetBase();
	if (graphics == nullptr)
		return;
	IDirect3DDevice9* device = graphics->GetDevice();

	pD3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &d3dAdapterInfo);
	pD3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3dDisplayMode);

	CreateD3DQueries();
}
void SystemInfoPanel::_InitializeWMI() {
	WmiQuery wmi;
	if (wmi.Initialize()) {
		if (auto query = wmi.MakeQuery(L"Win32_ComputerSystem")) {
			if (query->Count() > 0) {
				SystemData::System system{};

				if (auto v = query->Get(0, L"Manufacturer")) {
					system.manufacturer = STR_MULTI((*v)->bstrVal);
				}
				if (auto v = query->Get(0, L"Model")) {
					system.model = STR_MULTI((*v)->bstrVal);
				}
				if (auto v = query->Get(0, L"Name")) {
					system.name = STR_MULTI((*v)->bstrVal);
				}

				wmiData.system = system;
			}
		}
		if (auto query = wmi.MakeQuery(L"Win32_BIOS")) {
			if (query->Count() > 0) {
				SystemData::Bios bios{};

				if (auto v = query->Get(0, L"Manufacturer")) {
					bios.manufacturer = STR_MULTI((*v)->bstrVal);
				}
				if (auto v = query->Get(0, L"Name")) {
					bios.name = STR_MULTI((*v)->bstrVal);
				}

				wmiData.bios = bios;
			}
		}
		if (auto query = wmi.MakeQuery(L"Win32_OperatingSystem")) {
			if (query->Count() > 0) {
				SystemData::OS os{};

				if (auto v = query->Get(0, L"Version")) {
					auto nums = StringUtility::Split(STR_MULTI((*v)->bstrVal), ".");
					os.major = std::stoi(nums[0]);
					os.minor = std::stoi(nums[1]);
					os.build = std::stoi(nums[2]);
				}

				wmiData.os = os;
			}
		}
		if (auto query = wmi.MakeQuery(L"Win32_SoundDevice")) {
			for (size_t i = 0; i < query->Count(); ++i) {
				SystemData::SoundDevice sd{};

				if (auto v = query->Get(i, L"Manufacturer")) {
					sd.manufacturer = STR_MULTI((*v)->bstrVal);
				}
				if (auto v = query->Get(i, L"Name")) {
					sd.name = STR_MULTI((*v)->bstrVal);
				}

				wmiData.soundDevices.push_back(sd);
			}
		}
		if (auto query = wmi.MakeQuery(L"Win32_VideoController")) {
			for (size_t i = 0; i < query->Count(); ++i) {
				SystemData::VideoController vc{};

				if (auto v = query->Get(i, L"Name")) {
					vc.name = STR_MULTI((*v)->bstrVal);
				}
				if (auto v = query->Get(i, L"AdapterCompatibility")) {
					vc.compatibility = STR_MULTI((*v)->bstrVal);
				}
				if (auto v = query->Get(i, L"AdapterRAM")) {
					vc.ram = (*v)->ullVal;
				}
				if (auto v = query->Get(i, L"DriverVersion")) {
					vc.driverVersion = STR_MULTI((*v)->bstrVal);
				}

				wmiData.videoControllers.push_back(vc);
			}
		}
	}
}
void SystemInfoPanel::_InitializeCounters() {
	hQueryProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, ::GetProcessId(hOwnProcess));
	::PdhOpenQueryW(nullptr, 0, &hQuery);

	auto _MakeCounter = [&](const std::wstring& name) -> Counter {
		HCOUNTER counter = nullptr;
		::PdhAddEnglishCounterW(hQuery, name.c_str(), 0, &counter);
		return { name, counter };
	};

	counterThermalZone = _MakeCounter(L"\\Thermal Zone Information(*)\\Temperature");

	{
		auto _MakeProcessorCounter = [&](const std::wstring& core) -> CounterProcessor {
			CounterProcessor res;
			res.counters[0] = _MakeCounter(STR_FMT(L"\\Processor Information(%s)\\Processor Frequency", core.c_str()));
			res.counters[1] = _MakeCounter(STR_FMT(L"\\Processor Information(%s)\\%% of Maximum Frequency", core.c_str()));
			res.counters[2] = _MakeCounter(STR_FMT(L"\\Processor Information(%s)\\%% Processor Time", core.c_str()));
			return res;
		};

		processors.push_back(_MakeProcessorCounter(L"_Total"));
		for (size_t i = 0; i < sysInfo.dwNumberOfProcessors; ++i) {
			processors.push_back(_MakeProcessorCounter(
				STR_FMT(L"0,%d", i)));
		}
	}

	PdhCollectQueryData(hQuery);
}

void SystemInfoPanel::Update() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	if (graphics == nullptr)
		return;
	IDirect3DDevice9* device = graphics->GetDevice();

	// Get counter values
	{
		PDH_FMT_COUNTERVALUE value;

		PdhCollectQueryData(hQuery);

		{
			PdhGetFormattedCounterValue(counterThermalZone.handle, PDH_FMT_DOUBLE, nullptr, &value);
			valueThermalZone = value.doubleValue;
		}
		{
			auto _CollectProcessorData = [&](CounterProcessor& processor) {
				PdhGetFormattedCounterValue(processor.counters[0].handle, PDH_FMT_DOUBLE, nullptr, &value);
				processor.maxFreq = value.doubleValue;

				PdhGetFormattedCounterValue(processor.counters[1].handle, PDH_FMT_DOUBLE, nullptr, &value);
				processor.rateFreq = value.doubleValue / 100.0;

				PdhGetFormattedCounterValue(processor.counters[2].handle, PDH_FMT_DOUBLE, nullptr, &value);
				processor.time = value.doubleValue;

				processor.AddHistory(processor.time);
			};

			for (auto& i : processors) {
				_CollectProcessorData(i);
			}
		}
	}

	// Get RAM info
	{
		memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&memoryStatus);
	}
}

static constexpr size_t CPU_HISTORY_DISPLAY_MAX = 80;
void SystemInfoPanel::CounterProcessor::AddHistory(double value) {
	if (history.size() >= CPU_HISTORY_DISPLAY_MAX * 2) {
		std::vector<double> newHistory;
		newHistory.reserve(CPU_HISTORY_DISPLAY_MAX * 2);

		newHistory.insert(newHistory.end(),
			history.begin() + CPU_HISTORY_DISPLAY_MAX, history.end());

		history = MOVE(newHistory);
	}
	history.push_back(value);
}

void SystemInfoPanel::CreateD3DQueries() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	if (graphics == nullptr)
		return;
	IDirect3DDevice9* device = graphics->GetDevice();

	d3dQueries.clear();

	/*
	std::vector<D3DQUERYTYPE> queryTypes = {
		D3DQUERYTYPE_BANDWIDTHTIMINGS,
		D3DQUERYTYPE_CACHEUTILIZATION,
		D3DQUERYTYPE_PIPELINETIMINGS,
		D3DQUERYTYPE_VERTEXSTATS,
	};

	queries.resize(queryTypes.size());

	for (size_t i = 0; i < queryTypes.size(); ++i) {
		HRESULT hr = device->CreateQuery(queryTypes[i], &queries[i]);
	}
	*/
}
void SystemInfoPanel::StartD3DQuery() {
	if (d3dQueryCollecting) return;

	for (auto& iQuery : d3dQueries) {
		iQuery->Issue(D3DISSUE_BEGIN);
	}
	d3dQueryCollecting = true;
}
void SystemInfoPanel::EndD3DQuery() {
	if (!d3dQueryCollecting) return;

	for (auto& iQuery : d3dQueries) {
		iQuery->Issue(D3DISSUE_END);
	}

	/*
	D3dQueryStats stats = {};

	while (queries[0]->GetData(&stats.bandwidthTimings, sizeof(D3DDEVINFO_D3D9BANDWIDTHTIMINGS), D3DGETDATA_FLUSH) == S_FALSE);
	while (queries[1]->GetData(&stats.cacheUtilization, sizeof(D3DDEVINFO_D3D9CACHEUTILIZATION), D3DGETDATA_FLUSH) == S_FALSE);
	while (queries[2]->GetData(&stats.pipelineTimings, sizeof(D3DDEVINFO_D3D9PIPELINETIMINGS), D3DGETDATA_FLUSH) == S_FALSE);
	while (queries[3]->GetData(&stats.vertexStats, sizeof(D3DDEVINFO_D3DVERTEXSTATS), D3DGETDATA_FLUSH) == S_FALSE);

	d3dQueryStats = stats;
	*/

	d3dQueryCollecting = false;
}

static const char* GetOSName(int major, int minor, int build) {
	switch (major) {
	case 10:
		if (build >= 22000)
			return "Windows 11";
		else
			return "Windows 10";
		break;
	case 6:
		switch (minor) {
		case 3:
			return "Windows 8.1";
		case 2:
			return "Windows 8";
		case 1:
			return "Windows 7";
		case 0:
			return "Windows Vista";
		}
		break;
	case 5:
		switch (minor) {
		case 2:
			return "Windows XP Professional x64";
		case 1:
			return "Windows XP";
		case 0:
			return "Windows 2000";
		}
		break;
	}
	return "Unknown";
}

void SystemInfoPanel::ProcessGui() {
	Logger* parent = Logger::GetTop();

	auto& cpuInfo = SystemUtility::GetCpuInfo();

	float wd = ImGui::GetContentRegionAvail().x;
	float ht = ImGui::GetContentRegionAvail().y;

	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImU32)ImColor(32, 144, 255));
	ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImU32)ImColor(32, 144, 255));

	if (ImGui::BeginChild("psysinfo_child_table", ImVec2(0, ht), false, ImGuiWindowFlags_HorizontalScrollbar)) {
		constexpr float indent = 20;

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("System")) {
			ImGui::Indent(indent);
			{
				{
					ImGui::TextUnformatted("OS:");
					ImGui::SameLine(indent + 120);

					int major, minor, build;
					if (wmiData.os) {
						major = wmiData.os->major;
						minor = wmiData.os->minor;
						build = wmiData.os->build;
					}
					else {
						major = osInfo.dwMajorVersion;
						minor = osInfo.dwMinorVersion;
						build = osInfo.dwBuildNumber;
					}

					ImGui::Text("%s (%d.%d) Build %d (%s bit)",
						GetOSName(major, minor, build),
						major, minor, build, is64bit ? "64" : "32");
				}

				if (wmiData.system)  {
					constexpr float gap = 120;

					auto _Line = [=](const char* title, const std::string& text) {
						ImGui::TextUnformatted(title);
						ImGui::SameLine(indent + gap);
						ImGui::TextUnformatted(text.c_str());
					};

					_Line("Name:", wmiData.system->name);
					_Line("Model:", wmiData.system->model);
					_Line("Manufacturer:", wmiData.system->manufacturer);
				}

				/*
				if (wmiData.bios) {
					constexpr float gap = 160;

					ImGui::TextUnformatted("BIOS Name:");
					ImGui::SameLine(indent + gap);
					ImGui::TextUnformatted(wmiData.bios->name.c_str());

					ImGui::TextUnformatted("BIOS Manufacturer:");
					ImGui::SameLine(indent + gap);
					ImGui::TextUnformatted(wmiData.bios->manufacturer.c_str());
				}
				*/

				/*
				{
					ImGui::Text("CPU:");
					ImGui::SameLine(indent + gap);

					ImGui::Text(cpuInfo.Brand().data());

					ImGui::NewLine(); ImGui::SameLine(indent + gap);
					ImGui::Text("(%s; family=%d, model=%d, type=%d, stepping=%d)",
						cpuInfo.Vendor().data(),
						cpuInfo.Family(), cpuInfo.Model(), cpuInfo.Type(), cpuInfo.Stepping());
				}
				*/
			}
			ImGui::Unindent(indent);

			//ImGuiEndGroupPanel();
			ImGui::Separator();
			ImGui::TreePop();
		}

		//ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Graphics")) {
			ImGui::Indent(indent);
			{
				constexpr float gap = 120;

				auto _Line = [=](const char* title, const std::string& text) {
					ImGui::TextUnformatted(title);
					ImGui::SameLine(indent + gap);
					ImGui::TextUnformatted(text.c_str());
				};

				_Line("GDI Name:", d3dAdapterInfo.DeviceName);
				_Line("Driver:", d3dAdapterInfo.Driver);
				_Line("Description:", d3dAdapterInfo.Description);
				_Line("Version:", STR_FMT("%016llx", d3dAdapterInfo.DriverVersion.QuadPart));
				_Line("GUID:", StringUtility::FromGuid(&d3dAdapterInfo.DeviceIdentifier));
			}
			ImGui::Unindent(indent);

			ImGui::Separator();
			ImGui::TreePop();
		}

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Memory")) {
			ImGui::Indent(indent);
			{
				auto _Line = [=](float indent, float gap, uint64_t v1, uint64_t v2) {
					ImGui::SameLine(indent);
					ImGui::Text("%u", v1);
					ImGui::SameLine(indent + gap);
					ImGui::TextUnformatted("/");
					ImGui::SameLine(indent + 15 + gap);
					ImGui::Text("%u", v2);
					ImGui::SameLine(indent + 20 + gap * 2);
					ImGui::TextUnformatted("MB");

					float height = ImGui::GetCurrentContext()->FontSize;

					ImGui::SameLine(indent + 20 + gap * 2 + 30);
					ImGui::ProgressBar(v1 / (float)v2, ImVec2(128, height), "");
				};

				constexpr uint64_t mb = 1024ui64 * 1024ui64;

				ImGui::TextUnformatted("Physical:");
				_Line(indent + 120, 50, memoryStatus.ullAvailPhys / mb, memoryStatus.ullTotalPhys / mb);

				ImGui::TextUnformatted("Virtual:");
				_Line(indent + 120, 50, memoryStatus.ullAvailVirtual / mb, memoryStatus.ullTotalVirtual / mb);

				ImGui::TextUnformatted("Page File:");
				_Line(indent + 120, 50, memoryStatus.ullAvailPageFile / mb, memoryStatus.ullTotalPageFile / mb);
			}
			ImGui::Unindent(indent);

			ImGui::Separator();
			ImGui::TreePop();
		}

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("CPU")) {
			ImGui::Indent(indent);
			{
				{
					constexpr float gap = 120;

					ImGui::TextUnformatted("Name:");
					ImGui::SameLine(indent + gap);
					ImGui::Text(cpuInfo.Brand().data());

					ImGui::NewLine();
					ImGui::SameLine(indent + gap);
					ImGui::Text("(%s; family=%d, model=%d, type=%d, stepping=%d)",
						cpuInfo.Vendor().data(),
						cpuInfo.Family(), cpuInfo.Model(), cpuInfo.Type(), cpuInfo.Stepping());

					ImGui::Text("Processors:");
					ImGui::SameLine(indent + gap);
					ImGui::Text("%u", sysInfo.dwNumberOfProcessors);

					ImGui::Text("Temperature:");
					ImGui::SameLine(indent + gap);
					ImGui::Text(u8"%d °C", (int)(valueThermalZone - 273));
				}

				ImGui::Dummy(ImVec2(0, 2));
				ImGui::Separator();
				ImGui::Dummy(ImVec2(0, 2));

				ImGui::Text("Overall:");
				ImGui::Indent(20);
				{
					auto _DisplayCPU = [=](CounterProcessor& proc) {
						constexpr float gap = 120;

						{
							const float pre = indent + gap;
							ImGui::Text("Speed:");
							ImGui::SameLine(pre);
							ImGui::Text("%u", (uint64_t)(proc.maxFreq * proc.rateFreq));
							ImGui::SameLine(pre + 50);
							ImGui::Text("/");
							ImGui::SameLine(pre + 50 + 15);
							ImGui::Text("%u", (uint64_t)proc.maxFreq);
							ImGui::SameLine(pre + 50 * 2 + 20);
							ImGui::Text("Mhz");
						}

						ImGui::Text("Utilization:");
						ImGui::SameLine(indent + gap);
						{
							ImGui::BeginGroup();

							auto& history = proc.history;

							double valueNow = history.empty() ? 0 : std::clamp(history.back(), 0.0, 100.0);

							constexpr double smoothing = 0.08;
							double rate = (1 - smoothing) * proc.prevRender + valueNow * smoothing;
							proc.prevRender = rate;

							ImGui::Text("%.2f", valueNow);
							ImGui::SameLine(45);
							ImGui::TextUnformatted("%");

							auto height = ImGui::GetCurrentContext()->FontSize;

							ImGui::SameLine(70);
							ImGui::ProgressBar(rate / 100, ImVec2(128, height), "");

							ImGui::EndGroup();

							{
								ImGui::Dummy(ImVec2(0, 2));

								const ImVec2 size = ImVec2(gap + 50 + 128, 64);
								size_t count = std::min<size_t>(CPU_HISTORY_DISPLAY_MAX, history.size());
								
								ImGui::BeginDisabled();
								if (count > 0) {
									auto itrStart = history.end() - count;

									auto getter = [](void* data, int idx) -> float {
										return ((double*)data)[idx];
									};
									ImGui::PlotLines("##cpuhist", getter, &*itrStart,
										count, 0, "", 0, 100, size);
								}
								else {
									ImGui::PlotLines("##cpuhist", nullptr, 
										0, 0, "", 0, 100, size);
								}
								ImGui::EndDisabled();
							}

							
							/*
							if (ImGui::IsItemHovered()) {
								size_t count = std::min<size_t>(64, history.size());
								if (count > 0) {
									ImGui::BeginTooltipEx(ImGuiTooltipFlags_OverridePreviousTooltip, ImGuiWindowFlags_None);

									auto itrStart = history.end() - count;

									auto getter = [](void* data, int idx) -> float {
										return ((double*)data)[idx];
									};
									ImGui::PlotLines("##cpuhist", getter, &*itrStart,
										count, 0, "", 0, 100, ImVec2(0, 80));

									ImGui::EndTooltip();
								}
							}
							*/
						}
					};

					_DisplayCPU(processors[0]);
				}
				ImGui::Unindent(20);
			}
			ImGui::Unindent(indent);

			ImGui::Separator();
			ImGui::TreePop();
		}

		if (wmiData.videoControllers.size() > 0) {
			//ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Video Controllers")) {
				for (size_t i = 0; i < wmiData.videoControllers.size(); ++i) {
					auto& vc = wmiData.videoControllers[i];

					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
					if (ImGui::TreeNode(STR_FMT("%u##vc", i).c_str())) {
						ImGui::Indent(indent);

						constexpr float gap = 160;

						auto _Line = [=](const char* title, const std::string& text) {
							ImGui::TextUnformatted(title);
							ImGui::SameLine(indent + gap);
							ImGui::TextUnformatted(text.c_str());
						};

						_Line("Name:", vc.name);
						_Line("Compatibility:", vc.compatibility);
						_Line("Version:", vc.driverVersion);
						_Line("RAM:", STR_FMT("%u MB", vc.ram / (1024ui64 * 1024ui64)));
						
						ImGui::Unindent(indent);
						ImGui::TreePop();
					}
				}

				ImGui::Separator();
				ImGui::TreePop();
			}
		}

		if (wmiData.soundDevices.size() > 0) {
			//ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Sound Devices")) {

				ImGuiTableFlags table_flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter
					| ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersInnerH;
				if (ImGui::BeginTable("Table_SoundDevices", 3, table_flags)) {
					ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_NoSort
						| ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 48);
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("Manufacturer", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableHeadersRow();

					for (size_t i = 0; i < wmiData.soundDevices.size(); ++i) {
						auto& sd = wmiData.soundDevices[i];

						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(0);
						ImGui::Text("%u", i);

						ImGui::TableSetColumnIndex(1);
						ImGui::Text(sd.name.c_str());
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(sd.name.c_str());

						ImGui::TableSetColumnIndex(2);
						ImGui::Text(sd.manufacturer.c_str());
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(sd.manufacturer.c_str());
					}

					ImGui::EndTable();
				}

				ImGui::Separator();
				ImGui::TreePop();
			}
		}
	}
	ImGui::EndChild();

	ImGui::PopStyleColor(2);
}

//****************************************************************************
//WmiQuery
//****************************************************************************
WmiQuery::WmiQuery() {
	pWbemLocator = nullptr;
	pWbemService = nullptr;
}
WmiQuery::~WmiQuery() {
	ptr_release(pWbemService);
	ptr_release(pWbemLocator);
}

bool WmiQuery::Initialize() {
	try {
		HRESULT hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID*)&pWbemLocator);
		if (FAILED(hr)) {
			throw STR_FMT("Failed to create IWbemLocator [code=0x%08x]", hr);
		}

		hr = pWbemLocator->ConnectServer(L"ROOT\\CIMV2", nullptr, nullptr,
			nullptr, 0, nullptr, nullptr, &pWbemService);
		if (FAILED(hr)) {
			throw STR_FMT("Failed to create IWbemServices [code=0x%08x]", hr);
		}

		hr = CoSetProxyBlanket(pWbemService,
			RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
			RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
		if (FAILED(hr)) {
			throw STR_FMT("CoSetProxyBlanket failed [code=0x%08x]", hr);
		}
	}
	catch (const std::string& err) {
		Logger::WriteError(err);
		return false;
	}

	return true;
}

optional<WmiQuery::Result> WmiQuery::MakeQuery(const wchar_t* query) {
	IEnumWbemClassObject* pEnumerator = nullptr;

	HRESULT hr = pWbemService->ExecQuery(
		L"WQL", (BSTR)STR_FMT(L"SELECT * FROM %s", query).c_str(),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		nullptr, &pEnumerator);
	if (FAILED(hr)) {
		Logger::WriteWarn(STR_FMT("WMI query for \"%s\" failed [code=0x%08x]", query, hr));
		return {};
	}

	return std::make_optional<Result>(pEnumerator);
}

//****************************************************************************
//WmiQuery::Result
//****************************************************************************
WmiQuery::Result::Result(IEnumWbemClassObject* pEnumerator) {
	pClsObj = unique_ptr_fd<IEnumWbemClassObject>(pEnumerator,
		[](IEnumWbemClassObject* p) { ptr_release(p); });

	// I want to commit arson on Microsoft's HQ

	if (pClsObj) {
		while (true) {
			ULONG resNext = 0;
			IWbemClassObject* pObj = nullptr;

			HRESULT hr = pClsObj->Next(WBEM_INFINITE, 1, &pObj, &resNext);
			if (resNext == 0)
				break;

			std::map<std::wstring, VARIANT> valuesCurrent;

			hr = pObj->BeginEnumeration(0);
			if (SUCCEEDED(hr)) {
				BSTR name;
				VARIANT variant;
				while (true) {
					VariantInit(&variant);

					hr = pObj->Next(0, &name, &variant, nullptr, nullptr);
					if (FAILED(hr) || name == nullptr)
						break;

					valuesCurrent[(wchar_t*)name] = MOVE(variant);
				}
			}
			pObj->EndEnumeration();

			values.push_back(valuesCurrent);
			ptr_release(pObj);
		}
	}
}
WmiQuery::Result::~Result() {
	for (auto& m : values) {
		for (auto& [_, v] : m) {
			VariantClear(&v);
		}
	}
	values.clear();
}

optional<const VARIANT*> WmiQuery::Result::Get(size_t i, const std::wstring& name) const {
	if (i < Count()) {
		auto itr = values[i].find(name);
		if (itr != values[i].end()) {
			return &itr->second;
		}
	}
	return {};
}

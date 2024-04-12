#pragma once

#include "../pch.h"
#include "DxConstant.hpp"

namespace directx {
	//****************************************************************************
	//WmiQuery
	//****************************************************************************
	class WmiQuery {
		IWbemLocator* pWbemLocator;
		IWbemServices* pWbemService;
	public:
		class Result {
			unique_ptr_fd<IEnumWbemClassObject> pClsObj;
			std::vector<std::map<std::wstring, VARIANT>> values;
		public:
			Result(IEnumWbemClassObject* pEnumerator);
			virtual ~Result();

			size_t Count() const { return values.size(); }
			optional<const VARIANT*> Get(size_t i, const std::wstring& name) const;
		};
	public:
		WmiQuery();
		virtual ~WmiQuery();

		bool Initialize();

		optional<Result> MakeQuery(const wchar_t* query);
	};

	//****************************************************************************
	//SystemInfoPanel
	//****************************************************************************
	class SystemInfoPanel : public gstd::ILoggerPanel {
		struct SystemData {
			struct System {
				std::string manufacturer;
				std::string model;
				std::string name;
			};
			struct Bios {
				std::string manufacturer;
				std::string name;
			};
			struct OS {
				uint32_t major;
				uint32_t minor;
				uint32_t build;
			};
			struct SoundDevice {
				std::string manufacturer;
				std::string name;
			};
			struct VideoController {
				std::string name;
				std::string compatibility;
				uint64_t ram;
				std::string driverVersion;
			};

			// Win32_ComputerSystem
			optional<System> system;

			// Win32_BIOS
			optional<Bios> bios;

			// Win32_OperatingSystem
			optional<OS> os;

			// Win32_SoundDevice
			std::vector<SoundDevice> soundDevices;

			// Win32_VideoController
			std::vector<VideoController> videoControllers;
		};
		struct Counter {
			std::wstring path;
			HCOUNTER handle;
		};
		struct CounterProcessor {
			Counter counters[3];

			// \Processor Information(*)\Processor Frequency
			// \Processor Information(*)\% of Maximum Frequency
			// \Processor Information(*)\% Processor Time
			double maxFreq;
			double rateFreq;
			double time;

			double prevRender = 0;
			std::vector<double> history;
			void AddHistory(double value);
		};
		/*
		struct D3dQueryStats {
			D3DDEVINFO_D3D9BANDWIDTHTIMINGS bandwidthTimings;
			D3DDEVINFO_D3D9CACHEUTILIZATION cacheUtilization;
			D3DDEVINFO_D3D9PIPELINETIMINGS pipelineTimings;
			D3DDEVINFO_D3DVERTEXSTATS vertexStats;
		};
		*/

	private:
		D3DADAPTER_IDENTIFIER9 d3dAdapterInfo;
		D3DDISPLAYMODE d3dDisplayMode;
		//optional<D3dQueryStats> d3dQueryStats;

		std::vector<IDirect3DQuery9*> d3dQueries;
		bool d3dQueryCollecting;

		HANDLE hOwnProcess;
		SYSTEM_INFO sysInfo;
		OSVERSIONINFOEXA osInfo;
		BOOL is64bit;

		SystemData wmiData;

		HANDLE hQueryProcess;
		HQUERY hQuery;

		Counter counterThermalZone;		// \Thermal Zone Information(*)\Temperature
		double valueThermalZone;
		std::vector<CounterProcessor> processors;

		MEMORYSTATUSEX memoryStatus;
	private:
		void _InitializeD3D();
		void _InitializeWMI();
		void _InitializeCounters();
	public:
		SystemInfoPanel();
		~SystemInfoPanel();

		virtual void Initialize(const std::string& name);

		virtual void Update();
		virtual void ProcessGui();

		void CreateD3DQueries();
		void StartD3DQuery();
		void EndD3DQuery();
	};
}

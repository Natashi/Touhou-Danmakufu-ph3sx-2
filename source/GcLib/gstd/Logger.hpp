#pragma once

#include "../pch.h"

#include "GstdUtility.hpp"
#include "File.hpp"

#include "../directx/ImGuiWindow.hpp"

namespace gstd {
	class ILogger {
	protected:
		std::string name_;
	public:
		enum class LogType {
			Info,
			Warning,
			Error,
			User1,
		};

		struct LogData {
			SYSTEMTIME time;
			LogType type;
			std::string text;
		};
	public:
		virtual ~ILogger() = default;

		virtual bool Initialize(const std::string& name) = 0;

		const std::string& GetName() const { return name_; }

		virtual void LoadState() = 0;
		virtual void SaveState() = 0;

		virtual void Update() = 0;
		virtual void ProcessGui() = 0;

		virtual void Write(const LogData& data) = 0;
		virtual void Flush() {};
	};

	class Logger : public directx::imgui::ImGuiBaseWindow {
	protected:
		static Logger* top_;
	protected:
		virtual LRESULT _SubWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	protected:
		std::list<unique_ptr<ILogger>> listLogger_;

		bool bWindowActive_;
	protected:
		virtual void _SetImguiStyle(float scale);

		virtual void _ProcessGui();
		virtual void _Update();
	public:
		Logger();
		virtual ~Logger();

		virtual bool Initialize();
		bool Initialize(bool bEnable = true);

		void AddLogger(unique_ptr<ILogger>&& logger) { listLogger_.push_back(MOVE(logger)); }
		template<class T> T* GetLogger(const std::string& name);

		virtual void Write(ILogger::LogType type, const std::string& str);
		virtual void Write(ILogger::LogType type, const std::wstring& str);

		static void SetTop(Logger* logger) { top_ = logger; }
		static Logger* GetTop() { return top_; }

#define DEF_WRITE(_name, _type) \
		static void _name(const std::string& str) { WriteTop(_type, str); } \
		static void _name(const std::wstring& str) { WriteTop(_type, str); }

		static void WriteTop(ILogger::LogType type, const std::string& str) { if (top_) top_->Write(type, str); }
		static void WriteTop(ILogger::LogType type, const std::wstring& str) { if (top_) top_->Write(type, str); }

		DEF_WRITE(WriteInfo, ILogger::LogType::Info);
		DEF_WRITE(WriteWarn, ILogger::LogType::Warning);
		DEF_WRITE(WriteError, ILogger::LogType::Error);

		static void WriteTop(const std::string& str) { WriteInfo(str); }
		static void WriteTop(const std::wstring& str) { WriteInfo(str); }

#undef DEF_WRITE

		virtual void Flush();

		void ShowLogWindow();
	};

	template<class T> T* Logger::GetLogger(const std::string& name) {
		static_assert(std::is_base_of<ILogger, T>::value, "Type parameter must derive from ILogger");

		for (auto& iLogger : listLogger_) {
			if (iLogger->GetName() == name)
				return dcast(T*, iLogger.get());
		}
		return nullptr;
	}

#if defined(DNH_PROJ_EXECUTOR)
	//*******************************************************************
	//FileLogger
	//*******************************************************************
	class FileLogger : public ILogger {
	protected:
		bool bEnable_;

		shared_ptr<File> file_;
		std::wstring path_;

		size_t sizeMax_;
	protected:
		void _CreateFile();
	public:
		FileLogger();
		~FileLogger();

		bool SetPath(const std::wstring& path);
		void SetMaxFileSize(int size) { sizeMax_ = size; }

		virtual bool Initialize(const std::string& name);
		bool Initialize(const std::string& name, bool bEnable = true);
		bool Initialize(const std::string& name, const std::wstring& path, bool bEnable = true);

		virtual void LoadState() {};
		virtual void SaveState() {};

		virtual void Update() {};
		virtual void ProcessGui() {};

		virtual void Write(const LogData& data);
		virtual void Flush();
	};
	
	class ILoggerPanel {
	protected:
		std::string name_;
		std::string displayName_;
		bool bActive_;
	public:
		ILoggerPanel() = default;
		virtual ~ILoggerPanel() = default;

		virtual void Initialize(const std::string& name) {
			name_ = name;
			bActive_ = false;
		}

		virtual void Update() = 0;
		virtual void ProcessGui() = 0;

		const bool IsActive() const { return bActive_; }
		void SetActive(bool b) { bActive_ = b; }

		const std::string& GetName() const { return name_; }
		const std::string& GetDisplayName() const { return displayName_; }
		void SetDisplayName(const std::string& name) { displayName_ = name; }
	};

	//*******************************************************************
	//WindowLogger
	//*******************************************************************
	class WindowLogger : public ILogger {
	public:
		class PanelEventLog;
		class PanelInfo;
	protected:
		bool bEnable_;

		bool bTabInit_;
		std::string iniPanel_;

		std::list<shared_ptr<ILoggerPanel>> panels_;
		shared_ptr<ILoggerPanel> currentPanel_;

		shared_ptr<PanelEventLog> panelEventLog_;
		shared_ptr<PanelInfo> panelInfo_;
	public:
		WindowLogger();
		~WindowLogger();

		virtual bool Initialize(const std::string& name);
		bool Initialize(const std::string& name, bool bEnable = true);

		virtual void LoadState();
		virtual void SaveState();

		virtual void Update() {};
		virtual void ProcessGui();

		virtual void Write(const LogData& data);
		virtual void Flush() {};

		bool AddPanel(shared_ptr<ILoggerPanel> panel, const std::string& name);

		template<class T> shared_ptr<T> GetPanel(const std::string& name);

		shared_ptr<PanelEventLog> GetLogPanel() { return panelEventLog_; }
		shared_ptr<PanelInfo> GetInfoPanel() { return panelInfo_; }
	};

	template<class T> shared_ptr<T> WindowLogger::GetPanel(const std::string& name) {
		static_assert(std::is_base_of<ILoggerPanel, T>::value, "Type parameter must derive from ILoggerPanel");

		for (auto& iPanel : panels_) {
			if (iPanel->GetName() == name)
				return std::dynamic_pointer_cast<T>(iPanel);
		}
		return nullptr;
	}

	class WindowLogger::PanelEventLog : public ILoggerPanel {
		struct LogEntry {
			LogType type;
			std::string time;
			std::string text;
		};
		std::list<LogEntry> events_;
		std::vector<LogEntry> eventsCopy_;
		bool bLogChanged_;

		std::string filter_;

		stdch::high_resolution_clock::time_point timeStart_;
	public:
		PanelEventLog();
		~PanelEventLog();

		virtual void Initialize(const std::string& name);

		virtual void Update();
		virtual void ProcessGui();

		void AddEvent(const LogData& data);
		void ClearEvents();
		void CopyEventsToClipboard();
	};

	class WindowLogger::PanelInfo : public ILoggerPanel {
		struct CpuInfo {
			char venderID[13];
			char name[49];
			std::string cpuName;
			double clock;
			int type;
			int family;
			int model;
			int stepping;
			bool bMMXEnabled;
			bool bAMD3DNowEnabled;
			bool bSIMDEnabled;
		};
	private:
		HANDLE hProcess_;
		HQUERY hQuery_;
		HCOUNTER hCounter_;

		CpuInfo infoCpu_;
		uint64_t ramMemAvail_, ramMemMax_;
		double rateCpuUsage_;

		std::map<int, std::array<std::string, 2>> infoLines_;
	protected:
		void _InitializeHandle();

		CpuInfo _GetCpuInformation();
		double _GetCpuPerformance();

		void _GetRamInfo();
	public:
		PanelInfo();
		~PanelInfo();

		virtual void Initialize(const std::string& name);

		virtual void Update();
		virtual void ProcessGui();

		void SetInfo(int row, const std::string& label, const std::string& text);
	};
#endif
}

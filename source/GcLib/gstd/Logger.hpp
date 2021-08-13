#pragma once

#include "../pch.h"

#include "GstdUtility.hpp"
#include "File.hpp"
#include "Thread.hpp"

#include "Window.hpp"

namespace gstd {
	//*******************************************************************
	//Logger
	//*******************************************************************
	class Logger {
	protected:
		static Logger* top_;
		gstd::CriticalSection lock_;
		std::list<shared_ptr<Logger>> listLogger_;
		virtual void _WriteChild(SYSTEMTIME& time, const std::wstring& str);
		virtual void _Write(SYSTEMTIME& time, const std::wstring& str) = 0;
	public:
		Logger();
		virtual ~Logger();

		virtual bool Initialize() { return true; }

		void AddLogger(shared_ptr<Logger> logger) { listLogger_.push_back(logger); }
		
		virtual void Write(const std::string& str);
		virtual void Write(const std::wstring& str);

		static void SetTop(Logger* logger) { top_ = logger; }
		static void WriteTop(const std::string& str) { if (top_) top_->Write(str); }
		static void WriteTop(const std::wstring& str) { if (top_) top_->Write(str); }

		void FlushFileLogger();
	};

#if defined(DNH_PROJ_EXECUTOR)
	//*******************************************************************
	//FileLogger
	//*******************************************************************
	class FileLogger : public Logger {
	protected:
		bool bEnable_;

		shared_ptr<File> file_;
		std::wstring path_;

		size_t sizeMax_;

		virtual void _Write(SYSTEMTIME& systemTime, const std::wstring& str);
		void _CreateFile();
	public:
		FileLogger();
		~FileLogger();

		void FlushFile();

		bool Initialize(bool bEnable = true);
		bool Initialize(std::wstring path, bool bEnable = true);
		
		bool SetPath(const std::wstring& path);
		void SetMaxFileSize(int size) { sizeMax_ = size; }
	};

	//*******************************************************************
	//WindowLogger
	//ログウィンドウ
	//ウィンドウは別スレッド動作です
	//Natashi: Only instantiate once.
	//*******************************************************************
	class WindowLogger : public Logger, public WindowBase {
	public:
		class WindowThread;
		class Panel;
		class LogPanel;
		class InfoPanel;
		friend WindowThread;
		friend LogPanel;
		friend InfoPanel;
		enum {
			WM_ENDLOGGER = WM_APP + 1,
			WM_ADDPANEL,

			STATUS_MEMORY = 0,
			STATUS_CPU = 1,

			MENU_ID_OPEN = 1,

			STATE_INITIALIZING = 0,
			STATE_RUNNING,
			STATE_CLOSED,
		};

		struct AddPanelEvent {
			std::wstring name;
			shared_ptr<Panel> panel;
		};
	public:
		static WindowLogger* loggerParentGlobal_;
	protected:
		bool bEnable_;
		int windowState_;

		shared_ptr<WindowThread> threadWindow_;
		shared_ptr<WTabControll> wndTab_;
		shared_ptr<WStatusBar> wndStatus_;
		shared_ptr<LogPanel> wndLogPanel_;
		shared_ptr<InfoPanel> wndInfoPanel_;
		gstd::CriticalSection lock_;
		std::list<AddPanelEvent> listEventAddPanel_;

		void _Run();
		void _CreateWindow();
		virtual void _Write(SYSTEMTIME& systemTime, const std::wstring& str);
		virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	public:
		WindowLogger();
		~WindowLogger();

		bool Initialize(bool bEnable = true);

		void SaveState();
		void LoadState();

		void SetInfo(int row, const std::wstring& textInfo, const std::wstring& textData);
		bool AddPanel(shared_ptr<Panel> panel, const std::wstring& name);

		int GetState() { return windowState_; }

		void ShowLogWindow();
		static void InsertOpenCommandInSystemMenu(HWND hWnd);

		shared_ptr<WStatusBar> GetStatusBar() { return wndStatus_; }
		void ClearStatusBar();
		
		static WindowLogger* GetParent() { return loggerParentGlobal_; }
	};
	class WindowLogger::WindowThread : public gstd::Thread, public gstd::InnerClass<WindowLogger> {
		friend WindowLogger;
	protected:
		WindowThread(WindowLogger* logger);

		void _Run();
	};
	class WindowLogger::Panel : public WPanel {
		friend WindowLogger;
	protected:
		virtual bool _AddedLogger(HWND hTab) = 0;

		virtual void _ReadRecord(RecordBuffer& record) {}
		virtual void _WriteRecord(RecordBuffer& record) {}
	};

	class WindowLogger::LogPanel : public WindowLogger::Panel {
		WEditBox wndEdit_;
	protected:
		virtual bool _AddedLogger(HWND hTab);
	public:
		LogPanel();
		~LogPanel();

		virtual void LocateParts();

		void AddText(const std::wstring& text);
		void ClearText();
	};

	class WindowLogger::InfoPanel : public WindowLogger::Panel, public Thread {
	private:
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
		class InfoCollector;
		friend class InfoCollector;

		enum {
			ROW_INFO = 0,
			ROW_DATA,
		};

		gstd::CriticalSection lock_;

		WListView wndListView_;

		CpuInfo infoCpu_;

		HANDLE hProcess_;
		HQUERY hQuery_;
		HCOUNTER hCounter_;
	protected:
		virtual void _Run();

		virtual bool _AddedLogger(HWND hTab);

		void _InitializeHandle();
		CpuInfo _GetCpuInformation();
		double _GetCpuPerformance();

		void _SetRamInfo();
	public:
		InfoPanel();
		~InfoPanel();

		virtual void LocateParts();

		void SetInfo(int row, const std::wstring& textInfo, const std::wstring& textData);
	};
#endif
}

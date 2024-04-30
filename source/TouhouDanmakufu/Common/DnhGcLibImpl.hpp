#pragma once

#include "../../GcLib/pch.h"

#include "DnhConstant.hpp"

#if defined(DNH_PROJ_EXECUTOR)
//*******************************************************************
//EPathProperty
//*******************************************************************
class EPathProperty : public Singleton<EPathProperty>, public PathProperty {
public:
	static const std::wstring& GetSystemResourceDirectory();
	static const std::wstring& GetSystemImageDirectory();
	static const std::wstring& GetSystemBgmDirectory();
	static const std::wstring& GetSystemSeDirectory();

	static const std::wstring& GetStgScriptRootDirectory();
	static const std::wstring& GetStgDefaultScriptDirectory();
	static const std::wstring& GetPlayerScriptRootDirectory();

	static std::wstring GetReplaySaveDirectory(const std::wstring& scriptPath);
	static std::wstring GetCommonDataPath(const std::wstring& scriptPath, const std::wstring& area);
};

//*******************************************************************
//ELogger
//*******************************************************************
class ELogger : public Singleton<ELogger>, public Logger {
	class WindowThread : public Thread, public InnerClass<ELogger> {
		friend ELogger;
	protected:
		WindowThread(ELogger* logger);
		void _Run();
	};

	struct PanelData {
		shared_ptr<gstd::ILoggerPanel> panel;
		uint32_t updateFreq;
		uint32_t timer;
		bool bPrevVisible;
	};
public:
	enum {
		MY_SYSCMD_OPEN = 1,
	};
protected:
	bool bWindow_;
	shared_ptr<WindowThread> threadWindow_;

	std::list<PanelData> listPanel_;
	uint64_t time_;
protected:
	void _Run();

	bool _AddPanel(shared_ptr<ILoggerPanel> panel, const std::wstring& name);
public:
	ELogger();
	virtual ~ELogger();

	virtual bool Initialize();
	bool Initialize(bool bFile, bool bWindow);

	void LoadState();
	void SaveState();
	void Close();

	bool EAddPanel(shared_ptr<ILoggerPanel> panel, const std::wstring& name, uint32_t updateFreq);
	bool EAddPanelNoUpdate(shared_ptr<ILoggerPanel> panel, const std::wstring& name);
	bool EAddPanelUpdateData(shared_ptr<ILoggerPanel> panel, uint32_t updateFreq);

	gstd::WindowLogger* GetWindowLogger() { return GetLogger<gstd::WindowLogger>("Window"); }
	gstd::FileLogger* GetFileLogger() { return GetLogger<gstd::FileLogger>("File"); }

	shared_ptr<gstd::WindowLogger::PanelEventLog> GetEventLog() {
		if (auto wLogger = GetWindowLogger())
			return wLogger->GetLogPanel();
		return nullptr;
	}
	shared_ptr<gstd::WindowLogger::PanelInfo> GetInfoPanel() {
		if (auto wLogger = GetWindowLogger())
			return wLogger->GetInfoPanel();
		return nullptr;
	}

	void InsertOpenCommandInSystemMenu(HWND hWnd);

	void ResetDevice();
};


//*******************************************************************
//EFpsController
//*******************************************************************
class EFpsController : public Singleton<EFpsController> {
private:
	int16_t fastModeKey_;
	ref_count_ptr<FpsController> controller_;
public:
	EFpsController();

	void SetFps(int fps) { controller_->SetFps(fps); }
	int GetFps() { return controller_->GetFps(); }

	std::array<bool, 2> Advance() { return controller_->Advance(); }

	void SetCriticalFrame() { controller_->SetCriticalFrame(); }

	float GetCurrentFps() { return controller_->GetCurrentFps(); }
	float GetCurrentWorkFps() { return controller_->GetCurrentWorkFps(); }
	float GetCurrentRenderFps() { return controller_->GetCurrentRenderFps(); }

	bool IsFastMode() { return controller_->IsFastMode(); }
	void SetFastMode(bool b) { controller_->SetFastMode(b); }
	void SetFastModeRate(size_t rate) { controller_->SetFastModeRate(rate); }

	void AddFpsControlObject(unique_ptr<FpsControlObject>&& obj) { controller_->AddFpsControlObject(MOVE(obj)); }
	void RemoveFpsControlObject(FpsControlObject* obj) { controller_->RemoveFpsControlObject(obj); }

	int16_t GetFastModeKey() { return fastModeKey_; }
	void SetFastModeKey(int16_t key) { fastModeKey_ = key; }
};
#endif

//*******************************************************************
//EFileManager
//*******************************************************************
class EFileManager : public Singleton<EFileManager>, public FileManager {

};

#if defined(DNH_PROJ_EXECUTOR)
//*******************************************************************
//ETaskManager
//*******************************************************************
class ETaskManager : public Singleton<ETaskManager>, public WorkRenderTaskManager {
	enum {
		TASK_WORK_PRI_MAX = 10,
		TASK_RENDER_PRI_MAX = 10,
	};

	uint64_t timeSpentOnRender_;
	uint64_t timeSpentOnWork_;
public:
	ETaskManager();

	bool Initialize();

	uint64_t GetRenderTime() { return timeSpentOnRender_; }
	void SetRenderTime(uint64_t t) { timeSpentOnRender_ = t; }
	uint64_t SetWorkTime() { return timeSpentOnWork_; }
	void SetWorkTime(uint64_t t) { timeSpentOnWork_ = t; }
};

//*******************************************************************
//ETextureManager
//*******************************************************************
class ETextureManager : public Singleton<ETextureManager>, public TextureManager {
	enum {
		MAX_RESERVED_RENDERTARGET = 3,
	};
public:
	virtual bool Initialize();
	std::wstring GetReservedRenderTargetName(int index);
};

//*******************************************************************
//EMeshManager
//*******************************************************************
class EMeshManager : public Singleton<EMeshManager>, public DxMeshManager {

};

//*******************************************************************
//EMeshManager
//*******************************************************************
class EShaderManager : public Singleton<EShaderManager>, public ShaderManager {

};

//*******************************************************************
//EDxTextRenderer
//*******************************************************************
class EDxTextRenderer : public Singleton<EDxTextRenderer>, public DxTextRenderer {

};

//*******************************************************************
//EDirectSoundManager
//*******************************************************************
class EDirectSoundManager : public Singleton<EDirectSoundManager>, public DirectSoundManager {

};
#endif

//*******************************************************************
//EDirectInput
//*******************************************************************
class EDirectInput : public Singleton<EDirectInput>, public VirtualKeyManager {
public:
	enum {
		KEY_INVALID = -1,

		KEY_LEFT,
		KEY_RIGHT,
		KEY_UP,
		KEY_DOWN,

		KEY_OK,
		KEY_CANCEL,

		KEY_SHOT,
		KEY_BOMB,
		KEY_SLOWMOVE,
		KEY_USER1,
		KEY_USER2,

		KEY_PAUSE,

		VK_USER_ID_STAGE = 256,
		VK_USER_ID_PLAYER = 512,

	};

	int padIndex_;
public:
	virtual bool Initialize(HWND hWnd);

	void ResetVirtualKeyMap();

	int GetPadIndex() { return padIndex_; }
};
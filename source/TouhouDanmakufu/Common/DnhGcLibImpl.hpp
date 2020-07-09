#ifndef __TOUHOUDANMAKUFU_DNHGCLIBIMPL__
#define __TOUHOUDANMAKUFU_DNHGCLIBIMPL__

#include "../../GcLib/pch.h"

#include "DnhConstant.hpp"

/**********************************************************
//EPathProperty
**********************************************************/
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

#if defined(DNH_PROJ_EXECUTOR)
/**********************************************************
//ELogger
**********************************************************/
class ELogger : public Singleton<ELogger>, public WindowLogger {
	gstd::ref_count_ptr<gstd::ScriptCommonDataInfoPanel> panelCommonData_;
public:
	ELogger();
	void Initialize(bool bFile, bool bWindow);

	gstd::ref_count_ptr<gstd::ScriptCommonDataInfoPanel> GetScriptCommonDataInfoPanel() { return panelCommonData_; }
	void UpdateCommonDataInfoPanel(gstd::ref_count_ptr<ScriptCommonDataManager> commonDataManager);
};


/**********************************************************
//EFpsController
**********************************************************/
class EFpsController : public Singleton<EFpsController> {
private:
	int16_t fastModeKey_;
	ref_count_ptr<FpsController> controller_;
public:
	EFpsController();

	void SetFps(int fps) { controller_->SetFps(fps); }
	int GetFps() { return controller_->GetFps(); }
	void SetTimerEnable(bool b) { controller_->SetTimerEnable(b); }

	void Wait() { controller_->Wait(); }
	bool IsSkip() { return controller_->IsSkip(); }
	void SetCriticalFrame() { controller_->SetCriticalFrame(); }
	float GetCurrentFps() { return controller_->GetCurrentFps(); }
	float GetCurrentWorkFps() { return controller_->GetCurrentWorkFps(); }
	float GetCurrentRenderFps() { return controller_->GetCurrentRenderFps(); }
	bool IsFastMode() { return controller_->IsFastMode(); }
	void SetFastMode(bool b) { controller_->SetFastMode(b); }
	void SetFastModeRate(size_t rate) { controller_->SetFastModeRate(rate); }

	void AddFpsControlObject(ref_count_weak_ptr<FpsControlObject> obj) { controller_->AddFpsControlObject(obj); }
	void RemoveFpsControlObject(ref_count_weak_ptr<FpsControlObject> obj) { controller_->RemoveFpsControlObject(obj); }

	int16_t GetFastModeKey() { return fastModeKey_; }
	void SetFastModeKey(int16_t key) { fastModeKey_ = key; }
};
#endif

/**********************************************************
//EFileManager
**********************************************************/
class EFileManager : public Singleton<EFileManager>, public FileManager {

};

#if defined(DNH_PROJ_EXECUTOR)
/**********************************************************
//ETaskManager
**********************************************************/
class ETaskManager : public Singleton<ETaskManager>, public WorkRenderTaskManager {
	enum {
		TASK_WORK_PRI_MAX = 10,
		TASK_RENDER_PRI_MAX = 10,
	};
public:
	bool Initialize();
};

/**********************************************************
//ETextureManager
**********************************************************/
class ETextureManager : public Singleton<ETextureManager>, public TextureManager {
	enum {
		MAX_RESERVED_RENDERTARGET = 3,
	};
public:
	virtual bool Initialize();
	std::wstring GetReservedRenderTargetName(int index);
};

/**********************************************************
//EMeshManager
**********************************************************/
class EMeshManager : public Singleton<EMeshManager>, public DxMeshManager {

};

/**********************************************************
//EMeshManager
**********************************************************/
class EShaderManager : public Singleton<EShaderManager>, public ShaderManager {

};

/**********************************************************
//EDxTextRenderer
**********************************************************/
class EDxTextRenderer : public Singleton<EDxTextRenderer>, public DxTextRenderer {

};

/**********************************************************
//EDirectSoundManager
**********************************************************/
class EDirectSoundManager : public Singleton<EDirectSoundManager>, public DirectSoundManager {

};
#endif

/**********************************************************
//EDirectInput
**********************************************************/
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

#endif
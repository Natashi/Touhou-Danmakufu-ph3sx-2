#pragma once

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"

class StgControlScript;
//*******************************************************************
//StgControlScriptManager
//*******************************************************************
class StgControlScriptManager : public ScriptManager {
public:
	StgControlScriptManager();
	virtual ~StgControlScriptManager();
};

//*******************************************************************
//StgControlScriptInformation
//*******************************************************************
class StgControlScriptInformation {
	std::vector<ref_count_ptr<ScriptInformation>> listFreePlayer_;
	ref_count_ptr<ReplayInformationManager> replayManager_;
public:
	StgControlScriptInformation();
	virtual ~StgControlScriptInformation();

	void LoadFreePlayerList();
	std::vector<ref_count_ptr<ScriptInformation>>& GetFreePlayerList() { return listFreePlayer_; }

	void LoadReplayInformation(std::wstring pathMainScript);
	ref_count_ptr<ReplayInformationManager> GetReplayInformationManager() { return replayManager_; }
};

//*******************************************************************
//StgControlScript
//*******************************************************************
class StgControlScript : public DnhScript {
	friend StgControlScriptManager;
public:
	enum {
		EV_USER_COUNT = 100000,
		EV_USER = 1000000,
		EV_USER_SYSTEM = 2000000,
		EV_USER_STAGE = 3000000,
		EV_USER_PLAYER = 4000000,
		EV_USER_PACKAGE = 5000000,

		EV_APP_LOSE_FOCUS = 10000,
		EV_APP_RESTORE_FOCUS,

		TYPE_SCRIPT_ALL = -1,
		TYPE_SCRIPT_PLAYER = ScriptInformation::TYPE_PLAYER,
		TYPE_SCRIPT_SINGLE = ScriptInformation::TYPE_SINGLE,
		TYPE_SCRIPT_PLURAL = ScriptInformation::TYPE_PLURAL,
		TYPE_SCRIPT_STAGE = ScriptInformation::TYPE_STAGE,
		TYPE_SCRIPT_PACKAGE = ScriptInformation::TYPE_PACKAGE,

		INFO_SCRIPT_TYPE,
		INFO_SCRIPT_PATH,
		INFO_SCRIPT_ID,
		INFO_SCRIPT_TITLE,
		INFO_SCRIPT_TEXT,
		INFO_SCRIPT_IMAGE,
		INFO_SCRIPT_REPLAY_NAME,

		REPLAY_FILE_PATH,
		REPLAY_DATE_TIME,
		REPLAY_USER_NAME,
		REPLAY_TOTAL_SCORE,
		REPLAY_FPS_AVERAGE,
		REPLAY_PLAYER_NAME,
		REPLAY_STAGE_INDEX_LIST,
		REPLAY_STAGE_START_SCORE_LIST,
		REPLAY_STAGE_LAST_SCORE_LIST,
		REPLAY_COMMENT,

		RESULT_CANCEL,
		RESULT_END,
		RESULT_RETRY,
		RESULT_SAVE_REPLAY,
	};
protected:
	StgSystemController* systemController_;

	std::map<std::wstring, ref_count_ptr<ScriptInformation>> mapScriptInfo_;
public:
	StgControlScript(StgSystemController* systemController);

	// Common data
	DNH_FUNCAPI_DECL_(Func_SetCommonData);
	DNH_FUNCAPI_DECL_(Func_GetCommonData);
	DNH_FUNCAPI_DECL_(Func_ClearCommonData);
	DNH_FUNCAPI_DECL_(Func_DeleteCommonData);

	DNH_FUNCAPI_DECL_(Func_SetAreaCommonData);
	DNH_FUNCAPI_DECL_(Func_GetAreaCommonData);
	DNH_FUNCAPI_DECL_(Func_ClearAreaCommonData);
	DNH_FUNCAPI_DECL_(Func_DeleteAreaCommonData);

	DNH_FUNCAPI_DECL_(Func_CreateCommonDataArea);
	DNH_FUNCAPI_DECL_(Func_DeleteWholeAreaCommonData);
	DNH_FUNCAPI_DECL_(Func_CopyCommonDataArea);
	DNH_FUNCAPI_DECL_(Func_IsCommonDataAreaExists);
	DNH_FUNCAPI_DECL_(Func_GetCommonDataAreaKeyList);
	DNH_FUNCAPI_DECL_(Func_GetCommonDataValueKeyList);

	DNH_FUNCAPI_DECL_(Func_LoadCommonDataValuePointer);
	DNH_FUNCAPI_DECL_(Func_LoadAreaCommonDataValuePointer);
	DNH_FUNCAPI_DECL_(Func_IsValidCommonDataValuePointer);
	DNH_FUNCAPI_DECL_(Func_SetCommonDataPtr);
	DNH_FUNCAPI_DECL_(Func_GetCommonDataPtr);

	// Common data save/load
	static gstd::value Func_SaveCommonDataAreaA1(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_LoadCommonDataAreaA1(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SaveCommonDataAreaA2(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_LoadCommonDataAreaA2(gstd::script_machine* machine, int argc, const gstd::value* argv);

	//Virtual keys
	static gstd::value Func_AddVirtualKey(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_AddReplayTargetVirtualKey(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SetSkipModeKey(gstd::script_machine* machine, int argc, const gstd::value* argv);

	//STG制御共通関数：システム関連
	template<int64_t(StgStageInformation::* Func)()>
	DNH_FUNCAPI_DECL_(Func_StgStageInformation_int64_void);
	template<void(StgStageInformation::* Func)(int64_t)>
	DNH_FUNCAPI_DECL_(Func_StgStageInformation_void_int64);

	static gstd::value Func_IsReplay(gstd::script_machine* machine, int argc, const gstd::value* argv);

	static gstd::value Func_AddArchiveFile(gstd::script_machine* machine, int argc, const gstd::value* argv);
	DNH_FUNCAPI_DECL_(Func_GetArchiveFilePathList);

	DNH_FUNCAPI_DECL_(Func_GetCurrentUpdateFps);
	DNH_FUNCAPI_DECL_(Func_GetCurrentRenderFps);
	DNH_FUNCAPI_DECL_(Func_GetLastFrameUpdateSpeed);
	DNH_FUNCAPI_DECL_(Func_GetLastFrameRenderSpeed);
	static gstd::value Func_GetStageTime(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetStageTimeF(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetPackageTime(gstd::script_machine* machine, int argc, const gstd::value* argv);

	static gstd::value Func_GetStgFrameLeft(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetStgFrameTop(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetStgFrameWidth(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetStgFrameHeight(gstd::script_machine* machine, int argc, const gstd::value* argv);

	static gstd::value Func_GetMainPackageScriptPath(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetScriptPathList(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetScriptInfoA1(gstd::script_machine* machine, int argc, const gstd::value* argv);

	//Engine utility
	DNH_FUNCAPI_DECL_(Func_IsEngineFastMode);
	DNH_FUNCAPI_DECL_(Func_GetConfigWindowSizeIndex);
	DNH_FUNCAPI_DECL_(Func_GetConfigWindowSizeList);
	DNH_FUNCAPI_DECL_(Func_GetConfigVirtualKeyMapping);
	DNH_FUNCAPI_DECL_(Func_GetConfigWindowTitle);
	DNH_FUNCAPI_DECL_(Func_SetWindowTitle);
	DNH_FUNCAPI_DECL_(Func_SetEnableUnfocusedProcessing);
	DNH_FUNCAPI_DECL_(Func_IsWindowFocused);

	//STG制御共通関数：描画関連
	static gstd::value Func_ClearInvalidRenderPriority(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SetInvalidRenderPriorityA1(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetReservedRenderTargetName(gstd::script_machine* machine, int argc, const gstd::value* argv);
	template<bool OVERRIDE_RT> DNH_FUNCAPI_DECL_(Func_RenderToTextureA);
	template<bool OVERRIDE_RT> DNH_FUNCAPI_DECL_(Func_RenderToTextureB);
	static gstd::value Func_SaveSnapShotA1(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SaveSnapShotA2(gstd::script_machine* machine, int argc, const gstd::value* argv);
	DNH_FUNCAPI_DECL_(Func_SaveSnapShotA3);

	//STG制御共通関数：自機関連
	static gstd::value Func_GetPlayerID(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetPlayerReplayName(gstd::script_machine* machine, int argc, const gstd::value* argv);

	//STG制御共通関数：ユーザスクリプト
	static gstd::value Func_SetPauseScriptPath(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SetEndSceneScriptPath(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SetReplaySaveSceneScriptPath(gstd::script_machine* machine, int argc, const gstd::value* argv);

	//STG制御共通関数：自機スクリプト
	static gstd::value Func_GetLoadFreePlayerScriptList(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetFreePlayerScriptCount(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetFreePlayerScriptInfo(gstd::script_machine* machine, int argc, const gstd::value* argv);

	//STG制御共通関数：リプレイ関連
	static gstd::value Func_LoadReplayList(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetValidReplayIndices(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_IsValidReplayIndex(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetReplayInfo(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SetReplayInfo(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetReplayUserData(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SetReplayUserData(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_IsReplayUserDataExists(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SaveReplay(gstd::script_machine* machine, int argc, const gstd::value* argv);
};

//*******************************************************************
//ScriptInfoPanel
//*******************************************************************
class ScriptInfoPanel : public gstd::ILoggerPanel {
	struct CacheDisplay {
		ScriptEngineData* script;
		std::string name;
		std::string path;
	};

	struct ScriptDisplay {
		enum Column : ImGuiID {
			Address, Id,
			Type, Path,
			Status, Task, Time,
			_NoSort,
		};
		enum class ScriptStatus {
			Running,
			Loaded,
			Paused,
			Closing,
		};

		weak_ptr<ManagedScript> script;
		uintptr_t address;

		size_t id;
		std::string type;
		std::string name;
		ScriptStatus status;
		size_t tasks;
		uint64_t time;
	public:
		ScriptDisplay(shared_ptr<ManagedScript> script, ScriptStatus status);
	public:
		static const ImGuiTableSortSpecs* imguiSortSpecs;
		static bool IMGUI_CDECL Compare(const ScriptDisplay& a, const ScriptDisplay& b);
	};
	class ManagerDisplay {
	public:
		struct DataPair {
			std::string key;
			std::string value;
		};
	public:
		weak_ptr<ScriptManager> manager;
		uintptr_t address;

		size_t idThread;
		size_t nScriptRun;
		size_t nScriptLoad;

		std::vector<ScriptDisplay> listScripts;

		ManagerDisplay(shared_ptr<ScriptManager> manager);
	public:
		// Lazy-loaded
		void LoadScripts();
	};
protected:
	std::vector<CacheDisplay> listCachedScript_;

	std::vector<ManagerDisplay> listManager_;

	uintptr_t selectedManagerAddr_;
	uintptr_t selectedScriptAddr_;
private:
	static const char* GetScriptTypeName(ManagedScript* script);
	static const char* GetScriptStatusStr(ScriptDisplay::ScriptStatus status);

	void _TerminateScriptAll();
	void _TerminateScript(weak_ptr<ManagedScript> script);
public:
	ScriptInfoPanel();

	virtual void Initialize(const std::string& name);

	virtual void Update();
	virtual void ProcessGui();
};
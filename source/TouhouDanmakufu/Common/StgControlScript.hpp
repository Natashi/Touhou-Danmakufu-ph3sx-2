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

	//Area common data save/load
	static gstd::value Func_SaveCommonDataAreaA1(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_LoadCommonDataAreaA1(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SaveCommonDataAreaA2(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_LoadCommonDataAreaA2(gstd::script_machine* machine, int argc, const gstd::value* argv);

	//Virtual keys
	static gstd::value Func_AddVirtualKey(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_AddReplayTargetVirtualKey(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SetSkipModeKey(gstd::script_machine* machine, int argc, const gstd::value* argv);

	//STG制御共通関数：システム関連
	static gstd::value Func_GetScore(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_AddScore(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetGraze(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_AddGraze(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetPoint(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_AddPoint(gstd::script_machine* machine, int argc, const gstd::value* argv);

	static gstd::value Func_IsReplay(gstd::script_machine* machine, int argc, const gstd::value* argv);

	static gstd::value Func_AddArchiveFile(gstd::script_machine* machine, int argc, const gstd::value* argv);
	DNH_FUNCAPI_DECL_(Func_GetArchiveFilePathList);

	static gstd::value Func_GetCurrentFps(gstd::script_machine* machine, int argc, const gstd::value* argv);
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
	static gstd::value Func_RenderToTextureA1(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_RenderToTextureB1(gstd::script_machine* machine, int argc, const gstd::value* argv);
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
class ScriptInfoPanel : public WindowLogger::Panel {
protected:
	gstd::CriticalSection lock_;

	WButton buttonTerminateAllScript_;
	WButton buttonTerminateSingleScript_;
	WListView wndManager_;
	WListView wndCache_;
	WListView wndScript_;
	//WListBox wndListViewData_;
	WSplitter wndSplitter_;
	WSplitter wndSplitter2_;

	std::list<weak_ptr<ManagedScript>> listScript_;

	virtual bool _AddedLogger(HWND hTab);
	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void _TerminateScriptAll();

	static const wchar_t* GetScriptTypeName(ManagedScript* script);
public:
	ScriptInfoPanel();
	~ScriptInfoPanel();

	virtual void LocateParts();
	virtual void PanelUpdate();
	void Update(StgSystemController* systemController);
};
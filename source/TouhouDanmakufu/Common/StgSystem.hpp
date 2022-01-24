#pragma once

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgStageScript.hpp"
#include "StgStageController.hpp"
#include "StgPlayer.hpp"
#include "StgEnemy.hpp"
#include "StgShot.hpp"
#include "StgItem.hpp"
#include "StgIntersection.hpp"
#include "StgUserExtendScene.hpp"
#include "StgPackageController.hpp"

class StgSystemInformation;
//*******************************************************************
//StgSystemController
//*******************************************************************
class StgSystemController : public TaskBase {
	static StgSystemController* base_;
public:
	enum {
		TASK_PRI_WORK = 4,
		TASK_PRI_RENDER = 4,
	};
protected:
	ref_count_ptr<StgSystemInformation> infoSystem_;
	ref_count_ptr<ScriptEngineCache> scriptEngineCache_;
	shared_ptr<ScriptCommonDataManager> commonDataManager_;

	shared_ptr<StgEndScene> endScene_;
	shared_ptr<StgReplaySaveScene> replaySaveScene_;

	shared_ptr<StgStageController> stageController_;
	shared_ptr<StgPackageController> packageController_;

	ref_count_ptr<StgControlScriptInformation> infoControlScript_;

	bool bPrevWindowFocused_;

	virtual void DoEnd() = 0;
	virtual void DoRetry() = 0;
	void _ControlScene();
public:
	StgSystemController();
	~StgSystemController();

	void Initialize(ref_count_ptr<StgSystemInformation> infoSystem);
	void Start(ref_count_ptr<ScriptInformation> infoPlayer, ref_count_ptr<ReplayInformation> infoReplay);

	void Work();
	void Render();

	void RenderScriptObject();
	void RenderScriptObject(int priMin, int priMax);

	bool CheckMeshAndClearZBuffer(DxScriptRenderObject* obj);

	static StgSystemController* GetBase() { return base_; }

	ref_count_ptr<StgSystemInformation> GetSystemInformation() { return infoSystem_; }

	shared_ptr<StgStageController> GetStageController() { return stageController_; }
	StgPackageController* GetPackageController() { return packageController_.get(); }

	ref_count_ptr<StgControlScriptInformation> GetControlScriptInformation() { return infoControlScript_; }

	ref_count_ptr<ScriptEngineCache> GetScriptEngineCache() { return scriptEngineCache_; }
	shared_ptr<ScriptCommonDataManager> GetCommonDataManager() { return commonDataManager_; }

	void StartStgScene(ref_count_ptr<StgStageInformation> infoStage, ref_count_ptr<ReplayInformation::StageData> replayStageData);
	void StartStgScene(ref_count_ptr<StgStageStartData> startData);

	void TransStgEndScene();
	void TransReplaySaveScene();

	ref_count_ptr<ReplayInformation> CreateReplayInformation();
	void TerminateScriptAll();
	void GetAllScriptList(std::list<weak_ptr<ScriptManager>>& listRes);
};

//*******************************************************************
//StgSystemInformation
//*******************************************************************
class StgSystemInformation {
public:
	enum {
		SCENE_NULL,
		SCENE_PACKAGE_CONTROL,
		SCENE_STG,
		SCENE_REPLAY_SAVE,
		SCENE_END,
	};

private:
	int scene_;
	bool bEndStg_;
	bool bRetry_;

	std::wstring pathPauseScript_;
	std::wstring pathEndSceneScript_;
	std::wstring pathReplaySaveSceneScript_;

	std::list<std::wstring> listError_;
	ref_count_ptr<ScriptInformation> infoMain_;
	ref_count_ptr<ReplayInformation> infoReplayActive_; //アクティブリプレイ情報

	int invalidPriMin_;
	int invalidPriMax_;
	std::set<int16_t> listReplayTargetKey_;
public:
	StgSystemInformation();
	virtual ~StgSystemInformation();

	bool IsPackageMode();
	void ResetRetry();
	int GetScene() { return scene_; }
	void SetScene(int scene) { scene_ = scene; }
	bool IsStgEnd() { return bEndStg_; }
	void SetStgEnd() { bEndStg_ = true; }
	bool IsRetry() { return bRetry_; }
	void SetRetry() { bRetry_ = true; }
	bool IsError() { return listError_.size() > 0; }
	void SetError(const std::wstring& error) { listError_.push_back(error); }
	std::wstring GetErrorMessage();

	std::wstring& GetPauseScriptPath() { return pathPauseScript_; }
	void SetPauseScriptPath(const std::wstring& path) { pathPauseScript_ = path; }
	std::wstring& GetEndSceneScriptPath() { return pathEndSceneScript_; }
	void SetEndSceneScriptPath(const std::wstring& path) { pathEndSceneScript_ = path; }
	std::wstring& GetReplaySaveSceneScriptPath() { return pathReplaySaveSceneScript_; }
	void SetReplaySaveSceneScriptPath(const std::wstring& path) { pathReplaySaveSceneScript_ = path; }

	ref_count_ptr<ScriptInformation> GetMainScriptInformation() { return infoMain_; }
	void SetMainScriptInformation(ref_count_ptr<ScriptInformation> info) { infoMain_ = info; }

	ref_count_ptr<ReplayInformation> GetActiveReplayInformation() { return infoReplayActive_; }
	void SetActiveReplayInformation(ref_count_ptr<ReplayInformation> info) { infoReplayActive_ = info; }

	void SetInvalidRenderPriority(int priMin, int priMax);
	int GetInvalidRenderPriorityMin() { return invalidPriMin_; }
	int GetInvalidRenderPriorityMax() { return invalidPriMax_; }

	void AddReplayTargetKey(int16_t id) { listReplayTargetKey_.insert(id); }
	std::set<int16_t>& GetReplayTargetKeyList() { return listReplayTargetKey_; }

};
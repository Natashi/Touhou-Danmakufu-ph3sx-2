#pragma once

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgStageScript.hpp"
#include "StgPlayer.hpp"
#include "StgEnemy.hpp"
#include "StgShot.hpp"
#include "StgItem.hpp"
#include "StgIntersection.hpp"
#include "StgUserExtendScene.hpp"

class StgStageInformation;
class StgStageStartData;
class PseudoSlowInformation;
//*******************************************************************
//StgStageController
//*******************************************************************
class StgStageController {
private:
	StgSystemController* systemController_;
	ref_count_ptr<StgSystemInformation> infoSystem_;
	ref_count_ptr<StgStageInformation> infoStage_;
	ref_count_ptr<PseudoSlowInformation> infoSlow_;

	ref_count_ptr<StgPauseScene> pauseManager_;
	ref_count_ptr<KeyReplayManager> keyReplayManager_;
	shared_ptr<StgStageScriptObjectManager> objectManagerMain_;
	shared_ptr<StgStageScriptManager> scriptManager_;
	StgEnemyManager* enemyManager_;
	StgShotManager* shotManager_;
	StgItemManager* itemManager_;
	StgIntersectionManager* intersectionManager_;

	void _SetupReplayTargetCommonDataArea(shared_ptr<ManagedScript> pScript);
public:
	StgStageController(StgSystemController* systemController);
	virtual ~StgStageController();

	void Initialize(ref_count_ptr<StgStageStartData> startData);
	void CloseScene();

	void Work();
	void Render();

	void RenderToTransitionTexture();

	StgStageScriptObjectManager* GetMainObjectManager() { return objectManagerMain_.get(); }
	shared_ptr<StgStageScriptObjectManager> GetMainObjectManagerRef() { return objectManagerMain_; }
	StgStageScriptManager* GetScriptManager() { return scriptManager_.get(); }
	shared_ptr<StgStageScriptManager> GetScriptManagerRef() { return scriptManager_; }
	StgEnemyManager* GetEnemyManager() { return enemyManager_; }
	StgShotManager* GetShotManager() { return shotManager_; }
	StgItemManager* GetItemManager() { return itemManager_; }
	StgIntersectionManager* GetIntersectionManager() { return intersectionManager_; }
	ref_count_ptr<StgPauseScene> GetPauseManager() { return pauseManager_; }

	ref_unsync_ptr<DxScriptObjectBase> GetMainRenderObject(int idObject) { return objectManagerMain_->GetObject(idObject); }
	ref_unsync_ptr<StgPlayerObject> GetPlayerObject() { return objectManagerMain_->GetPlayerObject(); }

	StgSystemController* GetSystemController() { return systemController_; }
	ref_count_ptr<StgSystemInformation> GetSystemInformation() { return infoSystem_; }
	ref_count_ptr<StgStageInformation> GetStageInformation() { return infoStage_; }
	ref_count_ptr<KeyReplayManager> GetKeyReplayManager() { return keyReplayManager_; }

	ref_count_ptr<PseudoSlowInformation> GetSlowInformation() { return infoSlow_; }
};


//*******************************************************************
//StgStageInformation
//*******************************************************************
class StgStageInformation {
public:
	enum {
		RESULT_UNKNOWN,
		RESULT_BREAK_OFF,
		RESULT_PLAYER_DOWN,
		RESULT_CLEARED,
	};
private:
	bool bEndStg_;
	bool bPause_;
	bool bReplay_;
	DWORD frame_;
	int stageIndex_;

	ref_count_ptr<ScriptInformation> infoMainScript_;
	ref_count_ptr<ScriptInformation> infoPlayerScript_;
	ref_count_ptr<StgPlayerInformation> infoPlayerObject_;
	ref_count_ptr<ReplayInformation::StageData> replayStageData_;
	DxRect<LONG> rcStgFrame_;
	int priMinStgFrame_;
	int priMaxStgFrame_;
	int priShotObject_;
	int priItemObject_;
	int priCameraFocusPermit_;
	DxRect<LONG> rcShotAutoDeleteClipOffset_;
	DxRect<LONG> rcShotAutoDeleteClip_;

	shared_ptr<RandProvider> rand_;
	int64_t score_;
	int64_t graze_;
	int64_t point_;

	int result_;

	uint64_t timeStart_;
public:
	StgStageInformation();
	virtual ~StgStageInformation();

	bool IsEnd() { return bEndStg_; }
	void SetEnd() { bEndStg_ = true; }
	bool IsPause() { return bPause_; }
	void SetPause(bool bPause) { bPause_ = bPause; }
	bool IsReplay() { return bReplay_; }
	void SetReplay(bool bReplay) { bReplay_ = bReplay; }
	DWORD GetCurrentFrame() { return frame_; }
	void AdvanceFrame() { frame_++; }
	int GetStageIndex() { return stageIndex_; }
	void SetStageIndex(int index) { stageIndex_ = index; }

	ref_count_ptr<ScriptInformation> GetMainScriptInformation() { return infoMainScript_; }
	void SetMainScriptInformation(ref_count_ptr<ScriptInformation> info) { infoMainScript_ = info; }
	ref_count_ptr<ScriptInformation> GetPlayerScriptInformation() { return infoPlayerScript_; }
	void SetPlayerScriptInformation(ref_count_ptr<ScriptInformation> info) { infoPlayerScript_ = info; }
	ref_count_ptr<StgPlayerInformation> GetPlayerObjectInformation() { return infoPlayerObject_; }
	void SetPlayerObjectInformation(ref_count_ptr<StgPlayerInformation> info) { infoPlayerObject_ = info; }
	ref_count_ptr<ReplayInformation::StageData> GetReplayData() { return replayStageData_; }
	void SetReplayData(ref_count_ptr<ReplayInformation::StageData> data) { replayStageData_ = data; }

	DxRect<LONG>* GetStgFrameRect() { return &rcStgFrame_; }
	void SetStgFrameRect(const DxRect<LONG>& rect, bool bUpdateFocusResetValue = true);
	int GetStgFrameMinPriority() { return priMinStgFrame_; }
	void SetStgFrameMinPriority(int pri) { priMinStgFrame_ = pri; }
	int GetStgFrameMaxPriority() { return priMaxStgFrame_; }
	void SetStgFrameMaxPriority(int pri) { priMaxStgFrame_ = pri; }
	int GetShotObjectPriority() { return priShotObject_; }
	void SetShotObjectPriority(int pri) { priShotObject_ = pri; }
	int GetItemObjectPriority() { return priItemObject_; }
	void SetItemObjectPriority(int pri) { priItemObject_ = pri; }
	int GetCameraFocusPermitPriority() { return priCameraFocusPermit_; }
	void SetCameraFocusPermitPriority(int pri) { priCameraFocusPermit_ = pri; }

	shared_ptr<RandProvider> GetRandProvider() { return rand_; }
	//void SetRandProviderSeed(int seed) { rand_->Initialize(seed); }
	int64_t GetScore() { return score_; }
	void SetScore(int64_t score) { score_ = score; }
	void AddScore(int64_t inc) { score_ += inc; }
	int64_t GetGraze() { return graze_; }
	void SetGraze(int64_t graze) { graze_ = graze; }
	void AddGraze(int64_t inc) { graze_ += inc; }
	int64_t GetPoint() { return point_; }
	void SetPoint(int64_t point) { point_ = point; }
	void AddPoint(int64_t inc) { point_ += inc; }

	int GetResult() { return result_; }
	void SetResult(int result) { result_ = result; }

	uint64_t GetStageStartTime() { return timeStart_; }
	void SetStageStartTime(uint64_t time) { timeStart_ = time; }
};

//*******************************************************************
//StgStageStartData
//*******************************************************************
class StgStageStartData {
private:
	ref_count_ptr<StgStageInformation> infoStage_;
	ref_count_ptr<ReplayInformation::StageData> replayStageData_;
	ref_count_ptr<StgStageInformation> prevStageInfo_;
	ref_count_ptr<StgPlayerInformation> prevPlayerInfo_;
public:
	StgStageStartData() {}
	virtual ~StgStageStartData() {}

	ref_count_ptr<StgStageInformation> GetStageInformation() { return infoStage_; }
	void SetStageInformation(ref_count_ptr<StgStageInformation> info) { infoStage_ = info; }
	ref_count_ptr<ReplayInformation::StageData> GetStageReplayData() { return replayStageData_; }
	void SetStageReplayData(ref_count_ptr<ReplayInformation::StageData> data) { replayStageData_ = data; }
	ref_count_ptr<StgStageInformation> GetPrevStageInformation() { return prevStageInfo_; }
	void SetPrevStageInformation(ref_count_ptr<StgStageInformation> info) { prevStageInfo_ = info; }
	ref_count_ptr<StgPlayerInformation> GetPrevPlayerInformation() { return prevPlayerInfo_; }
	void SetPrevPlayerInformation(ref_count_ptr<StgPlayerInformation> info) { prevPlayerInfo_ = info; }

};

//*******************************************************************
//PseudoSlowInformation
//*******************************************************************
class PseudoSlowInformation : public gstd::FpsControlObject {
public:
	class SlowData {
	private:
		DWORD fps_;
	public:
		SlowData() { fps_ = 60; }
		virtual ~SlowData() {}

		DWORD GetFps() { return fps_; }
		void SetFps(DWORD fps) { fps_ = fps; }
	};

	enum {
		OWNER_PLAYER = 0,
		OWNER_ENEMY,
		TARGET_ALL,
	};
private:
	int current_;
	std::map<int, gstd::ref_count_ptr<SlowData>> mapDataPlayer_;
	std::map<int, gstd::ref_count_ptr<SlowData>> mapDataEnemy_;
	std::map<int, bool> mapValid_;
public:
	PseudoSlowInformation() { current_ = 0; }
	virtual ~PseudoSlowInformation() {}

	virtual DWORD GetFps();

	bool IsValidFrame(int target);
	void Next();

	void AddSlow(DWORD fps, int owner, int target);
	void RemoveSlow(int owner, int target);
};
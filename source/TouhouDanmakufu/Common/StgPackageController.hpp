#ifndef __TOUHOUDANMAKUFU_DNHSTG_PACKAGECONTROLLER__
#define __TOUHOUDANMAKUFU_DNHSTG_PACKAGECONTROLLER__

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgPackageScript.hpp"

class StgPackageInformation;
/**********************************************************
//StgPackageController
**********************************************************/
class StgPackageController {
private:
	StgSystemController* systemController_;
	ref_count_ptr<StgPackageInformation> infoPackage_;
	std::shared_ptr<StgPackageScriptManager> scriptManager_;

public:
	StgPackageController(StgSystemController* systemController);
	virtual ~StgPackageController();
	void Initialize();

	void Work();
	void Render();
	void RenderToTransitionTexture();

	StgSystemController* GetSystemController() { return systemController_; }
	ref_count_ptr<StgPackageInformation> GetPackageInformation() { return infoPackage_; }

	StgPackageScriptManager* GetScriptManager() { return scriptManager_.get(); }
	std::shared_ptr<StgPackageScriptManager> GetScriptManagerRef() { return scriptManager_; }
	DxScriptObjectManager* GetMainObjectManager() { return scriptManager_->GetObjectManager(); }
};

/**********************************************************
//StgPackageInformation
**********************************************************/
class StgPackageInformation {
	bool bEndPackage_;
	ref_count_ptr<StgStageStartData> nextStageStartData_;
	ref_count_ptr<ReplayInformation> infoReplay_;
	std::vector<ref_count_ptr<StgStageStartData> > listStageData_;
	ref_count_ptr<ScriptInformation> infoMainScript_;
	int timeStart_;

public:
	StgPackageInformation();
	virtual ~StgPackageInformation();

	bool IsEnd() { return bEndPackage_; }
	void SetEnd() { bEndPackage_ = true; }

	void InitializeStageData();
	void FinishCurrentStage();
	ref_count_ptr<StgStageStartData> GetNextStageData() { return nextStageStartData_; }
	void SetNextStageData(ref_count_ptr<StgStageStartData> data) { nextStageStartData_ = data; }
	std::vector<ref_count_ptr<StgStageStartData> >& GetStageDataList() { return listStageData_; }

	ref_count_ptr<ReplayInformation> GetReplayInformation() { return infoReplay_; }
	void SetReplayInformation(ref_count_ptr<ReplayInformation> info) { infoReplay_ = info; }

	ref_count_ptr<ScriptInformation> GetMainScriptInformation() { return infoMainScript_; }
	void SetMainScriptInformation(ref_count_ptr<ScriptInformation> info) { infoMainScript_ = info; }

	int GetPackageStartTime() { return timeStart_; }
	void SetPackageStartTime(int time) { timeStart_ = time; }
};

#endif

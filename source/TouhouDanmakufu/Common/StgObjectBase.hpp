#pragma once

#include "../../GcLib/pch.h"

//*******************************************************************
//StgObjectBase
//*******************************************************************
class StgStageController;
class StgObjectBase {
protected:
	StgStageController* stageController_;

	StgObjectBase(StgStageController* stageController) {
		stageController_ = stageController;
	}
public:
	StgStageController* GetStageController() { return stageController_; }
};
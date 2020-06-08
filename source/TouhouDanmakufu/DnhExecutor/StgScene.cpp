#include "source/GcLib/pch.h"

#include "StgScene.hpp"
#include "System.hpp"

/**********************************************************
//EStgSystemController
**********************************************************/
void EStgSystemController::DoEnd() {
	SystemController::GetInstance()->GetSceneManager()->TransScriptSelectScene_Last();
}
void EStgSystemController::DoRetry() {
	SceneManager* sceneManager = SystemController::GetInstance()->GetSceneManager();
	ref_count_ptr<StgStageInformation> infoStage = stageController_->GetStageInformation();
	sceneManager->TransStgScene(infoStage->GetMainScriptInformation(), infoStage->GetPlayerScriptInformation(), NULL);
}


/**********************************************************
//PStgSystemController
**********************************************************/
void PStgSystemController::DoEnd() {
	EDirectGraphics* graphics = EDirectGraphics::CreateInstance();
	graphics->SetWindowVisible(false);
	EApplication::GetInstance()->End();
}
void PStgSystemController::DoRetry() {
	SystemController::GetInstance()->Reset();
}

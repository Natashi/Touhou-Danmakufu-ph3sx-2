#include "source/GcLib/pch.h"

#include "StgScene.hpp"
#include "System.hpp"

//*******************************************************************
//EStgSystemController
//*******************************************************************
void EStgSystemController::DoEnd() {
    SystemController* systemController = SystemController::GetInstance();
    systemController->GetSceneManager()->TransScriptSelectScene_Last();
    systemController->ResetWindowTitle();

    EShaderManager* shaderManager = EShaderManager::GetInstance();
    shaderManager->Clear();

    ETaskManager* taskManager = ETaskManager::GetInstance();
    taskManager->RemoveTask(typeid(EStgSystemController));
}
void EStgSystemController::DoRetry() {
    SystemController* systemController = SystemController::GetInstance();
    SceneManager* sceneManager = systemController->GetSceneManager();
    ref_count_ptr<StgStageInformation> infoStage = stageController_->GetStageInformation();
    sceneManager->TransStgScene(infoStage->GetMainScriptInformation(), infoStage->GetPlayerScriptInformation(), nullptr);
    systemController->ResetWindowTitle();

    EShaderManager* shaderManager = EShaderManager::GetInstance();
    shaderManager->Clear();
}


//*******************************************************************
//PStgSystemController
//*******************************************************************
void PStgSystemController::DoEnd() {
	EDirectGraphics* graphics = EDirectGraphics::CreateInstance();
	graphics->SetWindowVisible(false);
	EApplication::GetInstance()->End();

	ETaskManager* taskManager = ETaskManager::GetInstance();
	taskManager->RemoveTask(typeid(PStgSystemController));
}
void PStgSystemController::DoRetry() {
	SystemController::GetInstance()->Reset();

	EShaderManager* shaderManager = EShaderManager::GetInstance();
	shaderManager->Clear();
}

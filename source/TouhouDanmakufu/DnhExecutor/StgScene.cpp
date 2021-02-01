#include "source/GcLib/pch.h"

#include "StgScene.hpp"
#include "System.hpp"

/**********************************************************
//EStgSystemController
**********************************************************/
void EStgSystemController::DoEnd() {
	SystemController::GetInstance()->GetSceneManager()->TransScriptSelectScene_Last();

	EShaderManager* shaderManager = EShaderManager::GetInstance();
	shaderManager->Clear();

	ETaskManager* taskManager = ETaskManager::GetInstance();
	taskManager->RemoveTask(typeid(EStgSystemController));
}
void EStgSystemController::DoRetry() {
	SceneManager* sceneManager = SystemController::GetInstance()->GetSceneManager();
	ref_count_ptr<StgStageInformation> infoStage = stageController_->GetStageInformation();
	sceneManager->TransStgScene(infoStage->GetMainScriptInformation(), infoStage->GetPlayerScriptInformation(), nullptr);

	EShaderManager* shaderManager = EShaderManager::GetInstance();
	shaderManager->Clear();
}


/**********************************************************
//PStgSystemController
**********************************************************/
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

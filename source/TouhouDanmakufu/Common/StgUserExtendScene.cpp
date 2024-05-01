#include "source/GcLib/pch.h"

#include "StgUserExtendScene.hpp"

#include "StgSystem.hpp"
#include "StgStageScript.hpp"

//*******************************************************************
//StgUserExtendScene
//*******************************************************************
StgUserExtendScene::StgUserExtendScene(StgSystemController* controller) {
	systemController_ = controller;
	scriptManager_ = nullptr;
}
StgUserExtendScene::~StgUserExtendScene() {
}
void StgUserExtendScene::_InitializeScript(const std::wstring& path, int type) {
	if (scriptManager_ == nullptr) return;
	auto idScript = scriptManager_->LoadScript(scriptManager_, path, type);
	scriptManager_->StartScript(idScript);
}
void StgUserExtendScene::_CallScriptMainLoop() {
	if (scriptManager_ == nullptr) return;
	scriptManager_->Work();
}
void StgUserExtendScene::_CallScriptFinalize() {
	if (scriptManager_ == nullptr) return;
	scriptManager_->CallScriptFinalizeAll();
}
void StgUserExtendScene::_AddRelativeManager() {
	if (scriptManager_ == nullptr) return;

	shared_ptr<ScriptManager> scriptManager = std::dynamic_pointer_cast<ScriptManager>(scriptManager_);

	StgStageController* stageController = systemController_->GetStageController();
	if (stageController) {
		shared_ptr<ScriptManager> stageScriptManager = stageController->GetScriptManager();
		if (stageScriptManager)
			ScriptManager::AddRelativeScriptManagerMutual(scriptManager, stageScriptManager);
	}

	StgPackageController* packageController = systemController_->GetPackageController();
	if (packageController) {
		shared_ptr<ScriptManager> packageScriptManager = packageController->GetScriptManager();
		if (packageScriptManager)
			ScriptManager::AddRelativeScriptManagerMutual(scriptManager, packageScriptManager);
	}
}

void StgUserExtendScene::Work() {}
void StgUserExtendScene::Render() {
	if (scriptManager_ == nullptr) return;
	scriptManager_->Render();
}

void StgUserExtendScene::Start() {}
void StgUserExtendScene::Finish() {}

//*******************************************************************
//StgUserExtendSceneScriptManager
//*******************************************************************
StgUserExtendSceneScriptManager::StgUserExtendSceneScriptManager(StgSystemController* controller) {
	systemController_ = controller;
	objectManager_ = std::shared_ptr<DxScriptObjectManager>(new DxScriptObjectManager);
}
StgUserExtendSceneScriptManager::~StgUserExtendSceneScriptManager() {}
void StgUserExtendSceneScriptManager::Work() {
	if (!IsError()) {
		objectManager_->CleanupObject();
		StgControlScriptManager::Work();
		objectManager_->WorkObject();
	}
	else {
		systemController_->GetSystemInformation()->SetError(error_);
	}

}
void StgUserExtendSceneScriptManager::Render() {
	objectManager_->RenderObject();
}
shared_ptr<ManagedScript> StgUserExtendSceneScriptManager::Create(shared_ptr<ScriptManager> manager, int type) {
	shared_ptr<ManagedScript> res;
	switch (type) {
	case StgUserExtendSceneScript::TYPE_PAUSE_SCENE:
		res = make_shared<StgPauseSceneScript>(systemController_);
		break;
	case StgUserExtendSceneScript::TYPE_END_SCENE:
		res = make_shared<StgEndSceneScript>(systemController_);
		break;
	case StgUserExtendSceneScript::TYPE_REPLAY_SCENE:
		res = make_shared<StgReplaySaveScript>(systemController_);
		break;
	}

	if (res) {
		res->SetObjectManager(objectManager_);
		res->SetScriptManager(manager);
	}

	return res;
}
void StgUserExtendSceneScriptManager::CallScriptFinalizeAll() {
	std::map<std::string, script_block*>::iterator itrEvent;
	for (auto itr = listScriptRun_.begin(); itr != listScriptRun_.end(); itr++) {
		shared_ptr<ManagedScript> script = (*itr);
		if (script->IsEventExists("Finalize", itrEvent))
			script->Run(itrEvent);
	}
}
gstd::value StgUserExtendSceneScriptManager::GetResultValue() {
	gstd::value res;
	for (auto itr = listScriptRun_.begin(); itr != listScriptRun_.end(); itr++) {
		shared_ptr<ManagedScript> script = (*itr);
		gstd::value v = script->GetResultValue();
		if (v.has_data()) {
			res = v;
			break;
		}
	}
	return res;
}

//*******************************************************************
//StgUserExtendSceneScript
//*******************************************************************
StgUserExtendSceneScript::StgUserExtendSceneScript(StgSystemController* systemController) : StgControlScript(systemController) {
	auto stageController = systemController_->GetStageController();
	auto scriptManager = stageController->GetScriptManager();

	mainThreadID_ = scriptManager->GetMainThreadID();
}
StgUserExtendSceneScript::~StgUserExtendSceneScript() {}

//*******************************************************************
//StgPauseScene
//*******************************************************************
StgPauseScene::StgPauseScene(StgSystemController* controller) : StgUserExtendScene(controller) {}
StgPauseScene::~StgPauseScene() {}
void StgPauseScene::Work() {
	if (scriptManager_ == nullptr) return;
	_CallScriptMainLoop();

	auto infoSystem = systemController_->GetSystemInformation();
	auto infoStage = systemController_->GetStageController()->GetStageInformation();
	gstd::value resValue = scriptManager_->GetResultValue();
	if (resValue.has_data()) {
		int result = resValue.as_int();
		if (result == StgControlScript::RESULT_CANCEL) {
		}
		else if (result == StgControlScript::RESULT_END) {
			if (infoSystem->IsPackageMode()) {
				infoStage->SetResult(StgStageInformation::RESULT_BREAK_OFF);
				infoStage->SetEnd();
				//infoSystem->SetScene(StgSystemInformation::SCENE_PACKAGE_CONTROL);
			}
			else {
				infoSystem->SetStgEnd();
			}
		}
		else if (result == StgControlScript::RESULT_RETRY) {
			if (!infoStage->IsReplay())
				infoSystem->SetRetry();
		}
		EDirectInput::GetInstance()->ResetInputState();
		Finish();
	}
}

void StgPauseScene::Start() {
	//停止イベント呼び出し
	auto stageController = systemController_->GetStageController();

	//停止処理初期化
	scriptManager_ = nullptr;
	scriptManager_ = std::shared_ptr<StgUserExtendSceneScriptManager>(new StgUserExtendSceneScriptManager(systemController_));
	_AddRelativeManager();

	auto sysInfo = systemController_->GetSystemInformation();

	stageController->RenderToTransitionTexture();

	//スクリプト初期化
	const std::wstring& path = sysInfo->GetPauseScriptPath();
	_InitializeScript(path, StgUserExtendSceneScript::TYPE_PAUSE_SCENE);

	auto stageScriptManager = stageController->GetScriptManager();
	stageScriptManager->RequestEventAll(StgStageScript::EV_PAUSE_ENTER);

	if (stageController)
		stageController->GetStageInformation()->SetPause(true);
}
void StgPauseScene::Finish() {
	auto stageController = systemController_->GetStageController();
	if (stageController)
		stageController->GetStageInformation()->SetPause(false);

	if (scriptManager_ == nullptr) return;
	_CallScriptFinalize();
	scriptManager_ = nullptr;

	//解除イベント呼び出し
	auto stageScriptManager = stageController->GetScriptManager();
	stageScriptManager->RequestEventAll(StgStageScript::EV_PAUSE_LEAVE);
}

//*******************************************************************
//StgPauseSceneScript
//*******************************************************************
static const std::vector<constant> stgPauseConstant = {
	constant("__stgPauseFunction__", 0),
};

StgPauseSceneScript::StgPauseSceneScript(StgSystemController* controller) : StgUserExtendSceneScript(controller) {
	typeScript_ = TYPE_PAUSE_SCENE;
	_AddConstant(&stgPauseConstant);
}
StgPauseSceneScript::~StgPauseSceneScript() {}

//一時停止専用関数：一時停止操作

//*******************************************************************
//StgEndScene
//*******************************************************************
StgEndScene::StgEndScene(StgSystemController* controller) : StgUserExtendScene(controller) {

}
StgEndScene::~StgEndScene() {}
void StgEndScene::Work() {
	if (scriptManager_ == nullptr) return;
	_CallScriptMainLoop();

	auto infoSystem = systemController_->GetSystemInformation();
	gstd::value resValue = scriptManager_->GetResultValue();
	if (resValue.has_data()) {
		int result = resValue.as_int();
		if (result == StgControlScript::RESULT_SAVE_REPLAY) {
			//info->SetStgEnd();
			systemController_->TransReplaySaveScene();
		}
		else if (result == StgControlScript::RESULT_END) {
			infoSystem->SetStgEnd();
		}
		else if (result == StgControlScript::RESULT_RETRY) {
			infoSystem->SetRetry();
		}
		EDirectInput::GetInstance()->ResetInputState();
		Finish();
	}
}

void StgEndScene::Start() {
	scriptManager_ = nullptr;
	scriptManager_ = std::shared_ptr<StgUserExtendSceneScriptManager>(new StgUserExtendSceneScriptManager(systemController_));
	_AddRelativeManager();

	auto info = systemController_->GetSystemInformation();

	systemController_->GetStageController()->RenderToTransitionTexture();

	//スクリプト初期化
	const std::wstring& path = info->GetEndSceneScriptPath();
	_InitializeScript(path, StgUserExtendSceneScript::TYPE_END_SCENE);
}
void StgEndScene::Finish() {
	auto info = systemController_->GetSystemInformation();

	if (scriptManager_ == nullptr) return;
	_CallScriptFinalize();
	scriptManager_ = nullptr;
}

//*******************************************************************
//StgEndSceneScript
//*******************************************************************
static const std::vector<constant> stgEndFunction = {
	constant("__stgEndFunction__", 0),
};
StgEndSceneScript::StgEndSceneScript(StgSystemController* controller) : StgUserExtendSceneScript(controller) {
	typeScript_ = TYPE_END_SCENE;
	_AddConstant(&stgEndFunction);
}
StgEndSceneScript::~StgEndSceneScript() {}

//*******************************************************************
//StgReplaySaveScene
//*******************************************************************
StgReplaySaveScene::StgReplaySaveScene(StgSystemController* controller) : StgUserExtendScene(controller) {

}
StgReplaySaveScene::~StgReplaySaveScene() {}
void StgReplaySaveScene::Work() {
	if (scriptManager_ == nullptr) return;
	_CallScriptMainLoop();

	auto infoSystem = systemController_->GetSystemInformation();
	gstd::value resValue = scriptManager_->GetResultValue();
	if (resValue.has_data()) {
		int result = resValue.as_int();
		if (result == StgControlScript::RESULT_END) {
			infoSystem->SetStgEnd();
		}
		else if (result == StgControlScript::RESULT_CANCEL) {
			systemController_->TransStgEndScene();
		}

		EDirectInput::GetInstance()->ResetInputState();
		Finish();
	}
}

void StgReplaySaveScene::Start() {
	scriptManager_ = nullptr;
	scriptManager_ = std::shared_ptr<StgUserExtendSceneScriptManager>(new StgUserExtendSceneScriptManager(systemController_));
	_AddRelativeManager();

	auto info = systemController_->GetSystemInformation();

	//_InitializeTransitionTexture();

	//スクリプト初期化
	const std::wstring& path = info->GetReplaySaveSceneScriptPath();
	_InitializeScript(path, StgUserExtendSceneScript::TYPE_REPLAY_SCENE);
}
void StgReplaySaveScene::Finish() {
	auto info = systemController_->GetSystemInformation();

	if (scriptManager_ == nullptr) return;
	_CallScriptFinalize();
	scriptManager_ = nullptr;
}

//*******************************************************************
//StgReplaySaveScript
//*******************************************************************
static const std::vector<constant> stgReplaySaveFunction = {
	constant("__stgReplaySaveFunction__", 0),
};
StgReplaySaveScript::StgReplaySaveScript(StgSystemController* controller) : StgUserExtendSceneScript(controller) {
	typeScript_ = TYPE_REPLAY_SCENE;
	_AddConstant(&stgReplaySaveFunction);
}
StgReplaySaveScript::~StgReplaySaveScript() {}
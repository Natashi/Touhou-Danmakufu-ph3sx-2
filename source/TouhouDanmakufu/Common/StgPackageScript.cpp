#include "source/GcLib/pch.h"

#include "StgPackageScript.hpp"
#include "StgSystem.hpp"
#include "StgPackageController.hpp"

//*******************************************************************
//StgPackageScriptManager
//*******************************************************************
StgPackageScriptManager::StgPackageScriptManager(StgSystemController* controller) {
	systemController_ = controller;
	objectManager_ = std::shared_ptr<DxScriptObjectManager>(new DxScriptObjectManager);
}
StgPackageScriptManager::~StgPackageScriptManager() {}
void StgPackageScriptManager::Work() {
	if (!IsError()) {
		objectManager_->CleanupObject();
		StgControlScriptManager::Work();
		objectManager_->WorkObject();
	}
	else {
		systemController_->GetSystemInformation()->SetError(error_);
	}

}
void StgPackageScriptManager::Render() {
	objectManager_->RenderObject();
}
shared_ptr<ManagedScript> StgPackageScriptManager::Create(int type) {
	shared_ptr<ManagedScript> res;
	switch (type) {
	case StgPackageScript::TYPE_PACKAGE_MAIN:
		res = std::make_shared<StgPackageScript>(systemController_->GetPackageController());
		break;
	}

	if (res) {
		res->SetObjectManager(objectManager_);
		res->SetScriptManager(this);
	}

	return res;
}


//*******************************************************************
//StgPackageScript
//*******************************************************************
static const std::vector<function> stgPackageFunction = {
	//パッケージ共通関数：パッケージ操作
	{ "ClosePackage", StgPackageScript::Func_ClosePackage, 0 },

	//パッケージ共通関数：ステージ操作
	{ "InitializeStageScene", StgPackageScript::Func_InitializeStageScene, 0 },
	{ "FinalizeStageScene", StgPackageScript::Func_FinalizeStageScene, 0 },
	{ "StartStageScene", StgPackageScript::Func_StartStageScene, 0 },
	{ "SetStageIndex", StgPackageScript::Func_SetStageIndex, 1 },
	{ "SetStageMainScript", StgPackageScript::Func_SetStageMainScript, 1 },
	{ "SetStagePlayerScript", StgPackageScript::Func_SetStagePlayerScript, 1 },
	{ "SetStageReplayFile", StgPackageScript::Func_SetStageReplayFile, 1 },
	{ "GetStageSceneState", StgPackageScript::Func_GetStageSceneState, 0 },
	{ "GetStageSceneResult", StgPackageScript::Func_GetStageSceneResult, 0 },
	{ "PauseStageScene", StgPackageScript::Func_PauseStageScene, 1 },
	{ "TerminateStageScene", StgPackageScript::Func_TerminateStageScene, 0 },
};
static const std::vector<constant> stgPackageConstant = {
	//GetStageSceneState state
	constant("STAGE_STATE_FINISHED", StgPackageScript::STAGE_STATE_FINISHED),

	//TerminateStageScene results
	constant("STAGE_RESULT_BREAK_OFF", StgStageInformation::RESULT_BREAK_OFF),
	constant("STAGE_RESULT_PLAYER_DOWN", StgStageInformation::RESULT_PLAYER_DOWN),
	constant("STAGE_RESULT_CLEARED", StgStageInformation::RESULT_CLEARED),
};

StgPackageScript::StgPackageScript(StgPackageController* packageController) : StgControlScript(packageController->GetSystemController()) {
	_AddFunction(&stgPackageFunction);
	_AddConstant(&stgPackageConstant);
	{
		definedMacro_[L"SCRIPT_PACKAGE"] = L"";
	}

	typeScript_ = TYPE_PACKAGE_MAIN;
	packageController_ = packageController;
}
void StgPackageScript::_CheckNextStageExists() {
	ref_count_ptr<StgPackageInformation> infoPackage = packageController_->GetPackageInformation();
	ref_count_ptr<StgStageStartData> nextStageData = infoPackage->GetNextStageData();
	if (nextStageData == nullptr) RaiseError("Stage data not initialized.");
}

//パッケージ共通関数：パッケージ操作
gstd::value StgPackageScript::Func_ClosePackage(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;

	ref_count_ptr<StgPackageInformation> infoPackage = packageController->GetPackageInformation();
	infoPackage->SetEnd();

	return value();
}

//パッケージ共通関数：ステージ操作
gstd::value StgPackageScript::Func_InitializeStageScene(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;

	StgSystemController* systemController = packageController->GetSystemController();
	ref_count_ptr<StgSystemInformation> infoSystem = systemController->GetSystemInformation();
	infoSystem->SetActiveReplayInformation(nullptr);

	ref_count_ptr<StgPackageInformation> infoPackage = packageController->GetPackageInformation();
	infoPackage->InitializeStageData();

	return value();
}
gstd::value StgPackageScript::Func_FinalizeStageScene(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;
	StgSystemController* systemController = packageController->GetSystemController();
	ref_count_ptr<StgSystemInformation> infoSystem = systemController->GetSystemInformation();

	ref_count_ptr<StgPackageInformation> infoPackage = packageController->GetPackageInformation();
	if (infoPackage->GetNextStageData() != nullptr && infoPackage->GetNextStageData()->GetPrevStageInformation() == nullptr)
		script->RaiseError("Stage not yet finished.");

	std::vector<ref_count_ptr<StgStageStartData>> listStage = infoPackage->GetStageDataList();
	if (listStage.size() > 0) {
		ref_count_ptr<StgStageStartData> stageData = listStage[listStage.size() - 1];
		ref_count_ptr<StgStageInformation> infoStage = stageData->GetStageInformation();
		bool bReplay = infoStage->IsReplay();
		if (bReplay) return value();
	}

	ref_count_ptr<ReplayInformation> infoReplay = systemController->CreateReplayInformation();
	infoSystem->SetActiveReplayInformation(infoReplay);

	return value();
}
gstd::value StgPackageScript::Func_StartStageScene(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;
	script->_CheckNextStageExists();

	StgSystemController* systemController = packageController->GetSystemController();
	ref_count_ptr<StgSystemInformation> infoSystem = systemController->GetSystemInformation();
	ref_count_ptr<StgPackageInformation> infoPackage = packageController->GetPackageInformation();
	ref_count_ptr<StgStageStartData> nextStageData = infoPackage->GetNextStageData();
	ref_count_ptr<StgStageInformation> infoStage = nextStageData->GetStageInformation();

	int stageIndex = infoStage->GetStageIndex();
	ref_count_ptr<ReplayInformation> infoReplay = infoPackage->GetReplayInformation();
	std::wstring replayPlayerID;
	std::wstring replayPlayerScriptFileName;
	if (infoReplay) {
		ref_count_ptr<ReplayInformation::StageData> replayStageData = infoReplay->GetStageData(stageIndex);
		if (replayStageData == nullptr)
			script->RaiseError("Invalid stage replay index.");
		nextStageData->SetStageReplayData(replayStageData);

		//自機スクリプト
		replayPlayerID = replayStageData->GetPlayerScriptID();
		replayPlayerScriptFileName = replayStageData->GetPlayerScriptFileName();
	}
	else {
		ref_count_ptr<ScriptInformation> infoPlayer = infoStage->GetPlayerScriptInformation();
		if (infoPlayer) {
			replayPlayerID = infoPlayer->GetID();
			replayPlayerScriptFileName = PathProperty::GetFileName(infoPlayer->GetScriptPath());
		}
	}

	//自機を検索
	infoStage->SetPlayerScriptInformation(nullptr);
	ref_count_ptr<ScriptInformation> infoMain = infoSystem->GetMainScriptInformation();
	std::vector<ref_count_ptr<ScriptInformation>> listPlayer;
	if (infoMain->GetPlayerList().size() == 0) {
		const std::wstring& dir = EPathProperty::GetPlayerScriptRootDirectory();
		listPlayer = ScriptInformation::FindPlayerScriptInformationList(dir);
	}
	else {
		listPlayer = infoMain->CreatePlayerScriptInformationList();
	}

	for (size_t iPlayer = 0; iPlayer < listPlayer.size(); iPlayer++) {
		ref_count_ptr<ScriptInformation> tInfo = listPlayer[iPlayer];
		if (tInfo->GetID() != replayPlayerID) continue;
		std::wstring tPlayerScriptFileName = PathProperty::GetFileName(tInfo->GetScriptPath());
		if (tPlayerScriptFileName != replayPlayerScriptFileName) continue;

		infoStage->SetPlayerScriptInformation(tInfo);
		break;
	}

	if (infoStage->GetPlayerScriptInformation() == nullptr)
		script->RaiseError("Player not found.");

	//packageController->RenderToTransitionTexture();

	systemController->StartStgScene(nextStageData);

	return value();
}
gstd::value StgPackageScript::Func_SetStageIndex(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;
	script->_CheckNextStageExists();

	ref_count_ptr<StgPackageInformation> infoPackage = packageController->GetPackageInformation();
	ref_count_ptr<StgStageStartData> nextStageData = infoPackage->GetNextStageData();
	ref_count_ptr<StgStageInformation> infoStage = nextStageData->GetStageInformation();

	int stageIndex = argv[0].as_int();
	std::vector<ref_count_ptr<StgStageStartData>> listStage = infoPackage->GetStageDataList();
	for (size_t iStage = 0; iStage < listStage.size(); iStage++) {
		ref_count_ptr<StgStageStartData> stageData = listStage[iStage];
		if (stageIndex == stageData->GetStageInformation()->GetStageIndex())
			script->RaiseError("Stage index already used.");
	}

	infoStage->SetStageIndex(stageIndex);

	return value();
}
gstd::value StgPackageScript::Func_SetStageMainScript(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;
	script->_CheckNextStageExists();

	ref_count_ptr<StgPackageInformation> infoPackage = packageController->GetPackageInformation();
	ref_count_ptr<StgStageStartData> nextStageData = infoPackage->GetNextStageData();
	ref_count_ptr<StgStageInformation> infoStage = nextStageData->GetStageInformation();

	std::wstring path = argv[0].as_string();
	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr || !reader->Open()) {
		std::wstring error = ErrorUtility::GetFileNotFoundErrorMessage(path, true);
		script->RaiseError(L"SetStageMainScript: " + error);
	}

	std::string source = reader->ReadAllString();

	ref_count_ptr<ScriptInformation> infoScript =
		ScriptInformation::CreateScriptInformation(path, L"", source, false);
	if (infoScript == nullptr)
		script->RaiseError(L"SetStageMainScript: " + ErrorUtility::GetFileNotFoundErrorMessage(path, true));

	infoStage->SetMainScriptInformation(infoScript);

	return value();
}
gstd::value StgPackageScript::Func_SetStagePlayerScript(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;
	script->_CheckNextStageExists();

	ref_count_ptr<StgPackageInformation> infoPackage = packageController->GetPackageInformation();
	ref_count_ptr<StgStageStartData> nextStageData = infoPackage->GetNextStageData();
	ref_count_ptr<StgStageInformation> infoStage = nextStageData->GetStageInformation();

	std::wstring path = argv[0].as_string();
	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr || !reader->Open()) {
		std::wstring error = ErrorUtility::GetFileNotFoundErrorMessage(path, true);
		script->RaiseError(L"SetStagePlayerScript: " + error);
	}

	std::string source = reader->ReadAllString();

	ref_count_ptr<ScriptInformation> infoScript =
		ScriptInformation::CreateScriptInformation(path, L"", source);
	infoStage->SetPlayerScriptInformation(infoScript);

	return script->CreateBooleanValue(true);
}
gstd::value StgPackageScript::Func_SetStageReplayFile(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;
	script->_CheckNextStageExists();

	ref_count_ptr<StgPackageInformation> infoPackage = packageController->GetPackageInformation();

	std::wstring pathReplay = argv[0].as_string();
	ref_count_ptr<ReplayInformation> infoReplay =
		ReplayInformation::CreateFromFile(pathReplay);
	if (infoReplay == nullptr) {
		std::wstring path = ErrorUtility::GetFileNotFoundErrorMessage(pathReplay, true);
		script->RaiseError(L"SetStageReplayFile: " + path);
	}
	infoPackage->SetReplayInformation(infoReplay);
	return value();
}
gstd::value StgPackageScript::Func_GetStageSceneState(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;
	StgSystemController* systemController = packageController->GetSystemController();
	ref_count_ptr<StgSystemInformation> infoSystem = systemController->GetSystemInformation();

	int res = -1;
	int scene = infoSystem->GetScene();
	if (scene == StgSystemInformation::SCENE_PACKAGE_CONTROL)
		res = STAGE_STATE_FINISHED;

	return script->CreateIntValue(res);
}
gstd::value StgPackageScript::Func_GetStageSceneResult(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;
	script->_CheckNextStageExists();

	int res = StgStageInformation::RESULT_UNKNOWN;
	ref_count_ptr<StgPackageInformation> infoPackage = packageController->GetPackageInformation();
	std::vector<ref_count_ptr<StgStageStartData>> listStage = infoPackage->GetStageDataList();
	if (listStage.size() > 0) {
		ref_count_ptr<StgStageStartData> stageData = listStage[listStage.size() - 1];
		ref_count_ptr<StgStageInformation> infoStage = stageData->GetStageInformation();
		res = infoStage->GetResult();
	}

	return script->CreateIntValue(res);
}
gstd::value StgPackageScript::Func_PauseStageScene(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;
	StgSystemController* systemController = packageController->GetSystemController();
	shared_ptr<StgStageController> stageController = systemController->GetStageController();
	if (stageController) {
		bool bPause = argv[0].as_boolean();

		auto stageScriptManager = stageController->GetScriptManager();
		ref_count_ptr<StgStageInformation> infoStage = stageController->GetStageInformation();
		if (bPause && !infoStage->IsPause())
			stageScriptManager->RequestEventAll(StgStageScript::EV_PAUSE_ENTER);
		else if (!bPause && infoStage->IsPause())
			stageScriptManager->RequestEventAll(StgStageScript::EV_PAUSE_LEAVE);

		infoStage->SetPause(bPause);
	}
	return value();
}
gstd::value StgPackageScript::Func_TerminateStageScene(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgPackageScript* script = (StgPackageScript*)machine->data;
	StgPackageController* packageController = script->packageController_;
	StgSystemController* systemController = packageController->GetSystemController();
	shared_ptr<StgStageController> stageController = systemController->GetStageController();
	if (stageController) {
		ref_count_ptr<StgStageInformation> infoStage = stageController->GetStageInformation();
		infoStage->SetResult(StgStageInformation::RESULT_BREAK_OFF);
		infoStage->SetEnd();
	}
	return value();
}
#include "source/GcLib/pch.h"

#include "StgControlScript.hpp"
#include "StgSystem.hpp"

#include "../DnhExecutor/GcLibImpl.hpp"

//*******************************************************************
//StgControlScriptManager
//*******************************************************************
StgControlScriptManager::StgControlScriptManager() {

}
StgControlScriptManager::~StgControlScriptManager() {

}

//*******************************************************************
//StgControlScriptInformation
//*******************************************************************
StgControlScriptInformation::StgControlScriptInformation() {
	replayManager_.reset(new ReplayInformationManager());
}
StgControlScriptInformation::~StgControlScriptInformation() {}
void StgControlScriptInformation::LoadFreePlayerList() {
	const std::wstring& dir = EPathProperty::GetPlayerScriptRootDirectory();
	listFreePlayer_ = ScriptInformation::FindPlayerScriptInformationList(dir);

	std::sort(listFreePlayer_.begin(), listFreePlayer_.end(), 
		[](const ref_count_ptr<ScriptInformation>& r, const ref_count_ptr<ScriptInformation>& l) {
			return ScriptInformation::Sort::Compare(r, l);
		});
}
void StgControlScriptInformation::LoadReplayInformation(std::wstring pathMainScript) {
	replayManager_->UpdateInformationList(pathMainScript);
}

//*******************************************************************
//StgControlScript
//*******************************************************************
static const std::vector<function> stgControlFunction = {
	// Common data
	{ "SetCommonData", StgControlScript::Func_SetCommonData, 2 },
	{ "GetCommonData", StgControlScript::Func_GetCommonData, 2 },
	{ "GetCommonData", StgControlScript::Func_GetCommonData, 1 },	//Overloaded
	{ "ClearCommonData", StgControlScript::Func_ClearCommonData, 0 },
	{ "DeleteCommonData", StgControlScript::Func_DeleteCommonData, 1 },

	{ "SetAreaCommonData", StgControlScript::Func_SetAreaCommonData, 3 },
	{ "GetAreaCommonData", StgControlScript::Func_GetAreaCommonData, 3 },
	{ "GetAreaCommonData", StgControlScript::Func_GetAreaCommonData, 2 },	//Overloaded
	{ "ClearAreaCommonData", StgControlScript::Func_ClearAreaCommonData, 1 },
	{ "DeleteAreaCommonData", StgControlScript::Func_DeleteAreaCommonData, 2 },

	{ "DeleteWholeAreaCommonData", StgControlScript::Func_DeleteWholeAreaCommonData, 1 },
	{ "CreateCommonDataArea", StgControlScript::Func_CreateCommonDataArea, 1 },
	{ "CopyCommonDataArea", StgControlScript::Func_CopyCommonDataArea, 2 },
	{ "IsCommonDataAreaExists", StgControlScript::Func_IsCommonDataAreaExists, 1 },
	{ "GetCommonDataAreaKeyList", StgControlScript::Func_GetCommonDataAreaKeyList, 0 },
	{ "GetCommonDataValueKeyList", StgControlScript::Func_GetCommonDataValueKeyList, 1 },

	{ "LoadCommonDataValuePointer", StgControlScript::Func_LoadCommonDataValuePointer, 1 },
	{ "LoadCommonDataValuePointer", StgControlScript::Func_LoadCommonDataValuePointer, 2 },			//Overloaded
	{ "LoadAreaCommonDataValuePointer", StgControlScript::Func_LoadAreaCommonDataValuePointer, 2 },
	{ "LoadAreaCommonDataValuePointer", StgControlScript::Func_LoadAreaCommonDataValuePointer, 3 },	//Overloaded
	{ "IsValidCommonDataValuePointer", StgControlScript::Func_IsValidCommonDataValuePointer, 1 },
	{ "SetCommonDataPtr", StgControlScript::Func_SetCommonDataPtr, 2 },
	{ "GetCommonDataPtr", StgControlScript::Func_GetCommonDataPtr, 1 },
	{ "GetCommonDataPtr", StgControlScript::Func_GetCommonDataPtr, 2 },		//Overloaded

	// Common data save/load
	{ "SaveCommonDataAreaA1", StgControlScript::Func_SaveCommonDataAreaA1, 1 },
	{ "LoadCommonDataAreaA1", StgControlScript::Func_LoadCommonDataAreaA1, 1 },
	{ "SaveCommonDataAreaA2", StgControlScript::Func_SaveCommonDataAreaA2, 2 },
	{ "LoadCommonDataAreaA2", StgControlScript::Func_LoadCommonDataAreaA2, 2 },

	//STG制御共通関数：キー系
	{ "AddVirtualKey", StgControlScript::Func_AddVirtualKey, 3 },
	{ "AddReplayTargetVirtualKey", StgControlScript::Func_AddReplayTargetVirtualKey, 1 },
	{ "SetSkipModeKey", StgControlScript::Func_SetSkipModeKey, 1 },

	//STG制御共通関数：システム関連
	{ "GetScore", StgControlScript::Func_StgStageInformation_int64_void<&StgStageInformation::GetScore>, 0 },
	{ "SetScore", StgControlScript::Func_StgStageInformation_void_int64<&StgStageInformation::SetScore>, 1 },
	{ "AddScore", StgControlScript::Func_StgStageInformation_void_int64<&StgStageInformation::AddScore>, 1 },
	{ "GetGraze", StgControlScript::Func_StgStageInformation_int64_void<&StgStageInformation::GetGraze>, 0 },
	{ "SetGraze", StgControlScript::Func_StgStageInformation_void_int64<&StgStageInformation::SetGraze>, 1 },
	{ "AddGraze", StgControlScript::Func_StgStageInformation_void_int64<&StgStageInformation::AddGraze>, 1 },
	{ "GetPoint", StgControlScript::Func_StgStageInformation_int64_void<&StgStageInformation::GetPoint>, 0 },
	{ "SetPoint", StgControlScript::Func_StgStageInformation_void_int64<&StgStageInformation::SetPoint>, 1 },
	{ "AddPoint", StgControlScript::Func_StgStageInformation_void_int64<&StgStageInformation::AddPoint>, 1 },

	{ "IsReplay", StgControlScript::Func_IsReplay, 0 },

	{ "AddArchiveFile", StgControlScript::Func_AddArchiveFile, 1 },
	{ "AddArchiveFile", StgControlScript::Func_AddArchiveFile, 2 },		//Overloaded
	{ "GetArchiveFilePathList", StgControlScript::Func_GetArchiveFilePathList, 2 },

	{ "GetCurrentFps", StgControlScript::Func_GetCurrentRenderFps, 0 },
	{ "GetCurrentUpdateFps", StgControlScript::Func_GetCurrentUpdateFps, 0 },
	{ "GetCurrentRenderFps", StgControlScript::Func_GetCurrentRenderFps, 0 },
	{ "GetLastFrameUpdateSpeed", StgControlScript::Func_GetLastFrameUpdateSpeed, 0 },
	{ "GetLastFrameRenderSpeed", StgControlScript::Func_GetLastFrameRenderSpeed, 0 },
	{ "GetStageTime", StgControlScript::Func_GetStageTime, 0 },
	{ "GetStageTimeF", StgControlScript::Func_GetStageTimeF, 0 },
	{ "GetPackageTime", StgControlScript::Func_GetPackageTime, 0 },

	{ "GetStgFrameLeft", StgControlScript::Func_GetStgFrameLeft, 0 },
	{ "GetStgFrameTop", StgControlScript::Func_GetStgFrameTop, 0 },
	{ "GetStgFrameWidth", StgControlScript::Func_GetStgFrameWidth, 0 },
	{ "GetStgFrameHeight", StgControlScript::Func_GetStgFrameHeight, 0 },

	{ "GetMainPackageScriptPath", StgControlScript::Func_GetMainPackageScriptPath, 0 },
	{ "GetScriptPathList", StgControlScript::Func_GetScriptPathList, 2 },
	{ "GetScriptInfoA1", StgControlScript::Func_GetScriptInfoA1, 2 },

	//Engine utility
	{ "IsEngineFastMode", StgControlScript::Func_IsEngineFastMode, 0 },
	{ "GetConfigWindowSizeIndex", StgControlScript::Func_GetConfigWindowSizeIndex, 0 },
	{ "GetConfigWindowSizeList", StgControlScript::Func_GetConfigWindowSizeList, 0 },
	{ "GetConfigVirtualKeyMapping", StgControlScript::Func_GetConfigVirtualKeyMapping, 1 },
	{ "GetConfigWindowTitle", StgControlScript::Func_GetConfigWindowTitle, 0 },
	{ "SetWindowTitle", StgControlScript::Func_SetWindowTitle, 1 },
	{ "SetEnableUnfocusedProcessing", StgControlScript::Func_SetEnableUnfocusedProcessing, 1 },
	{ "IsWindowFocused", StgControlScript::Func_IsWindowFocused, 0 },

	//STG共通関数：描画関連
	{ "ClearInvalidRenderPriority", StgControlScript::Func_ClearInvalidRenderPriority, 0 },
	{ "SetInvalidRenderPriorityA1", StgControlScript::Func_SetInvalidRenderPriorityA1, 2 },
	{ "GetReservedRenderTargetName", StgControlScript::Func_GetReservedRenderTargetName, 1 },
	{ "RenderToTextureA1", StgControlScript::Func_RenderToTextureA<false>, 4 },
	{ "RenderToTextureA2", StgControlScript::Func_RenderToTextureA<true>, 4 },
	{ "RenderToTextureB1", StgControlScript::Func_RenderToTextureB<false>, 3 },
	{ "RenderToTextureB2", StgControlScript::Func_RenderToTextureB<true>, 3 },
	{ "SaveSnapShotA1", StgControlScript::Func_SaveSnapShotA1, 1 },
	{ "SaveSnapShotA2", StgControlScript::Func_SaveSnapShotA2, 5 },
	{ "SaveSnapShotA3", StgControlScript::Func_SaveSnapShotA3, 6 },

	//STG制御共通関数：自機関連
	{ "GetPlayerID", StgControlScript::Func_GetPlayerID, 0 },
	{ "GetPlayerReplayName", StgControlScript::Func_GetPlayerReplayName, 0 },

	//STG制御共通関数：ユーザスクリプト
	{ "SetPauseScriptPath", StgControlScript::Func_SetPauseScriptPath, 1 },
	{ "SetEndSceneScriptPath", StgControlScript::Func_SetEndSceneScriptPath, 1 },
	{ "SetReplaySaveSceneScriptPath", StgControlScript::Func_SetReplaySaveSceneScriptPath, 1 },

	//STG制御共通関数：自機スクリプト
	{ "GetLoadFreePlayerScriptList", StgControlScript::Func_GetLoadFreePlayerScriptList, 0 },
	{ "GetFreePlayerScriptCount", StgControlScript::Func_GetFreePlayerScriptCount, 0 },
	{ "GetFreePlayerScriptInfo", StgControlScript::Func_GetFreePlayerScriptInfo, 2 },

	//STG制御共通関数：リプレイ関連
	{ "LoadReplayList", StgControlScript::Func_LoadReplayList, 0 },
	{ "GetValidReplayIndices", StgControlScript::Func_GetValidReplayIndices, 0 },
	{ "IsValidReplayIndex", StgControlScript::Func_IsValidReplayIndex, 1 },
	{ "GetReplayInfo", StgControlScript::Func_GetReplayInfo, 2 },
	{ "SetReplayInfo", StgControlScript::Func_SetReplayInfo, 2 },
	{ "GetReplayUserData", StgControlScript::Func_GetReplayUserData, 2 },
	{ "SetReplayUserData", StgControlScript::Func_SetReplayUserData, 2 },
	{ "IsReplayUserDataExists", StgControlScript::Func_IsReplayUserDataExists, 2 },
	{ "SaveReplay", StgControlScript::Func_SaveReplay, 2 },
};
static const std::vector<constant> stgControlConstant = {
	//Events
	constant("EV_USER_COUNT", StgControlScript::EV_USER_COUNT),
	constant("EV_USER", StgControlScript::EV_USER),
	constant("EV_USER_SYSTEM", StgControlScript::EV_USER_SYSTEM),
	constant("EV_USER_STAGE", StgControlScript::EV_USER_STAGE),
	constant("EV_USER_PLAYER", StgControlScript::EV_USER_PLAYER),
	constant("EV_USER_PACKAGE", StgControlScript::EV_USER_PACKAGE),

	constant("EV_APP_LOSE_FOCUS", StgControlScript::EV_APP_LOSE_FOCUS),
	constant("EV_APP_RESTORE_FOCUS", StgControlScript::EV_APP_RESTORE_FOCUS),

	//GetScriptInfoA1 script types
	constant("TYPE_SCRIPT_ALL", StgControlScript::TYPE_SCRIPT_ALL),
	constant("TYPE_SCRIPT_PLAYER", StgControlScript::TYPE_SCRIPT_PLAYER),
	constant("TYPE_SCRIPT_SINGLE", StgControlScript::TYPE_SCRIPT_SINGLE),
	constant("TYPE_SCRIPT_PLURAL", StgControlScript::TYPE_SCRIPT_PLURAL),
	constant("TYPE_SCRIPT_STAGE", StgControlScript::TYPE_SCRIPT_STAGE),
	constant("TYPE_SCRIPT_PACKAGE", StgControlScript::TYPE_SCRIPT_PACKAGE),

	//Script infos
	constant("INFO_SCRIPT_TYPE", StgControlScript::INFO_SCRIPT_TYPE),
	constant("INFO_SCRIPT_PATH", StgControlScript::INFO_SCRIPT_PATH),
	constant("INFO_SCRIPT_ID", StgControlScript::INFO_SCRIPT_ID),
	constant("INFO_SCRIPT_TITLE", StgControlScript::INFO_SCRIPT_TITLE),
	constant("INFO_SCRIPT_TEXT", StgControlScript::INFO_SCRIPT_TEXT),
	constant("INFO_SCRIPT_IMAGE", StgControlScript::INFO_SCRIPT_IMAGE),
	constant("INFO_SCRIPT_REPLAY_NAME", StgControlScript::INFO_SCRIPT_REPLAY_NAME),

	//Replay data infos
	constant("REPLAY_FILE_PATH", StgControlScript::REPLAY_FILE_PATH),
	constant("REPLAY_DATE_TIME", StgControlScript::REPLAY_DATE_TIME),
	constant("REPLAY_USER_NAME", StgControlScript::REPLAY_USER_NAME),
	constant("REPLAY_TOTAL_SCORE", StgControlScript::REPLAY_TOTAL_SCORE),
	constant("REPLAY_FPS_AVERAGE", StgControlScript::REPLAY_FPS_AVERAGE),
	constant("REPLAY_PLAYER_NAME", StgControlScript::REPLAY_PLAYER_NAME),
	constant("REPLAY_STAGE_INDEX_LIST", StgControlScript::REPLAY_STAGE_INDEX_LIST),
	constant("REPLAY_STAGE_START_SCORE_LIST", StgControlScript::REPLAY_STAGE_START_SCORE_LIST),
	constant("REPLAY_STAGE_LAST_SCORE_LIST", StgControlScript::REPLAY_STAGE_LAST_SCORE_LIST),
	constant("REPLAY_COMMENT", StgControlScript::REPLAY_COMMENT),

	//Replay index
	constant("REPLAY_INDEX_ACTIVE", ReplayInformation::INDEX_ACTIVE),
	constant("REPLAY_INDEX_DIGIT_MIN", ReplayInformation::INDEX_DIGIT_MIN),
	constant("REPLAY_INDEX_DIGIT_MAX", ReplayInformation::INDEX_DIGIT_MAX),
	constant("REPLAY_INDEX_USER", ReplayInformation::INDEX_USER),

	//Common script results
	constant("RESULT_CANCEL", StgControlScript::RESULT_CANCEL),
	constant("RESULT_END", StgControlScript::RESULT_END),
	constant("RESULT_RETRY", StgControlScript::RESULT_RETRY),
	constant("RESULT_SAVE_REPLAY", StgControlScript::RESULT_SAVE_REPLAY),
};

StgControlScript::StgControlScript(StgSystemController* systemController) {
	systemController_ = systemController;

	_AddFunction(&stgControlFunction);
	_AddConstant(&stgControlConstant);

	SetScriptEngineCache(systemController->GetScriptEngineCache());
}

// Common data

value StgControlScript::Func_SetCommonData(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	auto area = commonDataManager->GetDefaultArea();
	std::string key = STR_MULTI(argv[0].as_string());

	area->SetValue(key, argv[1]);

	return value();
}
value StgControlScript::Func_GetCommonData(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	auto area = commonDataManager->GetDefaultArea();
	std::string key = STR_MULTI(argv[0].as_string());

	value res;

	if (auto pValue = area->GetValueRef(key))
		res = *pValue;

	if (!res.has_data() && argc == 2)
		res = argv[1];

	return res;
}
value StgControlScript::Func_ClearCommonData(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	auto area = commonDataManager->GetDefaultArea();
	area->Clear();

	return value();
}
value StgControlScript::Func_DeleteCommonData(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	auto area = commonDataManager->GetDefaultArea();
	std::string key = STR_MULTI(argv[0].as_string());

	area->DeleteValue(key);

	return value();
}
value StgControlScript::Func_SetAreaCommonData(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string area = STR_MULTI(argv[0].as_string());
	std::string key = STR_MULTI(argv[1].as_string());

	if (auto pArea = commonDataManager->GetArea(area)) {
		pArea->SetValue(key, argv[2]);
	}

	return value();
}
value StgControlScript::Func_GetAreaCommonData(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string area = STR_MULTI(argv[0].as_string());
	std::string key = STR_MULTI(argv[1].as_string());

	value res;

	if (auto pArea = commonDataManager->GetArea(area)) {
		if (auto pValue = pArea->GetValueRef(key))
			res = *pValue;
	}

	if (!res.has_data() && argc == 3)
			res = argv[2];

	return res;
}
value StgControlScript::Func_ClearAreaCommonData(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string area = STR_MULTI(argv[0].as_string());

	if (auto pArea = commonDataManager->GetArea(area)) {
		pArea->Clear();
	}

	return value();
}
value StgControlScript::Func_DeleteAreaCommonData(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string area = STR_MULTI(argv[0].as_string());
	std::string key = STR_MULTI(argv[1].as_string());

	if (auto pArea = commonDataManager->GetArea(area)) {
		pArea->DeleteValue(key);
	}

	return value();
}

value StgControlScript::Func_CreateCommonDataArea(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string area = STR_MULTI(argv->as_string());
	commonDataManager->CreateArea(area);

	return value();
}
value StgControlScript::Func_DeleteWholeAreaCommonData(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string area = STR_MULTI(argv->as_string());
	commonDataManager->Erase(area);

	return value();
}
value StgControlScript::Func_CopyCommonDataArea(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string areaDest = STR_MULTI(argv[0].as_string());
	std::string areaSrc = STR_MULTI(argv[1].as_string());

	if (auto pSrc = commonDataManager->GetArea(areaSrc)) {
		if (auto pDst = commonDataManager->GetArea(areaDest)) {
			pDst->Copy(pSrc);
		}
	}

	return value();
}

value StgControlScript::Func_IsCommonDataAreaExists(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string area = STR_MULTI(argv->as_string());
	bool res = commonDataManager->GetArea(area) != nullptr;

	return script->CreateBooleanValue(res);
}
value StgControlScript::Func_GetCommonDataAreaKeyList(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::vector<std::string> listKey;
	for (auto& [key, _] : *commonDataManager) {
		listKey.push_back(key);
	}

	return script->CreateStringArrayValue(listKey);
}
value StgControlScript::Func_GetCommonDataValueKeyList(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string area = STR_MULTI(argv->as_string());

	std::vector<std::string> listKey;

	if (auto pArea = commonDataManager->GetArea(area)) {
		for (auto& [key, _] : *pArea) {
			listKey.push_back(key);
		}
	}

	return script->CreateStringArrayValue(listKey);
}

value StgControlScript::Func_LoadCommonDataValuePointer(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	auto area = commonDataManager->GetDefaultArea();
	std::string key = STR_MULTI(argv[0].as_string());

	uint64_t res = 0;
	{
		value* pData = nullptr;

		if (auto pValue = area->GetValueRef(key)) {
			pData = pValue;
		}
		else if (argc == 2) {
			area->SetValue(key, argv[1]);
			pData = area->GetValueRef(key);
		}

		res = (uint64_t)ScriptCommonData_Pointer(area, pData);
	}

	return script->CreateIntValue((int64_t&)res);
}
value StgControlScript::Func_LoadAreaCommonDataValuePointer(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string nameArea = STR_MULTI(argv[0].as_string());
	std::string key = STR_MULTI(argv[1].as_string());

	uint64_t res = 0;
	if (auto area = commonDataManager->GetArea(nameArea)) {
		value* pData = nullptr;

		if (auto pValue = area->GetValueRef(key)) {
			pData = pValue;
		}
		else if (argc == 3) {
			area->SetValue(key, argv[2]);
			pData = area->GetValueRef(key);
		}

		res = (uint64_t)ScriptCommonData_Pointer(area, pData);
	}

	return script->CreateIntValue((int64_t&)res);
}
value StgControlScript::Func_SetCommonDataPtr(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	int64_t val = argv[0].as_int();

	if (auto cdPtr = ScriptCommonData_Pointer::From((uint64_t&)val)) {
		*cdPtr->data = argv[1];
	}

	return value();
}
value StgControlScript::Func_GetCommonDataPtr(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();
	
	int64_t val = argv[0].as_int();

	value res;

	if (auto cdPtr = ScriptCommonData_Pointer::From((uint64_t&)val)) {
		res = *cdPtr->data;
	}
	else if (argc == 2) {
		res = argv[1];
	}

	return res;
}
value StgControlScript::Func_IsValidCommonDataValuePointer(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = rcast(StgControlScript*, machine->data);
	auto commonDataManager = script->systemController_->GetCommonDataManager();

	int64_t val = argv[0].as_int();
	auto cdPtr = ScriptCommonData_Pointer::From((uint64_t&)val);

	return script->CreateBooleanValue(cdPtr.has_value());
}

// Common data save/load
gstd::value StgControlScript::Func_SaveCommonDataAreaA1(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	auto infoSystem = script->systemController_->GetSystemInformation();

	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::wstring nameAreaW = argv[0].as_string();
	std::string nameArea = STR_MULTI(nameAreaW);

	bool res = false;

	if (auto area = commonDataManager->GetArea(nameArea)) {
		const std::wstring& pathMain = infoSystem->GetMainScriptInformation()->pathScript_;

		std::wstring pathSave = EPathProperty::GetCommonDataPath(pathMain, nameAreaW);
		std::wstring dirSave = PathProperty::GetFileDirectory(pathSave);

		File::CreateFileDirectory(dirSave);

		RecordBuffer record;
		area->WriteRecord(record);
		res = record.WriteToFile(pathSave, DATA_VERSION_CAREA,
			ScriptCommonDataArea::HEADER_SAVED_DATA, 
			ScriptCommonDataArea::HEADER_SAVED_DATA_SIZE);
	}

	return script->CreateBooleanValue(res);
}
gstd::value StgControlScript::Func_LoadCommonDataAreaA1(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	auto infoSystem = script->systemController_->GetSystemInformation();

	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::wstring nameAreaW = argv[0].as_string();
	std::string nameArea = STR_MULTI(nameAreaW);

	bool res = false;

	const std::wstring& pathMain = infoSystem->GetMainScriptInformation()->pathScript_;
	std::wstring pathSave = EPathProperty::GetCommonDataPath(pathMain, nameAreaW);

	RecordBuffer record;
	res = record.ReadFromFile(pathSave, DATA_VERSION_CAREA,
		ScriptCommonDataArea::HEADER_SAVED_DATA, 
		ScriptCommonDataArea::HEADER_SAVED_DATA_SIZE);
	if (res) {
		auto commonData = make_unique<ScriptCommonDataArea>();
		commonData->ReadRecord(record);

		commonDataManager->SetArea(nameArea, MOVE(commonData));
	}

	return script->CreateBooleanValue(res);
}

gstd::value StgControlScript::Func_SaveCommonDataAreaA2(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	auto infoSystem = script->systemController_->GetSystemInformation();

	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string nameArea = STR_MULTI(argv[0].as_string());
	std::wstring pathSave = argv[1].as_string();

	bool res = false;

	if (auto area = commonDataManager->GetArea(nameArea)) {
		std::wstring dirSave = PathProperty::GetFileDirectory(pathSave);
		File::CreateFileDirectory(dirSave);

		RecordBuffer record;
		area->WriteRecord(record);
		res = record.WriteToFile(pathSave, DATA_VERSION_CAREA,
			ScriptCommonDataArea::HEADER_SAVED_DATA, 
			ScriptCommonDataArea::HEADER_SAVED_DATA_SIZE);
	}

	return script->CreateBooleanValue(res);
}
gstd::value StgControlScript::Func_LoadCommonDataAreaA2(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	auto infoSystem = script->systemController_->GetSystemInformation();

	auto commonDataManager = script->systemController_->GetCommonDataManager();

	std::string nameArea = STR_MULTI(argv[0].as_string());
	std::wstring pathSave = argv[1].as_string();

	bool res = false;
	
	RecordBuffer record;
	res = record.ReadFromFile(pathSave, DATA_VERSION_CAREA,
		ScriptCommonDataArea::HEADER_SAVED_DATA, 
		ScriptCommonDataArea::HEADER_SAVED_DATA_SIZE);
	if (res) {
		auto commonData = make_unique<ScriptCommonDataArea>();
		commonData->ReadRecord(record);

		commonDataManager->SetArea(nameArea, MOVE(commonData));
	}

	return script->CreateBooleanValue(res);
}

//STG制御共通関数：キー系
gstd::value StgControlScript::Func_AddVirtualKey(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	EDirectInput* input = EDirectInput::GetInstance();
	int padIndex = input->GetPadIndex();

	int16_t id = argv[0].as_int();
	int16_t key = argv[1].as_int();
	int16_t padButton = argv[2].as_int();

	ref_count_ptr<VirtualKey> vkey(new VirtualKey(key, padIndex, padButton));
	input->AddKeyMap(id, vkey);

	return value();
}
gstd::value StgControlScript::Func_AddReplayTargetVirtualKey(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	auto infoSystem = script->systemController_->GetSystemInformation();
	StgStageController* stageController = script->systemController_->GetStageController();

	int16_t id = argv[0].as_int();
	infoSystem->AddReplayTargetKey(id);
	if (stageController) {
		ref_count_ptr<KeyReplayManager> keyReplayManager = stageController->GetKeyReplayManager();
		keyReplayManager->AddTarget(id);
	}

	return value();
}
gstd::value StgControlScript::Func_SetSkipModeKey(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	EFpsController* fpsController = EFpsController::GetInstance();
	fpsController->SetFastModeKey((int16_t)argv[0].as_float());
	return value();
}

//STG制御共通関数：システム関連
template<int64_t(StgStageInformation::* Func)()>
gstd::value StgControlScript::Func_StgStageInformation_int64_void(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	int64_t res = 0;

	StgStageController* stageController = script->systemController_->GetStageController();
	if (stageController) {
		auto stgInfo = stageController->GetStageInformation().get();
		res = (stgInfo->*Func)();
	}

	return script->CreateIntValue(res);
}
template<void(StgStageInformation::* Func)(int64_t)>
gstd::value StgControlScript::Func_StgStageInformation_void_int64(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	StgStageController* stageController = script->systemController_->GetStageController();
	if (stageController) {
		int64_t inc = argv[0].as_int();
		auto stgInfo = stageController->GetStageInformation().get();
		(stgInfo->*Func)(inc);
	}

	return value();
}

gstd::value StgControlScript::Func_IsReplay(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	bool res = false;

	StgStageController* stageController = script->systemController_->GetStageController();
	if (stageController)
		res = stageController->GetStageInformation()->IsReplay();

	return script->CreateBooleanValue(res);
}
gstd::value StgControlScript::Func_AddArchiveFile(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FileManager* fileManager = FileManager::GetBase();
	std::wstring path = argv[0].as_string();
	size_t readOff = argc > 1 ? argv[1].as_int() : 0;
	bool res = fileManager->AddArchiveFile(path, readOff);
	return StgControlScript::CreateBooleanValue(res);
}
gstd::value StgControlScript::Func_GetArchiveFilePathList(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FileManager* fileManager = FileManager::GetBase();

	std::vector<std::wstring> pathList;
	std::wstring name = argv[0].as_string();
	bool bExtendPath = argv[1].as_boolean();

	ArchiveFile* archive = fileManager->GetArchiveFile(name);
	if (archive) {
		std::wstring archiveBaseDir = PathProperty::GetFileDirectory(archive->GetPath());

		auto& mapFileArchive = archive->GetEntryMap();
		for (auto itr = mapFileArchive.cbegin(); itr != mapFileArchive.cend(); ++itr) {
			const ArchiveFileEntry* entry = &itr->second;
			std::wstring path = bExtendPath ? entry->fullPath : entry->path;
			pathList.push_back(PathProperty::ReplaceYenToSlash(path));
		}
	}

	return StgControlScript::CreateStringArrayValue(pathList);
}
gstd::value StgControlScript::Func_GetCurrentUpdateFps(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	EFpsController* fpsController = EFpsController::GetInstance();
	return StgControlScript::CreateFloatValue(fpsController->GetCurrentWorkFps());
}
gstd::value StgControlScript::Func_GetCurrentRenderFps(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	EFpsController* fpsController = EFpsController::GetInstance();
	return StgControlScript::CreateFloatValue(fpsController->GetCurrentRenderFps());
}
gstd::value StgControlScript::Func_GetLastFrameUpdateSpeed(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ETaskManager* taskManager = ETaskManager::GetInstance();
	return StgControlScript::CreateIntValue(taskManager->SetWorkTime());
}
gstd::value StgControlScript::Func_GetLastFrameRenderSpeed(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ETaskManager* taskManager = ETaskManager::GetInstance();
	return StgControlScript::CreateIntValue(taskManager->GetRenderTime());
}
gstd::value StgControlScript::Func_GetStageTime(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	uint64_t res = 0;

	StgStageController* stageController = script->systemController_->GetStageController();
	if (stageController) {
		ref_count_ptr<StgStageInformation> infoStage = stageController->GetStageInformation();

		uint64_t time = SystemUtility::GetCpuTime2();
		uint64_t timeStart = infoStage->GetStageStartTime();
		res = (timeStart > 0) ? time - timeStart : 0;
	}

	return script->CreateIntValue(res);
}
gstd::value StgControlScript::Func_GetStageTimeF(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	DWORD res = 0;

	StgStageController* stageController = script->systemController_->GetStageController();
	if (stageController) {
		ref_count_ptr<StgStageInformation> infoStage = stageController->GetStageInformation();
		res = infoStage->GetCurrentFrame();
	}

	return script->CreateIntValue(res);
}
gstd::value StgControlScript::Func_GetPackageTime(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	DWORD res = 0;

	StgPackageController* packageController = script->systemController_->GetPackageController();
	if (packageController) {
		ref_count_ptr<StgPackageInformation> infoPackage = packageController->GetPackageInformation();

		uint64_t time = SystemUtility::GetCpuTime2();
		uint64_t timeStart = infoPackage->GetPackageStartTime();
		res = (timeStart > 0) ? time - timeStart : 0;
	}

	return script->CreateIntValue(res);
}

gstd::value StgControlScript::Func_GetStgFrameLeft(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	StgStageController* stageController = script->systemController_->GetStageController();

	LONG res = 0;
	if (stageController)
		res = stageController->GetStageInformation()->GetStgFrameRect()->left;

	return script->CreateFloatValue(res);
}
gstd::value StgControlScript::Func_GetStgFrameTop(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	StgStageController* stageController = script->systemController_->GetStageController();

	LONG res = 0;
	if (stageController)
		res = stageController->GetStageInformation()->GetStgFrameRect()->top;

	return script->CreateFloatValue(res);
}
gstd::value StgControlScript::Func_GetStgFrameWidth(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	StgStageController* stageController = script->systemController_->GetStageController();

	LONG res = 0;
	if (stageController) {
		DxRect<LONG>* rect = stageController->GetStageInformation()->GetStgFrameRect();
		res = rect->right - rect->left;
	}

	return script->CreateFloatValue(res);
}
gstd::value StgControlScript::Func_GetStgFrameHeight(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	StgStageController* stageController = script->systemController_->GetStageController();

	LONG res = 0;
	if (stageController) {
		DxRect<LONG>* rect = stageController->GetStageInformation()->GetStgFrameRect();
		res = rect->bottom - rect->top;
	}

	return script->CreateFloatValue(res);
}
gstd::value StgControlScript::Func_GetMainPackageScriptPath(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	StgPackageController* packageController = script->systemController_->GetPackageController();

	std::wstring path = L"";
	if (packageController) {
		ref_count_ptr<ScriptInformation> infoScript =
			packageController->GetPackageInformation()->GetMainScriptInformation();
		path = infoScript->pathScript_;
	}

	return script->CreateStringValue(path);
}
gstd::value StgControlScript::Func_GetScriptPathList(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	std::vector<std::wstring> listRes;
	std::wstring dir = argv[0].as_string();
	dir = PathProperty::GetFileDirectory(dir);

	int typeScript = argv[1].as_int();
	std::vector<std::wstring> listFile = File::GetFilePathList(dir);
	for (auto itr = listFile.begin(); itr != listFile.end(); ++itr) {
		std::wstring path = *itr;

		std::wstring ext = PathProperty::GetFileExtension(path);
		if (ScriptInformation::IsExcludeExtension(ext)) continue;

		path = PathProperty::GetUnique(path);
		ref_count_ptr<ScriptInformation> infoScript = ScriptInformation::CreateScriptInformation(path, true);
		if (infoScript == nullptr) continue;
		if (typeScript != TYPE_SCRIPT_ALL && typeScript != infoScript->type_) continue;

		script->mapScriptInfo_[path] = infoScript;
		listRes.push_back(path);
	}

	return script->CreateStringArrayValue(listRes);
}
gstd::value StgControlScript::Func_GetScriptInfoA1(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	std::wstring path = argv[0].as_string();
	int type = argv[1].as_int();

	ref_count_ptr<ScriptInformation> infoScript = nullptr;

	auto itr = script->mapScriptInfo_.find(path);
	if (itr != script->mapScriptInfo_.end())
		infoScript = itr->second;
	else {
		infoScript = ScriptInformation::CreateScriptInformation(path, true);
		script->mapScriptInfo_[path] = infoScript;
	}
	if (infoScript == nullptr)
		throw gstd::wexception(L"GetScriptInfoA1: " + ErrorUtility::GetFileNotFoundErrorMessage(path, true));

	value res;
	switch (type) {
	case INFO_SCRIPT_TYPE:
		res = script->CreateIntValue(infoScript->type_);
		break;
	case INFO_SCRIPT_PATH:
		res = script->CreateStringValue(infoScript->pathScript_);
		break;
	case INFO_SCRIPT_ID:
		res = script->CreateStringValue(infoScript->id_);
		break;
	case INFO_SCRIPT_TITLE:
		res = script->CreateStringValue(infoScript->title_);
		break;
	case INFO_SCRIPT_TEXT:
		res = script->CreateStringValue(infoScript->text_);
		break;
	case INFO_SCRIPT_IMAGE:
		res = script->CreateStringValue(infoScript->pathImage_);
		break;
	case INFO_SCRIPT_REPLAY_NAME:
		res = script->CreateStringValue(infoScript->replayName_);
		break;
	}

	return res;
}

gstd::value StgControlScript::Func_IsEngineFastMode(script_machine* machine, int argc, const value* argv) {
	EFpsController* controller = EFpsController::GetInstance();
	return StgControlScript::CreateBooleanValue(controller->IsFastMode());
}
gstd::value StgControlScript::Func_GetConfigWindowSizeIndex(script_machine* machine, int argc, const value* argv) {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	size_t index = config->windowSizeIndex_;
	return StgControlScript::CreateIntValue(index);
}
gstd::value StgControlScript::Func_GetConfigWindowSizeList(script_machine* machine, int argc, const value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	const auto& sizeList = config->windowSizeList_;
	std::vector<gstd::value> resListSizes;
	if (!sizeList.empty()) {
		for (const POINT& iPoint : sizeList)
			resListSizes.push_back(StgControlScript::CreateIntArrayValue((const LONG*)&iPoint, 2U));
	}
	return script->CreateValueArrayValue(resListSizes);
}
gstd::value StgControlScript::Func_GetConfigVirtualKeyMapping(script_machine* machine, int argc, const value* argv) {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	int16_t vk = (int16_t)argv->as_int();

	int16_t key_pad[2] = { EDirectInput::KEY_INVALID, EDirectInput::KEY_INVALID };

	auto itr = config->mapKey_.find(vk);
	if (itr != config->mapKey_.end()) {
		key_pad[0] = itr->second->GetKeyCode();
		key_pad[1] = itr->second->GetPadButton();
	}

	return StgControlScript::CreateIntArrayValue(key_pad, 2U);
}
gstd::value StgControlScript::Func_GetConfigWindowTitle(script_machine* machine, int argc, const value* argv) {
	EDirectGraphics* graphics = EDirectGraphics::GetInstance();
	return StgControlScript::CreateStringValue(graphics->GetDefaultWindowTitle());
}
gstd::value StgControlScript::Func_SetWindowTitle(script_machine* machine, int argc, const value* argv) {
	EDirectGraphics* graphics = EDirectGraphics::GetInstance();

	std::wstring title = argv[0].as_string();
	graphics->SetWindowTitle(title);

	return value();
}
gstd::value StgControlScript::Func_SetEnableUnfocusedProcessing(script_machine* machine, int argc, const value* argv) {
	DnhConfiguration* config = DnhConfiguration::GetInstance();

	bool enable = argv[0].as_boolean();
	config->bEnableUnfocusedProcessing_ = enable;

	return value();
}
value StgControlScript::Func_IsWindowFocused(gstd::script_machine* machine, int argc, const value* argv) {
	return StgControlScript::CreateBooleanValue(EApplication::GetInstance()->IsWindowFocused());
}

//STG共通関数：描画関連
gstd::value StgControlScript::Func_ClearInvalidRenderPriority(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	StgSystemController* systemController = script->systemController_;
	auto infoSystem = systemController->GetSystemInformation();

	infoSystem->SetInvalidRenderPriority(-1, -1);

	return value();
}
gstd::value StgControlScript::Func_SetInvalidRenderPriorityA1(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	StgSystemController* systemController = script->systemController_;
	auto infoSystem = systemController->GetSystemInformation();

	int priMin = argv[0].as_int();
	int priMax = argv[1].as_int();
	infoSystem->SetInvalidRenderPriority(priMin, priMax);

	return value();
}

gstd::value StgControlScript::Func_GetReservedRenderTargetName(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	ETextureManager* textureManager = ETextureManager::GetInstance();

	int index = argv[0].as_int();
	std::wstring name = textureManager->GetReservedRenderTargetName(index);

	return script->CreateStringValue(name);
}

shared_ptr<Texture> _RenderToTexture_LoadTexture(DxScriptResourceCache* rsrcCache, const std::wstring& name) {
	ETextureManager* textureManager = ETextureManager::GetInstance();

	shared_ptr<Texture> texture = rsrcCache->GetTexture(name);
	if (texture == nullptr) {
		texture = textureManager->GetTexture(name);
		if (texture == nullptr) {
			bool bExist = false;
			auto itrData = textureManager->IsDataExistsItr(name, &bExist);
			if (bExist) {	//Texture data exists, create a new texture object
				texture = make_shared<Texture>();
				texture->CreateFromData(itrData->second);
				textureManager->Add(name, texture);
			}
		}
	}

	return texture;
}
template<bool OVERRIDE_RT>
gstd::value StgControlScript::Func_RenderToTextureA(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	DirectGraphics* graphics = DirectGraphics::GetBase();

	std::wstring name = argv[0].as_string();
	int priMin = argv[1].as_int();
	int priMax = argv[2].as_int();
	bool bClear = argv[3].as_boolean();

	shared_ptr<Texture> texture = _RenderToTexture_LoadTexture(script->pResouceCache_, name);

	if (texture && texture->GetType() == TextureData::Type::TYPE_RENDER_TARGET) {
		graphics->SetAllowRenderTargetChange(OVERRIDE_RT);
		graphics->SetRenderTarget(texture);
		graphics->ResetDeviceState();

		graphics->BeginScene(false, bClear);
		script->systemController_->RenderScriptObject(priMin, priMax);
		graphics->EndScene(false);

		graphics->SetRenderTarget(nullptr);
		graphics->SetAllowRenderTargetChange(true);
	}

	return value();
}
template<bool OVERRIDE_RT>
gstd::value StgControlScript::Func_RenderToTextureB(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	DirectGraphics* graphics = DirectGraphics::GetBase();

	std::wstring name = argv[0].as_string();
	int id = argv[1].as_int();
	bool bClear = argv[2].as_boolean();

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Texture> texture = _RenderToTexture_LoadTexture(script->pResouceCache_, name);

		if (texture && texture->GetType() == TextureData::Type::TYPE_RENDER_TARGET) {
			graphics->SetAllowRenderTargetChange(OVERRIDE_RT);
			graphics->SetRenderTarget(texture);
			graphics->ResetDeviceState();

			graphics->BeginScene(false, bClear);
			obj->Render();
			graphics->EndScene(false);

			graphics->SetRenderTarget(nullptr);
			graphics->SetAllowRenderTargetChange(true);
		}
	}

	return value();
}

gstd::value StgControlScript::Func_SaveSnapShotA1(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	ETextureManager* textureManager = ETextureManager::GetInstance();
	StgSystemController* systemController = script->systemController_;

	std::wstring path = argv[0].as_string();

	DirectGraphics* graphics = DirectGraphics::GetBase();

	//Create the directory (if it doesn't exist)
	std::wstring dir = PathProperty::GetFileDirectory(path);
	File::CreateFileDirectory(dir);

	IDirect3DSurface9* pSurface = graphics->GetBaseSurface();
	DxRect<LONG> rect(0, 0, graphics->GetScreenWidth(), graphics->GetScreenHeight());
	HRESULT hr = D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_BMP,
		pSurface, nullptr, (RECT*)&rect);
	return script->CreateBooleanValue(SUCCEEDED(hr));
}
gstd::value StgControlScript::Func_SaveSnapShotA2(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	ETextureManager* textureManager = ETextureManager::GetInstance();
	StgSystemController* systemController = script->systemController_;

	std::wstring path = argv[0].as_string();
	DxRect<LONG> rect(argv[1].as_int(), argv[2].as_int(),
		argv[3].as_int(), argv[4].as_int());

	DirectGraphics* graphics = DirectGraphics::GetBase();

	//Create the directory (if it doesn't exist)
	std::wstring dir = PathProperty::GetFileDirectory(path);
	File::CreateFileDirectory(dir);

	IDirect3DSurface9* pSurface = graphics->GetBaseSurface();
	HRESULT hr = D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_BMP,
		pSurface, nullptr, (RECT*)&rect);
	return script->CreateBooleanValue(SUCCEEDED(hr));
}
gstd::value StgControlScript::Func_SaveSnapShotA3(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	ETextureManager* textureManager = ETextureManager::GetInstance();
	StgSystemController* systemController = script->systemController_;

	std::wstring path = argv[0].as_string();
	DxRect<LONG> rect(argv[1].as_int(), argv[2].as_int(),
		argv[3].as_int(), argv[4].as_int());
	BYTE imgFormat = argv[5].as_int();

	if (imgFormat < 0)
		imgFormat = 0;
	if (imgFormat > D3DXIFF_PPM)
		imgFormat = D3DXIFF_PPM;

	DirectGraphics* graphics = DirectGraphics::GetBase();

	//Create the directory (if it doesn't exist)
	std::wstring dir = PathProperty::GetFileDirectory(path);
	File::CreateFileDirectory(dir);

	IDirect3DSurface9* pSurface = graphics->GetBaseSurface();
	HRESULT hr = D3DXSaveSurfaceToFile(path.c_str(), (D3DXIMAGE_FILEFORMAT)imgFormat,
		pSurface, nullptr, (RECT*)&rect);
	return script->CreateBooleanValue(SUCCEEDED(hr));
}

//STG制御共通関数：自機関連
gstd::value StgControlScript::Func_GetPlayerID(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	std::wstring id = L"";

	StgStageController* stageController = script->systemController_->GetStageController();
	if (stageController)
		id = stageController->GetStageInformation()->GetPlayerScriptInformation()->id_;

	return script->CreateStringValue(id);
}
gstd::value StgControlScript::Func_GetPlayerReplayName(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	std::wstring replayName = L"";

	StgStageController* stageController = script->systemController_->GetStageController();
	if (stageController)
		replayName = stageController->GetStageInformation()->GetPlayerScriptInformation()->replayName_;

	return script->CreateStringValue(replayName);
}

//STG制御共通関数：ユーザスクリプト
gstd::value StgControlScript::Func_SetPauseScriptPath(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	auto info = script->systemController_->GetSystemInformation();

	std::wstring path = argv[0].as_string();
	info->SetPauseScriptPath(path);

	return value();
}
gstd::value StgControlScript::Func_SetEndSceneScriptPath(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	auto info = script->systemController_->GetSystemInformation();

	std::wstring path = argv[0].as_string();
	info->SetEndSceneScriptPath(path);

	return value();
}
gstd::value StgControlScript::Func_SetReplaySaveSceneScriptPath(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	auto info = script->systemController_->GetSystemInformation();

	std::wstring path = argv[0].as_string();
	info->SetReplaySaveSceneScriptPath(path);

	return value();
}

//STG制御共通関数：自機スクリプト
gstd::value StgControlScript::Func_GetLoadFreePlayerScriptList(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	StgControlScriptInformation* infoControlScript = script->systemController_->GetControlScriptInformation();

	infoControlScript->LoadFreePlayerList();
	std::vector<ref_count_ptr<ScriptInformation>>& listFreePlayer = infoControlScript->GetFreePlayerList();

	return script->CreateFloatValue(listFreePlayer.size());
}
gstd::value StgControlScript::Func_GetFreePlayerScriptCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	StgControlScriptInformation* infoControlScript = script->systemController_->GetControlScriptInformation();

	std::vector<ref_count_ptr<ScriptInformation>>& listFreePlayer = infoControlScript->GetFreePlayerList();

	return script->CreateFloatValue(listFreePlayer.size());
}
gstd::value StgControlScript::Func_GetFreePlayerScriptInfo(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;
	StgControlScriptInformation* infoControlScript = script->systemController_->GetControlScriptInformation();

	std::vector<ref_count_ptr<ScriptInformation>>& listFreePlayer = infoControlScript->GetFreePlayerList();

	int index = argv[0].as_int();
	int type = argv[1].as_int();
	if (index < 0 || index >= listFreePlayer.size())
		script->RaiseError(ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_OUTOFRANGE_INDEX));

	ref_count_ptr<ScriptInformation> infoPlayer = listFreePlayer[index];
	value res;
	switch (type) {
	case INFO_SCRIPT_PATH:
		res = script->CreateStringValue(infoPlayer->pathScript_);
		break;
	case INFO_SCRIPT_ID:
		res = script->CreateStringValue(infoPlayer->id_);
		break;
	case INFO_SCRIPT_TITLE:
		res = script->CreateStringValue(infoPlayer->title_);
		break;
	case INFO_SCRIPT_TEXT:
		res = script->CreateStringValue(infoPlayer->text_);
		break;
	case INFO_SCRIPT_IMAGE:
		res = script->CreateStringValue(infoPlayer->pathImage_);
		break;
	case INFO_SCRIPT_REPLAY_NAME:
		res = script->CreateStringValue(infoPlayer->replayName_);
		break;
	}

	return res;
}

//STG制御共通関数：リプレイ関連
gstd::value StgControlScript::Func_LoadReplayList(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	StgControlScriptInformation* infoControlScript = script->systemController_->GetControlScriptInformation();
	auto infoSystem = script->systemController_->GetSystemInformation();

	std::wstring& pathMainScript = infoSystem->GetMainScriptInformation()->pathScript_;
	infoControlScript->LoadReplayInformation(pathMainScript);

	return value();
}
gstd::value StgControlScript::Func_GetValidReplayIndices(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	StgControlScriptInformation* infoControlScript = script->systemController_->GetControlScriptInformation();
	ref_count_ptr<ReplayInformationManager> replayInfoManager = infoControlScript->GetReplayInformationManager();

	std::vector<int> listValidIndices = replayInfoManager->GetIndexList();
	return script->CreateIntArrayValue(listValidIndices);
}
gstd::value StgControlScript::Func_IsValidReplayIndex(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	StgControlScriptInformation* infoControlScript = script->systemController_->GetControlScriptInformation();
	ref_count_ptr<ReplayInformationManager> replayInfoManager = infoControlScript->GetReplayInformationManager();

	int index = argv[0].as_int();
	return script->CreateBooleanValue(replayInfoManager->GetInformation(index) != nullptr);
}
gstd::value StgControlScript::Func_GetReplayInfo(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	StgControlScriptInformation* infoControlScript = script->systemController_->GetControlScriptInformation();
	auto infoSystem = script->systemController_->GetSystemInformation();

	ref_count_ptr<ReplayInformationManager> replayInfoManager = infoControlScript->GetReplayInformationManager();

	int index = argv[0].as_int();
	int type = argv[1].as_int();

	ref_count_ptr<ReplayInformation> replayInfo;
	if (index == ReplayInformation::INDEX_ACTIVE) replayInfo = infoSystem->GetActiveReplayInformation();
	else replayInfo = replayInfoManager->GetInformation(index);

	if (replayInfo == nullptr)
		script->RaiseError(ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_OUTOFRANGE_INDEX));

	value res;
	switch (type) {
	case REPLAY_FILE_PATH:
		res = script->CreateStringValue(replayInfo->GetPath());
		break;
	case REPLAY_DATE_TIME:
		res = script->CreateStringValue(replayInfo->GetDateAsString());
		break;
	case REPLAY_USER_NAME:
		res = script->CreateStringValue(replayInfo->GetUserName());
		break;
	case REPLAY_TOTAL_SCORE:
		res = script->CreateIntValue(replayInfo->GetTotalScore());
		break;
	case REPLAY_FPS_AVERAGE:
		res = script->CreateFloatValue(replayInfo->GetAverageFps());
		break;
	case REPLAY_PLAYER_NAME:
		res = script->CreateStringValue(replayInfo->GetPlayerScriptReplayName());
		break;
	case REPLAY_STAGE_INDEX_LIST:
	{
		std::vector<int> listStageI = replayInfo->GetStageIndexList();
		res = script->CreateIntArrayValue(listStageI);
		break;
	}
	case REPLAY_STAGE_START_SCORE_LIST:
	{
		std::vector<int64_t> listScoreD;
		for (int iStage : replayInfo->GetStageIndexList()) {
			ref_count_ptr<ReplayInformation::StageData> data = replayInfo->GetStageData(iStage);
			listScoreD.push_back(data->GetStartScore());
		}
		res = script->CreateIntArrayValue(listScoreD);
		break;
	}
	case REPLAY_STAGE_LAST_SCORE_LIST:
	{
		std::vector<int64_t> listScoreD;
		for (int iStage : replayInfo->GetStageIndexList()) {
			ref_count_ptr<ReplayInformation::StageData> data = replayInfo->GetStageData(iStage);
			listScoreD.push_back(data->GetLastScore());
		}
		res = script->CreateIntArrayValue(listScoreD);
		break;
	}
	case REPLAY_COMMENT:
		res = script->CreateStringValue(replayInfo->GetComment());
		break;
	}

	return res;
}
gstd::value StgControlScript::Func_SetReplayInfo(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	StgControlScriptInformation* infoControlScript = script->systemController_->GetControlScriptInformation();
	auto infoSystem = script->systemController_->GetSystemInformation();

	ref_count_ptr<ReplayInformationManager> replayInfoManager = infoControlScript->GetReplayInformationManager();

	ref_count_ptr<ReplayInformation> replayInfo = infoSystem->GetActiveReplayInformation();
	if (replayInfo == nullptr)
		script->RaiseError("Cannot find a target replay data.");

	int type = argv[0].as_int();

	switch (type) {
	case REPLAY_COMMENT:
		replayInfo->SetComment(argv[1].as_string());
		break;
	}

	return value();
}
gstd::value StgControlScript::Func_GetReplayUserData(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	StgControlScriptInformation* infoControlScript = script->systemController_->GetControlScriptInformation();
	auto infoSystem = script->systemController_->GetSystemInformation();

	ref_count_ptr<ReplayInformationManager> replayInfoManager = infoControlScript->GetReplayInformationManager();

	int index = argv[0].as_int();
	std::string key = StringUtility::ConvertWideToMulti(argv[1].as_string());

	ref_count_ptr<ReplayInformation> replayInfo;
	if (index == ReplayInformation::INDEX_ACTIVE) replayInfo = infoSystem->GetActiveReplayInformation();
	else replayInfo = replayInfoManager->GetInformation(index);

	if (replayInfo == nullptr)
		script->RaiseError(ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_OUTOFRANGE_INDEX));

	return replayInfo->GetUserData(key);
}
gstd::value StgControlScript::Func_SetReplayUserData(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	StgControlScriptInformation* infoControlScript = script->systemController_->GetControlScriptInformation();
	auto infoSystem = script->systemController_->GetSystemInformation();

	ref_count_ptr<ReplayInformationManager> replayInfoManager = infoControlScript->GetReplayInformationManager();

	ref_count_ptr<ReplayInformation> replayInfo = infoSystem->GetActiveReplayInformation();
	if (replayInfo == nullptr)
		script->RaiseError("The replay data is not found.");

	std::string key = StringUtility::ConvertWideToMulti(argv[0].as_string());
	gstd::value val = argv[1];
	replayInfo->SetUserData(key, val);

	return value();
}
gstd::value StgControlScript::Func_IsReplayUserDataExists(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	StgControlScriptInformation* infoControlScript = script->systemController_->GetControlScriptInformation();
	auto infoSystem = script->systemController_->GetSystemInformation();

	ref_count_ptr<ReplayInformationManager> replayInfoManager = infoControlScript->GetReplayInformationManager();

	int index = argv[0].as_int();
	std::string key = StringUtility::ConvertWideToMulti(argv[1].as_string());

	ref_count_ptr<ReplayInformation> replayInfo;
	if (index == ReplayInformation::INDEX_ACTIVE) replayInfo = infoSystem->GetActiveReplayInformation();
	else replayInfo = replayInfoManager->GetInformation(index);

	if (replayInfo == nullptr)
		script->RaiseError(ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_OUTOFRANGE_INDEX));

	return script->CreateBooleanValue(replayInfo->IsUserDataExists(key));
}

gstd::value StgControlScript::Func_SaveReplay(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	StgControlScript* script = (StgControlScript*)machine->data;

	auto infoSystem = script->systemController_->GetSystemInformation();
	auto infoMain = script->systemController_->GetSystemInformation()->GetMainScriptInformation();

	ref_count_ptr<ReplayInformation> replayInfoActive = infoSystem->GetActiveReplayInformation();
	if (replayInfoActive == nullptr)
		script->RaiseError("The replay data is not found.");

	int index = argv[0].as_int();
	if (index <= 0)
		script->RaiseError("Invalid replay index.");

	std::wstring userName = argv[1].as_string();
	replayInfoActive->SetUserName(userName);

	bool res = replayInfoActive->SaveToFile(infoMain->pathScript_, index);
	return script->CreateBooleanValue(res);
}


//*******************************************************************
//ScriptInfoPanel
//*******************************************************************
ScriptInfoPanel::ScriptInfoPanel() {
	selectedManagerAddr_ = 0;
	selectedScriptAddr_ = 0;
}

void ScriptInfoPanel::Initialize(const std::string& name) {
	ILoggerPanel::Initialize(name);
}

void ScriptInfoPanel::Update() {
	listCachedScript_.clear();
	listManager_.clear();

	ETaskManager* taskManager = ETaskManager::GetInstance();
	if (taskManager == nullptr) {
		return;
	}

	{
		Lock lock(Logger::GetTop()->GetLock());

		std::list<shared_ptr<TaskBase>>& listTask = taskManager->GetTaskList();

		for (auto& task : listTask) {
			if (auto& systemController = dptr_cast(StgSystemController, task)) {
				{
					auto& pCacheMap = systemController->GetScriptEngineCache()->GetMap();

					for (auto& [path, pCacheData] : pCacheMap) {
						std::wstring pathShort = PathProperty::ReduceModuleDirectory(
							pCacheData->GetPath());

						auto displayData = CacheDisplay {
							pCacheData.get(),
							STR_MULTI(PathProperty::GetFileName(pathShort)),
							STR_MULTI(pathShort),
						};
						listCachedScript_.push_back(MOVE(displayData));
					}
				}

				{
					std::set<shared_ptr<ScriptManager>> scriptManagersAll;

					auto scriptManagers = systemController->GetScriptManagers();
					for (auto& wManager : scriptManagers) {
						if (auto manager = wManager.lock()) {
							scriptManagersAll.insert(manager);

							for (auto& wManagerRelative : manager->GetRelativeManagerList()) {
								if (!wManagerRelative.expired())
									scriptManagersAll.insert(wManagerRelative.lock());
							}
						}
					}

					for (auto& manager : scriptManagersAll) {
						auto managerData = ManagerDisplay(manager);
						listManager_.push_back(MOVE(managerData));
					}
				}

				break;
			}
		}
	}
}
void ScriptInfoPanel::ProcessGui() {
	Logger* parent = Logger::GetTop();

	auto font15 = parent->GetFont("Arial15");

	float ht = ImGui::GetContentRegionAvail().y - 32;

	if (ImGui::BeginChild("pscript_child_table", ImVec2(0, ht), false, ImGuiWindowFlags_HorizontalScrollbar)) {

#define _SETCOL(_i, _s) ImGui::TableSetColumnIndex(_i); ImGui::TextUnformatted((_s).c_str());

		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton;
		if (ImGui::BeginTabBar("pscript_tabs", tab_bar_flags)) {
			if (ImGui::BeginTabItem("Scripts")) {
				ImGui::Dummy(ImVec2(0, 2));

				if (ImGui::Button("Terminate All Scripts", ImVec2(160, 28))) {
					_TerminateScriptAll();
				}
				ImGui::SameLine();

				weak_ptr<ManagedScript> selectedScript = {};
				ManagerDisplay* selectedManager = nullptr;

				{
					{
						auto itrFind = std::find_if(listManager_.begin(), listManager_.end(),
							[&](const ManagerDisplay& x) { return x.address == selectedManagerAddr_; });
						if (itrFind != listManager_.end()) {
							selectedManager = &*itrFind;

							if (selectedScriptAddr_ != 0) {
								auto& scripts = selectedManager->listScripts;

								auto itrFindScr = std::find_if(scripts.cbegin(), scripts.cend(),
									[&](const ScriptDisplay& x) { return x.address == selectedScriptAddr_; });
								if (itrFindScr != scripts.cend()) {
									selectedScript = itrFindScr->script;
								}
							}
						}
					}

					directx::imgui::ImGuiExt::Disabled(selectedScript.expired(), [&]() {
						if (ImGui::Button("Terminate Selected Script", ImVec2(200, 28))) {
							selectedScriptAddr_ = 0;
							_TerminateScript(selectedScript);
						}
					});
				}

				ImGui::Dummy(ImVec2(0, 2));

				{
					ImGuiTableFlags flags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable
						| ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX
						| ImGuiTableFlags_RowBg;

					float ht2 = ImGui::GetContentRegionAvail().y * 0.3;

					if (ImGui::BeginTable("pscript_table_manager", 4, flags, ImVec2(0, ht2))) {
						ImGui::TableSetupScrollFreeze(0, 1);

						constexpr auto colFlags = ImGuiTableColumnFlags_WidthStretch;

						ImGui::TableSetupColumn("Address", colFlags);
						ImGui::TableSetupColumn("Thread ID", colFlags);
						ImGui::TableSetupColumn("Scripts Running", colFlags);
						ImGui::TableSetupColumn("Scripts Loaded", colFlags);

						ImGui::TableHeadersRow();

						{
							ImGui::PushFont(font15);

							ImGuiListClipper clipper;
							clipper.Begin(listManager_.size());
							while (clipper.Step()) {
								for (size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
									const auto& item = listManager_[i];
									bool selected = (selectedManagerAddr_ == item.address);

									ImGui::PushID(item.address);
									ImGui::TableNextRow();

									{
										auto strAddress = StringUtility::FromAddress(item.address);

										ImGui::TableSetColumnIndex(0);
										if (ImGui::Selectable(strAddress.c_str(), selected,
											ImGuiSelectableFlags_SpanAllColumns))
										{
											selectedManagerAddr_ = item.address;
										}
									}

									_SETCOL(1, std::to_string(item.idThread));
									_SETCOL(2, std::to_string(item.nScriptRun));
									_SETCOL(3, std::to_string(item.nScriptLoad));

									ImGui::PopID();
								}
							}

							ImGui::PopFont();
						}

						ImGui::EndTable();
					}
				}

				ImGui::Separator();

				{
					ImGuiTableFlags flags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable
						| ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX
						| ImGuiTableFlags_RowBg
						| ImGuiTableFlags_Sortable;

					float ht2 = ImGui::GetContentRegionAvail().y;

					if (ImGui::BeginTable("pscript_table_scripts", 7, flags, ImVec2(0, ht2))) {
						ImGui::TableSetupScrollFreeze(0, 1);

						constexpr auto sortDef = ImGuiTableColumnFlags_DefaultSort,
							sortNone = ImGuiTableColumnFlags_NoSort;
						constexpr auto colFlags = ImGuiTableColumnFlags_WidthStretch;

						ImGui::TableSetupColumn("Address", sortDef, 0, ScriptDisplay::Column::Address);
						ImGui::TableSetupColumn("ID", sortDef, 0, ScriptDisplay::Column::Id);
						ImGui::TableSetupColumn("Type", sortDef, 0, ScriptDisplay::Column::Type);
						ImGui::TableSetupColumn("Name", colFlags | sortDef, 100, ScriptDisplay::Column::Path);
						ImGui::TableSetupColumn("Status", sortDef, 0, ScriptDisplay::Column::Status);
						ImGui::TableSetupColumn("Task Count", sortDef, 0, ScriptDisplay::Column::Task);
						ImGui::TableSetupColumn("Time Spent (μs)", colFlags | sortDef, 60, ScriptDisplay::Column::Time);

						ImGui::TableHeadersRow();

						if (selectedManager != nullptr) {
							selectedManager->LoadScripts();
							auto& scripts = selectedManager->listScripts;

							if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
								if (specs->SpecsDirty) {
									ScriptDisplay::imguiSortSpecs = specs;

									if (scripts.size() > 1) {
										std::sort(scripts.begin(), scripts.end(),
											ScriptDisplay::Compare);
									}

									specs->SpecsDirty = false;
								}
							}

							ImGui::PushFont(font15);

							ImGuiListClipper clipper;
							clipper.Begin(scripts.size());
							while (clipper.Step()) {
								for (size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
									auto& item = scripts[i];
									bool selected = (selectedScriptAddr_ == item.address);

									ImGui::PushID(item.address);
									ImGui::TableNextRow();

									{
										auto strAddress = StringUtility::FromAddress(item.address);

										ImGui::TableSetColumnIndex(0);
										if (ImGui::Selectable(strAddress.c_str(), selected,
											ImGuiSelectableFlags_SpanAllColumns))
										{
											selectedScriptAddr_ = item.address;
										}
									}

									_SETCOL(1, std::to_string(item.id));
									_SETCOL(2, item.type);

									_SETCOL(3, item.name);
									if (ImGui::IsItemHovered())
										ImGui::SetTooltip(item.name.c_str());

									ImGui::TableSetColumnIndex(4);
									ImGui::Text(GetScriptStatusStr(item.status));

									_SETCOL(5, std::to_string(item.tasks));
									_SETCOL(6, std::to_string(item.time));

									ImGui::PopID();
								}
							}

							ImGui::PopFont();
						}

						ImGui::EndTable();
					}
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Cached")) {
				ImGui::Dummy(ImVec2(0, 2));

				{
					ImGuiTableFlags flags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable
						| ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX
						| ImGuiTableFlags_RowBg;

					if (ImGui::BeginTable("pscript_table_cached", 3, flags)) {
						ImGui::TableSetupScrollFreeze(0, 1);

						constexpr auto colFlags = ImGuiTableColumnFlags_WidthStretch;

						ImGui::TableSetupColumn("Address", colFlags);
						ImGui::TableSetupColumn("Name", colFlags);
						ImGui::TableSetupColumn("Full Path", colFlags);

						ImGui::TableHeadersRow();

						{
							ImGui::PushFont(font15);

							ImGuiListClipper clipper;
							clipper.Begin(listCachedScript_.size());
							while (clipper.Step()) {
								for (size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
									const auto& item = listCachedScript_[i];

									ImGui::TableNextRow();

									_SETCOL(0, StringUtility::FromAddress((uintptr_t)item.script));
									_SETCOL(1, item.name);

									_SETCOL(2, item.path);
									if (ImGui::IsItemHovered())
										ImGui::SetTooltip(item.path.c_str());
								}
							}

							ImGui::PopFont();
						}

						ImGui::EndTable();
					}
				}

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

#undef _SETCOL
	}
	ImGui::EndChild();

	ImGui::Dummy(ImVec2(0, 4));
	ImGui::Indent(6);
}

const char* ScriptInfoPanel::GetScriptTypeName(ManagedScript* script) {
	if (script == nullptr) 
		return "Null";

	if (dynamic_cast<StgStageScript*>(script)) {
		switch (script->GetScriptType()) {
		case StgStageScript::TYPE_SYSTEM:
			return "System";
		case StgStageScript::TYPE_STAGE:
			return "Stage";
		case StgStageScript::TYPE_PLAYER:
			return "Player";
		case StgStageScript::TYPE_ITEM:
			return "Item";
		case StgStageScript::TYPE_SHOT:
			return "Shot";
		}
	}
	else if (dynamic_cast<StgPackageScript*>(script)) {
		switch (script->GetScriptType()) {
		case StgPackageScript::TYPE_PACKAGE_MAIN:
			return "Package";
		}
	}
	else if (dynamic_cast<StgUserExtendSceneScript*>(script)) {
		switch (script->GetScriptType()) {
		case StgUserExtendSceneScript::TYPE_PAUSE_SCENE:
			return "PauseScene";
		case StgUserExtendSceneScript::TYPE_END_SCENE:
			return "EndScene";
		case StgUserExtendSceneScript::TYPE_REPLAY_SCENE:
			return "ReplaySaveScene";
		}
	}
	return "Unknown";
}
const char* ScriptInfoPanel::GetScriptStatusStr(ScriptDisplay::ScriptStatus status) {
	switch (status) {
	case ScriptDisplay::ScriptStatus::Running:
		return "Running";
	case ScriptDisplay::ScriptStatus::Loaded:
		return "Loaded";
	case ScriptDisplay::ScriptStatus::Paused:
		return "Paused";
	case ScriptDisplay::ScriptStatus::Closing:
		return "Closing";
	}
	return "";
}

ScriptInfoPanel::ManagerDisplay::ManagerDisplay(shared_ptr<ScriptManager> manager) :
	manager(manager), address((uintptr_t)manager.get())
{
	idThread = manager->GetMainThreadID();
	nScriptRun = manager->GetRunningScriptList().size();
	nScriptLoad = manager->GetMapScriptLoad().size();
}

void ScriptInfoPanel::ManagerDisplay::LoadScripts() {
	if (listScripts.size() > 0)
		return;

	{
		Lock lock(Logger::GetTop()->GetLock());

		LOCK_WEAK(pManager, manager) {
			for (auto& [sId, pScript] : pManager->GetMapScriptLoad()) {
				listScripts.push_back(ScriptDisplay(pScript, ScriptDisplay::ScriptStatus::Loaded));
			}

			for (auto& pScript : pManager->GetRunningScriptList()) {
				listScripts.push_back(ScriptDisplay(pScript, 
					pScript->IsPaused() ? 
					ScriptDisplay::ScriptStatus::Paused : 
					ScriptDisplay::ScriptStatus::Running));
			}
		}

		// Sort new data as well
		if (ScriptDisplay::imguiSortSpecs) {
			if (listScripts.size() > 1) {
				std::sort(listScripts.begin(), listScripts.end(), ScriptDisplay::Compare);
			}
		}
	}
}

ScriptInfoPanel::ScriptDisplay::ScriptDisplay(shared_ptr<ManagedScript> script, ScriptStatus status) :
	script(script), address((uintptr_t)script.get()),
	status(status)
{
	id = script->GetScriptID();
	name = STR_MULTI(PathProperty::GetFileName(script->GetPath()));
	tasks = script->GetThreadCount();
	time = script->GetScriptRunTime();

	type = GetScriptTypeName(script.get());
}

const ImGuiTableSortSpecs* ScriptInfoPanel::ScriptDisplay::imguiSortSpecs = nullptr;
bool ScriptInfoPanel::ScriptDisplay::Compare(const ScriptDisplay& a, const ScriptDisplay& b) {
	for (int i = 0; i < imguiSortSpecs->SpecsCount; ++i) {
		const ImGuiTableColumnSortSpecs* spec = &imguiSortSpecs->Specs[i];

		int rcmp = 0;

#define CASE_SORT(_id, _l, _r) \
		case _id: { \
			if (_l != _r) rcmp = (_l < _r) ? 1 : -1; \
			break; \
		}

		switch ((Column)spec->ColumnUserID) {
		CASE_SORT(Column::Address, a.address, b.address);
		CASE_SORT(Column::Id, a.id, b.id);
		case Column::Type:
			rcmp = a.type.compare(b.type);
			break;
		case Column::Path:
			rcmp = a.name.compare(b.name);
			break;
		CASE_SORT(Column::Status, (int)a.status, (int)b.status);
		CASE_SORT(Column::Task, a.tasks, b.tasks);
		CASE_SORT(Column::Time, a.time, b.time);
		}

#undef CASE_SORT

		if (rcmp != 0) {
			return spec->SortDirection == ImGuiSortDirection_Ascending 
				? rcmp < 0 : rcmp > 0;
		}
	}

	return a.address < b.address;
}

void ScriptInfoPanel::_TerminateScriptAll() {
	ETaskManager* taskManager = ETaskManager::GetInstance();

	std::list<shared_ptr<TaskBase>>& listTask = taskManager->GetTaskList();

	for (auto& task : listTask) {
		if (auto& systemController = dptr_cast(StgSystemController, task)) {
			systemController->TerminateScriptAll();
			break;
		}
	}
}
void ScriptInfoPanel::_TerminateScript(weak_ptr<ManagedScript> wScript) {
	if (auto script = wScript.lock()) {
		if (auto manager = script->GetScriptManager())
			manager->CloseScript(script);
	}
}


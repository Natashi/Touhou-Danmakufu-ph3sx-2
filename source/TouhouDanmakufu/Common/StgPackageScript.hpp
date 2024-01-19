#pragma once

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgControlScript.hpp"
#include "StgStageController.hpp"

//*******************************************************************
//StgPackageScriptManager
//*******************************************************************
class StgPackageScript;
class StgPackageScriptManager : public StgControlScriptManager {
protected:
	StgSystemController* systemController_;

	std::shared_ptr<DxScriptObjectManager> objectManager_;
public:
	StgPackageScriptManager(StgSystemController* controller);
	virtual ~StgPackageScriptManager();

	virtual void Work();
	virtual void Render();
	virtual shared_ptr<ManagedScript> Create(shared_ptr<ScriptManager> manager, int type) override;

	DxScriptObjectManager* GetObjectManager() { return objectManager_.get(); }
	std::shared_ptr<DxScriptObjectManager> GetObjectManagerRef() { return objectManager_; }
};

//*******************************************************************
//StgPackageScript
//*******************************************************************
class StgPackageScript : public StgControlScript {
public:
	enum {
		TYPE_PACKAGE_MAIN,

		STAGE_STATE_FINISHED,
	};
private:
	StgPackageController* packageController_;

	void _CheckNextStageExists();
public:
	StgPackageScript(StgPackageController* packageController);

	//パッケージ共通関数：パッケージ操作
	static gstd::value Func_ClosePackage(gstd::script_machine* machine, int argc, const gstd::value* argv);

	//パッケージ共通関数：ステージ操作
	static gstd::value Func_InitializeStageScene(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_FinalizeStageScene(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_StartStageScene(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SetStageIndex(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SetStageMainScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SetStagePlayerScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_SetStageReplayFile(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetStageSceneState(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_GetStageSceneResult(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_PauseStageScene(gstd::script_machine* machine, int argc, const gstd::value* argv);
	static gstd::value Func_TerminateStageScene(gstd::script_machine* machine, int argc, const gstd::value* argv);
};
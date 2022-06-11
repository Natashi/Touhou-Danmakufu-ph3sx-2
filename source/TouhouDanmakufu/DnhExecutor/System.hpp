#pragma once

#include "../../GcLib/pch.h"

#include "GcLibImpl.hpp"
#include "Common.hpp"

class SystemController;
class SceneManager;
class TransitionManager;
class SystemInformation;
//*******************************************************************
//SystemController
//*******************************************************************
class SystemController : public Singleton<SystemController> {
	friend Singleton<SystemController>;
private:
	ref_count_ptr<SceneManager> sceneManager_;
	ref_count_ptr<TransitionManager> transitionManager_;
	ref_count_ptr<SystemInformation> infoSystem_;
public:
	SystemController();
	virtual ~SystemController();

	void Reset();
	void ClearTaskWithoutSystem();

	SceneManager* GetSceneManager() { return sceneManager_.get(); }
	TransitionManager* GetTransitionManager() { return transitionManager_.get(); }
	SystemInformation* GetSystemInformation() { return infoSystem_.get(); }

	void ShowErrorDialog(const std::wstring& msg);

	void ResetWindowTitle();
};

//*******************************************************************
//SceneManager
//*******************************************************************
class SceneManager {
public:
	SceneManager();
	virtual ~SceneManager();

	void TransTitleScene();

	void TransScriptSelectScene(int type);
	void TransScriptSelectScene_All();
	void TransScriptSelectScene_Single();
	void TransScriptSelectScene_Plural();
	void TransScriptSelectScene_Stage();
	void TransScriptSelectScene_Package();
	void TransScriptSelectScene_Directory();
	void TransScriptSelectScene_Last();

	void TransStgScene(ref_count_ptr<ScriptInformation> infoMain, ref_count_ptr<ScriptInformation> infoPlayer, ref_count_ptr<ReplayInformation> infoReplay);
	void TransStgScene(ref_count_ptr<ScriptInformation> infoMain, ref_count_ptr<ReplayInformation> infoReplay);

	void TransPackageScene(ref_count_ptr<ScriptInformation> infoMain, bool bOnlyPackage = false);
};

//*******************************************************************
//TransitionManager
//*******************************************************************
class TransitionManager {
	enum {
		TASK_PRI = 8,
	};
private:
	void _CreateCurrentSceneTexture();
	void _AddTask(ref_count_ptr<TransitionEffect> effect);
public:
	TransitionManager();
	virtual ~TransitionManager();

	void DoFadeOut();
};

class SystemTransitionEffectTask : public TransitionEffectTask {
public:
	void Work();
	void Render();
};

//*******************************************************************
//SystemInformation
//*******************************************************************
class SystemInformation {
	int lastTitleSelectedIndex_;
	std::wstring dirLastScriptSearch_;
	std::wstring pathLastSelectedScript_;
	int lastSelectScriptSceneType_;

	int lastPlayerSelectIndex_;
	std::vector<ref_count_ptr<ScriptInformation>> listLastPlayerSelect_;
	std::vector<ref_count_ptr<ScriptInformation>> listFreePlayer_;

	void _SearchFreePlayerScript(const std::wstring& dir);
public:
	SystemInformation();
	virtual ~SystemInformation();

	int GetLastTitleSelectedIndex() { return lastTitleSelectedIndex_; }
	void SetLastTitleSelectedIndex(int index) { lastTitleSelectedIndex_ = index; }
	std::wstring& GetLastScriptSearchDirectory() { return dirLastScriptSearch_; }
	void SetLastScriptSearchDirectory(const std::wstring& dir) { dirLastScriptSearch_ = dir; }
	std::wstring& GetLastSelectedScriptPath() { return pathLastSelectedScript_; }
	void SetLastSelectedScriptPath(const std::wstring& path) { pathLastSelectedScript_ = path; }
	int GetLastSelectScriptSceneType() { return lastSelectScriptSceneType_; }
	void SetLastSelectScriptSceneType(int type) { lastSelectScriptSceneType_ = type; }

	int GetLastSelectedPlayerIndex() { return lastPlayerSelectIndex_; }
	std::vector<ref_count_ptr<ScriptInformation>>& GetLastPlayerSelectedList() { return listLastPlayerSelect_; }
	void SetLastSelectedPlayerIndex(int index, std::vector<ref_count_ptr<ScriptInformation> >& list) { lastPlayerSelectIndex_ = index; listLastPlayerSelect_ = list; }

	void UpdateFreePlayerScriptInformationList();
	std::vector<ref_count_ptr<ScriptInformation>>& GetFreePlayerScriptInformationList() { return listFreePlayer_; }
};

//*******************************************************************
//SystemResidentTask
//*******************************************************************
class SystemResidentTask : public TaskBase {
public:
	enum {
		TASK_PRI_RENDER_FPS = 9,
	};
private:
	DxText textFps_;
public:
	SystemResidentTask();
	~SystemResidentTask();
	void RenderFps();
};
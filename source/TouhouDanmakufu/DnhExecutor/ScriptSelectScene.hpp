#pragma once

#include "../../GcLib/pch.h"

#include "GcLibImpl.hpp"
#include "Common.hpp"

class ScriptSelectSceneMenuItem;
class ScriptSelectModel;
//*******************************************************************
//ScriptSelectScene
//*******************************************************************
class ScriptSelectScene : public TaskBase, public MenuTask {
public:
	enum {
		TASK_PRI_WORK = 5,
		TASK_PRI_RENDER = 5,
	};
	enum {
		TYPE_SINGLE,
		TYPE_PLURAL,
		TYPE_STAGE,
		TYPE_PACKAGE,
		TYPE_DIR,
		TYPE_ALL,
	};
	enum {
		COUNT_MENU_TEXT = 12,
	};
	class Sort;

private:
	ref_count_ptr<ScriptSelectModel> model_;
	shared_ptr<Sprite2D> spriteBack_;
	shared_ptr<Sprite2D> spriteImage_;
	std::vector<shared_ptr<DxTextRenderObject>> objMenuText_;
	int frameSelect_;

	virtual void _ChangePage();
public:
	ScriptSelectScene();
	~ScriptSelectScene();

	virtual void Work();
	virtual void Render();
	virtual void Clear();

	int GetType();
	void SetModel(ref_count_ptr<ScriptSelectModel> model);
	void ClearModel();
	void AddMenuItem(std::list<ref_count_ptr<ScriptSelectSceneMenuItem>>& listItem);
};

class ScriptSelectSceneMenuItem : public MenuItem {
	friend ScriptSelectScene;
public:
	enum {
		TYPE_SINGLE,
		TYPE_PLURAL,
		TYPE_STAGE,
		TYPE_PACKAGE,
		TYPE_DIR,
	};
private:
	int type_;
	std::wstring path_;
	ref_count_ptr<ScriptInformation> info_;

	ScriptSelectScene* _GetScriptSelectScene() { return (ScriptSelectScene*)menu_; }
public:
	ScriptSelectSceneMenuItem(int type, const std::wstring& path, ref_count_ptr<ScriptInformation> info);
	~ScriptSelectSceneMenuItem();

	int GetType() { return type_; }
	std::wstring& GetPath() { return path_; }
	ref_count_ptr<ScriptInformation> GetScriptInformation() { return info_; }
};

class ScriptSelectScene::Sort {
public:
	BOOL operator()(const ref_count_ptr<MenuItem>& lf, const ref_count_ptr<MenuItem>& rf) {
		ref_count_ptr<MenuItem> lsp = lf;
		ref_count_ptr<MenuItem> rsp = rf;
		ScriptSelectSceneMenuItem* lp = (ScriptSelectSceneMenuItem*)lsp.get();
		ScriptSelectSceneMenuItem* rp = (ScriptSelectSceneMenuItem*)rsp.get();

		if (lp->GetType() == ScriptSelectSceneMenuItem::TYPE_DIR &&
			rp->GetType() != ScriptSelectSceneMenuItem::TYPE_DIR) return TRUE;
		if (lp->GetType() != ScriptSelectSceneMenuItem::TYPE_DIR &&
			rp->GetType() == ScriptSelectSceneMenuItem::TYPE_DIR) return FALSE;

		std::wstring& lPath = lp->GetPath();
		std::wstring& rPath = rp->GetPath();
		BOOL res = CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
			lPath.c_str(), -1, rPath.c_str(), -1);
		return res == CSTR_LESS_THAN ? TRUE : FALSE;
	}
};

//*******************************************************************
//ScriptSelectModel
//*******************************************************************
class ScriptSelectModel {
	friend ScriptSelectScene;
protected:
	bool bCreated_;
	ScriptSelectScene* scene_;
public:
	ScriptSelectModel();
	virtual ~ScriptSelectModel();

	virtual void CreateMenuItem() = 0;
	bool IsCreated() { return bCreated_; }
};

class ScriptSelectFileModel : public ScriptSelectModel, public Thread {
public:
	enum {
		TYPE_SINGLE = ScriptSelectScene::TYPE_SINGLE,
		TYPE_PLURAL = ScriptSelectScene::TYPE_PLURAL,
		TYPE_STAGE = ScriptSelectScene::TYPE_STAGE,
		TYPE_PACKAGE = ScriptSelectScene::TYPE_PACKAGE,
		TYPE_DIR = ScriptSelectScene::TYPE_DIR,
		TYPE_ALL = ScriptSelectScene::TYPE_ALL,
	};
protected:
	int type_;
	std::wstring dir_;
	std::wstring pathWait_;
	int timeLastUpdate_;

	std::list<ref_count_ptr<ScriptSelectSceneMenuItem>> listItem_;
	
	virtual void _Run();
	virtual void _SearchScript(const std::wstring& dir);
	void _CreateMenuItem(const std::wstring& path);
	bool _IsValidScriptInformation(ref_count_ptr<ScriptInformation> info);
	int _ConvertTypeInfoToItem(int typeInfo);
public:
	ScriptSelectFileModel(int type, const std::wstring& dir);
	virtual ~ScriptSelectFileModel();

	virtual void CreateMenuItem();

	int GetType() { return type_; }
	std::wstring& GetDirectory() { return dir_; }

	std::wstring& GetWaitPath() { return pathWait_; }
	void SetWaitPath(const std::wstring& path) { pathWait_ = path; }
};

//*******************************************************************
//PlayTypeSelectScene
//*******************************************************************
class PlayTypeSelectScene : public TaskBase, public MenuTask {
public:
	enum {
		TASK_PRI_WORK = 6,
		TASK_PRI_RENDER = 6,
	};
private:
	ref_count_ptr<ScriptInformation> info_;
	ref_count_ptr<ReplayInformationManager> replayInfoManager_;
public:
	PlayTypeSelectScene(ref_count_ptr<ScriptInformation> info);

	void Work();
	void Render();
};
class PlayTypeSelectMenuItem : public TextLightUpMenuItem {
	shared_ptr<DxTextRenderObject> objText_;
	POINT pos_;
    POINT posRoot_;

	PlayTypeSelectScene* _GetTitleScene() { return (PlayTypeSelectScene*)menu_; }
public:
	PlayTypeSelectMenuItem(const std::wstring& text, int x, int y);
	virtual ~PlayTypeSelectMenuItem();

	void Work();
	void Render();
};

//*******************************************************************
//PlayerSelectScene
//*******************************************************************
class PlayerSelectScene : public TaskBase, public MenuTask {
public:
	enum {
		TASK_PRI_WORK = 7,
		TASK_PRI_RENDER = 7,
	};
private:
	shared_ptr<Sprite2D> spriteBack_;
	shared_ptr<Sprite2D> spriteImage_;
	ref_count_ptr<ScriptInformation> info_;
	std::vector<ref_count_ptr<ScriptInformation>> listPlayer_;
	int frameSelect_;

	virtual void _ChangePage() { frameSelect_ = 0; };
public:
	PlayerSelectScene(ref_count_ptr<ScriptInformation> info);

	void Work();
	void Render();
};
class PlayerSelectMenuItem : public TextLightUpMenuItem {
	ref_count_ptr<ScriptInformation> info_;

	PlayerSelectScene* _GetTitleScene() { return (PlayerSelectScene*)menu_; }
public:
	PlayerSelectMenuItem(ref_count_ptr<ScriptInformation> info);
	virtual ~PlayerSelectMenuItem();

	void Work();
	void Render();

	ref_count_ptr<ScriptInformation> GetScriptInformation() { return info_; }
};
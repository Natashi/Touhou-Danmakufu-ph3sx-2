#ifndef __TOUHOUDANMAKUFU_DNHSTG_USER_EXTEND_SCENE__
#define __TOUHOUDANMAKUFU_DNHSTG_USER_EXTEND_SCENE__

#include "../../GcLib/pch.h"
#include "StgCommon.hpp"
#include "StgControlScript.hpp"

/**********************************************************
//StgUserExtendScene
**********************************************************/
class StgUserExtendSceneScriptManager;
class StgUserExtendScene {
protected:
	StgSystemController* systemController_;
	std::shared_ptr<StgUserExtendSceneScriptManager> scriptManager_;

	void _InitializeScript(const std::wstring& path, int type);
	void _CallScriptMainLoop();
	void _CallScriptFinalize();
	void _AddRelativeManager();
public:
	StgUserExtendScene(StgSystemController* controller);
	virtual ~StgUserExtendScene();
	StgUserExtendSceneScriptManager* GetScriptManager() { return scriptManager_.get(); }
	std::shared_ptr<StgUserExtendSceneScriptManager> GetScriptManagerRef() { return scriptManager_; }

	virtual void Work();
	virtual void Render();

	virtual void Start();
	virtual void Finish();

};

/**********************************************************
//StgUserExtendSceneScriptManager
**********************************************************/
class StgUserExtendSceneScript;
class StgUserExtendSceneScriptManager : public StgControlScriptManager {
protected:
	StgSystemController* systemController_;
	shared_ptr<DxScriptObjectManager> objectManager_;

public:
	StgUserExtendSceneScriptManager(StgSystemController* controller);
	virtual ~StgUserExtendSceneScriptManager();
	virtual void Work();
	virtual void Render();
	virtual shared_ptr<ManagedScript> Create(int type);

	void CallScriptFinalizeAll();
	gstd::value GetResultValue();
};

/**********************************************************
//StgUserExtendSceneScript
**********************************************************/
class StgUserExtendSceneScript : public StgControlScript {
public:
	enum {
		TYPE_PAUSE_SCENE,
		TYPE_END_SCENE,
		TYPE_REPLAY_SCENE,
	};

protected:


public:
	StgUserExtendSceneScript(StgSystemController* controller);
	virtual ~StgUserExtendSceneScript();
};

/**********************************************************
//StgPauseScene
**********************************************************/
class StgPauseSceneScript;
class StgPauseScene : public StgUserExtendScene {
public:

private:

public:
	StgPauseScene(StgSystemController* controller);
	virtual ~StgPauseScene();

	virtual void Work();

	virtual void Start();
	virtual void Finish();
};

class StgPauseSceneScript : public StgUserExtendSceneScript {
public:
	enum {

	};

protected:


public:
	StgPauseSceneScript(StgSystemController* controller);
	virtual ~StgPauseSceneScript();
};


/**********************************************************
//StgEndScene
**********************************************************/
class StgEndScript;
class StgEndScene : public StgUserExtendScene {
public:

private:

public:
	StgEndScene(StgSystemController* controller);
	virtual ~StgEndScene();

	void Work();

	void Start();
	void Finish();
};

/**********************************************************
//StgEndSceneScript
**********************************************************/
class StgEndSceneScript : public StgUserExtendSceneScript {
public:
	enum {

	};

protected:

public:
	StgEndSceneScript(StgSystemController* controller);
	virtual ~StgEndSceneScript();
};

/**********************************************************
//StgReplaySaveScene
**********************************************************/
class StgReplaySaveScript;
class StgReplaySaveScene : public StgUserExtendScene {
public:

private:

public:
	StgReplaySaveScene(StgSystemController* controller);
	virtual ~StgReplaySaveScene();

	void Work();

	void Start();
	void Finish();
};

/**********************************************************
//StgReplaySaveScript
**********************************************************/
class StgReplaySaveScript : public StgUserExtendSceneScript {
public:
	enum {

	};

protected:

public:
	StgReplaySaveScript(StgSystemController* controller);
	virtual ~StgReplaySaveScript();

};


#endif


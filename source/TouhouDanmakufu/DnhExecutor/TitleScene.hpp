#pragma once

#include "../../GcLib/pch.h"

#include "GcLibImpl.hpp"
#include "Common.hpp"

//*******************************************************************
//TitleScene
//*******************************************************************
class TitleScene : public TaskBase, public MenuTask {
public:
	enum {
		TASK_PRI_WORK = 5,
		TASK_PRI_RENDER = 5,
	};
private:
	enum {
		ITEM_COUNT = 7,
		ITEM_ALL = 0,
		ITEM_SINGLE,
		ITEM_PLURAL,
		ITEM_STAGE,
		ITEM_PACKAGE,
		ITEM_DIRECTORY,
		ITEM_QUIT,
	};
	shared_ptr<Sprite2D> spriteBack_;
public:
	TitleScene();

	void Work();
	void Render();
};

class TitleSceneMenuItem : public TextLightUpMenuItem {
	shared_ptr<DxTextRenderObject> objText_;
	POINT pos_;
    POINT posRoot_;

	TitleScene* _GetTitleScene() { return (TitleScene*)menu_; }
public:
	TitleSceneMenuItem(std::wstring text, std::wstring description, int x, int y, int id);
	virtual ~TitleSceneMenuItem();

	void Work();
	void Render();
};
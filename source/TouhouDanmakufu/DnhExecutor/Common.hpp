#ifndef __TOUHOUDANMAKUFU_EXE_COMMON__
#define __TOUHOUDANMAKUFU_EXE_COMMON__

#include "../../GcLib/pch.h"

#include "../Common/DnhCommon.hpp"
#include "../Common/DnhReplay.hpp"

#include "GcLibImpl.hpp"

/**********************************************************
//MenuTask
**********************************************************/
class MenuItem;
class MenuTask {
protected:
	enum {
		CURSOR_NONE = -1,
		CURSOR_LEFT = 0,
		CURSOR_RIGHT,
		CURSOR_UP,
		CURSOR_DOWN,
		CURSOR_COUNT = 4,
	};
	CriticalSection cs_;
	bool bActive_;
	std::vector<ref_count_ptr<MenuItem>> item_;

	std::vector<int> cursorFrame_;
	int cursorX_;
	int cursorY_;
	int pageCurrent_;
	int pageMaxX_;
	int pageMaxY_;
	bool bPageChangeX_;
	bool bPageChangeY_;
	bool bWaitedKeyFree_;

	int _GetCursorKeyState();
	virtual void _MoveCursor();
	virtual void _ChangePage() {};

	bool _IsWaitedKeyFree() { return bWaitedKeyFree_; }

public:
	MenuTask();
	virtual ~MenuTask();
	virtual void Clear();
	virtual void Work();
	virtual void Render();
	void SetActive(bool bActive) { bActive_ = bActive; }

	virtual void AddMenuItem(ref_count_ptr<MenuItem> item);

	int GetPageCount();
	int GetSelectedItemIndex();
	ref_count_ptr<MenuItem> GetSelectedMenuItem();

	int GetCurrentPageItemCount();
	int GetCurrentPageMaxX();
	int GetCurrentPageMaxY();
};
class MenuItem {
	friend MenuTask;
protected:
	MenuTask* menu_;
public:
	MenuItem() {}
	virtual ~MenuItem() {}
	virtual void Work() {}
	virtual void Render() {}
};
class TextLightUpMenuItem : public MenuItem {
protected:
	int frameSelected_;

	void _WorkSelectedItem();
	int _GetSelectedItemAlpha();
public:
	TextLightUpMenuItem();

};

#endif
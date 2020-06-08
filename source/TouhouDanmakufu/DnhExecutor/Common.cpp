#include "source/GcLib/pch.h"

#include "Common.hpp"

/**********************************************************
//MenuTask
**********************************************************/
MenuTask::MenuTask() {
	bActive_ = true;
	cursorFrame_.resize(CURSOR_COUNT);
	cursorX_ = 0;
	cursorY_ = 0;
	pageMaxX_ = 0;
	pageMaxY_ = 0;
	pageCurrent_ = 1;
	bPageChangeX_ = false;
	bPageChangeY_ = false;
	bWaitedKeyFree_ = false;
}
MenuTask::~MenuTask() {
}
void MenuTask::Clear() {
	item_.clear();
	cursorX_ = 0;
	cursorY_ = 0;
}
int MenuTask::_GetCursorKeyState() {
	EDirectInput* input = EDirectInput::GetInstance();
	int res = CURSOR_NONE;
	int vkeys[] = { EDirectInput::KEY_LEFT, EDirectInput::KEY_RIGHT, EDirectInput::KEY_UP, EDirectInput::KEY_DOWN };
	for (int iKey = 0; iKey < CURSOR_COUNT && res == CURSOR_NONE; iKey++) {
		int state = input->GetVirtualKeyState(vkeys[iKey]);
		if (state == KEY_PUSH) {
			cursorFrame_[iKey] = 0;
			res = iKey;
		}
		else if (state == KEY_HOLD) {
			cursorFrame_[iKey]++;
			if (cursorFrame_[iKey] % 5 == 4 && cursorFrame_[iKey] > 15)
				res = iKey;
		}
	}
	return res;
}
void MenuTask::_MoveCursor() {
	int stateCursor = _GetCursorKeyState();

	if (stateCursor != CURSOR_NONE) {
		Lock lock(cs_);
		int pageLast = pageCurrent_;
		int countItem = item_.size();
		int countCurrentPageItem = GetCurrentPageItemCount();
		int countCurrentPageMaxX = GetCurrentPageMaxX();
		int countCurrentPageMaxY = GetCurrentPageMaxY();
		if (stateCursor == CURSOR_LEFT) {
			cursorX_--;
			if (cursorX_ < 0) {
				if (bPageChangeX_) {
					pageCurrent_--;
					if (pageCurrent_ <= 0)
						pageCurrent_ = GetPageCount();
				}
				cursorX_ = countCurrentPageMaxX;
			}
			cursorY_ = std::min(cursorY_, GetCurrentPageMaxY());
		}
		else if (stateCursor == CURSOR_RIGHT) {
			cursorX_++;
			if (cursorX_ > countCurrentPageMaxX) {
				if (bPageChangeX_) {
					pageCurrent_++;
					if (pageCurrent_ > GetPageCount())
						pageCurrent_ = 1;
				}
				cursorX_ = 0;
			}
			cursorY_ = std::min(cursorY_, GetCurrentPageMaxY());
		}
		else if (stateCursor == CURSOR_UP) {
			cursorY_--;
			if (cursorY_ < 0) {
				if (bPageChangeY_) {
					pageCurrent_--;
					if (pageCurrent_ <= 0)
						pageCurrent_ = GetPageCount();
				}
				cursorY_ = countCurrentPageMaxY;
			}
		}
		else if (stateCursor == CURSOR_DOWN) {
			cursorY_++;
			if (cursorY_ > countCurrentPageMaxY) {
				if (bPageChangeY_) {
					pageCurrent_++;
					if (pageCurrent_ >= GetPageCount())
						pageCurrent_ = 1;
				}
				cursorY_ = 0;
			}
		}

		if (pageLast != pageCurrent_) {
			//ページ変更
			_ChangePage();
		}
	}
}
void MenuTask::Work() {
	if (!bActive_) {
		bWaitedKeyFree_ = false;
		return;
	}

	EDirectInput* input = EDirectInput::GetInstance();
	if (input->GetVirtualKeyState(EDirectInput::KEY_OK) == KEY_FREE &&
		input->GetVirtualKeyState(EDirectInput::KEY_CANCEL) == KEY_FREE) {
		bWaitedKeyFree_ = true;
	}

	_MoveCursor();

	{
		Lock lock(cs_);
		int indexTop = ((pageMaxX_ + 1) * (pageMaxY_ + 1)) * (pageCurrent_ - 1);
		int countCurrentPageItem = GetCurrentPageItemCount();
		for (int iItem = 0; iItem < countCurrentPageItem; iItem++) {
			int index = indexTop + iItem;
			ref_count_ptr<MenuItem> item = item_[index];
			if (item == NULL)continue;
			item->Work();
		}
	}
}
void MenuTask::Render() {
	{
		Lock lock(cs_);
		int indexTop = ((pageMaxX_ + 1) * (pageMaxY_ + 1)) * (pageCurrent_ - 1);
		int countCurrentPageItem = GetCurrentPageItemCount();
		for (int iItem = 0; iItem < countCurrentPageItem; iItem++) {
			int index = indexTop + iItem;
			ref_count_ptr<MenuItem> item = item_[index];
			if (item == NULL)continue;
			item->Render();
		}
	}
}
void MenuTask::AddMenuItem(ref_count_ptr<MenuItem> item) {
	{
		Lock lock(cs_);
		item->menu_ = this;
		item_.push_back(item);
	}
}
int MenuTask::GetPageCount() {
	int res = 0;
	{
		Lock lock(cs_);
		int countOnePage = (pageMaxX_ + 1) * (pageMaxY_ + 1);
		int countItem = item_.size();
		res = std::max(1, (countItem - 1) / countOnePage + 1);
	}
	return res;
}
int MenuTask::GetSelectedItemIndex() {
	int res = -1;
	{
		Lock lock(cs_);
		int indexTop = ((pageMaxX_ + 1) * (pageMaxY_ + 1)) * (pageCurrent_ - 1);
		res = indexTop + cursorX_ + cursorY_ * (pageMaxX_ + 1);
	}
	return res;
}
ref_count_ptr<MenuItem> MenuTask::GetSelectedMenuItem() {
	ref_count_ptr<MenuItem> res = NULL;
	{
		Lock lock(cs_);
		int index = GetSelectedItemIndex();
		if (index >= 0 && index < item_.size()) {
			res = item_[index];
		}
	}
	return res;
}

int MenuTask::GetCurrentPageItemCount() {
	int countItem = item_.size();
	int countCurrentPageItem = countItem - ((pageMaxX_ + 1) * (pageMaxY_ + 1)) * (pageCurrent_ - 1);
	countCurrentPageItem = std::min(countCurrentPageItem, (pageMaxX_ + 1) * (pageMaxY_ + 1));
	return countCurrentPageItem;
}
int MenuTask::GetCurrentPageMaxX() {
	int countCurrentPageItem = GetCurrentPageItemCount();
	int countCurrentPageMaxX = std::min(pageMaxX_, std::max(0, countCurrentPageItem - 1));
	return countCurrentPageMaxX;
}
int MenuTask::GetCurrentPageMaxY() {
	int countCurrentPageItem = GetCurrentPageItemCount();
	int countCurrentPageMaxY = std::min(pageMaxY_, std::max(0, (countCurrentPageItem - 1) / (pageMaxX_ + 1)));
	return countCurrentPageMaxY;
}

//TextLightUpMenuItem
TextLightUpMenuItem::TextLightUpMenuItem() {
	frameSelected_ = 0;
}
void TextLightUpMenuItem::_WorkSelectedItem() {
	if (menu_->GetSelectedMenuItem() == this)frameSelected_++;
	else frameSelected_ = 0;
}
int TextLightUpMenuItem::_GetSelectedItemAlpha() {
	int res = 0;
	if (menu_->GetSelectedMenuItem() == this) {
		int cycle = 60;
		int alpha = frameSelected_ % cycle;
		if (alpha < cycle / 2)alpha = 255 * (float)((float)(cycle / 2 - alpha) / (float)(cycle / 2));
		else alpha = 255 * (float)((float)(alpha - cycle / 2) / (float)(cycle / 2));
		res = alpha;
	}
	return res;
}

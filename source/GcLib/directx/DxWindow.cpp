#include "source/GcLib/pch.h"

#include "DxWindow.hpp"
#include "DirectGraphics.hpp"
#include "DirectInput.hpp"

using namespace gstd;
using namespace directx;

//*******************************************************************
//DxWindowManager
//*******************************************************************
DxWindowManager::DxWindowManager() {
}
DxWindowManager::~DxWindowManager() {
	Clear();
}
void DxWindowManager::Clear() {
	listWindow_.clear();
	wndCapture_ = nullptr;
}
void DxWindowManager::_ArrangeWindow() {
	for (auto itr = listWindow_.begin(); itr != listWindow_.end();) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr)
			itr = listWindow_.erase(itr);
		else if (win->IsWindowDelete()) {
			win->Dispose();
			itr = listWindow_.erase(itr);
		}
		else ++itr;
	}
}

void DxWindowManager::AddWindow(gstd::ref_count_ptr<DxWindow> window) {
	for (auto itr = listWindow_.begin(); itr != listWindow_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (win == window) return;//多重登録はさせない		
	}
	window->manager_ = this;
	listWindow_.push_back(window);
	window->AddedManager();
}
void DxWindowManager::DeleteWindow(DxWindow* window) {
	for (auto itr = listWindow_.begin(); itr != listWindow_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (win != window) continue;
		win->DeleteWindow();
		return;
	}
}
void DxWindowManager::DeleteWindowFromID(int id) {
	for (auto itr = listWindow_.begin(); itr != listWindow_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (win->IsWindowDelete() || win->idWindow_ != id) continue;
		win->DeleteWindow();
		return;
	}
}
void DxWindowManager::Work() {
	for (auto itr = listWindow_.begin(); itr != listWindow_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (!win->IsWindowEnable() || win->IsWindowDelete()) continue;
		win->Work();
	}
	_DispatchMouseEvent();
	_ArrangeWindow();
}
void DxWindowManager::Render() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetLightingEnable(false);
	graphics->SetZBufferEnable(false);
	graphics->SetZWriteEnable(false);
	graphics->SetBlendMode(MODE_BLEND_ALPHA);
	
	for (auto itr = listWindow_.rbegin(); itr != listWindow_.rend(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (!win->IsWindowVisible() || win->IsWindowDelete()) continue;
		win->Render();
	}
}
gstd::ref_count_ptr<DxWindow> DxWindowManager::GetIntersectedWindow() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	if (graphics == nullptr) return nullptr;

	DirectInput* input = DirectInput::GetBase();
	if (input == nullptr) return nullptr;

	gstd::ref_count_ptr<DxWindow> res = nullptr;
	POINT posMouse = graphics->GetMousePosition();
	
	for (auto itr = listWindow_.begin(); itr != listWindow_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (win->IsWindowDelete() || !win->IsWindowEnable() || !win->IsWindowVisible()) continue;

		if (res = GetIntersectedWindow(posMouse, win)) break;
	}

	return res;
}
gstd::ref_count_ptr<DxWindow> DxWindowManager::GetIntersectedWindow(POINT& pos, gstd::ref_count_ptr<DxWindow> parent) {
	gstd::ref_count_ptr<DxWindow> res = nullptr;
	if (parent == nullptr) {
		parent = *listWindow_.begin();
	}
	if (parent == nullptr) return nullptr;

	if (!parent->IsWindowEnable() || !parent->IsWindowVisible() || parent->IsWindowDelete())
		return nullptr;

	for (auto itr = parent->listWindowChild_.begin(); itr != parent->listWindowChild_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (win->IsWindowDelete() || !win->IsWindowEnable() || !win->IsWindowVisible()) continue;

		if (res = GetIntersectedWindow(pos, win)) break;

		bool bIntersect = win->IsIntersected(pos);
		if (!bIntersect) continue;
		res = *itr;
		break;
	}

	if (res == nullptr) {
		res = parent->IsIntersected(pos) ? parent : nullptr;
	}

	return res;
}
void DxWindowManager::_DispatchMouseEvent() {
	DirectInput* input = DirectInput::GetBase();
	if (input == nullptr) return;

	gstd::ref_count_ptr<DxWindowEvent> event(new DxWindowEvent());
	gstd::ref_count_ptr<DxWindow> wndIntersect = GetIntersectedWindow();

	//左クリック
	int mLeftState = input->GetMouseState(DI_MOUSE_LEFT);
	int mRightState = input->GetMouseState(DI_MOUSE_RIGHT);
	if (wndCapture_ == nullptr) {
		if (wndIntersect) {
			if (mLeftState == KEY_PUSH) {
				wndCapture_ = wndIntersect;
				event->AddEventType(DxWindowEvent::TYPE_MOUSE_LEFT_PUSH);
				event->SetSourceWindow(wndIntersect);
			}
			else if (mLeftState == KEY_HOLD) {
				wndCapture_ = wndIntersect;
				event->AddEventType(DxWindowEvent::TYPE_MOUSE_LEFT_PUSH);
				event->AddEventType(DxWindowEvent::TYPE_MOUSE_LEFT_HOLD);
				event->SetSourceWindow(wndIntersect);
			}
		}
	}
	else {
		if (wndIntersect != nullptr && wndIntersect == wndCapture_) {
			if (mLeftState == KEY_PULL) {
				event->AddEventType(DxWindowEvent::TYPE_MOUSE_LEFT_RELEASE);
				event->AddEventType(DxWindowEvent::TYPE_MOUSE_LEFT_CLICK);
				event->SetSourceWindow(wndIntersect);
				wndCapture_ = nullptr;
			}
		}
		else {
			if (mLeftState == KEY_PULL || mLeftState == KEY_FREE) {
				event->AddEventType(DxWindowEvent::TYPE_MOUSE_LEFT_RELEASE);
				event->SetSourceWindow(wndCapture_);
				wndCapture_ = nullptr;
			}
			else if (mLeftState == KEY_PUSH || mLeftState == KEY_HOLD) {
				event->AddEventType(DxWindowEvent::TYPE_MOUSE_LEFT_HOLD);
				event->SetSourceWindow(wndCapture_);
			}
		}
	}

	if (event->IsEmpty()) {
		if (mLeftState == KEY_PUSH)
			event->AddEventType(DxWindowEvent::TYPE_MOUSE_LEFT_PUSH);
		else if (mLeftState == KEY_PULL)
			event->AddEventType(DxWindowEvent::TYPE_MOUSE_LEFT_RELEASE);

		if (mRightState == KEY_PUSH)
			event->AddEventType(DxWindowEvent::TYPE_MOUSE_RIGHT_PUSH);
		else if (mRightState == KEY_PULL)
			event->AddEventType(DxWindowEvent::TYPE_MOUSE_RIGHT_RELEASE);
	}

	if (!event->IsEmpty()) {
		for (auto itr = listWindow_.begin(); itr != listWindow_.end(); ++itr) {
			ref_count_ptr<DxWindow>& win = *itr;
			if (win == nullptr) continue;
			if (win->IsWindowDelete() || !win->IsWindowEnable() || !win->IsWindowVisible()) continue;
			win->DispatchedEvent(event);
		}
	}

}
void DxWindowManager::SetAllWindowEnable(bool bEnable) {
	for (auto itr = listWindow_.begin(); itr != listWindow_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (win->IsWindowDelete()) continue;
		win->SetWindowEnable(bEnable);
	}
	listLockID_.clear();
}
void DxWindowManager::SetWindowEnableWithoutArgumentWindow(bool bEnable, DxWindow* window) {
	DxWindow* parent = window;
	while (parent->windowParent_) {
		parent = parent->windowParent_;
	}
	int id = parent->GetID();

	if (bEnable) {
		bool bError = false;
		int idLock = -1;
		if (listLockID_.size() > 0) {
			idLock = *listLockID_.begin();
			if (id != idLock)
				bError = true;

			listLockID_.pop_front();
			if (listLockID_.size() > 0)
				id = *listLockID_.begin();
		}
		else {
			return;
		}

		if (bError) {
			throw gstd::wexception(StringUtility::Format(L"DxWindowロックが不正です:id[%d] idLock[%d]",
				id, idLock).c_str());
		}
	}
	else {
		for (auto itr = listLockID_.begin(); itr != listLockID_.end(); ++itr) {
			if ((*itr) != id) continue;
			return;
		}
		listLockID_.push_front(id);
	}

	for (auto itr = listWindow_.begin(); itr != listWindow_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (win->IsWindowDelete()) continue;
		if (id != -1 && win->GetID() == id) continue;
		win->SetWindowEnable(bEnable);
	}
}

//*******************************************************************
//DxWindow
//*******************************************************************
std::list<int> DxWindow::listWndId_;
DxWindow::DxWindow() {
	windowParent_ = nullptr;
	bWindowDelete_ = false;
	bWindowEnable_ = true;
	bWindowVisible_ = true;

	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);

	//空いているWindowID取得
	listWndId_.sort();
	int idFree = 0;
	std::list<int>::iterator itr;
	for (itr = listWndId_.begin(); itr != listWndId_.end(); ++itr) {
		if (*itr != idFree) break;
		idFree++;
	}
	idWindow_ = idFree;
	listWndId_.push_back(idFree);

	typeRenderFrame_ = MODE_BLEND_ALPHA;
}
DxWindow::~DxWindow() {
	//WindowID解放
	for (auto itr = listWndId_.begin(); itr != listWndId_.end(); ++itr) {
		if (*itr != idWindow_) continue;
		listWndId_.erase(itr);
		break;
	}
}
void DxWindow::DeleteWindow() {
	bWindowDelete_ = true;
	if (windowParent_ == nullptr && manager_ != nullptr) {
		manager_->DeleteWindowFromID(idWindow_);
	}

	for (auto itr = listWindowChild_.begin(); itr != listWindowChild_.end(); ++itr) {
		if ((*itr)->IsWindowDelete()) continue;
		(*itr)->DeleteWindow();
	}
}
void DxWindow::Dispose() {
	windowParent_ = nullptr;
	listWindowChild_.clear();
}
void DxWindow::AddChild(gstd::ref_count_ptr<DxWindow> window) {
	for (auto itr = listWindowChild_.begin(); itr != listWindowChild_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (win == window) return;//多重登録はさせない		
	}
	window->manager_ = manager_;
	window->windowParent_ = this;
	listWindowChild_.push_back(window);
	window->AddedManager();
}
void DxWindow::_WorkChild() {
	if (bWindowDelete_) return;
	for (auto itr = listWindowChild_.begin(); itr != listWindowChild_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (win->IsWindowDelete()) continue;
		win->Work();
	}
}
void DxWindow::_RenderChild() {
	if (!bWindowVisible_ || bWindowDelete_) return;
	for (auto itr = listWindowChild_.begin(); itr != listWindowChild_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (!win->IsWindowVisible() || win->IsWindowDelete()) continue;
		win->Render();
	}
}
void DxWindow::_DispatchEventToChild(gstd::ref_count_ptr<DxWindowEvent> event) {
	if (!bWindowEnable_ || bWindowDelete_) return;
	for (auto itr = listWindowChild_.begin(); itr != listWindowChild_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (!win->IsWindowVisible() || win->IsWindowDelete()) continue;
		win->DispatchedEvent(event);
	}
}
void DxWindow::_RenderFrame() {
	if (spriteFrame_ == nullptr) return;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetBlendMode(typeRenderFrame_);
	int alphaWindow = GetAbsoluteAlpha();
	int alphaSprite = ColorAccess::GetColorA(spriteFrame_->GetVertex(0)->diffuse_color);
	int alpha = std::min(255, alphaWindow * alphaSprite / 255);

	DxRect<int> rcDest = GetAbsoluteWindowRect();
	DxRect<double> drcDest = rcDest;

	spriteFrame_->SetDestinationRect(drcDest);
	spriteFrame_->SetAlpha(alpha);
	spriteFrame_->Render();
	spriteFrame_->SetAlpha(alphaSprite);

	graphics->SetBlendMode(MODE_BLEND_ALPHA);
}
bool DxWindow::IsIntersected(POINT pos) {
	DxRect<int> rect = GetAbsoluteWindowRect();
	return pos.x >= rect.left && pos.x <= rect.right && pos.y >= rect.top && pos.y <= rect.bottom;
}
DxRect<int> DxWindow::GetAbsoluteWindowRect() {
	DxRect<int> res = rectWindow_;
	DxWindow* parent = windowParent_;
	while (parent) {
		DxRect<int>& rect = parent->rectWindow_;
		res.left += rect.left;
		res.right += rect.left;
		res.top += rect.top;
		res.bottom += rect.top;
		parent = parent->windowParent_;
	}
	return res;
}
bool DxWindow::IsWindowExists(int id) {
	if (bWindowDelete_) return false;
	bool res = false;
	if (GetID() == id) return true;
	for (auto itr = listWindowChild_.begin(); itr != listWindowChild_.end(); ++itr) {
		ref_count_ptr<DxWindow>& win = *itr;
		if (win == nullptr) continue;
		if (win->IsWindowDelete()) continue;
		res |= win->IsWindowExists(id);
		if (res) break;
	}
	return res;
}
int DxWindow::GetAbsoluteAlpha() {
	int res = GetAlpha();
	DxWindow* parent = windowParent_;
	while (parent) {
		res = res * parent->GetAlpha() / 255;
		parent = parent->windowParent_;
	}
	return res;
}

//*******************************************************************
//DxLabel
//*******************************************************************
DxLabel::DxLabel() {
}
void DxLabel::Work() {
}
void DxLabel::Render() {
	_RenderFrame();
	if (text_) text_->Render();
	_RenderChild();
}
void DxLabel::SetText(const std::wstring& str) {
	if (text_ == nullptr) {
		DxRect<int> rect = GetAbsoluteWindowRect();
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		text_.reset(new DxText());
		text_->SetHorizontalAlignment(TextAlignment::Center);
		text_->SetFontSize(std::min(width, height));
		text_->SetPosition(rect.top, rect.bottom);
	}
	text_->SetText(str);
}
void DxLabel::SetText(ref_count_ptr<DxText> text, bool bArrange) {
	text_ = text;

	if (bArrange) {
		int sizeFont = text->GetFontSize();
		DxRect<int> rect = GetAbsoluteWindowRect();
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		text_->SetMaxWidth(width);
		text_->SetHorizontalAlignment(TextAlignment::Center);
		text_->SetPosition(rect.left, rect.top + (height - sizeFont) / 2);
	}
}


//*******************************************************************
//DxButton
//*******************************************************************
DxButton::DxButton() {
	bIntersected_ = false;
	bSelected_ = false;
}
void DxButton::Work() {
	if (manager_ == nullptr) return;
	if (IsWindowDelete()) return;
	ref_count_ptr<DxWindow> wnd = manager_->GetIntersectedWindow();

	bool bOldIntersected = bIntersected_;
	bIntersected_ = wnd != nullptr && wnd->GetID() == GetID();
	if (!bOldIntersected && bIntersected_) {
		IntersectMouseCursor();
	}
	_WorkChild();
}
void DxButton::Render() {
	_RenderFrame();
	if (text_) text_->Render();
	_RenderChild();

	if (bIntersected_)
		RenderIntersectedFrame();
	if (bSelected_)
		RenderSelectedFrame();
}
void DxButton::RenderIntersectedFrame() {
	if (!bIntersected_) return;
	if (!IsWindowEnable()) return;
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetBlendMode(MODE_BLEND_ADD_RGB);
	Sprite2D sprite;
	int alpha = 64;
	DxRect<int> rcSrc(1, 1, 2, 2);
	DxRect<double> rcDest = GetAbsoluteWindowRect();
	sprite.SetVertex(rcSrc, rcDest, D3DCOLOR_ARGB(alpha, alpha, alpha, alpha));
	sprite.Render();
	graphics->SetBlendMode(MODE_BLEND_ALPHA);
}
void DxButton::RenderSelectedFrame() {
	if (!bSelected_) return;
	if (!IsWindowEnable()) return;
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetBlendMode(MODE_BLEND_ADD_RGB);
	Sprite2D sprite;
	int alpha = 64;
	DxRect<int> rcSrc(1, 1, 2, 2);
	DxRect<double> rcDest = GetAbsoluteWindowRect();
	sprite.SetVertex(rcSrc, rcDest, D3DCOLOR_ARGB(alpha, alpha, alpha, alpha));
	sprite.Render();
	graphics->SetBlendMode(MODE_BLEND_ALPHA);
}


//*******************************************************************
//DxMessageBox
//*******************************************************************
DxMessageBox::DxMessageBox() {
	index_ = INDEX_NULL;
}
void DxMessageBox::DispatchedEvent(gstd::ref_count_ptr<DxWindowEvent> event) {
	_DispatchEventToChild(event);

}
void DxMessageBox::SetText(gstd::ref_count_ptr<DxText> text) {
	text_ = text;
}
void DxMessageBox::SetButton(std::vector<gstd::ref_count_ptr<DxButton>> listButton) {
	listButton_ = listButton;
}
void DxMessageBox::UpdateWindowRect() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	int scrnWidth = graphics->GetScreenWidth();
	int scrnHeight = graphics->GetScreenHeight();

	int margin = 16;
	DxRect<int> rcWnd = GetWindowRect();
	int wndWidth = rcWnd.right - rcWnd.left;
	text_->SetMaxWidth(wndWidth - margin * 2);
	text_->SetPosition(rcWnd.left + margin, rcWnd.top + margin);

	DxTextInfo textInfo = text_->CreateTextInfo();
	int textHeight = textInfo.GetTotalHeight();

	int iButton = 0;
	int totalButtonWidth = 0;
	int buttonHeight = 0;
	for (iButton = 0; iButton < listButton_.size(); iButton++) {
		DxRect<int> rect = listButton_[iButton]->GetWindowRect();
		totalButtonWidth += rect.right - rect.left + margin;
		buttonHeight = std::max(buttonHeight, rect.bottom - rect.top);
	}

	int leftButton = wndWidth / 2 - totalButtonWidth / 2;
	int topButton = textHeight + margin * 2;
	for (iButton = 0; iButton < listButton_.size(); iButton++) {
		DxRect<int> rcButton = listButton_[iButton]->GetWindowRect();
		int width = rcButton.right - rcButton.left;
		int height = rcButton.bottom - rcButton.top;
		DxRect<int> rect(leftButton, topButton, leftButton + width, topButton + height);
		leftButton += width + margin;
		listButton_[iButton]->SetWindowRect(rect);
	}

	DxRect<int> rect(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.top + textHeight + buttonHeight + margin * 3);
	SetWindowRect(rect);
}

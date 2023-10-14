#include "source/GcLib/pch.h"

#include "Window.hpp"

using namespace gstd;

//****************************************************************************
//WindowBase
//****************************************************************************
const wchar_t* PROP_THIS = L"__THIS__";
std::list<int> WindowBase::listWndId_;
CriticalSection WindowBase::lock_;

WindowBase::WindowBase() {
	hWnd_ = nullptr;
	oldWndProc_ = nullptr;

	//空いているWindowID取得
	{
		Lock lock(lock_);

		listWndId_.sort();

		int idFree = 0;
		for (auto itr = listWndId_.begin(); itr != listWndId_.end(); ++itr) {
			if (*itr != idFree) break;
			idFree++;
		}

		idWindow_ = idFree;
		listWndId_.push_back(idFree);
	}
}

WindowBase::~WindowBase() {
	this->Detach();

	//WindowID解放
	for (auto itr = listWndId_.begin(); itr != listWndId_.end(); ++itr) {
		if (*itr != idWindow_) continue;
		listWndId_.erase(itr);
		break;
	}
}

bool WindowBase::Attach(HWND hWnd) {
	if (!hWnd) return false;
	hWnd_ = hWnd;
	//ダイアログかウィンドウかを判定
	int typeProc = ::GetWindowLong(hWnd, DWL_DLGPROC) != 0 ? DWL_DLGPROC : GWL_WNDPROC;

	//プロパティにインスタンスを登録
	::SetProp(hWnd_, PROP_THIS, (HANDLE)this);

	//既存のウィンドウをサブクラス化
	if (::GetWindowLong(hWnd_, typeProc) != (LONG)_StaticWindowProcedure)
		oldWndProc_ = (WNDPROC)::SetWindowLong(hWnd_, typeProc, (LONG)_StaticWindowProcedure);
	return true;
}

bool WindowBase::Detach() {
	if (hWnd_ == nullptr) return false;

	//サブクラス化を解除
	if (oldWndProc_) {
		int typeProc = ::GetWindowLong(hWnd_, DWL_DLGPROC) != 0 ? DWL_DLGPROC : GWL_WNDPROC;
		::SetWindowLong(hWnd_, typeProc, (DWORD)oldWndProc_);
		oldWndProc_ = nullptr;
	}
	::RemoveProp(hWnd_, PROP_THIS);
	return true;
}


LRESULT CALLBACK WindowBase::_StaticWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	WindowBase* tWnd = (WindowBase*)::GetProp(hWnd, PROP_THIS);

	//取得できなかったときの処理
	if (tWnd == nullptr) {
		if ((uMsg == WM_CREATE) || (uMsg == WM_NCCREATE))
			tWnd = (WindowBase*)((LPCREATESTRUCT)lParam)->lpCreateParams;
		else if (uMsg == WM_INITDIALOG)
			tWnd = (WindowBase*)lParam;
		if (tWnd) tWnd->Attach(hWnd);
	}

	if (tWnd) {
		LRESULT lResult = tWnd->_WindowProcedure(hWnd, uMsg, wParam, lParam);
		if (uMsg == WM_DESTROY) tWnd->Detach();
		return lResult;
	}

	//ダイアログとウィンドウで返す値を分ける
	return ::GetWindowLong(hWnd, DWL_DLGPROC) ?
		FALSE : ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
LRESULT WindowBase::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return _CallPreviousWindowProcedure(hWnd, uMsg, wParam, lParam);
}
LRESULT WindowBase::_CallPreviousWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (oldWndProc_)
		return CallWindowProc(oldWndProc_, hWnd, uMsg, wParam, lParam);

	//ダイアログとウィンドウで返す値を分ける
	return ::GetWindowLong(hWnd, DWL_DLGPROC) ?
		FALSE : ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void WindowBase::MoveWindowCenter() {
	RECT rcWindow;
	::GetWindowRect(hWnd_, &rcWindow);
	MoveWindowCenter(rcWindow);
}
void WindowBase::MoveWindowCenter(const RECT& rcWindow) {
	RECT rcMonitor = GetActiveMonitorRect(hWnd_);
	MoveWindowCenter(hWnd_, rcMonitor, rcWindow);
}
void WindowBase::MoveWindowCenter(HWND hWnd, const RECT& rcMonitor, const RECT& rcWindow) {
	LONG tWidth = rcWindow.right - rcWindow.left;
	LONG tHeight = rcWindow.bottom - rcWindow.top;
	LONG left = rcMonitor.right / 2L - tWidth / 2L;
	LONG top = std::max(0L, rcMonitor.bottom / 2L - tHeight / 2L);

	::MoveWindow(hWnd, left, top, tWidth, tHeight, TRUE);
}

HWND WindowBase::GetTopParentWindow(HWND hWnd) {
	HWND res = hWnd;
	while (true) {
		HWND hParent = GetParent(hWnd);
		if (hParent == nullptr) break;
		res = hParent;
	}
	return res;
}

RECT WindowBase::GetMonitorRect(HMONITOR hMonitor) {
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfoW(hMonitor, &monitorInfo);

	return monitorInfo.rcMonitor;
}
RECT WindowBase::GetActiveMonitorRect(HWND hWnd) {
	HMONITOR hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
	return GetMonitorRect(hMonitor);
}
RECT WindowBase::GetPrimaryMonitorRect() {
	HMONITOR hMonitor = ::MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
	return GetMonitorRect(hMonitor);
}

//****************************************************************************
//ModalDialog
//****************************************************************************
void ModalDialog::Create(HWND hParent, LPCTSTR resource) {
	hParent_ = hParent;
	hWnd_ = CreateDialog((HINSTANCE)GetWindowLong(hParent, GWL_HINSTANCE),
		resource,
		hParent, (DLGPROC)this->_StaticWindowProcedure);
	this->Attach(hWnd_);
}
void ModalDialog::_RunMessageLoop() {
	bEndDialog_ = false;
	::EnableWindow(hParent_, FALSE);

	::ShowWindow(hWnd_, SW_SHOW);

	MSG msg;
	while (!bEndDialog_) {	//メッセージループ
		::GetMessage(&msg, nullptr, 0, 0);
		if (IsDialog() && hWnd_ && IsDialogMessage(hWnd_, &msg)) continue;

		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	::EnableWindow(hParent_, TRUE);
	::SetWindowPos(hParent_, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	::BringWindowToTop(hParent_);
	::SetActiveWindow(hParent_);
}
void ModalDialog::_FinishMessageLoop() {
	::SetWindowPos(hParent_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	bEndDialog_ = true;
}

//****************************************************************************
//WButton
//****************************************************************************
void WButton::Create(HWND hWndParent) {
	WButton::Style style;

	style.SetStyle(WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
	style.SetStyleEx(0);

	Create(hWndParent, style);
}
void WButton::Create(HWND hWndParent, Style& style) {
	HINSTANCE hInst = (HINSTANCE)::GetWindowLong(hWndParent, GWL_HINSTANCE);

	hWnd_ = ::CreateWindowEx(
		style.GetStyleEx(), L"BUTTON", nullptr,
		style.GetStyle(),
		0, 0, 0, 0, hWndParent, (HMENU)GetWindowId(),
		hInst, nullptr);

	this->Attach(hWnd_);
	::SendMessageW(hWnd_, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));
}
void WButton::SetText(const std::wstring& text) {
	::SetWindowText(hWnd_, text.c_str());
}
bool WButton::IsChecked() {
	bool bCheck = ::SendMessageW(hWnd_, BM_GETCHECK, 0, 0) == BST_CHECKED;
	return bCheck;
}

//****************************************************************************
//WEditBox
//****************************************************************************
void WEditBox::Create(HWND hWndParent, WEditBox::Style& style) {
	HINSTANCE hInst = (HINSTANCE)::GetWindowLong(hWndParent, GWL_HINSTANCE);

	//	DWORD style = 0;//ES_MULTILINE|ES_READONLY|ES_AUTOHSCROLL|ES_AUTOVSCROLL|WS_HSCROLL | WS_VSCROLL;
	hWnd_ = ::CreateWindowEx(
		style.GetStyleEx(), L"EDIT", nullptr,
		style.GetStyle(),
		0, 0, 0, 0, hWndParent, (HMENU)GetWindowId(),
		hInst, nullptr);

	this->Attach(hWnd_);
	::SendMessageW(hWnd_, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));
}
void WEditBox::SetText(const std::wstring& text) {
	if (text == GetText()) return;
	::SetWindowText(hWnd_, text.c_str());
}
std::wstring WEditBox::GetText() {
	if (GetTextLength() == 0) return L"";

	std::wstring res;
	int count = GetTextLength() + 1;
	res.resize(count);
	::GetWindowText(hWnd_, &res[0], count);
	return res;
}
int WEditBox::GetTextLength() {
	return ::GetWindowTextLength(hWnd_);
}

//****************************************************************************
//WindowUtility
//****************************************************************************
void WindowUtility::SetMouseVisible(bool bVisible) {
	if (bVisible) {
		while (ShowCursor(true) < 0) {}
	}
	else {
		while (ShowCursor(false) >= 0) {}
	}
}

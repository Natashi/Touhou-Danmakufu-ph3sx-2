#pragma once

#include "../pch.h"

#include "GstdUtility.hpp"
#include "Thread.hpp"

namespace gstd {
	//****************************************************************************
	//WindowBase
	//****************************************************************************
	class WindowBase {
	public:
		class Style {
		protected:
			DWORD style_;
			DWORD styleEx_;
		public:
			Style() { style_ = 0; styleEx_ = 0; }
			virtual ~Style() {}

			DWORD GetStyle() { return style_; }
			DWORD SetStyle(DWORD style) { style_ |= style; return style_; }
			DWORD RemoveStyle(DWORD style) { style_ &= ~style; return style_; }

			DWORD GetStyleEx() { return styleEx_; }
			DWORD SetStyleEx(DWORD style) { styleEx_ |= style; return styleEx_; }
			DWORD RemoveStyleEx(DWORD style) { styleEx_ &= ~style; return styleEx_; }
		};
	private:
		static std::list<int> listWndId_;
		static CriticalSection lock_;
	protected:
		HWND hWnd_;
		WNDPROC oldWndProc_;
		int idWindow_;

		static LRESULT CALLBACK _StaticWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT _CallPreviousWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	public:
		WindowBase();
		virtual ~WindowBase();

		HWND GetWindowHandle() { return hWnd_; }
		bool IsDialog() { return ::GetWindowLong(hWnd_, DWL_DLGPROC) != 0; }

		bool Attach(HWND hWnd);
		bool Detach();

		int GetWindowId() { return idWindow_; }

		virtual void SetBounds(int x, int y, int width, int height) { ::MoveWindow(hWnd_, x, y, width, height, TRUE); }
		RECT GetClientRect() { RECT rect; ::GetClientRect(hWnd_, &rect); return rect; }
		int GetClientX() { return GetClientRect().left; }
		int GetClientY() { return GetClientRect().top; }
		int GetClientWidth() { RECT rect = GetClientRect(); return rect.right - rect.left; }
		int GetClientHeight() { RECT rect = GetClientRect(); return rect.bottom - rect.top; }

		void SetWindowEnable(bool bEnable) { ::EnableWindow(hWnd_, bEnable); }
		void SetWindowVisible(bool bVisible) { ::ShowWindow(hWnd_, bVisible ? SW_SHOW : SW_HIDE); }
		bool IsWindowVisible() { return ::IsWindowVisible(hWnd_) ? true : false; }
		void SetParentWindow(HWND hParentWnd) { if (hWnd_ != NULL)::SetParent(hWnd_, hParentWnd); }

		DWORD SetWindowStyle(DWORD style) { 
			DWORD prev = GetCurrentWindowStyle(); 
			DWORD next = prev | style; 
			::SetWindowLong(hWnd_, GWL_STYLE, next); 
			return next; 
		}
		DWORD RemoveWindowStyle(DWORD style) { 
			DWORD prev = GetCurrentWindowStyle(); 
			DWORD next = prev & ~style; 
			::SetWindowLong(hWnd_, GWL_STYLE, next); 
			return next; 
		}
		DWORD GetCurrentWindowStyle() { return GetWindowLong(hWnd_, GWL_STYLE); }

		virtual void LocateParts() {};
		void MoveWindowCenter();
		void MoveWindowCenter(const RECT& rcWindow);	//rcWindow must have already been adjusted
		static void MoveWindowCenter(HWND hWnd, const RECT& rcMonitor, const RECT& rcWindow);

		static HWND GetTopParentWindow(HWND hWnd);

		static RECT GetMonitorRect(HMONITOR hMonitor);
		static RECT GetActiveMonitorRect(HWND hWnd);
		static RECT GetPrimaryMonitorRect();
	};

	//****************************************************************************
	//ModalDialog
	//****************************************************************************
	class ModalDialog : public WindowBase {
	protected:
		HWND hParent_;
		bool bEndDialog_;

		void _RunMessageLoop();
		void _FinishMessageLoop();
	public:
		ModalDialog() { bEndDialog_ = false; }
		void Create(HWND hParent, LPCTSTR resource);
	};

	//****************************************************************************
	//WButton
	//****************************************************************************
	class WButton : public WindowBase {
	public:
		void Create(HWND hWndParent);
		void Create(HWND hWndParent, WButton::Style& style);

		void SetText(const std::wstring& text);

		bool IsChecked();
	};

	//****************************************************************************
	//WEditBox
	//****************************************************************************
	class WEditBox : public WindowBase {
	public:
		class Style : public WindowBase::Style {};
	public:
		void Create(HWND hWndParent, WEditBox::Style& style);

		void SetText(const std::wstring& text);
		std::wstring GetText();

		int GetTextLength();
		int GetMaxTextLength() { return ::SendMessage(hWnd_, EM_GETLIMITTEXT, 0, 0); }
		void SetMaxTextLength(int length) { ::SendMessage(hWnd_, EM_SETLIMITTEXT, (WPARAM)length, 0); }
	};

	//****************************************************************************
	//WindowUtility
	//****************************************************************************
	class WindowUtility {
	public:
		static void SetMouseVisible(bool bVisible);
	};
}
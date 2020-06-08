#ifndef __GSTD_WINDOW__
#define __GSTD_WINDOW__

#include "../pch.h"

#include "GstdUtility.hpp"
#include "Thread.hpp"

namespace gstd {
	/**********************************************************
	//WindowBase
	**********************************************************/
	class WindowBase {
	public:
		class Style;
	private:
		static std::list<int> listWndId_;
		static CriticalSection lock_;

	protected:
		HWND hWnd_;
		WNDPROC oldWndProc_;//ウィンドウプロシージャアドレス
		int idWindow_;

		static LRESULT CALLBACK _StaticWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);//静的プロシージャ
		LRESULT _CallPreviousWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);//オーバーライド用プロシージャ
	public:
		WindowBase();
		virtual ~WindowBase();
		HWND GetWindowHandle() { return hWnd_; }
		bool IsDialog() { return ::GetWindowLong(hWnd_, DWL_DLGPROC) != 0; }

		bool Attach(HWND hWnd);//セット
		bool Detach();//解除
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
		DWORD SetWindowStyle(DWORD style) { DWORD prev = GetCurrentWindowStyle(); DWORD next = prev | style; ::SetWindowLong(hWnd_, GWL_STYLE, next); return next; }
		DWORD RemoveWindowStyle(DWORD style) { DWORD prev = GetCurrentWindowStyle(); DWORD next = prev & ~style; ::SetWindowLong(hWnd_, GWL_STYLE, next); return next; }
		DWORD GetCurrentWindowStyle() { return GetWindowLong(hWnd_, GWL_STYLE); }

		virtual void LocateParts() {}//画面部品配置
		void MoveWindowCenter();

		static HWND GetTopParentWindow(HWND hWnd);
	};

	class WindowBase::Style {
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

	/**********************************************************
	//ModalDialog
	**********************************************************/
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

	/**********************************************************
	//WPanel
	**********************************************************/
	class WPanel : public WindowBase {
	protected:
		virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);//オーバーライド用プロシージャ
	public:
		void Create(HWND hWndParent);
	};

	/**********************************************************
	//WLabel
	**********************************************************/
	class WLabel : public WindowBase {
		int colorText_;
		int colorBack_;
		virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	public:
		WLabel();
		void Create(HWND hWndParent);
		void SetText(std::wstring text);
		void SetTextColor(int color);
		void SetBackColor(int color);

		std::wstring GetText();
		int GetTextLength();
	};

	/**********************************************************
	//WButton
	**********************************************************/
	class WButton : public WindowBase {
	public:
		class Style;
	public:
		void Create(HWND hWndParent);
		void Create(HWND hWndParent, WButton::Style& style);
		void SetText(std::wstring text);
		bool IsChecked();
	};
	class WButton::Style : public WindowBase::Style {};

	/**********************************************************
	//WGroupBox
	**********************************************************/
	class WGroupBox : public WindowBase {
	public:
		void Create(HWND hWndParent);
		void SetText(std::wstring text);
	};

	/**********************************************************
	//WEditBox
	**********************************************************/
	class WEditBox : public WindowBase {
	public:
		class Style;
	public:
		void Create(HWND hWndParent, WEditBox::Style& style);
		void SetText(std::wstring text);
		std::wstring GetText();
		int GetTextLength();
		int GetMaxTextLength() { return ::SendMessage(hWnd_, EM_GETLIMITTEXT, 0, 0); }
		void SetMaxTextLength(int length) { ::SendMessage(hWnd_, EM_SETLIMITTEXT, (WPARAM)length, 0); }
	};
	class WEditBox::Style : public WindowBase::Style {};


	/**********************************************************
	//WListBox
	**********************************************************/
	class WListBox : public WindowBase {
	public:
		class Style;
	public:
		void Create(HWND hWndParent, WListBox::Style& style);

		void Clear();
		int GetCount();
		void SetSelectedIndex(int index);
		int GetSelectedIndex();
		void AddString(std::wstring str);
		void InsertString(int index, std::wstring str);
		void DeleteString(int index);
		std::wstring GetText(int index);
	};
	class WListBox::Style : public WindowBase::Style {};

	/**********************************************************
	//WComboBox
	**********************************************************/
	class WComboBox : public WindowBase {
	public:
		class Style;
	public:
		void Create(HWND hWndParent, WComboBox::Style& style);

		void SetItemHeight(int height);
		void SetDropDownListWidth(int width);

		void Clear();
		int GetCount();
		void SetSelectedIndex(int index);
		int GetSelectedIndex();
		std::wstring GetSelectedText();
		void AddString(std::wstring str);
		void InsertString(int index, std::wstring str);
	};
	class WComboBox::Style : public WindowBase::Style {};

	/**********************************************************
	//WListView
	**********************************************************/
	class WListView : public WindowBase {
	public:
		class Style;
	public:
		void Create(HWND hWndParent, Style& style);
		void Clear();
		void AddColumn(int cx, int sub, DWORD fmt, std::wstring text);
		void AddColumn(int cx, int sub, std::wstring text) { AddColumn(cx, sub, LVCFMT_LEFT, text); }
		void SetColumnText(int cx, std::wstring text);
		void AddRow(std::wstring text);
		void SetText(int row, int column, std::wstring text);
		void DeleteRow(int row);
		int GetRowCount();
		std::wstring GetText(int row, int column);
		bool IsExistsInColumn(std::wstring value, int column);
		int GetIndexInColumn(std::wstring value, int column);
		void SetSelectedRow(int index);
		int GetSelectedRow();
		void ClearSelection();
	};
	class WListView::Style : public WindowBase::Style {
	protected:
		DWORD styleListViewEx_;
	public:
		Style() { styleListViewEx_ = 0; }
		DWORD GetListViewStyleEx() { return styleListViewEx_; }
		DWORD SetListViewStyleEx(DWORD style) { styleListViewEx_ |= style; return styleListViewEx_; }
		DWORD RemoveListViewStyleEx(DWORD style) { styleListViewEx_ &= ~style; return styleListViewEx_; }
	};

	/**********************************************************
	//WTreeView
	**********************************************************/
	class WTreeView : public WindowBase {
	public:
		class Item;
		class ItemStyle;
		class Style;

		ref_count_ptr<Item> itemRoot_;
	public:
		WTreeView();
		~WTreeView();
		void Create(HWND hWndParent, Style& style);
		void Clear() { itemRoot_ = NULL; TreeView_DeleteAllItems(hWnd_); }
		void CreateRootItem(ItemStyle& style);
		ref_count_ptr<Item> GetRootItem() { return itemRoot_; }
		ref_count_ptr<Item> GetSelectedItem();
	};
	class WTreeView::Item {
		friend WTreeView;
		HWND hTree_;
		HTREEITEM hItem_;
	public:
		Item();
		virtual ~Item();
		ref_count_ptr<Item> CreateChild(WTreeView::ItemStyle& style);
		void Delete();
		void SetText(std::wstring text);
		std::wstring GetText();
		void SetParam(LPARAM param);
		LPARAM GetParam();
		std::list<ref_count_ptr<Item> > GetChildList();
	};
	class WTreeView::ItemStyle {
		friend WTreeView;
		TVINSERTSTRUCT tvis_;
	public:
		ItemStyle() { ZeroMemory(&tvis_, sizeof(TVINSERTSTRUCT)); tvis_.hInsertAfter = TVI_LAST; }
		TVINSERTSTRUCT& GetInsertStruct() { return tvis_; }
		void SetMask(UINT mask) { tvis_.item.mask |= mask; }
	};
	class WTreeView::Style : public WindowBase::Style {};

	/**********************************************************
	//WTabControll
	**********************************************************/
	class WTabControll : public WindowBase {
		std::vector<ref_count_ptr<WPanel> > vectPanel_;
	protected:
		virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	public:
		~WTabControll();
		void Create(HWND hWndParent);
		void AddTab(std::wstring text);
		void AddTab(std::wstring text, ref_count_ptr<WPanel> panel);
		void ShowPage();
		int GetCurrentPage() { return TabCtrl_GetCurSel(hWnd_); }
		void SetCurrentPage(int page) { TabCtrl_SetCurSel(hWnd_, page); ShowPage(); }
		ref_count_ptr<WPanel> GetPanel(int page) { return vectPanel_[page]; }
		int GetPageCount() { return vectPanel_.size(); }
		virtual void LocateParts();
	};

	/**********************************************************
	//WStatusBar
	**********************************************************/
	class WStatusBar : public WindowBase {
	public:
		void Create(HWND hWndParent);
		void SetPartsSize(std::vector<int> parts);
		void SetText(int pos, std::wstring text);
		void SetBounds(WPARAM wParam, LPARAM lParam);
	};

	/**********************************************************
	//WSplitter
	**********************************************************/
	class WSplitter : public WindowBase {
	public:
		enum SplitType {
			TYPE_VERTICAL,
			TYPE_HORIZONTAL,
		};
	protected:
		SplitType type_;
		bool bCapture_;
		float ratioX_;
		float ratioY_;
		virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	public:
		WSplitter();
		~WSplitter();
		void Create(HWND hWndParent, SplitType type);
		void SetRatioX(float ratio) { ratioX_ = ratio; }
		float GetRatioX() { return ratioX_; }
		void SetRatioY(float ratio) { ratioY_ = ratio; }
		float GetRatioY() { return ratioY_; }
	};

	/**********************************************************
	//WindowUtility
	**********************************************************/
	class WindowUtility {
	public:
		static void SetMouseVisible(bool bVisible);
	};
}

#endif

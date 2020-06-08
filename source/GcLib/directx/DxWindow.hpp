#ifndef __DIRECTX_DXWINDOW__
#define __DIRECTX_DXWINDOW__

#include "../pch.h"

#include "DxUtility.hpp"
#include "RenderObject.hpp"
#include "DxText.hpp"

namespace directx {
	class DxWindowManager;
	class DxWindow;
	class DxWindowEvent;

	/**********************************************************
	//DxWindowEvent
	**********************************************************/
	class DxWindowEvent {
		friend DxWindowManager;
	public:
		enum {
			TYPE_MOUSE_LEFT_PUSH = 1 << 0,
			TYPE_MOUSE_LEFT_RELEASE = 1 << 1,
			TYPE_MOUSE_LEFT_CLICK = 1 << 2,
			TYPE_MOUSE_LEFT_HOLD = 1 << 3,

			TYPE_MOUSE_RIGHT_PUSH = 1 << 4,
			TYPE_MOUSE_RIGHT_RELEASE = 1 << 5,
			TYPE_MOUSE_RIGHT_CLICK = 1 << 6,
			TYPE_MOUSE_RIGHT_HOLD = 1 << 7,
		};
	protected:
		gstd::ref_count_ptr<DxWindow> windowSource_;//ウィンドウ
		int type_;
	public:
		DxWindowEvent() { type_ = 0; };
		virtual ~DxWindowEvent() {};
		void SetSourceWindow(gstd::ref_count_ptr<DxWindow> source) { windowSource_ = source; }
		gstd::ref_count_ptr<DxWindow> GetSourceWindow() { return windowSource_; }
		void AddEventType(int type) { type_ |= type; }
		bool HasEventType(int type) { return (type_ & type) != 0; }
		bool IsEmpty() { return type_ == 0; }
	};

	/**********************************************************
	//DxWindowManager
	**********************************************************/
	class DxWindowManager : public gstd::TaskBase {
	protected:
		std::list<gstd::ref_count_ptr<DxWindow> > listWindow_;//最前面がアクティブ
		gstd::ref_count_ptr<DxWindow> wndCapture_;
		std::list<int> listLockID_;

		void _ArrangeWindow();//必要のなくなった領域削除
		virtual void _DispatchMouseEvent();
	public:
		DxWindowManager();
		virtual ~DxWindowManager();
		void Clear();

		void AddWindow(gstd::ref_count_ptr<DxWindow> window);
		void DeleteWindow(DxWindow* window);
		void DeleteWindowFromID(int id);

		virtual void Work();
		virtual void Render();

		void SetAllWindowEnable(bool bEnable);
		void SetWindowEnableWithoutArgumentWindow(bool bEnable, DxWindow* window);

		gstd::ref_count_ptr<DxWindow> GetIntersectedWindow();
		gstd::ref_count_ptr<DxWindow> GetIntersectedWindow(POINT& pos, gstd::ref_count_ptr<DxWindow> parent = NULL);
	};

	/**********************************************************
	//DxWindow
	**********************************************************/
	class DxWindow {
		friend DxWindowManager;
	private:
		static std::list<int> listWndId_;
	protected:
		DxWindowManager* manager_;
		int idWindow_;
		bool bWindowDelete_;//削除フラグ
		bool bWindowEnable_;
		bool bWindowVisible_;
		RECT rectWindow_;//ウィンドウ相対座標
		DxWindow* windowParent_;//親ウィンドウ
		std::list<gstd::ref_count_ptr<DxWindow> > listWindowChild_;//子ウィンドウ

		D3DCOLOR color_;
		gstd::ref_count_ptr<Sprite2D> spriteFrame_;
		int typeRenderFrame_;

		void _WorkChild();
		void _RenderChild();
		void _DispatchEventToChild(gstd::ref_count_ptr<DxWindowEvent> event);
		virtual void _RenderFrame();
	public:
		DxWindow();
		virtual ~DxWindow();
		virtual void DeleteWindow();//削除フラグを立てます
		bool IsWindowDelete() { return bWindowDelete_; }
		void Dispose();//各参照などを解放します
		virtual void AddedManager() {}
		void AddChild(gstd::ref_count_ptr<DxWindow> window);
		virtual void Work() { _WorkChild(); }
		virtual void Render() { _RenderFrame(); _RenderChild(); }
		virtual void DispatchedEvent(gstd::ref_count_ptr<DxWindowEvent> event) { _DispatchEventToChild(event); }

		virtual void IntersectMouseCursor() {}

		int GetID() { return idWindow_; }
		virtual bool IsIntersected(POINT pos);
		void SetWindowRect(RECT rect) { rectWindow_ = rect; }
		RECT GetWindowRect() { return rectWindow_; }
		RECT GetAbsoluteWindowRect();
		virtual void SetWindowEnable(bool bEnable) { bWindowEnable_ = bEnable; }
		virtual bool IsWindowEnable() { return bWindowEnable_; }
		virtual void SetWindowVisible(bool bVisible) { bWindowVisible_ = bVisible; }
		virtual bool IsWindowVisible() { return bWindowVisible_; }
		virtual bool IsWindowExists(int id);

		void SetAlpha(int alpha) { ColorAccess::SetColorA(color_, alpha); }
		int GetAlpha() { return ColorAccess::GetColorA(color_); }
		int GetAbsoluteAlpha();

		void SetFrameSprite(gstd::ref_count_ptr<Sprite2D> sprite) { spriteFrame_ = sprite; }
	};

	/**********************************************************
	//DxLabel
	**********************************************************/
	class DxLabel : public DxWindow {
	protected:
		gstd::ref_count_ptr<DxText> text_;

	public:
		DxLabel();
		virtual void Work();
		virtual void Render();
		void SetText(std::wstring str);
		void SetText(gstd::ref_count_ptr<DxText> text, bool bArrange = false);
	};

	/**********************************************************
	//DxButton
	**********************************************************/
	class DxButton : public DxLabel {
	protected:
		bool bIntersected_;
		bool bSelected_;
	public:
		DxButton();
		virtual void Work();
		virtual void Render();
		virtual void RenderIntersectedFrame();
		virtual void RenderSelectedFrame();

		bool IsIntersected() { return bIntersected_; }
		bool IsSelected() { return bSelected_; }
		void SetSelected(bool bSelected) { bSelected_ = bSelected; }
	};

	/**********************************************************
	//DxMessageBox
	**********************************************************/
	class DxMessageBox : public DxWindow {
	public:
		enum {
			INDEX_NULL = -1,
		};

	protected:
		int index_;
		gstd::ref_count_ptr<DxText> text_;
		std::vector<gstd::ref_count_ptr<DxButton> > listButton_;

	public:
		DxMessageBox();
		virtual void DispatchedEvent(gstd::ref_count_ptr<DxWindowEvent> event);
		void SetText(gstd::ref_count_ptr<DxText> text);
		void SetButton(std::vector<gstd::ref_count_ptr<DxButton> > listButton);
		int GetSelectedIndex() { return index_; }
		void UpdateWindowRect();
	};
}


#endif

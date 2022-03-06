#pragma once

#include "../../GcLib/pch.h"

#include "Constant.hpp"

#if defined(DNH_PROJ_EXECUTOR)
#include "../Common/DnhGcLibImpl.hpp"

//*******************************************************************
//EApplication
//*******************************************************************
class EDirectGraphics;
class EApplication : public Singleton<EApplication>, public Application {
	friend Singleton<EApplication>;
protected:
	EDirectGraphics* ptrGraphics;

	bool bWindowFocused_;

	shared_ptr<Texture> secondaryBackBuffer_;
protected:
	void _RenderDisplay();
public:
	EApplication();
	~EApplication();

	bool _Initialize();
	bool _Loop();
	bool _Finalize();
public:
	EDirectGraphics* GetPtrGraphics() { return ptrGraphics; }

	bool IsWindowFocused() { return bWindowFocused_; }

	void SetSecondaryBackBuffer(shared_ptr<Texture> texture) { secondaryBackBuffer_ = texture; }
};

//*******************************************************************
//EDirectGraphics
//*******************************************************************
class EDirectGraphics : public Singleton<EDirectGraphics>, public DirectGraphicsPrimaryWindow {
	friend Singleton<EDirectGraphics>;
protected:
	std::wstring defaultWindowTitle_;
protected:
	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	EDirectGraphics();
	~EDirectGraphics();

	virtual bool Initialize(const std::wstring& windowTitle);
	void SetRenderStateFor2D(BlendMode type);

	const std::wstring& GetDefaultWindowTitle() { return defaultWindowTitle_; }
	void SetWindowTitle(const std::wstring& title);
};

#endif
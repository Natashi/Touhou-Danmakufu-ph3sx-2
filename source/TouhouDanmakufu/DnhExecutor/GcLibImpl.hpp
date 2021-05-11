#pragma once

#include "../../GcLib/pch.h"

#include "Constant.hpp"
#include "../Common/DnhGcLibImpl.hpp"

//*******************************************************************
//EApplication
//*******************************************************************
class EDirectGraphics;
class EApplication : public Singleton<EApplication>, public Application {
	friend Singleton<EApplication>;
protected:
	EDirectGraphics* ptrGraphics;
public:
	EApplication();
	~EApplication();

	bool _Initialize();
	bool _Loop();
	bool _Finalize();

	EDirectGraphics* GetPtrGraphics() { return ptrGraphics; }
};

//*******************************************************************
//EDirectGraphics
//*******************************************************************
class EDirectGraphics : public Singleton<EDirectGraphics>, public DirectGraphicsPrimaryWindow {
	friend Singleton<EDirectGraphics>;
protected:
	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	EDirectGraphics();
	~EDirectGraphics();

	virtual bool Initialize(const std::wstring& windowTitle);
	void SetRenderStateFor2D(BlendMode type);
};
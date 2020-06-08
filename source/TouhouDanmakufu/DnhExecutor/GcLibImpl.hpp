#ifndef __TOUHOUDANMAKUFU_EXE_GCLIBIMPL__
#define __TOUHOUDANMAKUFU_EXE_GCLIBIMPL__

#include "../../GcLib/pch.h"

#include "Constant.hpp"
#include "../Common/DnhGcLibImpl.hpp"

/**********************************************************
//EApplication
**********************************************************/
class EDirectGraphics;
class EApplication : public Singleton<EApplication>, public Application {
	friend Singleton<EApplication>;
	EApplication();
protected:
	EDirectGraphics* ptrGraphics;
public:
	~EApplication();

	bool _Initialize();
	bool _Loop();
	bool _Finalize();

	EDirectGraphics* GetPtrGraphics() { return ptrGraphics; }
};

/**********************************************************
//EDirectGraphics
**********************************************************/
class EDirectGraphics : public Singleton<EDirectGraphics>, public DirectGraphicsPrimaryWindow {
	friend Singleton<EDirectGraphics>;
	EDirectGraphics();
protected:
	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	~EDirectGraphics();
	virtual bool Initialize();
	void SetRenderStateFor2D(int blend);
};

#endif

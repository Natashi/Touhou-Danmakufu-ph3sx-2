#ifndef __DIRECTX_DXCONSTANT__
#define __DIRECTX_DXCONSTANT__

#include "../pch.h"
#include "../gstd/GstdLib.hpp"

class DirectGraphicsListener {
public:
	virtual ~DirectGraphicsListener() {}
	virtual void ReleaseDxResource() {}
	virtual void RestoreDxResource() {}
};

#endif

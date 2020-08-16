#pragma once

#include "../pch.h"
#include "../gstd/GstdLib.hpp"

#if defined(DNH_PROJ_EXECUTOR)
#include "Vertex.hpp"
#endif

class DirectGraphicsListener {
public:
	virtual ~DirectGraphicsListener() {}
	virtual void ReleaseDxResource() {}
	virtual void RestoreDxResource() {}
};
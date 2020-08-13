#pragma once

#include "../pch.h"
#include "../gstd/GstdLib.hpp"

#include "Vertex.hpp"

class DirectGraphicsListener {
public:
	virtual ~DirectGraphicsListener() {}
	virtual void ReleaseDxResource() {}
	virtual void RestoreDxResource() {}
};
#pragma once

#include "../pch.h"
#include "../gstd/GstdLib.hpp"

#include "DxTypes.hpp"

#if defined(DNH_PROJ_EXECUTOR)
#include "Vertex.hpp"
#endif

namespace directx {
	//*******************************************************************
	//EDirect3D9
	//*******************************************************************
	class EDirect3D9 : public gstd::Singleton<EDirect3D9> {
	private:
		IDirect3D9* pDirect3D_;
	public:
		EDirect3D9();
		~EDirect3D9();

		IDirect3D9* GetD3D() { return pDirect3D_; }
	};
}
#ifndef __DIRECTX_VERTEXBUFFER__
#define __DIRECTX_VERTEXBUFFER__

#include "../pch.h"

#include "DxConstant.hpp"
#include "Vertex.hpp"

namespace directx {
	template<typename T>
	class GrowableBufferBase {
	public:
		GrowableBufferBase();
		GrowableBufferBase(IDirect3DDevice9* device);
		virtual ~GrowableBufferBase();

		virtual void Setup(size_t iniSize, size_t stride) = 0;
		virtual void Create() = 0;
		virtual inline void Release() { ptr_release(buffer_); }

		virtual void Expand(size_t newSize) = 0;

		T* GetBuffer() { return buffer_; }
		size_t GetSize() { return size_; }
	protected:
		IDirect3DDevice9* pDevice_;
		T* buffer_;
		size_t size_;
		size_t stride_;
	};

	class GrowableVertexBuffer : public GrowableBufferBase<IDirect3DVertexBuffer9> {
	public:
		GrowableVertexBuffer(IDirect3DDevice9* device);
		virtual ~GrowableVertexBuffer();

		virtual void Setup(size_t iniSize, size_t stride) {};
		virtual void Setup(size_t iniSize, size_t stride, DWORD fvf);
		virtual void Create();
		virtual void Expand(size_t newSize);
	private:
		DWORD fvf_;
	};
	class GrowableIndexBuffer : public GrowableBufferBase<IDirect3DIndexBuffer9> {
	public:
		GrowableIndexBuffer(IDirect3DDevice9* device);
		virtual ~GrowableIndexBuffer();

		virtual void Setup(size_t iniSize, size_t stride) {}
		virtual void Setup(size_t iniSize, size_t stride, D3DFORMAT format);
		virtual void Create();
		virtual void Expand(size_t newSize);
	private:
		D3DFORMAT format_;
	};

	class DirectGraphics;
	class VertexBufferManager : public DirectGraphicsListener {
		static VertexBufferManager* thisBase_;
	public:
		enum : size_t {
			BUFFER_VERTEX_TLX,
			BUFFER_VERTEX_LX,
			BUFFER_VERTEX_NX,

			BYTE_MAX_TLX = 65536 * sizeof(VERTEX_TLX),
			BYTE_MAX_LX = 65536 * sizeof(VERTEX_LX),
			BYTE_MAX_NX = 65536 * sizeof(VERTEX_NX),
			BYTE_MAX_INDEX = 65536 * sizeof(uint16_t),
		};

		VertexBufferManager();
		~VertexBufferManager();

		virtual void ReleaseDxResource();
		virtual void RestoreDxResource();

		virtual bool Initialize(DirectGraphics* graphics);
		virtual void Release();

		IDirect3DVertexBuffer9* GetVertexBuffer(size_t index) { return vertexBuffers_[index]; }
		IDirect3DIndexBuffer9* GetIndexBuffer() { return indexBuffer_; }

		GrowableVertexBuffer* GetGrowableVertexBuffer() { return vertexBufferGrowable_; }
		GrowableIndexBuffer* GetGrowableIndexBuffer() { return indexBufferGrowable_; }

		GrowableVertexBuffer* GetInstancingVertexBuffer() { return vertexBuffer_HWInstancing_; }

		static VertexBufferManager* GetBase() { return thisBase_; }
	private:
		std::vector<IDirect3DVertexBuffer9*> vertexBuffers_;
		IDirect3DIndexBuffer9* indexBuffer_;

		GrowableVertexBuffer* vertexBufferGrowable_;
		GrowableIndexBuffer* indexBufferGrowable_;

		GrowableVertexBuffer* vertexBuffer_HWInstancing_;

		virtual void CreateBuffers(IDirect3DDevice9* device);
	};
}

#endif
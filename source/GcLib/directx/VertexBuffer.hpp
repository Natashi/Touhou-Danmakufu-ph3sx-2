#pragma once

#include "../pch.h"

#include "DxConstant.hpp"

namespace directx {
	struct BufferLockParameter {
		UINT lockOffset = 0U;
		DWORD lockFlag = 0U;
		void* data = nullptr;
		size_t dataCount = 0U;
		size_t dataStride = 1U;

		BufferLockParameter() {
			lockOffset = 0U;
			lockFlag = 0U;
			data = nullptr;
			dataCount = 0U;
			dataStride = 1U;
		};
		BufferLockParameter(DWORD _lockFlag) {
			lockOffset = 0U;
			lockFlag = _lockFlag;
			data = nullptr;
			dataCount = 0U;
			dataStride = 1U;
		};
		BufferLockParameter(void* _data, size_t _count, size_t _stride, DWORD _lockFlag) {
			lockOffset = 0U;
			lockFlag = _lockFlag;
			data = 0U;
			dataCount = _count;
			dataStride = _stride;
		};

		template<typename T>
		void SetSource(T& vecSrc, size_t countMax, size_t _stride) {
			data = vecSrc.data();
			dataCount = std::min(countMax, vecSrc.size());
			dataStride = _stride;
		}
	};

	class VertexBufferManager;

	template<typename T>
	class BufferBase {
		static_assert(std::is_base_of<IDirect3DResource9, T>::value, "T must be a Direct3D resource");
		friend class VertexBufferManager;
	public:
		BufferBase();
		BufferBase(IDirect3DDevice9* device);
		virtual ~BufferBase();

		void SetUsage(DWORD usage) { usage_ = usage; }
		void SetPool(D3DPOOL pool) { pool_ = pool; }

		inline void Release() { ptr_release(buffer_); }

		HRESULT Create(DWORD usage, D3DPOOL pool);
		HRESULT UpdateBuffer(BufferLockParameter* pLock);

		T* GetBuffer() { return buffer_; }
		size_t GetSize() { return size_; }
		size_t GetSizeInBytes() { return sizeInBytes_; }
	protected:
		virtual HRESULT _Create() = 0;
	protected:
		IDirect3DDevice9* pDevice_;
		DWORD usage_;
		D3DPOOL pool_;

		T* buffer_;
		size_t size_;
		size_t stride_;
		size_t sizeInBytes_;
	};

	class FixedVertexBuffer : public BufferBase<IDirect3DVertexBuffer9> {
	public:
		FixedVertexBuffer(IDirect3DDevice9* device);
		virtual ~FixedVertexBuffer();

		virtual void Setup(size_t iniSize, size_t stride, DWORD fvf);
	protected:
		virtual HRESULT _Create();
	private:
		DWORD fvf_;
	};
	class FixedIndexBuffer : public BufferBase<IDirect3DIndexBuffer9> {
	public:
		FixedIndexBuffer(IDirect3DDevice9* device);
		virtual ~FixedIndexBuffer();

		virtual void Setup(size_t iniSize, size_t stride, D3DFORMAT format);
	protected:
		virtual HRESULT _Create();
	private:
		D3DFORMAT format_;
	};

	template<typename T>
	class GrowableBuffer : public BufferBase<T> {
	public:
		GrowableBuffer(IDirect3DDevice9* device);
		virtual ~GrowableBuffer();

		virtual void Expand(size_t newSize) = 0;
	};

	class GrowableVertexBuffer : public GrowableBuffer<IDirect3DVertexBuffer9> {
	public:
		GrowableVertexBuffer(IDirect3DDevice9* device);
		virtual ~GrowableVertexBuffer();

		virtual void Setup(size_t iniSize, size_t stride, DWORD fvf);
		virtual void Expand(size_t newSize);
	protected:
		virtual HRESULT _Create();
	private:
		DWORD fvf_;
	};
	class GrowableIndexBuffer : public GrowableBuffer<IDirect3DIndexBuffer9> {
	public:
		GrowableIndexBuffer(IDirect3DDevice9* device);
		virtual ~GrowableIndexBuffer();

		virtual void Setup(size_t iniSize, size_t stride, D3DFORMAT format);
		virtual void Expand(size_t newSize);
	protected:
		virtual HRESULT _Create();
	private:
		D3DFORMAT format_;
	};

	class DirectGraphics;
	class VertexBufferManager : public DirectGraphicsListener {
		static VertexBufferManager* thisBase_;
	public:
		enum : size_t {
			MAX_STRIDE_STATIC = 65536U,
		};

		VertexBufferManager();
		~VertexBufferManager();

		static VertexBufferManager* GetBase() { return thisBase_; }
		gstd::CriticalSection* GetCriticalSection() { return &cs_; }

		virtual void ReleaseDxResource();
		virtual void RestoreDxResource();

		virtual bool Initialize(DirectGraphics* graphics);
		virtual void Release();

		FixedVertexBuffer* GetVertexBufferTLX() { return vertexBuffers_[0].get(); }
		FixedVertexBuffer* GetVertexBufferLX() { return vertexBuffers_[1].get(); }
		FixedVertexBuffer* GetVertexBufferNX() { return vertexBuffers_[2].get(); }
		FixedIndexBuffer* GetIndexBuffer() { return indexBuffer_.get(); }

		GrowableVertexBuffer* GetGrowableVertexBuffer() { return vertexBufferGrowable_.get(); }
		GrowableIndexBuffer* GetGrowableIndexBuffer() { return indexBufferGrowable_.get(); }

		GrowableVertexBuffer* GetInstancingVertexBuffer() { return vertexBuffer_HWInstancing_.get(); }

		static void AssertBuffer(HRESULT hr, const std::wstring& bufferID);
		
		BufferBase<IDirect3DVertexBuffer9>* CreateExtraVertexBuffer();
		BufferBase<IDirect3DVertexBuffer9>* GetExtraVertexBuffer(size_t addr);
		void ReleaseExtraVertexBuffer(size_t addr);
	private:
		gstd::CriticalSection cs_;

		/*
		 * 0 -> TLX
		 * 1 -> LX
		 * 2 -> NX
		 */
		std::vector<unique_ptr<FixedVertexBuffer>> vertexBuffers_;
		unique_ptr<FixedIndexBuffer> indexBuffer_;

		unique_ptr<GrowableVertexBuffer> vertexBufferGrowable_;
		unique_ptr<GrowableIndexBuffer> indexBufferGrowable_;

		unique_ptr<GrowableVertexBuffer> vertexBuffer_HWInstancing_;

		std::unordered_map<size_t, unique_ptr<BufferBase<IDirect3DVertexBuffer9>>> mapExtraBuffer_Vertex_;

		virtual void CreateBuffers(IDirect3DDevice9* device);
	};
}
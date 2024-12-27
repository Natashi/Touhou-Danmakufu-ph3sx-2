#pragma once

#include "../pch.h"

#include "DxConstant.hpp"

namespace directx {
	struct BufferLockParameter {
		UINT lockOffset;
		DWORD lockFlag;
		const void* data;
		size_t dataCount;
		size_t dataStride;

		BufferLockParameter() : BufferLockParameter(nullptr, 0, 1, 0) {}

		BufferLockParameter(DWORD lockFlag) : BufferLockParameter(nullptr, 0, 1, lockFlag) {}

		BufferLockParameter(const void* data, size_t count, size_t stride, DWORD lockFlag) 
			: data(data), dataCount(count), dataStride(stride), lockFlag(lockFlag), lockOffset(0) {}

		template<typename T>
		void SetSource(const T& src, size_t countMax, size_t stride) {
			data = src.data();
			dataCount = std::min(countMax, src.size());
			dataStride = stride;
		}
	};

	class VertexBufferManager;

	class BufferBase {
		friend VertexBufferManager;
	public:
		BufferBase(IDirect3DDevice9* device, const std::string& name, DWORD usage, D3DPOOL pool);
		BufferBase(IDirect3DDevice9* device, const std::string& name, DWORD usage, D3DPOOL pool, size_t size, size_t stride);
		virtual ~BufferBase();

		DWORD GetUsage() const { return usage_; }
		D3DPOOL GetPool() const { return pool_; }

		virtual void Release() {}

		void Create();

		HRESULT UpdateBuffer(BufferLockParameter* lock);

		virtual IDirect3DResource9* GetBuffer() { return nullptr; }

		size_t GetSize() const { return size_; }
		size_t GetSizeInBytes() const { return size_ * stride_; }

		const std::string& GetName() const { return name_; }
	protected:
		virtual HRESULT _Create() = 0;
		virtual HRESULT _Update(BufferLockParameter* lock, size_t offset, size_t size) = 0;
	protected:
		IDirect3DDevice9* pDevice_;

		DWORD usage_;
		D3DPOOL pool_;
		std::string name_;

		size_t size_;
		size_t stride_;
	};

	class VertexBuffer : public BufferBase {
	protected:
		IDirect3DVertexBuffer9* buffer_;

		DWORD fvf_;
	protected:
		virtual HRESULT _Create() override;
		HRESULT _Update(BufferLockParameter* lock, size_t offset, size_t size) override;
	public:
		VertexBuffer(IDirect3DDevice9* device, const std::string& name,
			DWORD usage, D3DPOOL pool, size_t size, size_t stride, DWORD fvf);
		virtual ~VertexBuffer() {}

		virtual void Release() override;

		virtual IDirect3DVertexBuffer9* GetBuffer() override { return buffer_; }
	};

	class IndexBuffer : public BufferBase {
	protected:
		IDirect3DIndexBuffer9* buffer_;

		D3DFORMAT format_; 
	protected:
		virtual HRESULT _Create() override;
		HRESULT _Update(BufferLockParameter* lock, size_t offset, size_t size) override;
	public:
		IndexBuffer(IDirect3DDevice9* device, const std::string& name,
			DWORD usage, D3DPOOL pool, size_t size, size_t stride, D3DFORMAT format);
		virtual ~IndexBuffer() {}

		virtual void Release() override;

		virtual IDirect3DIndexBuffer9* GetBuffer() override { return buffer_; }
	};

	class GrowableBuffer {
	public:
		virtual ~GrowableBuffer() {}

		virtual void Expand(size_t newSize) = 0;
	};

	class GrowableVertexBuffer : public VertexBuffer, public GrowableBuffer {
	public:
		GrowableVertexBuffer(IDirect3DDevice9* device, const std::string& name,
			DWORD usage, D3DPOOL pool, size_t size, size_t stride, DWORD fvf);

		virtual void Expand(size_t newSize) override;
	};
	class GrowableIndexBuffer : public IndexBuffer, public GrowableBuffer {
	public:
		GrowableIndexBuffer(IDirect3DDevice9* device, const std::string& name,
			DWORD usage, D3DPOOL pool, size_t size, size_t stride, D3DFORMAT format);

		virtual void Expand(size_t newSize) override;
	};

	class DirectGraphics;
	class VertexBufferManager : public DirectGraphicsListener {
		static VertexBufferManager* thisBase_;
	public:
		static constexpr size_t MAX_STRIDE_STATIC = 65536U;

		static const std::string NAME_VB_TLX;
		static const std::string NAME_VB_LX;
		static const std::string NAME_VB_NX;
		static const std::string NAME_IB;

		static const std::string NAME_DYN_VB_TLX;
		static const std::string NAME_DYN_IB;

		static const std::string NAME_INSTANCE;
	private:
		gstd::CriticalSection cs_;

		std::map<std::string, unique_ptr<BufferBase>> buffers_;
	private:
		void CreateBuffers();
	public:
		VertexBufferManager();
		~VertexBufferManager();

		static VertexBufferManager* GetBase() { return thisBase_; }
		gstd::CriticalSection* GetCriticalSection() { return &cs_; }

		virtual void ReleaseDxResource();
		virtual void RestoreDxResource();

		virtual bool Initialize(DirectGraphics* graphics);
		virtual void Release();

		void SetBuffer(const std::string& name, unique_ptr<BufferBase>&& buffer);
		void SetBuffer(unique_ptr<BufferBase>&& buffer);
		void RemoveBuffer(const std::string& name);

		template<typename T>
		T* GetBuffer(const std::string& name) {
			static_assert(std::is_base_of<BufferBase, T>::value, "T must be a Direct3D resource");

			auto itr = buffers_.find(name);
			if (itr != buffers_.end()) {
				return reinterpret_cast<T*>(itr->second.get());
			}
			return nullptr;
		}

		// ------------------------------------------------------

		VertexBuffer* GetVertexBufferTLX() { return GetBuffer<VertexBuffer>(NAME_VB_TLX); }
		VertexBuffer* GetVertexBufferLX() { return GetBuffer<VertexBuffer>(NAME_VB_LX); }
		VertexBuffer* GetVertexBufferNX() { return GetBuffer<VertexBuffer>(NAME_VB_NX); }
		IndexBuffer* GetIndexBuffer() { return GetBuffer<IndexBuffer>(NAME_IB); }

		GrowableVertexBuffer* GetGrowableVertexBuffer() { return GetBuffer<GrowableVertexBuffer>(NAME_DYN_VB_TLX); }
		GrowableIndexBuffer* GetGrowableIndexBuffer() { return GetBuffer<GrowableIndexBuffer>(NAME_DYN_IB); }

		GrowableVertexBuffer* GetInstancingVertexBuffer() { return GetBuffer<GrowableVertexBuffer>(NAME_INSTANCE); }
	};
}
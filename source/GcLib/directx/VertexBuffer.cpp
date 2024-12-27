#include "source/GcLib/pch.h"

#include "VertexBuffer.hpp"
#include "DirectGraphics.hpp"

using namespace gstd;

namespace directx {
	BufferBase::BufferBase(
		IDirect3DDevice9* device, const std::string& name, DWORD usage, D3DPOOL pool
	) : BufferBase(device, name, usage, pool, 0, 1) {}

	BufferBase::BufferBase(
		IDirect3DDevice9* device, const std::string& name,
		DWORD usage, D3DPOOL pool, size_t size, size_t stride
	) :
		pDevice_(device), name_(name),
		usage_(usage), pool_(pool), size_(size), stride_(stride) {}

	BufferBase::~BufferBase() {
		Release();
	}

	HRESULT BufferBase::UpdateBuffer(BufferLockParameter* lock) {
		if (lock == nullptr) return E_POINTER;
		else if (lock->lockOffset >= size_) return E_INVALIDARG;
		else if (lock->dataCount == 0U || lock->dataStride == 0U) return S_OK;

		size_t lockOffset = lock->lockOffset * lock->dataStride;
		size_t lockCopySize = std::min(lock->dataCount, size_ - lock->lockOffset) * lock->dataStride;

		HRESULT hr = _Update(lock, lockOffset, lockCopySize);
		return hr;
	}

	void BufferBase::Create() {
		Lock lock(VertexBufferManager::GetBase()->GetCriticalSection());
		Release();

		HRESULT hr = _Create();
		if (FAILED(hr)) {
			auto err = StringUtility::Format(
				"VertexBufferManager: Buffer creation failure [%s]\t\r\n%s: %s",
				name_.c_str(), DXGetErrorStringA(hr), DXGetErrorDescriptionA(hr));
			Logger::WriteError(err);
			throw wexception(err);
		}
	}

	//-----------------------------------------------------------------------------------------
	
	VertexBuffer::VertexBuffer(
		IDirect3DDevice9* device, const std::string& name,
		DWORD usage, D3DPOOL pool, size_t size, size_t stride, DWORD fvf
	) :
		BufferBase(device, name, usage, pool, size, stride), buffer_(nullptr), fvf_(fvf) {}

	void VertexBuffer::Release() {
		ptr_release(buffer_);
	}

	HRESULT VertexBuffer::_Create() {
		return pDevice_->CreateVertexBuffer(GetSizeInBytes(), usage_,
			fvf_, pool_, &buffer_, nullptr);
	}
	HRESULT VertexBuffer::_Update(BufferLockParameter* lock, size_t offset, size_t size) {
		if (buffer_ == nullptr) return E_POINTER;

		void* data;

		HRESULT hr = buffer_->Lock(offset, size, &data, lock->lockFlag);
		if (SUCCEEDED(hr)) {
			memcpy_s(data, GetSizeInBytes(), lock->data, size);

			buffer_->Unlock();
		}

		return hr;
	}

	//-----------------------------------------------------------------------------------------

	IndexBuffer::IndexBuffer(
		IDirect3DDevice9* device, const std::string& name,
		DWORD usage, D3DPOOL pool, size_t size, size_t stride, D3DFORMAT format
	) :
		BufferBase(device, name, usage, pool, size, stride), buffer_(nullptr), format_(format) {}

	void IndexBuffer::Release() {
		ptr_release(buffer_);
	}

	HRESULT IndexBuffer::_Create() {
		return pDevice_->CreateIndexBuffer(GetSizeInBytes(), usage_,
			format_, pool_, &buffer_, nullptr);
	}
	HRESULT IndexBuffer::_Update(BufferLockParameter* lock, size_t offset, size_t size) {
		if (buffer_ == nullptr) return E_POINTER;

		void* data;

		HRESULT hr = buffer_->Lock(offset, size, &data, lock->lockFlag);
		if (SUCCEEDED(hr)) {
			memcpy_s(data, GetSizeInBytes(), lock->data, size);

			buffer_->Unlock();
		}

		return hr;
	}

	//-----------------------------------------------------------------------------------------

	GrowableVertexBuffer::GrowableVertexBuffer(
		IDirect3DDevice9* device, const std::string& name,
		DWORD usage, D3DPOOL pool, size_t size, size_t stride, DWORD fvf
	) :
		VertexBuffer(device, name, usage, pool, size, stride, fvf) {}

	void GrowableVertexBuffer::Expand(size_t newSize) {
		if (size_ >= newSize) return;
		while (size_ < newSize) size_ *= 2U;

		Create();
	}

	//-----------------------------------------------------------------------------------------

	GrowableIndexBuffer::GrowableIndexBuffer(
		IDirect3DDevice9* device, const std::string& name,
		DWORD usage, D3DPOOL pool, size_t size, size_t stride, D3DFORMAT format
	) :
		IndexBuffer(device, name, usage, pool, size, stride, format) {}

	void GrowableIndexBuffer::Expand(size_t newSize) {
		if (size_ >= newSize) return;
		while (size_ < newSize) size_ *= 2U;

		Create();
	}

	//-----------------------------------------------------------------------------------------

	VertexBufferManager* VertexBufferManager::thisBase_ = nullptr;

	const std::string VertexBufferManager::NAME_VB_TLX = "FixedVB_TLX";
	const std::string VertexBufferManager::NAME_VB_LX = "FixedVB_LX";
	const std::string VertexBufferManager::NAME_VB_NX = "FixedVB_NX";
	const std::string VertexBufferManager::NAME_IB = "FixedIB_i16";
	const std::string VertexBufferManager::NAME_DYN_VB_TLX = "DynamicVB_TLX";
	const std::string VertexBufferManager::NAME_DYN_IB = "DynamicIB_i32";
	const std::string VertexBufferManager::NAME_INSTANCE = "DynamicVB_Instance";

	VertexBufferManager::VertexBufferManager() {
	}
	VertexBufferManager::~VertexBufferManager() {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		graphics->RemoveDirectGraphicsListener(this);

		Release();
	}

	bool VertexBufferManager::Initialize(DirectGraphics* graphics) {
		if (thisBase_) return false;
		thisBase_ = this;

		graphics->AddDirectGraphicsListener(this);
		
		IDirect3DDevice9* device = graphics->GetDevice();

		const DWORD usage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
		const D3DPOOL pool = D3DPOOL_DEFAULT;

		{
			std::tuple<std::string, size_t, DWORD> listVertexData[] = {
				{ NAME_VB_TLX, sizeof(VERTEX_TLX), VERTEX_TLX::fvf},
				{ NAME_VB_LX, sizeof(VERTEX_LX), VERTEX_LX::fvf },
				{ NAME_VB_NX, sizeof(VERTEX_NX), VERTEX_NX::fvf },
			};
			for (auto& [name, stride, fvf] : listVertexData) {
				SetBuffer(make_unique<VertexBuffer>(device, name,
					usage, pool, MAX_STRIDE_STATIC, stride, fvf));
			}

			SetBuffer(make_unique<IndexBuffer>(device, NAME_IB,
				usage, pool, MAX_STRIDE_STATIC, sizeof(uint16_t), D3DFMT_INDEX16));
		}

		SetBuffer(make_unique<GrowableVertexBuffer>(device, NAME_DYN_VB_TLX,
			usage, pool, 8192U, sizeof(VERTEX_TLX), VERTEX_TLX::fvf));

		SetBuffer(make_unique<GrowableIndexBuffer>(device, NAME_DYN_IB,
			usage, pool, 8192U, sizeof(uint32_t), D3DFMT_INDEX32));

		SetBuffer(make_unique<GrowableVertexBuffer>(device, NAME_INSTANCE,
			usage, pool, 512U, sizeof(VERTEX_INSTANCE), D3DFMT_UNKNOWN));

		CreateBuffers();

		return true;
	}
	
	void VertexBufferManager::CreateBuffers() {
		for (auto& [_, buffer] : buffers_) {
			if (buffer)
				buffer->Create();
		}
	}
	void VertexBufferManager::Release() {
		for (auto& [_, buffer] : buffers_) {
			if (buffer)
				buffer->Release();
		}
	}

	void VertexBufferManager::SetBuffer(unique_ptr<BufferBase>&& buffer) {
		buffers_[buffer->GetName()] = MOVE(buffer);
	}
	void VertexBufferManager::SetBuffer(const std::string& name, unique_ptr<BufferBase>&& buffer) {
		buffers_[name] = MOVE(buffer);
	}
	void VertexBufferManager::RemoveBuffer(const std::string& name) {
		buffers_.erase(name);
	}

	void VertexBufferManager::ReleaseDxResource() {
		//Release();

		for (auto& [_, buffer] : buffers_) {
			if (buffer->pool_ != D3DPOOL_DEFAULT)
				continue;

			buffer->Release();

			std::string msg = StringUtility::Format(
				"VertexBufferManager: Release buffer resource [%s]",
				buffer->GetName().c_str());
			Logger::WriteInfo(msg);
		}
	}
	void VertexBufferManager::RestoreDxResource() {
		//CreateBuffers(DirectGraphics::GetBase()->GetDevice());

		for (auto& [_, buffer] : buffers_) {
			if (buffer->pool_ != D3DPOOL_DEFAULT)
				continue;

			buffer->Create();

			std::string msg = StringUtility::Format(
				"VertexBufferManager: Restore buffer resource [%s]",
				buffer->GetName().c_str());
			Logger::WriteInfo(msg);
		}
	}
}
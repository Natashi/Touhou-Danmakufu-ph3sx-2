#include "source/GcLib/pch.h"

#include "VertexBuffer.hpp"
#include "DirectGraphics.hpp"

using namespace gstd;

namespace directx {
	template class BufferBase<IDirect3DVertexBuffer9>;
	template class BufferBase<IDirect3DIndexBuffer9>;

	template<typename T>
	BufferBase<T>::BufferBase() {
		pDevice_ = nullptr;
		buffer_ = nullptr;
		size_ = 0U;
		stride_ = 0U;
		sizeInBytes_ = 0U;
	}
	template<typename T>
	BufferBase<T>::BufferBase(IDirect3DDevice9* device) {
		pDevice_ = device;
		buffer_ = nullptr;
		size_ = 0U;
		stride_ = 0U;
		sizeInBytes_ = 0U;
	}
	template<typename T>
	BufferBase<T>::~BufferBase() {
		Release();
	}
	template<typename T>
	HRESULT BufferBase<T>::UpdateBuffer(BufferLockParameter* pLock) {
		HRESULT hr = S_OK;

		if (pLock == nullptr || buffer_ == nullptr) return E_POINTER;
		else if (pLock->lockOffset >= size_) return E_INVALIDARG;
		else if (pLock->dataCount == 0U || pLock->dataStride == 0U) return S_OK;

		size_t usableCount = size_ - pLock->lockOffset;
		size_t lockCopySize = std::min(pLock->dataCount, usableCount) * pLock->dataStride;

		void* tmp;
		hr = buffer_->Lock(pLock->lockOffset * pLock->dataStride, lockCopySize, &tmp, pLock->lockFlag);
		if (SUCCEEDED(hr)) {
			memcpy_s(tmp, sizeInBytes_, pLock->data, lockCopySize);
			buffer_->Unlock();
		}

		return hr;
	}

	FixedVertexBuffer::FixedVertexBuffer(IDirect3DDevice9* device) : BufferBase(device) {
		fvf_ = 0U;
	}
	FixedVertexBuffer::~FixedVertexBuffer() {
	}
	void FixedVertexBuffer::Setup(size_t iniSize, size_t stride, DWORD fvf) {
		fvf_ = fvf;
		stride_ = stride;
		size_ = iniSize;
	}
	HRESULT FixedVertexBuffer::Create() {
		//gstd::Lock lock(pManager_->GetLock());
		this->Release();
		sizeInBytes_ = size_ * stride_;
		return pDevice_->CreateVertexBuffer(sizeInBytes_, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 
			fvf_, D3DPOOL_DEFAULT, &buffer_, nullptr);
	}
	FixedIndexBuffer::FixedIndexBuffer(IDirect3DDevice9* device) : BufferBase(device) {
		pDevice_ = device;
		format_ = D3DFMT_INDEX16;
	}
	FixedIndexBuffer::~FixedIndexBuffer() {
	}
	void FixedIndexBuffer::Setup(size_t iniSize, size_t stride, D3DFORMAT format) {
		format_ = format;
		stride_ = stride;
		size_ = iniSize;
	}
	HRESULT FixedIndexBuffer::Create() {
		//gstd::Lock lock(pManager_->GetLock());
		this->Release();
		sizeInBytes_ = size_ * stride_;
		return pDevice_->CreateIndexBuffer(sizeInBytes_, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
			format_, D3DPOOL_DEFAULT, &buffer_, nullptr);
	}

	template<typename T>
	GrowableBuffer<T>::GrowableBuffer(IDirect3DDevice9* device) : BufferBase(device) {
	}
	template<typename T>
	GrowableBuffer<T>::~GrowableBuffer() {
		Release();
	}

	GrowableVertexBuffer::GrowableVertexBuffer(IDirect3DDevice9* device) : GrowableBuffer(device) {
		fvf_ = 0U;
	}
	GrowableVertexBuffer::~GrowableVertexBuffer() {
	}
	void GrowableVertexBuffer::Setup(size_t iniSize, size_t stride, DWORD fvf) {
		fvf_ = fvf;
		stride_ = stride;
		size_ = iniSize;
	}
	HRESULT GrowableVertexBuffer::Create() {
		//gstd::Lock lock(pManager_->GetLock());
		this->Release();
		sizeInBytes_ = size_ * stride_;
		return pDevice_->CreateVertexBuffer(sizeInBytes_, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 
			fvf_, D3DPOOL_DEFAULT, &buffer_, nullptr);
	}
	void GrowableVertexBuffer::Expand(size_t newSize) {
		if (size_ >= newSize) return;
		while (size_ < newSize) size_ *= 2U;
		VertexBufferManager::AssertBuffer(Create(), L"VB_Growable");
	}

	GrowableIndexBuffer::GrowableIndexBuffer(IDirect3DDevice9* device) : GrowableBuffer(device) {
		format_ = D3DFMT_INDEX16;
	}
	GrowableIndexBuffer::~GrowableIndexBuffer() {
	}
	void GrowableIndexBuffer::Setup(size_t iniSize, size_t stride, D3DFORMAT format) {
		format_ = format;
		stride_ = stride;
		size_ = iniSize;
	}
	HRESULT GrowableIndexBuffer::Create() {
		//gstd::Lock lock(pManager_->GetLock());
		this->Release();
		sizeInBytes_ = size_ * stride_;
		return pDevice_->CreateIndexBuffer(sizeInBytes_, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
			format_, D3DPOOL_DEFAULT, &buffer_, nullptr);
	}
	void GrowableIndexBuffer::Expand(size_t newSize) {
		if (size_ >= newSize) return;
		while (size_ < newSize) size_ *= 2U;
		VertexBufferManager::AssertBuffer(Create(), L"IB_Growable");
	}

	VertexBufferManager* VertexBufferManager::thisBase_ = nullptr;
	VertexBufferManager::VertexBufferManager() {
		indexBuffer_ = nullptr;
		vertexBufferGrowable_ = nullptr;
		indexBufferGrowable_ = nullptr;
		vertexBuffer_HWInstancing_ = nullptr;
	}
	VertexBufferManager::~VertexBufferManager() {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		graphics->RemoveDirectGraphicsListener(this);

		Release();

		for (auto& iVB : vertexBuffers_)
			ptr_delete(iVB);
		ptr_delete(indexBuffer_);

		ptr_delete(vertexBufferGrowable_);
		ptr_delete(indexBufferGrowable_);
		ptr_delete(vertexBuffer_HWInstancing_);
	}

	bool VertexBufferManager::Initialize(DirectGraphics* graphics) {
		if (thisBase_) return false;
		thisBase_ = this;

		graphics->AddDirectGraphicsListener(this);
		
		IDirect3DDevice9* device = graphics->GetDevice();

		{
			//Stride, FVF
			std::pair<size_t, DWORD> listVertexData[3] = {
				std::make_pair(sizeof(VERTEX_TLX), VERTEX_TLX::fvf),
				std::make_pair(sizeof(VERTEX_LX), VERTEX_LX::fvf),
				std::make_pair(sizeof(VERTEX_NX), VERTEX_NX::fvf),
			};
			for (size_t i = 0; i < 3; ++i) {
				std::pair<size_t, DWORD>* data = &listVertexData[i];
				FixedVertexBuffer* buffer = new FixedVertexBuffer(device);
				buffer->Setup(MAX_STRIDE_STATIC, data->first, data->second);
				vertexBuffers_.push_back(buffer);
			}

			indexBuffer_ = new FixedIndexBuffer(device);
			indexBuffer_->Setup(MAX_STRIDE_STATIC, sizeof(uint16_t), D3DFMT_INDEX16);
		}
		vertexBufferGrowable_ = new GrowableVertexBuffer(device);
		vertexBufferGrowable_->Setup(8192U, sizeof(VERTEX_TLX), VERTEX_TLX::fvf);
		indexBufferGrowable_ = new GrowableIndexBuffer(device);
		indexBufferGrowable_->Setup(8192U, sizeof(uint32_t), D3DFMT_INDEX32);

		vertexBuffer_HWInstancing_ = new GrowableVertexBuffer(device);
		vertexBuffer_HWInstancing_->Setup(512U, sizeof(VERTEX_INSTANCE), 0);

		CreateBuffers(device);

		return true;
	}
	void VertexBufferManager::AssertBuffer(HRESULT hr, const std::wstring& bufferName) {
		if (SUCCEEDED(hr)) return;
		std::wstring err = StringUtility::Format(L"VertexBufferManager::CreateBuffers: "
			"Buffer creation failure. [%s]\t\r\n%s: %s",
			bufferName.c_str(), DXGetErrorString(hr), DXGetErrorDescription(hr));
		throw gstd::wexception(err);
	}
	void VertexBufferManager::CreateBuffers(IDirect3DDevice9* device) {
		{
			size_t iVB = 0;
			for (auto& pVB : vertexBuffers_) {
				AssertBuffer(pVB->Create(), StringUtility::Format(L"VB%d", iVB));
				++iVB;
			}
		}
		AssertBuffer(indexBuffer_->Create(), L"IB");

		AssertBuffer(vertexBufferGrowable_->Create(), L"VB_Growable");
		AssertBuffer(indexBufferGrowable_->Create(), L"IB_Growable");
		AssertBuffer(vertexBuffer_HWInstancing_->Create(), L"VB_InstanceHW");
	}
	void VertexBufferManager::Release() {
		for (auto& iVB : vertexBuffers_)
			iVB->Release();
		indexBuffer_->Release();

		vertexBufferGrowable_->Release();
		indexBufferGrowable_->Release();
		vertexBuffer_HWInstancing_->Release();
	}

	void VertexBufferManager::ReleaseDxResource() {
		Release();
	}
	void VertexBufferManager::RestoreDxResource() {
		CreateBuffers(DirectGraphics::GetBase()->GetDevice());
	}
}
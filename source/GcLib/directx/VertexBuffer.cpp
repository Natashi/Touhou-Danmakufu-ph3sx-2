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
		usage_ = 0;
		pool_ = D3DPOOL_DEFAULT;

		buffer_ = nullptr;
		size_ = 0U;
		stride_ = 0U;
	}
	template<typename T>
	BufferBase<T>::BufferBase(IDirect3DDevice9* device) : BufferBase() {
		pDevice_ = device;
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

		size_t totalSizeInBytes = GetSizeInBytes();
		size_t usableCount = size_ - pLock->lockOffset;
		size_t lockCopySize = std::min(pLock->dataCount, usableCount) * pLock->dataStride;

		void* tmp;
		hr = buffer_->Lock(pLock->lockOffset * pLock->dataStride, lockCopySize, &tmp, pLock->lockFlag);
		if (SUCCEEDED(hr)) {
			memcpy_s(tmp, totalSizeInBytes, pLock->data, lockCopySize);
			buffer_->Unlock();
		}

		return hr;
	}
	template<typename T>
	HRESULT BufferBase<T>::Create(DWORD usage, D3DPOOL pool) {
		gstd::Lock lock(VertexBufferManager::GetBase()->GetCriticalSection());
		this->Release();

		usage_ = usage;
		pool_ = pool;

		HRESULT res = _Create();
		return res;
	}

	//-----------------------------------------------------------------------------------------

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
	HRESULT FixedVertexBuffer::_Create() {
		return pDevice_->CreateVertexBuffer(GetSizeInBytes(), usage_,
			fvf_, pool_, &buffer_, nullptr);
	}

	//-----------------------------------------------------------------------------------------

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
	HRESULT FixedIndexBuffer::_Create() {
		return pDevice_->CreateIndexBuffer(GetSizeInBytes(), usage_,
			format_, pool_, &buffer_, nullptr);
	}

	//-----------------------------------------------------------------------------------------

	template<typename T>
	GrowableBuffer<T>::GrowableBuffer(IDirect3DDevice9* device) : BufferBase(device) {
	}
	template<typename T>
	GrowableBuffer<T>::~GrowableBuffer() {
		Release();
	}

	//-----------------------------------------------------------------------------------------

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
	void GrowableVertexBuffer::Expand(size_t newSize) {
		if (size_ >= newSize) return;
		while (size_ < newSize) size_ *= 2U;

		HRESULT hr = _Create();
		VertexBufferManager::AssertBuffer(hr, L"VB_Growable");
	}
	HRESULT GrowableVertexBuffer::_Create() {
		return pDevice_->CreateVertexBuffer(GetSizeInBytes(), usage_,
			fvf_, pool_, &buffer_, nullptr);
	}

	//-----------------------------------------------------------------------------------------

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
	void GrowableIndexBuffer::Expand(size_t newSize) {
		if (size_ >= newSize) return;
		while (size_ < newSize) size_ *= 2U;

		HRESULT hr = _Create();
		VertexBufferManager::AssertBuffer(hr, L"IB_Growable");
	}
	HRESULT GrowableIndexBuffer::_Create() {
		return pDevice_->CreateIndexBuffer(GetSizeInBytes(), usage_,
			format_, pool_, &buffer_, nullptr);
	}

	//-----------------------------------------------------------------------------------------

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

		vertexBuffers_.clear();
		indexBuffer_.reset();

		vertexBufferGrowable_.reset();
		indexBufferGrowable_.reset();
		vertexBuffer_HWInstancing_.reset();

		for (auto& [addr, pBuffer] : mapExtraBuffer_Vertex_)
			pBuffer.reset();
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
			for (auto& [stride, fvf] : listVertexData) {
				FixedVertexBuffer* buffer = new FixedVertexBuffer(device);
				buffer->Setup(MAX_STRIDE_STATIC, stride, fvf);

				vertexBuffers_.push_back(unique_ptr<FixedVertexBuffer>(buffer));
			}

			indexBuffer_.reset(new FixedIndexBuffer(device));
			indexBuffer_->Setup(MAX_STRIDE_STATIC, sizeof(uint16_t), D3DFMT_INDEX16);
		}

		vertexBufferGrowable_.reset(new GrowableVertexBuffer(device));
		vertexBufferGrowable_->Setup(8192U, sizeof(VERTEX_TLX), VERTEX_TLX::fvf);
		indexBufferGrowable_.reset(new GrowableIndexBuffer(device));
		indexBufferGrowable_->Setup(8192U, sizeof(uint32_t), D3DFMT_INDEX32);

		vertexBuffer_HWInstancing_.reset(new GrowableVertexBuffer(device));
		vertexBuffer_HWInstancing_->Setup(512U, sizeof(VERTEX_INSTANCE), 0);

		CreateBuffers(device);

		return true;
	}
	void VertexBufferManager::AssertBuffer(HRESULT hr, const std::wstring& bufferName) {
		if (SUCCEEDED(hr)) return;
		std::wstring err = StringUtility::Format(L"VertexBufferManager: "
			"Buffer creation failure [%s]\t\r\n%s: %s",
			bufferName.c_str(), DXGetErrorString(hr), DXGetErrorDescription(hr));
		throw gstd::wexception(err);
	}
	void VertexBufferManager::CreateBuffers(IDirect3DDevice9* device) {
		const DWORD usage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
		const D3DPOOL pool = D3DPOOL_DEFAULT;

		{
			size_t iVB = 0;
			for (auto& pVB : vertexBuffers_) {
				AssertBuffer(pVB->Create(usage, pool), StringUtility::Format(L"VB%d", iVB));
				++iVB;
			}
		}
		AssertBuffer(indexBuffer_->Create(usage, pool), L"IB");

		AssertBuffer(vertexBufferGrowable_->Create(usage, pool), L"VB_Growable");
		AssertBuffer(indexBufferGrowable_->Create(usage, pool), L"IB_Growable");
		AssertBuffer(vertexBuffer_HWInstancing_->Create(usage, pool), L"VB_InstanceHW");
	}
	void VertexBufferManager::Release() {
		for (auto& iVB : vertexBuffers_)
			iVB->Release();
		indexBuffer_->Release();

		vertexBufferGrowable_->Release();
		indexBufferGrowable_->Release();
		vertexBuffer_HWInstancing_->Release();
	}

	BufferBase<IDirect3DVertexBuffer9>* VertexBufferManager::CreateExtraVertexBuffer() {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		IDirect3DDevice9* device = graphics->GetDevice();

		unique_ptr<BufferBase<IDirect3DVertexBuffer9>> pBufferOwned;
		pBufferOwned.reset(new FixedVertexBuffer(device));

		size_t addr = (size_t)pBufferOwned.get();

		auto itr = mapExtraBuffer_Vertex_.find(addr);
		if (itr != mapExtraBuffer_Vertex_.end())
			return false;

		auto res = mapExtraBuffer_Vertex_.insert(std::make_pair(addr, std::move(pBufferOwned)));

		//HRESULT hr = pBuffer->Create(pBuffer->usage_, pBuffer->pool_);
		//AssertBuffer(hr, StringUtility::FormatToWide("ExtraBuffer%0*x", sizeof(size_t) * 2, addr));	

		return res.first->second.get();
	}
	BufferBase<IDirect3DVertexBuffer9>* VertexBufferManager::GetExtraVertexBuffer(size_t addr) {
		auto itr = mapExtraBuffer_Vertex_.find(addr);
		if (itr != mapExtraBuffer_Vertex_.end())
			return itr->second.get();
		return nullptr;
	}
	void VertexBufferManager::ReleaseExtraVertexBuffer(size_t addr) {
		auto itr = mapExtraBuffer_Vertex_.find(addr);
		if (itr != mapExtraBuffer_Vertex_.end()) {
			itr->second->Release();
			mapExtraBuffer_Vertex_.erase(itr);
		}
	}

	void VertexBufferManager::ReleaseDxResource() {
		Release();

		for (auto& [addr, pBuffer] : mapExtraBuffer_Vertex_) {
			if (pBuffer->pool_ != D3DPOOL_DEFAULT)
				continue;
			pBuffer->Release();
		}
	}
	void VertexBufferManager::RestoreDxResource() {
		CreateBuffers(DirectGraphics::GetBase()->GetDevice());

		for (auto& [addr, pBuffer] : mapExtraBuffer_Vertex_) {
			if (pBuffer->pool_ != D3DPOOL_DEFAULT)
				continue;
			HRESULT hr = pBuffer->Create(pBuffer->usage_, pBuffer->pool_);
			AssertBuffer(hr, StringUtility::FormatToWide("ExtraBuffer%0*x", sizeof(size_t) * 2, addr));
		}
	}
}
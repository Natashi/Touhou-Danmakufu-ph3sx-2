#include "source/GcLib/pch.h"

#include "VertexBuffer.hpp"
#include "DirectGraphics.hpp"

namespace directx {
	template<typename T>
	GrowableBufferBase<T>::GrowableBufferBase() {
		pDevice_ = nullptr;
		buffer_ = nullptr;
		size_ = 0U;
		stride_ = 0U;
	}
	template<typename T>
	GrowableBufferBase<T>::GrowableBufferBase(IDirect3DDevice9* device) {
		pDevice_ = device;
		buffer_ = nullptr;
		size_ = 0U;
		stride_ = 0U;
	}
	template<typename T>
	GrowableBufferBase<T>::~GrowableBufferBase() {
		Release();
	}

	GrowableVertexBuffer::GrowableVertexBuffer(IDirect3DDevice9* device) {
		pDevice_ = device;
		fvf_ = 0U;
	}
	GrowableVertexBuffer::~GrowableVertexBuffer() {
	}
	void GrowableVertexBuffer::Setup(size_t iniSize, size_t stride, DWORD fvf) {
		fvf_ = fvf;
		stride_ = stride;
		size_ = iniSize;
	}
	void GrowableVertexBuffer::Create() {
		this->Release();
		pDevice_->CreateVertexBuffer(size_ * stride_, D3DUSAGE_DYNAMIC, fvf_,
			D3DPOOL_DEFAULT, &buffer_, nullptr);
	}
	void GrowableVertexBuffer::Expand(size_t newSize) {
		if (size_ >= newSize) return;
		while (size_ < newSize) size_ *= 2U;
		Create();
	}

	GrowableIndexBuffer::GrowableIndexBuffer(IDirect3DDevice9* device) {
		pDevice_ = device;
		format_ = D3DFMT_INDEX16;
	}
	GrowableIndexBuffer::~GrowableIndexBuffer() {
	}
	void GrowableIndexBuffer::Setup(size_t iniSize, size_t stride, D3DFORMAT format) {
		format_ = format;
		stride_ = stride;
		size_ = iniSize;
	}
	void GrowableIndexBuffer::Create() {
		this->Release();
		pDevice_->CreateIndexBuffer(size_ * stride_, D3DUSAGE_DYNAMIC,
			format_, D3DPOOL_DEFAULT, &buffer_, nullptr);
	}
	void GrowableIndexBuffer::Expand(size_t newSize) {
		if (size_ >= newSize) return;
		while (size_ < newSize) size_ *= 2U;
		Create();
	}

	VertexBufferManager* VertexBufferManager::thisBase_ = nullptr;
	VertexBufferManager::VertexBufferManager() {
		indexBuffer_ = nullptr;
		vertexBufferGrowable_ = nullptr;
		indexBufferGrowable_ = nullptr;
	}
	VertexBufferManager::~VertexBufferManager() {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		graphics->RemoveDirectGraphicsListener(this);

		Release();

		ptr_delete(vertexBufferGrowable_);
		ptr_delete(indexBufferGrowable_);
		ptr_delete(vertexBuffer_HWInstancing_);
	}

	bool VertexBufferManager::Initialize(DirectGraphics* graphics) {
		if (thisBase_) return false;

		graphics->AddDirectGraphicsListener(this);
		
		IDirect3DDevice9* device = graphics->GetDevice();

		vertexBufferGrowable_ = new GrowableVertexBuffer(device);
		indexBufferGrowable_ = new GrowableIndexBuffer(device);
		vertexBufferGrowable_->Setup(8192U, sizeof(VERTEX_TLX), VERTEX_TLX::fvf);
		indexBufferGrowable_->Setup(8192U, sizeof(uint32_t), D3DFMT_INDEX32);

		vertexBuffer_HWInstancing_ = new GrowableVertexBuffer(device);
		vertexBuffer_HWInstancing_->Setup(512U, sizeof(VERTEX_INSTANCE), 0);

		CreateBuffers(device);

		thisBase_ = this;

		return true;
	}
	void VertexBufferManager::CreateBuffers(IDirect3DDevice9* device) {
		const DWORD fvfs[] = {
			VERTEX_TLX::fvf,
			VERTEX_NX::fvf,
			VERTEX_LX::fvf
		};
		const size_t sizes[] = {
			sizeof(VERTEX_TLX),
			sizeof(VERTEX_LX),
			sizeof(VERTEX_NX)
		};

		for (size_t i = 0; i < 3; ++i) {
			IDirect3DVertexBuffer9* buffer = nullptr;

			device->CreateVertexBuffer(sizes[i] * 65536U, D3DUSAGE_DYNAMIC, fvfs[i], D3DPOOL_DEFAULT, &buffer, nullptr);

			vertexBuffers_.push_back(buffer);
		}
		device->CreateIndexBuffer(sizeof(uint16_t) * 65536U, D3DUSAGE_DYNAMIC, D3DFMT_INDEX16, D3DPOOL_DEFAULT,
			&indexBuffer_, nullptr);

		vertexBufferGrowable_->Create();
		indexBufferGrowable_->Create();
		vertexBuffer_HWInstancing_->Create();
	}
	void VertexBufferManager::Release() {
		for (auto itr = vertexBuffers_.begin(); itr != vertexBuffers_.end(); ++itr)
			ptr_release(*itr);
		vertexBuffers_.clear();
		ptr_release(indexBuffer_);

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
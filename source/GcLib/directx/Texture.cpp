#include "source/GcLib/pch.h"

#include "Texture.hpp"
#include "DirectGraphics.hpp"

using namespace gstd;
using namespace directx;

//****************************************************************************
//TextureData
//****************************************************************************
TextureData::TextureData(TextureManager* manager, Type type) :
	manager_(manager), type_(type),
	ready_(false),
	resourceSize_(0),
	imageInfo({}),
	createData({}),
	pTexture_(nullptr), lpRenderSurface_(nullptr), lpRenderZ_(nullptr) {}
	
TextureData::~TextureData() {
	ptr_release(pTexture_);
	ptr_release(lpRenderSurface_);
	ptr_release(lpRenderZ_);
}

void TextureData::CalculateResourceSize() {
	resourceSize_ = 0;

	// D3DX_DEFAULT or 0 -> complete mipmap chain

	size_t width = imageInfo.Width, height = imageInfo.Height;
	
	UINT mipLevels = createData.mipmaps;
	if (mipLevels == 0 || mipLevels == D3DX_DEFAULT) {
		size_t size = std::max(width, height);
		mipLevels = 1;
		
		while (size > 1) {
			size >>= 1;
			++mipLevels;
		}
	}

	for (size_t level = 0; level < mipLevels; ++level) {
		size_t mipWidth  = std::max<size_t>(1, width  >> level);
		size_t mipHeight = std::max<size_t>(1, height >> level);

		resourceSize_ += GetSurfaceSize(mipWidth, mipHeight, imageInfo.Format);
	}
}

size_t TextureData::GetFormatBPP(D3DFORMAT format) {
	switch (format) {
	case D3DFMT_A16B16G16R16:
	case D3DFMT_Q16W16V16U16:
		return 8;
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
	case D3DFMT_A8B8G8R8:
	case D3DFMT_X8B8G8R8:
	case D3DFMT_A2R10G10B10:
	case D3DFMT_A2B10G10R10:
	case D3DFMT_G16R16:
	case D3DFMT_R32F:
	case D3DFMT_D32:
	case D3DFMT_D24X8:
	case D3DFMT_D24S8:
		return 4;
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:
	case D3DFMT_A4R4G4B4:
	case D3DFMT_A8L8:
	case D3DFMT_V16U16:
	case D3DFMT_G16R16F:
	case D3DFMT_D16:
	case D3DFMT_L16:
	case D3DFMT_R16F:
		return 2;
	case D3DFMT_L8:
	case D3DFMT_A8:
	case D3DFMT_R3G3B2:
		return 1;
	default:
		return 0;
	}
}
size_t TextureData::GetSurfaceSize(size_t width, size_t height, D3DFORMAT format) {
	size_t bpp = GetFormatBPP(format);
	if (bpp == 0) {
		switch (format) {
		case D3DFMT_DXT1:
			return ((width + 3) / 4) * ((height + 3) / 4) * 8;
		case D3DFMT_DXT3:
		case D3DFMT_DXT5:
			return ((width + 3) / 4) * ((height + 3) / 4) * 16;
		default:
			return 0;
		}
	}
	else {
		return width * height * bpp;
	}
}

//****************************************************************************
//Texture
//****************************************************************************
Texture::Texture(const shared_ptr<TextureData>& data) : data_(data) {}

Texture::~Texture() {
	Release();
}

void Texture::Release() {
	if (auto manager = TextureManager::GetBase()) {
		Lock lock(manager->GetLock());

		// If no other uses than in data_ and in manager, dispose data
		if (data_ && data_.use_count() <= 2) {
			manager->ReleaseTextureData(data_->name);
		}
	}

	// if manager has already been destroyed, just drop local ref
	data_ = nullptr;
}

std::wstring Texture::GetName() const {
	return data_ ? data_->name : L"";
}

void Texture::SetTextureData(const shared_ptr<TextureData>& data) {
	Release();
	data_ = data;
}

IDirect3DTexture9* Texture::GetD3DTexture() {
	if (data_ == nullptr) {
		return nullptr;
	}
	else if (data_->ready_) {
		return data_->GetD3DTexture();
	}
	else {
		Lock lock(TextureManager::GetBase()->GetLock());

		auto name = PathProperty::ReduceModuleDirectory(data_->name);

		Logger::WriteWarn(STR_FMT(
			L"Texture not loaded yet, waiting... (%s)",
			name.c_str()));
	
		// TODO: see if there is a better way than spin-sleep waiting
	
		uint64_t timeOrg = SystemUtility::GetCpuTime2();
		while (data_) {
			if (data_->ready_) {
				return data_->GetD3DTexture();
			}
			else if (SystemUtility::GetCpuTime2() - timeOrg > 500) {	// 0.5s timeout
				Logger::WriteError(STR_FMT(
					L"Texture wait timed out (%s)",
					name.c_str()));
			
				return nullptr;
			}
		
			Sleep(10);
		}
		return nullptr;
	}
}
IDirect3DSurface9* Texture::GetD3DSurface() {
	IDirect3DSurface9* res = nullptr;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_) 
			res = data_->GetD3DSurface();
	}
	return res;
}
IDirect3DSurface9* Texture::GetD3DZBuffer() {
	IDirect3DSurface9* res = nullptr;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_)
			res = data_->GetD3DZBuffer();
	}
	return res;
}

UINT Texture::GetWidth() {
	UINT res = 0U;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_)
			res = data_->imageInfo.Width;
	}
	return res;
}
UINT Texture::GetHeight() {
	UINT res = 0U;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_)
			res = data_->imageInfo.Height;
	}
	return res;
}
TextureData::Type Texture::GetType() {
	TextureData::Type res = TextureData::Type::Texture;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_)
			res = data_->type_;
	}
	return res;
}

//****************************************************************************
//TextureManager
//****************************************************************************
TextureManager::~TextureManager() {
	mapTexture_.clear();
	mapTextureData_.clear();
	
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->RemoveDirectGraphicsListener(this);

	FileManager::GetBase()->RemoveLoadThreadListener(this);

	thisBase_ = nullptr;
}

bool TextureManager::Initialize() {
	if (thisBase_)
		return false;
	thisBase_ = this;
	
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->AddDirectGraphicsListener(this);

	auto texTransition = CreateRenderTarget(TARGET_TRANSITION, {});
	Add(TARGET_TRANSITION, texTransition);

	FileManager::GetBase()->AddLoadThreadListener(this);

	return texTransition.get() != nullptr;
}

void TextureManager::Clear() {
	Lock lock(lock_);

	mapTexture_.clear();
	mapTextureData_.clear();
}

void TextureManager::ReleaseDxResource() {
	auto graphics = DirectGraphics::GetBase();
	auto device = graphics->GetDevice();

	HRESULT deviceHr = graphics->GetDeviceStatus();
	if (deviceHr != D3DERR_DEVICELOST) {
		Lock lock(lock_);

		for (auto& [name, data] : mapTextureData_) {
			if (data->type_ == TextureData::Type::RenderTarget) {
				// TODO: Figure out a way to actually restore lost render target data
				//       GetRenderTargetData just returns failure as the device is already lost at this point

				/*
				// IDirect3DDevice9::Reset requires all D3DPOOL_DEFAULT resources to be released
				// Releasing render targets causes the surface data to be lost
				//    so this is used to restore original texture data when they are restored

				IDirect3DSurface9* pSurfaceCopy = nullptr;
				HRESULT hr = device->CreateOffscreenPlainSurface(infoImage->Width, infoImage->Height, infoImage->Format,
					D3DPOOL_SYSTEMMEM, &pSurfaceCopy, nullptr);
				if (SUCCEEDED(hr)) {
					hr = device->GetRenderTargetData(data->lpRenderSurface_, pSurfaceCopy);
					if (SUCCEEDED(hr)) {
						listRefreshSurface_[name] = { data, pSurfaceCopy };
					}
					else {
						std::wstring err = StringUtility::Format(L"TextureManager::ReleaseDxResource: "
							"Failed to create temporary surface [%s]\r\n    %s: %s",
							PathProperty::ReduceModuleDirectory(name).c_str(),
							DXGetErrorString(hr), DXGetErrorDescription(hr));
						Logger::WriteError(err);

						pSurfaceCopy->Release();
					}
				}
				*/

				ptr_release(data->pTexture_);
				ptr_release(data->lpRenderSurface_);
				ptr_release(data->lpRenderZ_);
			}
		}
	}
	else {
		Logger::WriteError(STR_FMT(
			L"TextureManager::ReleaseDxResource: "
			"D3D device abnormal. Render target surfaces cannot be saved.\r\n    %s: %s",
			DXGetErrorString(deviceHr), DXGetErrorDescription(deviceHr)));
	}
}
void TextureManager::RestoreDxResource() {
	auto graphics = DirectGraphics::GetBase();

	{
		Lock lock(lock_);

		for (auto& [name, data] : mapTextureData_) {
			if (data->type_ == TextureData::Type::RenderTarget) {
				UINT width = data->imageInfo.Width;
				UINT height = data->imageInfo.Height;

				D3DMULTISAMPLE_TYPE typeSample = graphics->GetMultiSampleType();

				HRESULT hr = graphics->GetDevice()->CreateTexture(
					width, height, 1, D3DUSAGE_RENDERTARGET, 
					data->imageInfo.Format, D3DPOOL_DEFAULT, &data->pTexture_, nullptr);
				if (FAILED(hr)) {
					auto err = STR_FMT(
						L"TextureManager::RestoreDxResource: Failed to restore texture for \"%s\" [%s]\n\t%s",
						name.c_str(), DXGetErrorString(hr), DXGetErrorDescription(hr));
					throw wexception(err);
				}

				hr = data->pTexture_->GetSurfaceLevel(0, &data->lpRenderSurface_);
				if (FAILED(hr)) {
					auto err = STR_FMT(
						L"TextureManager::RestoreDxResource: Failed to restore surface for \"%s\" [%s]\n\t%s",
						name.c_str(), DXGetErrorString(hr), DXGetErrorDescription(hr));
					throw wexception(err);
				}

				hr = graphics->GetDevice()->CreateDepthStencilSurface(
					width, height, D3DFMT_D16, typeSample,
					0, FALSE, &data->lpRenderZ_, nullptr);
				if (FAILED(hr)) {
					auto err = STR_FMT(
						L"TextureManager::RestoreDxResource: Failed to restore depth stencil for \"%s\" [%s]\n\t%s",
						name.c_str(), DXGetErrorString(hr), DXGetErrorDescription(hr));
					throw wexception(err);
				}
			}
		}

		/*
		for (auto& [name, data] : listRefreshSurface_) {
			auto& [textureData, surfaceData] = data;

			D3DXIMAGE_INFO* info = textureData->GetImageInfo();

			IDirect3DSurface9* surfaceDst = textureData->lpRenderSurface_;
			if (surfaceData == nullptr)
				continue;

			HRESULT hr = graphics->GetDevice()->UpdateSurface(surfaceData, nullptr, surfaceDst, nullptr);
			if (FAILED(hr)) {
				std::wstring err = StringUtility::Format(L"TextureManager::RestoreDxResource: "
					"Render target restoration failed [%s]\r\n    %s: %s",
					PathProperty::ReduceModuleDirectory(name).c_str(),
					DXGetErrorString(hr), DXGetErrorDescription(hr));
				Logger::WriteError(err);
			}

			ptr_release(surfaceData);
		}
		listRefreshSurface_.clear();
		*/
	}
}

void TextureManager::ReleaseTextureData(const std::wstring& name) {
	{
		Lock lock(lock_);

		auto itr = mapTextureData_.find(name);
		if (itr == mapTextureData_.end())
			return;

		auto& data = itr->second;
		data->ready_ = true;

		mapTextureData_.erase(itr);
	}

	{
		Lock lock(Logger::GetTop()->GetLock());
		
		Logger::WriteTop(STR_FMT(
		   L"TextureManager: Texture released. [%s]", 
		   PathProperty::ReduceModuleDirectory(name).c_str()));
	}
}

shared_ptr<TextureData> TextureManager::CreateDataFromFile(const std::wstring& path, const CreateTextureData& params) {
	auto pathReduce = PathProperty::ReduceModuleDirectory(path);
	
	try {
		auto reader = FileManager::GetBase()->GetFileReader(path);
		if (reader == nullptr || !reader->Open())
			throw wexception(ErrorUtility::GetFileNotFoundErrorMessage(
				PathProperty::ReduceModuleDirectory(path), true));

		auto source = reader->ReadToString();

		auto res = make_shared<TextureData>(this, TextureData::Type::Texture);
		
		res->name = path;
		res->createData = params;

		{
			Lock lock(lock_);
			
			HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(DirectGraphics::GetBase()->GetDevice(),
			   source.c_str(), source.size(),
			   params.sizeType, params.sizeType,
			   params.mipmaps, 0,
			   D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
			   params.textureFilter, params.mipmapFilter,
			   params.colorKey,
			   nullptr, nullptr,
			   &res->pTexture_);
			if (FAILED(hr))
				throw wexception("D3DXCreateTextureFromFileInMemoryEx failure.");

			hr = D3DXGetImageInfoFromFileInMemory(source.c_str(), source.size(), &res->imageInfo);
			if (FAILED(hr))
				throw wexception("D3DXGetImageInfoFromFileInMemory failure.");

			UINT mipLevels = res->pTexture_->GetLevelCount();
			res->createData.mipmaps = mipLevels;

			res->CalculateResourceSize();
			
			res->ready_ = true;

			mapTextureData_[path] = res;
		}

		{
			Lock lock(Logger::GetTop()->GetLock());
			
			Logger::WriteTop(STR_FMT(
				L"TextureManager: Texture loaded. [%s]",
				pathReduce.c_str()));
		}

		return res;
	}
	catch (wexception& e) {
		Lock lock(Logger::GetTop()->GetLock());
		
		Logger::WriteError(STR_FMT(
			L"TextureManager: Failed to load texture \"%s\"\r\n    %s", 
			pathReduce.c_str(), e.what()));

		return nullptr;
	}
}

shared_ptr<TextureData> TextureManager::CreateDataRenderTarget(const std::wstring& name, const CreateTextureData& params) {
	auto graphics = DirectGraphics::GetBase();
	auto device = graphics->GetDevice();

	try {
		auto width = params.renderTargetWidth, height = params.renderTargetHeight;
		
		if (width == 0U) {
			size_t screenWidth = graphics->GetScreenWidth();
			width = Math::GetNextPow2(screenWidth);
		}
		if (height == 0U) {
			size_t screenHeight = graphics->GetScreenHeight();
			height = Math::GetNextPow2(screenHeight);
		}
		{
			size_t maxWidth = std::min<DWORD>(graphics->GetDeviceCaps()->MaxTextureWidth, 16384);
			size_t maxHeight = std::min<DWORD>(graphics->GetDeviceCaps()->MaxTextureHeight, 16384);
			width = std::min(width, maxWidth);
			height = std::min(height, maxHeight);
		}

		auto res = make_shared<TextureData>(this, TextureData::Type::RenderTarget);

		D3DMULTISAMPLE_TYPE typeSample = graphics->GetMultiSampleType();
		ColorMode colorMode = graphics->GetGraphicsConfig().colorMode;
		D3DFORMAT fmt = colorMode == ColorMode::COLOR_MODE_32BIT ?
			D3DFMT_A8R8G8B8 : D3DFMT_A4R4G4B4;

		{
			Lock lock(lock_);
			
			HRESULT hr = device->CreateTexture(
			   width, height, 1, D3DUSAGE_RENDERTARGET, fmt, D3DPOOL_DEFAULT,
			   &res->pTexture_, nullptr);
			if (FAILED(hr))
				throw wexception("CreateTexture failure.");

			hr = res->pTexture_->GetSurfaceLevel(0, &res->lpRenderSurface_);
			if (FAILED(hr))
				throw wexception("GetSurfaceLevel failure.");

			hr = device->CreateDepthStencilSurface(
				width, height, D3DFMT_D16, typeSample,
				0, FALSE, &res->lpRenderZ_, nullptr);
			if (FAILED(hr))
				throw wexception("CreateDepthStencilSurface failure.");

			res->name = name;
	
			res->createData = params;
	
			res->imageInfo.Width = width;
			res->imageInfo.Height = height;
			res->imageInfo.Format = fmt;

			res->ready_ = true;
	
			res->CalculateResourceSize();

			mapTextureData_[name] = res;
		}

		{
			Lock lock(Logger::GetTop()->GetLock());
		
			Logger::WriteTop(STR_FMT(
				L"TextureManager: Render target created. [%s]",
				name.c_str()));
		}

		return res;
	}
	catch (wexception& e) {
		Lock lock(Logger::GetTop()->GetLock());
		
		Logger::WriteError(STR_FMT(
			L"Failed to create render target \"%s\"\r\n    %s", 
			name.c_str(), e.what()));

		return nullptr;
	}
}

shared_ptr<Texture> TextureManager::CreateFromFile(const std::wstring& path, const CreateTextureData& params) {
	auto itr = mapTexture_.find(path);
	if (itr != mapTexture_.end()) {
		return itr->second;
	}
	else {
		shared_ptr<TextureData> data;

		auto itrFind = mapTextureData_.find(path);
		if (itrFind != mapTextureData_.end()) {
			data = itrFind->second;
		}
		else {
			data = CreateDataFromFile(path, params);
		}
		
		return data ? make_shared<Texture>(data) : nullptr;
	}
}

shared_ptr<Texture> TextureManager::CreateRenderTarget(const std::wstring& name, const CreateTextureData& params) {
	auto itr = mapTexture_.find(name);
	if (itr != mapTexture_.end()) {
		return itr->second;
	}
	else {
		shared_ptr<TextureData> data;

		auto itrFind = mapTextureData_.find(name);
		if (itrFind != mapTextureData_.end()) {
			data = itrFind->second;
		}
		else {
			data = CreateDataRenderTarget(name, params);
		}
		
		return data ? make_shared<Texture>(data) : nullptr;
	}
}

shared_ptr<Texture> TextureManager::CreateFromData(const shared_ptr<TextureData>& data) {
	return make_shared<Texture>(data);
}

shared_ptr<Texture> TextureManager::CreateFromD3DTexture(IDirect3DTexture9* pTexture, TextureData::Type type) {
	auto data = make_shared<TextureData>(this, type);

	D3DSURFACE_DESC desc;
	{
		Lock lock(lock_);
		
		pTexture->GetLevelDesc(0, &desc);
	}

	data->imageInfo.Width = desc.Width;
	data->imageInfo.Height = desc.Height;
	data->imageInfo.Format = desc.Format;
	data->imageInfo.ImageFileFormat = D3DXIFF_BMP;
	data->imageInfo.ResourceType = D3DRTYPE_TEXTURE;

	data->pTexture_ = pTexture;

	data->CalculateResourceSize();

	data->ready_ = true;
	
	return make_shared<Texture>(data);
}

shared_ptr<Texture> TextureManager::CreateFromFileInLoadThread(
	const std::wstring& path, 
	const CreateTextureData& params, bool loadImageInfoNow) 
{
	auto itr = mapTexture_.find(path);
	if (itr != mapTexture_.end()) {
		return itr->second;
	}
	else {
		auto itrFind = mapTextureData_.find(path);
		if (itrFind != mapTextureData_.end()) {
			auto& data = itrFind->second;
			return data ? make_shared<Texture>(data) : nullptr;
		}
		else {
			auto pathReduce = PathProperty::ReduceModuleDirectory(path);

			auto data = make_shared<TextureData>(this, TextureData::Type::Texture);
			
			data->name = path;
			data->createData = params;

			if (loadImageInfoNow) {
				try {
					auto reader = FileManager::GetBase()->GetFileReader(path);
					if (reader == nullptr || !reader->Open())
						throw wexception(ErrorUtility::GetFileNotFoundErrorMessage(
							PathProperty::ReduceModuleDirectory(path), true));

					auto source = reader->ReadToString();

					{
						Lock lock(lock_);
						
						HRESULT hr = D3DXGetImageInfoFromFileInMemory(
							source.c_str(), source.size(), &data->imageInfo);
						if (FAILED(hr))
							throw wexception("D3DXGetImageInfoFromFileInMemory failure.");

						data->CalculateResourceSize();
					}
				}
				catch (wexception& e) {
					Lock lock(Logger::GetTop()->GetLock());
					
					Logger::WriteError(STR_FMT(
						L"TextureManager: Failed to load texture info \"%s\"\r\n    %s", 
						pathReduce.c_str(), e.what()));
				}
			}

			{
				Lock lock(lock_);
				
				mapTextureData_[path] = data;
			}

			auto res = make_shared<Texture>(data);
			
			{
				Lock lockFile(FileManager::GetBase()->GetLock());
				
				auto event = make_shared<FileManager::LoadThreadEvent>(
				   this, path,
				   std::dynamic_pointer_cast<FileManager::LoadObject>(res));
				FileManager::GetBase()->AddLoadThreadEvent(event);
			}

			return res;
		}
	}
}
void TextureManager::CallFromLoadThread(shared_ptr<FileManager::LoadThreadEvent> event) {
	auto& path = event->GetPath();
	
	auto texture = std::dynamic_pointer_cast<Texture>(event->GetSource());
	if (texture == nullptr)
		return;

	auto data = texture->data_;
	if (data == nullptr || data->ready_)
		return;

	if (data.use_count() <= 2) {
		data->ready_ = true;
		return;
	}

	auto pathReduce = PathProperty::ReduceModuleDirectory(path);
	
	try {
		auto reader = FileManager::GetBase()->GetFileReader(path);
		if (reader == nullptr || !reader->Open())
			throw wexception(ErrorUtility::GetFileNotFoundErrorMessage(
				PathProperty::ReduceModuleDirectory(path), true));

		auto source = reader->ReadToString();

		{
			Lock lock(lock_);

			auto& params = data->createData;
			
			HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(DirectGraphics::GetBase()->GetDevice(),
				source.c_str(), source.size(),
				params.sizeType, params.sizeType,
				params.mipmaps, 0,
				D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
				params.textureFilter, params.mipmapFilter,
				params.colorKey,
				nullptr, nullptr,
				&data->pTexture_);
			if (FAILED(hr))
				throw wexception("D3DXCreateTextureFromFileInMemoryEx failure.");

			UINT mipLevels = data->pTexture_->GetLevelCount();
			data->createData.mipmaps = mipLevels;

			// load image info if not loaded yet
			if (data->resourceSize_ == 0 || data->imageInfo.Width == 0 || data->imageInfo.Height == 0) {
				hr = D3DXGetImageInfoFromFileInMemory(source.c_str(), source.size(), &data->imageInfo);
				if (FAILED(hr))
					throw wexception("D3DXGetImageInfoFromFileInMemory failure.");
			}

			// always calculate resource size
			data->CalculateResourceSize();

			data->ready_ = true;
		}

		{
			Lock lock(Logger::GetTop()->GetLock());
				
			Logger::WriteTop(STR_FMT(
				L"TextureManager(LT): Texture loaded. [%s]",
				pathReduce.c_str()));
		}
	}
	catch (wexception& e) {
		Lock lock(Logger::GetTop()->GetLock());
		
		Logger::WriteError(STR_FMT(
			L"TextureManager(LT): Failed to load texture \"%s\"\r\n    %s", 
			pathReduce.c_str(), e.what()));
		
		// upon failure, mark as ready/loaded and remove from cache
		
		data->ready_ = true;
		texture->data_ = nullptr;
		mapTextureData_.erase(path);
	}
}

shared_ptr<TextureData> TextureManager::GetTextureData(const std::wstring& name) {
	auto itr = mapTextureData_.find(name);
	if (itr != mapTextureData_.end())
		return itr->second;
	return nullptr;
}

shared_ptr<Texture> TextureManager::GetTexture(const std::wstring& name) {
	auto itr = mapTexture_.find(name);
	if (itr != mapTexture_.end())
		return itr->second;
	return nullptr;
}

void TextureManager::Add(const std::wstring& name, shared_ptr<Texture> texture) {
	Lock lock(lock_);
	
	bool exist = mapTexture_.find(name) != mapTexture_.end();
	if (!exist) {
		mapTexture_[name] = texture;
	}
}
void TextureManager::Release(const std::wstring& name) {
	Lock lock(lock_);

	mapTexture_.erase(name);
}
shared_ptr<TextureData> TextureManager::GetData(const std::wstring& name) {
	auto res = mapTextureData_.find(name);
	if (res != mapTextureData_.end())
		return res->second;
	return nullptr;
}

//****************************************************************************
//TextureInfoPanel
//****************************************************************************
TextureInfoPanel::TextureInfoPanel() : videoMem_(0) {
}

void TextureInfoPanel::Initialize(const std::string& name) {
	ILoggerPanel::Initialize(name);
}

void TextureInfoPanel::Update() {
	TextureManager* manager = TextureManager::GetBase();
	if (manager == nullptr) {
		listDisplay_.clear();
		return;
	}

	{
		Lock lock(Logger::GetTop()->GetLock());

		listDisplay_.clear();
		for (auto& [path, data] : manager->mapTextureData_) {
			listDisplay_.push_back(TextureDisplay(data, path, &data->imageInfo));
		}

		// Sort new data as well
		if (TextureDisplay::imguiSortSpecs) {
			if (listDisplay_.size() > 1) {
				std::sort(listDisplay_.begin(), listDisplay_.end(), TextureDisplay::Compare);
			}
		}
	}

	{
		IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();

		UINT texMem = device->GetAvailableTextureMem();
		videoMem_ = texMem / (1024U * 1024U);
	}
}
void TextureInfoPanel::ProcessGui() {
	Logger* parent = Logger::GetTop();

	float ht = ImGui::GetContentRegionAvail().y - 32;

	if (ImGui::BeginChild("ptexture_child_table", ImVec2(0, ht), false, ImGuiWindowFlags_HorizontalScrollbar)) {
		ImGuiTableFlags flags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable
			| ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX 
			| ImGuiTableFlags_RowBg
			| ImGuiTableFlags_Sortable /*| ImGuiTableFlags_SortMulti*/;

		if (ImGui::BeginTable("ptexture_table", 7, flags)) {
			ImGui::TableSetupScrollFreeze(0, 1);

			constexpr auto sortDef = ImGuiTableColumnFlags_DefaultSort, 
				sortNone = ImGuiTableColumnFlags_NoSort;
			constexpr auto colFlags = ImGuiTableColumnFlags_WidthStretch;
			ImGui::TableSetupColumn("Address", sortDef, 0, TextureDisplay::Address);
			ImGui::TableSetupColumn("Name", colFlags | sortDef, 160, TextureDisplay::Name);
			ImGui::TableSetupColumn("Path", colFlags | sortDef, 200, TextureDisplay::FullPath);
			ImGui::TableSetupColumn("Uses", sortDef, 0, TextureDisplay::Uses);
			ImGui::TableSetupColumn("Width", sortNone, 0, TextureDisplay::_NoSort);
			ImGui::TableSetupColumn("Height", sortNone, 0, TextureDisplay::_NoSort);
			ImGui::TableSetupColumn("Size", colFlags | sortDef, 100, TextureDisplay::Size);

			ImGui::TableHeadersRow();

			if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
				if (specs->SpecsDirty) {
					TextureDisplay::imguiSortSpecs = specs;
					if (listDisplay_.size() > 1) {
						std::sort(listDisplay_.begin(), listDisplay_.end(), TextureDisplay::Compare);
					}
					//TextureDisplay::imguiSortSpecs = nullptr;

					specs->SpecsDirty = false;
				}
			}

			{
				ImGui::PushFont(parent->GetFont("Arial15"));

#define _SETCOL(_i, _s) ImGui::TableSetColumnIndex(_i); ImGui::Text((_s).c_str());

				ImGuiListClipper clipper;
				clipper.Begin(listDisplay_.size());
				while (clipper.Step()) {
					for (size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
						const TextureDisplay& item = listDisplay_[i];

						ImGui::TableNextRow();

						SetCol(0, item.strAddress);

						SetCol(1, item.fileName);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(item.fileName.c_str());

						SetCol(2, item.fullPath);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(item.fullPath.c_str());

						SetCol(3, std::to_string(item.countRef));
						SetCol(4, std::to_string(item.wd));
						SetCol(5, std::to_string(item.ht));
						SetCol(6, std::to_string(item.size));
					}
				}

				ImGui::PopFont();
			}

			ImGui::EndTable();
		}
	}
	ImGui::EndChild();

	ImGui::Dummy(ImVec2(0, 4));
	ImGui::Indent(6);

	{
		ImGui::Text("Available Video Memory: %u MB", videoMem_);
	}
}

// --------------------------------------------------------------------------------------------

const ImGuiTableSortSpecs* TextureInfoPanel::TextureDisplay::imguiSortSpecs = nullptr;

TextureInfoPanel::TextureDisplay::TextureDisplay(
	const shared_ptr<TextureData>& data, const std::wstring& path, D3DXIMAGE_INFO* infoImage)
{
	address = (uintptr_t)data.get();
	strAddress = StringUtility::FromAddress(address);

	fileName = STR_MULTI(PathProperty::GetFileName(path));
	fullPath = STR_MULTI(PathProperty::ReduceModuleDirectory(path));

	countRef = data.use_count();

	wd = infoImage->Width;
	ht = infoImage->Height;
	size = data->GetResourceSize();

	dataRef = data;
	textureType = data->type_;
}

bool TextureInfoPanel::TextureDisplay::Compare(const TextureDisplay& a, const TextureDisplay& b) {
	for (int i = 0; i < imguiSortSpecs->SpecsCount; ++i) {
		const ImGuiTableColumnSortSpecs* spec = &imguiSortSpecs->Specs[i];

		int rcmp = 0;

#define CASE_SORT(_id, _l, _r) \
		case _id: \
			if ((_l) != (_r)) { rcmp = ((_l) < (_r)) ? 1 : -1; } break; \

		switch ((Column)spec->ColumnUserID) {
		CASE_SORT(Column::Address, a.address, b.address);
		CASE_SORT(Column::Uses, a.countRef, b.countRef);
		CASE_SORT(Column::Size, a.size, b.size);
		case Column::Name:
			rcmp = a.fileName.compare(b.fileName);
			break;
		case Column::FullPath:
			rcmp = a.fullPath.compare(b.fullPath);
			break;
		default: break;
		}

#undef CASE_SORT

		if (rcmp != 0) {
			return spec->SortDirection == ImGuiSortDirection_Ascending
				? rcmp < 0 : rcmp > 0;
		}
	}

	return a.address < b.address;
}

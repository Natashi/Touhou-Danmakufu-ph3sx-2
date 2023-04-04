#include "source/GcLib/pch.h"

#include "Texture.hpp"
#include "DirectGraphics.hpp"

using namespace gstd;
using namespace directx;

//****************************************************************************
//TextureData
//****************************************************************************
TextureData::TextureData() {
	manager_ = nullptr;

	pTexture_ = nullptr;
	lpRenderSurface_ = nullptr;
	lpRenderZ_ = nullptr;

	bReady_ = true;

	useMipMap_ = false;
	useNonPowerOfTwo_ = false;

	resourceSize_ = 0U;

	ZeroMemory(&infoImage_, sizeof(D3DXIMAGE_INFO));
	type_ = Type::TYPE_TEXTURE;
}
TextureData::~TextureData() {
	ptr_release(pTexture_);
	ptr_release(lpRenderSurface_);
	ptr_release(lpRenderZ_);
}
void TextureData::CalculateResourceSize() {
	size_t size = infoImage_.Width * infoImage_.Height;
	if (useMipMap_) {
		UINT wd = infoImage_.Width;
		UINT ht = infoImage_.Height;
		while (wd > 1U && ht > 1U) {
			wd /= 2U;
			ht /= 2U;
			size += wd * ht;
		}
	}
	resourceSize_ = size * Texture::GetFormatBPP(infoImage_.Format);
}

//****************************************************************************
//Texture
//****************************************************************************
Texture::Texture() {
}
Texture::Texture(Texture* texture) {
	{
		Lock lock(TextureManager::GetBase()->GetLock());

		data_ = texture->data_;
	}
}
Texture::~Texture() {
	Release();
}
void Texture::Release() {
	{
		Lock lock(TextureManager::GetBase()->GetLock());

		if (data_) {
			TextureManager* manager = data_->manager_;
			if (manager) {
				auto itrData = manager->IsDataExistsItr(data_->name_);

				//No other references other than the one here and the one in TextureManager
				if (data_.use_count() <= 2)
					manager->_ReleaseTextureData(itrData);
			}
			data_ = nullptr;
		}
	}
}
std::wstring Texture::GetName() {
	std::wstring res = L"";
	if (data_) res = data_->GetName();
	return res;
}

bool Texture::CreateFromData(const std::wstring& name) {
	if (data_) Release();

	TextureManager* manager = TextureManager::GetBase();
	auto itrData = manager->IsDataExistsItr(name);

	if (itrData != manager->mapTextureData_.end())
		data_ = itrData->second;
	return data_ != nullptr;
}
bool Texture::CreateFromData(shared_ptr<TextureData> data) {
	if (data_) Release();
	if (data) data_ = data;
	return data_ != nullptr;
}
bool Texture::CreateFromFile(const std::wstring& path, bool genMipmap, bool flgNonPowerOfTwo) {
	//path = PathProperty::GetUnique(path);
	if (data_) Release();

	TextureManager* manager = TextureManager::GetBase();
	shared_ptr<Texture> texture = manager->CreateFromFile(path, genMipmap, flgNonPowerOfTwo);
	if (texture) data_ = texture->data_;
	return data_ != nullptr;
}
bool Texture::CreateRenderTarget(const std::wstring& name, size_t width, size_t height) {
	if (data_) Release();

	TextureManager* manager = TextureManager::GetBase();
	shared_ptr<Texture> texture = manager->CreateRenderTarget(name, width, height);
	if (texture) data_ = texture->data_;
	return data_ != nullptr;
}
bool Texture::CreateFromFileInLoadThread(const std::wstring& path, bool genMipmap, bool flgNonPowerOfTwo, bool bLoadImageInfo) {
	//path = PathProperty::GetUnique(path);

	if (data_) Release();

	TextureManager* manager = TextureManager::GetBase();
	shared_ptr<Texture> texture = manager->CreateFromFileInLoadThread(path, bLoadImageInfo, genMipmap, flgNonPowerOfTwo);
	if (texture)
		data_ = texture->data_;
	return data_ != nullptr;
}
void Texture::SetTexture(IDirect3DTexture9* pTexture) {
	if (data_) Release();

	TextureData* textureData = new TextureData();
	textureData->pTexture_ = pTexture;
	D3DSURFACE_DESC desc;
	pTexture->GetLevelDesc(0, &desc);

	D3DXIMAGE_INFO* infoImage = &textureData->infoImage_;
	infoImage->Width = desc.Width;
	infoImage->Height = desc.Height;
	infoImage->Format = desc.Format;
	infoImage->ImageFileFormat = D3DXIFF_BMP;
	infoImage->ResourceType = D3DRTYPE_TEXTURE;

	data_ = shared_ptr<TextureData>(textureData);
}

IDirect3DTexture9* Texture::GetD3DTexture() {
	IDirect3DTexture9* res = nullptr;
	if (data_) {
		Lock lock(TextureManager::GetBase()->GetLock());

		uint64_t timeOrg = SystemUtility::GetCpuTime2();
		while (true) {
			if (data_->bReady_) {
				res = data_->GetD3DTexture();
				break;
			}
			else if (SystemUtility::GetCpuTime2() - timeOrg > 200) {		//0.2 second timer
				const std::wstring& path = data_->GetName();
				Logger::WriteTop(StringUtility::Format(L"GetTexture timed out. (%s)", 
					PathProperty::ReduceModuleDirectory(path).c_str()));
				break;
			}
			::Sleep(10);
		}
	}
	return res;
}
IDirect3DSurface9* Texture::GetD3DSurface() {
	IDirect3DSurface9* res = nullptr;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_) res = data_->GetD3DSurface();
	}
	return res;
}
IDirect3DSurface9* Texture::GetD3DZBuffer() {
	IDirect3DSurface9* res = nullptr;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_) res = data_->GetD3DZBuffer();
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
			res = data_->infoImage_.Width;
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
			res = data_->infoImage_.Height;
	}
	return res;
}
TextureData::Type Texture::GetType() {
	TextureData::Type res = TextureData::Type::TYPE_TEXTURE;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_)
			res = data_->type_;
	}
	return res;
}
size_t Texture::GetFormatBPP(D3DFORMAT format) {
	switch (format) {
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:
	case D3DFMT_A8R3G3B2:
	case D3DFMT_X4R4G4B4:
		return 2U;
	case D3DFMT_R8G8B8:
		return 3U;
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
	case D3DFMT_A2B10G10R10:
	case D3DFMT_A8B8G8R8:
	case D3DFMT_X8B8G8R8:
	case D3DFMT_G16R16:
	case D3DFMT_A2R10G10B10:
		return 4U;
	case D3DFMT_A16B16G16R16:
		return 8U;
	case D3DFMT_R3G3B2:
	case D3DFMT_A8:
	default:
		return 1U;
	}
}

//****************************************************************************
//TextureManager
//****************************************************************************
const std::wstring TextureManager::TARGET_TRANSITION = L"__RENDERTARGET_TRANSITION__";
TextureManager* TextureManager::thisBase_ = nullptr;
TextureManager::TextureManager() {

}
TextureManager::~TextureManager() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->RemoveDirectGraphicsListener(this);
	this->Clear();

	FileManager::GetBase()->RemoveLoadThreadListener(this);

	panelInfo_ = nullptr;
	thisBase_ = nullptr;
}
bool TextureManager::Initialize() {
	if (thisBase_) return false;

	thisBase_ = this;
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->AddDirectGraphicsListener(this);

	shared_ptr<Texture> texTransition(new Texture());
	bool res = texTransition->CreateRenderTarget(TARGET_TRANSITION);
	Add(TARGET_TRANSITION, texTransition);

	FileManager::GetBase()->AddLoadThreadListener(this);

	return res;
}
void TextureManager::Clear() {
	{
		Lock lock(lock_);

		mapTexture_.clear();
		mapTextureData_.clear();
	}
}
void TextureManager::_ReleaseTextureData(const std::wstring& name) {
	{
		Lock lock(lock_);

		auto itr = mapTextureData_.find(name);
		if (itr != mapTextureData_.end()) {
			itr->second->bReady_ = true;
			mapTextureData_.erase(itr);
			Logger::WriteTop(StringUtility::Format(L"TextureManager: Texture released. [%s]", 
				PathProperty::ReduceModuleDirectory(name).c_str()));
		}
	}
}
void TextureManager::_ReleaseTextureData(std::map<std::wstring, shared_ptr<TextureData>>::iterator itr) {
	{
		Lock lock(lock_);

		if (itr != mapTextureData_.end()) {
			const std::wstring& name = itr->second->name_;
			itr->second->bReady_ = true;
			mapTextureData_.erase(itr);
			Logger::WriteTop(StringUtility::Format(L"TextureManager: Texture released. [%s]", 
				PathProperty::ReduceModuleDirectory(name).c_str()));
		}
	}
}
void TextureManager::ReleaseDxResource() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	HRESULT deviceHr = graphics->GetDeviceStatus();
	if (deviceHr != D3DERR_DEVICELOST) {
		Lock lock(GetLock());

		for (auto itrMap = mapTextureData_.begin(); itrMap != mapTextureData_.end(); ++itrMap) {
			TextureData* data = (itrMap->second).get();

			if (data->type_ == TextureData::Type::TYPE_RENDER_TARGET) {
				D3DXIMAGE_INFO* infoImage = data->GetImageInfo();

				//Because IDirect3DDevice9::Reset requires me to delete all render targets, 
				//	this is used to copy back lost data when the render targets are recreated.
				IDirect3DSurface9* pSurfaceCopy = nullptr;
				HRESULT hr = device->CreateOffscreenPlainSurface(infoImage->Width, infoImage->Height, infoImage->Format,
					D3DPOOL_SYSTEMMEM, &pSurfaceCopy, nullptr);
				if (SUCCEEDED(hr)) {
					hr = device->GetRenderTargetData(data->lpRenderSurface_, pSurfaceCopy);
					if (SUCCEEDED(hr)) {
						listRefreshSurface_.push_back(std::make_pair(itrMap, pSurfaceCopy));
					}
					else {
						std::wstring err = StringUtility::Format(L"TextureManager::ReleaseDxResource: "
							"Failed to create temporary surface [%s]\r\n    %s: %s",
							PathProperty::ReduceModuleDirectory(itrMap->first).c_str(), 
							DXGetErrorString(hr), DXGetErrorDescription(hr));
						Logger::WriteTop(err);

						pSurfaceCopy->Release();
					}
				}

				ptr_release(data->pTexture_);
				ptr_release(data->lpRenderSurface_);
				ptr_release(data->lpRenderZ_);
			}
		}
	}
	else {
		std::wstring err = StringUtility::Format(L"TextureManager::ReleaseDxResource: "
			"D3D device abnormal. Render target surfaces cannot be saved.\r\n    %s: %s",
			DXGetErrorString(deviceHr), DXGetErrorDescription(deviceHr));
		Logger::WriteTop(err);
	}
}
void TextureManager::RestoreDxResource() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	std::map<std::wstring, shared_ptr<TextureData>>::iterator itrMap;
	{
		Lock lock(GetLock());

		for (itrMap = mapTextureData_.begin(); itrMap != mapTextureData_.end(); ++itrMap) {
			TextureData* data = (itrMap->second).get();

			if (data->type_ == TextureData::Type::TYPE_RENDER_TARGET) {
				UINT width = data->infoImage_.Width;
				UINT height = data->infoImage_.Height;

				D3DMULTISAMPLE_TYPE typeSample = graphics->GetMultiSampleType();

				HRESULT hr;
				hr = graphics->GetDevice()->CreateDepthStencilSurface(width, height, D3DFMT_D16, typeSample,
					0, FALSE, &data->lpRenderZ_, nullptr);
				if (FAILED(hr)) {
					if (width > height) height = width;
					else width = height;

					hr = graphics->GetDevice()->CreateDepthStencilSurface(width, height, D3DFMT_D16, typeSample,
						0, FALSE, &data->lpRenderZ_, nullptr);
					if (FAILED(hr)) {
						std::wstring err = StringUtility::Format(L"TextureManager::RestoreDxResource: (Depth)\n%s\n  %s",
							DXGetErrorString(hr), DXGetErrorDescription(hr));
						throw wexception(err);
					}
				}

				hr = graphics->GetDevice()->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, 
					data->GetImageInfo()->Format, D3DPOOL_DEFAULT, &data->pTexture_, nullptr);
				if (FAILED(hr)) {
					std::wstring err = StringUtility::Format(L"TextureManager::RestoreDxResource: (Texture)\n%s\n  %s",
						DXGetErrorString(hr), DXGetErrorDescription(hr));
					throw wexception(err);
				}
				data->pTexture_->GetSurfaceLevel(0, &data->lpRenderSurface_);
			}
		}

		for (auto itrSurface = listRefreshSurface_.begin(); itrSurface != listRefreshSurface_.end(); ++itrSurface) {
			shared_ptr<TextureData> data = itrSurface->first->second;
			D3DXIMAGE_INFO* info = data->GetImageInfo();

			IDirect3DSurface9* surfaceDst = data->lpRenderSurface_;
			IDirect3DSurface9*& surfaceSrc = itrSurface->second;
			if (surfaceSrc == nullptr) continue;

			HRESULT hr = graphics->GetDevice()->UpdateSurface(surfaceSrc, nullptr, surfaceDst, nullptr);
			if (FAILED(hr)) {
				std::wstring err = StringUtility::Format(L"TextureManager::RestoreDxResource: "
					"Render target restoration failed [%s]\r\n    %s: %s",
					PathProperty::ReduceModuleDirectory(data->name_).c_str(), 
					DXGetErrorString(hr), DXGetErrorDescription(hr));
				Logger::WriteTop(err);
			}

			ptr_release(surfaceSrc);
		}
		listRefreshSurface_.clear();
	}
}

void TextureManager::__CreateFromFile(shared_ptr<TextureData>& dst, const std::wstring& path, bool genMipmap, bool flgNonPowerOfTwo) {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr || !reader->Open())
		throw wexception(ErrorUtility::GetFileNotFoundErrorMessage(PathProperty::ReduceModuleDirectory(path), true));

	std::string source = reader->ReadAllString();

	dst->useMipMap_ = genMipmap;
	dst->useNonPowerOfTwo_ = flgNonPowerOfTwo;

	HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(DirectGraphics::GetBase()->GetDevice(),
		source.c_str(), source.size(),
		dst->useNonPowerOfTwo_ ? D3DX_DEFAULT_NONPOW2 : D3DX_DEFAULT,
		dst->useNonPowerOfTwo_ ? D3DX_DEFAULT_NONPOW2 : D3DX_DEFAULT,
		dst->useMipMap_ ? D3DX_DEFAULT : 1, 0,
		D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_BOX, D3DX_DEFAULT, 0x00000000,
		nullptr, nullptr, &(dst->pTexture_));
	if (FAILED(hr))
		throw wexception("D3DXCreateTextureFromFileInMemoryEx failure.");

	hr = D3DXGetImageInfoFromFileInMemory(source.c_str(), source.size(), &dst->infoImage_);
	if (FAILED(hr))
		throw wexception("D3DXGetImageInfoFromFileInMemory failure.");
	dst->CalculateResourceSize();

	dst->manager_ = this;
	dst->name_ = path;
	dst->type_ = TextureData::Type::TYPE_TEXTURE;
}
bool TextureManager::_CreateFromFile(shared_ptr<TextureData>& dst, const std::wstring& path, bool genMipmap, bool flgNonPowerOfTwo) {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	bool res = true;
	shared_ptr<TextureData> data;

	std::wstring pathReduce = PathProperty::ReduceModuleDirectory(path);
	try {
		data.reset(new TextureData());
		__CreateFromFile(data, path, genMipmap, flgNonPowerOfTwo);

		Logger::WriteTop(StringUtility::Format(L"TextureManager: Texture loaded. [%s]",
			pathReduce.c_str()));
	}
	catch (wexception& e) {
		std::wstring str = StringUtility::Format(L"TextureManager: Failed to load texture \"%s\"\r\n    %s", 
			pathReduce.c_str(), e.what());
		Logger::WriteTop(str);

		res = false;
	}

	if (res) mapTextureData_[path] = data;
	dst = data;

	return res;
}
bool TextureManager::_CreateRenderTarget(shared_ptr<TextureData>& dst, const std::wstring& name, 
	size_t width, size_t height, bool bManaged)
{
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	bool res = true;
	shared_ptr<TextureData> data;

	try {
		if (width == 0U) {
			size_t screenWidth = graphics->GetScreenWidth();
			width = Math::GetNextPow2(screenWidth);
		}
		if (height == 0U) {
			size_t screenHeight = graphics->GetScreenHeight();
			height = Math::GetNextPow2(screenHeight);
		}
		{
			size_t maxWidth = std::min<DWORD>(graphics->GetDeviceCaps()->MaxTextureWidth, 4096);
			size_t maxHeight = std::min<DWORD>(graphics->GetDeviceCaps()->MaxTextureHeight, 4096);
			if (width > maxWidth) width = maxWidth;
			if (height > maxHeight) height = maxHeight;
		}

		data.reset(new TextureData());

		D3DMULTISAMPLE_TYPE typeSample = graphics->GetMultiSampleType();

		HRESULT hr;
		hr = device->CreateDepthStencilSurface(width, height, D3DFMT_D16, typeSample,
			0, FALSE, &data->lpRenderZ_, nullptr);
		if (FAILED(hr)) throw false;

		ColorMode colorMode = graphics->GetGraphicsConfig().colorMode;
		D3DFORMAT fmt = colorMode == ColorMode::COLOR_MODE_32BIT ?
			D3DFMT_A8R8G8B8 : D3DFMT_A4R4G4B4;

		hr = device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, fmt, D3DPOOL_DEFAULT,
			&data->pTexture_, nullptr);
		if (FAILED(hr)) {
			if (width > height) height = width;
			else if (height > width) width = height;

			hr = device->CreateDepthStencilSurface(width, height, D3DFMT_D16, typeSample,
				0, FALSE, &data->lpRenderZ_, nullptr);
			if (FAILED(hr))
				throw wexception("CreateDepthStencilSurface failure.");

			hr = device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, fmt, D3DPOOL_DEFAULT,
				&data->pTexture_, nullptr);
			if (FAILED(hr))
				throw wexception("CreateTexture failure.");
		}

		hr = data->pTexture_->GetSurfaceLevel(0, &data->lpRenderSurface_);
		if (FAILED(hr))
			throw wexception("GetSurfaceLevel failure.");

		data->manager_ = this;
		data->name_ = name;
		data->type_ = TextureData::Type::TYPE_RENDER_TARGET;
		data->infoImage_.Width = width;
		data->infoImage_.Height = height;
		data->infoImage_.Format = fmt;
		data->resourceSize_ = width * height * Texture::GetFormatBPP(fmt);

		Logger::WriteTop(StringUtility::Format(L"TextureManager: Render target created. [%s]", name.c_str()));
	}
	catch (wexception& e) {
		Logger::WriteTop(StringUtility::Format(L"TextureManager: Failed to create render target \"%s\"\r\n    %s", 
			name.c_str(), e.what()));
		res = false;
	}

	if (res && bManaged) mapTextureData_[name] = data;
	dst = data;

	return res;
}
shared_ptr<Texture> TextureManager::CreateFromFile(const std::wstring& path, bool genMipmap, bool flgNonPowerOfTwo) {
	//path = PathProperty::GetUnique(path);
	shared_ptr<Texture> res;
	{
		Lock lock(lock_);

		auto itr = mapTexture_.find(path);
		if (itr != mapTexture_.end()) {
			res = itr->second;
		}
		else {
			shared_ptr<TextureData> data;

			auto itrFind = mapTextureData_.find(path);
			if (itrFind != mapTextureData_.end()) {
				data = itrFind->second;
			}
			else {
				if (!_CreateFromFile(data, path, genMipmap, flgNonPowerOfTwo))
					data = nullptr;
			}
			
			if (data) {
				res = std::make_shared<Texture>();
				res->data_ = data;
			}
		}
	}
	return res;
}

shared_ptr<Texture> TextureManager::CreateRenderTarget(const std::wstring& name, size_t width, size_t height) {
	shared_ptr<Texture> res;
	{
		Lock lock(lock_);

		auto itr = mapTexture_.find(name);
		if (itr != mapTexture_.end()) {
			res = itr->second;
		}
		else {
			shared_ptr<TextureData> data;

			auto itrFind = mapTextureData_.find(name);
			if (itrFind != mapTextureData_.end()) {
				data = itrFind->second;
			}
			else {
				if (!_CreateRenderTarget(data, name, width, height))
					data = nullptr;
			}

			if (data) {
				res = std::make_shared<Texture>();
				res->data_ = data;
			}
		}
	}
	return res;
}
shared_ptr<Texture> TextureManager::CreateFromFileInLoadThread(const std::wstring& path, 
	bool genMipmap, bool flgNonPowerOfTwo, bool bLoadImageInfo) 
{
	//path = PathProperty::GetUnique(path);
	shared_ptr<Texture> res;
	{
		//Lock lock(lock_);

		auto itr = mapTexture_.find(path);
		if (itr != mapTexture_.end()) {
			res = itr->second;
		}
		else {
			res = std::make_shared<Texture>();

			if (!IsDataExists(path)) {
				std::wstring pathReduce = PathProperty::ReduceModuleDirectory(path);

				shared_ptr<TextureData> data(new TextureData());

				data->manager_ = this;
				data->name_ = path;
				data->bReady_ = false;
				data->useMipMap_ = genMipmap;
				data->useNonPowerOfTwo_ = flgNonPowerOfTwo;
				data->type_ = TextureData::Type::TYPE_TEXTURE;

				if (bLoadImageInfo) {
					try {
						shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
						if (reader == nullptr || !reader->Open())
							throw wexception(ErrorUtility::GetFileNotFoundErrorMessage(pathReduce, true));

						std::string source = reader->ReadAllString();

						D3DXIMAGE_INFO info;
						HRESULT hr = D3DXGetImageInfoFromFileInMemory(source.c_str(), source.size(), &info);
						if (FAILED(hr))
							throw wexception("D3DXGetImageInfoFromFileInMemory failure.");

						data->infoImage_ = info;
						data->CalculateResourceSize();
					}
					catch (wexception& e) {
						std::wstring str = StringUtility::Format(
							L"TextureManager(LT): Failed to load texture \"%s\"\r\n    %s", 
							pathReduce.c_str(), e.what());
						Logger::WriteTop(str);
						data->bReady_ = true;

						return nullptr;
					}
				}

				res->data_ = data;
				mapTextureData_[path] = data;
				{
					shared_ptr<FileManager::LoadObject> source = res;
					shared_ptr<FileManager::LoadThreadEvent> event(new FileManager::LoadThreadEvent(this, path, res));
					FileManager::GetBase()->AddLoadThreadEvent(event);
				}
			}
		}
	}
	return res;
}
void TextureManager::CallFromLoadThread(shared_ptr<FileManager::LoadThreadEvent> event) {
	const std::wstring& path = event->GetPath();
	{
		//Lock lock(lock_);

		shared_ptr<Texture> texture = std::dynamic_pointer_cast<Texture>(event->GetSource());
		if (texture == nullptr) return;

		shared_ptr<TextureData> data = texture->data_;
		if (data == nullptr || data->bReady_) return;

		long countRef = data.use_count();
		if (countRef <= 2) {
			data->bReady_ = true;
			return;
		}

		std::wstring pathReduce = PathProperty::ReduceModuleDirectory(path);
		try {
			__CreateFromFile(data, path, data->useMipMap_, data->useNonPowerOfTwo_);

			data->bReady_ = true;

			Logger::WriteTop(StringUtility::Format(L"TextureManager(LT): Texture loaded. [%s]", pathReduce.c_str()));
		}
		catch (wexception& e) {
			std::wstring str = StringUtility::Format(L"TextureManager(LT): Failed to load texture \"%s\"\r\n    %s",
				pathReduce.c_str(), e.what());
			Logger::WriteTop(str);
			data->bReady_ = true;
			texture->data_ = nullptr;
			mapTextureData_.erase(path);
		}
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
	{
		Lock lock(lock_);

		bool bExist = mapTexture_.find(name) != mapTexture_.end();
		if (!bExist) {
			mapTexture_[name] = texture;
		}
	}
}
void TextureManager::Release(const std::wstring& name) {
	{
		Lock lock(lock_);

		mapTexture_.erase(name);
	}
}
void TextureManager::Release(std::map<std::wstring, shared_ptr<Texture>>::iterator itr) {
	{
		Lock lock(lock_);

		mapTexture_.erase(itr);
	}
}
bool TextureManager::IsDataExists(const std::wstring& name) {
	return mapTextureData_.find(name) != mapTextureData_.end();
}
std::map<std::wstring, shared_ptr<TextureData>>::iterator TextureManager::IsDataExistsItr(const std::wstring& name, bool* bRes) {
	auto res = mapTextureData_.find(name);
	if (bRes) *bRes = res != mapTextureData_.end();
	return res;
}

//****************************************************************************
//TextureInfoPanel
//****************************************************************************
TextureInfoPanel::TextureInfoPanel() {
}
TextureInfoPanel::~TextureInfoPanel() {
}
bool TextureInfoPanel::_AddedLogger(HWND hTab) {
	Create(hTab);

	WListView::Style styleListView;
	styleListView.SetStyle(WS_CHILD | WS_VISIBLE |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER);
	styleListView.SetStyleEx(WS_EX_CLIENTEDGE);
	styleListView.SetListViewStyleEx(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	wndListView_.Create(hWnd_, styleListView);

	wndListView_.AddColumn(60, ROW_ADDRESS, L"Address");
	wndListView_.AddColumn(144, ROW_NAME, L"Name");
	wndListView_.AddColumn(144, ROW_FULLNAME, L"Full Path");
	wndListView_.AddColumn(36, ROW_REFCOUNT, L"Uses");
	wndListView_.AddColumn(48, ROW_WIDTH_IMAGE, L"Width");
	wndListView_.AddColumn(48, ROW_HEIGHT_IMAGE, L"Height");
	wndListView_.AddColumn(60, ROW_SIZE, L"Size");

	SetWindowVisible(false);
	PanelInitialize();
	
	return true;
}
void TextureInfoPanel::LocateParts() {
	int wx = GetClientX();
	int wy = GetClientY();
	int wWidth = GetClientWidth();
	int wHeight = GetClientHeight();

	wndListView_.SetBounds(wx, wy, wWidth, wHeight);
}
void TextureInfoPanel::PanelUpdate() {
	TextureManager* manager = TextureManager::GetBase();
	if (manager == nullptr) return;

	if (!IsWindowVisible()) return;

	struct TextureDisplay {
		int address;
		std::wstring fileName;
		std::wstring fullPath;
		int countRef;
		uint32_t wd;
		uint32_t ht;
		uint32_t size;
	};

	std::vector<TextureDisplay> listTextureDisp;
	{
		Lock lock(manager->GetLock());

		{
			auto& mapData = manager->mapTextureData_;
			listTextureDisp.resize(mapData.size());

			int iTex = 0;
			for (auto itrMap = mapData.begin(); itrMap != mapData.end(); ++itrMap) {
				const std::wstring& name = itrMap->first;
				TextureData* data = (itrMap->second).get();

				int countRef = (itrMap->second).use_count();
				D3DXIMAGE_INFO* infoImage = &data->infoImage_;

				TextureDisplay displayData = {
					(int)data,
					PathProperty::GetFileName(name),
					name,
					countRef,
					infoImage->Width,
					infoImage->Height,
					data->GetResourceSize()
				};
				listTextureDisp[iTex++] = displayData;
			}
		}

		{
			int iRow = 0;
			for (; iRow < listTextureDisp.size(); ++iRow) {
				TextureDisplay* data = &listTextureDisp[iRow];

				wndListView_.SetText(iRow, ROW_ADDRESS, StringUtility::Format(L"%08x", data->address));
				wndListView_.SetText(iRow, ROW_NAME, data->fileName);
				wndListView_.SetText(iRow, ROW_FULLNAME, data->fullPath);
				wndListView_.SetText(iRow, ROW_REFCOUNT, std::to_wstring(data->countRef));
				wndListView_.SetText(iRow, ROW_WIDTH_IMAGE, std::to_wstring(data->wd));
				wndListView_.SetText(iRow, ROW_HEIGHT_IMAGE, std::to_wstring(data->ht));
				wndListView_.SetText(iRow, ROW_SIZE, std::to_wstring(data->size));
			}

			for (; iRow < wndListView_.GetRowCount(); ++iRow)
				wndListView_.DeleteRow(iRow);
		}
	}

	{
		IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
		UINT texMem = device->GetAvailableTextureMem() / (1024U * 1024U);

		if (WindowLogger* logger = WindowLogger::GetParent()) {
			shared_ptr<WStatusBar> statusBar = logger->GetStatusBar();
			statusBar->SetText(0, L"Available Video Memory");
			statusBar->SetText(1, StringUtility::Format(L"%u MB", texMem));
		}
	}
}

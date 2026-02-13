#pragma once

#include "../pch.h"

#include "DxConstant.hpp"
#include "DirectGraphics.hpp"

namespace directx {
	class TextureData;
	class Texture;
	class TextureManager;
	class TextureInfoPanel;

	struct CreateTextureData {
		UINT sizeType = D3DX_DEFAULT;
		UINT textureFilter = D3DX_FILTER_BOX;
			
		UINT mipmaps = 1;
		UINT mipmapFilter = D3DX_FILTER_BOX;
			
		DWORD colorKey = 0;

		size_t renderTargetWidth = 0;
		size_t renderTargetHeight = 0;
	};

	//****************************************************************************
	//Texture
	//****************************************************************************
	class TextureData : gstd::NonCopyable, gstd::NonMovable {
		friend Texture;
		friend TextureManager;
		friend TextureInfoPanel;
	public:
		enum class Type : uint8_t {
			Texture,
			RenderTarget,
		};
	protected:
		TextureManager* manager_;
		volatile std::atomic_bool ready_;
		
		Type type_;
		size_t resourceSize_;

		IDirect3DTexture9* pTexture_;
		IDirect3DSurface9* lpRenderSurface_;
		IDirect3DSurface9* lpRenderZ_;
	public:
		std::wstring name;
		D3DXIMAGE_INFO imageInfo;

		CreateTextureData createData;
	public:
		TextureData(TextureManager* manager, Type type);
		~TextureData();

		_NODISCARD IDirect3DTexture9* GetD3DTexture() { return pTexture_; }
		_NODISCARD IDirect3DSurface9* GetD3DSurface() { return lpRenderSurface_; }
		_NODISCARD IDirect3DSurface9* GetD3DZBuffer() { return lpRenderZ_; }

		_NODISCARD size_t GetResourceSize() const { return resourceSize_; }
		void CalculateResourceSize();
		
		static size_t GetFormatBPP(D3DFORMAT format);
		static size_t GetSurfaceSize(size_t width, size_t height, D3DFORMAT format);
	};

	class Texture :
		public gstd::FileManager::LoadObject,
		gstd::NonCopyable, gstd::NonMovable
	{
		friend TextureData;
		friend TextureManager;
		friend TextureInfoPanel;
	protected:
		shared_ptr<TextureData> data_;
	public:
		Texture(const shared_ptr<TextureData>& data);
		~Texture() override;
		
		void Release();

		std::wstring GetName() const;

		void SetTextureData(const shared_ptr<TextureData>& data);
		shared_ptr<TextureData> GetTextureData() { return data_; }

		IDirect3DTexture9* GetD3DTexture();
		IDirect3DSurface9* GetD3DSurface();
		IDirect3DSurface9* GetD3DZBuffer();

		TextureData::Type GetType();

		UINT GetWidth();
		UINT GetHeight();
		
		bool IsLoad() const { return data_ != nullptr && data_->ready_; }
	};

	//****************************************************************************
	//TextureManager
	//****************************************************************************
	class TextureManager :
		public DirectGraphicsListener, public gstd::FileManager::LoadThreadListener,
		gstd::NonCopyable, gstd::NonMovable
	{
		friend Texture;
		friend TextureData;
		friend TextureInfoPanel;
	private:
		static inline TextureManager* thisBase_ = nullptr;
	public:
		static const inline std::wstring TARGET_TRANSITION = L"__RENDERTARGET_TRANSITION__";
	protected:
		gstd::CriticalSection lock_;

		std::map<std::wstring, shared_ptr<Texture>> mapTexture_;
		std::map<std::wstring, shared_ptr<TextureData>> mapTextureData_;

		//std::map<std::wstring, std::pair<shared_ptr<TextureData>, IDirect3DSurface9*>> listRefreshSurface_;

		shared_ptr<TextureInfoPanel> panelInfo_;

		void ReleaseTextureData(const std::wstring& name);

		shared_ptr<TextureData> CreateDataFromFile(const std::wstring& path, const CreateTextureData& params);
		shared_ptr<TextureData> CreateDataRenderTarget(const std::wstring& name, const CreateTextureData& params);
	public:
		TextureManager() = default;
		~TextureManager() override;

		static TextureManager* GetBase() { return thisBase_; }
		
		virtual bool Initialize();
		gstd::CriticalSection& GetLock() { return lock_; }

		virtual void Clear();

		virtual void Add(const std::wstring& name, shared_ptr<Texture> texture);
		virtual void Release(const std::wstring& name);
		virtual shared_ptr<TextureData> GetData(const std::wstring& name);

		void ReleaseDxResource() override;
		void RestoreDxResource() override;

		shared_ptr<TextureData> GetTextureData(const std::wstring& name);
		shared_ptr<Texture> GetTexture(const std::wstring& name);
		
		shared_ptr<Texture> CreateFromFile(const std::wstring& path, const CreateTextureData& params = {});
		shared_ptr<Texture> CreateRenderTarget(const std::wstring& name, const CreateTextureData& params = {});
		shared_ptr<Texture> CreateFromData(const shared_ptr<TextureData>& data);
		shared_ptr<Texture> CreateFromD3DTexture(IDirect3DTexture9* pTexture, TextureData::Type type = TextureData::Type::Texture);
		
		shared_ptr<Texture> CreateFromFileInLoadThread(const std::wstring& path, const CreateTextureData& params = {},
			bool loadImageInfoNow = false);
		void CallFromLoadThread(shared_ptr<gstd::FileManager::LoadThreadEvent> event) override;

		void SetInfoPanel(shared_ptr<TextureInfoPanel> panel) { panelInfo_ = panel; }
	};

	//****************************************************************************
	//TextureInfoPanel
	//****************************************************************************
	class TextureInfoPanel : public gstd::ILoggerPanel {
		struct TextureDisplay {
			enum Column {
				Address,
				Name, FullPath,
				Uses, Size,
				_NoSort,
			};

			uintptr_t address;
			std::string strAddress;
			std::string fileName;
			std::string fullPath;
			int countRef;
			uint32_t wd;
			uint32_t ht;
			uint32_t size;

			weak_ptr<TextureData> dataRef;
			TextureData::Type textureType;
		public:
			TextureDisplay(const shared_ptr<TextureData>& data, const std::wstring& path, D3DXIMAGE_INFO* infoImage);
		public:
			static const ImGuiTableSortSpecs* imguiSortSpecs;
			static bool IMGUI_CDECL Compare(const TextureDisplay& a, const TextureDisplay& b);
		};
	protected:
		std::vector<TextureDisplay> listDisplay_;
		uint32_t videoMem_;
	public:
		TextureInfoPanel();

		void Initialize(const std::string& name) override;

		void Update() override;
		void ProcessGui() override;
	};
}
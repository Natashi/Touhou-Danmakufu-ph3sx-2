#pragma once

#include "../pch.h"

#include "DxConstant.hpp"
#include "DirectGraphics.hpp"
#include "Texture.hpp"

namespace directx {
	class ShaderManager;
	class Shader;
	class ShaderData;
	class ShaderInfoPanel;

	//*******************************************************************
	//ShaderData
	//*******************************************************************
	class ShaderData {
		friend Shader;
		friend ShaderManager;
		friend ShaderInfoPanel;
	private:
		ShaderManager* manager_;
		ID3DXEffect* effect_;
		std::wstring name_;
		bool bLoad_;
		bool bText_;
	public:
		ShaderData();
		virtual ~ShaderData();

		std::wstring& GetName() { return name_; }

		void ReleaseDxResource();
		void RestoreDxResource();
	};

	//*******************************************************************
	//ShaderManager
	//*******************************************************************
	class RenderShaderLibrary;
	class ShaderManager : public DirectGraphicsListener {
		friend Shader;
		friend ShaderData;
		friend ShaderInfoPanel;
	private:
		static ShaderManager* thisBase_;
	protected:
		gstd::CriticalSection lock_;

		shared_ptr<ShaderInfoPanel> panelInfo_;

		std::map<std::wstring, shared_ptr<ShaderData>> mapShaderData_;
		std::wstring lastError_;

		RenderShaderLibrary* renderManager_;

		void _ReleaseShaderData(const std::wstring& name);
		void _ReleaseShaderData(std::map<std::wstring, shared_ptr<ShaderData>>::iterator itr);
		bool _CreateFromFile(const std::wstring& path, shared_ptr<ShaderData>& dest);
		bool _CreateFromText(const std::string& source, shared_ptr<ShaderData>& dest);
		bool _CreateUnmanagedFromEffect(ID3DXEffect* effect, shared_ptr<ShaderData>& dest);
		static std::wstring _GetTextSourceID(const std::string& source);
	public:
		ShaderManager();
		virtual ~ShaderManager();

		static ShaderManager* GetBase() { return thisBase_; }
		
		virtual bool Initialize();
		gstd::CriticalSection& GetLock() { return lock_; }
		void Clear();

		RenderShaderLibrary* GetRenderLib() { return renderManager_; }

		virtual void ReleaseDxResource();
		virtual void RestoreDxResource();

		virtual bool IsDataExists(const std::wstring& name);
		virtual std::map<std::wstring, shared_ptr<ShaderData>>::iterator IsDataExistsItr(std::wstring& name);
		shared_ptr<ShaderData> GetShaderData(const std::wstring& name);

		shared_ptr<Shader> CreateFromFile(const std::wstring& path);
		shared_ptr<Shader> CreateFromText(const std::string& source);
		shared_ptr<Shader> CreateFromData(shared_ptr<ShaderData> data);
		shared_ptr<Shader> CreateUnmanagedFromEffect(ID3DXEffect* effect);
		shared_ptr<Shader> CreateFromFileInLoadThread(const std::wstring& path);
		virtual void CallFromLoadThread(shared_ptr<gstd::FileManager::LoadThreadEvent> event);

		std::wstring& GetLastError() { return lastError_; }

		void SetInfoPanel(shared_ptr<ShaderInfoPanel> panel) { panelInfo_ = panel; }
	};
	//****************************************************************************
	//ShaderInfoPanel
	//****************************************************************************
	class ShaderInfoPanel : public gstd::WindowLogger::Panel, public gstd::Thread {
	protected:
		enum {
			ROW_ADDRESS,
			ROW_NAME,
			ROW_REFCOUNT,
			ROW_EFFECT,
			ROW_PARAMETERS,
			ROW_TECHNIQUES,
		};
		int timeUpdateInterval_;
		gstd::WListView wndListView_;

		virtual bool _AddedLogger(HWND hTab);
		void _Run();
	public:
		ShaderInfoPanel();
		~ShaderInfoPanel();

		virtual void LocateParts();
		virtual void Update(ShaderManager* manager);
	};

	//*******************************************************************
	//ShaderParameter
	//*******************************************************************
	class ShaderParameter {
	private:
		D3DXHANDLE handle_;
		ShaderParameterType type_;
		std::vector<byte> value_;
		shared_ptr<Texture> texture_;
	public:
		ShaderParameter(D3DXHANDLE handle);
		virtual ~ShaderParameter();

		void SubmitData(ID3DXEffect* effect);

		D3DXHANDLE GetHandle() { return handle_; }

		ShaderParameterType GetType() { return type_; }
		void SetMatrix(D3DXMATRIX& matrix);
		inline D3DXMATRIX* GetMatrix();
		void SetMatrixArray(std::vector<D3DXMATRIX>& matrix);
		std::vector<D3DXMATRIX> GetMatrixArray();
		void SetVector(D3DXVECTOR4& vector);
		inline D3DXVECTOR4* GetVector();
		void SetFloat(FLOAT value);
		inline FLOAT* GetFloat();
		void SetFloatArray(std::vector<FLOAT>& values);
		std::vector<FLOAT> GetFloatArray();
		void SetTexture(shared_ptr<Texture> texture);
		inline shared_ptr<Texture> GetTexture();

		inline std::vector<byte>* GetRaw() { return &value_; }
	};

	//*******************************************************************
	//Shader
	//*******************************************************************
	class Shader {
		friend ShaderManager;
	protected:
		shared_ptr<ShaderData> data_;

		//bool bLoadShader_;
		//IDirect3DVertexShader9* pVertexShader_;
		//IDirect3DPixelShader9* pPixelShader_;

		std::string technique_;
		std::map<D3DXHANDLE, shared_ptr<ShaderParameter>> mapParam_;

		ShaderData* _GetShaderData() { return data_.get(); }
		shared_ptr<ShaderParameter> _GetParameter(const std::string& name, bool bCreate);
	public:
		Shader();
		Shader(Shader* shader);
		virtual ~Shader();

		void Release();

		bool LoadTechnique();
		bool LoadParameter();

		shared_ptr<ShaderData> GetData() { return data_; }
		ID3DXEffect* GetEffect();

		bool CreateFromFile(const std::wstring& path);
		bool CreateFromText(const std::string& source);
		bool CreateFromData(shared_ptr<ShaderData> data);

		bool IsLoad() { return data_ != nullptr && data_->bLoad_; }

		bool SetTechnique(const std::string& name);
		bool SetMatrix(const std::string& name, D3DXMATRIX& matrix);
		bool SetMatrixArray(const std::string& name, std::vector<D3DXMATRIX>& matrix);
		bool SetVector(const std::string& name, D3DXVECTOR4& vector);
		bool SetFloat(const std::string& name, FLOAT value);
		bool SetFloatArray(const std::string& name, std::vector<FLOAT>& values);
		bool SetTexture(const std::string& name, shared_ptr<Texture> texture);
	};
}
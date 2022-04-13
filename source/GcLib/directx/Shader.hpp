#pragma once

#include "../pch.h"

#include "DxConstant.hpp"
#include "DirectGraphics.hpp"
#include "Texture.hpp"

namespace directx {
	class ShaderManager;
	class Shader;
	class ShaderData;
	class ShaderIncludeCallback;
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
		std::unique_ptr<ShaderIncludeCallback> pIncludeCallback_;
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

		unique_ptr<RenderShaderLibrary> renderManager_;

		void _ReleaseShaderData(const std::wstring& name);
		void _ReleaseShaderData(std::map<std::wstring, shared_ptr<ShaderData>>::iterator itr);

		bool _CreateFromFile(const std::wstring& path, shared_ptr<ShaderData>& dest);
		bool _CreateFromText(const std::wstring& name, const std::string& source, shared_ptr<ShaderData>& dest);
		bool _CreateUnmanagedFromEffect(ID3DXEffect* effect, shared_ptr<ShaderData>& dest);
	public:
		ShaderManager();
		virtual ~ShaderManager();

		static ShaderManager* GetBase() { return thisBase_; }
		
		virtual bool Initialize();
		gstd::CriticalSection& GetLock() { return lock_; }
		void Clear();

		RenderShaderLibrary* GetRenderLib() { return renderManager_.get(); }

		virtual void ReleaseDxResource();
		virtual void RestoreDxResource();

		virtual bool IsDataExists(const std::wstring& name);
		virtual std::map<std::wstring, shared_ptr<ShaderData>>::iterator IsDataExistsItr(std::wstring& name);
		shared_ptr<ShaderData> GetShaderData(const std::wstring& name);

		shared_ptr<Shader> CreateFromFile(const std::wstring& path);
		shared_ptr<Shader> CreateFromText(const std::wstring& name, const std::string& source);
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
	class ShaderInfoPanel : public gstd::WindowLogger::Panel {
	protected:
		enum {
			ROW_ADDRESS,
			ROW_NAME,
			ROW_REFCOUNT,
			ROW_EFFECT,
			ROW_PARAMETERS,
			ROW_TECHNIQUES,
		};
		gstd::WListView wndListView_;

		virtual bool _AddedLogger(HWND hTab);
	public:
		ShaderInfoPanel();
		~ShaderInfoPanel();

		virtual void LocateParts();
		virtual void PanelUpdate();
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

		void SetInt(const int32_t value);
		void SetIntArray(const std::vector<int32_t>& values);
		void SetFloat(const float value);
		void SetFloatArray(const std::vector<float>& values);
		void SetVector(const D3DXVECTOR4& vector);
		void SetMatrix(const D3DXMATRIX& matrix);
		void SetMatrixArray(const std::vector<D3DXMATRIX>& matrix);
		void SetTexture(shared_ptr<Texture> texture);

		inline int32_t* GetInt();
		std::vector<int32_t> GetIntArray();
		inline float* GetFloat();
		std::vector<float> GetFloatArray();
		inline D3DXVECTOR4* GetVector();
		inline D3DXMATRIX* GetMatrix();
		std::vector<D3DXMATRIX> GetMatrixArray();
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

		std::string technique_;
		std::map<D3DXHANDLE, ShaderParameter> mapParam_;

		ShaderData* _GetShaderData() { return data_.get(); }
		ShaderParameter* _GetParameter(const std::string& name, bool bCreate);
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
		bool CreateFromText(const std::wstring& name, const std::string& source);
		bool CreateFromData(shared_ptr<ShaderData> data);

		bool IsLoad() { return data_ != nullptr && data_->bLoad_; }

		bool SetTechnique(const std::string& name);
		bool ValidateTechnique(const std::string& name);

		bool SetInt(const std::string& name, const int32_t value);
		bool SetIntArray(const std::string& name, const std::vector<int32_t>& value);
		bool SetFloat(const std::string& name, const float value);
		bool SetFloatArray(const std::string& name, const std::vector<float>& value);
		bool SetVector(const std::string& name, const D3DXVECTOR4& value);
		bool SetMatrix(const std::string& name, const D3DXMATRIX& value);
		bool SetMatrixArray(const std::string& name, const std::vector<D3DXMATRIX>& value);
		bool SetTexture(const std::string& name, shared_ptr<Texture> value);
	protected:
		ShaderParameter* __GetParam(const std::string& name, void* pData);
	public:
		bool GetInt(const std::string& name, int32_t* value);
		bool GetIntArray(const std::string& name, std::vector<int32_t>* value);
		bool GetFloat(const std::string& name, float* value);
		bool GetFloatArray(const std::string& name, std::vector<float>* value);
		bool GetVector(const std::string& name, D3DXVECTOR4* value);
		bool GetMatrix(const std::string& name, D3DXMATRIX* value);
		bool GetMatrixArray(const std::string& name, std::vector<D3DXMATRIX>* value);
		bool GetTexture(const std::string& name, shared_ptr<Texture>* value);
	};

	//*******************************************************************
	//ShaderIncludeCallback
	//*******************************************************************
	class ShaderIncludeCallback : public ID3DXInclude {
	private:
		std::wstring includeLocalDir_;
		std::vector<char> buffer_;
	public:
		ShaderIncludeCallback(const std::wstring& localDir);
		virtual ~ShaderIncludeCallback();

		HRESULT __stdcall Open(D3DXINCLUDE_TYPE type, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
		HRESULT __stdcall Close(LPCVOID pData);
	};
}
#ifndef __DIRECTX_SHADER__
#define __DIRECTX_SHADER__

#include "../pch.h"

#include "DxConstant.hpp"
#include "DirectGraphics.hpp"
#include "Texture.hpp"

namespace directx {
	//http://msdn.microsoft.com/ja-jp/library/bb944006(v=vs.85).aspx
	//http://msdn.microsoft.com/ja-jp/library/bb509647(v=vs.85).aspx

	class ShaderManager;
	class Shader;
	class ShaderData;
	/**********************************************************
	//ShaderData
	**********************************************************/
	class ShaderData {
		friend Shader;
		friend ShaderManager;
	private:
		ShaderManager* manager_;
		ID3DXEffect* effect_;
		std::wstring name_;
		volatile bool bLoad_;
		volatile bool bText_;
	public:
		ShaderData();
		virtual ~ShaderData();
		std::wstring GetName() { return name_; }
	};

	/**********************************************************
	//ShaderManager
	**********************************************************/
	class RenderShaderManager;
	class ShaderManager : public DirectGraphicsListener {
		friend Shader;
		friend ShaderData;
		static ShaderManager* thisBase_;
	protected:
		gstd::CriticalSection lock_;
		std::map<std::wstring, shared_ptr<Shader>> mapShader_;
		std::map<std::wstring, shared_ptr<ShaderData>> mapShaderData_;

		std::wstring lastError_;

		RenderShaderManager* renderManager_;

		void _ReleaseShaderData(std::wstring name);
		void _ReleaseShaderData(std::map<std::wstring, shared_ptr<ShaderData>>::iterator itr);
		bool _CreateFromFile(std::wstring path, shared_ptr<ShaderData>& dest);
		bool _CreateFromText(std::string& source, shared_ptr<ShaderData>& dest);
		static std::wstring _GetTextSourceID(std::string& source);
	public:
		ShaderManager();
		virtual ~ShaderManager();
		static ShaderManager* GetBase() { return thisBase_; }
		virtual bool Initialize();
		gstd::CriticalSection& GetLock() { return lock_; }
		void Clear();

		RenderShaderManager* GetRenderLib() { return renderManager_; }

		virtual void ReleaseDxResource();
		virtual void RestoreDxResource();

		virtual bool IsDataExists(std::wstring name);
		virtual std::map<std::wstring, shared_ptr<ShaderData>>::iterator IsDataExistsItr(std::wstring& name);
		shared_ptr<ShaderData> GetShaderData(std::wstring name);
		shared_ptr<Shader> CreateFromFile(std::wstring path);
		shared_ptr<Shader> CreateFromText(std::string source);
		shared_ptr<Shader> CreateFromFileInLoadThread(std::wstring path);
		virtual void CallFromLoadThread(shared_ptr<gstd::FileManager::LoadThreadEvent> event);

		void AddShader(std::wstring name, shared_ptr<Shader> shader);
		void DeleteShader(std::wstring name);
		shared_ptr<Shader> GetShader(std::wstring name);

		std::wstring GetLastError();
	};

	/**********************************************************
	//ShaderParameter
	**********************************************************/
	class ShaderParameter {
	public:
		enum {
			TYPE_UNKNOWN,
			TYPE_MATRIX,
			TYPE_MATRIX_ARRAY,
			TYPE_VECTOR,
			TYPE_FLOAT,
			TYPE_FLOAT_ARRAY,
			TYPE_TEXTURE,
		};
	private:
		int type_;
		std::vector<byte> value_;
		shared_ptr<Texture> texture_;
	public:
		ShaderParameter();
		virtual ~ShaderParameter();

		int GetType() { return type_; }
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

	/**********************************************************
	//Shader
	**********************************************************/
	class Shader {
		friend ShaderManager;
	protected:
		shared_ptr<ShaderData> data_;

		//bool bLoadShader_;
		//IDirect3DVertexShader9* pVertexShader_;
		//IDirect3DPixelShader9* pPixelShader_;

		std::string technique_;
		std::map<std::string, shared_ptr<ShaderParameter>> mapParam_;

		ShaderData* _GetShaderData() { return data_.get(); }
		shared_ptr<ShaderParameter> _GetParameter(std::string name, bool bCreate);
	public:
		Shader();
		Shader(Shader* shader);
		virtual ~Shader();
		void Release();

		bool LoadParameter();

		ID3DXEffect* GetEffect();

		bool CreateFromFile(std::wstring path);
		bool CreateFromText(std::string& source);
		bool IsLoad() { return data_ != nullptr && data_->bLoad_; }

		bool SetTechnique(std::string name);
		bool SetMatrix(std::string name, D3DXMATRIX& matrix);
		bool SetMatrixArray(std::string name, std::vector<D3DXMATRIX>& matrix);
		bool SetVector(std::string name, D3DXVECTOR4& vector);
		bool SetFloat(std::string name, FLOAT value);
		bool SetFloatArray(std::string name, std::vector<FLOAT>& values);
		bool SetTexture(std::string name, shared_ptr<Texture> texture);
	};
}


#endif

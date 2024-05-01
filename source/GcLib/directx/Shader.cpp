#include "source/GcLib/pch.h"

#include "Shader.hpp"
#include "HLSL.hpp"

using namespace gstd;
using namespace directx;

//*******************************************************************
//ShaderData
//*******************************************************************
ShaderData::ShaderData() {
	manager_ = nullptr;
	effect_ = nullptr;
	pIncludeCallback_ = nullptr;
	bLoad_ = false;
	bText_ = false;
}
ShaderData::~ShaderData() {
	ptr_release(effect_);
	bLoad_ = false;
}
void ShaderData::ReleaseDxResource() {
	if (effect_ == nullptr) return;
	effect_->OnLostDevice();
}
void ShaderData::RestoreDxResource() {
	if (effect_ == nullptr) return;
	effect_->OnResetDevice();
}

//*******************************************************************
//ShaderParameter
//*******************************************************************
ShaderParameter::ShaderParameter(D3DXHANDLE handle) {
	handle_ = handle;
	type_ = ShaderParameterType::Unknown;
	texture_ = nullptr;
}
ShaderParameter::~ShaderParameter() {
}

void ShaderParameter::SubmitData(ID3DXEffect* effect) {
	if (effect == nullptr) return;
	switch (type_) {
	case ShaderParameterType::Texture:
		if (texture_)
			effect->SetTexture(handle_, texture_->GetD3DTexture());
		else
			effect->SetTexture(handle_, nullptr);
		break;
	case ShaderParameterType::Int:
	case ShaderParameterType::IntArray:
	case ShaderParameterType::Float:
	case ShaderParameterType::FloatArray:
	case ShaderParameterType::Vector:
	case ShaderParameterType::Matrix:
	case ShaderParameterType::MatrixArray:
		effect->SetRawValue(handle_, value_.data(), 0, value_.size());
		break;
	}
}

void ShaderParameter::SetInt(const int32_t value) {
	type_ = ShaderParameterType::Int;

	value_.resize(sizeof(int32_t));
	memcpy(value_.data(), &value, sizeof(int32_t));
}
void ShaderParameter::SetIntArray(const std::vector<int32_t>& values) {
	type_ = ShaderParameterType::IntArray;

	value_.resize(values.size() * sizeof(int32_t));
	memcpy(value_.data(), values.data(), values.size() * sizeof(int32_t));
}
void ShaderParameter::SetFloat(const float value) {
	type_ = ShaderParameterType::Float;

	value_.resize(sizeof(float));
	memcpy(value_.data(), &value, sizeof(float));
}
void ShaderParameter::SetFloatArray(const std::vector<float>& values) {
	type_ = ShaderParameterType::FloatArray;

	value_.resize(values.size() * sizeof(float));
	memcpy(value_.data(), values.data(), values.size() * sizeof(float));
}
void ShaderParameter::SetVector(const D3DXVECTOR4& vector) {
	type_ = ShaderParameterType::Vector;

	value_.resize(sizeof(D3DXVECTOR4));
	memcpy(value_.data(), &vector, sizeof(D3DXVECTOR4));
}
void ShaderParameter::SetMatrix(const D3DXMATRIX& matrix) {
	type_ = ShaderParameterType::Matrix;

	value_.resize(sizeof(D3DXMATRIX));
	memcpy(value_.data(), &matrix, sizeof(D3DXMATRIX));
}
void ShaderParameter::SetMatrixArray(const std::vector<D3DXMATRIX>& listMatrix) {
	type_ = ShaderParameterType::MatrixArray;

	value_.resize(listMatrix.size() * sizeof(D3DXMATRIX));
	memcpy(value_.data(), listMatrix.data(), listMatrix.size() * sizeof(D3DXMATRIX));
}
void ShaderParameter::SetTexture(shared_ptr<Texture> texture) {
	type_ = ShaderParameterType::Texture;

	texture_ = texture;
}

int32_t* ShaderParameter::GetInt() {
	return (int32_t*)(value_.data());
}
std::vector<int32_t> ShaderParameter::GetIntArray() {
	std::vector<int32_t> res;
	res.resize(value_.size() / sizeof(int32_t));
	memcpy(res.data(), value_.data(), res.size() * sizeof(int32_t));
	return res;
}
float* ShaderParameter::GetFloat() {
	return (float*)(value_.data());
}
std::vector<float> ShaderParameter::GetFloatArray() {
	std::vector<float> res;
	res.resize(value_.size() / sizeof(float));
	memcpy(res.data(), value_.data(), res.size() * sizeof(float));
	return res;
}
D3DXVECTOR4* ShaderParameter::GetVector() {
	return (D3DXVECTOR4*)(value_.data());
}
D3DXMATRIX* ShaderParameter::GetMatrix() {
	return (D3DXMATRIX*)(value_.data());
}
std::vector<D3DXMATRIX> ShaderParameter::GetMatrixArray() {
	std::vector<D3DXMATRIX> res;
	res.resize(value_.size() / sizeof(D3DXMATRIX));
	memcpy(res.data(), value_.data(), res.size() * sizeof(D3DXMATRIX));
	return res;
}
shared_ptr<Texture> ShaderParameter::GetTexture() {
	return texture_;
}

//*******************************************************************
//Shader
//*******************************************************************
Shader::Shader() {
	data_ = nullptr;
}
Shader::Shader(Shader* shader) {
	{
		Lock lock(ShaderManager::GetBase()->GetLock());
		data_ = shader->data_;
	}
}
Shader::~Shader() {
	Release();
}
void Shader::Release() {
	{
		Lock lock(ShaderManager::GetBase()->GetLock());
		if (data_) {
			ShaderManager* manager = data_->manager_;
			if (manager) {
				auto itrData = manager->IsDataExistsItr(data_->name_);

				//No other references other than the one here and the one in ShaderManager
				if (data_.use_count() == 2) {
					manager->_ReleaseShaderData(itrData);
				}
			}
			data_ = nullptr;
		}
	}
}

ID3DXEffect* Shader::GetEffect() {
	ID3DXEffect* res = nullptr;
	if (data_) res = data_->effect_;
	return res;
}

bool Shader::CreateFromFile(const std::wstring& path) {
	bool res = false;
	{
		Lock lock(ShaderManager::GetBase()->GetLock());
		if (data_) Release();

		ShaderManager* manager = ShaderManager::GetBase();
		shared_ptr<Shader> shader = manager->CreateFromFile(path);
		if (shader)
			data_ = shader->data_;
		res = data_ != nullptr;
	}
	return res;
}
bool Shader::CreateFromText(const std::wstring& name, const std::string& source) {
	bool res = false;
	{
		Lock lock(ShaderManager::GetBase()->GetLock());
		if (data_) Release();

		ShaderManager* manager = ShaderManager::GetBase();
		shared_ptr<Shader> shader = manager->CreateFromText(name, source);
		if (shader)
			data_ = shader->data_;
		res = data_ != nullptr;
	}
	return res;
}
bool Shader::CreateFromData(shared_ptr<ShaderData> data) {
	bool res = false;
	{
		Lock lock(ShaderManager::GetBase()->GetLock());
		if (data_) Release();

		ShaderManager* manager = ShaderManager::GetBase();
		shared_ptr<Shader> shader = manager->CreateFromData(data);
		if (shader)
			data_ = shader->data_;
		res = data_ != nullptr;
	}
	return res;
}

bool Shader::LoadTechnique() {
	ID3DXEffect* effect = GetEffect();
	if (effect == nullptr) return false;

	D3DXHANDLE hTechnique = effect->GetTechniqueByName(technique_.c_str());

	HRESULT hr = effect->SetTechnique(hTechnique);
	if (FAILED(hr)) {
		hr = effect->ValidateTechnique(hTechnique);
		if (FAILED(hr)) {
			const char* err = DXGetErrorStringA(hr);
			const char* desc = DXGetErrorDescriptionA(hr);
			std::string log = StringUtility::Format("Shader: Invalid technique [%s]\r\n\t%s",
				err, desc);
			Logger::WriteTop(log);
		}
		return false;
	}

	return true;
}
bool Shader::LoadParameter() {
	ID3DXEffect* effect = GetEffect();
	if (effect == nullptr) return false;

	HRESULT hr = S_OK;

	for (auto itrParam = mapParam_.begin(); itrParam != mapParam_.end(); ++itrParam) {
		D3DXHANDLE name = itrParam->first;
		ShaderParameter* param = &itrParam->second;

		param->SubmitData(effect);
	}

	return true;
}
ShaderParameter* Shader::_GetParameter(const std::string& name, bool bCreate) {
	if (data_ == nullptr || data_->effect_ == nullptr) return nullptr;
	D3DXHANDLE handle = data_->effect_->GetParameterByName(nullptr, name.c_str());
	if (handle) {
		auto itr = mapParam_.find(handle);
		bool bFind = itr != mapParam_.end();
		if (!bFind && !bCreate) return nullptr;

		if (!bFind) {
			auto itrInsert = mapParam_.insert({ handle, ShaderParameter(handle) });
			return &itrInsert.first->second;
		}
		else {
			return &itr->second;
		}
	}
	return nullptr;
}

bool Shader::SetTechnique(const std::string& name) {
	technique_ = name;
	return true;
}
bool Shader::ValidateTechnique(const std::string& name) {
	ID3DXEffect* effect = GetEffect();
	if (effect == nullptr) return false;

	D3DXHANDLE hTechnique = effect->GetTechniqueByName(technique_.c_str());
	HRESULT hr = effect->ValidateTechnique(hTechnique);

	return SUCCEEDED(hr);
}

bool Shader::SetInt(const std::string& name, const int32_t value) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetInt(value);
	return true;
}
bool Shader::SetIntArray(const std::string& name, const std::vector<int32_t>& value) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetIntArray(value);
	return true;
}
bool Shader::SetFloat(const std::string& name, const float value) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetFloat(value);
	return true;
}
bool Shader::SetFloatArray(const std::string& name, const std::vector<float>& value) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetFloatArray(value);
	return true;
}
bool Shader::SetVector(const std::string& name, const D3DXVECTOR4& value) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetVector(value);
	return true;
}
bool Shader::SetMatrix(const std::string& name, const D3DXMATRIX& value) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetMatrix(value);
	return true;
}
bool Shader::SetMatrixArray(const std::string& name, const std::vector<D3DXMATRIX>& value) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetMatrixArray(value);
	return true;
}
bool Shader::SetTexture(const std::string& name, shared_ptr<Texture> value) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetTexture(value);
	return true;
}

ShaderParameter* Shader::__GetParam(const std::string& name, void* pData) {
	ID3DXEffect* effect = GetEffect();
	if (effect == nullptr || pData == nullptr) return nullptr;
	return _GetParameter(name, false);
}
bool Shader::GetInt(const std::string& name, int32_t* value) {
	if (ShaderParameter* param = __GetParam(name, value)) {
		*value = *param->GetInt();
		return true;
	}
	return false;
}
bool Shader::GetIntArray(const std::string& name, std::vector<int32_t>* value) {
	if (ShaderParameter* param = __GetParam(name, value)) {
		*value = param->GetIntArray();
		return true;
	}
	return false;
}
bool Shader::GetFloat(const std::string& name, float* value) {
	if (ShaderParameter * param = __GetParam(name, value)) {
		*value = *param->GetFloat();
		return true;
	}
	return false;
}
bool Shader::GetFloatArray(const std::string& name, std::vector<float>* value) {
	if (ShaderParameter* param = __GetParam(name, value)) {
		*value = param->GetFloatArray();
		return true;
	}
	return false;
}
bool Shader::GetVector(const std::string& name, D3DXVECTOR4* value) {
	if (ShaderParameter* param = __GetParam(name, value)) {
		*value = *param->GetVector();
		return true;
	}
	return false;
}
bool Shader::GetMatrix(const std::string& name, D3DXMATRIX* value) {
	if (ShaderParameter* param = __GetParam(name, value)) {
		*value = *param->GetMatrix();
		return true;
	}
	return false;
}
bool Shader::GetMatrixArray(const std::string& name, std::vector<D3DXMATRIX>* value) {
	if (ShaderParameter* param = __GetParam(name, value)) {
		*value = param->GetMatrixArray();
		return true;
	}
	return false;
}
bool Shader::GetTexture(const std::string& name, shared_ptr<Texture>* value) {
	if (ShaderParameter* param = __GetParam(name, value)) {
		*value = param->GetTexture();
		return true;
	}
	return false;
}

//*******************************************************************
//ShaderManager
//*******************************************************************
ShaderManager* ShaderManager::thisBase_ = nullptr;
ShaderManager::ShaderManager() {
	renderManager_ = nullptr;
}
ShaderManager::~ShaderManager() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->RemoveDirectGraphicsListener(this);

	Clear();
}
bool ShaderManager::Initialize() {
	if (thisBase_) return false;
	thisBase_ = this;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->AddDirectGraphicsListener(this);

	renderManager_.reset(new RenderShaderLibrary());

	return true;
}
void ShaderManager::Clear() {
	{
		Lock lock(lock_);
		mapShaderData_.clear();
	}
}
void ShaderManager::_ReleaseShaderData(const std::wstring& name) {
	auto itr = mapShaderData_.find(name);
	_ReleaseShaderData(itr);
}
void ShaderManager::_ReleaseShaderData(std::map<std::wstring, shared_ptr<ShaderData>>::iterator itr) {
	{
		Lock lock(lock_);
		if (itr != mapShaderData_.end()) {
			const std::wstring& name = itr->second->name_;
			itr->second->bLoad_ = false;
			mapShaderData_.erase(itr);
			Logger::WriteTop(StringUtility::Format(L"ShaderManager: Shader released [%s]", 
				PathProperty::ReduceModuleDirectory(name).c_str()));
		}
	}
}
bool ShaderManager::_CreateFromFile(const std::wstring& path, shared_ptr<ShaderData>& dest) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	lastError_ = L"";

	auto itr = mapShaderData_.find(path);
	if (itr != mapShaderData_.end()) {
		dest = itr->second;
		return true;
	}

	//path = PathProperty::GetUnique(path);
	std::wstring pathReduce = PathProperty::ReduceModuleDirectory(path);

	try {
		shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader == nullptr || !reader->Open()) {
			std::wstring err = ErrorUtility::GetFileNotFoundErrorMessage(path, true);
			throw wexception(err);
		}

		std::string source = reader->ReadAllString();

		dest->pIncludeCallback_.reset(new ShaderIncludeCallback(PathProperty::GetFileDirectory(path)));

		ID3DXBuffer* pErr = nullptr;
		HRESULT hr = D3DXCreateEffect(graphics->GetDevice(), source.c_str(), source.size(),
			nullptr, dest->pIncludeCallback_.get(), 0, nullptr, &dest->effect_, &pErr);

		if (FAILED(hr)) {
			std::wstring compileError = L"unknown error";
			if (pErr) {
				char* cText = (char*)pErr->GetBufferPointer();
				compileError = StringUtility::ConvertMultiToWide(cText);
			}

			ptr_release(dest->effect_);
			dest->pIncludeCallback_ = nullptr;

			std::wstring err = StringUtility::Format(L"%s\r\n\t%s",
				DXGetErrorStringW(hr), compileError.c_str());
			throw wexception(err);
		}
		else {
			dest->manager_ = this;
			dest->name_ = path;
			dest->bLoad_ = true;

			mapShaderData_[path] = dest;

			std::wstring log = StringUtility::Format(L"ShaderManager: Shader loaded [%s]", pathReduce.c_str());
			Logger::WriteTop(log);
		}
	}
	catch (gstd::wexception& e) {
		std::wstring err = StringUtility::Format(L"ShaderManager: Shader compile failed [%s]\r\n\t%s",
			pathReduce.c_str(), e.what());
		Logger::WriteTop(err);
		lastError_ = err;

		dest = nullptr;

		return false;
	}

	return true;
}
bool ShaderManager::_CreateFromText(const std::wstring& name, const std::string& source, shared_ptr<ShaderData>& dest) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	lastError_ = L"";

	auto itr = mapShaderData_.find(name);
	if (itr != mapShaderData_.end()) {
		dest = itr->second;
		return true;
	}

	try {
		ID3DXBuffer* pErr = nullptr;
		HRESULT hr = D3DXCreateEffect(graphics->GetDevice(), source.c_str(), source.size(),
			nullptr, nullptr, 0, nullptr, &dest->effect_, &pErr);

		if (FAILED(hr)) {
			char* compileError = "unknown error";
			if (pErr) {
				compileError = (char*)pErr->GetBufferPointer();
			}

			ptr_release(dest->effect_);

			std::string err = StringUtility::Format("%s\r\n\t%s",
				DXGetErrorStringA(hr), compileError);
			throw wexception(err);
		}
		else {
			dest->manager_ = this;
			dest->name_ = name;
			dest->bLoad_ = true;
			dest->bText_ = true;

			mapShaderData_[name] = dest;

			std::wstring log = StringUtility::Format(L"ShaderManager: Shader loaded [%s]", name.c_str());
			Logger::WriteTop(log);
		}
	}
	catch (gstd::wexception& e) {
		std::wstring err = StringUtility::Format(L"ShaderManager: Shader compile failed [%s]\r\n\t%s",
			name.c_str(), e.what());
		Logger::WriteTop(err);
		lastError_ = err;

		dest = nullptr;

		return false;
	}

	return true;
}
bool ShaderManager::_CreateCloneFromEffect(ID3DXEffect* effect, shared_ptr<ShaderData>& dest) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	lastError_ = L"";

	std::wstring shaderID = StringUtility::Format(L"Clone[%08x]", (int)effect);

	try {
		HRESULT hr = effect->CloneEffect(graphics->GetDevice(), &dest->effect_);

		if (FAILED(hr)) {
			const char* err = DXGetErrorStringA(hr);

			ptr_release(dest->effect_);

			throw wexception(err);
		}
		else {
			dest->manager_ = this;
			dest->name_ = shaderID;
			dest->bLoad_ = true;
			dest->bText_ = true;

			mapShaderData_[shaderID] = dest;

			std::wstring log = StringUtility::Format(L"ShaderManager: Shader cloned [%s]", shaderID.c_str());
			Logger::WriteTop(log);
		}
	}
	catch (gstd::wexception& e) {
		std::wstring err = StringUtility::Format(L"ShaderManager: Shader clone failed [%s]\r\n\t%s",
			shaderID.c_str(), e.what());
		Logger::WriteTop(err);
		lastError_ = err;

		dest = nullptr;

		return false;
	}

	return true;
}

void ShaderManager::ReleaseDxResource() {
	std::map<std::wstring, shared_ptr<ShaderData>>::iterator itrMap;
	{
		Lock lock(lock_);

		for (itrMap = mapShaderData_.begin(); itrMap != mapShaderData_.end(); ++itrMap) {
			shared_ptr<ShaderData> data = itrMap->second;
			data->ReleaseDxResource();
		}
		renderManager_->OnLostDevice();
	}
}
void ShaderManager::RestoreDxResource() {
	std::map<std::wstring, shared_ptr<ShaderData>>::iterator itrMap;
	{
		Lock lock(lock_);

		for (itrMap = mapShaderData_.begin(); itrMap != mapShaderData_.end(); ++itrMap) {
			shared_ptr<ShaderData> data = itrMap->second;
			data->RestoreDxResource();
		}
		renderManager_->OnResetDevice();
	}
}

bool ShaderManager::IsDataExists(const std::wstring& name) {
	bool res = false;
	{
		Lock lock(lock_);
		res = mapShaderData_.find(name) != mapShaderData_.end();
	}
	return res;
}
std::map<std::wstring, shared_ptr<ShaderData>>::iterator ShaderManager::IsDataExistsItr(std::wstring& name) {
	auto res = mapShaderData_.end();
	{
		Lock lock(lock_);
		res = mapShaderData_.find(name);
	}
	return res;
}
shared_ptr<ShaderData> ShaderManager::GetShaderData(const std::wstring& name) {
	shared_ptr<ShaderData> res;
	{
		Lock lock(lock_);
		auto itr = mapShaderData_.find(name);
		if (itr != mapShaderData_.end()) {
			res = itr->second;
		}
	}
	return res;
}
shared_ptr<Shader> ShaderManager::CreateFromFile(const std::wstring& path) {
	//path = PathProperty::GetUnique(path);
	shared_ptr<Shader> res = nullptr;
	{
		Lock lock(lock_);

		shared_ptr<ShaderData> data(new ShaderData());
		if (_CreateFromFile(path, data)) {
			res = make_shared<Shader>();
			res->data_ = data;
		}
	}
	return res;
}
shared_ptr<Shader> ShaderManager::CreateFromText(const std::wstring& name, const std::string& source) {
	shared_ptr<Shader> res = nullptr;
	{
		Lock lock(lock_);

		shared_ptr<ShaderData> data(new ShaderData());
		if (_CreateFromText(name, source, data)) {
			res = make_shared<Shader>();
			res->data_ = data;
		}
	}
	return res;
}
shared_ptr<Shader> ShaderManager::CreateFromData(shared_ptr<ShaderData> data) {
	shared_ptr<Shader> res = nullptr;
	{
		Lock lock(lock_);

		if (data) {
			res = make_shared<Shader>();
			res->data_ = data;
		}
	}
	return res;
}
shared_ptr<Shader> ShaderManager::CreateCloneFromEffect(ID3DXEffect* effect) {
	shared_ptr<Shader> res = nullptr;
	{
		Lock lock(lock_);

		shared_ptr<ShaderData> data(new ShaderData());
		if (_CreateCloneFromEffect(effect, data)) {
			res = make_shared<Shader>();
			res->data_ = data;
		}
	}
	return res;
}
shared_ptr<Shader> ShaderManager::CreateFromFileInLoadThread(const std::wstring& path) {
	return false;
}
void ShaderManager::CallFromLoadThread(shared_ptr<gstd::FileManager::LoadThreadEvent> event) {
}

//*******************************************************************
//ShaderIncludeCallback
//*******************************************************************
ShaderIncludeCallback::ShaderIncludeCallback(const std::wstring& localDir) {
	includeLocalDir_ = PathProperty::ReplaceYenToSlash(localDir);
	if (includeLocalDir_.back() != '/')
		includeLocalDir_ += '/';
}
ShaderIncludeCallback::~ShaderIncludeCallback() {
}

HRESULT __stdcall ShaderIncludeCallback::Open(D3DXINCLUDE_TYPE type, LPCSTR pFileName, LPCVOID pParentData,
	LPCVOID* ppData, UINT* pBytes)
{
	std::wstring sPath = StringUtility::ConvertMultiToWide(pFileName);
	sPath = PathProperty::ReplaceYenToSlash(sPath);

	if (!sPath._Starts_with(L"./"))
		sPath = L"./" + sPath;

	if (type == D3DXINC_LOCAL) {
		sPath = includeLocalDir_ + sPath.substr(2);
	}
	else {
		sPath = PathProperty::GetModuleDirectory() + sPath.substr(2);
	}
	sPath = PathProperty::GetUnique(sPath);

	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(sPath);
	if (reader == nullptr || !reader->Open()) {
		std::wstring error = StringUtility::Format(
			L"Shader Compiler: Include file is not found [%s]\r\n", sPath.c_str());
		Logger::WriteTop(error);
		return E_FAIL;
	}

	buffer_.resize(reader->GetFileSize());
	if (buffer_.size() > 0) {
		reader->Read(buffer_.data(), reader->GetFileSize());

		*ppData = buffer_.data();
		*pBytes = buffer_.size();
	}
	else {
		*ppData = nullptr;
		*pBytes = 0;
	}

	return S_OK;
}
HRESULT __stdcall ShaderIncludeCallback::Close(LPCVOID pData) {
	buffer_.clear();
	return S_OK;
}

//****************************************************************************
//TextureInfoPanel
//****************************************************************************
ShaderInfoPanel::ShaderInfoPanel() {
}

void ShaderInfoPanel::Initialize(const std::string& name) {
	ILoggerPanel::Initialize(name);
}

void ShaderInfoPanel::Update() {
	ShaderManager* manager = ShaderManager::GetBase();
	if (manager == nullptr) {
		listDisplay_.clear();
		return;
	}

	RenderShaderLibrary* renderLib = manager->GetRenderLib();

	{
		Lock lock(Logger::GetTop()->GetLock());

		auto& mapData = manager->mapShaderData_;

		size_t iDataAdd = 0;
		{
			size_t size = mapData.size();
			if (renderLib)
				size += renderLib->GetShaderCount();
			listDisplay_.resize(size);
		}

		auto _AddData = [&](ID3DXEffect* pEffect, const std::string& name, size_t dataAddress, shared_ptr<ShaderData> ref) {
			ShaderDisplay displayData(pEffect, name, dataAddress, ref);
			listDisplay_[iDataAdd++] = displayData;
		};

		// Add built-in shaders
		if (renderLib) {
#define _ADD_BUILTIN(_eff, _name) _AddData(_eff, _name, (size_t)(_eff), nullptr);

			_ADD_BUILTIN(renderLib->GetRender2DShader(), ShaderSource::nameRender2D_);
			_ADD_BUILTIN(renderLib->GetInstancing2DShader(), ShaderSource::nameHwInstance2D_);
			_ADD_BUILTIN(renderLib->GetInstancing3DShader(), ShaderSource::nameHwInstance3D_);
			_ADD_BUILTIN(renderLib->GetIntersectVisualShader1(), ShaderSource::nameIntersectVisual1_);
			_ADD_BUILTIN(renderLib->GetIntersectVisualShader2(), ShaderSource::nameIntersectVisual2_);

#undef _ADD_BUILTIN
		}

		// Add user shaders
		{
			for (auto& [name, data] : mapData) {
				std::string nameReduce = STR_MULTI(PathProperty::ReduceModuleDirectory(name));
				_AddData(data->effect_, nameReduce, (size_t)data.get(), data);
			}
		}

		// Sort new data as well
		if (ShaderDisplay::imguiSortSpecs) {
			if (listDisplay_.size() > 1) {
				std::sort(listDisplay_.begin(), listDisplay_.end(), ShaderDisplay::Compare);
			}
		}	
	}
}

const char* ShaderParamClassToString(D3DXPARAMETER_CLASS type) {
	switch (type) {
	case D3DXPC_SCALAR:		return "scalar";
	case D3DXPC_VECTOR:		return "vector";
	case D3DXPC_MATRIX_ROWS:
	case D3DXPC_MATRIX_COLUMNS:
		return "matrix";
	case D3DXPC_OBJECT:		return "object";
	case D3DXPC_STRUCT:		return "struct";
	}
	return "other";
}
const char* ShaderParamTypeToString(D3DXPARAMETER_TYPE type) {
	switch (type) {
	case D3DXPT_VOID:		return "void";
	case D3DXPT_BOOL:		return "bool";
	case D3DXPT_INT:		return "int";
	case D3DXPT_FLOAT:		return "float";
	case D3DXPT_STRING:		return "string";
	case D3DXPT_TEXTURE:	return "texture";
	case D3DXPT_TEXTURE1D:	return "texture1d";
	case D3DXPT_TEXTURE2D:	return "texture2d";
	case D3DXPT_TEXTURE3D:	return "texture3d";
	case D3DXPT_TEXTURECUBE:	return "texture_cube";
	case D3DXPT_SAMPLER:	return "sampler";
	case D3DXPT_SAMPLER1D:	return "sampler1d";
	case D3DXPT_SAMPLER2D:	return "sampler2d";
	case D3DXPT_SAMPLER3D:	return "sampler3d";
	case D3DXPT_SAMPLERCUBE:	return "sampler_cube";
	case D3DXPT_PIXELSHADER:	return "pixel_shader";
	case D3DXPT_VERTEXSHADER:	return "vertex_shader";
	case D3DXPT_PIXELFRAGMENT:	return "pixel_fragment";
	case D3DXPT_VERTEXFRAGMENT:	return "vertex_fragment";
	}
	return "other";
}

void ShaderInfoPanel::ProcessGui() {
	Logger* parent = Logger::GetTop();

	auto orgTextColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);

	auto _Tooltip = [&orgTextColor](std::function<void()> gui) {
		ImGui::BeginTooltipEx(ImGuiTooltipFlags_OverridePreviousTooltip, ImGuiWindowFlags_None);

		ImGui::PushStyleColor(ImGuiCol_Text, orgTextColor);
		gui();
		ImGui::PopStyleColor();

		ImGui::EndTooltip();
	};

	float ht = ImGui::GetContentRegionAvail().y;

	if (ImGui::BeginChild("pshader_child_table", ImVec2(0, ht), false, ImGuiWindowFlags_HorizontalScrollbar)) {
		ImGuiTableFlags flags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable
			| ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX
			| ImGuiTableFlags_RowBg
			| ImGuiTableFlags_Sortable /*| ImGuiTableFlags_SortMulti*/;

		if (ImGui::BeginTable("pshader_table", 7, flags)) {
			ImGui::TableSetupScrollFreeze(0, 1);

			constexpr auto sortDef = ImGuiTableColumnFlags_DefaultSort,
				sortNone = ImGuiTableColumnFlags_NoSort;
			constexpr auto colFlags = ImGuiTableColumnFlags_WidthStretch;
			ImGui::TableSetupColumn("Address", sortDef, 0, ShaderDisplay::Address);
			ImGui::TableSetupColumn("Name", colFlags | sortDef, 220, ShaderDisplay::Name);
			ImGui::TableSetupColumn("Effect", sortDef, 0, ShaderDisplay::Effect);
			ImGui::TableSetupColumn("Uses", sortDef, 0, ShaderDisplay::Uses);
			ImGui::TableSetupColumn("Parameters", colFlags | sortNone, 80, ShaderDisplay::_NoSort);
			ImGui::TableSetupColumn("Techniques", colFlags | sortNone, 80, ShaderDisplay::_NoSort);
			ImGui::TableSetupColumn("Functions", colFlags | sortNone, 80, ShaderDisplay::_NoSort);

			ImGui::TableHeadersRow();

			if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
				if (specs->SpecsDirty) {
					ShaderDisplay::imguiSortSpecs = specs;
					if (listDisplay_.size() > 1) {
						std::sort(listDisplay_.begin(), listDisplay_.end(), ShaderDisplay::Compare);
					}
					//ShaderDisplay::imguiSortSpecs = nullptr;

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
						ShaderDisplay& item = listDisplay_[i];
						bool bBuiltin = !item.refData.has_value();

						ImGui::TableNextRow();

						if (bBuiltin)
							ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(32, 32, 192));

						_SETCOL(0, item.strAddress);

						_SETCOL(1, item.name);
						if (ImGui::IsItemHovered())
							_Tooltip([&]() { ImGui::TextUnformatted(item.name.c_str()); });

						_SETCOL(2, item.strAddrEffect);
						if (ImGui::IsItemHovered())
							_Tooltip([&]() { ImGui::TextUnformatted(item.descEffect.Creator); });

						if (!bBuiltin) {
							_SETCOL(3, std::to_string(item.countRef));
						}
						else {
							ImGui::TableSetColumnIndex(3);
							ImGui::Text("[internal]");
						}

						_SETCOL(4, std::to_string(item.descEffect.Parameters));
						if (ImGui::IsItemHovered()) {
							item.LoadDescParameters();
							if (item.descParams.has_value() && !item.descParams->empty()) {
								_Tooltip([&]() {
									for (auto& param : *item.descParams) {
										ImGui::TextUnformatted(param.c_str());
									}
								});
							}
						}

						_SETCOL(5, std::to_string(item.descEffect.Techniques));
						if (ImGui::IsItemHovered()) {
							item.LoadDescTechniques();
							if (item.descTechs.has_value() && !item.descTechs->empty()) {
								_Tooltip([&]() {
									for (auto& tech : *item.descTechs) {
										ImGui::TextUnformatted(tech.c_str());
									}
								});
							}
						}
						
						_SETCOL(6, std::to_string(item.descEffect.Functions));
						if (ImGui::IsItemHovered()) {
							item.LoadDescFunctions();
							if (item.descFuncs.has_value() && !item.descFuncs->empty()) {
								_Tooltip([&]() {
									for (auto& func : *item.descFuncs) {
										ImGui::TextUnformatted(func.c_str());
									}
								});
							}
						}

						if (bBuiltin)
							ImGui::PopStyleColor();
					}
				}

				ImGui::PopFont();

#undef _SETCOL
			}

			ImGui::EndTable();
		}
	}
	ImGui::EndChild();
}

ShaderInfoPanel::ShaderDisplay::ShaderDisplay(ID3DXEffect* pEffect, const std::string& name, 
	uintptr_t dataAddress, shared_ptr<ShaderData> ref) :
	pEffect(pEffect), address(dataAddress), name(name)
{
	if (ref) {
		this->refData = ref;
		this->countRef = ref.use_count() - 1;
	}

	strAddress = StringUtility::FromAddress(dataAddress);
	strAddrEffect = StringUtility::FromAddress((uintptr_t)pEffect);

	pEffect->GetDesc(&descEffect);
}
void ShaderInfoPanel::ShaderDisplay::LoadDescParameters() {
	if (descParams.has_value()) return;
	{
		Lock lock(Logger::GetTop()->GetLock());

		// If refData is not empty (not a builtin), shader must not have already been released
		if (!refData.has_value() || !refData->expired()) {
			std::vector<std::string> res(descEffect.Parameters);

			for (size_t i = 0; i < res.size(); ++i) {
				D3DXPARAMETER_DESC desc;
				D3DXHANDLE hParam = pEffect->GetParameter(nullptr, i);
				pEffect->GetParameterDesc(hParam, &desc);

				res[i] = STR_FMT("%s %s %s : %s",
					ShaderParamClassToString(desc.Class),
					ShaderParamTypeToString(desc.Type),
					desc.Name, desc.Semantic);
			}

			descParams = res;
		}
	}
}
void ShaderInfoPanel::ShaderDisplay::LoadDescTechniques() {
	if (descTechs.has_value()) return;
	{
		Lock lock(Logger::GetTop()->GetLock());

		// If refData is not empty (not a builtin), shader must not have already been released
		if (!refData.has_value() || !refData->expired()) {
			std::vector<std::string> res(descEffect.Techniques);

			for (size_t i = 0; i < res.size(); ++i) {
				D3DXHANDLE hTech = pEffect->GetTechnique(i);

				D3DXTECHNIQUE_DESC desc;
				pEffect->GetTechniqueDesc(hTech, &desc);

				/*
				std::vector<D3DXPASS_DESC> passes(desc.Passes);
				for (size_t iPass = 0; iPass < passes.size(); ++iPass) {
					D3DXHANDLE hPass = pEffect->GetPass(hTech, iPass);
					pEffect->GetPassDesc(hPass, &passes[i]);
				}
				*/

				res[i] = STR_FMT("%s : %u pass",
					desc.Name, desc.Passes);
			}

			descTechs = res;
		}
	}
}
void ShaderInfoPanel::ShaderDisplay::LoadDescFunctions() {
	if (descFuncs.has_value()) return;
	{
		Lock lock(Logger::GetTop()->GetLock());

		// If refData is not empty (not a builtin), shader must not have already been released
		if (!refData.has_value() || !refData->expired()) {
			std::vector<std::string> res(descEffect.Functions);

			for (size_t i = 0; i < res.size(); ++i) {
				D3DXFUNCTION_DESC desc;
				D3DXHANDLE hFunc = pEffect->GetFunction(i);
				pEffect->GetFunctionDesc(hFunc, &desc);

				res[i] = STR_FMT("%s", desc.Name);
			}

			descFuncs = res;
		}
	}
}

const ImGuiTableSortSpecs* ShaderInfoPanel::ShaderDisplay::imguiSortSpecs = nullptr;
bool ShaderInfoPanel::ShaderDisplay::Compare(const ShaderDisplay& a, const ShaderDisplay& b) {
	for (int i = 0; i < imguiSortSpecs->SpecsCount; ++i) {
		const ImGuiTableColumnSortSpecs* spec = &imguiSortSpecs->Specs[i];

		int rcmp = 0;

#define CASE_SORT(_id, _l, _r) \
		case _id: { \
			if (_l != _r) rcmp = (_l < _r) ? 1 : -1; \
			break; \
		}

		switch ((Column)spec->ColumnUserID) {
		CASE_SORT(Column::Address, a.address, b.address);
		case Column::Name:
			rcmp = a.name.compare(b.name);
			break;
		CASE_SORT(Column::Uses, a.countRef, b.countRef);
		CASE_SORT(Column::Effect, a.pEffect, b.pEffect);
		}

#undef CASE_SORT

		if (rcmp != 0) {
			return spec->SortDirection == ImGuiSortDirection_Ascending
				? rcmp < 0 : rcmp > 0;
		}
	}

	return a.address < b.address;
}


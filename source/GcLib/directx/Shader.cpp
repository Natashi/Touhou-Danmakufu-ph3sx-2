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
	case ShaderParameterType::Float:
		effect->SetFloat(handle_, *GetFloat());
		break;
	case ShaderParameterType::Texture:
		if (texture_)
			effect->SetTexture(handle_, texture_->GetD3DTexture());
		else
			effect->SetTexture(handle_, nullptr);
		break;
	case ShaderParameterType::FloatArray:
	case ShaderParameterType::Vector:
	case ShaderParameterType::Matrix:
	case ShaderParameterType::MatrixArray:
		effect->SetRawValue(handle_, value_.data(), 0, value_.size());
		break;
	}
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
void ShaderParameter::SetVector(const D3DXVECTOR4& vector) {
	type_ = ShaderParameterType::Vector;

	value_.resize(sizeof(D3DXVECTOR4));
	memcpy(value_.data(), &vector, sizeof(D3DXVECTOR4));
}
void ShaderParameter::SetFloat(const FLOAT value) {
	type_ = ShaderParameterType::Float;

	value_.resize(sizeof(FLOAT));
	memcpy(value_.data(), &value, sizeof(FLOAT));
}
void ShaderParameter::SetFloatArray(const std::vector<FLOAT>& values) {
	type_ = ShaderParameterType::FloatArray;

	value_.resize(values.size() * sizeof(FLOAT));
	memcpy(value_.data(), values.data(), values.size() * sizeof(FLOAT));
}
void ShaderParameter::SetTexture(shared_ptr<Texture> texture) {
	type_ = ShaderParameterType::Texture;

	texture_ = texture;
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
D3DXVECTOR4* ShaderParameter::GetVector() {
	return (D3DXVECTOR4*)(value_.data());
}
FLOAT* ShaderParameter::GetFloat() {
	return (FLOAT*)(value_.data());
}
std::vector<FLOAT> ShaderParameter::GetFloatArray() {
	std::vector<FLOAT> res;
	res.resize(value_.size() / sizeof(FLOAT));
	memcpy(res.data(), value_.data(), res.size() * sizeof(FLOAT));
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
			std::string log = StringUtility::Format("Shader: Invalid technique. [%s]\r\n\t%s",
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
bool Shader::SetMatrix(const std::string& name, const D3DXMATRIX& matrix) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetMatrix(matrix);
	return true;
}
bool Shader::SetMatrixArray(const std::string& name, const std::vector<D3DXMATRIX>& matrix) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetMatrixArray(matrix);
	return true;
}
bool Shader::SetVector(const std::string& name, const D3DXVECTOR4& vector) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetVector(vector);
	return true;
}
bool Shader::SetFloat(const std::string& name, const FLOAT value) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetFloat(value);
	return true;
}
bool Shader::SetFloatArray(const std::string& name, const std::vector<FLOAT>& values) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetFloatArray(values);
	return true;
}
bool Shader::SetTexture(const std::string& name, shared_ptr<Texture> texture) {
	ShaderParameter* param = _GetParameter(name, true);
	if (param)
		param->SetTexture(texture);
	return true;
}

bool Shader::ValidateTechnique(const std::string& name) {
	ID3DXEffect* effect = GetEffect();
	if (effect == nullptr) return false;

	D3DXHANDLE hTechnique = effect->GetTechniqueByName(technique_.c_str());
	HRESULT hr = effect->ValidateTechnique(hTechnique);

	return SUCCEEDED(hr);
}
bool Shader::GetMatrix(const std::string& name, D3DXMATRIX* matrix) {
	ID3DXEffect* effect = GetEffect();
	if (effect == nullptr || matrix == nullptr) return false;

	ShaderParameter* param = _GetParameter(name, false);
	if (param) {
		*matrix = *param->GetMatrix();
		return true;
	}
	return false;
}
bool Shader::GetMatrixArray(const std::string& name, std::vector<D3DXMATRIX>* matrix) {
	ID3DXEffect* effect = GetEffect();
	if (effect == nullptr || matrix == nullptr) return false;

	ShaderParameter* param = _GetParameter(name, false);
	if (param) {
		*matrix = param->GetMatrixArray();
		return true;
	}
	return false;
}
bool Shader::GetVector(const std::string& name, D3DXVECTOR4* vector) {
	ID3DXEffect* effect = GetEffect();
	if (effect == nullptr || vector == nullptr) return false;

	ShaderParameter* param = _GetParameter(name, false);
	if (param) {
		*vector = *param->GetVector();
		return true;
	}
	return false;
}
bool Shader::GetFloat(const std::string& name, FLOAT* value) {
	ID3DXEffect* effect = GetEffect();
	if (effect == nullptr || value == nullptr) return false;

	ShaderParameter* param = _GetParameter(name, false);
	if (param) {
		*value = *param->GetFloat();
		return true;
	}
	return false;
}
bool Shader::GetFloatArray(const std::string& name, std::vector<FLOAT>* values) {
	ID3DXEffect* effect = GetEffect();
	if (effect == nullptr || values == nullptr) return false;

	ShaderParameter* param = _GetParameter(name, false);
	if (param) {
		*values = param->GetFloatArray();
		return true;
	}
	return false;
}
bool Shader::GetTexture(const std::string& name, shared_ptr<Texture>* texture) {
	ID3DXEffect* effect = GetEffect();
	if (effect == nullptr || texture == nullptr) return false;

	ShaderParameter* param = _GetParameter(name, false);
	if (param) {
		*texture = param->GetTexture();
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
			Logger::WriteTop(StringUtility::Format(L"ShaderManager: Shader released. [%s]", 
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

			std::wstring log = StringUtility::Format(L"ShaderManager: Shader loaded. [%s]", pathReduce.c_str());
			Logger::WriteTop(log);
		}
	}
	catch (gstd::wexception& e) {
		std::wstring err = StringUtility::Format(L"ShaderManager: Shader compile failed. [%s]\r\n\t%s",
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

			std::wstring log = StringUtility::Format(L"ShaderManager: Shader loaded. [%s]", name.c_str());
			Logger::WriteTop(log);
		}
	}
	catch (gstd::wexception& e) {
		std::wstring err = StringUtility::Format(L"ShaderManager: Shader compile failed. [%s]\r\n\t%s",
			name.c_str(), e.what());
		Logger::WriteTop(err);
		lastError_ = err;

		dest = nullptr;

		return false;
	}

	return true;
}
bool ShaderManager::_CreateUnmanagedFromEffect(ID3DXEffect* effect, shared_ptr<ShaderData>& dest) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	lastError_ = L"";

	std::string shaderID = StringUtility::Format("%08x", (int)effect);

	try {
		HRESULT hr = effect->CloneEffect(graphics->GetDevice(), &dest->effect_);

		if (FAILED(hr)) {
			const char* err = DXGetErrorStringA(hr);

			ptr_release(dest->effect_);

			throw wexception(err);
		}
		else {
			dest->name_ = L"";
			dest->bLoad_ = true;

			std::string log = StringUtility::Format("ShaderManager: Shader cloned. [%s]", shaderID.c_str());
			Logger::WriteTop(log);
		}
	}
	catch (gstd::wexception& e) {
		std::wstring err = StringUtility::Format(L"ShaderManager: Shader clone failed. [%s]\r\n\t%s",
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
			res = std::make_shared<Shader>();
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
			res = std::make_shared<Shader>();
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
			res = std::make_shared<Shader>();
			res->data_ = data;
		}
	}
	return res;
}
shared_ptr<Shader> ShaderManager::CreateUnmanagedFromEffect(ID3DXEffect* effect) {
	shared_ptr<Shader> res = nullptr;
	{
		Lock lock(lock_);

		shared_ptr<ShaderData> data(new ShaderData());
		if (_CreateUnmanagedFromEffect(effect, data)) {
			res = std::make_shared<Shader>();
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
			L"Shader Compiler: Include file is not found. [%s]\r\n", sPath.c_str());
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
ShaderInfoPanel::~ShaderInfoPanel() {
}
bool ShaderInfoPanel::_AddedLogger(HWND hTab) {
	Create(hTab);

	gstd::WListView::Style styleListView;
	styleListView.SetStyle(WS_CHILD | WS_VISIBLE |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER);
	styleListView.SetStyleEx(WS_EX_CLIENTEDGE);
	styleListView.SetListViewStyleEx(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	wndListView_.Create(hWnd_, styleListView);

	wndListView_.AddColumn(60, ROW_ADDRESS, L"Address");
	wndListView_.AddColumn(224, ROW_NAME, L"Name");
	wndListView_.AddColumn(36, ROW_REFCOUNT, L"Uses");
	wndListView_.AddColumn(60, ROW_EFFECT, L"Effect");
	wndListView_.AddColumn(72, ROW_PARAMETERS, L"Parameters");
	wndListView_.AddColumn(72, ROW_TECHNIQUES, L"Techniques");

	SetWindowVisible(false);
	PanelInitialize();

	return true;
}
void ShaderInfoPanel::LocateParts() {
	int wx = GetClientX();
	int wy = GetClientY();
	int wWidth = GetClientWidth();
	int wHeight = GetClientHeight();

	wndListView_.SetBounds(wx, wy, wWidth, wHeight);
}
void ShaderInfoPanel::PanelUpdate() {
	ShaderManager* manager = ShaderManager::GetBase();
	if (manager == nullptr) return;

	if (!IsWindowVisible()) return;

	struct ShaderDisplay {
		uint32_t address;
		std::wstring name;
		int countRef;
		ID3DXEffect* pEffect;
		uint32_t parameters;
		uint32_t techniques;
	};

	if (manager) {
		StaticLock lock = StaticLock();

		std::vector<ShaderDisplay> listShaderDisp;
		{
			int iData = 0;
			auto _AddData = [&](ID3DXEffect* pEffect, const std::wstring& name, uint32_t dataAddress, int ref) {
				D3DXEFFECT_DESC desc;
				pEffect->GetDesc(&desc);

				ShaderDisplay shaderDisp;
				shaderDisp.address = dataAddress;
				shaderDisp.name = name;
				shaderDisp.countRef = ref;
				shaderDisp.pEffect = pEffect;
				shaderDisp.parameters = desc.Parameters;
				shaderDisp.techniques = desc.Techniques;

				listShaderDisp[iData++] = shaderDisp;
			};

			if (RenderShaderLibrary* renderLib = manager->GetRenderLib()) {
				static std::vector<std::pair<ID3DXEffect*, const std::string*>> listShader = {
					std::make_pair(renderLib->GetRender2DShader(), &ShaderSource::nameRender2D_),
					std::make_pair(renderLib->GetInstancing2DShader(), &ShaderSource::nameHwInstance2D_),
					std::make_pair(renderLib->GetInstancing3DShader(), &ShaderSource::nameHwInstance3D_),
					std::make_pair(renderLib->GetIntersectVisualShader1(), &ShaderSource::nameIntersectVisual1_),
					std::make_pair(renderLib->GetIntersectVisualShader2(), &ShaderSource::nameIntersectVisual2_)
				};

				listShaderDisp.resize(listShader.size());
				for (size_t i = 0; i < listShader.size(); ++i) {
					ID3DXEffect* pEffect = listShader[i].first;
					const std::string* name = listShader[i].second;

					_AddData(pEffect, StringUtility::ConvertMultiToWide(*name), (uint32_t)pEffect, 1);
				}
			}

			{
				auto& mapData = manager->mapShaderData_;
				listShaderDisp.resize(listShaderDisp.size() + mapData.size());

				for (auto itrMap = mapData.begin(); itrMap != mapData.end(); ++itrMap) {
					const std::wstring& name = itrMap->first;
					const shared_ptr<ShaderData>& data = itrMap->second;

					ID3DXEffect* pEffect = data->effect_;

					if (data->bText_)
						_AddData(pEffect, name, (uint32_t)data.get(), data.use_count());
					else {
						_AddData(pEffect, PathProperty::GetPathWithoutModuleDirectory(name),
							(uint32_t)data.get(), data.use_count());
					}
				}
			}
		}
		{
			int iRow = 0;
			for (; iRow < listShaderDisp.size(); ++iRow) {
				ShaderDisplay* data = &listShaderDisp[iRow];

				wndListView_.SetText(iRow, ROW_ADDRESS, StringUtility::Format(L"%08x", data->address));
				wndListView_.SetText(iRow, ROW_NAME, data->name);
				wndListView_.SetText(iRow, ROW_REFCOUNT, std::to_wstring(data->countRef));
				wndListView_.SetText(iRow, ROW_EFFECT, StringUtility::Format(L"%08x", (uint32_t)data->pEffect));
				wndListView_.SetText(iRow, ROW_PARAMETERS, std::to_wstring(data->parameters));
				wndListView_.SetText(iRow, ROW_TECHNIQUES, std::to_wstring(data->techniques));
			}

			for (; iRow < wndListView_.GetRowCount(); ++iRow)
				wndListView_.DeleteRow(iRow);
		}
	}
	else {
		wndListView_.Clear();
	}
}
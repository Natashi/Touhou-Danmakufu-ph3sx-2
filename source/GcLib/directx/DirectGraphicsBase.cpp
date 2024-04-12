#include "source/GcLib/pch.h"

#include "DxConstant.hpp"
#include "DirectGraphicsBase.hpp"

using namespace gstd;
using namespace directx;

//*******************************************************************
//EDirect3D9
//*******************************************************************
EDirect3D9::EDirect3D9() {
	pDirect3D_ = Direct3DCreate9(D3D_SDK_VERSION);
	if (pDirect3D_ == nullptr)
		throw wexception("Direct3DCreate9 error.");
}
EDirect3D9::~EDirect3D9() {
	ptr_release(pDirect3D_);
}

//*******************************************************************
//DirectGraphicsBase
//*******************************************************************
DirectGraphicsBase::DirectGraphicsBase() {
	pDevice_ = nullptr;
	pBackSurf_ = nullptr;
	pZBuffer_ = nullptr;

	ZeroMemory(&deviceCaps_, sizeof(deviceCaps_));
	deviceStatus_ = S_OK;

	hAttachedWindow_ = nullptr;
}
DirectGraphicsBase::~DirectGraphicsBase() {
	Release();
}

void DirectGraphicsBase::_ReleaseDxResource() {
	ptr_release(pZBuffer_);
	ptr_release(pBackSurf_);

	for (auto itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
		(*itr)->ReleaseDxResource();
	}
}
void DirectGraphicsBase::_RestoreDxResource() {
	pDevice_->GetRenderTarget(0, &pBackSurf_);
	pDevice_->GetDepthStencilSurface(&pZBuffer_);

	for (auto itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
		(*itr)->RestoreDxResource();
	}
}

void DirectGraphicsBase::_VerifyDeviceCaps() {
	std::vector<std::string> listError;
	std::vector<std::string> listWarning;

	if ((deviceCaps_.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE) == 0)
		listError.push_back("D3DPRESENT_INTERVAL_IMMEDIATE is unavailable");
	if ((deviceCaps_.PresentationIntervals & D3DPRESENT_INTERVAL_ONE) == 0)
		listWarning.push_back("V-Sync is unavailable");

	if (deviceCaps_.VertexShaderVersion < D3DVS_VERSION(2, 0)
		|| deviceCaps_.MaxVertexShaderConst < 4)
		listError.push_back("The device's vertex shader support is insufficient (vs_2_0 required)");
	else if (deviceCaps_.VertexShaderVersion < D3DVS_VERSION(3, 0))
		listWarning.push_back("The device's vertex shader support is insufficient (vs_3_0 recommended)");

	if (deviceCaps_.NumSimultaneousRTs < 1)
		listError.push_back("Device must support at least 1 render target");

	//-------------------------------------------------------------------------------

	_VerifyDeviceCaps_Result(listError, listWarning);
}
void DirectGraphicsBase::_VerifyDeviceCaps_Result(const std::vector<std::string>& err, const std::vector<std::string>& warn) {
	if (err.size() > 0) {
		std::string strAll = "The game cannot start as the\r\n"
			"Direct3D device has the following issue(s):\r\n";
		for (auto& str : err)
			strAll += "   - " + str + "\r\n";
		strAll += "Try restarting in reference rasterizer mode\r\n";
		Logger::WriteError(strAll);
		throw wexception(strAll);
	}
	else if (warn.size() > 0) {
		std::string strAll = "The game's rendering might behave strangely as the\r\n"
			"Direct3D device has the following issue(s):\r\n";
		for (auto& str : warn)
			strAll += "   - " + str + "\r\n";
		Logger::WriteWarn(strAll);
	}
}

std::vector<std::wstring> DirectGraphicsBase::_GetRequiredModules() {
	return std::vector<std::wstring>({
		L"d3d9.dll", L"d3dx9_43.dll", L"d3dcompiler_43.dll",
		L"dsound.dll", L"dinput8.dll"
		});
}
void DirectGraphicsBase::_LoadModules() {
	HANDLE hCurrentProcess = ::GetCurrentProcess();

	std::vector<std::wstring> moduleNames = _GetRequiredModules();
	for (auto& iModule : moduleNames) {
		HMODULE hModule = ::LoadLibraryW(iModule.c_str());
		if (hModule == nullptr)
			throw gstd::wexception(L"Failed to load module: " + iModule);
		mapDxModules_[iModule] = hModule;
	}
}
void DirectGraphicsBase::_FreeModules() {
	for (auto itr = mapDxModules_.begin(); itr != mapDxModules_.end(); ++itr) {
		HMODULE pModule = itr->second;
		if (pModule)
			::FreeLibrary(pModule);
	}
	mapDxModules_.clear();
}

void DirectGraphicsBase::Release() {
	ptr_release(pZBuffer_);
	ptr_release(pBackSurf_);
	ptr_release(pDevice_);

	_FreeModules();
}

void DirectGraphicsBase::AddDirectGraphicsListener(DirectGraphicsListener* listener) {
	for (auto itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
		if ((*itr) == listener)
			return;
	}
	listListener_.push_back(listener);
}
void DirectGraphicsBase::RemoveDirectGraphicsListener(DirectGraphicsListener* listener) {
	listListener_.remove(listener);
}

bool DirectGraphicsBase::BeginScene(bool bClear = true) {
	return SUCCEEDED(pDevice_->BeginScene());
}
void DirectGraphicsBase::EndScene(bool bPresent = true) {
	pDevice_->EndScene();

	if (bPresent) {
		deviceStatus_ = pDevice_->Present(nullptr, nullptr, nullptr, nullptr);
		if (FAILED(deviceStatus_)) {
			if (_Restore()) {
				ResetDeviceState();
			}
		}
	}
}
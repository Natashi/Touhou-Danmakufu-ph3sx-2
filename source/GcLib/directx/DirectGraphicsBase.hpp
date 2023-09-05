#pragma once

#include "../pch.h"

#include "DxTypes.hpp"

namespace directx {
	//*******************************************************************
	//DirectGraphicsBase
	//*******************************************************************
	class DirectGraphicsBase {
	protected:
		std::map<std::wstring, HMODULE> mapDxModules_;

		IDirect3DDevice9* pDevice_;
		IDirect3DSurface9* pBackSurf_;
		IDirect3DSurface9* pZBuffer_;

		D3DCAPS9 deviceCaps_;
		HRESULT deviceStatus_;

		HWND hAttachedWindow_;
	protected:
		std::list<DirectGraphicsListener*> listListener_;

		D3DVIEWPORT9 viewPort_;
		D3DXMATRIX matViewPort_;
	protected:
		virtual void _ReleaseDxResource();
		virtual void _RestoreDxResource();
		virtual bool _Restore() = 0;

		virtual void _VerifyDeviceCaps();
		void _VerifyDeviceCaps_Result(const std::vector<std::string>& err, const std::vector<std::string>& warn);

		virtual std::vector<std::wstring> _GetRequiredModules();
		void _LoadModules();
		void _FreeModules();
	public:
		DirectGraphicsBase();
		virtual ~DirectGraphicsBase();

		virtual bool Initialize(HWND hWnd) = 0;
		virtual void Release();

		void AddDirectGraphicsListener(DirectGraphicsListener* listener);
		void RemoveDirectGraphicsListener(DirectGraphicsListener* listener);

		IDirect3DDevice9* GetDevice() { return pDevice_; }
		IDirect3DSurface9* GetBaseSurface() { return pBackSurf_; }
		IDirect3DSurface9* GetZBufferSurface() { return pZBuffer_; }
		HWND GetAttachedWindowHandle() { return hAttachedWindow_; }

		D3DCAPS9* GetDeviceCaps() { return &deviceCaps_; }
		HRESULT GetDeviceStatus() { return deviceStatus_; }

		virtual bool BeginScene(bool bClear);
		virtual void EndScene(bool bPresent);

		virtual void ResetDeviceState() = 0;
	};
}

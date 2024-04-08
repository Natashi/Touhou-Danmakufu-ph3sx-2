#pragma once

#include "../pch.h"
#include "DxConstant.hpp"

#include "DirectGraphicsBase.hpp"

#if defined(DNH_PROJ_EXECUTOR)
#include "VertexBuffer.hpp"
#endif

namespace directx {
#if defined(DNH_PROJ_EXECUTOR)
	class DxCamera;
	class DxCamera2D;
	class Texture;
	class TextureData;
	class Shader;
#endif

	//*******************************************************************
	//DirectGraphicsConfig
	//*******************************************************************
	class DirectGraphicsConfig {
	public:
		bool bShowWindow;
		bool bShowCursor;

		bool bWindowed;
		bool bBorderlessFullscreen;

		std::array<size_t, 2> sizeScreen;
		std::array<size_t, 2> sizeScreenDisplay;

		ColorMode colorMode;
		D3DMULTISAMPLE_TYPE typeMultiSample;

		bool bUseRef;
		bool bUseTripleBuffer;
		bool bVSync;

		bool bCheckDeviceCaps;
	public:
		DirectGraphicsConfig();
	};

#if defined(DNH_PROJ_EXECUTOR)
	struct DisplaySettings {
		D3DXMATRIX matDisplay;
		shared_ptr<Shader> shader;
	};

	class SystemInfoPanel;
	class DirectGraphics : public DirectGraphicsBase {
		static DirectGraphics* thisBase_;
	public:
		static float g_dxCoordsMul_;
	protected:
		D3DPRESENT_PARAMETERS d3dppFull_;
		D3DPRESENT_PARAMETERS d3dppWin_;

		DirectGraphicsConfig config_;

		shared_ptr<SystemInfoPanel> panelSystem_;
	protected:
		static constexpr DWORD wndStyleFull_ = WS_POPUP;
		static constexpr DWORD wndStyleWin_ = WS_OVERLAPPEDWINDOW - WS_SIZEBOX;
	protected:
		ScreenMode modeScreen_;

		std::map<D3DMULTISAMPLE_TYPE, std::array<bool, 2>> mapSupportMultisamples_;

		bool bMainRender_;
		bool bAllowRenderTargetChange_;
		BlendMode previousBlendMode_;

		gstd::ref_count_ptr<DxCamera> camera_;
		gstd::ref_count_ptr<DxCamera2D> camera2D_;

		D3DVIEWPORT9 viewPort_;
		D3DXMATRIX matViewPort_;

		//-----------------------------------------------------------

		shared_ptr<TextureData> defaultBackBufferRenderTarget_;
		shared_ptr<Texture> currentRenderTarget_;

		size_t defaultRenderTargetSize_[2];

		DisplaySettings displaySettingsWindowed_;
		DisplaySettings displaySettingsFullscreen_;

		//-----------------------------------------------------------

		VertexBufferManager* bufferManager_;
		VertexFogState stateFog_;

		//-----------------------------------------------------------

		virtual void _RestoreDxResource();
		virtual bool _Restore();
		bool _Reset();

		virtual void _VerifyDeviceCaps();
	public:
		DirectGraphics();
		virtual ~DirectGraphics();

		virtual bool Initialize(HWND hWnd);
		virtual bool Initialize(HWND hWnd, const DirectGraphicsConfig& config);
		virtual void Release();

		static DirectGraphics* GetBase() { return thisBase_; }

		ScreenMode GetScreenMode() { return modeScreen_; }
		D3DPRESENT_PARAMETERS GetFullScreenPresentParameter() { return d3dppFull_; }
		D3DPRESENT_PARAMETERS GetWindowPresentParameter() { return d3dppWin_; }

		const DirectGraphicsConfig& GetGraphicsConfig() { return config_; }

		void SetSystemPanel(shared_ptr<SystemInfoPanel> panel) { panelSystem_ = panel; }

		void ResetCamera();
		void ResetDeviceState();
		void ResetDisplaySettings();

		bool BeginScene(bool bMainRender, bool bClear);
		virtual bool BeginScene(bool bClear);
		virtual void EndScene(bool bPresent);

		//-----------------------------------------------------------

		void ClearRenderTarget();
		void ClearRenderTarget(DxRect<LONG>* rect);

		void SetDefaultBackBufferRenderTarget(shared_ptr<TextureData> texture) { defaultBackBufferRenderTarget_ = texture; }
		shared_ptr<TextureData> GetDefaultBackBufferRenderTarget() { return defaultBackBufferRenderTarget_; }

		void SetRenderTarget(shared_ptr<Texture> texture);
		void SetRenderTargetNull();
		shared_ptr<Texture> GetRenderTarget() { return currentRenderTarget_; }
		UINT* GetDefaultRenderTargetSize() { return defaultRenderTargetSize_; }

		DisplaySettings* GetDisplaySettingsWindowed() { return &displaySettingsWindowed_; }
		DisplaySettings* GetDisplaySettingsFullscreen() { return &displaySettingsFullscreen_; }
		DisplaySettings* GetDisplaySettings() {
			return GetScreenMode() == ScreenMode::SCREENMODE_WINDOW ?
				&displaySettingsWindowed_ : &displaySettingsFullscreen_;
		}

		void UpdateDefaultRenderTargetSize();

		//-----------------------------------------------------------

		//Render states
		void SetLightingEnable(bool bEnable);
		void SetSpecularEnable(bool bEnable);
		void SetCullingMode(DWORD mode);
		void SetShadingMode(DWORD mode);
		void SetZBufferEnable(bool bEnable);
		void SetZWriteEnable(bool bEnable);
		void SetAlphaTest(bool bEnable, DWORD ref, D3DCMPFUNC func);
		void SetBlendMode(BlendMode mode, int stage = 0);
		BlendMode GetBlendMode() { return previousBlendMode_; }
		void SetFillMode(DWORD mode);
		void SetFogEnable(bool bEnable);
		bool IsFogEnable();
		void SetVertexFog(bool bEnable, D3DCOLOR color, float start, float end);
		void SetPixelFog(bool bEnable, D3DCOLOR color, float start, float end);

		void SetTextureFilter(D3DTEXTUREFILTERTYPE fMin, D3DTEXTUREFILTERTYPE fMag,
			D3DTEXTUREFILTERTYPE fMip, int stage = 0);
		DWORD GetTextureFilter(D3DTEXTUREFILTERTYPE* fMin, D3DTEXTUREFILTERTYPE* fMag,
			D3DTEXTUREFILTERTYPE* fMip, int stage = 0);

		bool IsMainRenderLoop() { return bMainRender_; }
		void SetAllowRenderTargetChange(bool b) { bAllowRenderTargetChange_ = b; }
		bool IsAllowRenderTargetChange() { return bAllowRenderTargetChange_; }

		VertexFogState* GetFogState() { return &stateFog_; }

		void SetDirectionalLight(D3DVECTOR& dir);
		void SetMultiSampleType(D3DMULTISAMPLE_TYPE type);
		D3DMULTISAMPLE_TYPE GetMultiSampleType();
		HRESULT SetAntiAliasing(bool bEnable);
		bool IsSupportMultiSample(D3DMULTISAMPLE_TYPE type, bool bWindowed);

		//because D3DXMatrixOrthoOffCenterRH is broken for some reason
		static D3DXMATRIX CreateOrthographicProjectionMatrix(float x, float y, float width, float height);
		void SetViewPort(int x, int y, int width, int height);
		void ResetViewPort();
		const D3DXMATRIX& GetViewPortMatrix() { return matViewPort_; }

		size_t GetScreenWidth() { return config_.sizeScreen[0]; }
		size_t GetScreenHeight() { return config_.sizeScreen[1]; }
		size_t GetRenderScreenWidth() {
			return config_.sizeScreenDisplay[0];
		}
		size_t GetRenderScreenHeight() {
			return config_.sizeScreenDisplay[1];
		}

		double GetScreenWidthRatio();
		double GetScreenHeightRatio();
		POINT GetMousePosition();
		static DxRect<LONG> ClientSizeToWindowSize(const DxRect<LONG>& rc, ScreenMode mode);

		const gstd::ref_count_ptr<DxCamera>& GetCamera() { return camera_; }
		const gstd::ref_count_ptr<DxCamera2D>& GetCamera2D() { return camera2D_; }

		void SaveBackSurfaceToFile(const std::wstring& path);
	};

	//*******************************************************************
	//DirectGraphicsPrimaryWindow
	//*******************************************************************
	class DirectGraphicsPrimaryWindow : public DirectGraphics, public gstd::WindowBase {
	protected:
		gstd::WindowBase wndGraphics_;
		HCURSOR lpCursor_;

		HWND hWndParent_;
		HWND hWndContent_;

		ScreenMode newScreenMode_;

		bool bWindowMoveEnable_;
		POINT cPosOffset_;
	protected:
		virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void _PauseDrawing();
		void _RestartDrawing();

		void _StartWindowMove(LPARAM lParam);
		void _StopWindowMove();
		void _WindowMove();
	public:
		DirectGraphicsPrimaryWindow();
		virtual ~DirectGraphicsPrimaryWindow();

		virtual bool Initialize();
		virtual bool Initialize(DirectGraphicsConfig& config);

		void ChangeScreenMode();
		void ChangeScreenMode(ScreenMode newMode, bool bNoRepeated = true);

		HWND GetParentHWND() { return hWndParent_; }
		HWND GetContentHWND() { return hWndContent_; }
	};
#endif
}

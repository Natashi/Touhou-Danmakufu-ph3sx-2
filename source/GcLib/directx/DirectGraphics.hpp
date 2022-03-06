#pragma once

#include "../pch.h"
#include "DxConstant.hpp"

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
		bool bShowWindow_;
		bool bShowCursor_;

		bool bWindowed_;
		bool bBorderlessFullscreen_;

		POINT sizeScreen_;
		POINT sizeScreenDisplay_;
		bool bUseDynamicScaling_;

		ColorMode colorMode_;
		D3DMULTISAMPLE_TYPE typeMultiSample_;

		bool bUseRef_;
		bool bUseTripleBuffer_;
		bool bVSync_;

		bool bCheckDeviceCaps_;
	public:
		DirectGraphicsConfig();
	};

#if defined(DNH_PROJ_EXECUTOR)
	struct DxModules {
		HMODULE hLibrary_d3d9;
		HMODULE hLibrary_d3dx9;
		HMODULE hLibrary_d3dcompiler;
		HMODULE hLibrary_dinput8;
		HMODULE hLibrary_dsound;
	};
	struct DisplaySettings {
		D3DXMATRIX matDisplay;
		shared_ptr<Shader> shader;
	};

	class DirectGraphics {
		static DirectGraphics* thisBase_;
	public:
		static float g_dxCoordsMul_;
	protected:
		DxModules dxModules_;

		IDirect3D9* pDirect3D_;
		IDirect3DDevice9* pDevice_;
		D3DPRESENT_PARAMETERS d3dppFull_;
		D3DPRESENT_PARAMETERS d3dppWin_;
		IDirect3DSurface9* pBackSurf_;
		IDirect3DSurface9* pZBuffer_;

		D3DCAPS9 deviceCaps_;
		HRESULT deviceStatus_;

		DirectGraphicsConfig config_;
		HWND hAttachedWindow_;
	protected:
		static constexpr DWORD wndStyleFull_ = WS_POPUP;
		static constexpr DWORD wndStyleWin_ = WS_OVERLAPPEDWINDOW - WS_SIZEBOX;
	protected:
		ScreenMode modeScreen_;
		std::list<DirectGraphicsListener*> listListener_;

		std::map<D3DMULTISAMPLE_TYPE, std::pair<bool, DWORD*>> mapSupportMultisamples_;

		bool bMainRender_;
		bool bAllowRenderTargetChange_;
		BlendMode previousBlendMode_;

		gstd::ref_count_ptr<DxCamera> camera_;
		gstd::ref_count_ptr<DxCamera2D> camera2D_;

		D3DVIEWPORT9 viewPort_;
		D3DXMATRIX matViewPort_;

		shared_ptr<TextureData> defaultBackBufferRenderTarget_;
		shared_ptr<Texture> currentRenderTarget_;
		UINT defaultRenderTargetSize_[2];

		DisplaySettings displaySettingsWindowed_;
		DisplaySettings displaySettingsFullscreen_;

		VertexBufferManager* bufferManager_;
		VertexFogState stateFog_;

		void _ReleaseDxResource();
		void _RestoreDxResource();
		bool _Restore();

		void _VerifyDeviceCaps();

		void _LoadModules();
		void _FreeModules();
	public:
		DirectGraphics();
		virtual ~DirectGraphics();

		static DirectGraphics* GetBase() { return thisBase_; }

		HWND GetAttachedWindowHandle() { return hAttachedWindow_; }

		virtual bool Initialize(HWND hWnd);
		virtual bool Initialize(HWND hWnd, const DirectGraphicsConfig& config);

		void AddDirectGraphicsListener(DirectGraphicsListener* listener);
		void RemoveDirectGraphicsListener(DirectGraphicsListener* listener);
		ScreenMode GetScreenMode() { return modeScreen_; }
		D3DPRESENT_PARAMETERS GetFullScreenPresentParameter() { return d3dppFull_; }
		D3DPRESENT_PARAMETERS GetWindowPresentParameter() { return d3dppWin_; }

		const DirectGraphicsConfig& GetConfigData() { return config_; }
		IDirect3DDevice9* GetDevice() { return pDevice_; }

		IDirect3DSurface9* GetBaseSurface() { return pBackSurf_; }

		D3DCAPS9* GetDeviceCaps() { return &deviceCaps_; }
		HRESULT GetDeviceStatus() { return deviceStatus_; }

		void ResetCamera();
		void ResetDeviceState();
		void ResetDisplaySettings();

		void BeginScene(bool bMainRender = true, bool bClear = true);
		void EndScene(bool bPresent = true);

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
		void SetMultiSampleQuality(DWORD* quality);
		DWORD* GetMultiSampleQuality();
		HRESULT SetFullscreenAntiAliasing(bool bEnable);
		bool IsSupportMultiSample(D3DMULTISAMPLE_TYPE type);

		//because D3DXMatrixOrthoOffCenterRH is broken for some reason
		static D3DXMATRIX CreateOrthographicProjectionMatrix(float x, float y, float width, float height);
		void SetViewPort(int x, int y, int width, int height);
		void ResetViewPort();
		const D3DXMATRIX& GetViewPortMatrix() { return matViewPort_; }

		size_t GetScreenWidth() { return config_.sizeScreen_.x; }
		size_t GetScreenHeight() { return config_.sizeScreen_.y; }
		size_t GetRenderScreenWidth() {
			return config_.bUseDynamicScaling_ ? config_.sizeScreenDisplay_.x : config_.sizeScreen_.x;
		}
		size_t GetRenderScreenHeight() {
			return config_.bUseDynamicScaling_ ? config_.sizeScreenDisplay_.y : config_.sizeScreen_.y;
		}

		double GetScreenWidthRatio();
		double GetScreenHeightRatio();
		POINT GetMousePosition();
		static DxRect<LONG> ClientSizeToWindowSize(const DxRect<LONG>& rc, ScreenMode mode);

		gstd::ref_count_ptr<DxCamera> GetCamera() { return camera_; }
		gstd::ref_count_ptr<DxCamera2D> GetCamera2D() { return camera2D_; }

		void SaveBackSurfaceToFile(const std::wstring& path);

		void UpdateDefaultRenderTargetSize();
	};

	//-----------------------------------------------------------------------------------------------

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
		~DirectGraphicsPrimaryWindow();

		virtual bool Initialize();
		virtual bool Initialize(DirectGraphicsConfig& config);

		void ChangeScreenMode();
		void ChangeScreenMode(ScreenMode newMode, bool bNoRepeated = true);

		HWND GetParentHWND() { return hWndParent_; }
		HWND GetContentHWND() { return hWndContent_; }
	};

	//*******************************************************************
	//DxCamera
	//*******************************************************************
	class DxCamera {
		D3DXVECTOR3 pos_;
		D3DXVECTOR3 camPos_;

		//D3DXVECTOR3 laPosEye_;
		D3DXVECTOR3 laPosLookAt_;

		float radius_;
		float angleAzimuth_;
		float angleElevation_;

		float yaw_;
		float pitch_;
		float roll_;

		float projWidth_;
		float projHeight_;
		float clipNear_;
		float clipFar_;

		D3DXMATRIX matView_;
		D3DXMATRIX matProjection_;

		D3DXMATRIX matViewProjection_;
		D3DXMATRIX matViewInverse_;
		D3DXMATRIX matViewTranspose_;
		D3DXMATRIX matProjectionInverse_;

		D3DXMATRIX matIdentity_;

		int modeCamera_;

		std::list<D3DXMATRIX> listMatrixState_;
	public:
		enum {
			MODE_NORMAL,
			MODE_LOOKAT
		};

		bool thisViewChanged_;
		bool thisProjectionChanged_;

		DxCamera();
		virtual ~DxCamera();

		void Reset();

		const D3DXVECTOR3& GetCameraPosition() { return camPos_; }
		const D3DXVECTOR3& GetFocusPosition() { return pos_; }
		void SetFocus(float x, float y, float z) { pos_.x = x; pos_.y = y; pos_.z = z; }
		void SetFocusX(float x) { pos_.x = x; }
		void SetFocusY(float y) { pos_.y = y; }
		void SetFocusZ(float z) { pos_.z = z; }
		float GetRadius() { return radius_; }
		void SetRadius(float r) { radius_ = r; }
		float GetAzimuthAngle() { return angleAzimuth_; }
		void SetAzimuthAngle(float angle) { angleAzimuth_ = angle; }
		float GetElevationAngle() { return angleElevation_; }
		void SetElevationAngle(float angle) { angleElevation_ = angle; }

		void SetCameraLookAtVector(D3DXVECTOR3 vec) { laPosLookAt_ = vec; }
		//void SetCameraEyeVector(D3DXVECTOR3 vec) { laPosEye_ = vec; }

		float GetYaw() { return yaw_; }
		void SetYaw(float yaw) { yaw_ = yaw; }
		float GetPitch() { return pitch_; }
		void SetPitch(float pitch) { pitch_ = pitch; }
		float GetRoll() { return roll_; }
		void SetRoll(float roll) { roll_ = roll; }

		void SetPerspectiveWidth(float w) { projWidth_ = w; }
		void SetPerspectiveHeight(float h) { projHeight_ = h; }
		void SetPerspectiveClip(float pNear, float pFar) {
			if (pNear < 1.0f) pNear = 1.0f;
			if (pFar < 1.0f) pFar = 1.0f;
			clipNear_ = pNear;
			clipFar_ = pFar;
		}
		float GetNearClip() { return clipNear_; }
		float GetFarClip() { return clipFar_; }

		void SetCameraMode(int mode) { modeCamera_ = mode; }
		int GetCameraMode() { return modeCamera_; }

		D3DXMATRIX GetMatrixLookAtLH();
		void SetWorldViewMatrix();
		void SetProjectionMatrix();
		void UpdateDeviceViewProjectionMatrix();

		D3DXVECTOR2 TransformCoordinateTo2D(D3DXVECTOR3 pos);

		const D3DXMATRIX& GetViewMatrix() { return matView_; }
		const D3DXMATRIX& GetViewProjectionMatrix() { return matViewProjection_; }
		const D3DXMATRIX& GetViewInverseMatrix() { return matViewInverse_; }
		const D3DXMATRIX& GetViewTransposedMatrix() { return matViewTranspose_; }
		const D3DXMATRIX& GetProjectionMatrix() { return matProjection_; }
		const D3DXMATRIX& GetProjectionInverseMatrix() { return matProjectionInverse_; }

		const D3DXMATRIX& GetIdentity() { return matIdentity_; }

		void PushMatrixState();
		void PopMatrixState();
	};

	//*******************************************************************
	//DxCamera2D
	//*******************************************************************
	class DxCamera2D {
	private:
		bool bEnable_;

		D3DXVECTOR2 pos_;

		float ratioX_;
		float ratioY_;
		double angleZ_;

		DxRect<LONG> rcClip_;

		gstd::ref_count_ptr<D3DXVECTOR2> posReset_;

		D3DXMATRIX matCamera_;
		D3DXMATRIX matIdentity_;
	public:
		DxCamera2D();
		virtual ~DxCamera2D();

		bool IsEnable() { return bEnable_; }
		void SetEnable(bool bEnable) { bEnable_ = bEnable; }

		const D3DXVECTOR2& GetFocusPosition() { return pos_; }
		float GetFocusX() { return pos_.x; }
		float GetFocusY() { return pos_.y; }
		void SetFocus(float x, float y) { pos_.x = x; pos_.y = y; }
		void SetFocus(const D3DXVECTOR2& pos) { pos_ = pos; }
		void SetFocusX(float x) { pos_.x = x; }
		void SetFocusY(float y) { pos_.y = y; }
		float GetRatio() { return std::max(ratioX_, ratioY_); }
		void SetRatio(float ratio) { ratioX_ = ratio; ratioY_ = ratio; }
		float GetRatioX() { return ratioX_; }
		void SetRatioX(float ratio) { ratioX_ = ratio; }
		float GetRatioY() { return ratioY_; }
		void SetRatioY(float ratio) { ratioY_ = ratio; }
		double GetAngleZ() { return angleZ_; }
		void SetAngleZ(double angle) { angleZ_ = angle; }

		const DxRect<LONG>& GetClip() { return rcClip_; }
		void SetClip(const DxRect<LONG>& rect) { rcClip_ = rect; }

		void SetResetFocus(gstd::ref_count_ptr<D3DXVECTOR2>& pos) { posReset_ = pos; }
		void Reset();
		void ResetAll() {
			posReset_ = nullptr;
			Reset();
		}

		inline D3DXVECTOR2 GetLeftTopPosition();
		inline static D3DXVECTOR2 GetLeftTopPosition(const D3DXVECTOR2& focus, float ratio);
		inline static D3DXVECTOR2 GetLeftTopPosition(const D3DXVECTOR2& focus, float ratioX, float ratioY);
		inline static D3DXVECTOR2 GetLeftTopPosition(const D3DXVECTOR2& focus, float ratioX, float ratioY, const DxRect<LONG>& rcClip);

		void UpdateMatrix();
		const D3DXMATRIX& GetMatrix() { return bEnable_ ? matCamera_ : matIdentity_; }
	};
#endif
}

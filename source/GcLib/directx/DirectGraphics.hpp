#ifndef __DIRECTX_DIRECTGRAPHICS__
#define __DIRECTX_DIRECTGRAPHICS__

#include "../pch.h"
#include "DxConstant.hpp"

#if defined(DNH_PROJ_EXECUTOR)
#include "VertexBuffer.hpp"
#endif

namespace directx {
	struct VertexFogState {
		bool bEnable;
		D3DXVECTOR3 color;
		D3DXVECTOR2 fogDist;
	};

	class DxCamera;
	class DxCamera2D;
	class Texture;
	/**********************************************************
	//DirectGraphicsConfig
	**********************************************************/
	class DirectGraphicsConfig {
	public:
		enum {
			COLOR_MODE_32BIT,
			COLOR_MODE_16BIT,
		};
	protected:
		bool bShowWindow_;
		bool bShowCursor_;
		int widthScreen_;
		int heightScreen_;
		bool bWindowed_;
		bool bUseRef_;
		int colorMode_;
		bool bUseTripleBuffer_;
		bool bVSync_;
		bool bPseudoFullScreen_;
		D3DMULTISAMPLE_TYPE typeSamples_;
	public:
		DirectGraphicsConfig();
		virtual ~DirectGraphicsConfig();
		bool IsShowWindow() { return bShowWindow_; }
		void SetShowWindow(bool b) { bShowWindow_ = b; }
		bool IsShowCursor() { return bShowCursor_; }
		void SetShowCursor(bool b) { bShowCursor_ = b; }
		int GetScreenWidth() { return widthScreen_; }
		void SetScreenWidth(int width) { widthScreen_ = width; }
		int GetScreenHeight() { return heightScreen_; }
		void SetScreenHeight(int height) { heightScreen_ = height; }
		bool IsWindowed() { return bWindowed_; }
		void SetWindowd(bool bWindowed) { bWindowed_ = bWindowed; }
		bool IsReferenceEnable() { return bUseRef_; }
		void SetReferenceEnable(bool bEnable) { bUseRef_ = bEnable; }
		int GetColorMode() { return colorMode_; }
		void SetColorMode(int mode) { colorMode_ = mode; }
		bool IsTripleBufferEnable() { return bUseTripleBuffer_; }
		void SetTripleBufferEnable(bool bEnable) { bUseTripleBuffer_ = bEnable; }
		bool IsVSyncEnable() { return bVSync_; }
		void SetVSyncEnable(bool bEnable) { bVSync_ = bEnable; }
		bool IsPseudoFullScreen() { return bPseudoFullScreen_; }
		void SetbPseudoFullScreen(bool b) { bPseudoFullScreen_ = b; }
		D3DMULTISAMPLE_TYPE GetMultiSampleType() { return typeSamples_; }
		void SetMultiSampleType(D3DMULTISAMPLE_TYPE type) { typeSamples_ = type; }
	};

	class DirectGraphics {
		static DirectGraphics* thisBase_;
	public:
		enum {
			SCREENMODE_FULLSCREEN,
			SCREENMODE_WINDOW,
		};
		enum {
			MODE_BLEND_NONE,//なし
			MODE_BLEND_ALPHA,//αで半透明合成
			MODE_BLEND_ADD_RGB,//RGBで加算合成
			MODE_BLEND_ADD_ARGB,//αで加算合成
			MODE_BLEND_MULTIPLY,//乗算合成
			MODE_BLEND_SUBTRACT,//減算合成
			MODE_BLEND_SHADOW,//影描画用
			MODE_BLEND_INV_DESTRGB,//描画先色反転合成
			MODE_BLEND_ALPHA_INV,

			MODE_TEXTURE_FILTER_NONE,//フィルタなし
			MODE_TEXTURE_FILTER_POINT,//補間なし
			MODE_TEXTURE_FILTER_LINEAR,//線形補間
		};
	protected:
		IDirect3D9* pDirect3D_;
		IDirect3DDevice9* pDevice_;
		D3DPRESENT_PARAMETERS d3dppFull_;
		D3DPRESENT_PARAMETERS d3dppWin_;
		IDirect3DSurface9* pBackSurf_;
		IDirect3DSurface9* pZBuffer_;

		DirectGraphicsConfig config_;
		HWND hAttachedWindow_;
		DWORD wndStyleFull_;
		DWORD wndStyleWin_;
		int modeScreen_;
		std::list<DirectGraphicsListener*> listListener_;

		std::map<D3DMULTISAMPLE_TYPE, std::pair<bool, DWORD*>> mapSupportMultisamples_;

		bool bMainRender_;
		int previousBlendMode_;

#if defined(DNH_PROJ_EXECUTOR)
		gstd::ref_count_ptr<DxCamera> camera_;
		gstd::ref_count_ptr<DxCamera2D> camera2D_;
		shared_ptr<Texture> textureTarget_;

		VertexBufferManager* bufferManager_;

		VertexFogState stateFog_;

		void _ReleaseDxResource();
		void _RestoreDxResource();
		void _Restore();
		void _InitializeDeviceState(bool bResetCamera);
#endif
	public:
		DirectGraphics();
		virtual ~DirectGraphics();

		static DirectGraphics* GetBase() { return thisBase_; }

		HWND GetAttachedWindowHandle() { return hAttachedWindow_; }

		virtual bool Initialize(HWND hWnd);
		virtual bool Initialize(HWND hWnd, DirectGraphicsConfig& config);
		void AddDirectGraphicsListener(DirectGraphicsListener* listener);
		void RemoveDirectGraphicsListener(DirectGraphicsListener* listener);
		int GetScreenMode() { return modeScreen_; }
		D3DPRESENT_PARAMETERS GetFullScreenPresentParameter() { return d3dppFull_; }
		D3DPRESENT_PARAMETERS GetWindowPresentParameter() { return d3dppWin_; }

		IDirect3DDevice9* GetDevice() { return pDevice_; }
		DirectGraphicsConfig& GetConfigData() { return config_; }

		IDirect3DSurface9* GetBaseSurface() { return pBackSurf_; }

#if defined(DNH_PROJ_EXECUTOR)
		void BeginScene(bool bMainRender = true, bool bClear = true);	//描画開始
		void EndScene(bool bPresent = true);	//描画終了
		void ClearRenderTarget();
		void ClearRenderTarget(RECT* rect);
		void SetRenderTarget(shared_ptr<Texture> texture, bool bResetState = true);
		shared_ptr<Texture> GetRenderTarget() { return textureTarget_; }

		//レンダリングステートラッパ
		void SetLightingEnable(bool bEnable);//ライティング
		void SetSpecularEnable(bool bEnable);//スペキュラ
		void SetCullingMode(DWORD mode);//カリング
		void SetShadingMode(DWORD mode);//シェーディング
		void SetZBufferEnable(bool bEnable);//Zバッファ参照
		void SetZWriteEnable(bool bEnable);//Zバッファ書き込み
		void SetAlphaTest(bool bEnable, DWORD ref = 0, D3DCMPFUNC func = D3DCMP_GREATER);
		void SetBlendMode(DWORD mode, int stage = 0);
		DWORD GetBlendMode() { return previousBlendMode_; }
		void SetFillMode(DWORD mode);
		void SetFogEnable(bool bEnable);
		bool IsFogEnable();
		void SetVertexFog(bool bEnable, D3DCOLOR color, float start, float end);
		void SetPixelFog(bool bEnable, D3DCOLOR color, float start, float end);
		void SetTextureFilter(DWORD mode, int stage = 0);
		DWORD GetTextureFilter(int stage = 0);

		bool IsMainRenderLoop() { return bMainRender_; }
		VertexFogState* GetFogState() { return &stateFog_; }

		void SetDirectionalLight(D3DVECTOR& dir);
#endif
		void SetMultiSampleType(D3DMULTISAMPLE_TYPE type);
		D3DMULTISAMPLE_TYPE GetMultiSampleType();
		void SetMultiSampleQuality(DWORD* quality);
		DWORD* GetMultiSampleQuality();
		HRESULT SetFullscreenAntiAliasing(bool bEnable);
		bool IsSupportMultiSample(D3DMULTISAMPLE_TYPE type);

#if defined(DNH_PROJ_EXECUTOR)
		D3DXMATRIX matViewPort_;
		void SetViewPort(int x, int y, int width, int height);
		void ResetViewPort();
		D3DXMATRIX& GetViewPortMatrix() { return matViewPort_; }

		int GetScreenWidth();
		int GetScreenHeight();
		double GetScreenWidthRatio();
		double GetScreenHeightRatio();
		POINT GetMousePosition();

		gstd::ref_count_ptr<DxCamera> GetCamera() { return camera_; }
		gstd::ref_count_ptr<DxCamera2D> GetCamera2D() { return camera2D_; }

		void SaveBackSurfaceToFile(std::wstring path);
		bool IsPixelShaderSupported(int major, int minor);
#endif
	};

#if defined(DNH_PROJ_EXECUTOR)
	/**********************************************************
	//DirectGraphicsPrimaryWindow
	**********************************************************/
	class DirectGraphicsPrimaryWindow : public DirectGraphics, public gstd::WindowBase {
	protected:
		gstd::WindowBase wndGraphics_;
		virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);//オーバーライド用プロシージャ
		void _PauseDrawing();
		void _RestartDrawing();

		HCURSOR lpCursor_;
	public:
		DirectGraphicsPrimaryWindow();
		~DirectGraphicsPrimaryWindow();
		virtual bool Initialize();
		virtual bool Initialize(DirectGraphicsConfig& config);
		void ChangeScreenMode();
	};

	/**********************************************************
	//DxCamera
	**********************************************************/
	class DxCamera {
		D3DXVECTOR3 pos_;//焦点
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
		D3DXVECTOR3 GetCameraPosition() { return camPos_; }
		D3DXVECTOR3 GetFocusPosition() { return pos_; }
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

		D3DXMATRIX& GetViewMatrix() { return matView_; }
		D3DXMATRIX& GetViewProjectionMatrix() { return matViewProjection_; }
		D3DXMATRIX& GetViewInverseMatrix() { return matViewInverse_; }
		D3DXMATRIX& GetViewTransposedMatrix() { return matViewTranspose_; }
		D3DXMATRIX& GetProjectionMatrix() { return matProjection_; }
		D3DXMATRIX& GetProjectionInverseMatrix() { return matProjectionInverse_; }

		D3DXMATRIX& GetIdentity() { return matIdentity_; }
	};

	/**********************************************************
	//DxCamera2D
	**********************************************************/
	class DxCamera2D {
	public:

	private:
		bool bEnable_;
		D3DXVECTOR2 pos_;//焦点
		double ratioX_;//拡大率
		double ratioY_;
		double angleZ_;
		RECT rcClip_;//視野

		gstd::ref_count_ptr<D3DXVECTOR2> posReset_;

		D3DXMATRIX matCamera_;
		D3DXMATRIX matIdentity_;
	public:
		DxCamera2D();
		virtual ~DxCamera2D();

		bool IsEnable() { return bEnable_; }
		void SetEnable(bool bEnable) { bEnable_ = bEnable; }

		D3DXVECTOR2 GetFocusPosition() { return pos_; }
		float GetFocusX() { return pos_.x; }
		float GetFocusY() { return pos_.y; }
		void SetFocus(float x, float y) { pos_.x = x; pos_.y = y; }
		void SetFocus(D3DXVECTOR2 pos) { pos_ = pos; }
		void SetFocusX(float x) { pos_.x = x; }
		void SetFocusY(float y) { pos_.y = y; }
		double GetRatio() { return std::max(ratioX_, ratioY_); }
		void SetRatio(double ratio) { ratioX_ = ratio; ratioY_ = ratio; }
		double GetRatioX() { return ratioX_; }
		void SetRatioX(double ratio) { ratioX_ = ratio; }
		double GetRatioY() { return ratioY_; }
		void SetRatioY(double ratio) { ratioY_ = ratio; }
		double GetAngleZ() { return angleZ_; }
		void SetAngleZ(double angle) { angleZ_ = angle; }

		RECT GetClip() { return rcClip_; }
		void SetClip(RECT rect) { rcClip_ = rect; }

		void SetResetFocus(gstd::ref_count_ptr<D3DXVECTOR2>& pos) { posReset_ = pos; }
		void Reset();
		inline D3DXVECTOR2 GetLeftTopPosition();
		inline static D3DXVECTOR2 GetLeftTopPosition(D3DXVECTOR2 focus, double ratio);
		inline static D3DXVECTOR2 GetLeftTopPosition(D3DXVECTOR2 focus, double ratioX, double ratioY);
		inline static D3DXVECTOR2 GetLeftTopPosition(D3DXVECTOR2 focus, double ratioX, double ratioY, RECT rcClip);

		void UpdateMatrix();
		D3DXMATRIX& GetMatrix() { return bEnable_ ? matCamera_ : matIdentity_; }
	};
#endif
}

#endif

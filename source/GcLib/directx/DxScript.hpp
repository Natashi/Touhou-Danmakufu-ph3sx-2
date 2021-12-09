#pragma once

#include "../pch.h"

#include "../gstd/ScriptClient.hpp"

#include "DxConstant.hpp"
#include "DxObject.hpp"

namespace directx {
	//****************************************************************************
	//DxScriptResourceCache
	//****************************************************************************
	class DxScriptResourceCache {
		static DxScriptResourceCache* base_;
	public:
		std::map<std::wstring, shared_ptr<Texture>> mapTexture;
		std::map<std::wstring, shared_ptr<DxMesh>> mapMesh;
		std::map<std::wstring, shared_ptr<ShaderData>> mapShader;
		std::map<std::wstring, shared_ptr<SoundSourceData>> mapSound;
	public:
		DxScriptResourceCache();

		static DxScriptResourceCache* GetBase() { return base_; }

		void ClearResource();

		shared_ptr<Texture> GetTexture(const std::wstring& name);
		shared_ptr<DxMesh> GetMesh(const std::wstring& name);
		shared_ptr<ShaderData> GetShader(const std::wstring& name);
		shared_ptr<SoundSourceData> GetSound(const std::wstring& name);
	};

	//*******************************************************************
	//DxScript
	//*******************************************************************
	class DxScript : public gstd::ScriptClientBase {
	public:
		enum {
			ID_INVALID = -1,

			CODE_ACP = CP_ACP,
			CODE_UTF8 = CP_UTF8,
			CODE_UTF16LE,
			CODE_UTF16BE,
		};
		static double g_posInvalidX_;
		static double g_posInvalidY_;
		static double g_posInvalidZ_;
	protected:
		std::shared_ptr<DxScriptObjectManager> objManager_;

		DxScriptResourceCache* pResouceCache_;
	public:
		DxScript();
		virtual ~DxScript();

		void SetObjectManager(std::shared_ptr<DxScriptObjectManager> manager) { objManager_ = manager; }
		std::shared_ptr<DxScriptObjectManager> GetObjectManager() { return objManager_; }

		void SetMaxObject(size_t size) { objManager_->SetMaxObject(size); }
		void SetRenderBucketCapacity(int capacity) { objManager_->SetRenderBucketCapacity(capacity); }

		virtual int AddObject(ref_unsync_ptr<DxScriptObjectBase> obj, bool bActivate = true);
		virtual void ActivateObject(int id, bool bActivate) { objManager_->ActivateObject(id, bActivate); }

		ref_unsync_ptr<DxScriptObjectBase> GetObject(int id) { return objManager_->GetObject(id); }
		DxScriptObjectBase* GetObjectPointer(int id) { return objManager_->GetObjectPointer(id); }
		template<class T> T* GetObjectPointerAs(int id) { return dynamic_cast<T*>(GetObjectPointer(id)); }

		virtual void DeleteObject(int id) { objManager_->DeleteObject(id); }
		void ClearObject() { objManager_->ClearObject(); }

		virtual void WorkObject() { objManager_->WorkObject(); }
		virtual void RenderObject() { objManager_->RenderObject(); }

		//------------------------------------------------------------------------------------------

		DNH_FUNCAPI_DECL_(Func_MatrixIdentity);
		DNH_FUNCAPI_DECL_(Func_MatrixInverse);
		DNH_FUNCAPI_DECL_(Func_MatrixAdd);
		DNH_FUNCAPI_DECL_(Func_MatrixSubtract);
		DNH_FUNCAPI_DECL_(Func_MatrixMultiply);
		DNH_FUNCAPI_DECL_(Func_MatrixDivide);
		DNH_FUNCAPI_DECL_(Func_MatrixTranspose);
		DNH_FUNCAPI_DECL_(Func_MatrixDeterminant);
		DNH_FUNCAPI_DECL_(Func_MatrixLookatLH);
		DNH_FUNCAPI_DECL_(Func_MatrixLookatRH);
		DNH_FUNCAPI_DECL_(Func_MatrixTransformVector);

		//Dx関数：システム系
		static gstd::value Func_InstallFont(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Dx関数：音声系
		static gstd::value Func_LoadSound(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_RemoveSound(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_PlayBGM(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_PlaySE(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_StopSound(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_SetSoundDivisionVolumeRate);
		DNH_FUNCAPI_DECL_(Func_GetSoundDivisionVolumeRate);

		//Dx関数：キー系
		static gstd::value Func_GetKeyState(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetMouseX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetMouseY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetMouseMoveZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetMouseState(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetVirtualKeyState(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetVirtualKeyState(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetVirtualKeyMapping(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Dx関数：描画系
		DNH_FUNCAPI_DECL_(Func_GetMonitorWidth);
		DNH_FUNCAPI_DECL_(Func_GetMonitorHeight);
		static gstd::value Func_GetScreenWidth(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetScreenHeight(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_GetWindowedWidth);
		DNH_FUNCAPI_DECL_(Func_GetWindowedHeight);
		DNH_FUNCAPI_DECL_(Func_IsFullscreenMode);
		DNH_FUNCAPI_DECL_(Func_GetCoordinateScalingFactor);
		DNH_FUNCAPI_DECL_(Func_SetCoordinateScalingFactor);

		static gstd::value Func_LoadTexture(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_LoadTextureInLoadThread(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_LoadTextureEx);
		DNH_FUNCAPI_DECL_(Func_LoadTextureInLoadThreadEx);
		DNH_FUNCAPI_DECL_(Func_IsLoadThreadLoading);
		static gstd::value Func_RemoveTexture(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetTextureWidth(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetTextureHeight(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetFogEnable(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetFogParam(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_CreateRenderTarget(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_CreateRenderTargetEx);
		static gstd::value Func_SetRenderTarget(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ResetRenderTarget);
		DNH_FUNCAPI_DECL_(Func_ClearRenderTargetA1);
		DNH_FUNCAPI_DECL_(Func_ClearRenderTargetA2);
		DNH_FUNCAPI_DECL_(Func_ClearRenderTargetA3);
		static gstd::value Func_GetTransitionRenderTargetName(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SaveRenderedTextureA1(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SaveRenderedTextureA2(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_SaveRenderedTextureA3);

		DNH_FUNCAPI_DECL_(Func_LoadShader);
		DNH_FUNCAPI_DECL_(Func_RemoveShader);
		static gstd::value Func_SetShader(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetShaderI(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ResetShader(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ResetShaderI(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_IsPixelShaderSupported(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_IsVertexShaderSupported);
		DNH_FUNCAPI_DECL_(Func_SetEnableAntiAliasing);

		DNH_FUNCAPI_DECL_(Func_LoadMesh);
		DNH_FUNCAPI_DECL_(Func_RemoveMesh);

		//Dx関数：カメラ3D
		static gstd::value Func_SetCameraFocusX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetCameraFocusY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetCameraFocusZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetCameraFocusXYZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetCameraRadius(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetCameraAzimuthAngle(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetCameraElevationAngle(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetCameraYaw(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetCameraPitch(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetCameraRoll(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_SetCameraMode);
		//DNH_FUNCAPI_DECL_(Func_SetCameraPosEye);
		DNH_FUNCAPI_DECL_(Func_SetCameraPosLookAt);

		static gstd::value Func_GetCameraX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetCameraY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetCameraZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetCameraFocusX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetCameraFocusY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetCameraFocusZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetCameraRadius(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetCameraAzimuthAngle(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetCameraElevationAngle(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetCameraYaw(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetCameraPitch(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetCameraRoll(gstd::script_machine* machine, int argc, const gstd::value* argv);

		static gstd::value Func_SetCameraPerspectiveClip(gstd::script_machine* machine, int argc, const gstd::value* argv);

		DNH_FUNCAPI_DECL_(Func_GetCameraViewMatrix);
		DNH_FUNCAPI_DECL_(Func_GetCameraProjectionMatrix);
		DNH_FUNCAPI_DECL_(Func_GetCameraViewProjectionMatrix);

		//Dx関数：カメラ2D
		static gstd::value Func_Set2DCameraFocusX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Set2DCameraFocusY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Set2DCameraAngleZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Set2DCameraRatio(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Set2DCameraRatioX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Set2DCameraRatioY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Reset2DCamera(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Get2DCameraX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Get2DCameraY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Get2DCameraAngleZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Get2DCameraRatio(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Get2DCameraRatioX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Get2DCameraRatioY(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Dx関数：その他
		static gstd::value Func_GetObjectDistance(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetObjectDistanceSq(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetObjectDeltaAngle(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetObject2dPosition(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Get2dPosition(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Intersection
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Point_Polygon);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Point_Circle);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Point_Ellipse);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Point_Line);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Point_RegularPolygon);

		DNH_FUNCAPI_DECL_(Func_IsIntersected_Circle_Polygon);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Circle_Circle);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Circle_Ellipse);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Circle_RegularPolygon);

		DNH_FUNCAPI_DECL_(Func_IsIntersected_Line_Polygon);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Line_Circle);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Line_Ellipse);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Line_Line);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Line_RegularPolygon);

		DNH_FUNCAPI_DECL_(Func_IsIntersected_Polygon_Polygon);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Polygon_Ellipse);
		DNH_FUNCAPI_DECL_(Func_IsIntersected_Polygon_RegularPolygon);

		//Color
		DNH_FUNCAPI_DECL_(Func_ColorARGBToHex);
		DNH_FUNCAPI_DECL_(Func_ColorHexToARGB);
		DNH_FUNCAPI_DECL_(Func_ColorRGBtoHSV);
		DNH_FUNCAPI_DECL_(Func_ColorHexRGBtoHSV);
		DNH_FUNCAPI_DECL_(Func_ColorHSVtoRGB);
		DNH_FUNCAPI_DECL_(Func_ColorHSVtoHexRGB);

		//Other stuff
		DNH_FUNCAPI_DECL_(Func_SetInvalidPositionReturn);

		//Dx関数：オブジェクト操作(共通)
		static gstd::value Func_Obj_Create(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_Delete(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_QueueDelete(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_IsDeleted(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_IsExists(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_SetVisible(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_IsVisible(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_SetRenderPriority(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_SetRenderPriorityI(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_GetRenderPriority(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_GetRenderPriorityI(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_GetValue(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_SetValue(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_DeleteValue(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_IsValueExists(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_Obj_GetValueI);
		DNH_FUNCAPI_DECL_(Func_Obj_SetValueI);
		DNH_FUNCAPI_DECL_(Func_Obj_DeleteValueI);
		DNH_FUNCAPI_DECL_(Func_Obj_IsValueExistsI);
		DNH_FUNCAPI_DECL_(Func_Obj_CopyValueTable);
		DNH_FUNCAPI_DECL_(Func_Obj_GetValueCount);
		DNH_FUNCAPI_DECL_(Func_Obj_GetValueCountI);
		static gstd::value Func_Obj_GetExistFrame(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_GetType(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_GetParentScriptID(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_SetNewParentScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_Obj_SetAutoDelete(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Dx関数：オブジェクト操作(RenderObject)
		static gstd::value Func_ObjRender_SetX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetPosition(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetAngleX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetAngleY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetAngleZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetAngleXYZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetScaleX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetScaleY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetScaleZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetScaleXYZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjRender_SetColor);
		static gstd::value Func_ObjRender_SetColorHSV(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjRender_GetColor);
		DNH_FUNCAPI_DECL_(Func_ObjRender_GetColorHex);
		static gstd::value Func_ObjRender_SetAlpha(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjRender_GetAlpha);
		static gstd::value Func_ObjRender_SetBlendType(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_GetX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_GetY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_GetZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_GetPosition(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_GetAngleX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_GetAngleY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_GetAngleZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_GetScaleX(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_GetScaleY(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_GetScaleZ(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetZWrite(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetZTest(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetFogEnable(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetCullingMode(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetRelativeObject(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_SetPermitCamera(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjRender_GetBlendType(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjRender_SetTextureFilterMin);
		DNH_FUNCAPI_DECL_(Func_ObjRender_SetTextureFilterMag);
		DNH_FUNCAPI_DECL_(Func_ObjRender_SetTextureFilterMip);
		DNH_FUNCAPI_DECL_(Func_ObjRender_SetVertexShaderRenderingMode);
		DNH_FUNCAPI_DECL_(Func_ObjRender_SetEnableDefaultTransformMatrix);
		DNH_FUNCAPI_DECL_(Func_ObjRender_SetLightingEnable);
		DNH_FUNCAPI_DECL_(Func_ObjRender_SetLightingDiffuseColor);
		DNH_FUNCAPI_DECL_(Func_ObjRender_SetLightingSpecularColor);
		DNH_FUNCAPI_DECL_(Func_ObjRender_SetLightingAmbientColor);
		DNH_FUNCAPI_DECL_(Func_ObjRender_SetLightingDirection);

		//Dx関数：オブジェクト操作(ShaderObject)
		static gstd::value Func_ObjShader_Create(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjShader_SetShaderF(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjShader_SetShaderO(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjShader_SetShaderT(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjShader_ResetShader(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjShader_SetTechnique(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjShader_SetMatrix(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjShader_SetMatrixArray(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjShader_SetVector(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjShader_SetFloat(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjShader_SetFloatArray(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjShader_SetTexture(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Dx関数：オブジェクト操作(PrimitiveObject)
		static gstd::value Func_ObjPrimitive_Create(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjPrimitive_SetPrimitiveType(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjPrimitive_GetPrimitiveType(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjPrimitive_SetVertexCount(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjPrimitive_SetTexture(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjPrimitive_GetTexture);
		static gstd::value Func_ObjPrimitive_GetVertexCount(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjPrimitive_SetVertexPosition(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjPrimitive_SetVertexUV(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjPrimitive_SetVertexUVT(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjPrimitive_SetVertexColor);
		DNH_FUNCAPI_DECL_(Func_ObjPrimitive_SetVertexColorHSV);
		static gstd::value Func_ObjPrimitive_SetVertexAlpha(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjPrimitive_GetVertexColor);
		DNH_FUNCAPI_DECL_(Func_ObjPrimitive_GetVertexColorHex);
		DNH_FUNCAPI_DECL_(Func_ObjPrimitive_GetVertexAlpha);
		static gstd::value Func_ObjPrimitive_GetVertexPosition(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjPrimitive_SetVertexIndex);
		

		//Dx関数：オブジェクト操作(Sprite2D)
		static gstd::value Func_ObjSprite2D_SetSourceRect(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSprite2D_SetDestRect(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSprite2D_SetDestCenter(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Dx関数：オブジェクト操作(SpriteList2D)
		static gstd::value Func_ObjSpriteList2D_SetSourceRect(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSpriteList2D_SetDestRect(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSpriteList2D_SetDestCenter(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSpriteList2D_AddVertex(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSpriteList2D_CloseVertex(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSpriteList2D_ClearVertexCount(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjSpriteList2D_SetAutoClearVertexCount);

		//Dx関数：オブジェクト操作(Sprite3D)
		static gstd::value Func_ObjSprite3D_SetSourceRect(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSprite3D_SetDestRect(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSprite3D_SetSourceDestRect(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSprite3D_SetBillboard(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Dx関数：オブジェクト操作(TrajectoryObject3D)
		static gstd::value Func_ObjTrajectory3D_SetInitialPoint(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjTrajectory3D_SetAlphaVariation(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjTrajectory3D_SetComplementCount(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//DxScriptParticleListObject
		DNH_FUNCAPI_DECL_(Func_ObjParticleList_Create);
		DNH_FUNCAPI_DECL_(Func_ObjParticleList_SetPosition);
		template<size_t ID> DNH_FUNCAPI_DECL_(Func_ObjParticleList_SetScaleSingle);
		DNH_FUNCAPI_DECL_(Func_ObjParticleList_SetScaleXYZ);
		template<size_t ID> DNH_FUNCAPI_DECL_(Func_ObjParticleList_SetAngleSingle);
		DNH_FUNCAPI_DECL_(Func_ObjParticleList_SetAngle);
		DNH_FUNCAPI_DECL_(Func_ObjParticleList_SetColor);
		DNH_FUNCAPI_DECL_(Func_ObjParticleList_SetAlpha);
		DNH_FUNCAPI_DECL_(Func_ObjParticleList_SetExtraData);
		DNH_FUNCAPI_DECL_(Func_ObjParticleList_AddInstance);
		DNH_FUNCAPI_DECL_(Func_ObjParticleList_ClearInstance);

		//Dx関数：オブジェクト操作(DxMesh)
		static gstd::value Func_ObjMesh_Create(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjMesh_Load(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjMesh_SetColor(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjMesh_SetAlpha(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjMesh_SetAnimation(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjMesh_SetCoordinate2D(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjMesh_GetPath(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Dx関数：テキスト操作(DxText)
		static gstd::value Func_ObjText_Create(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetText(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetFontType(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetFontSize(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetFontBold(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjText_SetFontWeight);
		static gstd::value Func_ObjText_SetFontColor(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetFontColorTop(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetFontColorBottom(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetFontBorderWidth(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetFontBorderType(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetFontBorderColor(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjText_SetFontCharacterSet);
		static gstd::value Func_ObjText_SetMaxWidth(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetMaxHeight(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetLinePitch(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetSidePitch(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjText_SetFixedWidth);
		static gstd::value Func_ObjText_SetVertexColor(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetTransCenter(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetAutoTransCenter(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetHorizontalAlignment(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetVerticalAlignment(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_SetSyntacticAnalysis(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_GetText(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_GetTextLength(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_GetTextLengthCU(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_GetTextLengthCUL(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_GetTotalWidth(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjText_GetTotalHeight(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Dx関数：音声操作(DxSoundObject)
		static gstd::value Func_ObjSound_Create(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_Load(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_Play(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_Stop(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_SetVolumeRate(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_SetPanRate(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_SetFade(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_SetLoopEnable(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_SetLoopTime(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_SetLoopSampleCount(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjSound_Seek);
		DNH_FUNCAPI_DECL_(Func_ObjSound_SeekSampleCount);
		static gstd::value Func_ObjSound_SetResumeEnable(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_SetSoundDivision(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_IsPlaying(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjSound_GetVolumeRate(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjSound_SetFrequency);
		DNH_FUNCAPI_DECL_(Func_ObjSound_GetInfo);

		//Dx関数：ファイル操作(DxFileObject)
		static gstd::value Func_ObjFile_Create(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFile_Open(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFile_OpenNW(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFile_Store(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFile_GetSize(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Dx関数：ファイル操作(DxTextFileObject)
		static gstd::value Func_ObjFileT_GetLineCount(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileT_GetLineText(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_ObjFileT_SetLineText);
		static gstd::value Func_ObjFileT_SplitLineText(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileT_AddLine(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileT_ClearLine(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//Dx関数：ファイル操作(DxBinaryFileObject)
		static gstd::value Func_ObjFileB_SetByteOrder(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileB_SetCharacterCode(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileB_GetPointer(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileB_Seek(gstd::script_machine* machine, int argc, const gstd::value* argv);

		DNH_FUNCAPI_DECL_(Func_ObjFileB_GetLastRead);

		static gstd::value Func_ObjFileB_ReadBoolean(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileB_ReadByte(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileB_ReadShort(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileB_ReadInteger(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileB_ReadLong(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileB_ReadFloat(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileB_ReadDouble(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_ObjFileB_ReadString(gstd::script_machine* machine, int argc, const gstd::value* argv);

		DNH_FUNCAPI_DECL_(Func_ObjFileB_WriteBoolean);
		DNH_FUNCAPI_DECL_(Func_ObjFileB_WriteByte);
		DNH_FUNCAPI_DECL_(Func_ObjFileB_WriteShort);
		DNH_FUNCAPI_DECL_(Func_ObjFileB_WriteInteger);
		DNH_FUNCAPI_DECL_(Func_ObjFileB_WriteLong);
		DNH_FUNCAPI_DECL_(Func_ObjFileB_WriteFloat);
		DNH_FUNCAPI_DECL_(Func_ObjFileB_WriteDouble);
	};
}
#include "source/GcLib/pch.h"

#include "DxUtility.hpp"
#include "DirectGraphics.hpp"
#include "DirectGraphics.hpp"
#include "DxScript.hpp"
#include "DxObject.hpp"
#include "DirectInput.hpp"
#include "MetasequoiaMesh.hpp"
//#include "ElfreinaMesh.hpp"

using namespace gstd;
using namespace directx;

//****************************************************************************
//DxScriptResourceCache
//****************************************************************************
DxScriptResourceCache* DxScriptResourceCache::base_ = nullptr;
DxScriptResourceCache::DxScriptResourceCache() {
	if (base_)
		throw wexception("DxScriptResourceCache already instantiated");
	base_ = this;
}

void DxScriptResourceCache::ClearResource() {
	mapTexture.clear();
	mapMesh.clear();
	mapShader.clear();
	mapSound.clear();
}
shared_ptr<Texture> DxScriptResourceCache::GetTexture(const std::wstring& name) {
	auto itr = mapTexture.find(name);
	if (itr != mapTexture.end())
		return itr->second;
	return nullptr;
}
shared_ptr<DxMesh> DxScriptResourceCache::GetMesh(const std::wstring& name) {
	auto itr = mapMesh.find(name);
	if (itr != mapMesh.end())
		return itr->second;
	return nullptr;
}
shared_ptr<ShaderData> DxScriptResourceCache::GetShader(const std::wstring& name) {
	auto itr = mapShader.find(name);
	if (itr != mapShader.end())
		return itr->second;
	return nullptr;
}
shared_ptr<SoundSourceData> DxScriptResourceCache::GetSound(const std::wstring& name) {
	auto itr = mapSound.find(name);
	if (itr != mapSound.end())
		return itr->second;
	return nullptr;
}

//****************************************************************************
//DxScript
//****************************************************************************
static const std::vector<function> dxFunction = {
	//Matrix operations
	{ "MatrixIdentity", DxScript::Func_MatrixIdentity, 0 },
	{ "MatrixInverse", DxScript::Func_MatrixInverse, 1 },
	{ "MatrixAdd", DxScript::Func_MatrixAdd, 2 },
	{ "MatrixSubtract", DxScript::Func_MatrixSubtract, 2 },
	{ "MatrixMultiply", DxScript::Func_MatrixMultiply, 2 },
	{ "MatrixDivide", DxScript::Func_MatrixDivide, 2 },
	{ "MatrixTranspose", DxScript::Func_MatrixTranspose, 1 },
	{ "MatrixDeterminant", DxScript::Func_MatrixDeterminant, 1 },
	{ "MatrixLookAtLH", DxScript::Func_MatrixLookatLH, 3 },
	{ "MatrixLookAtRH", DxScript::Func_MatrixLookatRH, 3 },
	{ "MatrixTransformVector", DxScript::Func_MatrixTransformVector, 2 },

	//Sound functions
	{ "LoadSound", DxScript::Func_LoadSound, 1 },
	{ "RemoveSound", DxScript::Func_RemoveSound, 1 },
	{ "PlayBGM", DxScript::Func_PlayBGM, 3 },
	{ "PlaySE", DxScript::Func_PlaySE, 1 },
	{ "StopSound", DxScript::Func_StopSound, 1 },
	{ "SetSoundDivisionVolumeRate", DxScript::Func_SetSoundDivisionVolumeRate, 2 },
	{ "GetSoundDivisionVolumeRate", DxScript::Func_GetSoundDivisionVolumeRate, 1 },

	//Input functions
	{ "GetKeyState", DxScript::Func_GetKeyState, 1 },
	{ "GetMouseX", DxScript::Func_GetMouseX, 0 },
	{ "GetMouseY", DxScript::Func_GetMouseY, 0 },
	{ "GetMouseMoveZ", DxScript::Func_GetMouseMoveZ, 0 },
	{ "GetMouseState", DxScript::Func_GetMouseState, 1 },
	{ "GetVirtualKeyState", DxScript::Func_GetVirtualKeyState, 1 },
	{ "SetVirtualKeyState", DxScript::Func_SetVirtualKeyState, 2 },

	//Graphics functions
	{ "GetMonitorWidth", DxScript::Func_GetMonitorWidth, 0 },
	{ "GetMonitorHeight", DxScript::Func_GetMonitorHeight, 0 },
	{ "GetScreenWidth", DxScript::Func_GetScreenWidth, 0 },
	{ "GetScreenHeight", DxScript::Func_GetScreenHeight, 0 },
	{ "GetWindowedWidth", DxScript::Func_GetWindowedWidth, 0 },
	{ "GetWindowedHeight", DxScript::Func_GetWindowedHeight, 0 },
	{ "IsFullscreenMode", DxScript::Func_IsFullscreenMode, 0 },

	{ "LoadTexture", DxScript::Func_LoadTexture, 1 },
	{ "LoadTextureEx", DxScript::Func_LoadTextureEx, 3 },
	{ "LoadTextureInLoadThread", DxScript::Func_LoadTextureInLoadThread, 1 },
	{ "LoadTextureInLoadThreadEx", DxScript::Func_LoadTextureInLoadThreadEx, 3 },
	{ "RemoveTexture", DxScript::Func_RemoveTexture, 1 },
	{ "GetTextureWidth", DxScript::Func_GetTextureWidth, 1 },
	{ "GetTextureHeight", DxScript::Func_GetTextureHeight, 1 },
	
	{ "CreateRenderTarget", DxScript::Func_CreateRenderTarget, 1 },
	{ "CreateRenderTargetEx", DxScript::Func_CreateRenderTargetEx, 3 },
	//{ "SetRenderTarget", DxScript::Func_SetRenderTarget, 2 },
	//{ "ResetRenderTarget", DxScript::Func_ResetRenderTarget, 0 },
	{ "ClearRenderTargetA1", DxScript::Func_ClearRenderTargetA1, 1 },
	{ "ClearRenderTargetA2", DxScript::Func_ClearRenderTargetA2, 5 },
	{ "ClearRenderTargetA3", DxScript::Func_ClearRenderTargetA3, 9 },
	{ "GetTransitionRenderTargetName", DxScript::Func_GetTransitionRenderTargetName, 0 },
	{ "SaveRenderedTextureA1", DxScript::Func_SaveRenderedTextureA1, 2 },
	{ "SaveRenderedTextureA2", DxScript::Func_SaveRenderedTextureA2, 6 },
	{ "SaveRenderedTextureA3", DxScript::Func_SaveRenderedTextureA3, 7 },

	{ "IsPixelShaderSupported", DxScript::Func_IsPixelShaderSupported, 2 },
	{ "IsVertexShaderSupported", DxScript::Func_IsVertexShaderSupported, 2 },
	//{ "SetAntiAliasing", DxScript::Func_SetEnableAntiAliasing, 1 },

	//Fog
	{ "SetFogEnable", DxScript::Func_SetFogEnable, 1 },
	{ "SetFogParam", DxScript::Func_SetFogParam, 5 },
	{ "SetFogParam", DxScript::Func_SetFogParam, 3 },		//Overloaded

	//Font functions
	{ "InstallFont", DxScript::Func_InstallFont, 1 },

	//Shader functions
	{ "LoadShader", DxScript::Func_LoadShader, 1 },
	{ "RemoveShader", DxScript::Func_RemoveShader, 1 },
	{ "SetShader", DxScript::Func_SetShader, 3 },
	{ "SetShaderI", DxScript::Func_SetShaderI, 3 },
	{ "ResetShader", DxScript::Func_ResetShader, 2 },
	{ "ResetShaderI", DxScript::Func_ResetShaderI, 2 },

	//Mesh functions
	{ "LoadMesh", DxScript::Func_LoadMesh, 1 },
	{ "RemoveMesh", DxScript::Func_RemoveMesh, 1 },

	//3D camera functions
	{ "SetCameraFocusX", DxScript::Func_SetCameraFocusX, 1 },
	{ "SetCameraFocusY", DxScript::Func_SetCameraFocusY, 1 },
	{ "SetCameraFocusZ", DxScript::Func_SetCameraFocusZ, 1 },
	{ "SetCameraFocusXYZ", DxScript::Func_SetCameraFocusXYZ, 3 },
	{ "SetCameraRadius", DxScript::Func_SetCameraRadius, 1 },
	{ "SetCameraAzimuthAngle", DxScript::Func_SetCameraAzimuthAngle, 1 },
	{ "SetCameraElevationAngle", DxScript::Func_SetCameraElevationAngle, 1 },
	{ "SetCameraYaw", DxScript::Func_SetCameraYaw, 1 },
	{ "SetCameraPitch", DxScript::Func_SetCameraPitch, 1 },
	{ "SetCameraRoll", DxScript::Func_SetCameraRoll, 1 },
	{ "SetCameraMode", DxScript::Func_SetCameraMode, 1 },
	//{ "SetCameraPosEye", DxScript::Func_SetCameraPosEye, 3 },
	{ "SetCameraPosEye", DxScript::Func_SetCameraFocusXYZ, 3 },
	{ "SetCameraPosLookAt", DxScript::Func_SetCameraPosLookAt, 3 },
	{ "GetCameraX", DxScript::Func_GetCameraX, 0 },
	{ "GetCameraY", DxScript::Func_GetCameraY, 0 },
	{ "GetCameraZ", DxScript::Func_GetCameraZ, 0 },
	{ "GetCameraFocusX", DxScript::Func_GetCameraFocusX, 0 },
	{ "GetCameraFocusY", DxScript::Func_GetCameraFocusY, 0 },
	{ "GetCameraFocusZ", DxScript::Func_GetCameraFocusZ, 0 },
	{ "GetCameraRadius", DxScript::Func_GetCameraRadius, 0 },
	{ "GetCameraAzimuthAngle", DxScript::Func_GetCameraAzimuthAngle, 0 },
	{ "GetCameraElevationAngle", DxScript::Func_GetCameraElevationAngle, 0 },
	{ "GetCameraYaw", DxScript::Func_GetCameraYaw, 0 },
	{ "GetCameraPitch", DxScript::Func_GetCameraPitch, 0 },
	{ "GetCameraRoll", DxScript::Func_GetCameraRoll, 0 },
	{ "SetCameraPerspectiveClip", DxScript::Func_SetCameraPerspectiveClip, 2 },
	{ "GetCameraViewMatrix", DxScript::Func_GetCameraViewMatrix, 0 },
	{ "GetCameraProjectionMatrix", DxScript::Func_GetCameraProjectionMatrix, 0 },
	{ "GetCameraViewProjectionMatrix", DxScript::Func_GetCameraViewProjectionMatrix, 0 },

	//2D camera functions
	{ "Set2DCameraFocusX", DxScript::Func_Set2DCameraFocusX, 1 },
	{ "Set2DCameraFocusY", DxScript::Func_Set2DCameraFocusY, 1 },
	{ "Set2DCameraAngleZ", DxScript::Func_Set2DCameraAngleZ, 1 },
	{ "Set2DCameraRatio", DxScript::Func_Set2DCameraRatio, 1 },
	{ "Set2DCameraRatioX", DxScript::Func_Set2DCameraRatioX, 1 },
	{ "Set2DCameraRatioY", DxScript::Func_Set2DCameraRatioY, 1 },
	{ "Reset2DCamera", DxScript::Func_Reset2DCamera, 0 },
	{ "Get2DCameraX", DxScript::Func_Get2DCameraX, 0 },
	{ "Get2DCameraY", DxScript::Func_Get2DCameraY, 0 },
	{ "Get2DCameraAngleZ", DxScript::Func_Get2DCameraAngleZ, 0 },
	{ "Get2DCameraRatio", DxScript::Func_Get2DCameraRatio, 0 },
	{ "Get2DCameraRatioX", DxScript::Func_Get2DCameraRatioX, 0 },
	{ "Get2DCameraRatioY", DxScript::Func_Get2DCameraRatioY, 0 },

	//Position functions
	{ "GetObjectDistance", DxScript::Func_GetObjectDistance, 2 },
	{ "GetObjectDistanceSq", DxScript::Func_GetObjectDistanceSq, 2 },
	{ "GetObjectDeltaAngle", DxScript::Func_GetObjectDeltaAngle, 2 },
	{ "GetObject2dPosition", DxScript::Func_GetObject2dPosition, 1 },
	{ "Get2dPosition", DxScript::Func_Get2dPosition, 3 },

	//Intersection checking functions
	{ "IsIntersected_Point_Polygon", DxScript::Func_IsIntersected_Point_Polygon, 3 },
	{ "IsIntersected_Point_Circle", DxScript::Func_IsIntersected_Point_Circle, 5 },
	{ "IsIntersected_Point_Ellipse", DxScript::Func_IsIntersected_Point_Ellipse, 6 },
	{ "IsIntersected_Point_Line", DxScript::Func_IsIntersected_Point_Line, 7 },
	{ "IsIntersected_Point_RegularPolygon", DxScript::Func_IsIntersected_Point_RegularPolygon, 7 },

	{ "IsIntersected_Circle_Polygon", DxScript::Func_IsIntersected_Circle_Polygon, 4 },
	{ "IsIntersected_Circle_Circle", DxScript::Func_IsIntersected_Circle_Circle, 6 },
	{ "IsIntersected_Circle_Ellipse", DxScript::Func_IsIntersected_Circle_Ellipse, 7 },
	{ "IsIntersected_Circle_RegularPolygon", DxScript::Func_IsIntersected_Circle_RegularPolygon, 8 },

	{ "IsIntersected_Line_Polygon", DxScript::Func_IsIntersected_Line_Polygon, 6 },
	{ "IsIntersected_Line_Circle", DxScript::Func_IsIntersected_Line_Circle, 8 },
	{ "IsIntersected_Line_Ellipse", DxScript::Func_IsIntersected_Line_Ellipse, 9 },
	{ "IsIntersected_Line_Line", DxScript::Func_IsIntersected_Line_Line, 10 },
	{ "IsIntersected_Line_RegularPolygon", DxScript::Func_IsIntersected_Line_RegularPolygon, 10 },

	{ "IsIntersected_Polygon_Polygon", DxScript::Func_IsIntersected_Polygon_Polygon, 2 },
	{ "IsIntersected_Polygon_Ellipse", DxScript::Func_IsIntersected_Polygon_Ellipse, 5 },
	{ "IsIntersected_Polygon_RegularPolygon", DxScript::Func_IsIntersected_Polygon_RegularPolygon, 6 },

	//Color conversion functions
	{ "ColorARGBToHex", DxScript::Func_ColorARGBToHex, 4 },
	{ "ColorARGBToHex", DxScript::Func_ColorARGBToHex, 1 },		//Overloaded
	{ "ColorHexToARGB", DxScript::Func_ColorHexToARGB, 1 },
	{ "ColorHexToARGB", DxScript::Func_ColorHexToARGB, 2 },		//Overloaded
	{ "ColorRGBtoHSV", DxScript::Func_ColorRGBtoHSV, 3 },
	{ "ColorHSVtoRGB", DxScript::Func_ColorHSVtoRGB, 3 },
	{ "ColorHSVtoHexRGB", DxScript::Func_ColorHSVtoHexRGB, 3 },

	//Other stuff
	{ "SetInvalidPositionReturn", DxScript::Func_SetInvalidPositionReturn, 2 },

	//Base object functions
	{ "Obj_Delete", DxScript::Func_Obj_Delete, 1 },
	{ "Obj_IsDeleted", DxScript::Func_Obj_IsDeleted, 1 },
	{ "Obj_IsExists", DxScript::Func_Obj_IsExists, 1 },
	{ "Obj_SetVisible", DxScript::Func_Obj_SetVisible, 2 },
	{ "Obj_IsVisible", DxScript::Func_Obj_IsVisible, 1 },
	{ "Obj_SetRenderPriority", DxScript::Func_Obj_SetRenderPriority, 2 },
	{ "Obj_SetRenderPriorityI", DxScript::Func_Obj_SetRenderPriorityI, 2 },
	{ "Obj_GetRenderPriority", DxScript::Func_Obj_GetRenderPriority, 1 },
	{ "Obj_GetRenderPriorityI", DxScript::Func_Obj_GetRenderPriorityI, 1 },
	{ "Obj_GetValue", DxScript::Func_Obj_GetValue, 2 },
	{ "Obj_GetValueD", DxScript::Func_Obj_GetValueD, 3 },
	{ "Obj_SetValue", DxScript::Func_Obj_SetValue, 3 },
	{ "Obj_DeleteValue", DxScript::Func_Obj_DeleteValue, 2 },
	{ "Obj_IsValueExists", DxScript::Func_Obj_IsValueExists, 2 },

	{ "Obj_GetValueI", DxScript::Func_Obj_GetValueI, 2 },
	{ "Obj_GetValueDI", DxScript::Func_Obj_GetValueDI, 3 },
	{ "Obj_SetValueI", DxScript::Func_Obj_SetValueI, 3 },
	{ "Obj_DeleteValueI", DxScript::Func_Obj_DeleteValueI, 2 },
	{ "Obj_IsValueExistsI", DxScript::Func_Obj_IsValueExistsI, 2 },

	{ "Obj_CopyValueTable", DxScript::Func_Obj_CopyValueTable, 3 },
	{ "Obj_GetType", DxScript::Func_Obj_GetType, 1 },

	//Render object functions
	{ "ObjRender_SetX", DxScript::Func_ObjRender_SetX, 2 },
	{ "ObjRender_SetY", DxScript::Func_ObjRender_SetY, 2 },
	{ "ObjRender_SetZ", DxScript::Func_ObjRender_SetZ, 2 },
	{ "ObjRender_SetPosition", DxScript::Func_ObjRender_SetPosition, 4 },
	{ "ObjRender_SetPosition", DxScript::Func_ObjRender_SetPosition, 3 }, //Overloaded
	{ "ObjRender_SetAngleX", DxScript::Func_ObjRender_SetAngleX, 2 },
	{ "ObjRender_SetAngleY", DxScript::Func_ObjRender_SetAngleY, 2 },
	{ "ObjRender_SetAngleZ", DxScript::Func_ObjRender_SetAngleZ, 2 },
	{ "ObjRender_SetAngleXYZ", DxScript::Func_ObjRender_SetAngleXYZ, 4 },
	{ "ObjRender_SetScaleX", DxScript::Func_ObjRender_SetScaleX, 2 },
	{ "ObjRender_SetScaleY", DxScript::Func_ObjRender_SetScaleY, 2 },
	{ "ObjRender_SetScaleZ", DxScript::Func_ObjRender_SetScaleZ, 2 },
	{ "ObjRender_SetScaleXYZ", DxScript::Func_ObjRender_SetScaleXYZ, 4 },
	{ "ObjRender_SetScaleXYZ", DxScript::Func_ObjRender_SetScaleXYZ, 2 }, //Overloaded
	{ "ObjRender_SetColor", DxScript::Func_ObjRender_SetColor, 4 },
	{ "ObjRender_SetColor", DxScript::Func_ObjRender_SetColor, 2 },		//Overloaded
	{ "ObjRender_SetColorHSV", DxScript::Func_ObjRender_SetColorHSV, 4 },
	{ "ObjRender_GetColor", DxScript::Func_ObjRender_GetColor, 1 },
	{ "ObjRender_GetColorHex", DxScript::Func_ObjRender_GetColorHex, 1 },
	{ "ObjRender_SetAlpha", DxScript::Func_ObjRender_SetAlpha, 2 },
	{ "ObjRender_GetAlpha", DxScript::Func_ObjRender_GetAlpha, 1 },
	{ "ObjRender_SetBlendType", DxScript::Func_ObjRender_SetBlendType, 2 },
	{ "ObjRender_GetBlendType", DxScript::Func_ObjRender_GetBlendType, 1 },
	{ "ObjRender_GetX", DxScript::Func_ObjRender_GetX, 1 },
	{ "ObjRender_GetY", DxScript::Func_ObjRender_GetY, 1 },
	{ "ObjRender_GetZ", DxScript::Func_ObjRender_GetZ, 1 },
	{ "ObjRender_GetPosition", DxScript::Func_ObjRender_GetPosition, 1 },
	{ "ObjRender_GetAngleX", DxScript::Func_ObjRender_GetAngleX, 1 },
	{ "ObjRender_GetAngleY", DxScript::Func_ObjRender_GetAngleY, 1 },
	{ "ObjRender_GetAngleZ", DxScript::Func_ObjRender_GetAngleZ, 1 },
	{ "ObjRender_GetScaleX", DxScript::Func_ObjRender_GetScaleX, 1 },
	{ "ObjRender_GetScaleY", DxScript::Func_ObjRender_GetScaleY, 1 },
	{ "ObjRender_GetScaleZ", DxScript::Func_ObjRender_GetScaleZ, 1 },
	{ "ObjRender_SetZWrite", DxScript::Func_ObjRender_SetZWrite, 2 },
	{ "ObjRender_SetZTest", DxScript::Func_ObjRender_SetZTest, 2 },
	{ "ObjRender_SetFogEnable", DxScript::Func_ObjRender_SetFogEnable, 2 },
	{ "ObjRender_SetCullingMode", DxScript::Func_ObjRender_SetCullingMode, 2 },
	{ "ObjRender_SetRelativeObject", DxScript::Func_ObjRender_SetRelativeObject, 3 },
	{ "ObjRender_SetPermitCamera", DxScript::Func_ObjRender_SetPermitCamera, 2 },
	{ "ObjRender_SetTextureFilterMin", DxScript::Func_ObjRender_SetTextureFilterMin, 2 },
	{ "ObjRender_SetTextureFilterMag", DxScript::Func_ObjRender_SetTextureFilterMag, 2 },
	{ "ObjRender_SetTextureFilterMip", DxScript::Func_ObjRender_SetTextureFilterMip, 2 },
	{ "ObjRender_SetVertexShaderRenderingMode", DxScript::Func_ObjRender_SetVertexShaderRenderingMode, 2 },
	{ "ObjRender_SetEnableDefaultTransformMatrix", DxScript::Func_ObjRender_SetEnableDefaultTransformMatrix, 2 },
	{ "ObjRender_SetLightingEnable", DxScript::Func_ObjRender_SetLightingEnable, 3 },
	{ "ObjRender_SetLightingDiffuseColor", DxScript::Func_ObjRender_SetLightingDiffuseColor, 4 },
	{ "ObjRender_SetLightingDiffuseColor", DxScript::Func_ObjRender_SetLightingDiffuseColor, 2 },	//Overloaded
	{ "ObjRender_SetLightingSpecularColor", DxScript::Func_ObjRender_SetLightingSpecularColor, 4 },
	{ "ObjRender_SetLightingSpecularColor", DxScript::Func_ObjRender_SetLightingSpecularColor, 2 },	//Overloaded
	{ "ObjRender_SetLightingAmbientColor", DxScript::Func_ObjRender_SetLightingAmbientColor, 4 },
	{ "ObjRender_SetLightingAmbientColor", DxScript::Func_ObjRender_SetLightingAmbientColor, 2 },	//Overloaded
	{ "ObjRender_SetLightingDirection", DxScript::Func_ObjRender_SetLightingDirection, 4 },

	//Shader object functions
	{ "ObjShader_Create", DxScript::Func_ObjShader_Create, 0 },
	{ "ObjShader_SetShaderF", DxScript::Func_ObjShader_SetShaderF, 2 },
	{ "ObjShader_SetShaderO", DxScript::Func_ObjShader_SetShaderO, 2 },
	{ "ObjShader_ResetShader", DxScript::Func_ObjShader_ResetShader, 1 },
	{ "ObjShader_SetTechnique", DxScript::Func_ObjShader_SetTechnique, 2 },
	{ "ObjShader_SetMatrix", DxScript::Func_ObjShader_SetMatrix, 3 },
	{ "ObjShader_SetMatrixArray", DxScript::Func_ObjShader_SetMatrixArray, 3 },
	{ "ObjShader_SetVector", DxScript::Func_ObjShader_SetVector, 6 },
	{ "ObjShader_SetFloat", DxScript::Func_ObjShader_SetFloat, 3 },
	{ "ObjShader_SetFloatArray", DxScript::Func_ObjShader_SetFloatArray, 3 },
	{ "ObjShader_SetTexture", DxScript::Func_ObjShader_SetTexture, 3 },

	//Primitive object functions
	{ "ObjPrim_Create", DxScript::Func_ObjPrimitive_Create, 1 },
	{ "ObjPrim_SetPrimitiveType", DxScript::Func_ObjPrimitive_SetPrimitiveType, 2 },
	{ "ObjPrim_GetPrimitiveType", DxScript::Func_ObjPrimitive_GetPrimitiveType, 1 },
	{ "ObjPrim_SetVertexCount", DxScript::Func_ObjPrimitive_SetVertexCount, 2 },
	{ "ObjPrim_SetTexture", DxScript::Func_ObjPrimitive_SetTexture, 2 },
	{ "ObjPrim_GetTexture", DxScript::Func_ObjPrimitive_GetTexture, 1 },
	{ "ObjPrim_GetVertexCount", DxScript::Func_ObjPrimitive_GetVertexCount, 1 },
	{ "ObjPrim_SetVertexPosition", DxScript::Func_ObjPrimitive_SetVertexPosition, 5 },
	{ "ObjPrim_SetVertexPosition", DxScript::Func_ObjPrimitive_SetVertexPosition, 4 }, //Overloaded
	{ "ObjPrim_SetVertexUV", DxScript::Func_ObjPrimitive_SetVertexUV, 4 },
	{ "ObjPrim_SetVertexUVT", DxScript::Func_ObjPrimitive_SetVertexUVT, 4 },
	{ "ObjPrim_SetVertexColor", DxScript::Func_ObjPrimitive_SetVertexColor, 5 },
	{ "ObjPrim_SetVertexColor", DxScript::Func_ObjPrimitive_SetVertexColor, 3 },	//Overloaded
	{ "ObjPrim_SetVertexColorHSV", DxScript::Func_ObjPrimitive_SetVertexColorHSV, 5 },
	{ "ObjPrim_SetVertexAlpha", DxScript::Func_ObjPrimitive_SetVertexAlpha, 3 },
	{ "ObjPrim_GetVertexColor", DxScript::Func_ObjPrimitive_GetVertexColor, 2 },
	{ "ObjPrim_GetVertexColorHex", DxScript::Func_ObjPrimitive_GetVertexColorHex, 2 },	// Overloaded
	{ "ObjPrim_GetVertexAlpha", DxScript::Func_ObjPrimitive_GetVertexAlpha, 2 },
	{ "ObjPrim_GetVertexPosition", DxScript::Func_ObjPrimitive_GetVertexPosition, 2 },
	{ "ObjPrim_SetVertexIndex", DxScript::Func_ObjPrimitive_SetVertexIndex, 2 },
	

	//2D sprite object functions
	{ "ObjSprite2D_SetSourceRect", DxScript::Func_ObjSprite2D_SetSourceRect, 5 },
	{ "ObjSprite2D_SetDestRect", DxScript::Func_ObjSprite2D_SetDestRect, 5 },
	{ "ObjSprite2D_SetDestCenter", DxScript::Func_ObjSprite2D_SetDestCenter, 1 },

	//2D sprite list object functions
	{ "ObjSpriteList2D_SetSourceRect", DxScript::Func_ObjSpriteList2D_SetSourceRect, 5 },
	{ "ObjSpriteList2D_SetDestRect", DxScript::Func_ObjSpriteList2D_SetDestRect, 5 },
	{ "ObjSpriteList2D_SetDestCenter", DxScript::Func_ObjSpriteList2D_SetDestCenter, 1 },
	{ "ObjSpriteList2D_AddVertex", DxScript::Func_ObjSpriteList2D_AddVertex, 1 },
	{ "ObjSpriteList2D_CloseVertex", DxScript::Func_ObjSpriteList2D_CloseVertex, 1 },
	{ "ObjSpriteList2D_ClearVertexCount", DxScript::Func_ObjSpriteList2D_ClearVertexCount, 1 },
	{ "ObjSpriteList2D_SetAutoClearVertexCount", DxScript::Func_ObjSpriteList2D_SetAutoClearVertexCount, 2 },

	//3D sprite object functions
	{ "ObjSprite3D_SetSourceRect", DxScript::Func_ObjSprite3D_SetSourceRect, 5 },
	{ "ObjSprite3D_SetDestRect", DxScript::Func_ObjSprite3D_SetDestRect, 5 },
	{ "ObjSprite3D_SetSourceDestRect", DxScript::Func_ObjSprite3D_SetSourceDestRect, 5 },
	{ "ObjSprite3D_SetBillboard", DxScript::Func_ObjSprite3D_SetBillboard, 2 },

	//2D trajectory object functions (what)
	{ "ObjTrajectory3D_SetInitialPoint", DxScript::Func_ObjTrajectory3D_SetInitialPoint, 7 },
	{ "ObjTrajectory3D_SetAlphaVariation", DxScript::Func_ObjTrajectory3D_SetAlphaVariation, 2 },
	{ "ObjTrajectory3D_SetComplementCount", DxScript::Func_ObjTrajectory3D_SetComplementCount, 2 },

	//Particle list object functions
	{ "ObjParticleList_Create", DxScript::Func_ObjParticleList_Create, 1 },
	{ "ObjParticleList_SetPosition", DxScript::Func_ObjParticleList_SetPosition, 4 },
	{ "ObjParticleList_SetPosition", DxScript::Func_ObjParticleList_SetPosition, 3 }, //Overloaded
	{ "ObjParticleList_SetScaleX", DxScript::Func_ObjParticleList_SetScaleSingle<0>, 2 },
	{ "ObjParticleList_SetScaleY", DxScript::Func_ObjParticleList_SetScaleSingle<1>, 2 },
	{ "ObjParticleList_SetScaleZ", DxScript::Func_ObjParticleList_SetScaleSingle<2>, 2 },
	{ "ObjParticleList_SetScale", DxScript::Func_ObjParticleList_SetScaleXYZ, 4 },
	{ "ObjParticleList_SetScale", DxScript::Func_ObjParticleList_SetScaleXYZ, 2 }, //Overloaded
	{ "ObjParticleList_SetAngleX", DxScript::Func_ObjParticleList_SetAngleSingle<0>, 2 },
	{ "ObjParticleList_SetAngleY", DxScript::Func_ObjParticleList_SetAngleSingle<1>, 2 },
	{ "ObjParticleList_SetAngleZ", DxScript::Func_ObjParticleList_SetAngleSingle<2>, 2 },
	{ "ObjParticleList_SetAngle", DxScript::Func_ObjParticleList_SetAngle, 4 },
	{ "ObjParticleList_SetColor", DxScript::Func_ObjParticleList_SetColor, 4 },
	{ "ObjParticleList_SetColor", DxScript::Func_ObjParticleList_SetColor, 2 },	//Overloaded
	{ "ObjParticleList_SetAlpha", DxScript::Func_ObjParticleList_SetAlpha, 2 },
	{ "ObjParticleList_SetExtraData", DxScript::Func_ObjParticleList_SetExtraData, 4 },
	{ "ObjParticleList_AddInstance", DxScript::Func_ObjParticleList_AddInstance, 1 },
	{ "ObjParticleList_ClearInstance", DxScript::Func_ObjParticleList_ClearInstance, 1 },

	//Mesh object functions
	{ "ObjMesh_Create", DxScript::Func_ObjMesh_Create, 0 },
	{ "ObjMesh_Load", DxScript::Func_ObjMesh_Load, 2 },
	{ "ObjMesh_SetColor", DxScript::Func_ObjMesh_SetColor, 4 },
	{ "ObjMesh_SetColor", DxScript::Func_ObjMesh_SetColor, 2 },	//Overloaded
	{ "ObjMesh_SetAlpha", DxScript::Func_ObjMesh_SetAlpha, 2 },
	{ "ObjMesh_SetAnimation", DxScript::Func_ObjMesh_SetAnimation, 3 },
	{ "ObjMesh_SetCoordinate2D", DxScript::Func_ObjMesh_SetCoordinate2D, 2 },
	{ "ObjMesh_GetPath", DxScript::Func_ObjMesh_GetPath, 1 },

	//Text object functions
	{ "ObjText_Create", DxScript::Func_ObjText_Create, 0 },
	{ "ObjText_SetText", DxScript::Func_ObjText_SetText, 2 },
	{ "ObjText_SetFontType", DxScript::Func_ObjText_SetFontType, 2 },
	{ "ObjText_SetFontSize", DxScript::Func_ObjText_SetFontSize, 2 },
	{ "ObjText_SetFontBold", DxScript::Func_ObjText_SetFontBold, 2 },
	{ "ObjText_SetFontWeight", DxScript::Func_ObjText_SetFontWeight, 2 },
	{ "ObjText_SetFontColor", DxScript::Func_ObjText_SetFontColor, 4 },
	{ "ObjText_SetFontColor", DxScript::Func_ObjText_SetFontColor, 2 }, //Overloaded
	{ "ObjText_SetFontColorTop", DxScript::Func_ObjText_SetFontColorTop, 4 },
	{ "ObjText_SetFontColorTop", DxScript::Func_ObjText_SetFontColorTop, 2 },			//Overloaded
	{ "ObjText_SetFontColorBottom", DxScript::Func_ObjText_SetFontColorBottom, 4 },
	{ "ObjText_SetFontColorBottom", DxScript::Func_ObjText_SetFontColorBottom, 2 },		//Overloaded
	{ "ObjText_SetFontBorderWidth", DxScript::Func_ObjText_SetFontBorderWidth, 2 },
	{ "ObjText_SetFontBorderType", DxScript::Func_ObjText_SetFontBorderType, 2 },
	{ "ObjText_SetFontBorderColor", DxScript::Func_ObjText_SetFontBorderColor, 4 },
	{ "ObjText_SetFontBorderColor", DxScript::Func_ObjText_SetFontBorderColor, 2 },		//Overloaded
	{ "ObjText_SetFontCharacterSet", DxScript::Func_ObjText_SetFontCharacterSet, 2 },
	{ "ObjText_SetMaxWidth", DxScript::Func_ObjText_SetMaxWidth, 2 },
	{ "ObjText_SetMaxHeight", DxScript::Func_ObjText_SetMaxHeight, 2 },
	{ "ObjText_SetLinePitch", DxScript::Func_ObjText_SetLinePitch, 2 },
	{ "ObjText_SetSidePitch", DxScript::Func_ObjText_SetSidePitch, 2 },
	{ "ObjText_SetVertexColor", DxScript::Func_ObjText_SetVertexColor, 5 },
	{ "ObjText_SetVertexColor", DxScript::Func_ObjText_SetVertexColor, 2 },				//Overloaded
	{ "ObjText_SetTransCenter", DxScript::Func_ObjText_SetTransCenter, 3 },
	{ "ObjText_SetAutoTransCenter", DxScript::Func_ObjText_SetAutoTransCenter, 2 },
	{ "ObjText_SetHorizontalAlignment", DxScript::Func_ObjText_SetHorizontalAlignment, 2 },
	{ "ObjText_SetSyntacticAnalysis", DxScript::Func_ObjText_SetSyntacticAnalysis, 2 },
	{ "ObjText_GetTextLength", DxScript::Func_ObjText_GetTextLength, 1 },
	{ "ObjText_GetTextLengthCU", DxScript::Func_ObjText_GetTextLengthCU, 1 },
	{ "ObjText_GetTextLengthCUL", DxScript::Func_ObjText_GetTextLengthCUL, 1 },
	{ "ObjText_GetTotalWidth", DxScript::Func_ObjText_GetTotalWidth, 1 },
	{ "ObjText_GetTotalHeight", DxScript::Func_ObjText_GetTotalHeight, 1 },

	//Sound object functions
	{ "ObjSound_Create", DxScript::Func_ObjSound_Create, 0 },
	{ "ObjSound_Load", DxScript::Func_ObjSound_Load, 2 },
	{ "ObjSound_Play", DxScript::Func_ObjSound_Play, 1 },
	{ "ObjSound_Stop", DxScript::Func_ObjSound_Stop, 1 },
	{ "ObjSound_SetVolumeRate", DxScript::Func_ObjSound_SetVolumeRate, 2 },
	{ "ObjSound_SetPanRate", DxScript::Func_ObjSound_SetPanRate, 2 },
	{ "ObjSound_SetFade", DxScript::Func_ObjSound_SetFade, 2 },
	{ "ObjSound_SetLoopEnable", DxScript::Func_ObjSound_SetLoopEnable, 2 },
	{ "ObjSound_SetLoopTime", DxScript::Func_ObjSound_SetLoopTime, 3 },
	{ "ObjSound_SetLoopSampleCount", DxScript::Func_ObjSound_SetLoopSampleCount, 3 },
	{ "ObjSound_Seek", DxScript::Func_ObjSound_Seek, 2 },
	{ "ObjSound_SeekSampleCount", DxScript::Func_ObjSound_SeekSampleCount, 2 },
	{ "ObjSound_SetResumeEnable", DxScript::Func_ObjSound_SetResumeEnable, 2 },
	{ "ObjSound_SetSoundDivision", DxScript::Func_ObjSound_SetSoundDivision, 2 },
	{ "ObjSound_IsPlaying", DxScript::Func_ObjSound_IsPlaying, 1 },
	{ "ObjSound_GetVolumeRate", DxScript::Func_ObjSound_GetVolumeRate, 1 },
	//{ "ObjSound_DebugGetCopyPos", DxScript::Func_ObjSound_DebugGetCopyPos, 1 },
	{ "ObjSound_SetFrequency", DxScript::Func_ObjSound_SetFrequency, 2 },
	{ "ObjSound_GetInfo", DxScript::Func_ObjSound_GetInfo, 2 },

	//File object functions
	{ "ObjFile_Create", DxScript::Func_ObjFile_Create, 1 },
	{ "ObjFile_Open", DxScript::Func_ObjFile_Open, 2 },
	{ "ObjFile_OpenNW", DxScript::Func_ObjFile_OpenNW, 2 },
	{ "ObjFile_Store", DxScript::Func_ObjFile_Store, 1 },
	{ "ObjFile_GetSize", DxScript::Func_ObjFile_GetSize, 1 },

	//Text file object functions
	{ "ObjFileT_GetLineCount", DxScript::Func_ObjFileT_GetLineCount, 1 },
	{ "ObjFileT_GetLineText", DxScript::Func_ObjFileT_GetLineText, 2 },
	{ "ObjFileT_SetLineText", DxScript::Func_ObjFileT_SetLineText, 3 },
	{ "ObjFileT_SplitLineText", DxScript::Func_ObjFileT_SplitLineText, 3 },
	{ "ObjFileT_AddLine", DxScript::Func_ObjFileT_AddLine, 2 },
	{ "ObjFileT_ClearLine", DxScript::Func_ObjFileT_ClearLine, 1 },

	//Binary file object functions
	{ "ObjFileB_SetByteOrder", DxScript::Func_ObjFileB_SetByteOrder, 2 },
	{ "ObjFileB_SetCharacterCode", DxScript::Func_ObjFileB_SetCharacterCode, 2 },
	{ "ObjFileB_GetPointer", DxScript::Func_ObjFileB_GetPointer, 1 },
	{ "ObjFileB_Seek", DxScript::Func_ObjFileB_Seek, 2 },
	{ "ObjFileB_GetLastRead", DxScript::Func_ObjFileB_GetLastRead, 1 },
	{ "ObjFileB_ReadBoolean", DxScript::Func_ObjFileB_ReadBoolean, 1 },
	{ "ObjFileB_ReadByte", DxScript::Func_ObjFileB_ReadByte, 1 },
	{ "ObjFileB_ReadShort", DxScript::Func_ObjFileB_ReadShort, 1 },
	{ "ObjFileB_ReadInteger", DxScript::Func_ObjFileB_ReadInteger, 1 },
	{ "ObjFileB_ReadLong", DxScript::Func_ObjFileB_ReadLong, 1 },
	{ "ObjFileB_ReadFloat", DxScript::Func_ObjFileB_ReadFloat, 1 },
	{ "ObjFileB_ReadDouble", DxScript::Func_ObjFileB_ReadDouble, 1 },
	{ "ObjFileB_ReadString", DxScript::Func_ObjFileB_ReadString, 2 },
	{ "ObjFileB_WriteBoolean", DxScript::Func_ObjFileB_WriteBoolean, 2 },
	{ "ObjFileB_WriteByte", DxScript::Func_ObjFileB_WriteByte, 2 },
	{ "ObjFileB_WriteShort", DxScript::Func_ObjFileB_WriteShort, 2 },
	{ "ObjFileB_WriteInteger", DxScript::Func_ObjFileB_WriteInteger, 2 },
	{ "ObjFileB_WriteLong", DxScript::Func_ObjFileB_WriteLong, 2 },
	{ "ObjFileB_WriteFloat", DxScript::Func_ObjFileB_WriteFloat, 2 },
	{ "ObjFileB_WriteDouble", DxScript::Func_ObjFileB_WriteDouble, 2 },
};
static const std::vector<constant> dxConstant = {
	//Object types
	constant("ID_INVALID", DxScript::ID_INVALID),
	constant("OBJ_PRIMITIVE_2D", (int)TypeObject::Primitive2D),
	constant("OBJ_SPRITE_2D", (int)TypeObject::Sprite2D),
	constant("OBJ_SPRITE_LIST_2D", (int)TypeObject::SpriteList2D),
	constant("OBJ_PRIMITIVE_3D", (int)TypeObject::Primitive3D),
	constant("OBJ_SPRITE_3D", (int)TypeObject::Sprite3D),
	constant("OBJ_TRAJECTORY_3D", (int)TypeObject::Trajectory3D),
	constant("OBJ_PARTICLE_LIST_2D", (int)TypeObject::ParticleList2D),
	constant("OBJ_PARTICLE_LIST_3D", (int)TypeObject::ParticleList3D),
	constant("OBJ_SHADER", (int)TypeObject::Shader),
	constant("OBJ_MESH", (int)TypeObject::Mesh),
	constant("OBJ_TEXT", (int)TypeObject::Text),
	constant("OBJ_SOUND", (int)TypeObject::Sound),
	constant("OBJ_FILE_TEXT", (int)TypeObject::FileText),
	constant("OBJ_FILE_BINARY", (int)TypeObject::FileBinary),

	//ColorHexToARGB permutations
	constant("COLOR_PERMUTE_ARGB", 0x31b),
	constant("COLOR_PERMUTE_RGBA", 0x36c),
	constant("COLOR_PERMUTE_BGRA", 0x3e4),
	constant("COLOR_PERMUTE_RGB", 0x26c),
	constant("COLOR_PERMUTE_BGR", 0x2e4),
	constant("COLOR_PERMUTE_A", 0x000),
	constant("COLOR_PERMUTE_R", 0x040),
	constant("COLOR_PERMUTE_G", 0x080),
	constant("COLOR_PERMUTE_B", 0x0c0),

	//Blend types
	constant("BLEND_NONE", BlendMode::MODE_BLEND_NONE),
	constant("BLEND_ALPHA", BlendMode::MODE_BLEND_ALPHA),
	constant("BLEND_ADD_RGB", BlendMode::MODE_BLEND_ADD_RGB),
	constant("BLEND_ADD_ARGB", BlendMode::MODE_BLEND_ADD_ARGB),
	constant("BLEND_MULTIPLY", BlendMode::MODE_BLEND_MULTIPLY),
	constant("BLEND_SUBTRACT", BlendMode::MODE_BLEND_SUBTRACT),
	constant("BLEND_SHADOW", BlendMode::MODE_BLEND_SHADOW),
	constant("BLEND_INV_DESTRGB", BlendMode::MODE_BLEND_INV_DESTRGB),
	constant("BLEND_ALPHA_INV", BlendMode::MODE_BLEND_ALPHA_INV),

	//Cull modes
	constant("CULL_NONE", D3DCULL_NONE),
	constant("CULL_CW", D3DCULL_CW),
	constant("CULL_CCW", D3DCULL_CCW),

	//Image file formats
	constant("IFF_BMP", D3DXIFF_BMP),
	constant("IFF_JPG", D3DXIFF_JPG),
	constant("IFF_TGA", D3DXIFF_TGA),
	constant("IFF_PNG", D3DXIFF_PNG),
	constant("IFF_DDS", D3DXIFF_DDS),
	constant("IFF_PPM", D3DXIFF_PPM),
	//constant("IFF_DIB", D3DXIFF_DIB),
	//constant("IFF_HDR", D3DXIFF_HDR),
	//constant("IFF_PFM", D3DXIFF_PFM),

	//Texture filtering
	constant("FILTER_NONE", D3DTEXF_NONE),
	constant("FILTER_POINT", D3DTEXF_POINT),
	constant("FILTER_LINEAR", D3DTEXF_LINEAR),
	constant("FILTER_ANISOTROPIC", D3DTEXF_ANISOTROPIC),

	//Camera modes
	constant("CAMERA_NORMAL", DxCamera::MODE_NORMAL),
	constant("CAMERA_LOOKAT", DxCamera::MODE_LOOKAT),

	//Primitive types
	constant("PRIMITIVE_POINT_LIST", D3DPT_POINTLIST),
	constant("PRIMITIVE_LINELIST", D3DPT_LINELIST),
	constant("PRIMITIVE_LINESTRIP", D3DPT_LINESTRIP),
	constant("PRIMITIVE_TRIANGLELIST", D3DPT_TRIANGLELIST),
	constant("PRIMITIVE_TRIANGLESTRIP", D3DPT_TRIANGLESTRIP),
	constant("PRIMITIVE_TRIANGLEFAN", D3DPT_TRIANGLEFAN),

	//Text object border types
	constant("BORDER_NONE", (int)TextBorderType::None),
	constant("BORDER_FULL", (int)TextBorderType::Full),
	constant("BORDER_SHADOW", (int)TextBorderType::Shadow),

	//Text object alignments
	constant("ALIGNMENT_LEFT", (int)TextAlignment::Left),
	constant("ALIGNMENT_RIGHT", (int)TextAlignment::Right),
	constant("ALIGNMENT_CENTER", (int)TextAlignment::Center),
	constant("ALIGNMENT_TOP", (int)TextAlignment::Top),
	constant("ALIGNMENT_BOTTOM", (int)TextAlignment::Bottom),

	//Font character sets
	constant("CHARSET_ANSI", ANSI_CHARSET),
	constant("CHARSET_DEFAULT", DEFAULT_CHARSET),
	constant("CHARSET_SHIFTJIS", SHIFTJIS_CHARSET),
	constant("CHARSET_HANGUL", HANGUL_CHARSET),
	constant("CHARSET_JOHAB", JOHAB_CHARSET),
	constant("CHARSET_CHINESEBIG5", CHINESEBIG5_CHARSET),
	constant("CHARSET_TURKISH", TURKISH_CHARSET),
	constant("CHARSET_VIETNAMESE", VIETNAMESE_CHARSET),
	constant("CHARSET_HEBREW", HEBREW_CHARSET),
	constant("CHARSET_ARABIC", ARABIC_CHARSET),
	constant("CHARSET_THAI", THAI_CHARSET),

	//Sound divisions
	constant("SOUND_BGM", SoundDivision::DIVISION_BGM),
	constant("SOUND_SE", SoundDivision::DIVISION_SE),
	constant("SOUND_VOICE", SoundDivision::DIVISION_VOICE),

	//Audio file formats
	constant("SOUND_UNKNOWN", (int)SoundFileFormat::Unknown),
	constant("SOUND_WAVE", (int)SoundFileFormat::Wave),
	constant("SOUND_OGG", (int)SoundFileFormat::Ogg),
	constant("SOUND_MP3", (int)SoundFileFormat::Mp3),

	//ObjSound_GetInfo info types
	constant("INFO_FORMAT", SoundPlayer::INFO_FORMAT),
	constant("INFO_CHANNEL", SoundPlayer::INFO_CHANNEL),
	constant("INFO_SAMPLE_RATE", SoundPlayer::INFO_SAMPLE_RATE),
	constant("INFO_AVG_BYTE_PER_SEC", SoundPlayer::INFO_AVG_BYTE_PER_SEC),
	constant("INFO_BLOCK_ALIGN", SoundPlayer::INFO_BLOCK_ALIGN),
	constant("INFO_BIT_PER_SAMPLE", SoundPlayer::INFO_BIT_PER_SAMPLE),
	constant("INFO_POSITION", SoundPlayer::INFO_POSITION),
	constant("INFO_POSITION_SAMPLE", SoundPlayer::INFO_POSITION_SAMPLE),
	constant("INFO_LENGTH", SoundPlayer::INFO_LENGTH),
	constant("INFO_LENGTH_SAMPLE", SoundPlayer::INFO_LENGTH_SAMPLE),

	//Binary file code pages
	constant("CODE_ACP", DxScript::CODE_ACP),
	constant("CODE_UTF8", DxScript::CODE_UTF8),
	constant("CODE_UTF16LE", DxScript::CODE_UTF16LE),
	constant("CODE_UTF16BE", DxScript::CODE_UTF16BE),

	//Binary file endianness
	constant("ENDIAN_LITTLE", ByteOrder::ENDIAN_LITTLE),
	constant("ENDIAN_BIG", ByteOrder::ENDIAN_BIG),

	//Key states
	constant("KEY_FREE", KEY_FREE),
	constant("KEY_PUSH", KEY_PUSH),
	constant("KEY_PULL", KEY_PULL),
	constant("KEY_HOLD", KEY_HOLD),

	//Mouse key IDs
	constant("MOUSE_LEFT", DI_MOUSE_LEFT),
	constant("MOUSE_RIGHT", DI_MOUSE_RIGHT),
	constant("MOUSE_MIDDLE", DI_MOUSE_MIDDLE),

	//Keyboard key IDs
	constant("KEY_ESCAPE", DIK_ESCAPE),
	constant("KEY_1", DIK_1),
	constant("KEY_2", DIK_2),
	constant("KEY_3", DIK_3),
	constant("KEY_4", DIK_4),
	constant("KEY_5", DIK_5),
	constant("KEY_6", DIK_6),
	constant("KEY_7", DIK_7),
	constant("KEY_8", DIK_8),
	constant("KEY_9", DIK_9),
	constant("KEY_0", DIK_0),
	constant("KEY_MINUS", DIK_MINUS),
	constant("KEY_EQUALS", DIK_EQUALS),
	constant("KEY_BACK", DIK_BACK),
	constant("KEY_TAB", DIK_TAB),
	constant("KEY_Q", DIK_Q),
	constant("KEY_W", DIK_W),
	constant("KEY_E", DIK_E),
	constant("KEY_R", DIK_R),
	constant("KEY_T", DIK_T),
	constant("KEY_Y", DIK_Y),
	constant("KEY_U", DIK_U),
	constant("KEY_I", DIK_I),
	constant("KEY_O", DIK_O),
	constant("KEY_P", DIK_P),
	constant("KEY_LBRACKET", DIK_LBRACKET),
	constant("KEY_RBRACKET", DIK_RBRACKET),
	constant("KEY_RETURN", DIK_RETURN),
	constant("KEY_LCONTROL", DIK_LCONTROL),
	constant("KEY_A", DIK_A),
	constant("KEY_S", DIK_S),
	constant("KEY_D", DIK_D),
	constant("KEY_F", DIK_F),
	constant("KEY_G", DIK_G),
	constant("KEY_H", DIK_H),
	constant("KEY_J", DIK_J),
	constant("KEY_K", DIK_K),
	constant("KEY_L", DIK_L),
	constant("KEY_SEMICOLON", DIK_SEMICOLON),
	constant("KEY_APOSTROPHE", DIK_APOSTROPHE),
	constant("KEY_GRAVE", DIK_GRAVE),
	constant("KEY_LSHIFT", DIK_LSHIFT),
	constant("KEY_BACKSLASH", DIK_BACKSLASH),
	constant("KEY_Z", DIK_Z),
	constant("KEY_X", DIK_X),
	constant("KEY_C", DIK_C),
	constant("KEY_V", DIK_V),
	constant("KEY_B", DIK_B),
	constant("KEY_N", DIK_N),
	constant("KEY_M", DIK_M),
	constant("KEY_COMMA", DIK_COMMA),
	constant("KEY_PERIOD", DIK_PERIOD),
	constant("KEY_SLASH", DIK_SLASH),
	constant("KEY_RSHIFT", DIK_RSHIFT),
	constant("KEY_MULTIPLY", DIK_MULTIPLY),
	constant("KEY_LMENU", DIK_LMENU),
	constant("KEY_SPACE", DIK_SPACE),
	constant("KEY_CAPITAL", DIK_CAPITAL),
	constant("KEY_F1", DIK_F1),
	constant("KEY_F2", DIK_F2),
	constant("KEY_F3", DIK_F3),
	constant("KEY_F4", DIK_F4),
	constant("KEY_F5", DIK_F5),
	constant("KEY_F6", DIK_F6),
	constant("KEY_F7", DIK_F7),
	constant("KEY_F8", DIK_F8),
	constant("KEY_F9", DIK_F9),
	constant("KEY_F10", DIK_F10),
	constant("KEY_NUMLOCK", DIK_NUMLOCK),
	constant("KEY_SCROLL", DIK_SCROLL),
	constant("KEY_NUMPAD7", DIK_NUMPAD7),
	constant("KEY_NUMPAD8", DIK_NUMPAD8),
	constant("KEY_NUMPAD9", DIK_NUMPAD9),
	constant("KEY_SUBTRACT", DIK_SUBTRACT),
	constant("KEY_NUMPAD4", DIK_NUMPAD4),
	constant("KEY_NUMPAD5", DIK_NUMPAD5),
	constant("KEY_NUMPAD6", DIK_NUMPAD6),
	constant("KEY_ADD", DIK_ADD),
	constant("KEY_NUMPAD1", DIK_NUMPAD1),
	constant("KEY_NUMPAD2", DIK_NUMPAD2),
	constant("KEY_NUMPAD3", DIK_NUMPAD3),
	constant("KEY_NUMPAD0", DIK_NUMPAD0),
	constant("KEY_DECIMAL", DIK_DECIMAL),
	constant("KEY_F11", DIK_F11),
	constant("KEY_F12", DIK_F12),
	constant("KEY_F13", DIK_F13),
	constant("KEY_F14", DIK_F14),
	constant("KEY_F15", DIK_F15),
	constant("KEY_KANA", DIK_KANA),
	constant("KEY_CONVERT", DIK_CONVERT),
	constant("KEY_NOCONVERT", DIK_NOCONVERT),
	constant("KEY_YEN", DIK_YEN),
	constant("KEY_NUMPADEQUALS", DIK_NUMPADEQUALS),
	constant("KEY_CIRCUMFLEX", DIK_CIRCUMFLEX),
	constant("KEY_AT", DIK_AT),
	constant("KEY_COLON", DIK_COLON),
	constant("KEY_UNDERLINE", DIK_UNDERLINE),
	constant("KEY_KANJI", DIK_KANJI),
	constant("KEY_STOP", DIK_STOP),
	constant("KEY_AX", DIK_AX),
	constant("KEY_UNLABELED", DIK_UNLABELED),
	constant("KEY_NUMPADENTER", DIK_NUMPADENTER),
	constant("KEY_RCONTROL", DIK_RCONTROL),
	constant("KEY_NUMPADCOMMA", DIK_NUMPADCOMMA),
	constant("KEY_DIVIDE", DIK_DIVIDE),
	constant("KEY_SYSRQ", DIK_SYSRQ),
	constant("KEY_RMENU", DIK_RMENU),
	constant("KEY_PAUSE", DIK_PAUSE),
	constant("KEY_HOME", DIK_HOME),
	constant("KEY_UP", DIK_UP),
	constant("KEY_PRIOR", DIK_PRIOR),
	constant("KEY_LEFT", DIK_LEFT),
	constant("KEY_RIGHT", DIK_RIGHT),
	constant("KEY_END", DIK_END),
	constant("KEY_DOWN", DIK_DOWN),
	constant("KEY_NEXT", DIK_NEXT),
	constant("KEY_INSERT", DIK_INSERT),
	constant("KEY_DELETE", DIK_DELETE),
	constant("KEY_LWIN", DIK_LWIN),
	constant("KEY_RWIN", DIK_RWIN),
	constant("KEY_APPS", DIK_APPS),
	constant("KEY_POWER", DIK_POWER),
	constant("KEY_SLEEP", DIK_SLEEP),
};

double DxScript::g_posInvalidX_ = 0;
double DxScript::g_posInvalidY_ = 0;
double DxScript::g_posInvalidZ_ = 0;
DxScript::DxScript() {
	_AddFunction(&dxFunction);
	_AddConstant(&dxConstant);
	{
		DirectGraphics* graphics = DirectGraphics::GetBase();
		const std::vector<constant> dxConstant2 = {
			constant("SCREEN_WIDTH", graphics->GetScreenWidth()),
			constant("SCREEN_HEIGHT", graphics->GetScreenHeight()),
		};
		_AddConstant(&dxConstant2);
	}

	objManager_ = std::shared_ptr<DxScriptObjectManager>(new DxScriptObjectManager());

	{
		pResouceCache_ = DxScriptResourceCache::GetBase();
		if (pResouceCache_ == nullptr)
			pResouceCache_ = new DxScriptResourceCache();
	}
}
DxScript::~DxScript() {
}
int DxScript::AddObject(ref_unsync_ptr<DxScriptObjectBase> obj, bool bActivate) {
	obj->idScript_ = idScript_;
	return objManager_->AddObject(obj, bActivate);
}

D3DXMATRIX _script_unpack_matrix(script_machine* machine, const value& v) {
	D3DXMATRIX res;
	FLOAT* ptrMat = (FLOAT*)&res;

	if (!v.has_data())
		goto lab_type_invalid;

	type_data* typeElem = v.get_type()->get_element();
	if (typeElem == nullptr)
		goto lab_type_invalid;

	if (typeElem->get_kind() == type_data::tk_array) {
		if (v.length_as_array() != 4U)
			goto lab_size_invalid;

		for (size_t i = 0; i < 4; ++i) {
			type_data* typeElemElem = typeElem->get_element();
			if (typeElemElem == nullptr)
				goto lab_type_invalid;

			const value& subArray = v.index_as_array(i);
			if (subArray.length_as_array() != 4U)
				goto lab_size_invalid;

			for (size_t j = 0; j < 4; ++i)
				ptrMat[i * 4 + j] = subArray.index_as_array(j).as_real();
		}
	}
	else {
		if (v.length_as_array() != 16U)
			goto lab_size_invalid;

		for (size_t i = 0; i < 16; ++i)
			ptrMat[i] = v.index_as_array(i).as_real();
	}
	
	goto lab_return;
lab_size_invalid:
	{
		std::string err = "Matrix size must be 4x4.";
		machine->raise_error(err);
		goto lab_return;
	}
lab_type_invalid:
	{
		std::string err = "Invalid value type for matrix.";
		machine->raise_error(err);
	}
lab_return:
	return res;
}
D3DXVECTOR3 _script_unpack_vector3(script_machine* machine, const value& v) {
	D3DXVECTOR3 res;

	if (!v.has_data())
		goto lab_type_invalid;
	if (v.get_type()->get_kind() != type_data::tk_array)
		goto lab_type_invalid;
	if (v.length_as_array() != 3)
		goto lab_size_invalid;

	res = D3DXVECTOR3(
		(FLOAT)v.index_as_array(0).as_real(), 
		(FLOAT)v.index_as_array(1).as_real(), 
		(FLOAT)v.index_as_array(2).as_real());

	goto lab_return;
lab_size_invalid:
	{
		std::string err = "Incorrect vector size. (Expected 3)";
		machine->raise_error(err);
		goto lab_return;
	}
lab_type_invalid:
	{
		std::string err = "Invalid value type for vector.";
		machine->raise_error(err);
	}
lab_return:
	return res;
}

gstd::value DxScript::Func_MatrixIdentity(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);

	return script->CreateRealArrayValue((FLOAT*)&mat, 16U);
}
gstd::value DxScript::Func_MatrixInverse(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXMATRIX mat = _script_unpack_matrix(machine, argv[0]);
	D3DXMatrixInverse(&mat, nullptr, &mat);

	return script->CreateRealArrayValue((FLOAT*)&mat, 16U);
}
gstd::value DxScript::Func_MatrixAdd(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXMATRIX mat1 = _script_unpack_matrix(machine, argv[0]);
	D3DXMATRIX mat2 = _script_unpack_matrix(machine, argv[1]);
	mat1 += mat2;

	return script->CreateRealArrayValue((FLOAT*)&mat1, 16U);
}
gstd::value DxScript::Func_MatrixSubtract(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXMATRIX mat1 = _script_unpack_matrix(machine, argv[0]);
	D3DXMATRIX mat2 = _script_unpack_matrix(machine, argv[1]);
	mat1 -= mat2;

	return script->CreateRealArrayValue((FLOAT*)&mat1, 16U);
}
gstd::value DxScript::Func_MatrixMultiply(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXMATRIX mat1 = _script_unpack_matrix(machine, argv[0]);
	D3DXMATRIX mat2 = _script_unpack_matrix(machine, argv[1]);
	D3DXMatrixMultiply(&mat1, &mat1, &mat2);

	return script->CreateRealArrayValue((FLOAT*)&mat1, 16U);
}
gstd::value DxScript::Func_MatrixDivide(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXMATRIX mat1 = _script_unpack_matrix(machine, argv[0]);
	D3DXMATRIX mat2 = _script_unpack_matrix(machine, argv[1]);
	D3DXMatrixInverse(&mat2, nullptr, &mat2);
	D3DXMatrixMultiply(&mat1, &mat1, &mat2);

	return script->CreateRealArrayValue((FLOAT*)&mat1, 16U);
}
gstd::value DxScript::Func_MatrixTranspose(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXMATRIX mat = _script_unpack_matrix(machine, argv[0]);
	D3DXMatrixTranspose(&mat, &mat);

	return script->CreateRealArrayValue((FLOAT*)&mat, 16U);
}
gstd::value DxScript::Func_MatrixDeterminant(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXMATRIX mat = _script_unpack_matrix(machine, argv[0]);

	return script->CreateRealValue(D3DXMatrixDeterminant(&mat));
}
gstd::value DxScript::Func_MatrixLookatLH(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXVECTOR3 eye = _script_unpack_vector3(machine, argv[0]);
	D3DXVECTOR3 dest = _script_unpack_vector3(machine, argv[1]);
	D3DXVECTOR3 up = _script_unpack_vector3(machine, argv[2]);

	D3DXMATRIX mat;
	D3DXMatrixLookAtLH(&mat, &eye, &dest, &up);

	return script->CreateRealArrayValue(reinterpret_cast<FLOAT*>(&mat), 16U);
}
gstd::value DxScript::Func_MatrixLookatRH(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXVECTOR3 eye = _script_unpack_vector3(machine, argv[0]);
	D3DXVECTOR3 dest = _script_unpack_vector3(machine, argv[1]);
	D3DXVECTOR3 up = _script_unpack_vector3(machine, argv[2]);

	D3DXMATRIX mat;
	D3DXMatrixLookAtRH(&mat, &eye, &dest, &up);

	return script->CreateRealArrayValue(reinterpret_cast<FLOAT*>(&mat), 16U);
}
gstd::value DxScript::Func_MatrixTransformVector(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXVECTOR3 vect = _script_unpack_vector3(machine, argv[0]);
	D3DXMATRIX mat = _script_unpack_matrix(machine, argv[1]);

	D3DXVECTOR4 out;
	D3DXVec3Transform(&out, &vect, &mat);

	return script->CreateRealArrayValue((FLOAT*)&out, 4U);
}

//Dx関数：システム系系
gstd::value DxScript::Func_InstallFont(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	DxTextRenderer* renderer = DxTextRenderer::GetBase();
	bool res = false;
	try {
		res = renderer->AddFontFromFile(path);
	}
	catch (gstd::wexception& e) {
		Logger::WriteTop(e.what());
	}

	return script->CreateBooleanValue(res);
}

//Dx関数：音声系
value DxScript::Func_LoadSound(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	bool res = true;

	auto& mapSound = script->pResouceCache_->mapSound;
	if (mapSound.find(path) == mapSound.end()) {
		shared_ptr<SoundSourceData> sound = manager->GetSoundSource(path, true);
		res = sound != nullptr;
		if (res) {
			Lock lock(script->criticalSection_);
			mapSound[path] = sound;
		}
	}
	return script->CreateBooleanValue(res);
}
value DxScript::Func_RemoveSound(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	{
		Lock lock(script->criticalSection_);
		script->pResouceCache_->mapSound.erase(path);
	}
	return value();
}
value DxScript::Func_PlayBGM(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	shared_ptr<SoundSourceData> soundSource = manager->GetSoundSource(path, true);
	if (soundSource) {
		shared_ptr<SoundPlayer> player = manager->CreatePlayer(soundSource);
		player->SetAutoDelete(true);
		player->SetSoundDivision(SoundDivision::DIVISION_BGM);

		double loopStart = argv[1].as_real();
		double loopEnd = argv[2].as_real();

		SoundPlayer::PlayStyle* pStyle = player->GetPlayStyle();
		pStyle->bLoop_ = true;
		pStyle->timeLoopStart_ = loopStart;
		pStyle->timeLoopEnd_ = loopEnd;

		//player->Play(style);
		script->GetObjectManager()->ReserveSound(player);
	}
	return value();
}
gstd::value DxScript::Func_PlaySE(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	shared_ptr<SoundSourceData> soundSource = manager->GetSoundSource(path, true);
	if (soundSource) {
		shared_ptr<SoundPlayer> player = manager->CreatePlayer(soundSource);
		player->SetAutoDelete(true);
		player->SetSoundDivision(SoundDivision::DIVISION_SE);

		SoundPlayer::PlayStyle* pStyle = player->GetPlayStyle();
		pStyle->bLoop_ = false;

		//player->Play(style);
		script->GetObjectManager()->ReserveSound(player);
	}
	return value();
}
value DxScript::Func_StopSound(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	shared_ptr<SoundPlayer> player = manager->GetPlayer(path);
	if (player) {
		player->Stop();
		script->GetObjectManager()->DeleteReservedSound(player);
	}
	return value();
}
value DxScript::Func_SetSoundDivisionVolumeRate(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	int idDivision = argv[0].as_int();
	auto division = manager->GetSoundDivision(idDivision);

	if (division) {
		double volume = argv[1].as_real();
		division->SetVolumeRate(volume);
	}

	return value();
}
value DxScript::Func_GetSoundDivisionVolumeRate(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	int idDivision = argv[0].as_int();
	auto division = manager->GetSoundDivision(idDivision);

	double res = 0;
	if (division)
		res = division->GetVolumeRate();

	return script->CreateRealValue(res);
}

//Dx関数：キー系
gstd::value DxScript::Func_GetKeyState(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectInput* input = DirectInput::GetBase();
	int16_t key = (int16_t)argv[0].as_int();
	DIKeyState state = input->GetKeyState(key);
	return DxScript::CreateIntValue(state);
}
gstd::value DxScript::Func_GetMouseX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG res = graphics->GetMousePosition().x;
	return DxScript::CreateRealValue(res);
}
gstd::value DxScript::Func_GetMouseY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG res = graphics->GetMousePosition().y;
	return DxScript::CreateRealValue(res);
}
gstd::value DxScript::Func_GetMouseMoveZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectInput* input = DirectInput::GetBase();
	LONG res = input->GetMouseMoveZ();
	return DxScript::CreateRealValue(res);
}
gstd::value DxScript::Func_GetMouseState(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectInput* input = DirectInput::GetBase();
	DIKeyState res = input->GetMouseState(argv[0].as_real());
	return DxScript::CreateIntValue(res);
}
gstd::value DxScript::Func_GetVirtualKeyState(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DIKeyState res = KEY_FREE;
	VirtualKeyManager* input = dynamic_cast<VirtualKeyManager*>(DirectInput::GetBase());
	if (input) {
		int16_t key = (int16_t)argv[0].as_int();
		res = input->GetVirtualKeyState(key);
	}
	return DxScript::CreateIntValue(res);
}
gstd::value DxScript::Func_SetVirtualKeyState(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	VirtualKeyManager* input = dynamic_cast<VirtualKeyManager*>(DirectInput::GetBase());
	if (input) {
		int16_t key = (int16_t)argv[0].as_int();
		DIKeyState state = (DIKeyState)(argv[1].as_real());
		ref_count_ptr<VirtualKey> vkey = input->GetVirtualKey(key);
		if (vkey)
			vkey->SetKeyState(state);
	}
	return value();
}
//Dx関数：描画系
gstd::value DxScript::Func_GetMonitorWidth(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	LONG res = ::GetSystemMetrics(SM_CXSCREEN);
	return DxScript::CreateIntValue(res);
}
gstd::value DxScript::Func_GetMonitorHeight(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	LONG res = ::GetSystemMetrics(SM_CYSCREEN);
	return DxScript::CreateIntValue(res);
}
gstd::value DxScript::Func_GetScreenWidth(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG res = graphics->GetScreenWidth();
	return DxScript::CreateIntValue(res);
}
gstd::value DxScript::Func_GetScreenHeight(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG res = graphics->GetScreenHeight();
	return DxScript::CreateIntValue(res);
}
gstd::value DxScript::Func_GetWindowedWidth(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG res = graphics->GetConfigData().sizeScreenDisplay_.x;
	return DxScript::CreateIntValue(res);
}
gstd::value DxScript::Func_GetWindowedHeight(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG res = graphics->GetConfigData().sizeScreenDisplay_.y;
	return DxScript::CreateIntValue(res);
}
gstd::value DxScript::Func_IsFullscreenMode(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool res = graphics->GetScreenMode() == ScreenMode::SCREENMODE_FULLSCREEN;
	return DxScript::CreateBooleanValue(res);
}
value DxScript::Func_LoadTexture(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	bool res = true;

	auto& mapTexture = script->pResouceCache_->mapTexture;
	if (mapTexture.find(path) == mapTexture.end()) {
		shared_ptr<Texture> texture(new Texture());
		res = texture->CreateFromFile(path, false, false);
		if (res) {
			Lock lock(script->criticalSection_);
			mapTexture[path] = texture;
		}
	}
	return script->CreateBooleanValue(res);
}
value DxScript::Func_LoadTextureInLoadThread(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	bool res = true;

	auto& mapTexture = script->pResouceCache_->mapTexture;
	if (mapTexture.find(path) == mapTexture.end()) {
		shared_ptr<Texture> texture(new Texture());
		res = texture->CreateFromFileInLoadThread(path, false, false);
		if (res) {
			Lock lock(script->criticalSection_);
			mapTexture[path] = texture;
		}
	}
	return script->CreateBooleanValue(res);
}
value DxScript::Func_LoadTextureEx(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	
	std::wstring path = argv[0].as_string();
	bool useMipMap = argv[1].as_boolean();
	bool useNonPowerOfTwo = argv[2].as_boolean();
	path = PathProperty::GetUnique(path);

	bool res = true;

	auto& mapTexture = script->pResouceCache_->mapTexture;
	if (mapTexture.find(path) == mapTexture.end()) {
		shared_ptr<Texture> texture(new Texture());
		res = texture->CreateFromFile(path, useMipMap, useNonPowerOfTwo);
		if (res) {
			Lock lock(script->criticalSection_);
			mapTexture[path] = texture;
		}
	}
	return script->CreateBooleanValue(res);
}
value DxScript::Func_LoadTextureInLoadThreadEx(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	
	std::wstring path = argv[0].as_string();
	bool useMipMap = argv[1].as_boolean();
	bool useNonPowerOfTwo = argv[2].as_boolean();
	path = PathProperty::GetUnique(path);

	bool res = true;

	auto& mapTexture = script->pResouceCache_->mapTexture;
	if (mapTexture.find(path) == mapTexture.end()) {
		shared_ptr<Texture> texture(new Texture());
		res = texture->CreateFromFileInLoadThread(path, useMipMap, useNonPowerOfTwo);
		if (res) {
			Lock lock(script->criticalSection_);
			mapTexture[path] = texture;
		}
	}
	return script->CreateBooleanValue(res);
}
value DxScript::Func_RemoveTexture(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	{
		Lock lock(script->criticalSection_);
		script->pResouceCache_->mapTexture.erase(path);
	}
	return value();
}
value DxScript::Func_GetTextureWidth(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	TextureManager* textureManager = TextureManager::GetBase();

	UINT res = 0;
	shared_ptr<TextureData> textureData = textureManager->GetTextureData(path);
	if (textureData) {
		D3DXIMAGE_INFO* imageInfo = textureData->GetImageInfo();
		res = imageInfo->Width;
	}

	return script->CreateRealValue(res);
}
value DxScript::Func_GetTextureHeight(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	TextureManager* textureManager = TextureManager::GetBase();

	UINT res = 0;
	shared_ptr<TextureData> textureData = textureManager->GetTextureData(path);
	if (textureData) {
		D3DXIMAGE_INFO* imageInfo = textureData->GetImageInfo();
		res = imageInfo->Height;
	}

	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_SetFogEnable(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool bEnable = argv[0].as_boolean();
	script->GetObjectManager()->SetFogParam(bEnable, 0, 0, 0);
	return value();
}
gstd::value DxScript::Func_SetFogParam(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	float start = argv[0].as_real();
	float end = argv[1].as_real();

	D3DCOLOR color = 0xffffffff;
	if (argc > 3) {
		__m128i c = Vectorize::SetI(0, argv[2].as_int(), argv[3].as_int(), argv[4].as_int());
		color = ColorAccess::ToD3DCOLOR(ColorAccess::ClampColorPacked(c));
	}
	else {
		color = argv[2].as_int();
	}
	script->GetObjectManager()->SetFogParam(true, color, start, end);

	return value();
}
gstd::value DxScript::Func_CreateRenderTarget(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = false;
	std::wstring name = argv[0].as_string();

	auto& mapTexture = script->pResouceCache_->mapTexture;
	if (mapTexture.find(name) == mapTexture.end()) {
		shared_ptr<Texture> texture(new Texture());
		res = texture->CreateRenderTarget(name);
		if (res) {
			Lock lock(script->criticalSection_);
			mapTexture[name] = texture;
		}
	}
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_CreateRenderTargetEx(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = false;
	std::wstring name = argv[0].as_string();
	double width = argv[1].as_real();
	double height = argv[2].as_real();

	if (width > 0 && height > 0) {
		auto& mapTexture = script->pResouceCache_->mapTexture;
		if (mapTexture.find(name) == mapTexture.end()) {
			shared_ptr<Texture> texture(new Texture());
			res = texture->CreateRenderTarget(name, (size_t)width, (size_t)height);
			if (res) {
				Lock lock(script->criticalSection_);
				mapTexture[name] = texture;
			}
		}
	}
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_SetRenderTarget(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	TextureManager* textureManager = TextureManager::GetBase();

	DxScriptResourceCache* rsrcCache = script->pResouceCache_;

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = rsrcCache->GetTexture(name);
	if (texture == nullptr) {
		texture = textureManager->GetTexture(name);
		script->RaiseError("The specified render target does not exist.");
	}
	if (texture->GetType() != TextureData::Type::TYPE_RENDER_TARGET)
		script->RaiseError("Target texture must be a render target.");

	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetRenderTarget(texture, false);
	if (argv[1].as_boolean()) graphics->ClearRenderTarget();

	return value();
}
gstd::value DxScript::Func_ResetRenderTarget(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetRenderTarget(nullptr, false);

	return value();
}
gstd::value DxScript::Func_ClearRenderTargetA1(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	TextureManager* textureManager = TextureManager::GetBase();

	DxScriptResourceCache* rsrcCache = script->pResouceCache_;

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = rsrcCache->GetTexture(name);
	if (texture == nullptr)
		texture = textureManager->GetTexture(name);
	if (texture == nullptr)
		return script->CreateBooleanValue(false);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	shared_ptr<Texture> current = graphics->GetRenderTarget();

	IDirect3DDevice9* device = graphics->GetDevice();
	graphics->SetRenderTarget(texture, false);
	device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	graphics->SetRenderTarget(current, false);

	return script->CreateBooleanValue(true);
}
gstd::value DxScript::Func_ClearRenderTargetA2(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	TextureManager* textureManager = TextureManager::GetBase();

	DxScriptResourceCache* rsrcCache = script->pResouceCache_;

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = rsrcCache->GetTexture(name);
	if (texture == nullptr)
		texture = textureManager->GetTexture(name);
	if (texture == nullptr)
		return script->CreateBooleanValue(false);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	shared_ptr<Texture> current = graphics->GetRenderTarget();

	__m128i c = Vectorize::SetI(argv[1].as_int(), argv[2].as_int(),
		argv[3].as_int(), argv[4].as_int());
	D3DCOLOR color = ColorAccess::ToD3DCOLOR(ColorAccess::ClampColorPacked(c));

	IDirect3DDevice9* device = graphics->GetDevice();
	graphics->SetRenderTarget(texture, false);
	device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.0f, 0);
	graphics->SetRenderTarget(current, false);

	return script->CreateBooleanValue(true);
}
gstd::value DxScript::Func_ClearRenderTargetA3(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	TextureManager* textureManager = TextureManager::GetBase();

	DxScriptResourceCache* rsrcCache = script->pResouceCache_;

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = rsrcCache->GetTexture(name);
	if (texture == nullptr)
		texture = textureManager->GetTexture(name);
	if (texture == nullptr)
		return script->CreateBooleanValue(false);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	shared_ptr<Texture> current = graphics->GetRenderTarget();

	byte cr = argv[1].as_int();
	byte cg = argv[2].as_int();
	byte cb = argv[3].as_int();
	byte ca = argv[4].as_int();

	DxRect<LONG> rc(argv[5].as_int(), argv[6].as_int(), 
		argv[7].as_int(), argv[8].as_int());

	IDirect3DDevice9* device = graphics->GetDevice();
	graphics->SetRenderTarget(texture, false);
	device->Clear(1, (D3DRECT*)&rc, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(ca, cr, cg, cb), 1.0f, 0);
	graphics->SetRenderTarget(current, false);

	return script->CreateBooleanValue(true);
}
gstd::value DxScript::Func_GetTransitionRenderTargetName(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	const std::wstring& res = TextureManager::TARGET_TRANSITION;
	return DxScript::CreateStringValue(res);
}
gstd::value DxScript::Func_SaveRenderedTextureA1(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	std::wstring nameTexture = argv[0].as_string();
	std::wstring path = argv[1].as_string();
	path = PathProperty::GetUnique(path);

	TextureManager* textureManager = TextureManager::GetBase();
	DirectGraphics* graphics = DirectGraphics::GetBase();

	DxScriptResourceCache* rsrcCache = script->pResouceCache_;

	shared_ptr<Texture> texture = rsrcCache->GetTexture(nameTexture);
	if (texture == nullptr)
		texture = textureManager->GetTexture(nameTexture);

	HRESULT res = E_FAIL;
	if (texture) {
		//Create the directory (if it doesn't exist)
		std::wstring dir = PathProperty::GetFileDirectory(path);
		File::CreateFileDirectory(dir);

		IDirect3DSurface9* pSurface = texture->GetD3DSurface();
		DxRect<LONG> rect(0, 0, graphics->GetScreenWidth(), graphics->GetScreenHeight());
		res = D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_BMP,
			pSurface, nullptr, (RECT*)&rect);
	}

	return script->CreateBooleanValue(SUCCEEDED(res));
}
gstd::value DxScript::Func_SaveRenderedTextureA2(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	std::wstring nameTexture = argv[0].as_string();
	std::wstring path = argv[1].as_string();
	path = PathProperty::GetUnique(path);

	DxRect<LONG> rect(argv[2].as_int(), argv[3].as_int(),
		argv[4].as_int(), argv[5].as_int());

	TextureManager* textureManager = TextureManager::GetBase();
	DirectGraphics* graphics = DirectGraphics::GetBase();

	DxScriptResourceCache* rsrcCache = script->pResouceCache_;

	shared_ptr<Texture> texture = rsrcCache->GetTexture(nameTexture);
	if (texture == nullptr)
		texture = textureManager->GetTexture(nameTexture);

	HRESULT res = E_FAIL;
	if (texture) {
		//Create the directory (if it doesn't exist)
		std::wstring dir = PathProperty::GetFileDirectory(path);
		File::CreateFileDirectory(dir);

		IDirect3DSurface9* pSurface = texture->GetD3DSurface();
		D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_BMP,
			pSurface, nullptr, (RECT*)&rect);
	}

	return script->CreateBooleanValue(SUCCEEDED(res));
}
gstd::value DxScript::Func_SaveRenderedTextureA3(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	std::wstring nameTexture = argv[0].as_string();
	std::wstring path = argv[1].as_string();
	path = PathProperty::GetUnique(path);

	DxRect<LONG> rect(argv[2].as_int(), argv[3].as_int(),
		argv[4].as_int(), argv[5].as_int());
	BYTE imgFormat = argv[6].as_int();

	if (imgFormat < 0)
		imgFormat = 0;
	if (imgFormat > D3DXIFF_PPM)
		imgFormat = D3DXIFF_PPM;

	TextureManager* textureManager = TextureManager::GetBase();
	DirectGraphics* graphics = DirectGraphics::GetBase();

	DxScriptResourceCache* rsrcCache = script->pResouceCache_;

	shared_ptr<Texture> texture = rsrcCache->GetTexture(nameTexture);
	if (texture == nullptr)
		texture = textureManager->GetTexture(nameTexture);

	HRESULT res = E_FAIL;
	if (texture) {
		//Create the directory (if it doesn't exist)
		std::wstring dir = PathProperty::GetFileDirectory(path);
		File::CreateFileDirectory(dir);

		IDirect3DSurface9* pSurface = texture->GetD3DSurface();
		D3DXSaveSurfaceToFile(path.c_str(), (D3DXIMAGE_FILEFORMAT)imgFormat,
			pSurface, nullptr, (RECT*)&rect);
	}

	return script->CreateBooleanValue(SUCCEEDED(res));
}
gstd::value DxScript::Func_IsPixelShaderSupported(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DWORD major = argv[0].as_int();
	DWORD minor = argv[1].as_int();

	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();

	D3DCAPS9 caps;
	device->GetDeviceCaps(&caps);
	return DxScript::CreateBooleanValue(caps.PixelShaderVersion >= D3DPS_VERSION(major, minor));
}
gstd::value DxScript::Func_IsVertexShaderSupported(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DWORD major = argv[0].as_int();
	DWORD minor = argv[1].as_int();

	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();

	D3DCAPS9 caps;
	device->GetDeviceCaps(&caps);
	return DxScript::CreateBooleanValue(caps.VertexShaderVersion >= D3DVS_VERSION(major, minor));
}
gstd::value DxScript::Func_SetEnableAntiAliasing(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	bool enable = argv[0].as_boolean();

	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool res = SUCCEEDED(graphics->SetFullscreenAntiAliasing(enable));

	return DxScript::CreateBooleanValue(res);
}

value DxScript::Func_LoadMesh(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	bool res = true;

	auto& mapMesh = script->pResouceCache_->mapMesh;
	if (mapMesh.find(path) == mapMesh.end()) {
		shared_ptr<DxMesh> mesh = std::make_shared<MetasequoiaMesh>();
		res = mesh->CreateFromFile(path);
		if (res) {
			Lock lock(script->criticalSection_);
			mapMesh[path] = mesh;
		}
	}
	return script->CreateBooleanValue(res);
}
value DxScript::Func_RemoveMesh(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	{
		Lock lock(script->criticalSection_);
		script->pResouceCache_->mapMesh.erase(path);
	}
	return value();
}

value DxScript::Func_LoadShader(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	bool res = true;

	auto& mapShader = script->pResouceCache_->mapShader;
	if (mapShader.find(path) == mapShader.end()) {
		ShaderManager* manager = ShaderManager::GetBase();
		shared_ptr<Shader> shader = manager->CreateFromFile(path);
		res = shader != nullptr;
		if (res) {
			Lock lock(script->criticalSection_);
			mapShader[path] = shader->GetData();
		}
	}
	return script->CreateBooleanValue(res);
}
value DxScript::Func_RemoveShader(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	{
		Lock lock(script->criticalSection_);
		script->pResouceCache_->mapShader.erase(path);
	}
	return value();
}
gstd::value DxScript::Func_SetShader(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();

		double min = argv[1].as_real();
		double max = argv[2].as_real();
		auto objectManager = script->GetObjectManager();
		int size = objectManager->GetRenderBucketCapacity();

		objectManager->SetShader(shader, min * size, max * size);
	}
	return value();
}
gstd::value DxScript::Func_SetShaderI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();

		int min = argv[1].as_int();
		int max = argv[2].as_int();

		auto objectManager = script->GetObjectManager();
		objectManager->SetShader(shader, min, max);
	}
	return value();
}
gstd::value DxScript::Func_ResetShader(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	double min = argv[0].as_real();
	double max = argv[1].as_real();
	auto objectManager = script->GetObjectManager();
	int size = objectManager->GetRenderBucketCapacity();

	objectManager->SetShader(nullptr, min * size, max * size);

	return value();
}
gstd::value DxScript::Func_ResetShaderI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	int min = argv[0].as_int();
	int max = argv[1].as_int();

	auto objectManager = script->GetObjectManager();
	objectManager->SetShader(nullptr, min, max);

	return value();
}

//Dx関数：カメラ3D
value DxScript::Func_SetCameraFocusX(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetFocusX(argv[0].as_real());
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraFocusY(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetFocusY(argv[0].as_real());
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraFocusZ(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetFocusZ(argv[0].as_real());
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraFocusXYZ(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetFocusX(argv[0].as_real());
	camera->SetFocusY(argv[1].as_real());
	camera->SetFocusZ(argv[2].as_real());
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraRadius(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetRadius(argv[0].as_real());
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraAzimuthAngle(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetAzimuthAngle(Math::DegreeToRadian(argv[0].as_real()));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraElevationAngle(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetElevationAngle(Math::DegreeToRadian(argv[0].as_real()));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraYaw(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetYaw(Math::DegreeToRadian(argv[0].as_real()));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraPitch(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetPitch(Math::DegreeToRadian(argv[0].as_real()));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraRoll(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetRoll(Math::DegreeToRadian(argv[0].as_real()));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraMode(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetCameraMode(argv[0].as_int());
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraPosLookAt(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetCameraLookAtVector(D3DXVECTOR3(argv[0].as_real(), argv[1].as_real(), argv[2].as_real()));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_GetCameraX(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetCameraPosition().x;
	return DxScript::CreateRealValue(res);
}
value DxScript::Func_GetCameraY(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetCameraPosition().y;
	return DxScript::CreateRealValue(res);
}
value DxScript::Func_GetCameraZ(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetCameraPosition().z;
	return DxScript::CreateRealValue(res);
}
value DxScript::Func_GetCameraFocusX(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetFocusPosition().x;
	return DxScript::CreateRealValue(res);
}
value DxScript::Func_GetCameraFocusY(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetFocusPosition().y;
	return DxScript::CreateRealValue(res);
}
value DxScript::Func_GetCameraFocusZ(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetFocusPosition().z;
	return DxScript::CreateRealValue(res);
}
value DxScript::Func_GetCameraRadius(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetRadius();
	return DxScript::CreateRealValue(res);
}
value DxScript::Func_GetCameraAzimuthAngle(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetAzimuthAngle();
	return DxScript::CreateRealValue(Math::RadianToDegree(res));
}
value DxScript::Func_GetCameraElevationAngle(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetElevationAngle();
	return DxScript::CreateRealValue(Math::RadianToDegree(res));
}
value DxScript::Func_GetCameraYaw(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetYaw();
	return DxScript::CreateRealValue(Math::RadianToDegree(res));
}
value DxScript::Func_GetCameraPitch(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetPitch();
	return DxScript::CreateRealValue(Math::RadianToDegree(res));
}
value DxScript::Func_GetCameraRoll(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera()->GetRoll();
	return DxScript::CreateRealValue(Math::RadianToDegree(res));
}
value DxScript::Func_SetCameraPerspectiveClip(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	auto camera = graphics->GetCamera();
	camera->SetPerspectiveClip(argv[0].as_real(), argv[1].as_real());
	camera->thisProjectionChanged_ = true;
	return value();
}
value DxScript::Func_GetCameraViewMatrix(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	const D3DXMATRIX* matView = &graphics->GetCamera()->GetViewMatrix();
	return DxScript::CreateRealArrayValue(reinterpret_cast<const FLOAT*>(matView), 16U);
}
value DxScript::Func_GetCameraProjectionMatrix(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	const D3DXMATRIX* matProj = &graphics->GetCamera()->GetProjectionMatrix();
	return DxScript::CreateRealArrayValue(reinterpret_cast<const FLOAT*>(matProj), 16U);
}
value DxScript::Func_GetCameraViewProjectionMatrix(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	const D3DXMATRIX* matViewProj = &graphics->GetCamera()->GetViewProjectionMatrix();
	return DxScript::CreateRealArrayValue(reinterpret_cast<const FLOAT*>(matViewProj), 16U);
}

//Dx関数：カメラ2D
gstd::value DxScript::Func_Set2DCameraFocusX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetFocusX(argv[0].as_real());
	return gstd::value();
}
gstd::value DxScript::Func_Set2DCameraFocusY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetFocusY(argv[0].as_real());
	return gstd::value();
}
gstd::value DxScript::Func_Set2DCameraAngleZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetAngleZ(Math::DegreeToRadian(argv[0].as_real()));
	return gstd::value();
}
gstd::value DxScript::Func_Set2DCameraRatio(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetRatio(argv[0].as_real());
	return gstd::value();
}
gstd::value DxScript::Func_Set2DCameraRatioX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetRatioX(argv[0].as_real());
	return gstd::value();
}
gstd::value DxScript::Func_Set2DCameraRatioY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetRatioY(argv[0].as_real());
	return gstd::value();
}
gstd::value DxScript::Func_Reset2DCamera(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->Reset();
	return gstd::value();
}
gstd::value DxScript::Func_Get2DCameraX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera2D()->GetFocusX();
	return DxScript::CreateRealValue(res);
}
gstd::value DxScript::Func_Get2DCameraY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	float res = graphics->GetCamera2D()->GetFocusY();
	return DxScript::CreateRealValue(res);
}
gstd::value DxScript::Func_Get2DCameraAngleZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera2D()->GetAngleZ();
	return DxScript::CreateRealValue(Math::RadianToDegree(res));
}
gstd::value DxScript::Func_Get2DCameraRatio(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera2D()->GetRatio();
	return DxScript::CreateRealValue(res);
}
gstd::value DxScript::Func_Get2DCameraRatioX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera2D()->GetRatioX();
	return DxScript::CreateRealValue(res);
}
gstd::value DxScript::Func_Get2DCameraRatioY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera2D()->GetRatioY();
	return DxScript::CreateRealValue(res);
}

//Dx関数：その他
static inline bool IsDxObjValid3D(DxScriptObjectBase* obj) {
	switch (obj->GetObjectType()) {
	case TypeObject::Primitive3D:
	case TypeObject::Sprite3D:
	case TypeObject::Trajectory3D:
	case TypeObject::ParticleList3D:
	case TypeObject::Mesh:
		return true;
	}
	return false;
}
gstd::value DxScript::Func_GetObjectDistance(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id1 = argv[0].as_int();
	int id2 = argv[1].as_int();

	FLOAT res = -1.0f;
	if (DxScriptRenderObject* obj1 = script->GetObjectPointerAs<DxScriptRenderObject>(id1)) {
		if (DxScriptRenderObject* obj2 = script->GetObjectPointerAs<DxScriptRenderObject>(id2)) {
			if (IsDxObjValid3D(obj1) || IsDxObjValid3D(obj2)) {
				D3DXVECTOR3 diff = obj1->GetPosition() - obj2->GetPosition();
				res = D3DXVec3Length(&diff);
			}
			else {
				D3DXVECTOR2 diff = { 
					obj1->GetPosition().x - obj2->GetPosition().x,
					obj1->GetPosition().y - obj2->GetPosition().y };
				res = D3DXVec2Length(&diff);
			}
		}
	}
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_GetObjectDistanceSq(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id1 = argv[0].as_int();
	int id2 = argv[1].as_int();

	FLOAT res = -1.0f;
	if (DxScriptRenderObject* obj1 = script->GetObjectPointerAs<DxScriptRenderObject>(id1)) {
		if (DxScriptRenderObject* obj2 = script->GetObjectPointerAs<DxScriptRenderObject>(id2)) {
			if (IsDxObjValid3D(obj1) || IsDxObjValid3D(obj2)) {
				D3DXVECTOR3 diff = obj1->GetPosition() - obj2->GetPosition();
				res = D3DXVec3LengthSq(&diff);
			}
			else {
				D3DXVECTOR2 diff = {
					obj1->GetPosition().x - obj2->GetPosition().x,
					obj1->GetPosition().y - obj2->GetPosition().y };
				res = D3DXVec2LengthSq(&diff);
			}
		}
	}
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_GetObjectDeltaAngle(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id1 = argv[0].as_int();
	int id2 = argv[1].as_int();

	double res = 0;
	if (DxScriptRenderObject* obj1 = script->GetObjectPointerAs<DxScriptRenderObject>(id1)) {
		if (DxScriptRenderObject* obj2 = script->GetObjectPointerAs<DxScriptRenderObject>(id2)) {
			bool bValid3D = IsDxObjValid3D(obj1) && IsDxObjValid3D(obj2);
			D3DXVECTOR3& pos1 = obj1->GetPosition();
			D3DXVECTOR3& pos2 = obj2->GetPosition();
			if (bValid3D && fabs(pos1.z - pos2.z) < 0.01f) {
				double dot = D3DXVec3Dot(&pos1, &pos2);
				double len1 = D3DXVec3Length(&pos1);
				double len2 = D3DXVec3Length(&pos2);
				res = acos(dot / (len1 * len2));
			}
			else {
				res = atan2(pos2.y - pos1.y, pos2.x - pos1.x);
			}
		}
	}
	return script->CreateRealValue(Math::RadianToDegree(res));
}
gstd::value DxScript::Func_GetObject2dPosition(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	float listRes[2] = { (float)DxScript::g_posInvalidX_, (float)DxScript::g_posInvalidY_ };

	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		ref_count_ptr<DxCamera> camera = graphics->GetCamera();

		D3DXVECTOR2 point = camera->TransformCoordinateTo2D(obj->GetPosition());
		listRes[0] = point.x;
		listRes[1] = point.y;
	}

	return script->CreateRealArrayValue(listRes, 2U);
}
gstd::value DxScript::Func_Get2dPosition(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	float px = argv[0].as_real();
	float py = argv[1].as_real();
	float pz = argv[2].as_real();
	D3DXVECTOR3 pos(px, py, pz);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();

	D3DXVECTOR2 point = camera->TransformCoordinateTo2D(pos);
	float listRes[2] = { point.x, point.y };

	return script->CreateRealArrayValue(listRes, 2U);
}

//Intersection
static std::vector<DxPoint> _script_value_to_dxpolygon(script_machine* machine, const gstd::value* v) {
	std::vector<DxPoint> res;

	if (!v->has_data())
		goto lab_value_invalid;

	type_data* typeElem = v->get_type()->get_element();
	if (typeElem == nullptr)
		goto lab_value_invalid;

	res.resize(v->length_as_array());
	for (size_t i = 0; i < res.size(); ++i) {
		const value& subArray = v->index_as_array(i);

		type_data* typeElemElem = typeElem->get_element();
		if (typeElemElem == nullptr || subArray.length_as_array() != 2U)
			goto lab_value_invalid;

		res[i] = DxPoint(subArray.index_as_array(0).as_real(), 
			subArray.index_as_array(1).as_real());
	}

	goto lab_return;
lab_value_invalid:
	{
		std::string err = "Invalid value for polygon.";
		machine->raise_error(err);
	}
lab_return:
	return res;
}
gstd::value DxScript::Func_IsIntersected_Point_Polygon(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxPoint point(argv[0].as_real(), argv[1].as_real());
	std::vector<DxPoint> polygon = _script_value_to_dxpolygon(machine, &argv[2]);

	bool res = DxIntersect::Point_Polygon(&point, &polygon);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Point_Circle(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxPoint point(argv[0].as_real(), argv[1].as_real());
	DxCircle circle(argv[2].as_real(), argv[3].as_real(), argv[4].as_real());

	bool res = DxIntersect::Point_Circle(&point, &circle);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Point_Ellipse(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxPoint point(argv[0].as_real(), argv[1].as_real());
	DxEllipse ellipse(argv[2].as_real(), argv[3].as_real(), 
		argv[4].as_real(), argv[5].as_real());

	bool res = DxIntersect::Point_Ellipse(&point, &ellipse);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Point_Line(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxPoint point(argv[0].as_real(), argv[1].as_real());
	DxWidthLine line(argv[2].as_real(), argv[3].as_real(), argv[4].as_real(),
		argv[5].as_real(), argv[6].as_real());

	bool res = DxIntersect::Point_LineW(&point, &line);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Point_RegularPolygon(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxPoint point(argv[0].as_real(), argv[1].as_real());
	DxRegularPolygon rPolygon(argv[2].as_real(), argv[3].as_real(), 
		argv[4].as_real(), argv[5].as_int(), Math::DegreeToRadian(argv[6].as_real()));

	bool res = DxIntersect::Point_RegularPolygon(&point, &rPolygon);
	return DxScript::CreateBooleanValue(res);
}

gstd::value DxScript::Func_IsIntersected_Circle_Polygon(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxCircle circle(argv[0].as_real(), argv[1].as_real(), argv[2].as_real());
	std::vector<DxPoint> polygon = _script_value_to_dxpolygon(machine, &argv[3]);

	bool res = DxIntersect::Circle_Polygon(&circle, &polygon);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Circle_Circle(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxCircle circle1(argv[0].as_real(), argv[1].as_real(), argv[2].as_real());
	DxCircle circle2(argv[3].as_real(), argv[4].as_real(), argv[5].as_real());

	bool res = DxIntersect::Circle_Circle(&circle1, &circle2);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Circle_Ellipse(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxCircle circle(argv[0].as_real(), argv[1].as_real(), argv[2].as_real());
	DxEllipse ellipse(argv[3].as_real(), argv[4].as_real(),
		argv[5].as_real(), argv[6].as_real());

	bool res = DxIntersect::Circle_Ellipse(&circle, &ellipse);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Circle_RegularPolygon(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxCircle circle(argv[0].as_real(), argv[1].as_real(), argv[2].as_real());
	DxRegularPolygon rPolygon(argv[3].as_real(), argv[4].as_real(), 
		argv[5].as_real(), argv[6].as_int(), Math::DegreeToRadian(argv[7].as_real()));

	bool res = DxIntersect::Circle_RegularPolygon(&circle, &rPolygon);
	return DxScript::CreateBooleanValue(res);
}

gstd::value DxScript::Func_IsIntersected_Line_Polygon(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxWidthLine line(argv[0].as_real(), argv[1].as_real(), argv[2].as_real(),
		argv[3].as_real(), argv[4].as_real());
	std::vector<DxPoint> polygon = _script_value_to_dxpolygon(machine, &argv[5]);

	bool res = DxIntersect::LineW_Polygon(&line, &polygon);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Line_Circle(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxWidthLine line(argv[0].as_real(), argv[1].as_real(), argv[2].as_real(),
		argv[3].as_real(), argv[4].as_real());
	DxCircle circle(argv[5].as_real(), argv[6].as_real(), argv[7].as_real());

	bool res = DxIntersect::Line_Circle(&line, &circle);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Line_Ellipse(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxWidthLine line(argv[0].as_real(), argv[1].as_real(), argv[2].as_real(),
		argv[3].as_real(), argv[4].as_real());
	DxEllipse ellipse(argv[5].as_real(), argv[6].as_real(),
		argv[7].as_real(), argv[8].as_real());

	bool res = DxIntersect::Line_Ellipse(&line, &ellipse);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Line_Line(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxWidthLine line1(argv[0].as_real(), argv[1].as_real(), argv[2].as_real(),
		argv[3].as_real(), argv[4].as_real());
	DxWidthLine line2(argv[5].as_real(), argv[6].as_real(), argv[7].as_real(),
		argv[8].as_real(), argv[9].as_real());

	bool res = DxIntersect::LineW_LineW(&line1, &line2);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Line_RegularPolygon(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxWidthLine line(argv[0].as_real(), argv[1].as_real(), argv[2].as_real(),
		argv[3].as_real(), argv[4].as_real());
	DxRegularPolygon rPolygon(argv[5].as_real(), argv[6].as_real(),
		argv[7].as_real(), argv[8].as_int(), Math::DegreeToRadian(argv[9].as_real()));

	bool res = DxIntersect::LineW_RegularPolygon(&line, &rPolygon);
	return DxScript::CreateBooleanValue(res);
}

gstd::value DxScript::Func_IsIntersected_Polygon_Polygon(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	std::vector<DxPoint> polygon1 = _script_value_to_dxpolygon(machine, &argv[0]);
	std::vector<DxPoint> polygon2 = _script_value_to_dxpolygon(machine, &argv[1]);

	bool res = DxIntersect::Polygon_Polygon(&polygon1, &polygon2);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Polygon_Ellipse(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	std::vector<DxPoint> polygon = _script_value_to_dxpolygon(machine, &argv[0]);
	DxEllipse ellipse(argv[1].as_real(), argv[2].as_real(),
		argv[3].as_real(), argv[4].as_real());

	bool res = DxIntersect::Polygon_Ellipse(&polygon, &ellipse);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Polygon_RegularPolygon(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	std::vector<DxPoint> polygon = _script_value_to_dxpolygon(machine, &argv[0]);
	DxRegularPolygon rPolygon(argv[1].as_real(), argv[2].as_real(),
		argv[3].as_real(), argv[4].as_int(), Math::DegreeToRadian(argv[5].as_real()));

	bool res = DxIntersect::Polygon_RegularPolygon(&polygon, &rPolygon);
	return DxScript::CreateBooleanValue(res);
}

//Color
gstd::value DxScript::Func_ColorARGBToHex(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	byte ca = 0xff;
	byte cr = 0xff;
	byte cg = 0xff;
	byte cb = 0xff;
	if (argc == 4) {
		ca = argv[0].as_int();
		cr = argv[1].as_int();
		cg = argv[2].as_int();
		cb = argv[3].as_int();
	}
	else {
		const value& val = argv[0];
		if (DxScript::IsArrayValue(const_cast<value&>(val))) {
			if (val.length_as_array() >= 4) {
				ca = val.index_as_array(0).as_int();
				cr = val.index_as_array(1).as_int();
				cg = val.index_as_array(2).as_int();
				cb = val.index_as_array(3).as_int();
			}
		}
	}
	return DxScript::CreateIntValue(D3DCOLOR_ARGB(ca, cr, cg, cb));
}
gstd::value DxScript::Func_ColorHexToARGB(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	D3DCOLOR color = (D3DCOLOR)argv[0].as_int();

	byte nChannel = 4;
	uint16_t permute = ColorAccess::PERMUTE_ARGB;
	D3DXVECTOR4 vecColor;

	if (argc > 1) {
		//Control format
		//MSB                         LSB   (16 bit)
		//000000   00      00  00  00  00
		//Unused   Count   1st 2nd 3rd 4th

		uint16_t control = argv[1].as_int() & 0x3ff;
		if (control != 0x31b) {		//0x31b -> 11 00 01 10 11 (regular ARGB)
			nChannel = ((control >> 8) & 0x3) + 1;
			permute = control & 0xff;
		}
	}

	vecColor = ColorAccess::ToVec4(color, permute);
	return DxScript::CreateIntArrayValue(reinterpret_cast<FLOAT*>(&vecColor), nChannel);
}
gstd::value DxScript::Func_ColorRGBtoHSV(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	byte cr = argv[0].as_int();
	byte cg = argv[1].as_int();
	byte cb = argv[2].as_int();

	D3DXVECTOR3 hsv;
	ColorAccess::RGBtoHSV(hsv, cr, cg, cb);

	return DxScript::CreateIntArrayValue(reinterpret_cast<FLOAT*>(&hsv), 3U);
}
gstd::value DxScript::Func_ColorHSVtoRGB(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	int ch = argv[0].as_int();
	byte cs = argv[1].as_int();
	byte cv = argv[2].as_int();
	
	D3DCOLOR rgb = 0xffffffff;
	ColorAccess::HSVtoRGB(rgb, ch, cs, cv);

	byte listColor[4];
	ColorAccess::ToByteArray(rgb, listColor);
	return DxScript::CreateIntArrayValue(listColor + 1, 3U);
}
gstd::value DxScript::Func_ColorHSVtoHexRGB(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	int ch = argv[0].as_int();
	byte cs = argv[1].as_int();
	byte cv = argv[2].as_int();

	D3DCOLOR rgb = 0xffffffff;
	ColorAccess::HSVtoRGB(rgb, ch, cs, cv);

	return DxScript::CreateIntValue(rgb);
}

//Other stuff
value DxScript::Func_SetInvalidPositionReturn(script_machine* machine, int argc, const value* argv) {
	DxScript::g_posInvalidX_ = argv[0].as_real();
	DxScript::g_posInvalidY_ = argv[1].as_real();
	//DxScript::g_posInvalidZ_ = argv[2].as_real();
	return value();
}

//Dx関数：オブジェクト操作(共通)
value DxScript::Func_Obj_Delete(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	script->CheckRunInMainThread();
	int id = argv[0].as_int();
	script->DeleteObject(id);
	return value();
}
value DxScript::Func_Obj_IsDeleted(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	return script->CreateBooleanValue(obj == nullptr);
}
value DxScript::Func_Obj_IsExists(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	return script->CreateBooleanValue(obj != nullptr);
}
value DxScript::Func_Obj_SetVisible(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj)
		obj->bVisible_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_Obj_IsVisible(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	bool res = false;
	if (obj) res = obj->bVisible_;
	return script->CreateBooleanValue(res);
}
value DxScript::Func_Obj_SetRenderPriority(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		size_t maxPri = script->GetObjectManager()->GetRenderBucketCapacity() - 1U;
		double pri = argv[1].as_real();

		if (pri < 0) pri = 0;
		else if (pri > 1) pri = 1;

		obj->priRender_ = pri * maxPri;
	}
	return value();
}
value DxScript::Func_Obj_SetRenderPriorityI(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		int pri = argv[1].as_int();
		size_t maxPri = script->GetObjectManager()->GetRenderBucketCapacity() - 1U;

		if (pri < 0) pri = 0;
		else if (pri > maxPri) pri = maxPri;

		obj->priRender_ = pri;
	}
	return value();
}
gstd::value DxScript::Func_Obj_GetRenderPriority(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = 0;

	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj)
		res = obj->GetRenderPriorityI() / (double)(script->GetObjectManager()->GetRenderBucketCapacity() - 1U);

	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_Obj_GetRenderPriorityI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = 0;

	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) res = obj->GetRenderPriorityI();

	return script->CreateRealValue(res);
}

gstd::value DxScript::Func_Obj_GetValue(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	std::wstring key = argv[1].as_string();

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		auto itr = obj->mapObjectValue_.find(DxScriptObjectBase::GetKeyHash(key));
		if (itr != obj->mapObjectValue_.end()) return itr->second;
	}
	return value();
}
gstd::value DxScript::Func_Obj_GetValueD(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	std::wstring key = argv[1].as_string();
	gstd::value def = argv[2];

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		auto itr = obj->mapObjectValue_.find(DxScriptObjectBase::GetKeyHash(key));
		if (itr != obj->mapObjectValue_.end()) return itr->second;
	}
	return def;
}
gstd::value DxScript::Func_Obj_SetValue(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	std::wstring key = argv[1].as_string();
	gstd::value val = argv[2];

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) obj->SetObjectValue(key, val);

	return value();
}
gstd::value DxScript::Func_Obj_DeleteValue(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	std::wstring key = argv[1].as_string();

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) obj->DeleteObjectValue(key);

	return value();
}
gstd::value DxScript::Func_Obj_IsValueExists(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	std::wstring key = argv[1].as_string();

	bool res = false;

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) res = obj->IsObjectValueExists(key);

	return script->CreateBooleanValue(res);
}

gstd::value DxScript::Func_Obj_GetValueI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int64_t keyInt = argv[1].as_int();

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		auto itr = obj->mapObjectValue_.find(DxScriptObjectBase::GetKeyHashI64(keyInt));
		if (itr != obj->mapObjectValue_.end()) return itr->second;
	}
	return value();
}
gstd::value DxScript::Func_Obj_GetValueDI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int64_t keyInt = argv[1].as_int();
	gstd::value def = argv[2];

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		auto itr = obj->mapObjectValue_.find(DxScriptObjectBase::GetKeyHashI64(keyInt));
		if (itr != obj->mapObjectValue_.end()) return itr->second;
	}
	return def;
}
gstd::value DxScript::Func_Obj_SetValueI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int64_t keyInt = argv[1].as_int();
	gstd::value val = argv[2];

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		obj->SetObjectValue(DxScriptObjectBase::GetKeyHashI64(keyInt), val);
	}

	return value();
}
gstd::value DxScript::Func_Obj_DeleteValueI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int64_t keyInt = argv[1].as_int();

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		obj->DeleteObjectValue(DxScriptObjectBase::GetKeyHashI64(keyInt));
	}

	return value();
}
gstd::value DxScript::Func_Obj_IsValueExistsI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	double keyDbl = argv[1].as_real();

	bool res = false;

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		res = obj->IsObjectValueExists(DxScriptObjectBase::GetKeyHashI64(keyDbl));
	}

	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_Obj_CopyValueTable(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	size_t countValue = 0;

	int idDst = argv[0].as_int();
	DxScriptObjectBase* objDst = script->GetObjectPointer(idDst);
	if (objDst) {
		int idSrc = argv[1].as_int();
		DxScriptObjectBase* objSrc = script->GetObjectPointer(idSrc);
		if (objSrc) {
			auto& valMap = objSrc->mapObjectValue_;
			countValue = valMap.size();

			int copyMode = argv[2].as_int();
			//Mode 0 - Clear dest and copy
			if (copyMode == 0) {
				objDst->mapObjectValue_.clear();
				for (auto itr = valMap.begin(); itr != valMap.end(); ++itr)
					objDst->mapObjectValue_.insert(*itr);
			}
			//Mode 1 - Source takes priority (Always overwrite)
			else if (copyMode == 1) {
				for (auto itr = valMap.begin(); itr != valMap.end(); ++itr) {
					auto itrDst = objSrc->mapObjectValue_.find(itr->first);
					if (itrDst != objSrc->mapObjectValue_.end())
						itrDst->second = itr->second;
					else
						objDst->mapObjectValue_.insert(*itr);
				}
			}
			//Mode 2 - Dest takes priority (No overwrite)
			else if (copyMode == 2) {
				for (auto itr = valMap.begin(); itr != valMap.end(); ++itr) {
					auto itrDst = objSrc->mapObjectValue_.find(itr->first);
					if (itrDst == objSrc->mapObjectValue_.end())
						objDst->mapObjectValue_.insert(*itr);
				}
			}
		}
	}

	return script->CreateIntValue(countValue);
}

value DxScript::Func_Obj_GetType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	TypeObject res = TypeObject::Invalid;

	DxScriptObjectBase* obj = script->GetObjectPointerAs<DxScriptObjectBase>(id);
	if (obj) res = obj->GetObjectType();

	return script->CreateIntValue((uint8_t)res);
}


//Dx関数：オブジェクト操作(RenderObject)
value DxScript::Func_ObjRender_SetX(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetX(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetY(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetY(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetZ(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		obj->SetX(argv[1].as_real());
		obj->SetY(argv[2].as_real());
		if (argc == 4) obj->SetZ(argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjRender_SetAngleX(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetAngleX(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetAngleY(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetAngleY(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetAngleZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetAngleZ(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetAngleXYZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		obj->SetAngleX(argv[1].as_real());
		obj->SetAngleY(argv[2].as_real());
		obj->SetAngleZ(argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjRender_SetScaleX(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetScaleX(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetScaleY(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetScaleY(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetScaleZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetScaleZ(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetScaleXYZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		if (argc == 4) {
			obj->SetScaleX(argv[1].as_real());
			obj->SetScaleY(argv[2].as_real());
			obj->SetScaleZ(argv[3].as_real());
		}
		else {
			float scale = argv[1].as_real();
			obj->SetScaleX(scale);
			obj->SetScaleY(scale);
			obj->SetScaleZ(scale);
		}
	}
	return value();
}
value DxScript::Func_ObjRender_SetColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		if (argc == 4) {
			obj->SetColor(argv[1].as_int(), argv[2].as_int(), argv[3].as_int());
		}
		else {
			D3DCOLOR color = argv[1].as_int();
			obj->SetColor(ColorAccess::GetColorR(color), ColorAccess::GetColorG(color), 
				ColorAccess::GetColorB(color));
		}
	}
	return value();
}
value DxScript::Func_ObjRender_SetColorHSV(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		D3DCOLOR color = 0xffffffff;
		ColorAccess::HSVtoRGB(color, argv[1].as_int(), argv[2].as_int(), argv[3].as_int());

		obj->SetColor(ColorAccess::GetColorR(color), ColorAccess::GetColorG(color), 
			ColorAccess::GetColorB(color));
	}
	return value();
}
value DxScript::Func_ObjRender_GetColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	D3DCOLOR color = 0xffffffff;
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) color = obj->color_;

    byte listColor[4];
    ColorAccess::ToByteArray(color, listColor);
    return script->CreateIntArrayValue(listColor + 1, 3U);	
}
value DxScript::Func_ObjRender_GetColorHex(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	D3DCOLOR color = 0xffffffff;
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) color = obj->color_;

	return script->CreateIntValue(color - 0xff000000); // Investigate better methods later
}
value DxScript::Func_ObjRender_SetAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetAlpha(argv[1].as_int());
	return value();
}
value DxScript::Func_ObjRender_GetAlpha(script_machine* machine, int argc, const value* argv) {
	byte res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		res = ColorAccess::GetColorA(obj->color_);
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjRender_SetBlendType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetBlendType((BlendMode)argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_GetX(script_machine* machine, int argc, const value* argv) {
	FLOAT res = DxScript::g_posInvalidX_;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		res = obj->position_.x;
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjRender_GetY(script_machine* machine, int argc, const value* argv) {
	FLOAT res = DxScript::g_posInvalidY_;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		res = obj->position_.y;
	return script->CreateRealValue(res);
}

value DxScript::Func_ObjRender_GetZ(script_machine* machine, int argc, const value* argv) {
	FLOAT res = DxScript::g_posInvalidZ_;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		res = obj->position_.z;
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjRender_GetPosition(script_machine* machine, int argc, const value* argv) {
	D3DXVECTOR3 pos(DxScript::g_posInvalidX_, DxScript::g_posInvalidY_, DxScript::g_posInvalidZ_);
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		pos = obj->position_;

	return script->CreateRealArrayValue(reinterpret_cast<float*>(&pos), 3U);
}
gstd::value DxScript::Func_ObjRender_GetAngleX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		res = obj->angle_.x;
	return script->CreateRealValue(Math::RadianToDegree(res));
}
gstd::value DxScript::Func_ObjRender_GetAngleY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		res = obj->angle_.y;
	return script->CreateRealValue(Math::RadianToDegree(res));
}
gstd::value DxScript::Func_ObjRender_GetAngleZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		res = obj->angle_.z;
	return script->CreateRealValue(Math::RadianToDegree(res));
}
gstd::value DxScript::Func_ObjRender_GetScaleX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		res = obj->scale_.x;
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjRender_GetScaleY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		res = obj->scale_.y;
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjRender_GetScaleZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		res = obj->scale_.z;
	return script->CreateRealValue(res);
}

value DxScript::Func_ObjRender_SetZWrite(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->bZWrite_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_ObjRender_SetZTest(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->bZTest_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_ObjRender_SetFogEnable(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->bFogEnable_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_ObjRender_SetCullingMode(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->modeCulling_ = (D3DCULL)argv[1].as_int();
	return value();
}
value DxScript::Func_ObjRender_SetRelativeObject(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		int idRelative = argv[1].as_int();
		std::wstring nameBone = argv[2].as_string();
		auto objRelative = ref_unsync_ptr<DxScriptRenderObject>::Cast(script->GetObject(idRelative));
		if (objRelative)
			obj->SetRelativeObject(objRelative, nameBone);
	}
	return value();
}
value DxScript::Func_ObjRender_SetPermitCamera(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	bool bEnable = argv[1].as_boolean();

	DxScriptObjectBase* pObj = script->GetObjectPointer(id);
	DxScriptPrimitiveObject2D* obj2D = dynamic_cast<DxScriptPrimitiveObject2D*>(pObj);
	DxScriptTextObject* objText = dynamic_cast<DxScriptTextObject*>(pObj);
	if (obj2D)
		obj2D->SetPermitCamera(bEnable);
	else if (objText)
		objText->SetPermitCamera(bEnable);

	return value();
}

gstd::value DxScript::Func_ObjRender_GetBlendType(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	BlendMode res = MODE_BLEND_NONE;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		res = obj->GetBlendType();
	return script->CreateIntValue((int)res);
}

value DxScript::Func_ObjRender_SetTextureFilterMin(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int type = argv[1].as_int();

	type = std::clamp(type, (int)D3DTEXF_NONE, (int)D3DTEXF_ANISOTROPIC);

	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->filterMin_ = (D3DTEXTUREFILTERTYPE)type;

	return value();
}
value DxScript::Func_ObjRender_SetTextureFilterMag(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int type = argv[1].as_int();

	type = std::clamp(type, (int)D3DTEXF_NONE, (int)D3DTEXF_ANISOTROPIC);

	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->filterMag_ = (D3DTEXTUREFILTERTYPE)type;

	return value();
}
value DxScript::Func_ObjRender_SetTextureFilterMip(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int type = argv[1].as_int();

	type = std::clamp(type, (int)D3DTEXF_NONE, (int)D3DTEXF_ANISOTROPIC);

	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->filterMip_ = (D3DTEXTUREFILTERTYPE)type;

	return value();
}
value DxScript::Func_ObjRender_SetVertexShaderRenderingMode(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->bVertexShaderMode_ = argv[1].as_boolean();

	return value();
}
value DxScript::Func_ObjRender_SetEnableDefaultTransformMatrix(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->bEnableMatrix_ = argv[1].as_boolean();

	return value();
}
value DxScript::Func_ObjRender_SetLightingEnable(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		if (auto objLight = obj->GetLightPointer()) {
			objLight->SetLightEnable(argv[1].as_boolean());
			objLight->SetSpecularEnable(argv[2].as_boolean());
		}
	}

	return value();
}
value DxScript::Func_ObjRender_SetLightingDiffuseColor(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		if (auto objLight = obj->GetLightPointer()) {
			D3DCOLORVALUE color = { 1.0f, 1.0f, 1.0f, 0.0f };
			if (argc == 4) {
				color.r = (float)argv[1].as_real() / 255.0f;
				color.g = (float)argv[2].as_real() / 255.0f;
				color.b = (float)argv[3].as_real() / 255.0f;
			}
			else {
				D3DXVECTOR4 colorNorm = ColorAccess::ToVec4Normalized((D3DCOLOR)argv[1].as_int(), ColorAccess::PERMUTE_RGBA);
				memcpy(&color, &colorNorm, sizeof(FLOAT) * 3U);
			}
			objLight->SetColorDiffuse(color);
		}
	}

	return value();
}
value DxScript::Func_ObjRender_SetLightingSpecularColor(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		if (auto objLight = obj->GetLightPointer()) {
			D3DCOLORVALUE color = { 1.0f, 1.0f, 1.0f, 0.0f };
			if (argc == 4) {
				color.r = (float)argv[1].as_real() / 255.0f;
				color.g = (float)argv[2].as_real() / 255.0f;
				color.b = (float)argv[3].as_real() / 255.0f;
			}
			else {
				D3DXVECTOR4 colorNorm = ColorAccess::ToVec4Normalized((D3DCOLOR)argv[1].as_int(), ColorAccess::PERMUTE_RGBA);
				memcpy(&color, &colorNorm, sizeof(FLOAT) * 3U);
			}
			objLight->SetColorSpecular(color);
		}
	}

	return value();
}
value DxScript::Func_ObjRender_SetLightingAmbientColor(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		if (auto objLight = obj->GetLightPointer()) {
			D3DCOLORVALUE color = { 1.0f, 1.0f, 1.0f, 0.0f };
			if (argc == 4) {
				color.r = (float)argv[1].as_real() / 255.0f;
				color.g = (float)argv[2].as_real() / 255.0f;
				color.b = (float)argv[3].as_real() / 255.0f;
			}
			else {
				D3DXVECTOR4 colorNorm = ColorAccess::ToVec4Normalized((D3DCOLOR)argv[1].as_int(), ColorAccess::PERMUTE_RGBA);
				memcpy(&color, &colorNorm, sizeof(FLOAT) * 3U);
			}
			objLight->SetColorAmbient(color);
		}
	}

	return value();
}
value DxScript::Func_ObjRender_SetLightingDirection(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		if (auto objLight = obj->GetLightPointer())
			objLight->SetDirection(D3DXVECTOR3(argv[1].as_real(), argv[2].as_real(), argv[3].as_real()));
	}

	return value();
}

//Dx関数：オブジェクト操作(ShaderObject)
gstd::value DxScript::Func_ObjShader_Create(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	script->CheckRunInMainThread();

	ref_unsync_ptr<DxScriptShaderObject> obj = new DxScriptShaderObject();

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateIntValue(id);
}
gstd::value DxScript::Func_ObjShader_SetShaderF(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	bool res = false;
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		ShaderManager* manager = ShaderManager::GetBase();
		auto& mapShader = script->pResouceCache_->mapShader;

		shared_ptr<Shader> shader = nullptr;

		auto itr = mapShader.find(path);
		if (itr != mapShader.end()) {
			shader = manager->CreateFromData(itr->second);
		}
		else {
			shader = manager->CreateFromFile(path);
			if (shader == nullptr) {
				const std::wstring& error = manager->GetLastError();
				script->RaiseError(error);
			}
		}

		obj->SetShader(shader);
		res = shader != nullptr;
	}
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_ObjShader_SetShaderO(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = false;

	int id1 = argv[0].as_int();
	DxScriptRenderObject* obj1 = script->GetObjectPointerAs<DxScriptRenderObject>(id1);
	if (obj1) {
		int id2 = argv[1].as_int();
		DxScriptRenderObject* obj2 = script->GetObjectPointerAs<DxScriptRenderObject>(id2);
		if (obj2) {
			shared_ptr<Shader> shader = obj2->GetShader();
			if (shader) {
				obj1->SetShader(shader);
				res = true;
			}
		}
	}
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_ObjShader_ResetShader(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj)
		obj->SetShader(nullptr);
	return value();
}
gstd::value DxScript::Func_ObjShader_SetTechnique(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string aPath = StringUtility::ConvertWideToMulti(argv[1].as_string());
			shader->SetTechnique(aPath);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetMatrix(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			const gstd::value& sMatrix = argv[2];

			type_data* type_matrix = script_type_manager::get_real_array_type();
			if (sMatrix.get_type() == type_matrix) {
				if (sMatrix.length_as_array() == 16) {
					D3DXMATRIX matrix;
					FLOAT* ptrMat = &matrix._11;
					for (size_t i = 0; i < 16; ++i) {
						const value& arrayValue = sMatrix.index_as_array(i);
						ptrMat[i] = (FLOAT)arrayValue.as_real();
					}
					shader->SetMatrix(name, matrix);
				}
			}
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetMatrixArray(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			const gstd::value& array = argv[2];
			
			type_data* type_matrix = script_type_manager::get_real_array_type();
			type_data* type_matrix_array = script_type_manager::get_instance()->get_array_type(type_matrix);
			if (array.get_type() == type_matrix_array) {
				std::vector<D3DXMATRIX> listMatrix;
				for (size_t iArray = 0; iArray < array.length_as_array(); ++iArray) {
					const value& sMatrix = array.index_as_array(iArray);
					if (sMatrix.length_as_array() == 16) {
						D3DXMATRIX matrix;
						FLOAT* ptrMat = &matrix._11;
						for (size_t i = 0; i < 16; ++i) {
							const value& arrayValue = sMatrix.index_as_array(i);
							ptrMat[i] = (FLOAT)arrayValue.as_real();
						}
						listMatrix.push_back(matrix);
					}
				}
				shader->SetMatrixArray(name, listMatrix);
			}
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetVector(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			D3DXVECTOR4 vect4;
			vect4.x = (FLOAT)argv[2].as_real();
			vect4.y = (FLOAT)argv[3].as_real();
			vect4.z = (FLOAT)argv[4].as_real();
			vect4.w = (FLOAT)argv[5].as_real();

			shader->SetVector(name, vect4);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetFloat(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			shader->SetFloat(name, (FLOAT)argv[2].as_real());
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetFloatArray(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			const gstd::value& array = argv[2];

			type_data* type_array = script_type_manager::get_real_array_type();
			if (array.get_type() == type_array) {
				std::vector<FLOAT> listFloat;
				for (size_t iArray = 0; iArray < array.length_as_array(); ++iArray) {
					const value& aValue = array.index_as_array(iArray);
					listFloat.push_back((FLOAT)aValue.as_real());
				}
				shader->SetFloatArray(name, listFloat);
			}
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetTexture(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptRenderObject* obj = script->GetObjectPointerAs<DxScriptRenderObject>(id);
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			std::wstring path = argv[2].as_string();
			path = PathProperty::GetUnique(path);

			auto& mapTexture = script->pResouceCache_->mapTexture;

			auto itr = mapTexture.find(path);
			if (itr != mapTexture.end()) {
				shader->SetTexture(name, itr->second);
			}
			else {
				shared_ptr<Texture> texture(new Texture());
				texture->CreateFromFile(path, false, false);
				shader->SetTexture(name, texture);
			}
		}
	}
	return value();
}

//Dx関数：オブジェクト操作(PrimitiveObject)
value DxScript::Func_ObjPrimitive_Create(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	script->CheckRunInMainThread();
	TypeObject type = (TypeObject)argv[0].as_int();

	ref_unsync_ptr<DxScriptRenderObject> obj;
	if (type == TypeObject::Primitive2D) {
		obj = new DxScriptPrimitiveObject2D();
	}
	else if (type == TypeObject::Sprite2D) {
		obj = new DxScriptSpriteObject2D();
	}
	else if (type == TypeObject::SpriteList2D) {
		obj = new DxScriptSpriteListObject2D();
	}
	else if (type == TypeObject::Primitive3D) {
		obj = new DxScriptPrimitiveObject3D();
	}
	else if (type == TypeObject::Sprite3D) {
		obj = new DxScriptSpriteObject3D();
	}
	else if (type == TypeObject::Trajectory3D) {
		obj = new DxScriptTrajectoryObject3D();
	}

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateIntValue(id);
}
value DxScript::Func_ObjPrimitive_SetPrimitiveType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj)
		obj->SetPrimitiveType((D3DPRIMITIVETYPE)argv[1].as_int());
	return value();
}
value DxScript::Func_ObjPrimitive_GetPrimitiveType(script_machine* machine, int argc, const value* argv) {
	int res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj)
		res = obj->GetPrimitiveType();
	return script->CreateIntValue(res);
}
value DxScript::Func_ObjPrimitive_SetVertexCount(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj)
		obj->SetVertexCount(argv[1].as_int());
	return value();
}
value DxScript::Func_ObjPrimitive_SetTexture(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		auto& mapTexture = script->pResouceCache_->mapTexture;

		auto itr = mapTexture.find(path);
		if (itr != mapTexture.end()) {
			obj->SetTexture(itr->second);
		}
		else {
			shared_ptr<Texture> texture(new Texture());
			texture->CreateFromFile(path, false, false);
			obj->SetTexture(texture);
		}
	}
	return value();
}
value DxScript::Func_ObjPrimitive_GetTexture(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	std::wstring name = L"";

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		if (shared_ptr<Texture> pTexture = obj->GetTexture())
			name = pTexture->GetName();
	}

	return script->CreateStringValue(name);
}
value DxScript::Func_ObjPrimitive_GetVertexCount(script_machine* machine, int argc, const value* argv) {
	size_t res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj)
		res = obj->GetVertexCount();
	return script->CreateIntValue(res);
}
value DxScript::Func_ObjPrimitive_SetVertexPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int index = argv[1].as_int();
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj)
		obj->SetVertexPosition(index, argv[2].as_real(), argv[3].as_real(), (argc == 5) ? argv[4].as_real() : 0);
		
	return value();
}
value DxScript::Func_ObjPrimitive_SetVertexUV(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj)
		obj->SetVertexUV(argv[1].as_int(), argv[2].as_real(), argv[3].as_real());
	return value();
}
gstd::value DxScript::Func_ObjPrimitive_SetVertexUVT(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		shared_ptr<Texture> texture = obj->GetTexture();
		if (texture) {
			float width = texture->GetWidth();
			float height = texture->GetHeight();
			obj->SetVertexUV(argv[1].as_int(), (float)argv[2].as_real() / width, (float)argv[3].as_real() / height);
		}
	}
	return value();
}
value DxScript::Func_ObjPrimitive_SetVertexColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		if (argc == 5) {
			obj->SetVertexColor(argv[1].as_int(), argv[2].as_int(), argv[3].as_int(), argv[4].as_int());
		}
		else {
			D3DCOLOR color = argv[2].as_int();
			obj->SetVertexColor(argv[1].as_int(), ColorAccess::GetColorR(color), ColorAccess::GetColorG(color),
				ColorAccess::GetColorB(color));
		}
	}
	return value();
}
value DxScript::Func_ObjPrimitive_SetVertexColorHSV(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		D3DCOLOR cRGB = 0xffffffff;
		ColorAccess::HSVtoRGB(cRGB, argv[2].as_int(), argv[3].as_int(), argv[4].as_int());

		obj->SetVertexColor(argv[1].as_int(), ColorAccess::GetColorR(cRGB),
			ColorAccess::GetColorG(cRGB), ColorAccess::GetColorB(cRGB));
	}
	return value();
}
value DxScript::Func_ObjPrimitive_SetVertexAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj)
		obj->SetVertexAlpha(argv[1].as_int(), argv[2].as_int());
	return value();
}
value DxScript::Func_ObjPrimitive_GetVertexColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int index = argv[1].as_int();

	D3DCOLOR color = 0xffffffff;
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) color = obj->GetVertexColor(index);

	byte listColor[4];
	ColorAccess::ToByteArray(color, listColor);
	return script->CreateIntArrayValue(listColor + 1, 3U);

}
value DxScript::Func_ObjPrimitive_GetVertexColorHex(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int index = argv[1].as_int();

	D3DCOLOR color = 0xffffffff;
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) color = obj->GetVertexColor(index);

	return script->CreateIntValue(color - 0xff000000); // Ditto @ GetColor
}
value DxScript::Func_ObjPrimitive_GetVertexAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int index = argv[1].as_int();

	D3DCOLOR color = 0xffffffff;
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) color = obj->GetVertexColor(index);

	return script->CreateRealValue(ColorAccess::GetColorA(color));
}
value DxScript::Func_ObjPrimitive_GetVertexPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int index = argv[1].as_int();

	//D3DXVECTOR3 pos = D3DXVECTOR3(DxScript::g_posInvalidX_, DxScript::g_posInvalidY_, DxScript::g_posInvalidZ_);
	D3DXVECTOR3 pos = D3DXVECTOR3(0, 0, 0);
	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj)
		pos = obj->GetVertexPosition(index);

	return script->CreateRealArrayValue(reinterpret_cast<float*>(&pos), 3U);
}
value DxScript::Func_ObjPrimitive_SetVertexIndex(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		const value& valArr = argv[1];
		std::vector<uint16_t> vecIndex;
		vecIndex.resize(valArr.length_as_array());
		for (size_t i = 0; i < valArr.length_as_array(); ++i) {
			vecIndex[i] = (uint16_t)valArr.index_as_array(i).as_int();
		}
		obj->GetObjectPointer()->SetVertexIndices(vecIndex);
	}
	return value();
}

//Dx関数：オブジェクト操作(Sprite2D)
value DxScript::Func_ObjSprite2D_SetSourceRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteObject2D* obj = script->GetObjectPointerAs<DxScriptSpriteObject2D>(id);
	if (obj) {
		DxRect<int> rcSrc(argv[1].as_int(), argv[2].as_int(),
			argv[3].as_int(), argv[4].as_int());
		obj->GetSpritePointer()->SetSourceRect(rcSrc);
	}
	return value();
}
value DxScript::Func_ObjSprite2D_SetDestRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteObject2D* obj = script->GetObjectPointerAs<DxScriptSpriteObject2D>(id);
	if (obj) {
		DxRect<double> rcDest(argv[1].as_real(), argv[2].as_real(),
			argv[3].as_real(), argv[4].as_real());
		obj->GetSpritePointer()->SetDestinationRect(rcDest);
	}
	return value();
}
value DxScript::Func_ObjSprite2D_SetDestCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteObject2D* obj = script->GetObjectPointerAs<DxScriptSpriteObject2D>(id);
	if (obj)
		obj->GetSpritePointer()->SetDestinationCenter();
	return value();
}

//Dx関数：オブジェクト操作(SpriteList2D)
gstd::value DxScript::Func_ObjSpriteList2D_SetSourceRect(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteListObject2D* obj = script->GetObjectPointerAs<DxScriptSpriteListObject2D>(id);
	if (obj) {
		DxRect<int> rcSrc(argv[1].as_int(), argv[2].as_int(),
			argv[3].as_int(), argv[4].as_int());
		obj->GetSpritePointer()->SetSourceRect(rcSrc);
	}
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_SetDestRect(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteListObject2D* obj = script->GetObjectPointerAs<DxScriptSpriteListObject2D>(id);
	if (obj) {
		DxRect<double> rcDest(argv[1].as_real(), argv[2].as_real(),
			argv[3].as_real(), argv[4].as_real());
		obj->GetSpritePointer()->SetDestinationRect(rcDest);
	}
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_SetDestCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteListObject2D* obj = script->GetObjectPointerAs<DxScriptSpriteListObject2D>(id);
	if (obj)
		obj->GetSpritePointer()->SetDestinationCenter();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_AddVertex(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteListObject2D* obj = script->GetObjectPointerAs<DxScriptSpriteListObject2D>(id);
	if (obj)
		obj->AddVertex();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_CloseVertex(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteListObject2D* obj = script->GetObjectPointerAs<DxScriptSpriteListObject2D>(id);
	if (obj)
		obj->CloseVertex();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_ClearVertexCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteListObject2D* obj = script->GetObjectPointerAs<DxScriptSpriteListObject2D>(id);
	if (obj)
		obj->GetSpritePointer()->ClearVertexCount();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_SetAutoClearVertexCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteListObject2D* obj = script->GetObjectPointerAs<DxScriptSpriteListObject2D>(id);
	if (obj)
		obj->GetSpritePointer()->SetAutoClearVertex(argv[1].as_boolean());
	return value();
}

//Dx関数：オブジェクト操作(Sprite3D)
value DxScript::Func_ObjSprite3D_SetSourceRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteObject3D* obj = script->GetObjectPointerAs<DxScriptSpriteObject3D>(id);
	if (obj) {
		DxRect<int> rcSrc(argv[1].as_int(), argv[2].as_int(),
			argv[3].as_int(), argv[4].as_int());
		obj->GetSpritePointer()->SetSourceRect(rcSrc);
	}
	return value();
}
value DxScript::Func_ObjSprite3D_SetDestRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteObject3D* obj = script->GetObjectPointerAs<DxScriptSpriteObject3D>(id);
	if (obj) {
		DxRect<double> rcDest(argv[1].as_real(), argv[2].as_real(),
			argv[3].as_real(), argv[4].as_real());
		obj->GetSpritePointer()->SetDestinationRect(rcDest);
	}
	return value();
}
value DxScript::Func_ObjSprite3D_SetSourceDestRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteObject3D* obj = script->GetObjectPointerAs<DxScriptSpriteObject3D>(id);
	if (obj) {
		DxRect<double> rect(argv[1].as_real(), argv[2].as_real(),
			argv[3].as_real(), argv[4].as_real());
		obj->GetSpritePointer()->SetSourceDestRect(rect);
	}
	return value();
}
value DxScript::Func_ObjSprite3D_SetBillboard(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptSpriteObject3D* obj = script->GetObjectPointerAs<DxScriptSpriteObject3D>(id);
	if (obj) {
		bool bEnable = argv[1].as_boolean();
		obj->GetSpritePointer()->SetBillboardEnable(bEnable);
	}
	return value();
}
//Dx関数：オブジェクト操作(TrajectoryObject3D)
value DxScript::Func_ObjTrajectory3D_SetInitialPoint(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTrajectoryObject3D* obj = script->GetObjectPointerAs<DxScriptTrajectoryObject3D>(id);
	if (obj) {
		D3DXVECTOR3 pos1(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
		D3DXVECTOR3 pos2(argv[4].as_real(), argv[5].as_real(), argv[6].as_real());
		obj->GetObjectPointer()->SetInitialLine(pos1, pos2);
	}
	return value();
}
value DxScript::Func_ObjTrajectory3D_SetAlphaVariation(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTrajectoryObject3D* obj = script->GetObjectPointerAs<DxScriptTrajectoryObject3D>(id);
	if (obj)
		obj->GetObjectPointer()->SetAlphaVariation(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjTrajectory3D_SetComplementCount(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTrajectoryObject3D* obj = script->GetObjectPointerAs<DxScriptTrajectoryObject3D>(id);
	if (obj)
		obj->GetObjectPointer()->SetComplementCount(argv[1].as_int());
	return value();
}

//DxScriptParticleListObject
value DxScript::Func_ObjParticleList_Create(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	script->CheckRunInMainThread();
	TypeObject type = (TypeObject)argv[0].as_int();

	ref_unsync_ptr<DxScriptRenderObject> obj;
	if (type == TypeObject::ParticleList2D) {
		obj = new DxScriptParticleListObject2D();
	}
	else if (type == TypeObject::ParticleList3D) {
		obj = new DxScriptParticleListObject3D();
	}

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateIntValue(id);
}
value DxScript::Func_ObjParticleList_SetPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle)
			objParticle->SetInstancePosition(argv[1].as_real(), argv[2].as_real(), (argc == 4) ? argv[3].as_real() : 0);
	}
	return value();
}
template<size_t ID>
value DxScript::Func_ObjParticleList_SetScaleSingle(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle)
			objParticle->SetInstanceScaleSingle(ID, argv[1].as_real());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetScaleXYZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) {
			if (argc == 4)
				objParticle->SetInstanceScale(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
			else {
				float scale = argv[1].as_real();
				objParticle->SetInstanceScale(scale, scale, scale);
			}
		}
			
	}
	return value();
}
template<size_t ID>
value DxScript::Func_ObjParticleList_SetAngleSingle(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle)
			objParticle->SetInstanceAngleSingle(ID, argv[1].as_real());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetAngle(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle)
			objParticle->SetInstanceAngle(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) {
			if (argc == 4) {
				objParticle->SetInstanceColorRGB(argv[1].as_int(), argv[2].as_int(), argv[3].as_int());
			}
			else {
				D3DCOLOR color = argv[1].as_int();
				objParticle->SetInstanceColorRGB(ColorAccess::GetColorR(color), 
					ColorAccess::GetColorG(color), ColorAccess::GetColorB(color));
			}
		}
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle)
			objParticle->SetInstanceAlpha(argv[1].as_int());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetExtraData(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle)
			objParticle->SetInstanceUserData(D3DXVECTOR3(argv[1].as_real(), argv[2].as_real(), argv[3].as_real()));
	}
	return value();
}
value DxScript::Func_ObjParticleList_AddInstance(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle)
			objParticle->AddInstance();
	}
	return value();
}
value DxScript::Func_ObjParticleList_ClearInstance(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();

	DxScriptPrimitiveObject* obj = script->GetObjectPointerAs<DxScriptPrimitiveObject>(id);
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle)
			objParticle->ClearInstance();
	}
	return value();
}

//Dx関数：オブジェクト操作(DxMesh)
value DxScript::Func_ObjMesh_Create(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	ref_unsync_ptr<DxScriptMeshObject> obj = new DxScriptMeshObject();

	int id = ID_INVALID;
	if (obj) {
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateIntValue(id);
}
value DxScript::Func_ObjMesh_Load(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	bool res = false;

	DxScriptMeshObject* obj = script->GetObjectPointerAs<DxScriptMeshObject>(id);
	if (obj) {
		shared_ptr<DxMesh> mesh;
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		auto& mapMesh = script->pResouceCache_->mapMesh;

		auto itr = mapMesh.find(path);
		if (itr != mapMesh.end()) {
			mesh = itr->second;
			res = true;
		}
		else {
			std::wstring ext = PathProperty::GetFileExtension(path);
			if (ext == L".mqo") {
				mesh = std::make_shared<MetasequoiaMesh>();
				res = mesh->CreateFromFile(path);
			}
			/*
			else if (ext == L".elem") {
				mesh = std::make_shared<ElfreinaMesh>();
				res = mesh->CreateFromFile(path);
			}
			*/
		}

		if (res) {
			mesh->SetDxObjectReference(obj);
			obj->mesh_ = mesh;
		}
	}
	return script->CreateBooleanValue(res);
}
value DxScript::Func_ObjMesh_SetColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptMeshObject* obj = script->GetObjectPointerAs<DxScriptMeshObject>(id);
	if (obj) {
		if (argc == 4) {
			obj->SetColor(argv[1].as_int(), argv[2].as_int(), argv[3].as_int());
		}
		else {
			D3DCOLOR color = argv[1].as_int();
			obj->SetColor(ColorAccess::GetColorR(color), ColorAccess::GetColorG(color), 
				ColorAccess::GetColorB(color));
		}
	}
	return value();
}
value DxScript::Func_ObjMesh_SetAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptMeshObject* obj = script->GetObjectPointerAs<DxScriptMeshObject>(id);
	if (obj)
		obj->SetAlpha(argv[1].as_int());
	return value();
}
value DxScript::Func_ObjMesh_SetAnimation(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptMeshObject* obj = script->GetObjectPointerAs<DxScriptMeshObject>(id);
	if (obj) {
		std::wstring anime = argv[1].as_string();
		obj->anime_ = anime;
		obj->time_ = argv[2].as_int();

		//	D3DXMATRIX mat = obj->mesh_->GetAnimationMatrix(anime, obj->time_, "悠久前部");
		//	D3DXVECTOR3 pos;
		//	D3DXVec3TransformCoord(&pos, &D3DXVECTOR3(0,0,0), &mat);
	}
	return value();
}
gstd::value DxScript::Func_ObjMesh_SetCoordinate2D(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptMeshObject* obj = script->GetObjectPointerAs<DxScriptMeshObject>(id);
	if (obj) {
		bool bEnable = argv[1].as_boolean();
		obj->bCoordinate2D_ = bEnable;
	}
	return value();
}
value DxScript::Func_ObjMesh_GetPath(script_machine* machine, int argc, const value* argv) {
	std::wstring res;
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptMeshObject* obj = script->GetObjectPointerAs<DxScriptMeshObject>(id);
	if (obj) {
		DxMesh* mesh = obj->mesh_.get();
		if (mesh) res = mesh->GetPath();
	}
	return script->CreateStringValue(res);
}

//Dx関数：オブジェクト操作(DxText)
value DxScript::Func_ObjText_Create(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	ref_unsync_ptr<DxScriptTextObject> obj = new DxScriptTextObject();

	int id = ID_INVALID;
	if (obj) {
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateIntValue(id);
}
value DxScript::Func_ObjText_SetText(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		std::wstring wstr = argv[1].as_string();
		obj->SetText(wstr);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		std::wstring wstr = argv[1].as_string();
		obj->SetFontType(wstr);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontSize(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		LONG size = argv[1].as_int();
		obj->SetFontSize(size);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBold(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		bool bBold = argv[1].as_boolean();
		obj->SetFontWeight(bBold ? FW_BOLD : FW_NORMAL);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontWeight(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		LONG weight = argv[1].as_int();
		if (weight < 0) weight = 0;
		else if (weight > 1000) weight = 1000;
		obj->SetFontWeight(weight);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		byte r, g, b;
		if (argc == 4) {
			r = ColorAccess::ClampColorRet(argv[1].as_int());
			g = ColorAccess::ClampColorRet(argv[2].as_int());
			b = ColorAccess::ClampColorRet(argv[3].as_int());
		}
		else {
			D3DCOLOR color = argv[1].as_int();
			r = ColorAccess::GetColorR(color);
			g = ColorAccess::GetColorG(color);
			b = ColorAccess::GetColorB(color);
		}
		obj->SetFontColorTop(r, g, b);
		obj->SetFontColorBottom(r, g, b);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontColorTop(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		if (argc == 4) {
			byte r = ColorAccess::ClampColorRet(argv[1].as_int());
			byte g = ColorAccess::ClampColorRet(argv[2].as_int());
			byte b = ColorAccess::ClampColorRet(argv[3].as_int());
			obj->SetFontColorTop(r, g, b);
		}
		else {
			D3DCOLOR color = argv[1].as_int();
			obj->SetFontColorTop(ColorAccess::GetColorR(color), 
				ColorAccess::GetColorG(color), ColorAccess::GetColorB(color));
		}
	}
	return value();
}
value DxScript::Func_ObjText_SetFontColorBottom(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		if (argc == 4) {
			byte r = ColorAccess::ClampColorRet(argv[1].as_int());
			byte g = ColorAccess::ClampColorRet(argv[2].as_int());
			byte b = ColorAccess::ClampColorRet(argv[3].as_int());
			obj->SetFontColorBottom(r, g, b);
		}
		else {
			D3DCOLOR color = argv[1].as_int();
			obj->SetFontColorBottom(ColorAccess::GetColorR(color),
				ColorAccess::GetColorG(color), ColorAccess::GetColorB(color));
		}
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBorderWidth(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		LONG width = argv[1].as_int();
		obj->SetFontBorderWidth(width);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBorderType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		TextBorderType type = (TextBorderType)argv[1].as_int();
		obj->SetFontBorderType(type);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBorderColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		if (argc == 4) {
			byte r = ColorAccess::ClampColorRet(argv[1].as_int());
			byte g = ColorAccess::ClampColorRet(argv[2].as_int());
			byte b = ColorAccess::ClampColorRet(argv[3].as_int());
			obj->SetFontBorderColor(r, g, b);
		}
		else {
			D3DCOLOR color = argv[1].as_int();
			obj->SetFontBorderColor(ColorAccess::GetColorR(color),
				ColorAccess::GetColorG(color), ColorAccess::GetColorB(color));
		}
	}
	return value();
}
value DxScript::Func_ObjText_SetFontCharacterSet(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		BYTE cSet = ColorAccess::ClampColorRet(argv[1].as_int());	//Also clamps to [0-255], so it works too
		obj->SetCharset((BYTE)cSet);
	}
	return value();
}
value DxScript::Func_ObjText_SetMaxWidth(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		LONG width = argv[1].as_int();
		obj->SetMaxWidth(width);
	}
	return value();
}
value DxScript::Func_ObjText_SetMaxHeight(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		LONG height = argv[1].as_int();
		obj->SetMaxHeight(height);
	}
	return value();
}
value DxScript::Func_ObjText_SetLinePitch(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		float pitch = argv[1].as_real();
		obj->SetLinePitch(pitch);
	}
	return value();
}
value DxScript::Func_ObjText_SetSidePitch(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		float pitch = argv[1].as_real();
		obj->SetSidePitch(pitch);
	}
	return value();
}
value DxScript::Func_ObjText_SetVertexColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		if (argc == 5) {
			D3DCOLOR color = ColorAccess::ToD3DCOLOR(Vectorize::SetI(argv[1].as_int(), 
				argv[2].as_int(), argv[3].as_int(), argv[4].as_int()));
			obj->SetVertexColor(color);
		}
		else {
			obj->SetVertexColor((D3DCOLOR)argv[1].as_int());
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjText_SetTransCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		float centerX = argv[1].as_real();
		float centerY = argv[2].as_real();
		obj->center_ = D3DXVECTOR2(centerX, centerY);
	}
	return value();
}
gstd::value DxScript::Func_ObjText_SetAutoTransCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj)
		obj->bAutoCenter_ = argv[1].as_boolean();
	return value();
}
gstd::value DxScript::Func_ObjText_SetHorizontalAlignment(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		TextAlignment align = (TextAlignment)argv[1].as_int();
		obj->SetHorizontalAlignment(align);
	}
	return value();
}
gstd::value DxScript::Func_ObjText_SetSyntacticAnalysis(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj)
		obj->SetSyntacticAnalysis(argv[1].as_boolean());
	return value();
}
value DxScript::Func_ObjText_GetTextLength(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	size_t res = 0;
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		const std::wstring& text = obj->GetText();
		res = StringUtility::CountAsciiSizeCharacter(text);
	}
	return script->CreateIntValue(res);
}
value DxScript::Func_ObjText_GetTextLengthCU(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	size_t res = 0;
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		std::vector<size_t> listCount = obj->GetTextCountCU();
		for (size_t iLine = 0; iLine < listCount.size(); ++iLine) {
			res += listCount[iLine];
		}
	}
	return script->CreateIntValue(res);
}
value DxScript::Func_ObjText_GetTextLengthCUL(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	std::vector<size_t> listCount;
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj) {
		listCount = obj->GetTextCountCU();
	}
	return script->CreateIntArrayValue(listCount);
}
value DxScript::Func_ObjText_GetTotalWidth(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	LONG res = 0;
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj)
		res = obj->GetTotalWidth();
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjText_GetTotalHeight(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	LONG res = 0;
	DxScriptTextObject* obj = script->GetObjectPointerAs<DxScriptTextObject>(id);
	if (obj)
		res = obj->GetTotalHeight();
	return script->CreateRealValue(res);
}

//Dx関数：音声操作(DxSoundObject)
gstd::value DxScript::Func_ObjSound_Create(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	ref_unsync_ptr<DxSoundObject> obj = new DxSoundObject();
	obj->manager_ = script->objManager_.get();

	int id = script->AddObject(obj);
	return script->CreateIntValue(id);
}
gstd::value DxScript::Func_ObjSound_Load(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	bool bLoad = false;
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);
		bLoad = obj->Load(path);
	}
	return script->CreateBooleanValue(bLoad);
}
gstd::value DxScript::Func_ObjSound_Play(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			//obj->Play();
			script->GetObjectManager()->ReserveSound(player);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_Stop(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			player->Stop();
			script->GetObjectManager()->DeleteReservedSound(player);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetVolumeRate(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			player->SetVolumeRate(argv[1].as_real());
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetPanRate(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			player->SetPanRate(argv[1].as_real());
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetFade(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			player->SetFade(argv[1].as_real());
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetLoopEnable(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			SoundPlayer::PlayStyle* pStyle = player->GetPlayStyle();
			pStyle->bLoop_ = argv[1].as_boolean();
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetLoopTime(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			SoundPlayer::PlayStyle* pStyle = player->GetPlayStyle();
			pStyle->timeLoopStart_ = argv[1].as_real();
			pStyle->timeLoopEnd_ = argv[2].as_real();
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetLoopSampleCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			if (auto soundSource = player->GetSoundSource()) {
				WAVEFORMATEX* fmt = &soundSource->formatWave_;

				double startTime = argv[1].as_real() / (double)fmt->nSamplesPerSec;
				double endTime = argv[2].as_real() / (double)fmt->nSamplesPerSec;;

				SoundPlayer::PlayStyle* pStyle = player->GetPlayStyle();
				pStyle->timeLoopStart_ = startTime;
				pStyle->timeLoopEnd_ = endTime;
			}
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_Seek(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			double time = argv[1].as_real();
			if (player->IsPlaying()) {
				player->Seek(time);
				player->ResetStreamForSeek();
			}
			else {
				SoundPlayer::PlayStyle* pStyle = player->GetPlayStyle();
				pStyle->timeStart_ = time;
			}
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SeekSampleCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			if (auto soundSource = player->GetSoundSource()) {
				WAVEFORMATEX* fmt = &soundSource->formatWave_;

				DWORD samp = argv[1].as_int();
				if (player->IsPlaying()) {
					player->Seek(samp);
					player->ResetStreamForSeek();
				}
				else {
					SoundPlayer::PlayStyle* pStyle = player->GetPlayStyle();
					pStyle->timeStart_ = samp / (double)fmt->nSamplesPerSec;
				}
			}
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetResumeEnable(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			SoundPlayer::PlayStyle* pStyle = player->GetPlayStyle();
			pStyle->bResume_ = argv[1].as_boolean();
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetSoundDivision(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			player->SetSoundDivision(argv[1].as_int());
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_IsPlaying(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	bool bPlay = false;
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			bPlay = player->IsPlaying();
			if (!bPlay) {
				//If ObjSound_IsPlaying was called in the same frame as ObjSound_Play,
				//	player->IsPlaying() would return false
				auto reservedPlayer = script->GetObjectManager()->GetReservedSound(player);
				bPlay = reservedPlayer != nullptr;
			}
		}
	}
	return script->CreateBooleanValue(bPlay);
}
gstd::value DxScript::Func_ObjSound_GetVolumeRate(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	double rate = 0;
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			rate = player->GetVolumeRate();
	}
	return script->CreateRealValue(rate);
}
/*
gstd::value DxScript::Func_ObjSound_DebugGetCopyPos(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (auto streaming = dynamic_cast<SoundStreamingPlayer*>(player.get())) {
			size_t* ptr = streaming->DbgGetStreamCopyPos();
			size_t bps = player->GetWaveFormat()->nBlockAlign;
			std::vector<size_t> listSize = { ptr[0] / bps, ptr[1] / bps };
			return script->CreateRealArrayValue(listSize);
		}
	}
	return script->CreateRealArrayValue((int*)nullptr, 0U);
}
*/
gstd::value DxScript::Func_ObjSound_SetFrequency(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			player->SetFrequency(argv[1].as_int());
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_GetInfo(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	int type = argv[1].as_int();

	value res;

	int id = argv[0].as_int();
	DxSoundObject* obj = script->GetObjectPointerAs<DxSoundObject>(id);
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			if (auto soundSource = player->GetSoundSource()) {
				WAVEFORMATEX* fmt = &soundSource->formatWave_;
				switch (type) {
				case SoundPlayer::INFO_FORMAT:
					return script->CreateIntValue((int)soundSource->format_);
				case SoundPlayer::INFO_CHANNEL:
					return script->CreateIntValue(fmt->nChannels);
				case SoundPlayer::INFO_SAMPLE_RATE:
					return script->CreateIntValue(fmt->nSamplesPerSec);
				case SoundPlayer::INFO_AVG_BYTE_PER_SEC:
					return script->CreateIntValue(fmt->nAvgBytesPerSec);
				case SoundPlayer::INFO_BLOCK_ALIGN:
					return script->CreateIntValue(fmt->nBlockAlign);
				case SoundPlayer::INFO_BIT_PER_SAMPLE:
					return script->CreateIntValue(fmt->wBitsPerSample);
				case SoundPlayer::INFO_POSITION:
					return script->CreateIntValue(player->GetCurrentPosition() / (double)fmt->nAvgBytesPerSec);
				case SoundPlayer::INFO_POSITION_SAMPLE:
					return script->CreateIntValue(player->GetCurrentPosition() / fmt->nBlockAlign);
				case SoundPlayer::INFO_LENGTH:
					return script->CreateIntValue(soundSource->audioSizeTotal_ / (double)fmt->nAvgBytesPerSec);
				case SoundPlayer::INFO_LENGTH_SAMPLE:
					return script->CreateIntValue(soundSource->audioSizeTotal_ / fmt->nBlockAlign);
				}
			}
			return value();
		}
	}

	switch (type) {
	case SoundPlayer::INFO_FORMAT:
		return script->CreateIntValue((int)SoundFileFormat::Unknown);
	case SoundPlayer::INFO_CHANNEL:
	case SoundPlayer::INFO_SAMPLE_RATE:
	case SoundPlayer::INFO_AVG_BYTE_PER_SEC:
	case SoundPlayer::INFO_BLOCK_ALIGN:
	case SoundPlayer::INFO_BIT_PER_SAMPLE:
	case SoundPlayer::INFO_POSITION:
	case SoundPlayer::INFO_POSITION_SAMPLE:
	case SoundPlayer::INFO_LENGTH:
	case SoundPlayer::INFO_LENGTH_SAMPLE:
		return script->CreateIntValue(0);
	}

	return value();
}

//Dx関数：ファイル操作(DxFileObject)
gstd::value DxScript::Func_ObjFile_Create(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	//	script->CheckRunInMainThread();
	TypeObject type = (TypeObject)argv[0].as_int();

	ref_unsync_ptr<DxFileObject> obj;
	if (type == TypeObject::FileText) {
		obj = new DxTextFileObject();
	}
	else if (type == TypeObject::FileBinary) {
		obj = new DxBinaryFileObject();
	}

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateIntValue(id);
}
gstd::value DxScript::Func_ObjFile_Open(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	bool res = false;
	DxFileObject* obj = script->GetObjectPointerAs<DxFileObject>(id);
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader && reader->Open()) {
			//obj->bWritable_ = !reader->IsArchived();
			obj->bWritable_ = false;
			res = obj->OpenR(reader);
		}
	}
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_ObjFile_OpenNW(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	bool res = false;
	DxFileObject* obj = script->GetObjectPointerAs<DxFileObject>(id);
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader && reader->Open()) {
			//Cannot write to an archived file, fall back to read-only permission
			if (reader->IsArchived()) {
				res = obj->OpenR(reader);
				obj->bWritable_ = false;

				goto lab_open_done;
			}
		}

		res = obj->OpenRW(path);
		obj->bWritable_ = res;
	}
lab_open_done:
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_ObjFile_Store(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	bool res = false;
	DxFileObject* obj = script->GetObjectPointerAs<DxFileObject>(id);
	if (obj)
		res = obj->Store();
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_ObjFile_GetSize(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	int res = 0;
	DxFileObject* obj = script->GetObjectPointerAs<DxFileObject>(id);
	if (obj) {
		shared_ptr<File>& file = obj->GetFile();
		res = file != nullptr ? file->GetSize() : 0;
	}
	return script->CreateIntValue(res);
}

//Dx関数：ファイル操作(DxTextFileObject)
gstd::value DxScript::Func_ObjFileT_GetLineCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	size_t res = 0;
	DxTextFileObject* obj = script->GetObjectPointerAs<DxTextFileObject>(id);
	if (obj)
		res = obj->GetLineCount();
	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileT_GetLineText(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxTextFileObject* obj = script->GetObjectPointerAs<DxTextFileObject>(id);
	if (obj == nullptr) return script->CreateStringValue(std::wstring());

	int line = argv[1].as_int();
	std::wstring res = obj->GetLineAsWString(line);
	return script->CreateStringValue(res);
}
gstd::value DxScript::Func_ObjFileT_SetLineText(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxTextFileObject* obj = script->GetObjectPointerAs<DxTextFileObject>(id);

	if (obj ) {
		int line = argv[1].as_int();
		std::wstring text = argv[2].as_string();
		obj->SetLineAsWString(text, line);
	}

	return value();
}
gstd::value DxScript::Func_ObjFileT_SplitLineText(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxTextFileObject* obj = script->GetObjectPointerAs<DxTextFileObject>(id);
	if (obj == nullptr) return script->CreateStringArrayValue(std::vector<std::wstring>());

	int pos = argv[1].as_int();
	std::wstring delim = argv[2].as_string();
	std::wstring line = obj->GetLineAsWString(pos);
	std::vector<std::wstring> list = StringUtility::Split(line, delim);

	return script->CreateStringArrayValue(list);
}
gstd::value DxScript::Func_ObjFileT_AddLine(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxTextFileObject* obj = script->GetObjectPointerAs<DxTextFileObject>(id);

	if (obj)
		obj->AddLine(argv[1].as_string());

	return value();
}
gstd::value DxScript::Func_ObjFileT_ClearLine(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = argv[0].as_int();
	DxTextFileObject* obj = script->GetObjectPointerAs<DxTextFileObject>(id);

	//Cannot write to an archived file
	if (obj)
		obj->ClearLine();

	return value();
}

//Dx関数：ファイル操作(DxBinaryFileObject)
gstd::value DxScript::Func_ObjFileB_SetByteOrder(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	
	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		int order = argv[1].as_int();
		obj->SetByteOrder(order);
	}

	return gstd::value();
}
gstd::value DxScript::Func_ObjFileB_SetCharacterCode(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	
	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		byte code = (byte)argv[1].as_int();
		obj->SetCodePage(code);
	}

	return gstd::value();
}
gstd::value DxScript::Func_ObjFileB_GetPointer(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	
	size_t res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		ByteBuffer* buffer = obj->GetBuffer();
		res = buffer ? buffer->GetOffset() : 0;
	}

	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileB_Seek(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	
	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		int pos = argv[1].as_int();
		ByteBuffer* buffer = obj->GetBuffer();
		if (buffer)
			buffer->Seek(pos);
	}

	return gstd::value();
}
gstd::value DxScript::Func_ObjFileB_GetLastRead(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	size_t res = obj ? obj->GetLastReadSize() : 0;

	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadBoolean(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	bool res = false;
	
	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) 
		obj->Read(&res, sizeof(res));

	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadByte(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	int8_t res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj)
		obj->Read(&res, sizeof(res));

	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadShort(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	int16_t res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		obj->Read(&res, sizeof(res));
		if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG)
			ByteOrder::Reverse(&res, sizeof(res));
	}

	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadInteger(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	int32_t res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		obj->Read(&res, sizeof(res));
		if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG)
			ByteOrder::Reverse(&res, sizeof(res));
	}

	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadLong(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	int64_t res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		obj->Read(&res, sizeof(res));
		if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG)
			ByteOrder::Reverse(&res, sizeof(res));
	}

	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadFloat(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	float res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		obj->Read(&res, sizeof(res));
		if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG)
			ByteOrder::Reverse(&res, sizeof(res));
	}

	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadDouble(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	double res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		obj->Read(&res, sizeof(res));
		if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG)
			ByteOrder::Reverse(&res, sizeof(res));
	}

	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadString(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	std::wstring res;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		size_t readSize = (size_t)argv[1].as_real();

		std::vector<byte> data(readSize);
		obj->Read(&data[0], readSize);
		readSize = obj->GetLastReadSize();

		int cp = obj->GetCodePage();
		switch (cp) {
		case CODE_UTF16LE:
		case CODE_UTF16BE:
		{
			size_t strSize = readSize / 2 * 2;
			size_t wchCount = strSize / 2;

			res.resize(wchCount);
			memcpy(&res[0], &data[0], strSize);

			if (cp == CODE_UTF16BE)
				ByteOrder::Reverse(&res[0], strSize);
		}
		case CODE_ACP:
		case CODE_UTF8:
		default:
		{
			std::string str;
			str.resize(readSize);
			memcpy(&str[0], &data[0], readSize);

			res = StringUtility::ConvertMultiToWide(str, cp == CODE_UTF8 ? CODE_UTF8 : CODE_ACP);
			//Don't optimize that ternary, it's intentional so that any unknown formats get the ANSI treatment
		}
		}
	}

	return script->CreateStringValue(res);
}

gstd::value DxScript::Func_ObjFileB_WriteBoolean(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	DWORD res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		bool data = argv[1].as_boolean();
		res = obj->Write(&data, sizeof(bool));
	}

	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteByte(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	DWORD res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		byte data = (byte)argv[1].as_int();
		res = obj->Write(&data, sizeof(byte));
	}

	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteShort(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	DWORD res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		int16_t data = (int16_t)argv[1].as_int();
		res = obj->Write(&data, sizeof(int16_t));
	}

	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteInteger(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	DWORD res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		int32_t data = (int32_t)argv[1].as_int();
		res = obj->Write(&data, sizeof(int32_t));
	}

	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteLong(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	DWORD res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		int64_t data = (int64_t)argv[1].as_int();
		res = obj->Write(&data, sizeof(int64_t));
	}

	return script->CreateIntValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteFloat(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	DWORD res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		float data = argv[1].as_real();
		res = obj->Write(&data, sizeof(float));
	}

	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteDouble(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	DWORD res = 0;

	DxBinaryFileObject* obj = script->GetObjectPointerAs<DxBinaryFileObject>(argv[0].as_int());
	if (obj) {
		double data = argv[1].as_real();
		res = obj->Write(&data, sizeof(double));
	}

	return script->CreateRealValue(res);
}

#include "source/GcLib/pch.h"

#include "DxScript.hpp"
#include "DxUtility.hpp"
#include "DirectGraphics.hpp"
#include "DirectInput.hpp"
#include "MetasequoiaMesh.hpp"
#include "ElfreinaMesh.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//DxScript
**********************************************************/
/**********************************************************
//DxScript
**********************************************************/
function const dxFunction[] =
{
	//Dx関数：システム系系
	{ "InstallFont", DxScript::Func_InstallFont, 1 },

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

	//Dx関数：音声系
	{ "LoadSound", DxScript::Func_LoadSound, 1 },
	{ "RemoveSound", DxScript::Func_RemoveSound, 1 },
	{ "PlayBGM", DxScript::Func_PlayBGM, 3 },
	{ "PlaySE", DxScript::Func_PlaySE, 1 },
	{ "StopSound", DxScript::Func_StopSound, 1 },
	{ "SetSoundDivisionVolumeRate", DxScript::Func_SetSoundDivisionVolumeRate, 2 },
	{ "GetSoundDivisionVolumeRate", DxScript::Func_GetSoundDivisionVolumeRate, 1 },

	//Dx関数：キー系
	{ "GetKeyState", DxScript::Func_GetKeyState, 1 },
	{ "GetMouseX", DxScript::Func_GetMouseX, 0 },
	{ "GetMouseY", DxScript::Func_GetMouseY, 0 },
	{ "GetMouseMoveZ", DxScript::Func_GetMouseMoveZ, 0 },
	{ "GetMouseState", DxScript::Func_GetMouseState, 1 },
	{ "GetVirtualKeyState", DxScript::Func_GetVirtualKeyState, 1 },
	{ "SetVirtualKeyState", DxScript::Func_SetVirtualKeyState, 2 },

	//Dx関数：描画系
	{ "GetScreenWidth", DxScript::Func_GetScreenWidth, 0 },
	{ "GetScreenHeight", DxScript::Func_GetScreenHeight, 0 },
	{ "LoadTexture", DxScript::Func_LoadTexture, 1 },
	{ "LoadTextureEx", DxScript::Func_LoadTextureEx, 3 },
	{ "LoadTextureInLoadThread", DxScript::Func_LoadTextureInLoadThread, 1 },
	{ "LoadTextureInLoadThreadEx", DxScript::Func_LoadTextureInLoadThreadEx, 3 },
	{ "RemoveTexture", DxScript::Func_RemoveTexture, 1 },
	{ "GetTextureWidth", DxScript::Func_GetTextureWidth, 1 },
	{ "GetTextureHeight", DxScript::Func_GetTextureHeight, 1 },
	{ "SetFogEnable", DxScript::Func_SetFogEnable, 1 },
	{ "SetFogParam", DxScript::Func_SetFogParam, 5 },
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
	//{ "SetAntiAliasing", DxScript::Func_SetEnableAntiAliasing, 1 },

	{ "LoadShader", DxScript::Func_LoadShader, 1 },
	{ "RemoveShader", DxScript::Func_RemoveShader, 1 },
	{ "SetShader", DxScript::Func_SetShader, 3 },
	{ "SetShaderI", DxScript::Func_SetShaderI, 3 },
	{ "ResetShader", DxScript::Func_ResetShader, 2 },
	{ "ResetShaderI", DxScript::Func_ResetShaderI, 2 },

	{ "LoadMesh", DxScript::Func_LoadMesh, 1 },
	{ "RemoveMesh", DxScript::Func_RemoveMesh, 1 },

	//Dx関数：カメラ3D
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
	{ "GetCameraViewProjectionMatrix", DxScript::Func_GetCameraViewProjectionMatrix, 0 },
	{ "SetCameraPerspectiveClip", DxScript::Func_SetCameraPerspectiveClip, 2 },

	//Dx関数：カメラ2D
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

	//Dx関数：その他
	{ "GetObjectDistance", DxScript::Func_GetObjectDistance, 2 },
	{ "GetObject2dPosition", DxScript::Func_GetObject2dPosition, 1 },
	{ "Get2dPosition", DxScript::Func_Get2dPosition, 3 },

	//Intersection
	{ "IsIntersected_Circle_Circle", DxScript::Func_IsIntersected_Circle_Circle, 6 },
	{ "IsIntersected_Line_Circle", DxScript::Func_IsIntersected_Line_Circle, 8 },
	{ "IsIntersected_Line_Line", DxScript::Func_IsIntersected_Line_Line, 10 },

	//Color
	{ "ColorARGBToHex", DxScript::Func_ColorARGBToHex, 4 },
	{ "ColorHexToARGB", DxScript::Func_ColorHexToARGB, 1 },
	{ "ColorRGBtoHSV", DxScript::Func_ColorRGBtoHSV, 3 },
	{ "ColorHSVtoRGB", DxScript::Func_ColorHSVtoRGB, 3 },

	//Dx関数：オブジェクト操作(共通)
	{ "Obj_Delete", DxScript::Func_Obj_Delete, 1 },
	{ "Obj_IsDeleted", DxScript::Func_Obj_IsDeleted, 1 },
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
	{ "Obj_GetValueR", DxScript::Func_Obj_GetValueR, 2 },
	{ "Obj_GetValueDR", DxScript::Func_Obj_GetValueDR, 3 },
	{ "Obj_SetValueR", DxScript::Func_Obj_SetValueR, 3 },
	{ "Obj_DeleteValueR", DxScript::Func_Obj_DeleteValueR, 2 },
	{ "Obj_IsValueExistsR", DxScript::Func_Obj_IsValueExistsR, 2 },
	{ "Obj_CopyValueTable", DxScript::Func_Obj_CopyValueTable, 3 },
	{ "Obj_GetType", DxScript::Func_Obj_GetType, 1 },

	//Dx関数：オブジェクト操作(RenderObject)
	{ "ObjRender_SetX", DxScript::Func_ObjRender_SetX, 2 },
	{ "ObjRender_SetY", DxScript::Func_ObjRender_SetY, 2 },
	{ "ObjRender_SetZ", DxScript::Func_ObjRender_SetZ, 2 },
	{ "ObjRender_SetPosition", DxScript::Func_ObjRender_SetPosition, 4 },
	{ "ObjRender_SetAngleX", DxScript::Func_ObjRender_SetAngleX, 2 },
	{ "ObjRender_SetAngleY", DxScript::Func_ObjRender_SetAngleY, 2 },
	{ "ObjRender_SetAngleZ", DxScript::Func_ObjRender_SetAngleZ, 2 },
	{ "ObjRender_SetAngleXYZ", DxScript::Func_ObjRender_SetAngleXYZ, 4 },
	{ "ObjRender_SetScaleX", DxScript::Func_ObjRender_SetScaleX, 2 },
	{ "ObjRender_SetScaleY", DxScript::Func_ObjRender_SetScaleY, 2 },
	{ "ObjRender_SetScaleZ", DxScript::Func_ObjRender_SetScaleZ, 2 },
	{ "ObjRender_SetScaleXYZ", DxScript::Func_ObjRender_SetScaleXYZ, 4 },
	{ "ObjRender_SetColor", DxScript::Func_ObjRender_SetColor, 4 },
	{ "ObjRender_SetColor", DxScript::Func_ObjRender_SetColor, 2 },		//Overloaded
	{ "ObjRender_SetColorHSV", DxScript::Func_ObjRender_SetColorHSV, 4 },
	{ "ObjRender_GetColor", DxScript::Func_ObjRender_GetColor, 1 },
	{ "ObjRender_SetAlpha", DxScript::Func_ObjRender_SetAlpha, 2 },
	{ "ObjRender_GetAlpha", DxScript::Func_ObjRender_GetAlpha, 1 },
	{ "ObjRender_SetBlendType", DxScript::Func_ObjRender_SetBlendType, 2 },
	{ "ObjRender_GetBlendType", DxScript::Func_ObjRender_GetBlendType, 1 },
	{ "ObjRender_GetX", DxScript::Func_ObjRender_GetX, 1 },
	{ "ObjRender_GetY", DxScript::Func_ObjRender_GetY, 1 },
	{ "ObjRender_GetZ", DxScript::Func_ObjRender_GetZ, 1 },
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
	{ "ObjRender_SetLightingEnable", DxScript::Func_ObjRender_SetLightingEnable, 3 },
	{ "ObjRender_SetLightingDiffuseColor", DxScript::Func_ObjRender_SetLightingDiffuseColor, 4 },
	{ "ObjRender_SetLightingSpecularColor", DxScript::Func_ObjRender_SetLightingSpecularColor, 4 },
	{ "ObjRender_SetLightingAmbientColor", DxScript::Func_ObjRender_SetLightingAmbientColor, 4 },
	{ "ObjRender_SetLightingDirection", DxScript::Func_ObjRender_SetLightingDirection, 4 },

	//Dx関数：オブジェクト操作(ShaderObject)
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

	//Dx関数：オブジェクト操作(PrimitiveObject)
	{ "ObjPrim_Create", DxScript::Func_ObjPrimitive_Create, 1 },
	{ "ObjPrim_SetPrimitiveType", DxScript::Func_ObjPrimitive_SetPrimitiveType, 2 },
	{ "ObjPrim_GetPrimitiveType", DxScript::Func_ObjPrimitive_GetPrimitiveType, 1 },
	{ "ObjPrim_SetVertexCount", DxScript::Func_ObjPrimitive_SetVertexCount, 2 },
	{ "ObjPrim_SetTexture", DxScript::Func_ObjPrimitive_SetTexture, 2 },
	{ "ObjPrim_GetVertexCount", DxScript::Func_ObjPrimitive_GetVertexCount, 1 },
	{ "ObjPrim_SetVertexPosition", DxScript::Func_ObjPrimitive_SetVertexPosition, 5 },
	{ "ObjPrim_SetVertexUV", DxScript::Func_ObjPrimitive_SetVertexUV, 4 },
	{ "ObjPrim_SetVertexUVT", DxScript::Func_ObjPrimitive_SetVertexUVT, 4 },
	{ "ObjPrim_SetVertexColor", DxScript::Func_ObjPrimitive_SetVertexColor, 5 },
	{ "ObjPrim_SetVertexColor", DxScript::Func_ObjPrimitive_SetVertexColor, 3 },	//Overloaded
	{ "ObjPrim_SetVertexColorHSV", DxScript::Func_ObjPrimitive_SetVertexColorHSV, 5 },
	{ "ObjPrim_SetVertexAlpha", DxScript::Func_ObjPrimitive_SetVertexAlpha, 3 },
	{ "ObjPrim_GetVertexColor", DxScript::Func_ObjPrimitive_GetVertexColor, 2 },
	{ "ObjPrim_GetVertexAlpha", DxScript::Func_ObjPrimitive_GetVertexAlpha, 2 },
	{ "ObjPrim_GetVertexPosition", DxScript::Func_ObjPrimitive_GetVertexPosition, 2 },
	{ "ObjPrim_SetVertexIndex", DxScript::Func_ObjPrimitive_SetVertexIndex, 2 },

	//Dx関数：オブジェクト操作(Sprite2D)
	{ "ObjSprite2D_SetSourceRect", DxScript::Func_ObjSprite2D_SetSourceRect, 5 },
	{ "ObjSprite2D_SetDestRect", DxScript::Func_ObjSprite2D_SetDestRect, 5 },
	{ "ObjSprite2D_SetDestCenter", DxScript::Func_ObjSprite2D_SetDestCenter, 1 },

	//Dx関数：オブジェクト操作(SpriteList2D)
	{ "ObjSpriteList2D_SetSourceRect", DxScript::Func_ObjSpriteList2D_SetSourceRect, 5 },
	{ "ObjSpriteList2D_SetDestRect", DxScript::Func_ObjSpriteList2D_SetDestRect, 5 },
	{ "ObjSpriteList2D_SetDestCenter", DxScript::Func_ObjSpriteList2D_SetDestCenter, 1 },
	{ "ObjSpriteList2D_AddVertex", DxScript::Func_ObjSpriteList2D_AddVertex, 1 },
	{ "ObjSpriteList2D_CloseVertex", DxScript::Func_ObjSpriteList2D_CloseVertex, 1 },
	{ "ObjSpriteList2D_ClearVertexCount", DxScript::Func_ObjSpriteList2D_ClearVertexCount, 1 },
	{ "ObjSpriteList2D_SetAutoClearVertexCount", DxScript::Func_ObjSpriteList2D_SetAutoClearVertexCount, 2 },

	//Dx関数：オブジェクト操作(Sprite3D)
	{ "ObjSprite3D_SetSourceRect", DxScript::Func_ObjSprite3D_SetSourceRect, 5 },
	{ "ObjSprite3D_SetDestRect", DxScript::Func_ObjSprite3D_SetDestRect, 5 },
	{ "ObjSprite3D_SetSourceDestRect", DxScript::Func_ObjSprite3D_SetSourceDestRect, 5 },
	{ "ObjSprite3D_SetBillboard", DxScript::Func_ObjSprite3D_SetBillboard, 2 },

	//Dx関数：オブジェクト操作(TrajectoryObject3D)
	{ "ObjTrajectory3D_SetInitialPoint", DxScript::Func_ObjTrajectory3D_SetInitialPoint, 7 },
	{ "ObjTrajectory3D_SetAlphaVariation", DxScript::Func_ObjTrajectory3D_SetAlphaVariation, 2 },
	{ "ObjTrajectory3D_SetComplementCount", DxScript::Func_ObjTrajectory3D_SetComplementCount, 2 },

	//DxScriptParticleListObject
	{ "ObjParticleList_Create", DxScript::Func_ObjParticleList_Create, 1 },
	{ "ObjParticleList_SetPosition", DxScript::Func_ObjParticleList_SetPosition, 4 },
	{ "ObjParticleList_SetScaleX", DxScript::Func_ObjParticleList_SetScaleSingle<0>, 2 },
	{ "ObjParticleList_SetScaleY", DxScript::Func_ObjParticleList_SetScaleSingle<1>, 2 },
	{ "ObjParticleList_SetScaleZ", DxScript::Func_ObjParticleList_SetScaleSingle<2>, 2 },
	{ "ObjParticleList_SetScale", DxScript::Func_ObjParticleList_SetScaleXYZ, 4 },
	{ "ObjParticleList_SetAngleX", DxScript::Func_ObjParticleList_SetAngleSingle<0>, 2 },
	{ "ObjParticleList_SetAngleY", DxScript::Func_ObjParticleList_SetAngleSingle<1>, 2 },
	{ "ObjParticleList_SetAngleZ", DxScript::Func_ObjParticleList_SetAngleSingle<2>, 2 },
	{ "ObjParticleList_SetAngle", DxScript::Func_ObjParticleList_SetAngle, 4 },
	{ "ObjParticleList_SetColor", DxScript::Func_ObjParticleList_SetColor, 4 },
	{ "ObjParticleList_SetAlpha", DxScript::Func_ObjParticleList_SetAlpha, 2 },
	{ "ObjParticleList_SetExtraData", DxScript::Func_ObjParticleList_SetExtraData, 4 },
	{ "ObjParticleList_AddInstance", DxScript::Func_ObjParticleList_AddInstance, 1 },
	{ "ObjParticleList_ClearInstance", DxScript::Func_ObjParticleList_ClearInstance, 1 },

	//Dx関数：オブジェクト操作(DxMesh)
	{ "ObjMesh_Create", DxScript::Func_ObjMesh_Create, 0 },
	{ "ObjMesh_Load", DxScript::Func_ObjMesh_Load, 2 },
	{ "ObjMesh_SetColor", DxScript::Func_ObjMesh_SetColor, 4 },
	{ "ObjMesh_SetAlpha", DxScript::Func_ObjMesh_SetAlpha, 2 },
	{ "ObjMesh_SetAnimation", DxScript::Func_ObjMesh_SetAnimation, 3 },
	{ "ObjMesh_SetCoordinate2D", DxScript::Func_ObjMesh_SetCoordinate2D, 2 },
	{ "ObjMesh_GetPath", DxScript::Func_ObjMesh_GetPath, 1 },

	//Dx関数：テキスト操作(DxText)
	{ "ObjText_Create", DxScript::Func_ObjText_Create, 0 },
	{ "ObjText_SetText", DxScript::Func_ObjText_SetText, 2 },
	{ "ObjText_SetFontType", DxScript::Func_ObjText_SetFontType, 2 },
	{ "ObjText_SetFontSize", DxScript::Func_ObjText_SetFontSize, 2 },
	{ "ObjText_SetFontBold", DxScript::Func_ObjText_SetFontBold, 2 },
	{ "ObjText_SetFontWeight", DxScript::Func_ObjText_SetFontWeight, 2 },
	{ "ObjText_SetFontColorTop", DxScript::Func_ObjText_SetFontColorTop, 4 },
	{ "ObjText_SetFontColorBottom", DxScript::Func_ObjText_SetFontColorBottom, 4 },
	{ "ObjText_SetFontBorderWidth", DxScript::Func_ObjText_SetFontBorderWidth, 2 },
	{ "ObjText_SetFontBorderType", DxScript::Func_ObjText_SetFontBorderType, 2 },
	{ "ObjText_SetFontBorderColor", DxScript::Func_ObjText_SetFontBorderColor, 4 },
	{ "ObjText_SetFontCharacterSet", DxScript::Func_ObjText_SetFontCharacterSet, 2 },
	{ "ObjText_SetMaxWidth", DxScript::Func_ObjText_SetMaxWidth, 2 },
	{ "ObjText_SetMaxHeight", DxScript::Func_ObjText_SetMaxHeight, 2 },
	{ "ObjText_SetLinePitch", DxScript::Func_ObjText_SetLinePitch, 2 },
	{ "ObjText_SetSidePitch", DxScript::Func_ObjText_SetSidePitch, 2 },
	{ "ObjText_SetVertexColor", DxScript::Func_ObjText_SetVertexColor, 5 },
	{ "ObjText_SetTransCenter", DxScript::Func_ObjText_SetTransCenter, 3 },
	{ "ObjText_SetAutoTransCenter", DxScript::Func_ObjText_SetAutoTransCenter, 2 },
	{ "ObjText_SetHorizontalAlignment", DxScript::Func_ObjText_SetHorizontalAlignment, 2 },
	{ "ObjText_SetSyntacticAnalysis", DxScript::Func_ObjText_SetSyntacticAnalysis, 2 },
	{ "ObjText_GetTextLength", DxScript::Func_ObjText_GetTextLength, 1 },
	{ "ObjText_GetTextLengthCU", DxScript::Func_ObjText_GetTextLengthCU, 1 },
	{ "ObjText_GetTextLengthCUL", DxScript::Func_ObjText_GetTextLengthCUL, 1 },
	{ "ObjText_GetTotalWidth", DxScript::Func_ObjText_GetTotalWidth, 1 },
	{ "ObjText_GetTotalHeight", DxScript::Func_ObjText_GetTotalHeight, 1 },

	//Dx関数：音声操作(DxSoundObject)
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
	{ "ObjSound_SetRestartEnable", DxScript::Func_ObjSound_SetRestartEnable, 2 },
	{ "ObjSound_SetSoundDivision", DxScript::Func_ObjSound_SetSoundDivision, 2 },
	{ "ObjSound_IsPlaying", DxScript::Func_ObjSound_IsPlaying, 1 },
	{ "ObjSound_GetVolumeRate", DxScript::Func_ObjSound_GetVolumeRate, 1 },
	{ "ObjSound_GetWavePosition", DxScript::Func_ObjSound_GetWavePosition, 1 },
	{ "ObjSound_GetWavePositionSampleCount", DxScript::Func_ObjSound_GetWavePositionSampleCount, 1 },
	{ "ObjSound_GetTotalLength", DxScript::Func_ObjSound_GetTotalLength, 1 },
	{ "ObjSound_GetTotalLengthSampleCount", DxScript::Func_ObjSound_GetTotalLengthSampleCount, 1 },

	//Dx関数：ファイル操作(DxFileObject)
	{ "ObjFile_Create", DxScript::Func_ObjFile_Create, 1 },
	{ "ObjFile_Open", DxScript::Func_ObjFile_Open, 2 },
	{ "ObjFile_OpenNW", DxScript::Func_ObjFile_OpenNW, 2 },
	{ "ObjFile_Store", DxScript::Func_ObjFile_Store, 1 },
	{ "ObjFile_GetSize", DxScript::Func_ObjFile_GetSize, 1 },

	//Dx関数：ファイル操作(DxTextFileObject)
	{ "ObjFileT_GetLineCount", DxScript::Func_ObjFileT_GetLineCount, 1 },
	{ "ObjFileT_GetLineText", DxScript::Func_ObjFileT_GetLineText, 2 },
	{ "ObjFileT_SetLineText", DxScript::Func_ObjFileT_SetLineText, 3 },
	{ "ObjFileT_SplitLineText", DxScript::Func_ObjFileT_SplitLineText, 3 },
	{ "ObjFileT_AddLine", DxScript::Func_ObjFileT_AddLine, 2 },
	{ "ObjFileT_ClearLine", DxScript::Func_ObjFileT_ClearLine, 1 },

	////Dx関数：ファイル操作(DxBinalyFileObject)
	{ "ObjFileB_SetByteOrder", DxScript::Func_ObjFileB_SetByteOrder, 2 },
	{ "ObjFileB_SetCharacterCode", DxScript::Func_ObjFileB_SetCharacterCode, 2 },
	{ "ObjFileB_GetPointer", DxScript::Func_ObjFileB_GetPointer, 1 },
	{ "ObjFileB_Seek", DxScript::Func_ObjFileB_Seek, 2 },
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

	//定数
	{ "ID_INVALID", constant<DxScript::ID_INVALID>::func, 0 },
	{ "OBJ_PRIMITIVE_2D", constant<(int)TypeObject::OBJ_PRIMITIVE_2D>::func, 0 },
	{ "OBJ_SPRITE_2D", constant<(int)TypeObject::OBJ_SPRITE_2D>::func, 0 },
	{ "OBJ_SPRITE_LIST_2D", constant<(int)TypeObject::OBJ_SPRITE_LIST_2D>::func, 0 },
	{ "OBJ_PRIMITIVE_3D", constant<(int)TypeObject::OBJ_PRIMITIVE_3D>::func, 0 },
	{ "OBJ_SPRITE_3D", constant<(int)TypeObject::OBJ_SPRITE_3D>::func, 0 },
	{ "OBJ_TRAJECTORY_3D", constant<(int)TypeObject::OBJ_TRAJECTORY_3D>::func, 0 },
	{ "OBJ_PARTICLE_LIST_2D", constant<(int)TypeObject::OBJ_PARTICLE_LIST_2D>::func, 0 },
	{ "OBJ_PARTICLE_LIST_3D", constant<(int)TypeObject::OBJ_PARTICLE_LIST_3D>::func, 0 },
	{ "OBJ_SHADER", constant<(int)TypeObject::OBJ_SHADER>::func, 0 },
	{ "OBJ_MESH", constant<(int)TypeObject::OBJ_MESH>::func, 0 },
	{ "OBJ_TEXT", constant<(int)TypeObject::OBJ_TEXT>::func, 0 },
	{ "OBJ_SOUND", constant<(int)TypeObject::OBJ_SOUND>::func, 0 },
	{ "OBJ_FILE_TEXT", constant<(int)TypeObject::OBJ_FILE_TEXT>::func, 0 },
	{ "OBJ_FILE_BINARY", constant<(int)TypeObject::OBJ_FILE_BINARY>::func, 0 },

	{ "BLEND_NONE", constant<BlendMode::MODE_BLEND_NONE>::func, 0 },
	{ "BLEND_ALPHA", constant<BlendMode::MODE_BLEND_ALPHA>::func, 0 },
	{ "BLEND_ADD_RGB", constant<BlendMode::MODE_BLEND_ADD_RGB>::func, 0 },
	{ "BLEND_ADD_ARGB", constant<BlendMode::MODE_BLEND_ADD_ARGB>::func, 0 },
	{ "BLEND_MULTIPLY", constant<BlendMode::MODE_BLEND_MULTIPLY>::func, 0 },
	{ "BLEND_SUBTRACT", constant<BlendMode::MODE_BLEND_SUBTRACT>::func, 0 },
	{ "BLEND_SHADOW", constant<BlendMode::MODE_BLEND_SHADOW>::func, 0 },
	{ "BLEND_INV_DESTRGB", constant<BlendMode::MODE_BLEND_INV_DESTRGB>::func, 0 },
	{ "BLEND_ALPHA_INV", constant<BlendMode::MODE_BLEND_ALPHA_INV>::func, 0 },

	{ "CULL_NONE", constant<D3DCULL_NONE>::func, 0 },
	{ "CULL_CW", constant<D3DCULL_CW>::func, 0 },
	{ "CULL_CCW", constant<D3DCULL_CCW>::func, 0 },

	{ "IFF_BMP", constant<D3DXIFF_BMP>::func, 0 },
	{ "IFF_JPG", constant<D3DXIFF_JPG>::func, 0 },
	{ "IFF_TGA", constant<D3DXIFF_TGA>::func, 0 },
	{ "IFF_PNG", constant<D3DXIFF_PNG>::func, 0 },
	{ "IFF_DDS", constant<D3DXIFF_DDS>::func, 0 },
	{ "IFF_PPM", constant<D3DXIFF_PPM>::func, 0 },
	//	{"IFF_DIB",constant<D3DXIFF_DIB>::func,0},
	//	{"IFF_HDR",constant<D3DXIFF_HDR>::func,0},
	//	{"IFF_PFM",constant<D3DXIFF_PFM>::func,0},

	{ "FILTER_NONE", constant<D3DTEXF_NONE>::func, 0 },
	{ "FILTER_POINT", constant<D3DTEXF_POINT>::func, 0 },
	{ "FILTER_LINEAR", constant<D3DTEXF_LINEAR>::func, 0 },
	{ "FILTER_ANISOTROPIC", constant<D3DTEXF_ANISOTROPIC>::func, 0 },

	{ "CAMERA_NORMAL", constant<DxCamera::MODE_NORMAL>::func, 0 },
	{ "CAMERA_LOOKAT", constant<DxCamera::MODE_LOOKAT>::func, 0 },

	{ "PRIMITIVE_POINT_LIST", constant<D3DPT_POINTLIST>::func, 0 },
	{ "PRIMITIVE_LINELIST", constant<D3DPT_LINELIST>::func, 0 },
	{ "PRIMITIVE_LINESTRIP", constant<D3DPT_LINESTRIP>::func, 0 },
	{ "PRIMITIVE_TRIANGLELIST", constant<D3DPT_TRIANGLELIST>::func, 0 },
	{ "PRIMITIVE_TRIANGLESTRIP", constant<D3DPT_TRIANGLESTRIP>::func, 0 },
	{ "PRIMITIVE_TRIANGLEFAN", constant<D3DPT_TRIANGLEFAN>::func, 0 },

	{ "BORDER_NONE", constant<DxFont::BORDER_NONE>::func, 0 },
	{ "BORDER_FULL", constant<DxFont::BORDER_FULL>::func, 0 },
	{ "BORDER_SHADOW", constant<DxFont::BORDER_SHADOW>::func, 0 },

	/*
	{ "CHARSET_ANSI", constant<ANSI_CHARSET>::func, 0 },
	{ "CHARSET_DEFAULT", constant<DEFAULT_CHARSET>::func, 0 },
	{ "CHARSET_SHIFTJIS", constant<SHIFTJIS_CHARSET>::func, 0 },
	{ "CHARSET_HANGUL", constant<HANGUL_CHARSET>::func, 0 },
	{ "CHARSET_JOHAB", constant<JOHAB_CHARSET>::func, 0 },
	{ "CHARSET_GB2312", constant<GB2312_CHARSET>::func, 0 },
	{ "CHARSET_CHINESEBIG5", constant<CHINESEBIG5_CHARSET>::func, 0 },
	{ "CHARSET_GREEK", constant<GREEK_CHARSET>::func, 0 },
	{ "CHARSET_TURKISH", constant<TURKISH_CHARSET>::func, 0 },
	{ "CHARSET_VIETNAMESE", constant<VIETNAMESE_CHARSET>::func, 0 },
	{ "CHARSET_", constant<HEBREW_CHARSET>::func, 0 },
	{ "CHARSET_", constant<ARABIC_CHARSET>::func, 0 },
	{ "CHARSET_", constant<BALTIC_CHARSET>::func, 0 },
	{ "CHARSET_", constant<RUSSIAN_CHARSET>::func, 0 },
	{ "CHARSET_", constant<THAI_CHARSET>::func, 0 },
	{ "CHARSET_", constant<EASTEUROPE_CHARSET>::func, 0 },
	*/

	{ "SOUND_BGM", constant<SoundDivision::DIVISION_BGM>::func, 0 },
	{ "SOUND_SE", constant<SoundDivision::DIVISION_SE>::func, 0 },
	{ "SOUND_VOICE", constant<SoundDivision::DIVISION_VOICE>::func, 0 },

	{ "ALIGNMENT_LEFT", constant<DxText::ALIGNMENT_LEFT>::func, 0 },
	{ "ALIGNMENT_RIGHT", constant<DxText::ALIGNMENT_RIGHT>::func, 0 },
	{ "ALIGNMENT_CENTER", constant<DxText::ALIGNMENT_CENTER>::func, 0 },

	{ "CODE_ACP", constant<DxScript::CODE_ACP>::func, 0 },
	{ "CODE_UTF8", constant<DxScript::CODE_UTF8>::func, 0 },
	{ "CODE_UTF16LE", constant<DxScript::CODE_UTF16LE>::func, 0 },
	{ "CODE_UTF16BE", constant<DxScript::CODE_UTF16BE>::func, 0 },

	{ "ENDIAN_LITTLE", constant<ByteOrder::ENDIAN_LITTLE>::func, 0 },
	{ "ENDIAN_BIG", constant<ByteOrder::ENDIAN_BIG>::func, 0 },

	//DirectInput
	{ "KEY_FREE", constant<KEY_FREE>::func, 0 },
	{ "KEY_PUSH", constant<KEY_PUSH>::func, 0 },
	{ "KEY_PULL", constant<KEY_PULL>::func, 0 },
	{ "KEY_HOLD", constant<KEY_HOLD>::func, 0 },

	{ "MOUSE_LEFT", constant<DI_MOUSE_LEFT>::func, 0 },
	{ "MOUSE_RIGHT", constant<DI_MOUSE_RIGHT>::func, 0 },
	{ "MOUSE_MIDDLE", constant<DI_MOUSE_MIDDLE>::func, 0 },

	{ "KEY_ESCAPE", constant<DIK_ESCAPE>::func, 0 },
	{ "KEY_1", constant<DIK_1>::func, 0 },
	{ "KEY_2", constant<DIK_2>::func, 0 },
	{ "KEY_3", constant<DIK_3>::func, 0 },
	{ "KEY_4", constant<DIK_4>::func, 0 },
	{ "KEY_5", constant<DIK_5>::func, 0 },
	{ "KEY_6", constant<DIK_6>::func, 0 },
	{ "KEY_7", constant<DIK_7>::func, 0 },
	{ "KEY_8", constant<DIK_8>::func, 0 },
	{ "KEY_9", constant<DIK_9>::func, 0 },
	{ "KEY_0", constant<DIK_0>::func, 0 },
	{ "KEY_MINUS", constant<DIK_MINUS>::func, 0 },
	{ "KEY_EQUALS", constant<DIK_EQUALS>::func, 0 },
	{ "KEY_BACK", constant<DIK_BACK>::func, 0 },
	{ "KEY_TAB", constant<DIK_TAB>::func, 0 },
	{ "KEY_Q", constant<DIK_Q>::func, 0 },
	{ "KEY_W", constant<DIK_W>::func, 0 },
	{ "KEY_E", constant<DIK_E>::func, 0 },
	{ "KEY_R", constant<DIK_R>::func, 0 },
	{ "KEY_T", constant<DIK_T>::func, 0 },
	{ "KEY_Y", constant<DIK_Y>::func, 0 },
	{ "KEY_U", constant<DIK_U>::func, 0 },
	{ "KEY_I", constant<DIK_I>::func, 0 },
	{ "KEY_O", constant<DIK_O>::func, 0 },
	{ "KEY_P", constant<DIK_P>::func, 0 },
	{ "KEY_LBRACKET", constant<DIK_LBRACKET>::func, 0 },
	{ "KEY_RBRACKET", constant<DIK_RBRACKET>::func, 0 },
	{ "KEY_RETURN", constant<DIK_RETURN>::func, 0 },
	{ "KEY_LCONTROL", constant<DIK_LCONTROL>::func, 0 },
	{ "KEY_A", constant<DIK_A>::func, 0 },
	{ "KEY_S", constant<DIK_S>::func, 0 },
	{ "KEY_D", constant<DIK_D>::func, 0 },
	{ "KEY_F", constant<DIK_F>::func, 0 },
	{ "KEY_G", constant<DIK_G>::func, 0 },
	{ "KEY_H", constant<DIK_H>::func, 0 },
	{ "KEY_J", constant<DIK_J>::func, 0 },
	{ "KEY_K", constant<DIK_K>::func, 0 },
	{ "KEY_L", constant<DIK_L>::func, 0 },
	{ "KEY_SEMICOLON", constant<DIK_SEMICOLON>::func, 0 },
	{ "KEY_APOSTROPHE", constant<DIK_APOSTROPHE>::func, 0 },
	{ "KEY_GRAVE", constant<DIK_GRAVE>::func, 0 },
	{ "KEY_LSHIFT", constant<DIK_LSHIFT>::func, 0 },
	{ "KEY_BACKSLASH", constant<DIK_BACKSLASH>::func, 0 },
	{ "KEY_Z", constant<DIK_Z>::func, 0 },
	{ "KEY_X", constant<DIK_X>::func, 0 },
	{ "KEY_C", constant<DIK_C>::func, 0 },
	{ "KEY_V", constant<DIK_V>::func, 0 },
	{ "KEY_B", constant<DIK_B>::func, 0 },
	{ "KEY_N", constant<DIK_N>::func, 0 },
	{ "KEY_M", constant<DIK_M>::func, 0 },
	{ "KEY_COMMA", constant<DIK_COMMA>::func, 0 },
	{ "KEY_PERIOD", constant<DIK_PERIOD>::func, 0 },
	{ "KEY_SLASH", constant<DIK_SLASH>::func, 0 },
	{ "KEY_RSHIFT", constant<DIK_RSHIFT>::func, 0 },
	{ "KEY_MULTIPLY", constant<DIK_MULTIPLY>::func, 0 },
	{ "KEY_LMENU", constant<DIK_LMENU>::func, 0 },
	{ "KEY_SPACE", constant<DIK_SPACE>::func, 0 },
	{ "KEY_CAPITAL", constant<DIK_CAPITAL>::func, 0 },
	{ "KEY_F1", constant<DIK_F1>::func, 0 },
	{ "KEY_F2", constant<DIK_F2>::func, 0 },
	{ "KEY_F3", constant<DIK_F3>::func, 0 },
	{ "KEY_F4", constant<DIK_F4>::func, 0 },
	{ "KEY_F5", constant<DIK_F5>::func, 0 },
	{ "KEY_F6", constant<DIK_F6>::func, 0 },
	{ "KEY_F7", constant<DIK_F7>::func, 0 },
	{ "KEY_F8", constant<DIK_F8>::func, 0 },
	{ "KEY_F9", constant<DIK_F9>::func, 0 },
	{ "KEY_F10", constant<DIK_F10>::func, 0 },
	{ "KEY_NUMLOCK", constant<DIK_NUMLOCK>::func, 0 },
	{ "KEY_SCROLL", constant<DIK_SCROLL>::func, 0 },
	{ "KEY_NUMPAD7", constant<DIK_NUMPAD7>::func, 0 },
	{ "KEY_NUMPAD8", constant<DIK_NUMPAD8>::func, 0 },
	{ "KEY_NUMPAD9", constant<DIK_NUMPAD9>::func, 0 },
	{ "KEY_SUBTRACT", constant<DIK_SUBTRACT>::func, 0 },
	{ "KEY_NUMPAD4", constant<DIK_NUMPAD4>::func, 0 },
	{ "KEY_NUMPAD5", constant<DIK_NUMPAD5>::func, 0 },
	{ "KEY_NUMPAD6", constant<DIK_NUMPAD6>::func, 0 },
	{ "KEY_ADD", constant<DIK_ADD>::func, 0 },
	{ "KEY_NUMPAD1", constant<DIK_NUMPAD1>::func, 0 },
	{ "KEY_NUMPAD2", constant<DIK_NUMPAD2>::func, 0 },
	{ "KEY_NUMPAD3", constant<DIK_NUMPAD3>::func, 0 },
	{ "KEY_NUMPAD0", constant<DIK_NUMPAD0>::func, 0 },
	{ "KEY_DECIMAL", constant<DIK_DECIMAL>::func, 0 },
	{ "KEY_F11", constant<DIK_F11>::func, 0 },
	{ "KEY_F12", constant<DIK_F12>::func, 0 },
	{ "KEY_F13", constant<DIK_F13>::func, 0 },
	{ "KEY_F14", constant<DIK_F14>::func, 0 },
	{ "KEY_F15", constant<DIK_F15>::func, 0 },
	{ "KEY_KANA", constant<DIK_KANA>::func, 0 },
	{ "KEY_CONVERT", constant<DIK_CONVERT>::func, 0 },
	{ "KEY_NOCONVERT", constant<DIK_NOCONVERT>::func, 0 },
	{ "KEY_YEN", constant<DIK_YEN>::func, 0 },
	{ "KEY_NUMPADEQUALS", constant<DIK_NUMPADEQUALS>::func, 0 },
	{ "KEY_CIRCUMFLEX", constant<DIK_CIRCUMFLEX>::func, 0 },
	{ "KEY_AT", constant<DIK_AT>::func, 0 },
	{ "KEY_COLON", constant<DIK_COLON>::func, 0 },
	{ "KEY_UNDERLINE", constant<DIK_UNDERLINE>::func, 0 },
	{ "KEY_KANJI", constant<DIK_KANJI>::func, 0 },
	{ "KEY_STOP", constant<DIK_STOP>::func, 0 },
	{ "KEY_AX", constant<DIK_AX>::func, 0 },
	{ "KEY_UNLABELED", constant<DIK_UNLABELED>::func, 0 },
	{ "KEY_NUMPADENTER", constant<DIK_NUMPADENTER>::func, 0 },
	{ "KEY_RCONTROL", constant<DIK_RCONTROL>::func, 0 },
	{ "KEY_NUMPADCOMMA", constant<DIK_NUMPADCOMMA>::func, 0 },
	{ "KEY_DIVIDE", constant<DIK_DIVIDE>::func, 0 },
	{ "KEY_SYSRQ", constant<DIK_SYSRQ>::func, 0 },
	{ "KEY_RMENU", constant<DIK_RMENU>::func, 0 },
	{ "KEY_PAUSE", constant<DIK_PAUSE>::func, 0 },
	{ "KEY_HOME", constant<DIK_HOME>::func, 0 },
	{ "KEY_UP", constant<DIK_UP>::func, 0 },
	{ "KEY_PRIOR", constant<DIK_PRIOR>::func, 0 },
	{ "KEY_LEFT", constant<DIK_LEFT>::func, 0 },
	{ "KEY_RIGHT", constant<DIK_RIGHT>::func, 0 },
	{ "KEY_END", constant<DIK_END>::func, 0 },
	{ "KEY_DOWN", constant<DIK_DOWN>::func, 0 },
	{ "KEY_NEXT", constant<DIK_NEXT>::func, 0 },
	{ "KEY_INSERT", constant<DIK_INSERT>::func, 0 },
	{ "KEY_DELETE", constant<DIK_DELETE>::func, 0 },
	{ "KEY_LWIN", constant<DIK_LWIN>::func, 0 },
	{ "KEY_RWIN", constant<DIK_RWIN>::func, 0 },
	{ "KEY_APPS", constant<DIK_APPS>::func, 0 },
	{ "KEY_POWER", constant<DIK_POWER>::func, 0 },
	{ "KEY_SLEEP", constant<DIK_SLEEP>::func, 0 },
};

DxScript::DxScript() {
	_AddFunction(dxFunction, sizeof(dxFunction) / sizeof(function));
	objManager_ = std::shared_ptr<DxScriptObjectManager>(new DxScriptObjectManager);
}
DxScript::~DxScript() {
	_ClearResource();
}
void DxScript::_ClearResource() {
	mapTexture_.clear();
	mapShader_.clear();
	mapMesh_.clear();

	for (auto itrSound = mapSoundPlayer_.begin(); itrSound != mapSoundPlayer_.end(); ++itrSound) {
		itrSound->second->Delete();
	}
	mapSoundPlayer_.clear();
}
int DxScript::AddObject(shared_ptr<DxScriptObjectBase> obj, bool bActivate) {
	obj->idScript_ = idScript_;
	return objManager_->AddObject(obj, bActivate);
}
shared_ptr<Texture> DxScript::_GetTexture(const std::wstring& name) {
	shared_ptr<Texture> res;
	auto itr = mapTexture_.find(name);
	if (itr != mapTexture_.end()) res = itr->second;
	return res;
}
shared_ptr<Shader> DxScript::_GetShader(const std::wstring& name) {
	shared_ptr<Shader> res;
	auto itr = mapShader_.find(name);
	if (itr != mapShader_.end()) res = itr->second;
	return res;
}
shared_ptr<DxMesh> DxScript::_GetMesh(const std::wstring& name) {
	shared_ptr<DxMesh> res;
	auto itr = mapMesh_.find(name);
	if (itr != mapMesh_.end()) res = itr->second;
	return res;
}

gstd::value DxScript::Func_MatrixIdentity(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);

	return script->CreateRealArrayValue(reinterpret_cast<FLOAT*>(&(mat._11)), 16U);
}
gstd::value DxScript::Func_MatrixInverse(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	IsMatrix(machine, arg0);

	D3DXMATRIX mat;
	FLOAT* ptrMat = &mat._11;
	for (size_t i = 0; i < 16; ++i) {
		ptrMat[i] = arg0.index_as_array(i).as_real();
	}
	D3DXMatrixInverse(&mat, nullptr, &mat);

	return script->CreateRealArrayValue(ptrMat, 16U);
}
gstd::value DxScript::Func_MatrixAdd(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	const value& arg1 = argv[1];
	IsMatrix(machine, arg0);
	IsMatrix(machine, arg1);

	D3DXMATRIX mat;
	FLOAT* ptrMat = &mat._11;
	for (size_t i = 0; i < 16; ++i) {
		const value& v0 = arg0.index_as_array(i);
		const value& v1 = arg1.index_as_array(i);
		ptrMat[i] = v0.as_real() + v1.as_real();
	}

	return script->CreateRealArrayValue(ptrMat, 16U);
}
gstd::value DxScript::Func_MatrixSubtract(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	const value& arg1 = argv[1];
	IsMatrix(machine, arg0);
	IsMatrix(machine, arg1);

	D3DXMATRIX mat;
	FLOAT* ptrMat = &mat._11;
	for (size_t i = 0; i < 16; ++i) {
		const value& v0 = arg0.index_as_array(i);
		const value& v1 = arg1.index_as_array(i);
		ptrMat[i] = v0.as_real() - v1.as_real();
	}

	return script->CreateRealArrayValue(ptrMat, 16U);
}
gstd::value DxScript::Func_MatrixMultiply(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	const value& arg1 = argv[1];
	IsMatrix(machine, arg0);
	IsMatrix(machine, arg1);

	D3DXMATRIX mat;
	FLOAT* ptrMat = &mat._11;
	for (size_t i = 0; i < 16; ++i) {
		const value& v0 = arg0.index_as_array(i);
		const value& v1 = arg1.index_as_array(i);
		ptrMat[i] = v0.as_real() * v1.as_real();
	}

	return script->CreateRealArrayValue(ptrMat, 16U);
}
gstd::value DxScript::Func_MatrixDivide(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	const value& arg1 = argv[1];
	IsMatrix(machine, arg0);
	IsMatrix(machine, arg1);

	D3DXMATRIX mat;
	FLOAT* ptrMat = &mat._11;
	for (size_t i = 0; i < 16; ++i) {
		const value& v0 = arg0.index_as_array(i);
		const value& v1 = arg1.index_as_array(i);
		ptrMat[i] = v0.as_real() / v1.as_real();
	}

	return script->CreateRealArrayValue(ptrMat, 16U);
}
gstd::value DxScript::Func_MatrixTranspose(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	IsMatrix(machine, arg0);

	D3DXMATRIX mat;
	FLOAT* ptrMat = &mat._11;
	for (size_t i = 0; i < 16; ++i) {
		ptrMat[i] = arg0.index_as_array(i).as_real();
	}
	D3DXMatrixTranspose(&mat, &mat);

	return script->CreateRealArrayValue(ptrMat, 16U);
}
gstd::value DxScript::Func_MatrixDeterminant(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	IsMatrix(machine, arg0);

	D3DXMATRIX mat;
	FLOAT* ptrMat = &mat._11;
	for (size_t i = 0; i < 16; ++i) {
		ptrMat[i] = arg0.index_as_array(i).as_real();
	}

	return script->CreateRealValue(D3DXMatrixDeterminant(&mat));
}
gstd::value DxScript::Func_MatrixLookatLH(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	auto BuildVector = [&](const value& val) -> D3DXVECTOR3 {
		IsVector(machine, val, 3);
		return D3DXVECTOR3((FLOAT)val.index_as_array(0).as_real(),
			(FLOAT)val.index_as_array(1).as_real(), (FLOAT)val.index_as_array(2).as_real());
	};
	D3DXVECTOR3 eye = BuildVector(argv[0]);
	D3DXVECTOR3 dest = BuildVector(argv[1]);
	D3DXVECTOR3 up = BuildVector(argv[2]);

	D3DXMATRIX mat;
	D3DXMatrixLookAtLH(&mat, &eye, &dest, &up);

	return script->CreateRealArrayValue(reinterpret_cast<FLOAT*>(&(mat._11)), 16U);
}
gstd::value DxScript::Func_MatrixLookatRH(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	auto BuildVector = [&](const value& val) -> D3DXVECTOR3 {
		IsVector(machine, val, 3);
		return D3DXVECTOR3((FLOAT)val.index_as_array(0).as_real(),
			(FLOAT)val.index_as_array(1).as_real(), (FLOAT)val.index_as_array(2).as_real());
	};
	D3DXVECTOR3 eye = BuildVector(argv[0]);
	D3DXVECTOR3 dest = BuildVector(argv[1]);
	D3DXVECTOR3 up = BuildVector(argv[2]);

	D3DXMATRIX mat;
	D3DXMatrixLookAtRH(&mat, &eye, &dest, &up);

	return script->CreateRealArrayValue(reinterpret_cast<FLOAT*>(&(mat._11)), 16U);
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
	if (script->mapSoundPlayer_.find(path) != script->mapSoundPlayer_.end())
		return script->CreateBooleanValue(true);

	shared_ptr<SoundPlayer> player = manager->GetPlayer(path, true);
	if (player)
		script->mapSoundPlayer_[path] = player;
	
	return script->CreateBooleanValue(player != nullptr);
}
value DxScript::Func_RemoveSound(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	auto itr = script->mapSoundPlayer_.find(path);
	if (itr != script->mapSoundPlayer_.end()) {
		itr->second->Delete();
		script->mapSoundPlayer_.erase(itr);
	}
	return value();
}
value DxScript::Func_PlayBGM(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	auto itr = script->mapSoundPlayer_.find(path);
	if (itr != script->mapSoundPlayer_.end()) {
		double loopStart = argv[1].as_real();
		double loopEnd = argv[2].as_real();

		shared_ptr<SoundPlayer> player = itr->second;
		player->SetAutoDelete(true);
		player->SetSoundDivision(SoundDivision::DIVISION_BGM);

		SoundPlayer::PlayStyle style;
		style.SetLoopEnable(true);
		style.SetLoopStartTime(loopStart);
		style.SetLoopEndTime(loopEnd);
		//player->Play(style);
		script->GetObjectManager()->ReserveSound(player, style);
	}
	return value();
}
gstd::value DxScript::Func_PlaySE(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	auto itr = script->mapSoundPlayer_.find(path);
	if (itr != script->mapSoundPlayer_.end()) {
		shared_ptr<SoundPlayer> player = itr->second;
		player->SetAutoDelete(true);
		player->SetSoundDivision(SoundDivision::DIVISION_SE);

		SoundPlayer::PlayStyle style;
		style.SetLoopEnable(false);
		//player->Play(style);
		script->GetObjectManager()->ReserveSound(player, style);
	}
	return value();
}
value DxScript::Func_StopSound(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	auto itr = script->mapSoundPlayer_.find(path);
	if (itr != script->mapSoundPlayer_.end()) {
		shared_ptr<SoundPlayer> player = itr->second;
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
	int state = input->GetKeyState(key);
	return DxScript::CreateRealValue(state);
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
	return DxScript::CreateRealValue(res);
}
gstd::value DxScript::Func_GetVirtualKeyState(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DIKeyState res = KEY_FREE;
	VirtualKeyManager* input = dynamic_cast<VirtualKeyManager*>(DirectInput::GetBase());
	if (input) {
		int16_t key = (int16_t)argv[0].as_int();
		res = input->GetVirtualKeyState(key);
	}
	return DxScript::CreateRealValue(res);
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
gstd::value DxScript::Func_GetScreenWidth(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG res = graphics->GetScreenWidth();
	return DxScript::CreateRealValue(res);
}
gstd::value DxScript::Func_GetScreenHeight(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG res = graphics->GetScreenHeight();
	return DxScript::CreateRealValue(res);
}
value DxScript::Func_LoadTexture(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = true;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	if (script->mapTexture_.find(path) == script->mapTexture_.end()) {
		shared_ptr<Texture> texture(new Texture());
		res = texture->CreateFromFile(path, false, false);
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapTexture_[path] = texture;
		}
	}
	return script->CreateBooleanValue(res);
}
value DxScript::Func_LoadTextureInLoadThread(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = true;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	if (script->mapTexture_.find(path) == script->mapTexture_.end()) {
		shared_ptr<Texture> texture(new Texture());
		res = texture->CreateFromFileInLoadThread(path, false, false);
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapTexture_[path] = texture;
		}
	}
	return script->CreateBooleanValue(res);
}
value DxScript::Func_LoadTextureEx(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = true;
	std::wstring path = argv[0].as_string();
	bool useMipMap = argv[1].as_boolean();
	bool useNonPowerOfTwo = argv[2].as_boolean();
	path = PathProperty::GetUnique(path);

	if (script->mapTexture_.find(path) == script->mapTexture_.end()) {
		shared_ptr<Texture> texture(new Texture());
		res = texture->CreateFromFile(path, useMipMap, useNonPowerOfTwo);
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapTexture_[path] = texture;
		}
	}
	return script->CreateBooleanValue(res);
}
value DxScript::Func_LoadTextureInLoadThreadEx(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = true;
	std::wstring path = argv[0].as_string();
	bool useMipMap = argv[1].as_boolean();
	bool useNonPowerOfTwo = argv[2].as_boolean();
	path = PathProperty::GetUnique(path);

	if (script->mapTexture_.find(path) == script->mapTexture_.end()) {
		shared_ptr<Texture> texture(new Texture());
		res = texture->CreateFromFileInLoadThread(path, useMipMap, useNonPowerOfTwo);
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapTexture_[path] = texture;
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
		script->mapTexture_.erase(path);
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
	byte r = ColorAccess::ClampColorRet(argv[2].as_int());
	byte g = ColorAccess::ClampColorRet(argv[3].as_int());
	byte b = ColorAccess::ClampColorRet(argv[4].as_int());
	D3DCOLOR color = D3DCOLOR_ARGB(255, r, g, b);
	script->GetObjectManager()->SetFogParam(true, color, start, end);
	return value();
}
gstd::value DxScript::Func_CreateRenderTarget(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = false;
	std::wstring name = argv[0].as_string();

	if (script->mapTexture_.find(name) == script->mapTexture_.end()) {
		shared_ptr<Texture> texture(new Texture());
		res = texture->CreateRenderTarget(name);
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapTexture_[name] = texture;
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
		if (script->mapTexture_.find(name) == script->mapTexture_.end()) {
			shared_ptr<Texture> texture(new Texture());
			res = texture->CreateRenderTarget(name, (size_t)width, (size_t)height);
			if (res) {
				Lock lock(script->criticalSection_);
				script->mapTexture_[name] = texture;
			}
		}
	}
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_SetRenderTarget(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	TextureManager* textureManager = TextureManager::GetBase();

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = script->_GetTexture(name);
	if (texture == nullptr) {
		texture = textureManager->GetTexture(name);
		script->RaiseError("The specified render target does not exist.");
	}
	if (texture->GetType() != TextureData::TYPE_RENDER_TARGET)
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

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = script->_GetTexture(name);
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

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = script->_GetTexture(name);
	if (texture == nullptr)
		texture = textureManager->GetTexture(name);
	if (texture == nullptr)
		return script->CreateBooleanValue(false);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	shared_ptr<Texture> current = graphics->GetRenderTarget();

	byte ca = ColorAccess::ClampColorRet(argv[1].as_int());
	byte cr = ColorAccess::ClampColorRet(argv[2].as_int());
	byte cg = ColorAccess::ClampColorRet(argv[3].as_int());
	byte cb = ColorAccess::ClampColorRet(argv[4].as_int());

	IDirect3DDevice9* device = graphics->GetDevice();
	graphics->SetRenderTarget(texture, false);
	device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(ca, cr, cg, cb), 1.0f, 0);
	graphics->SetRenderTarget(current, false);

	return script->CreateBooleanValue(true);
}
gstd::value DxScript::Func_ClearRenderTargetA3(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	TextureManager* textureManager = TextureManager::GetBase();

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = script->_GetTexture(name);
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

	LONG rl = (LONG)argv[5].as_int();
	LONG rt = (LONG)argv[6].as_int();
	LONG rr = (LONG)argv[7].as_int();
	LONG rb = (LONG)argv[8].as_int();
	D3DRECT rc = { rl, rt, rr, rb };

	IDirect3DDevice9* device = graphics->GetDevice();
	graphics->SetRenderTarget(texture, false);
	device->Clear(1, &rc, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(ca, cr, cg, cb), 1.0f, 0);
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

	shared_ptr<Texture> texture = script->_GetTexture(nameTexture);
	if (texture == nullptr)
		texture = textureManager->GetTexture(nameTexture);

	if (texture) {
		//フォルダ生成
		std::wstring dir = PathProperty::GetFileDirectory(path);
		File::CreateFileDirectory(dir);

		//保存
		IDirect3DSurface9* pSurface = texture->GetD3DSurface();
		RECT rect = { 0, 0, graphics->GetScreenWidth(), graphics->GetScreenHeight() };
		D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_BMP,
			pSurface, nullptr, &rect);
	}

	return value();
}
gstd::value DxScript::Func_SaveRenderedTextureA2(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	std::wstring nameTexture = argv[0].as_string();
	std::wstring path = argv[1].as_string();
	path = PathProperty::GetUnique(path);

	LONG rcLeft = (LONG)argv[2].as_int();
	LONG rcTop = (LONG)argv[3].as_int();
	LONG rcRight = (LONG)argv[4].as_int();
	LONG rcBottom = (LONG)argv[5].as_int();

	TextureManager* textureManager = TextureManager::GetBase();
	DirectGraphics* graphics = DirectGraphics::GetBase();

	shared_ptr<Texture> texture = script->_GetTexture(nameTexture);
	if (texture == nullptr)
		texture = textureManager->GetTexture(nameTexture);
	if (texture) {
		//フォルダ生成
		std::wstring dir = PathProperty::GetFileDirectory(path);
		File::CreateFileDirectory(dir);

		//保存
		IDirect3DSurface9* pSurface = texture->GetD3DSurface();
		RECT rect = { rcLeft, rcTop, rcRight, rcBottom };
		D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_BMP,
			pSurface, nullptr, &rect);
	}

	return value();
}
gstd::value DxScript::Func_SaveRenderedTextureA3(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	std::wstring nameTexture = argv[0].as_string();
	std::wstring path = argv[1].as_string();
	path = PathProperty::GetUnique(path);

	LONG rcLeft = (LONG)argv[2].as_int();
	LONG rcTop = (LONG)argv[3].as_int();
	LONG rcRight = (LONG)argv[4].as_int();
	LONG rcBottom = (LONG)argv[5].as_int();
	BYTE imgFormat = argv[6].as_int();

	if (imgFormat < 0)
		imgFormat = 0;
	if (imgFormat > D3DXIFF_PPM)
		imgFormat = D3DXIFF_PPM;

	TextureManager* textureManager = TextureManager::GetBase();
	DirectGraphics* graphics = DirectGraphics::GetBase();

	shared_ptr<Texture> texture = script->_GetTexture(nameTexture);
	if (texture == nullptr)
		texture = textureManager->GetTexture(nameTexture);
	if (texture) {
		//フォルダ生成
		std::wstring dir = PathProperty::GetFileDirectory(path);
		File::CreateFileDirectory(dir);

		//保存
		IDirect3DSurface9* pSurface = texture->GetD3DSurface();
		RECT rect = { rcLeft, rcTop, rcRight, rcBottom };
		D3DXSaveSurfaceToFile(path.c_str(), (D3DXIMAGE_FILEFORMAT)imgFormat,
			pSurface, nullptr, &rect);
	}

	return value();
}
gstd::value DxScript::Func_IsPixelShaderSupported(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	int major = argv[0].as_int();
	int minor = argv[1].as_int();

	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool res = graphics->IsPixelShaderSupported(major, minor);

	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_SetEnableAntiAliasing(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	bool enable = argv[0].as_boolean();

	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool res = SUCCEEDED(graphics->SetFullscreenAntiAliasing(enable));

	return DxScript::CreateBooleanValue(res);
}

value DxScript::Func_LoadMesh(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = true;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	if (script->mapMesh_.find(path) == script->mapMesh_.end()) {
		shared_ptr<DxMesh> mesh;
		res = mesh->CreateFromFile(path);
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapMesh_[path] = mesh;
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
		script->mapMesh_.erase(path);
	}
	return value();
}

value DxScript::Func_LoadShader(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = true;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	if (script->mapShader_.find(path) == script->mapShader_.end()) {
		ShaderManager* manager = ShaderManager::GetBase();
		shared_ptr<Shader> shader = manager->CreateFromFile(path);
		res = shader != nullptr;
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapShader_[path] = shader;
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
		script->mapShader_.erase(path);
	}
	return value();
}
gstd::value DxScript::Func_SetShader(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
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
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
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
value DxScript::Func_GetCameraViewProjectionMatrix(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectGraphics* graphics = DirectGraphics::GetBase();

	D3DXMATRIX* matViewProj = &graphics->GetCamera()->GetViewProjectionMatrix();

	return script->CreateRealArrayValue(reinterpret_cast<FLOAT*>(matViewProj), 16U);
}
value DxScript::Func_SetCameraPerspectiveClip(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	auto camera = graphics->GetCamera();
	camera->SetPerspectiveClip(argv[0].as_real(), argv[1].as_real());
	camera->thisProjectionChanged_ = true;
	return value();
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
gstd::value DxScript::Func_GetObjectDistance(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id1 = (int)argv[0].as_real();
	int id2 = (int)argv[1].as_real();

	FLOAT res = -1.0f;
	DxScriptRenderObject* obj1 = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id1));
	if (obj1) {
		DxScriptRenderObject* obj2 = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id2));
		if (obj2) {
			D3DXVECTOR3 diff = obj1->GetPosition() - obj2->GetPosition();
			res = D3DXVec3Length(&diff);
		}
	}
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_GetObject2dPosition(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj == nullptr) script->RaiseError("Invalid object; object might have been deleted.");

	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();

	D3DXVECTOR2 point = camera->TransformCoordinateTo2D(obj->GetPosition());
	float listRes[2] = { point.x, point.y };

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
gstd::value DxScript::Func_IsIntersected_Circle_Circle(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxCircle circle1(argv[0].as_real(), argv[1].as_real(), argv[2].as_real());
	DxCircle circle2(argv[3].as_real(), argv[4].as_real(), argv[5].as_real());

	bool res = DxMath::IsIntersected(circle1, circle2);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Line_Circle(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxWidthLine line(
		argv[0].as_real(),
		argv[1].as_real(),
		argv[2].as_real(),
		argv[3].as_real(),
		argv[4].as_real()
	);
	DxCircle circle(argv[5].as_real(), argv[6].as_real(), argv[7].as_real());

	bool res = DxMath::IsIntersected(circle, line);
	return DxScript::CreateBooleanValue(res);
}
gstd::value DxScript::Func_IsIntersected_Line_Line(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxWidthLine line1(
		argv[0].as_real(),
		argv[1].as_real(),
		argv[2].as_real(),
		argv[3].as_real(),
		argv[4].as_real()
	);
	DxWidthLine line2(
		argv[5].as_real(),
		argv[6].as_real(),
		argv[7].as_real(),
		argv[8].as_real(),
		argv[9].as_real()
	);

	bool res = DxMath::IsIntersected(line1, line2);
	return DxScript::CreateBooleanValue(res);
}

//Color
gstd::value DxScript::Func_ColorARGBToHex(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	byte ca = argv[0].as_int();
	byte cr = argv[1].as_int();
	byte cg = argv[2].as_int();
	byte cb = argv[3].as_int();
	return DxScript::CreateRealValue(D3DCOLOR_ARGB(ca, cr, cg, cb));
}
gstd::value DxScript::Func_ColorHexToARGB(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	D3DCOLOR color = (D3DCOLOR)argv[0].as_int();
	D3DXVECTOR4 vecColor = ColorAccess::ToVec4(color);
	return DxScript::CreateRealArrayValue(reinterpret_cast<FLOAT*>(&vecColor), 4U);
}
gstd::value DxScript::Func_ColorRGBtoHSV(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	byte cr = argv[0].as_int();
	byte cg = argv[1].as_int();
	byte cb = argv[2].as_int();

	D3DXVECTOR3 hsv;
	ColorAccess::RGBtoHSV(hsv, cr, cg, cb);

	return DxScript::CreateRealArrayValue(reinterpret_cast<FLOAT*>(&hsv), 3U);
}
gstd::value DxScript::Func_ColorHSVtoRGB(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	int ch = argv[0].as_int();
	byte cs = argv[1].as_int();
	byte cv = argv[2].as_int();
	
	D3DCOLOR rgb = 0xffffffff;
	ColorAccess::HSVtoRGB(rgb, ch, cs, cv);

	D3DXVECTOR4 rgbVec = ColorAccess::ToVec4(rgb);
	return DxScript::CreateRealArrayValue(reinterpret_cast<FLOAT*>(&rgbVec.y), 3U);
}

//Dx関数：オブジェクト操作(共通)
value DxScript::Func_Obj_Delete(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	script->CheckRunInMainThread();
	int id = (int)argv[0].as_real();
	script->DeleteObject(id);
	return value();
}
value DxScript::Func_Obj_IsDeleted(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	return script->CreateBooleanValue(obj == nullptr);
}
value DxScript::Func_Obj_SetVisible(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj)
		obj->bVisible_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_Obj_IsVisible(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	bool res = false;
	if (obj) res = obj->bVisible_;
	return script->CreateBooleanValue(res);
}
value DxScript::Func_Obj_SetRenderPriority(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
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
	int id = (int)argv[0].as_real();
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
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj)
		res = obj->GetRenderPriorityI() / (double)(script->GetObjectManager()->GetRenderBucketCapacity() - 1U);

	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_Obj_GetRenderPriorityI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = 0;

	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) res = obj->GetRenderPriorityI();

	return script->CreateRealValue(res);
}

gstd::value DxScript::Func_Obj_GetValue(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
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
	int id = (int)argv[0].as_real();
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
	int id = (int)argv[0].as_real();
	std::wstring key = argv[1].as_string();
	gstd::value val = argv[2];

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) obj->SetObjectValue(key, val);

	return value();
}
gstd::value DxScript::Func_Obj_DeleteValue(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	std::wstring key = argv[1].as_string();

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) obj->DeleteObjectValue(key);

	return value();
}
gstd::value DxScript::Func_Obj_IsValueExists(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	std::wstring key = argv[1].as_string();

	bool res = false;

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) res = obj->IsObjectValueExists(key);

	return script->CreateBooleanValue(res);
}

gstd::value DxScript::Func_Obj_GetValueR(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double keyDbl = argv[1].as_real();

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		auto itr = obj->mapObjectValue_.find(DxScriptObjectBase::GetKeyHashReal(keyDbl));
		if (itr != obj->mapObjectValue_.end()) return itr->second;
	}
	return value();
}
gstd::value DxScript::Func_Obj_GetValueDR(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double keyDbl = argv[1].as_real();
	gstd::value def = argv[2];

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		auto itr = obj->mapObjectValue_.find(DxScriptObjectBase::GetKeyHashReal(keyDbl));
		if (itr != obj->mapObjectValue_.end()) return itr->second;
	}
	return def;
}
gstd::value DxScript::Func_Obj_SetValueR(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double keyDbl = argv[1].as_real();
	gstd::value val = argv[2];

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		obj->SetObjectValue(DxScriptObjectBase::GetKeyHashReal(keyDbl), val);
	}

	return value();
}
gstd::value DxScript::Func_Obj_DeleteValueR(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double keyDbl = argv[1].as_real();

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		obj->DeleteObjectValue(DxScriptObjectBase::GetKeyHashReal(keyDbl));
	}

	return value();
}
gstd::value DxScript::Func_Obj_IsValueExistsR(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double keyDbl = argv[1].as_real();

	bool res = false;

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		res = obj->IsObjectValueExists(DxScriptObjectBase::GetKeyHashReal(keyDbl));
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

	return script->CreateRealValue(countValue);
}

value DxScript::Func_Obj_GetType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	TypeObject res = TypeObject::OBJ_INVALID;

	DxScriptObjectBase* obj = dynamic_cast<DxScriptObjectBase*>(script->GetObjectPointer(id));
	if (obj) res = obj->GetObjectType();

	return script->CreateRealValue((int8_t)res);
}


//Dx関数：オブジェクト操作(RenderObject)
value DxScript::Func_ObjRender_SetX(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetX(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetY(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetY(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetZ(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		obj->SetX(argv[1].as_real());
		obj->SetY(argv[2].as_real());
		obj->SetZ(argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjRender_SetAngleX(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetAngleX(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetAngleY(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetAngleY(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetAngleZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetAngleZ(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetAngleXYZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		obj->SetAngleX(argv[1].as_real());
		obj->SetAngleY(argv[2].as_real());
		obj->SetAngleZ(argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjRender_SetScaleX(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetScaleX(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetScaleY(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetScaleY(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetScaleZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetScaleZ(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetScaleXYZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		obj->SetScaleX(argv[1].as_real());
		obj->SetScaleY(argv[2].as_real());
		obj->SetScaleZ(argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjRender_SetColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		if (argc == 4) {
			obj->SetColor(argv[1].as_int(), argv[2].as_int(), argv[3].as_int());
		}
		else {
			D3DXVECTOR4 vecColor = ColorAccess::ToVec4((D3DCOLOR)argv[1].as_int());
			obj->SetColor(vecColor.y, vecColor.z, vecColor.w);
		}
	}
	return value();
}
value DxScript::Func_ObjRender_SetColorHSV(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		D3DCOLOR color = 0xffffffff;
		ColorAccess::HSVtoRGB(color, argv[1].as_int(), argv[2].as_int(), argv[3].as_int());

		D3DXVECTOR4 vecColor = ColorAccess::ToVec4(color);
		obj->SetColor(vecColor.y, vecColor.z, vecColor.w);
	}
	return value();
}
value DxScript::Func_ObjRender_GetColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	D3DCOLOR color = 0xffffffff;
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) color = obj->color_;

	byte listColor[4];
	ColorAccess::ToByteArray(color, listColor);
	return script->CreateRealArrayValue((byte*)(listColor + 1), 3U);
}
value DxScript::Func_ObjRender_SetAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetAlpha(argv[1].as_int());
	return value();
}
value DxScript::Func_ObjRender_GetAlpha(script_machine* machine, int argc, const value* argv) {
	byte res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = ColorAccess::GetColorA(obj->color_);
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjRender_SetBlendType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetBlendType((BlendMode)argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_GetX(script_machine* machine, int argc, const value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->position_.x;
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjRender_GetY(script_machine* machine, int argc, const value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->position_.y;
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjRender_GetZ(script_machine* machine, int argc, const value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->position_.z;
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjRender_GetAngleX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->angle_.x;
	return script->CreateRealValue(Math::RadianToDegree(res));
}
gstd::value DxScript::Func_ObjRender_GetAngleY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->angle_.y;
	return script->CreateRealValue(Math::RadianToDegree(res));
}
gstd::value DxScript::Func_ObjRender_GetAngleZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->angle_.z;
	return script->CreateRealValue(Math::RadianToDegree(res));
}
gstd::value DxScript::Func_ObjRender_GetScaleX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->scale_.x;
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjRender_GetScaleY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->scale_.y;
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjRender_GetScaleZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	FLOAT res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->scale_.z;
	return script->CreateRealValue(res);
}

value DxScript::Func_ObjRender_SetZWrite(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->bZWrite_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_ObjRender_SetZTest(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->bZTest_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_ObjRender_SetFogEnable(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->bFogEnable_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_ObjRender_SetCullingMode(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->modeCulling_ = (D3DCULL)argv[1].as_int();
	return value();
}
value DxScript::Func_ObjRender_SetRelativeObject(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		int idRelative = argv[1].as_int();
		std::wstring nameBone = argv[2].as_string();
		auto objRelative = std::dynamic_pointer_cast<DxScriptRenderObject>(script->GetObject(idRelative));
		if (objRelative)
			obj->SetRelativeObject(objRelative, nameBone);
	}
	return value();
}
value DxScript::Func_ObjRender_SetPermitCamera(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
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
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetBlendType();
	return script->CreateRealValue((int)res);
}

value DxScript::Func_ObjRender_SetTextureFilterMin(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int type = argv[1].as_int();

	type = std::max(type, (int)D3DTEXF_NONE);
	type = std::min(type, (int)D3DTEXF_ANISOTROPIC);

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->filterMin_ = (D3DTEXTUREFILTERTYPE)type;

	return value();
}
value DxScript::Func_ObjRender_SetTextureFilterMag(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int type = argv[1].as_int();

	type = std::max(type, (int)D3DTEXF_NONE);
	type = std::min(type, (int)D3DTEXF_ANISOTROPIC);

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->filterMag_ = (D3DTEXTUREFILTERTYPE)type;

	return value();
}
value DxScript::Func_ObjRender_SetTextureFilterMip(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int type = argv[1].as_int();

	type = std::max(type, (int)D3DTEXF_NONE);
	type = std::min(type, (int)D3DTEXF_ANISOTROPIC);

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->filterMip_ = (D3DTEXTUREFILTERTYPE)type;

	return value();
}
value DxScript::Func_ObjRender_SetVertexShaderRenderingMode(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->bVertexShaderMode_ = argv[1].as_boolean();

	return value();
}
value DxScript::Func_ObjRender_SetLightingEnable(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
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
	int id = (int)argv[0].as_real();

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		if (auto objLight = obj->GetLightPointer()) {
			D3DCOLORVALUE color = {
				(float)argv[1].as_real() / 255.0f,
				(float)argv[2].as_real() / 255.0f,
				(float)argv[3].as_real() / 255.0f,
				0.0f,
			};
			objLight->SetColorDiffuse(color);
		}
	}

	return value();
}
value DxScript::Func_ObjRender_SetLightingSpecularColor(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		if (auto objLight = obj->GetLightPointer()) {
			D3DCOLORVALUE color = {
				(float)argv[1].as_real() / 255.0f,
				(float)argv[2].as_real() / 255.0f,
				(float)argv[3].as_real() / 255.0f,
				0.0f,
			};
			objLight->SetColorSpecular(color);
		}
	}

	return value();
}
value DxScript::Func_ObjRender_SetLightingAmbientColor(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		if (auto objLight = obj->GetLightPointer()) {
			D3DCOLORVALUE color = {
				(float)argv[1].as_real() / 255.0f,
				(float)argv[2].as_real() / 255.0f,
				(float)argv[3].as_real() / 255.0f,
				0.0f,
			};
			objLight->SetColorAmbient(color);
		}
	}

	return value();
}
value DxScript::Func_ObjRender_SetLightingDirection(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
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
	shared_ptr<DxScriptShaderObject> obj = std::make_shared<DxScriptShaderObject>();

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateRealValue(id);
}
gstd::value DxScript::Func_ObjShader_SetShaderF(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool res = false;
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		auto itr = script->mapShader_.find(path);
		if (itr != script->mapShader_.end()) {
			obj->SetShader(itr->second);
			res = true;
		}
		else {
			ShaderManager* manager = ShaderManager::GetBase();
			shared_ptr<Shader> shader = manager->CreateFromFile(path);
			obj->SetShader(shader);

			res = shader != nullptr;
			if (!res) {
				const std::wstring& error = manager->GetLastError();
				script->RaiseError(error);
			}
		}
	}
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_ObjShader_SetShaderO(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = false;

	int id1 = argv[0].as_int();
	DxScriptRenderObject* obj1 = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id1));
	if (obj1) {
		int id2 = argv[1].as_int();
		DxScriptRenderObject* obj2 = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id2));
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
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetShader(nullptr);
	return value();
}
gstd::value DxScript::Func_ObjShader_SetTechnique(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
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
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
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
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
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
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
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
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
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
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
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
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			std::wstring path = argv[2].as_string();
			path = PathProperty::GetUnique(path);

			auto itr = script->mapTexture_.find(path);
			if (itr != script->mapTexture_.end()) {
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

	shared_ptr<DxScriptPrimitiveObject> obj;
	if (type == TypeObject::OBJ_PRIMITIVE_2D) {
		obj = std::make_shared<DxScriptPrimitiveObject2D>();
	}
	else if (type == TypeObject::OBJ_SPRITE_2D) {
		obj = std::make_shared<DxScriptSpriteObject2D>();
	}
	else if (type == TypeObject::OBJ_SPRITE_LIST_2D) {
		obj = std::make_shared<DxScriptSpriteListObject2D>();
	}
	else if (type == TypeObject::OBJ_PRIMITIVE_3D) {
		obj = std::make_shared<DxScriptPrimitiveObject3D>();
	}
	else if (type == TypeObject::OBJ_SPRITE_3D) {
		obj = std::make_shared<DxScriptSpriteObject3D>();
	}
	else if (type == TypeObject::OBJ_TRAJECTORY_3D) {
		obj = std::make_shared<DxScriptTrajectoryObject3D>();
	}

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateRealValue(id);
}
value DxScript::Func_ObjPrimitive_SetPrimitiveType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetPrimitiveType((D3DPRIMITIVETYPE)argv[1].as_int());
	return value();
}
value DxScript::Func_ObjPrimitive_GetPrimitiveType(script_machine* machine, int argc, const value* argv) {
	int res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetPrimitiveType();
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjPrimitive_SetVertexCount(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetVertexCount(argv[1].as_int());
	return value();
}
value DxScript::Func_ObjPrimitive_SetTexture(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		auto itr = script->mapTexture_.find(path);
		if (itr != script->mapTexture_.end()) {
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
value DxScript::Func_ObjPrimitive_GetVertexCount(script_machine* machine, int argc, const value* argv) {
	size_t res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetVertexCount();
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjPrimitive_SetVertexPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetVertexPosition(argv[1].as_int(), argv[2].as_real(), argv[3].as_real(), argv[4].as_real());
	return value();
}
value DxScript::Func_ObjPrimitive_SetVertexUV(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetVertexUV(argv[1].as_int(), argv[2].as_real(), argv[3].as_real());
	return value();
}
gstd::value DxScript::Func_ObjPrimitive_SetVertexUVT(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Texture> texture = obj->GetTexture();
		if (texture) {
			int width = texture->GetWidth();
			int height = texture->GetHeight();
			obj->SetVertexUV(argv[1].as_int(), (float)argv[2].as_real() / width, (float)argv[3].as_real() / height);
		}
	}
	return value();
}
value DxScript::Func_ObjPrimitive_SetVertexColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		if (argc == 5) {
			obj->SetVertexColor(argv[1].as_int(), argv[2].as_int(), argv[3].as_int(), argv[4].as_int());
		}
		else {
			D3DXVECTOR4 vecColor = ColorAccess::ToVec4((D3DCOLOR)argv[2].as_int());
			obj->SetVertexColor(argv[1].as_int(), vecColor.y, vecColor.z, vecColor.w);
		}
	}
	return value();
}
value DxScript::Func_ObjPrimitive_SetVertexColorHSV(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		D3DCOLOR cHSV = 0xffffffff;
		ColorAccess::HSVtoRGB(cHSV, argv[2].as_int(), argv[3].as_int(), argv[4].as_int());

		byte listRGB[4];
		ColorAccess::ToByteArray(cHSV, listRGB);
		obj->SetVertexColor(argv[1].as_int(), listRGB[1], listRGB[2], listRGB[3]);
	}
	return value();
}
value DxScript::Func_ObjPrimitive_SetVertexAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetVertexAlpha(argv[1].as_int(), argv[2].as_int());
	return value();
}
value DxScript::Func_ObjPrimitive_GetVertexColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int index = argv[1].as_int();

	D3DCOLOR color = 0xffffffff;
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) color = obj->GetVertexColor(index);

	byte listRGB[4];
	ColorAccess::ToByteArray(color, listRGB);

	return script->CreateRealArrayValue((byte*)(listRGB + 1), 3);
}
value DxScript::Func_ObjPrimitive_GetVertexAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int index = argv[1].as_int();

	D3DCOLOR color = 0xffffffff;
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) color = obj->GetVertexColor(index);

	return script->CreateRealValue(ColorAccess::GetColorA(color));
}
value DxScript::Func_ObjPrimitive_GetVertexPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int index = argv[1].as_int();

	D3DXVECTOR3 pos = D3DXVECTOR3(0, 0, 0);
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		pos = obj->GetVertexPosition(index);

	float listPos[3] = { pos.x, pos.y, pos.z };
	return script->CreateRealArrayValue(listPos, 3U);
}
value DxScript::Func_ObjPrimitive_SetVertexIndex(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		value valArr = argv[1];
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
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject2D* obj = dynamic_cast<DxScriptSpriteObject2D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcSrc = {
			(double)argv[1].as_int(),
			(double)argv[2].as_int(),
			(double)argv[3].as_int(),
			(double)argv[4].as_int()
		};
		obj->GetSpritePointer()->SetSourceRect(rcSrc);
	}
	return value();
}
value DxScript::Func_ObjSprite2D_SetDestRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject2D* obj = dynamic_cast<DxScriptSpriteObject2D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcDest = {
			argv[1].as_real(),
			argv[2].as_real(),
			argv[3].as_real(),
			argv[4].as_real()
		};
		obj->GetSpritePointer()->SetDestinationRect(rcDest);
	}
	return value();
}
value DxScript::Func_ObjSprite2D_SetDestCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject2D* obj = dynamic_cast<DxScriptSpriteObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetSpritePointer()->SetDestinationCenter();
	return value();
}

//Dx関数：オブジェクト操作(SpriteList2D)
gstd::value DxScript::Func_ObjSpriteList2D_SetSourceRect(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcSrc = {
			(double)argv[1].as_int(),
			(double)argv[2].as_int(),
			(double)argv[3].as_int(),
			(double)argv[4].as_int()
		};
		obj->GetSpritePointer()->SetSourceRect(rcSrc);
	}
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_SetDestRect(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcDest = {
			argv[1].as_real(),
			argv[2].as_real(),
			argv[3].as_real(),
			argv[4].as_real()
		};
		obj->GetSpritePointer()->SetDestinationRect(rcDest);
	}
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_SetDestCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetSpritePointer()->SetDestinationCenter();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_AddVertex(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->AddVertex();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_CloseVertex(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->CloseVertex();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_ClearVertexCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetSpritePointer()->ClearVertexCount();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_SetAutoClearVertexCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetSpritePointer()->SetAutoClearVertex(argv[1].as_boolean());
	return value();
}

//Dx関数：オブジェクト操作(Sprite3D)
value DxScript::Func_ObjSprite3D_SetSourceRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject3D* obj = dynamic_cast<DxScriptSpriteObject3D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcSrc = {
			(double)argv[1].as_int(),
			(double)argv[2].as_int(),
			(double)argv[3].as_int(),
			(double)argv[4].as_int()
		};
		obj->GetSpritePointer()->SetSourceRect(rcSrc);
	}
	return value();
}
value DxScript::Func_ObjSprite3D_SetDestRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject3D* obj = dynamic_cast<DxScriptSpriteObject3D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcDest = {
			argv[1].as_real(),
			argv[2].as_real(),
			argv[3].as_real(),
			argv[4].as_real()
		};
		obj->GetSpritePointer()->SetDestinationRect(rcDest);
	}
	return value();
}
value DxScript::Func_ObjSprite3D_SetSourceDestRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject3D* obj = dynamic_cast<DxScriptSpriteObject3D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcSrc = {
			argv[1].as_real(),
			argv[2].as_real(),
			argv[3].as_real(),
			argv[4].as_real()
		};
		obj->GetSpritePointer()->SetSourceDestRect(rcSrc);
	}
	return value();
}
value DxScript::Func_ObjSprite3D_SetBillboard(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject3D* obj = dynamic_cast<DxScriptSpriteObject3D*>(script->GetObjectPointer(id));
	if (obj) {
		bool bEnable = argv[1].as_boolean();
		obj->GetSpritePointer()->SetBillboardEnable(bEnable);
	}
	return value();
}
//Dx関数：オブジェクト操作(TrajectoryObject3D)
value DxScript::Func_ObjTrajectory3D_SetInitialPoint(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTrajectoryObject3D* obj = dynamic_cast<DxScriptTrajectoryObject3D*>(script->GetObjectPointer(id));
	if (obj) {
		D3DXVECTOR3 pos1(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
		D3DXVECTOR3 pos2(argv[4].as_real(), argv[5].as_real(), argv[6].as_real());
		obj->GetObjectPointer()->SetInitialLine(pos1, pos2);
	}
	return value();
}
value DxScript::Func_ObjTrajectory3D_SetAlphaVariation(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTrajectoryObject3D* obj = dynamic_cast<DxScriptTrajectoryObject3D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetObjectPointer()->SetAlphaVariation(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjTrajectory3D_SetComplementCount(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTrajectoryObject3D* obj = dynamic_cast<DxScriptTrajectoryObject3D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetObjectPointer()->SetComplementCount(argv[1].as_int());
	return value();
}

//DxScriptParticleListObject
value DxScript::Func_ObjParticleList_Create(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	script->CheckRunInMainThread();
	TypeObject type = (TypeObject)argv[0].as_int();

	shared_ptr<DxScriptPrimitiveObject> obj;
	if (type == TypeObject::OBJ_PARTICLE_LIST_2D) {
		obj = std::make_shared<DxScriptParticleListObject2D>();
	}
	else if (type == TypeObject::OBJ_PARTICLE_LIST_3D) {
		obj = std::make_shared<DxScriptParticleListObject3D>();
	}

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateRealValue(id);
}
value DxScript::Func_ObjParticleList_SetPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstancePosition(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
	}
	return value();
}
template<size_t ID>
value DxScript::Func_ObjParticleList_SetScaleSingle(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceScaleSingle(ID, argv[1].as_real());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetScaleXYZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceScale(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
	}
	return value();
}
template<size_t ID>
value DxScript::Func_ObjParticleList_SetAngleSingle(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceAngleSingle(ID, argv[1].as_real());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetAngle(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceAngle(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceColorRGB(argv[1].as_int(), argv[2].as_int(), argv[3].as_int());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceAlpha(argv[1].as_int());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetExtraData(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceUserData(D3DXVECTOR3(argv[1].as_real(), argv[2].as_real(), argv[3].as_real()));
	}
	return value();
}
value DxScript::Func_ObjParticleList_AddInstance(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->AddInstance();
	}
	return value();
}
value DxScript::Func_ObjParticleList_ClearInstance(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->ClearInstance();
	}
	return value();
}

//Dx関数：オブジェクト操作(DxMesh)
value DxScript::Func_ObjMesh_Create(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	shared_ptr<DxScriptMeshObject> obj = shared_ptr<DxScriptMeshObject>(new DxScriptMeshObject());
	int id = ID_INVALID;
	if (obj) {
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateRealValue(id);
}
value DxScript::Func_ObjMesh_Load(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool res = false;

	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<DxMesh> mesh;
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		auto itr = script->mapMesh_.find(path);
		if (itr != script->mapMesh_.end()) {
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
	int id = (int)argv[0].as_real();
	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetColor(argv[1].as_int(), argv[2].as_int(), argv[3].as_int());
	return value();
}
value DxScript::Func_ObjMesh_SetAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetAlpha(argv[1].as_int());
	return value();
}
value DxScript::Func_ObjMesh_SetAnimation(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
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
	int id = (int)argv[0].as_real();
	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
	if (obj) {
		bool bEnable = argv[1].as_boolean();
		obj->bCoordinate2D_ = bEnable;
	}
	return value();
}
value DxScript::Func_ObjMesh_GetPath(script_machine* machine, int argc, const value* argv) {
	std::wstring res;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
	if (obj) {
		DxMesh* mesh = obj->mesh_.get();
		if (mesh) res = mesh->GetPath();
	}
	return script->CreateStringValue(res);
}

//Dx関数：オブジェクト操作(DxText)
value DxScript::Func_ObjText_Create(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	shared_ptr<DxScriptTextObject> obj = shared_ptr<DxScriptTextObject>(new DxScriptTextObject());
	int id = ID_INVALID;
	if (obj) {
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateRealValue(id);
}
value DxScript::Func_ObjText_SetText(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring wstr = argv[1].as_string();
		obj->SetText(wstr);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring wstr = argv[1].as_string();
		obj->SetFontType(wstr);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontSize(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int size = argv[1].as_int();
		obj->SetFontSize(size);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBold(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		bool bBold = argv[1].as_boolean();
		obj->SetFontWeight(bBold ? FW_BOLD : FW_NORMAL);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontWeight(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int weight = argv[1].as_int();
		if (weight < 0) weight = 0;
		else if (weight > 1000) weight = 1000;
		obj->SetFontWeight(weight);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontColorTop(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		byte r = ColorAccess::ClampColorRet(argv[1].as_int());
		byte g = ColorAccess::ClampColorRet(argv[2].as_int());
		byte b = ColorAccess::ClampColorRet(argv[3].as_int());
		obj->SetFontColorTop(r, g, b);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontColorBottom(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		byte r = ColorAccess::ClampColorRet(argv[1].as_int());
		byte g = ColorAccess::ClampColorRet(argv[2].as_int());
		byte b = ColorAccess::ClampColorRet(argv[3].as_int());
		obj->SetFontColorBottom(r, g, b);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBorderWidth(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int width = argv[1].as_int();
		obj->SetFontBorderWidth(width);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBorderType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int type = argv[1].as_int();
		obj->SetFontBorderType(type);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBorderColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		byte r = ColorAccess::ClampColorRet(argv[1].as_int());
		byte g = ColorAccess::ClampColorRet(argv[2].as_int());
		byte b = ColorAccess::ClampColorRet(argv[3].as_int());
		obj->SetFontBorderColor(r, g, b);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontCharacterSet(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int cSet = argv[1].as_int();
		ColorAccess::ClampColor(cSet);	//Also clamps to [0-255], so it works too
		obj->SetCharset((BYTE)cSet);
	}
	return value();
}
value DxScript::Func_ObjText_SetMaxWidth(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int width = argv[1].as_int();
		obj->SetMaxWidth(width);
	}
	return value();
}
value DxScript::Func_ObjText_SetMaxHeight(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int height = argv[1].as_int();
		obj->SetMaxHeight(height);
	}
	return value();
}
value DxScript::Func_ObjText_SetLinePitch(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		float pitch = argv[1].as_real();
		obj->SetLinePitch(pitch);
	}
	return value();
}
value DxScript::Func_ObjText_SetSidePitch(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		float pitch = argv[1].as_real();
		obj->SetSidePitch(pitch);
	}
	return value();
}
value DxScript::Func_ObjText_SetVertexColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		byte a = ColorAccess::ClampColorRet(argv[1].as_int());
		byte r = ColorAccess::ClampColorRet(argv[2].as_int());
		byte g = ColorAccess::ClampColorRet(argv[3].as_int());
		byte b = ColorAccess::ClampColorRet(argv[4].as_int());
		obj->SetVertexColor(D3DCOLOR_ARGB(a, r, g, b));
	}
	return value();
}
gstd::value DxScript::Func_ObjText_SetTransCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		float centerX = argv[1].as_real();
		float centerY = argv[2].as_real();
		obj->center_ = D3DXVECTOR2(centerX, centerY);
	}
	return value();
}
gstd::value DxScript::Func_ObjText_SetAutoTransCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->bAutoCenter_ = argv[1].as_boolean();
	return value();
}
gstd::value DxScript::Func_ObjText_SetHorizontalAlignment(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int align = argv[1].as_int();
		obj->SetHorizontalAlignment(align);
	}
	return value();
}
gstd::value DxScript::Func_ObjText_SetSyntacticAnalysis(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetSyntacticAnalysis(argv[1].as_boolean());
	return value();
}
value DxScript::Func_ObjText_GetTextLength(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	size_t res = 0;
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		const std::wstring& text = obj->GetText();
		res = StringUtility::CountAsciiSizeCharacter(text);
	}
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjText_GetTextLengthCU(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int res = 0;
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::vector<int> listCount = obj->GetTextCountCU();
		for (size_t iLine = 0; iLine < listCount.size(); ++iLine) {
			res += listCount[iLine];
		}
	}
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjText_GetTextLengthCUL(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	std::vector<int> listCountD;
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::vector<int> listCount = obj->GetTextCountCU();
		for (size_t iLine = 0; iLine < listCount.size(); ++iLine)
			listCountD.push_back(listCount[iLine]);
	}
	return script->CreateRealArrayValue(listCountD);
}
value DxScript::Func_ObjText_GetTotalWidth(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int res = 0;
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetTotalWidth();
	return script->CreateRealValue(res);
}
value DxScript::Func_ObjText_GetTotalHeight(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int res = 0;
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetTotalHeight();
	return script->CreateRealValue(res);
}

//Dx関数：音声操作(DxSoundObject)
gstd::value DxScript::Func_ObjSound_Create(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	shared_ptr<DxSoundObject> obj = shared_ptr<DxSoundObject>(new DxSoundObject());
	obj->manager_ = script->objManager_.get();

	int id = script->AddObject(obj);
	return script->CreateRealValue(id);
}
gstd::value DxScript::Func_ObjSound_Load(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool bLoad = false;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);
		bLoad = obj->Load(path);
	}
	return script->CreateBooleanValue(bLoad);
}
gstd::value DxScript::Func_ObjSound_Play(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			//obj->Play();
			script->GetObjectManager()->ReserveSound(player, obj->GetStyle());
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_Stop(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
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
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			player->SetVolumeRate(argv[1].as_real());
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetPanRate(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			player->SetPanRate(argv[1].as_real());
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetFade(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			player->SetFade(argv[1].as_real());
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetLoopEnable(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			SoundPlayer::PlayStyle& style = obj->GetStyle();
			style.SetLoopEnable(argv[1].as_boolean());
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetLoopTime(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			SoundPlayer::PlayStyle& style = obj->GetStyle();
			style.SetLoopStartTime(argv[1].as_real());
			style.SetLoopEndTime(argv[2].as_real());
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetLoopSampleCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			WAVEFORMATEX* fmt = player->GetWaveFormat();
			double startTime = argv[1].as_real() / (double)fmt->nSamplesPerSec;
			double endTime = argv[2].as_real() / (double)fmt->nSamplesPerSec;;
			SoundPlayer::PlayStyle& style = obj->GetStyle();
			style.SetLoopStartTime(startTime);
			style.SetLoopEndTime(endTime);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_Seek(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			//if (player->IsPlaying()) {
			player->Seek(argv[1].as_real());
			player->ResetStreamForSeek();
		//}
		//else
		//	obj->GetStyle().SetStartTime(seekTime);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SeekSampleCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			WAVEFORMATEX* fmt = player->GetWaveFormat();
			//if (player->IsPlaying()) {
			player->Seek(argv[1].as_int());
			player->ResetStreamForSeek();
		//}
		//else
		//	obj->GetStyle().SetStartTime(seekSample / (double)fmt->nSamplesPerSec);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetRestartEnable(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			SoundPlayer::PlayStyle& style = obj->GetStyle();
			style.SetRestart(argv[1].as_boolean());
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetSoundDivision(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			player->SetSoundDivision(argv[1].as_int());
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_IsPlaying(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool bPlay = false;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			bPlay = player->IsPlaying();
	}
	return script->CreateBooleanValue(bPlay);
}
gstd::value DxScript::Func_ObjSound_GetVolumeRate(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double rate = 0;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player)
			rate = player->GetVolumeRate();
	}
	return script->CreateRealValue(rate);
}
gstd::value DxScript::Func_ObjSound_GetWavePosition(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double posSec = 0;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			DWORD pos = player->GetCurrentPosition();
			posSec = (double)pos / player->GetWaveFormat()->nAvgBytesPerSec;
		}
	}
	return script->CreateRealValue(posSec);
}
gstd::value DxScript::Func_ObjSound_GetWavePositionSampleCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DWORD posSamp = 0;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			DWORD pos = player->GetCurrentPosition();
			posSamp = pos / player->GetWaveFormat()->nBlockAlign;
		}
	}
	return script->CreateRealValue(posSamp);
}
gstd::value DxScript::Func_ObjSound_GetTotalLength(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double posSec = 0;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			DWORD pos = player->GetTotalAudioSize();
			posSec = (double)pos / player->GetWaveFormat()->nAvgBytesPerSec;
		}
	}
	return script->CreateRealValue(posSec);
}
gstd::value DxScript::Func_ObjSound_GetTotalLengthSampleCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DWORD posSamp = 0;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			DWORD pos = player->GetTotalAudioSize();
			posSamp = pos / player->GetWaveFormat()->nBlockAlign;
		}
	}
	return script->CreateRealValue(posSamp);
}

//Dx関数：ファイル操作(DxFileObject)
gstd::value DxScript::Func_ObjFile_Create(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	//	script->CheckRunInMainThread();
	TypeObject type = (TypeObject)argv[0].as_int();

	shared_ptr<DxFileObject> obj;
	if (type == TypeObject::OBJ_FILE_TEXT) {
		obj = shared_ptr<DxFileObject>(new DxTextFileObject());
	}
	else if (type == TypeObject::OBJ_FILE_BINARY) {
		obj = shared_ptr<DxFileObject>(new DxBinaryFileObject());
	}

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return script->CreateRealValue(id);
}
gstd::value DxScript::Func_ObjFile_Open(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool res = false;
	DxFileObject* obj = dynamic_cast<DxFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader && reader->Open()) {
			obj->isArchived_ = reader->IsArchived();
			res = obj->OpenR(reader);
		}
	}
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_ObjFile_OpenNW(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool res = false;
	DxFileObject* obj = dynamic_cast<DxFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		obj->isArchived_ = false;

		//Try finding a managed file object
		ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader && reader->Open()) {
			//Cannot write to an archived file, fall back to read-only permission
			ManagedFileReader* ptrManaged = dynamic_cast<ManagedFileReader*>(reader.GetPointer());
			if (ptrManaged->IsArchived()) {
				obj->isArchived_ = true;
				res = obj->OpenR(reader);
			}
			else res = obj->OpenRW(path);
		}
		else res = obj->OpenRW(path);
	}
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_ObjFile_Store(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool res = false;
	DxFileObject* obj = dynamic_cast<DxFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->isArchived_)
		res = obj->Store();
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_ObjFile_GetSize(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int res = 0;
	DxFileObject* obj = dynamic_cast<DxFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<File> file = obj->GetFile();
		res = file != nullptr ? file->GetSize() : 0;
	}
	return script->CreateRealValue(res);
}

//Dx関数：ファイル操作(DxTextFileObject)
gstd::value DxScript::Func_ObjFileT_GetLineCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int res = 0;
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetLineCount();
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileT_GetLineText(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr) return script->CreateStringValue(std::wstring());

	int line = argv[1].as_int();
	std::wstring res = obj->GetLineAsWString(line);
	return script->CreateStringValue(res);
}
gstd::value DxScript::Func_ObjFileT_SetLineText(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));

	//Cannot write to an archived file
	if (obj && !obj->isArchived_) {
		int line = argv[1].as_int();
		std::wstring text = argv[2].as_string();
		obj->SetLineAsWString(text, line);
	}

	return value();
}
gstd::value DxScript::Func_ObjFileT_SplitLineText(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr) return script->CreateStringValue(std::wstring());

	int pos = argv[1].as_int();
	std::wstring delim = argv[2].as_string();
	std::wstring line = obj->GetLineAsWString(pos);
	std::vector<std::wstring> list = StringUtility::Split(line, delim);

	return script->CreateStringArrayValue(list);
}
gstd::value DxScript::Func_ObjFileT_AddLine(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));

	//Cannot write to an archived file
	if (obj && !obj->isArchived_) obj->AddLine(argv[1].as_string());

	return value();
}
gstd::value DxScript::Func_ObjFileT_ClearLine(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));

	//Cannot write to an archived file
	if (obj && !obj->isArchived_) obj->ClearLine();

	return value();
}

//Dx関数：ファイル操作(DxBinaryFileObject)
gstd::value DxScript::Func_ObjFileB_SetByteOrder(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		int order = argv[1].as_int();
		obj->SetByteOrder(order);
	}
	return gstd::value();
}
gstd::value DxScript::Func_ObjFileB_SetCharacterCode(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		byte code = (byte)argv[1].as_int();
		obj->SetCodePage(code);
	}
	return gstd::value();
}
gstd::value DxScript::Func_ObjFileB_GetPointer(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	size_t res = 0;
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
		res = buffer->GetOffset();
	}
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_Seek(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		int pos = argv[1].as_int();
		gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
		buffer->Seek(pos);
	}
	return gstd::value();
}
gstd::value DxScript::Func_ObjFileB_ReadBoolean(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr) return script->CreateBooleanValue(false);
	if (!obj->IsReadableSize(1))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	bool res = buffer->ReadBoolean();
	return script->CreateBooleanValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadByte(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr) return script->CreateRealValue(0);
	if (!obj->IsReadableSize(1))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	char res = buffer->ReadCharacter();
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadShort(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr) return script->CreateRealValue(0);
	if (!obj->IsReadableSize(2))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	short bv = buffer->ReadShort();
	if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG)
		ByteOrder::Reverse(&bv, sizeof(bv));

	return script->CreateRealValue(bv);
}
gstd::value DxScript::Func_ObjFileB_ReadInteger(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr) return script->CreateRealValue(0);
	if (!obj->IsReadableSize(4))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	int bv = buffer->ReadInteger();
	if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG)
		ByteOrder::Reverse(&bv, sizeof(bv));

	double res = bv;
	return script->CreateRealValue(bv);
}
gstd::value DxScript::Func_ObjFileB_ReadLong(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr) return script->CreateRealValue(0);
	if (!obj->IsReadableSize(8))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	int64_t bv = buffer->ReadInteger64();
	if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG)
		ByteOrder::Reverse(&bv, sizeof(bv));

	return script->CreateRealValue(bv);
}
gstd::value DxScript::Func_ObjFileB_ReadFloat(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr) return script->CreateRealValue(0);
	if (!obj->IsReadableSize(4))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	float res = 0;
	if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG) {
		int bv = buffer->ReadInteger();
		ByteOrder::Reverse(&bv, sizeof(bv));
		res = (float&)bv;
	}
	else
		res = buffer->ReadFloat();

	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadDouble(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr) return script->CreateRealValue(0);
	if (!obj->IsReadableSize(8))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	double res = 0;
	if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG) {
		int64_t bv = buffer->ReadInteger64();
		ByteOrder::Reverse(&bv, sizeof(bv));
		res = (double&)bv;
	}
	else
		res = buffer->ReadDouble();

	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_ReadString(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr) return script->CreateStringValue(std::wstring());

	size_t readSize = (size_t)argv[1].as_real();
	if (!obj->IsReadableSize(readSize))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	std::vector<byte> data;
	data.resize(readSize);

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	buffer->Read(&data[0], readSize);

	std::wstring res;
	int code = obj->GetCodePage();
	if (code == CODE_ACP || code == CODE_UTF8) {
		std::string str;
		str.resize(readSize);
		memcpy(&str[0], &data[0], readSize);

		res = StringUtility::ConvertMultiToWide(str, code == CODE_UTF8 ? CP_UTF8 : CP_ACP);
	}
	else if (code == CODE_UTF16LE || code == CODE_UTF16BE) {
		size_t strSize = readSize / 2 * 2;
		size_t wsize = strSize / 2;

		res.resize(wsize);
		memcpy(&res[0], &data[0], readSize);

		if (code == CODE_UTF16BE) {
			for (auto itr = res.begin(); itr != res.end(); ++itr) {
				wchar_t ch = *itr;
				*itr = (ch >> 8) | (ch << 8);
			}
		}
	}

	return script->CreateStringValue(res);
}

gstd::value DxScript::Func_ObjFileB_WriteBoolean(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		bool data = argv[1].as_boolean();
		res = obj->GetBuffer()->Write(&data, sizeof(bool));
	}
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteByte(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		byte data = (byte)argv[1].as_int();
		res = obj->GetBuffer()->Write(&data, sizeof(byte));
	}
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteShort(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		int16_t data = (int16_t)argv[1].as_int();
		res = obj->GetBuffer()->Write(&data, sizeof(int16_t));
	}
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteInteger(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		int32_t data = (int32_t)argv[1].as_int();
		res = obj->GetBuffer()->Write(&data, sizeof(int32_t));
	}
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteLong(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		int64_t data = (int64_t)argv[1].as_int();
		res = obj->GetBuffer()->Write(&data, sizeof(int64_t));
	}
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteFloat(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		float data = argv[1].as_real();
		res = obj->GetBuffer()->Write(&data, sizeof(float));
	}
	return script->CreateRealValue(res);
}
gstd::value DxScript::Func_ObjFileB_WriteDouble(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		double data = argv[1].as_real();
		res = obj->GetBuffer()->Write(&data, sizeof(double));
	}
	return script->CreateRealValue(res);
}

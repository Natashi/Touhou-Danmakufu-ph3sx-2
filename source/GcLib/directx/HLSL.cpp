#include "source/GcLib/pch.h"

#include "DirectGraphics.hpp"
#include "HLSL.hpp"

using namespace gstd;

namespace directx {
	const std::string ShaderSource::nameRender2D_ = "_HLSL_INTERNAL_RENDER2D";
	const std::string ShaderSource::sourceRender2D_ =
		"sampler samp0_ : register(s0);"
		"float4x4 g_mWorld : WORLD : register(c0);"
		"float4x4 g_mViewProj : VIEWPROJECTION : register(c4);"
		"float4 g_vColor : ICOLOR : register(c9);"

		"struct VS_INPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
		"};"
		"struct VS_OUTPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
		"};"
		"struct PS_OUTPUT {"
			"float4 color : COLOR0;"
		"};"

		"VS_OUTPUT mainVS(VS_INPUT inVs) {"
			"VS_OUTPUT outVs;"
			
			"outVs.diffuse = inVs.diffuse * g_vColor;"
			"outVs.texCoord = inVs.texCoord;"
			"outVs.position = mul(inVs.position, g_mWorld);"
			"outVs.position = mul(outVs.position, g_mViewProj);"
			"outVs.position.z = 1.0f;"

			"return outVs;"
		"}"

		"float4 mainPS(VS_OUTPUT inPs) : COLOR0 {"
			"return tex2D(samp0_, inPs.texCoord) * inPs.diffuse;"
		"}"
		"float4 mainPS_inv(VS_OUTPUT inPs) : COLOR0 {"
			"float4 color = tex2D(samp0_, inPs.texCoord);"
			"color.rgb = 1.0f - color.rgb;"

			"return color * inPs.diffuse;"
		"}"

		"technique Render {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 mainVS();"
				"PixelShader = compile ps_2_0 mainPS();"
			"}"
		"}"
		"technique RenderInv {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 mainVS();"
				"PixelShader = compile ps_2_0 mainPS_inv();"
			"}"
		"}";

	const std::string ShaderSource::nameRender3D_ = "_HLSL_INTERNAL_RENDER3D";
	const std::string ShaderSource::sourceRender3D_ =
		"sampler samp0_ : register(s0);"
		"static const int MAX_MATRICES = 30;"
		"int countMatrixTransform;"
		"float4x4 matTransform[MAX_MATRICES];"
		"float4x4 matViewProj : VIEWPROJECTION;"

		"bool useFog;"
		"bool useTexture;"

		"float3 fogColor;"
		"float2 fogPos;"

		"struct VS_INPUT {"
			"float3 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
		"};"
		"struct VS_OUTPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
			"float fog : FOG;"
		"};"

		"VS_OUTPUT mainVS(VS_INPUT inVs) {"
			"VS_OUTPUT outVs = (VS_OUTPUT)0;"

			"outVs.position = float4(inVs.position, 1.0f);"
			"for (int i = 0; i < countMatrixTransform; i++) {"
				"outVs.position = mul(outVs.position, matTransform[i]);"
			"}"

			"outVs.diffuse = inVs.diffuse;"
			"outVs.texCoord = inVs.texCoord;"

			"outVs.fog = saturate((outVs.position.z - fogPos.x) / (fogPos.y - fogPos.x));"
			"outVs.position = mul(outVs.position, matViewProj);"

			"return outVs;"
		"}"

		"float4 mainPS(VS_OUTPUT inPs) : COLOR0 {"
			"float4 ret = inPs.diffuse;"
			"if (useTexture)"
				"ret *= tex2D(samp0_, inPs.texCoord);"
			"if (useFog)"
				"ret.rgb = smoothstep(ret.rgb, fogColor.rgb, inPs.fog);"
			"return ret;"
		"}"
		
		"technique Render {"
			"pass P0 {"
				"ZENABLE = true;"
				"VertexShader = compile vs_3_0 mainVS();"
				"PixelShader = compile ps_3_0 mainPS();"
			"}"
		"}";

	const std::string ShaderSource::nameHwInstance2D_ = "_HLSL_INTERNAL_HW_INST_2D";
	const std::string ShaderSource::sourceHwInstance2D_ =
		"sampler samp0_ : register(s0);"
		"float4x4 g_mWorldViewProj : WORLDVIEWPROJ : register(c0);"

		"struct VS_INPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
			
			"float4 i_color : COLOR1;"
			"float4 i_xyzpos_xsc : TEXCOORD1;"
			"float4 i_yzsc_xyang : TEXCOORD2;"
			"float4 i_zang_usdat : TEXCOORD3;"
		"};"
		"struct VS_OUTPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
		"};"

		"VS_OUTPUT mainVS(VS_INPUT inVs) {"
			"VS_OUTPUT outVs;"
			
			"float3 t_scale = float3(inVs.i_xyzpos_xsc.w, inVs.i_yzsc_xyang.xy);"

			"float2 ax = float2(sin(inVs.i_yzsc_xyang.z), cos(inVs.i_yzsc_xyang.z));"
			"float2 ay = float2(sin(inVs.i_yzsc_xyang.w), cos(inVs.i_yzsc_xyang.w));"
			"float2 az = float2(sin(inVs.i_zang_usdat.x), cos(inVs.i_zang_usdat.x));"

			"float4x4 matInstance = float4x4("
				"float4("
					"t_scale.x * (ay.y * az.y - ax.x * ay.x * az.x),"
					"t_scale.x * (-ax.y * az.x),"
					"t_scale.x * (ay.x * az.y + ax.x * ay.y * az.x),"
					"0"
				"),"
				"float4("
					"t_scale.y * (ay.y * az.x + ax.x * ay.x * az.y),"
					"t_scale.y * (ax.y * az.y),"
					"t_scale.y * (ay.x * az.x - ax.x * ay.y * az.y),"
					"0"
				"),"
				"float4("
					"t_scale.z * (-ax.y * ay.x),"
					"t_scale.z * (ax.x),"
					"t_scale.z * (ax.y * ay.y),"
					"0"
				"),"
				"float4(inVs.i_xyzpos_xsc.x, inVs.i_xyzpos_xsc.y, inVs.i_xyzpos_xsc.z, 1)"
			");"

			"outVs.diffuse = inVs.diffuse * inVs.i_color;"
			"outVs.texCoord = inVs.texCoord;"
			"outVs.position = mul(inVs.position, matInstance);"
			"outVs.position = mul(outVs.position, g_mWorldViewProj);"
			"outVs.position.z = 1.0f;"

			"return outVs;"
		"}"

		"float4 mainPS(VS_OUTPUT inPs) : COLOR0 {"
			"return (tex2D(samp0_, inPs.texCoord) * inPs.diffuse);"
		"}"
		"float4 mainPS_inv(VS_OUTPUT inPs) : COLOR0 {"
			"float4 color = tex2D(samp0_, inPs.texCoord);"
			"color.rgb = 1.0f - color.rgb;"
			"return (color * inPs.diffuse);"
		"}"
		"float4 mainPS_textureless(VS_OUTPUT inPs) : COLOR0 {"
			"return inPs.diffuse;"
		"}"

		"technique Render {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 mainVS();"
				"PixelShader = compile ps_2_0 mainPS();"
			"}"
		"}"
		"technique RenderInv {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 mainVS();"
				"PixelShader = compile ps_2_0 mainPS_inv();"
			"}"
		"}"
		"technique RenderNoTexture {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 mainVS();"
				"PixelShader = compile ps_2_0 mainPS_textureless();"
			"}"
		"}";

	const std::string ShaderSource::nameHwInstance3D_ = "_HLSL_INTERNAL_HW_INST_3D";
	const std::string ShaderSource::sourceHwInstance3D_ =
		"sampler samp0_ : register(s0);"
		"float4x4 g_mCamera : WORLD : register(c0);"
		"float4x4 g_mView : VIEW : register(c4);"
		"float4x4 g_mProjection : PROJECTION : register(c8);"
		"bool g_bUseFog : FOGENABLE : register(b0) = false;"
		"float4 g_vFogColor : FOGCOLOR : register(c12) = float4(0.0f, 0.0f, 0.0f, 0.0f);"
		"float2 g_vFogDist : FOGDIST : register(c13) = float2(0.0f, 256.0f);"

		"struct VS_INPUT {"
			"float3 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
			
			"float4 i_color : COLOR1;"
			"float4 i_xyzpos_xsc : TEXCOORD1;"
			"float4 i_yzsc_xyang : TEXCOORD2;"
			"float4 i_zang_usdat : TEXCOORD3;"
		"};"
		"struct VS_OUTPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
			"float fogBlend : FOG;"
		"};"

		"VS_OUTPUT mainVS(VS_INPUT inVs) {"
			"VS_OUTPUT outVs;"
			
			"float3 t_scale = float3(inVs.i_xyzpos_xsc.w, inVs.i_yzsc_xyang.xy);"

			"float2 ax = float2(sin(inVs.i_yzsc_xyang.z), cos(inVs.i_yzsc_xyang.z));"
			"float2 ay = float2(sin(inVs.i_yzsc_xyang.w), cos(inVs.i_yzsc_xyang.w));"
			"float2 az = float2(sin(inVs.i_zang_usdat.x), cos(inVs.i_zang_usdat.x));"

			"float4x4 matInstance = float4x4("
				"float4("
					"t_scale.x * (ay.y * az.y - ax.x * ay.x * az.x),"
					"t_scale.x * (-ax.y * az.x),"
					"t_scale.x * (ay.x * az.y + ax.x * ay.y * az.x),"
					"0"
				"),"
				"float4("
					"t_scale.y * (ay.y * az.x + ax.x * ay.x * az.y),"
					"t_scale.y * (ax.y * az.y),"
					"t_scale.y * (ay.x * az.x - ax.x * ay.y * az.y),"
					"0"
				"),"
				"float4("
					"t_scale.z * (-ax.y * ay.x),"
					"t_scale.z * (ax.x),"
					"t_scale.z * (ax.y * ay.y),"
					"0"
				"),"
				"float4(0, 0, 0, 1)"
			");"

			"outVs.diffuse = inVs.diffuse * inVs.i_color;"
			"outVs.texCoord = inVs.texCoord;"

			"outVs.position = mul(float4(inVs.position, 1.0f), matInstance);"
			//"outVs.position.w = outVs.position.z;"
			"outVs.position = mul(outVs.position, g_mCamera);"
			"outVs.position.xyz += inVs.i_xyzpos_xsc.xyz;"
			"outVs.position = mul(outVs.position, g_mView);"

			"if (g_bUseFog) outVs.fogBlend = "
				"saturate((g_vFogDist.y - outVs.position.z) / (g_vFogDist.y - g_vFogDist.x));"
			
			"outVs.position = mul(outVs.position, g_mProjection);"

			"return outVs;"
		"}"

		"float4 mainPS(VS_OUTPUT inPs) : COLOR0 {"
			"float4 color = tex2D(samp0_, inPs.texCoord) * inPs.diffuse;"
			"if (g_bUseFog)"
				"color.rgb = lerp(g_vFogColor.rgb, color.rgb, inPs.fogBlend);"
			"return color;"
		"}"
		"float4 mainPS_inv(VS_OUTPUT inPs) : COLOR0 {"
			"float4 color = tex2D(samp0_, inPs.texCoord);"
			"color.rgb = (1.0f - color.rgb) *  inPs.diffuse;"
			"if (g_bUseFog)"
				"color.rgb = lerp(g_vFogColor.rgb, color.rgb, inPs.fogBlend);"
			"return color;"
		"}"
		"float4 mainPS_textureless(VS_OUTPUT inPs) : COLOR0 {"
			"float4 color = inPs.diffuse;"
			"if (g_bUseFog)"
				"color.rgb = lerp(g_vFogColor.rgb, color.rgb, inPs.fogBlend);"
			"return color;"
		"}"

		"technique Render {"
			"pass P0 {"
				"VertexShader = compile vs_3_0 mainVS();"
				"PixelShader = compile ps_3_0 mainPS();"
			"}"
		"}"
		"technique RenderInv {"
			"pass P0 {"
				"VertexShader = compile vs_3_0 mainVS();"
				"PixelShader = compile ps_3_0 mainPS_inv();"
			"}"
		"}"
		"technique RenderNoTexture {"
			"pass P0 {"
				"VertexShader = compile vs_3_0 mainVS();"
				"PixelShader = compile ps_3_0 mainPS_textureless();"
			"}"
		"}";

	const std::string ShaderSource::nameIntersectVisual1_ = "_HLSL_INTERNAL_INTVISUAL_A";
	const std::string ShaderSource::sourceIntersectVisual1_ = 
		"sampler samp0_ : register(s0);"
		"float4x4 g_mWorldViewProj : WORLDVIEWPROJ : register(c0);"

		"struct VS_INPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
			
			"float4 i_color : COLOR1;"
			"float4 i_xyzpos_xsc : TEXCOORD1;"
			"float4 i_yzsc_xyang : TEXCOORD2;"
			"float4 i_zang_usdat : TEXCOORD3;"
		"};"
		"struct VS_OUTPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
		"};"

		"VS_OUTPUT mainVS(VS_INPUT inVs) {"
			"VS_OUTPUT outVs;"
			
			"float t_scale = inVs.i_xyzpos_xsc.w;"

			"float4x4 matInstance = float4x4("
				"float4(t_scale, 0, 0, 0),"
				"float4(0, t_scale, 0, 0),"
				"(float4)0,"
				"float4(inVs.i_xyzpos_xsc.xyz, 1)"
			");"

			"outVs.diffuse = inVs.diffuse * inVs.i_color;"
			"outVs.position = mul(inVs.position, matInstance);"
			"outVs.position = mul(outVs.position, g_mWorldViewProj);"
			"outVs.position.z = 1.0f;"

			"return outVs;"
		"}"

		"float4 mainPS(VS_OUTPUT inPs) : COLOR0 {"
			"return inPs.diffuse;"
		"}"

		"technique Render {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 mainVS();"
				"PixelShader = compile ps_2_0 mainPS();"
			"}"
		"}";

	const std::string ShaderSource::nameIntersectVisual2_ = "_HLSL_INTERNAL_INTVISUAL_B";
	const std::string ShaderSource::sourceIntersectVisual2_ = 
		"sampler samp0_ : register(s0);"
		"float4x4 g_mWorld : WORLD : register(c0);"
		"float4x4 g_ViewProj : VIEWPROJECTION : register(c4);"

		"struct VS_INPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
		"};"
		"struct VS_OUTPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
		"};"

		"VS_OUTPUT mainVS(VS_INPUT inVs) {"
			"VS_OUTPUT outVs;"

			"outVs.diffuse = inVs.diffuse;"
			"outVs.position = mul(inVs.position, g_mWorld);"
			"outVs.position = mul(outVs.position, g_ViewProj);"
			"outVs.position.z = 1.0f;"

			"return outVs;"
		"}"

		"float4 mainPS(VS_OUTPUT inPs) : COLOR0 {"
			"return inPs.diffuse;"
		"}"

		"technique Render {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 mainVS();"
				"PixelShader = compile ps_2_0 mainPS();"
			"}"
		"}";

	//*******************************************************************
	//RenderShaderLibrary
	//*******************************************************************
	RenderShaderLibrary::RenderShaderLibrary() {
		Initialize();
	}
	RenderShaderLibrary::~RenderShaderLibrary() {
		Release();
	}

	void RenderShaderLibrary::Initialize() {
		ID3DXBuffer* error = nullptr;

		DirectGraphics* graphics = DirectGraphics::GetBase();
		IDirect3DDevice9* device = graphics->GetDevice();

		HRESULT hr = S_OK;

		{
			//<source, name>
			std::vector<std::pair<const std::string*, const std::string*>> listCreate = {
				std::make_pair(&ShaderSource::sourceRender2D_, &ShaderSource::nameRender2D_),
				std::make_pair(&ShaderSource::sourceHwInstance2D_, &ShaderSource::nameHwInstance2D_),
				std::make_pair(&ShaderSource::sourceHwInstance3D_, &ShaderSource::nameHwInstance3D_),
				std::make_pair(&ShaderSource::sourceIntersectVisual1_, &ShaderSource::nameIntersectVisual1_),
				std::make_pair(&ShaderSource::sourceIntersectVisual2_, &ShaderSource::nameIntersectVisual2_)
			};
			listEffect_.resize(listCreate.size(), nullptr);
			for (size_t iEff = 0U; iEff < listCreate.size(); ++iEff) {
				const std::string* source = listCreate[iEff].first;
				const std::string* name = listCreate[iEff].second;

				hr = D3DXCreateEffect(device, source->c_str(), source->size(), nullptr, nullptr, 0,
					nullptr, &listEffect_[iEff], &error);
				if (FAILED(hr)) {
					const char* strCompileError = "unknown error";
					if (error)
						strCompileError = reinterpret_cast<const char*>(error->GetBufferPointer());
					std::string err = StringUtility::Format(
						"RenderShaderLibrary: Shader compile failed. [%s]\r\n\t%s\r\n\t%s",
						name->c_str(), DXGetErrorStringA(hr), strCompileError);
					throw gstd::wexception(err);
				}
			}
		}
		if (listEffect_[0])
			listEffect_[0]->SetTechnique("Render");

		{
			std::vector<std::pair<const D3DVERTEXELEMENT9*, std::string>> listCreate = {
				std::make_pair(ELEMENTS_TLX, "ELEMENTS_TLX"),
				std::make_pair(ELEMENTS_LX, "ELEMENTS_LX"),
				std::make_pair(ELEMENTS_NX, "ELEMENTS_NX"),
				std::make_pair(ELEMENTS_TLX_INSTANCED, "ELEMENTS_TLX_INSTANCED"),
				std::make_pair(ELEMENTS_LX_INSTANCED, "ELEMENTS_LX_INSTANCED")
			};
			listDeclaration_.resize(listCreate.size(), nullptr);
			for (size_t iDecl = 0U; iDecl < listCreate.size(); ++iDecl) {
				const D3DVERTEXELEMENT9* elem = listCreate[iDecl].first;
				const std::string& name = listCreate[iDecl].second;

				hr = device->CreateVertexDeclaration(elem, &listDeclaration_[iDecl]);
				if (FAILED(hr)) {
					std::string err = StringUtility::Format(
						"RenderShaderLibrary: CreateVertexDeclaration failed. [%s]\r\n\t%s",
						name.c_str(), DXGetErrorStringA(hr));
					throw gstd::wexception(err);
				}
			}
		}
	}
	void RenderShaderLibrary::Release() {
		for (auto& iEffect : listEffect_)
			ptr_release(iEffect);
		for (auto& iDecl : listDeclaration_)
			ptr_release(iDecl);
	}

	void RenderShaderLibrary::OnLostDevice() {
		for (auto iEffect : listEffect_) {
			if (iEffect)
				iEffect->OnLostDevice();
		}
	}
	void RenderShaderLibrary::OnResetDevice() {
		for (auto iEffect : listEffect_) {
			if (iEffect)
				iEffect->OnResetDevice();
		}
	}
}
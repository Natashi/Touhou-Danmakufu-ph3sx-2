#ifndef __DIRECTX_HLSL__
#define __DIRECTX_HLSL__

#include "../pch.h"

#include "DxConstant.hpp"

namespace directx {
	static const std::string NAME_DEFAULT_SKINNED_MESH = "__NAME_DEFAULT_SKINNED_MESH__";
	static const std::string HLSL_DEFAULT_SKINNED_MESH =
		"float4 lightDirection;"
		"float4 materialAmbient : MATERIALAMBIENT;"
		"float4 materialDiffuse : MATERIALDIFFUSE;"
		"float fogNear;"
		"float fogFar;"
		"static const int MAX_MATRICES = 80;"
		"float4x3 mWorldMatrixArray[MAX_MATRICES] : WORLDMATRIXARRAY;"
		"float4x4 mViewProj : VIEWPROJECTION;"

		"struct VS_INPUT {"
			"float4 pos : POSITION;"
			"float4 blendWeights : BLENDWEIGHT;"
			"float4 blendIndices : BLENDINDICES;"
			"float3 normal : NORMAL;"
			"float2 tex0 : TEXCOORD0;"
		"};"

		"struct VS_OUTPUT {"
			"float4 pos : POSITION;"
			"float4 diffuse : COLOR;"
			"float2 tex0 : TEXCOORD0;"
			"float fog : FOG;"
		"};"

		"float3 CalcDiffuse(float3 normal) {"
			"float res;"
			"res = max(0.0f, dot(normal, lightDirection.xyz));"
			"return (res);"
		"}"

		"VS_OUTPUT DefaultTransform(VS_INPUT i, uniform int numBones) {"
			"VS_OUTPUT o;"
			"float3 pos = 0.0f;"
			"float3 normal = 0.0f;"
			"float lastWeight = 0.0f;"

			"float blendWeights[4] = (float[4])i.blendWeights;"
			"int idxAry[4] = (int[4])i.blendIndices;"

			"for (int iBone = 0; iBone < numBones-1; iBone++) {"
				"lastWeight = lastWeight + blendWeights[iBone];"
				"pos += mul(i.pos, mWorldMatrixArray[idxAry[iBone]]) * blendWeights[iBone];"
				"normal += mul(i.normal, mWorldMatrixArray[idxAry[iBone]]) * blendWeights[iBone];"
			"}"
			"lastWeight = 1.0f - lastWeight;"

			"pos += (mul(i.pos, mWorldMatrixArray[idxAry[numBones-1]]) * lastWeight);"
			"normal += (mul(i.normal, mWorldMatrixArray[idxAry[numBones-1]]) * lastWeight);"
			"o.pos = mul(float4(pos.xyz, 1.0f), mViewProj);"

			"normal = normalize(normal);"
			"o.diffuse.xyz = materialAmbient.xyz + CalcDiffuse(normal) * materialDiffuse.xyz;"
			"o.diffuse.w = materialAmbient.w * materialDiffuse.w;"

			"o.tex0 = i.tex0;"
			"o.fog = 1.0f - (o.pos.z - fogNear) / (fogFar - fogNear);"

			"return o;"
		"}"

		"technique BasicTec {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 DefaultTransform(4);"
			"}"
		"}";

	static const std::string NAME_DEFAULT_RENDER3D = "__NAME_DEFAULT_RENDER3D__";
	static const std::string HLSL_DEFAULT_RENDER3D =
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
			"if (useTexture) {"
				"ret *= tex2D(samp0_, inPs.texCoord);"
			"}"
			"if (useFog) {"
				"ret.rgb = smoothstep(ret.rgb, fogColor.rgb, inPs.fog);"
			"}"
			"return ret;"
		"}"
		
		"technique Render {"
			"pass P0 {"
				"ZENABLE = true;"
				"VertexShader = compile vs_3_0 mainVS();"
				"PixelShader = compile ps_3_0 mainPS();"
			"}"
		"}";

	static const std::string NAME_DEFAULT_RENDER2D = "__NAME_DEFAULT_RENDER2D__";
	static const std::string HLSL_DEFAULT_RENDER2D =
		"sampler samp0_ : register(s0);"
		"float4x4 g_mWorld : WORLD : register(c0);"

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

			"outVs.diffuse = inVs.diffuse;"
			"outVs.texCoord = inVs.texCoord;"
			"outVs.position = mul(inVs.position, g_mWorld);"
			"outVs.position.z = 1.0f;"

			"return outVs;"
		"}"

		"PS_OUTPUT mainPS(VS_OUTPUT inPs) {"
			"PS_OUTPUT outPs;"

			"outPs.color = tex2D(samp0_, inPs.texCoord) * inPs.diffuse;"

			"return outPs;"
		"}"
		"PS_OUTPUT mainPS_inv(VS_OUTPUT inPs) {"
			"PS_OUTPUT outPs;"

			"float4 color = tex2D(samp0_, inPs.texCoord);"
			"color.rgb = 1.0f - color.rgb;"
			"outPs.color = color * inPs.diffuse;"

			"return outPs;"
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

	static const std::string NAME_DEFAULT_HWINSTANCE2D = "__NAME_DEFAULT_HW_INST_2D__";
	static const std::string HLSL_DEFAULT_HWINSTANCE2D =
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

			"float4x4 instanceMat = float4x4("
				"float4("
					"t_scale.x * (ay.y * az.y - ax.x * ay.x * az.x),"
					"t_scale.y * (-ax.y * az.x),"
					"t_scale.z * (ay.x * az.y + ax.x * ay.y * az.x),"
					"0"
				"),"
				"float4("
					"t_scale.x * (ay.y * az.x + ax.x * ay.x * az.y),"
					"t_scale.y * (ax.y * az.y),"
					"t_scale.z * (ay.x * az.x - ax.x * ay.y * az.y),"
					"0"
				"),"
				"float4("
					"t_scale.x * (-ax.y * ay.x),"
					"t_scale.y * (ax.x),"
					"t_scale.z * (ax.y * ay.y),"
					"0"
				"),"
				"float4(inVs.i_xyzpos_xsc.x, inVs.i_xyzpos_xsc.y, inVs.i_xyzpos_xsc.z, 1)"
			");"

			"outVs.diffuse = inVs.diffuse * inVs.i_color;"
			"outVs.texCoord = inVs.texCoord;"
			"outVs.position = mul(inVs.position, instanceMat);"
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

	static const std::string NAME_DEFAULT_HWINSTANCE3D = "__NAME_DEFAULT_HW_INST_3D__";
	static const std::string HLSL_DEFAULT_HWINSTANCE3D =
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

			"float4x4 instanceMat = float4x4("
				"float4("
					"t_scale.x * (ay.y * az.y - ax.x * ay.x * az.x),"
					"t_scale.y * (-ax.y * az.x),"
					"t_scale.z * (ay.x * az.y + ax.x * ay.y * az.x),"
					"0"
				"),"
				"float4("
					"t_scale.x * (ay.y * az.x + ax.x * ay.x * az.y),"
					"t_scale.y * (ax.y * az.y),"
					"t_scale.z * (ay.x * az.x - ax.x * ay.y * az.y),"
					"0"
				"),"
				"float4("
					"t_scale.x * (-ax.y * ay.x),"
					"t_scale.y * (ax.x),"
					"t_scale.z * (ax.y * ay.y),"
					"0"
				"),"
				"float4(0, 0, 0, 1)"
			");"

			"outVs.diffuse = inVs.diffuse * inVs.i_color;"
			"outVs.texCoord = inVs.texCoord;"

			"outVs.position = mul(float4(inVs.position, 1.0f), instanceMat);"
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
				"VertexShader = compile vs_2_0 mainVS();"
				"PixelShader = compile ps_3_0 mainPS();"
			"}"
		"}"
		"technique RenderInv {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 mainVS();"
				"PixelShader = compile ps_3_0 mainPS_inv();"
			"}"
		"}"
		"technique RenderNoTexture {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 mainVS();"
				"PixelShader = compile ps_3_0 mainPS_textureless();"
			"}"
		"}";

	class RenderShaderManager {
	public:
		enum {
			MAX_MATRIX = 30
		};
		RenderShaderManager();
		~RenderShaderManager();

		void Initialize();
		void Release();

		void OnLostDevice();
		void OnResetDevice();

		ID3DXEffect* GetSkinnedMeshShader() { return listEffect_[0]; }
		ID3DXEffect* GetRender2DShader() { return listEffect_[1]; }
		ID3DXEffect* GetInstancing2DShader() { return listEffect_[2]; }
		ID3DXEffect* GetInstancing3DShader() { return listEffect_[3]; }

		IDirect3DVertexDeclaration9* GetVertexDeclarationTLX() { return listDeclaration_[0]; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationLX() { return listDeclaration_[1]; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationNX() { return listDeclaration_[2]; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationBNX() { return listDeclaration_[3]; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationInstancedTLX() { return listDeclaration_[4]; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationInstancedLX() { return listDeclaration_[5]; }

		D3DXMATRIX* GetArrayMatrix() { return arrayMatrix; }
	private:
		/*
		 * 0 -> Skinned Mesh
		 * 1 -> 2D Render
		 * 2 -> 2D Hardware Instancing
		 * 3 -> 3D Hardware Instancing
		 */
		std::vector<ID3DXEffect*> listEffect_;

		/*
		 * 0 -> TLX
		 * 1 -> LX
		 * 2 -> NX
		 * 3 -> BNX
		 * 4 -> Instanced TLX
		 * 5 -> Instanced LX
		 */
		std::vector<IDirect3DVertexDeclaration9*> listDeclaration_/*of Independence*/;

		D3DXMATRIX arrayMatrix[MAX_MATRIX];
	};
}

#endif

#pragma once

#include "../pch.h"

#include "DxConstant.hpp"

namespace directx {
	class ShaderSource {
	public:
		static const std::string NAME_DEFAULT_SKINNED_MESH;
		static const std::string HLSL_DEFAULT_SKINNED_MESH;

		static const std::string NAME_DEFAULT_RENDER3D;
		static const std::string HLSL_DEFAULT_RENDER3D;

		static const std::string NAME_DEFAULT_RENDER2D;
		static const std::string HLSL_DEFAULT_RENDER2D;

		static const std::string NAME_DEFAULT_HWINSTANCE2D;
		static const std::string HLSL_DEFAULT_HWINSTANCE2D;

		static const std::string NAME_DEFAULT_HWINSTANCE3D;
		static const std::string HLSL_DEFAULT_HWINSTANCE3D;
	};
	

	class RenderShaderLibrary {
	public:
		enum {
			MAX_MATRIX = 30
		};
	public:
		RenderShaderLibrary();
		~RenderShaderLibrary();

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
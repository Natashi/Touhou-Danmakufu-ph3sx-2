#pragma once

#include "../pch.h"

#include "DxConstant.hpp"

namespace directx {
	class ShaderSource {
	public:
		static const std::string nameRender2D_;
		static const std::string sourceRender2D_;

		static const std::string nameRender3D_;
		static const std::string sourceRender3D_;

		static const std::string nameHwInstance2D_;
		static const std::string sourceHwInstance2D_;

		static const std::string nameHwInstance3D_;
		static const std::string sourceHwInstance3D_;

		static const std::string nameIntersectVisual1_;
		static const std::string sourceIntersectVisual1_;

		static const std::string nameIntersectVisual2_;
		static const std::string sourceIntersectVisual2_;
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

		ID3DXEffect* GetRender2DShader() { return listEffect_[0]; }
		ID3DXEffect* GetInstancing2DShader() { return listEffect_[1]; }
		ID3DXEffect* GetInstancing3DShader() { return listEffect_[2]; }
		ID3DXEffect* GetIntersectVisualShader1() { return listEffect_[3]; }
		ID3DXEffect* GetIntersectVisualShader2() { return listEffect_[4]; }
		size_t GetShaderCount() const { return listEffect_.size(); }

		IDirect3DVertexDeclaration9* GetVertexDeclarationTLX() { return listDeclaration_[0]; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationLX() { return listDeclaration_[1]; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationNX() { return listDeclaration_[2]; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationInstancedTLX() { return listDeclaration_[3]; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationInstancedLX() { return listDeclaration_[4]; }

		D3DXMATRIX* GetArrayMatrix() { return arrayMatrix; }
	private:
		/*
		 * 0 -> 2D Render
		 * 1 -> 2D Hardware Instancing
		 * 2 -> 3D Hardware Instancing
		 * 3 -> Intersection visualizer (circle)
		 * 4 -> Intersection visualizer (line)
		 */
		std::vector<ID3DXEffect*> listEffect_;

		/*
		 * 0 -> TLX
		 * 1 -> LX
		 * 2 -> NX
		 * 3 -> Instanced TLX
		 * 4 -> Instanced LX
		 */
		std::vector<IDirect3DVertexDeclaration9*> listDeclaration_/*of Independence*/;

		D3DXMATRIX arrayMatrix[MAX_MATRIX];
	};
}
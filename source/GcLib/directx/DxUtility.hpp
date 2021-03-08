#pragma once

#include "../pch.h"
#include "DxConstant.hpp"

namespace directx {
#if defined(DNH_PROJ_EXECUTOR)
	//*******************************************************************
	//ColorAccess
	//*******************************************************************
	class ColorAccess {
	public:
		enum {
			BIT_ALPHA = 24,
			BIT_RED = 16,
			BIT_GREEN = 8,
			BIT_BLUE = 0,
		};
		static inline byte GetColorA(const D3DCOLOR& color) {
			//return gstd::BitAccess::GetByte(color, BIT_ALPHA);
			return (color >> 24) & 0xff;
		}
		static inline byte GetColorR(const D3DCOLOR& color) {
			//return gstd::BitAccess::GetByte(color, BIT_RED);
			return (color >> 16) & 0xff;
		}
		static inline byte GetColorG(const D3DCOLOR& color) {
			//return gstd::BitAccess::GetByte(color, BIT_GREEN);
			return (color >> 8) & 0xff;
		}
		static inline byte GetColorB(const D3DCOLOR& color) {
			//return gstd::BitAccess::GetByte(color, BIT_BLUE);
			return color & 0xff;
		}
		static inline D3DCOLOR& SetColorA(D3DCOLOR& color, int alpha) {
			ClampColor(alpha);
			return gstd::BitAccess::SetByte(color, BIT_ALPHA, (byte)alpha);
		}		
		static inline D3DCOLOR& SetColorR(D3DCOLOR& color, int red) {
			ClampColor(red);
			return gstd::BitAccess::SetByte(color, BIT_RED, (byte)red);
		}
		static inline D3DCOLOR& SetColorG(D3DCOLOR& color, int green) {
			ClampColor(green);
			return gstd::BitAccess::SetByte(color, BIT_GREEN, (byte)green);
		}
		static inline D3DCOLOR& SetColorB(D3DCOLOR& color, int blue) {
			ClampColor(blue);
			return gstd::BitAccess::SetByte(color, BIT_BLUE, (byte)blue);
		}

		static D3DCOLORVALUE SetColor(D3DCOLORVALUE& value, D3DCOLOR color);
		static D3DMATERIAL9 SetColor(D3DMATERIAL9 mat, D3DCOLOR color);
		static D3DXVECTOR4& SetColor(D3DXVECTOR4& value, D3DCOLOR color);
		static D3DCOLOR& SetColor(D3DCOLOR& src, const D3DCOLOR& mul);
		static D3DCOLOR& ApplyAlpha(D3DCOLOR& color, float alpha);

		static D3DXVECTOR3& RGBtoHSV(D3DXVECTOR3& color, int red, int green, int blue);
		static D3DCOLOR& HSVtoRGB(D3DCOLOR& color, int hue, int saturation, int value);

		template<typename T>
		static inline void ClampColor(T& color) {
			color = std::clamp(color, (T)0x00, (T)0xff);
		}
		template<typename T>
		static inline T ClampColorRet(T color) {
			ClampColor(color);
			return color;
		}
		static D3DXVECTOR4 ClampColorPacked(const D3DXVECTOR4& src);
		static D3DXVECTOR4 ClampColorPacked(const __m128i& src);
		static __m128i ClampColorPackedM(const D3DXVECTOR4& src);
		static __m128i ClampColorPackedM(const __m128i& src);

		//ARGB representation
		template<typename T>
		static void ToByteArray(D3DCOLOR color, T* arr) {
			arr[0] = GetColorA(color);
			arr[1] = GetColorR(color);
			arr[2] = GetColorG(color);
			arr[3] = GetColorB(color);
		}

		enum : uint8_t {
			PERMUTE_ARGB = 0b00011011,
			PERMUTE_RGBA = 0b01101100,
		};
		static D3DXVECTOR4 ToVec4(const D3DCOLOR& color, uint8_t permute = PERMUTE_ARGB);
		static D3DXVECTOR4 ToVec4Normalized(const D3DCOLOR& color, uint8_t permute = PERMUTE_ARGB);
		//ARGB
		static D3DCOLOR ToD3DCOLOR(const __m128i& color);
		//ARGB
		static D3DCOLOR ToD3DCOLOR(const D3DXVECTOR4& color);
	};

	//*******************************************************************
	//DxMath
	//*******************************************************************
	class DxMath {
	public:
		static inline D3DXVECTOR2 Normalize(const D3DXVECTOR2 &v) {
			return *D3DXVec2Normalize(&D3DXVECTOR2(), &v);
		}
		static inline D3DXVECTOR3 Normalize(const D3DXVECTOR3 &v) {
			return *D3DXVec3Normalize(&D3DXVECTOR3(), &v);
		}
		static inline float DotProduct(const D3DXVECTOR2 &v1, const D3DXVECTOR2 &v2) {
			return D3DXVec2Dot(&v1, &v2);
		}
		static inline float DotProduct(const D3DXVECTOR3 &v1, const D3DXVECTOR3 &v2) {
			return D3DXVec3Dot(&v1, &v2);
		}
		static inline float CrossProduct(const D3DXVECTOR2 &v1, const D3DXVECTOR2 &v2) {
			return D3DXVec2CCW(&v1, &v2);
		}
		static inline D3DXVECTOR3 CrossProduct(const D3DXVECTOR3 &v1, const D3DXVECTOR3 &v2) {
			return *D3DXVec3Cross(&D3DXVECTOR3(), &v1, &v2);
		}

		static bool IsIntersected(D3DXVECTOR2& pos, std::vector<D3DXVECTOR2>& list);
		static bool IsIntersected(DxCircle& circle1, DxCircle& circle2);
		static bool IsIntersected(DxCircle& circle, DxWidthLine& line);
		static bool IsIntersected(DxWidthLine& line1, DxWidthLine& line2);
		static bool IsIntersected(DxLine3D& line, std::vector<DxTriangle>& triangles, std::vector<D3DXVECTOR3>& out);

		static size_t SplitWidthLine(DxWidthLine(&dest)[2], DxWidthLine* src, float mulWidth = 1.0f, bool bForceDouble = false);

		static inline D3DXVECTOR4 VectMatMulti(D3DXVECTOR4 v, D3DXMATRIX& mat) {
			D3DXVECTOR4 res;
			D3DXVec4Transform(&res, &v, &mat);
			return res;
		}
		static void ConstructRotationMatrix(D3DXMATRIX* mat, const D3DXVECTOR2& angleX, 
			const D3DXVECTOR2& angleY, const D3DXVECTOR2& angleZ);
		static void MatrixApplyScaling(D3DXMATRIX* mat, const D3DXVECTOR3& scale);
		static D3DXVECTOR4 RotatePosFromXYZFactor(D3DXVECTOR4& vec, D3DXVECTOR2* angX, D3DXVECTOR2* angY, D3DXVECTOR2* angZ);
		static void TransformVertex2D(VERTEX_TLX(&vert)[4], D3DXVECTOR2* scale, D3DXVECTOR2* angle, 
			D3DXVECTOR2* position, D3DXVECTOR2* textureSize);
	};
#endif
}
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

		static D3DCOLORVALUE MultiplyColor(D3DCOLORVALUE& value, D3DCOLOR color);
		static D3DMATERIAL9 MultiplyColor(D3DMATERIAL9 mat, D3DCOLOR color);
		static D3DXVECTOR4& MultiplyColor(D3DXVECTOR4& value, D3DCOLOR color);
		static D3DCOLOR& MultiplyColor(D3DCOLOR& src, const D3DCOLOR& mul);
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
		static inline D3DCOLOR ToD3DCOLOR(const __m128i& col) {
			return D3DCOLOR_ARGB(col.m128i_i32[0], col.m128i_i32[1], col.m128i_i32[2], col.m128i_i32[3]);
		}
		//ARGB, vector of [0~255]
		static inline D3DCOLOR ToD3DCOLOR(const D3DXVECTOR4& col) {
			return D3DCOLOR_ARGB((byte)col.x, (byte)col.y, (byte)col.z, (byte)col.w);
		}
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

	class DxIntersect {
	public:
		static inline DxWidthLine _LineW_From_Line(const DxLine* line) {
			return DxWidthLine(line->GetX1(), line->GetY1(), line->GetX2(), line->GetY2(), 1.0f);
		}
		static std::vector<DxPoint> _Polygon_From_LineW(const DxWidthLine* line);

		static size_t SplitWidthLine(DxLine* dest, const DxWidthLine* src, float mulWidth = 1.0f, bool bForceDouble = false);

		//----------------------------------------------------------------

		static bool Point_Polygon(const DxPoint* pos, const std::vector<DxPoint>* verts);
		static bool Point_Circle(const DxPoint* pos, const DxCircle* circle);
		static bool Point_Ellipse(const DxPoint* pos, const DxEllipse* ellipse);
		static bool Point_Line(const DxPoint* pos, const DxLine* line);
		static bool Point_LineW(const DxPoint* pos, const DxWidthLine* line);
		static bool Point_RegularPolygon(const DxPoint* pos, const DxRegularPolygon* polygon);

		static bool Circle_Polygon(const DxCircle* circle, const std::vector<DxPoint>* verts);
		static bool Circle_Circle(const DxCircle* circle1, const DxCircle* circle2);
		static bool Circle_Ellipse(const DxCircle* circle, const DxEllipse* ellipse);
		static bool Circle_Line(const DxCircle* circle, const DxLine* line);
		static bool Circle_LineW(const DxCircle* circle, const DxWidthLine* line);
		static bool Circle_RegularPolygon(const DxCircle* circle, const DxRegularPolygon* polygon);

		static bool Line_Polygon(const DxLine* line, const std::vector<DxPoint>* verts);
		static bool Line_Circle(const DxLine* line, const  DxCircle* circle);
		static bool Line_Ellipse(const DxLine* line, const DxEllipse* ellipse);
		static bool Line_Line(const DxLine* line1, const DxLine* line2);
		static bool Line_LineW(const DxLine* line1, const DxWidthLine* line2);
		static bool Line_RegularPolygon(const DxLine* line, const DxRegularPolygon* polygon);

		static bool LineW_Polygon(const DxWidthLine* line, const std::vector<DxPoint>* verts);
		static bool LineW_Circle(const DxWidthLine* line, const DxCircle* circle);
		static bool LineW_Ellipse(const DxWidthLine* line, const DxEllipse* ellipse);
		static bool LineW_Line(const DxWidthLine* line1, const DxLine* line2);
		static bool LineW_LineW(const DxWidthLine* line1, const DxWidthLine* line2);
		static bool LineW_RegularPolygon(const DxWidthLine* line, const DxRegularPolygon* polygon);

		static bool Polygon_Polygon(const std::vector<DxPoint>* verts1, const std::vector<DxPoint>* verts2);
		static bool Polygon_Circle(const std::vector<DxPoint>* verts, const DxCircle* circle);
		static bool Polygon_Ellipse(const std::vector<DxPoint>* verts, const DxEllipse* ellipse);
		static bool Polygon_Line(const std::vector<DxPoint>* verts, const DxLine* line);
		static bool Polygon_LineW(const std::vector<DxPoint>* verts, const DxWidthLine* line);
		static bool Polygon_RegularPolygon(const std::vector<DxPoint>* verts, const DxRegularPolygon* polygon);
	};
#endif
}
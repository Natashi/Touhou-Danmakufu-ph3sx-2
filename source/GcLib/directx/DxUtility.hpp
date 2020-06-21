#ifndef __DIRECTX_UTILITY__
#define __DIRECTX_UTILITY__

#include "../pch.h"
#include "DxConstant.hpp"

namespace directx {
	/**********************************************************
	//ColorAccess
	**********************************************************/
	class ColorAccess {
	public:
		enum {
			BIT_ALPHA = 24,
			BIT_RED = 16,
			BIT_GREEN = 8,
			BIT_BLUE = 0,
		};
		static int GetColorA(D3DCOLOR& color) {
			return gstd::BitAccess::GetByte(color, BIT_ALPHA);
		}
		static int GetColorR(D3DCOLOR color) {
			return gstd::BitAccess::GetByte(color, BIT_RED);
		}
		static int GetColorG(D3DCOLOR& color) {
			return gstd::BitAccess::GetByte(color, BIT_GREEN);
		}
		static int GetColorB(D3DCOLOR& color) {
			return gstd::BitAccess::GetByte(color, BIT_BLUE);
		}
		static D3DCOLOR& SetColorA(D3DCOLOR& color, int alpha) {
			ClampColor(alpha);
			return gstd::BitAccess::SetByte(color, BIT_ALPHA, (byte)alpha);
		}		
		static D3DCOLOR& SetColorR(D3DCOLOR& color, int red) {
			ClampColor(red);
			return gstd::BitAccess::SetByte(color, BIT_RED, (byte)red);
		}
		static D3DCOLOR& SetColorG(D3DCOLOR& color, int green) {
			ClampColor(green);
			return gstd::BitAccess::SetByte(color, BIT_GREEN, (byte)green);
		}
		static D3DCOLOR& SetColorB(D3DCOLOR& color, int blue) {
			ClampColor(blue);
			return gstd::BitAccess::SetByte(color, BIT_BLUE, (byte)blue);
		}

		static D3DCOLORVALUE SetColor(D3DCOLORVALUE value, D3DCOLOR color);
		static D3DMATERIAL9 SetColor(D3DMATERIAL9 mat, D3DCOLOR color);
		static D3DCOLOR& ApplyAlpha(D3DCOLOR& color, float alpha);

		static D3DCOLOR& SetColorHSV(D3DCOLOR& color, int hue, int saturation, int value);

		static inline void ClampColor(int& color) {
			if (color > 0xff) color = 0xff;
			else if (color < 0x00) color = 0x00;
		}
		static inline int ClampColorRet(int color) {
			if (color > 0xff) color = 0xff;
			else if (color < 0x00) color = 0x00;
			return color;
		}
	};


	/**********************************************************
	//衝突判定用図形
	**********************************************************/
	class DxPoint {
	public:
		DxPoint() { pos_ = D3DXVECTOR2(0, 0); }
		DxPoint(float x, float y) { pos_ = D3DXVECTOR2(x, y); }
		virtual ~DxPoint() {}
		float GetX() { return pos_.x; }
		void SetX(float x) { pos_.x = x; }
		float GetY() { return pos_.y; }
		void SetY(float y) { pos_.y = y; }
	private:
		D3DXVECTOR2 pos_;
	};

	class DxCircle {
	public:
		DxCircle() { cen_ = DxPoint(); r_ = 0; }
		DxCircle(float x, float y, float r) { cen_ = DxPoint(x, y); r_ = r; }
		virtual ~DxCircle() {}
		float GetX() { return cen_.GetX(); }
		void SetX(float x) { cen_.SetX(x); }
		float GetY() { return cen_.GetY(); }
		void SetY(float y) { cen_.SetY(y); }
		float GetR() { return r_; }
		void SetR(float r) { r_ = r; }
	private:
		DxPoint cen_;
		float r_;
	};

	class DxWidthLine {
		//幅のある線分
	public:
		DxWidthLine() { p1_ = DxPoint(); p2_ = DxPoint(); width_ = 0; }
		DxWidthLine(float x1, float y1, float x2, float y2, float width) {
			p1_ = DxPoint(x1, y1); p2_ = DxPoint(x2, y2); width_ = width;
		}
		virtual ~DxWidthLine() {}
		float GetX1() { return p1_.GetX(); }
		float GetY1() { return p1_.GetY(); }
		float GetX2() { return p2_.GetX(); }
		float GetY2() { return p2_.GetY(); }
		float GetWidth() { return width_; }
	private:
		DxPoint p1_;
		DxPoint p2_;
		float width_;
	};

	class DxLine3D {
	private:
		D3DXVECTOR3 vertex_[2];
	public:
		DxLine3D() {};
		DxLine3D(const D3DXVECTOR3 &p1, const D3DXVECTOR3 &p2) {
			vertex_[0] = p1;
			vertex_[1] = p2;
		}

		D3DXVECTOR3& GetPosition(size_t index) { return vertex_[index]; }
		D3DXVECTOR3& GetPosition1() { return vertex_[0]; }
		D3DXVECTOR3& GetPosition2() { return vertex_[1]; }

	};

	class DxTriangle {
	private:
		D3DXVECTOR3 vertex_[3];
		D3DXVECTOR3 normal_;

		void _Compute() {
			D3DXVECTOR3 lv[3];
			lv[0] = vertex_[1] - vertex_[0];
			lv[0] = *D3DXVec3Normalize(&D3DXVECTOR3(), &lv[0]);

			lv[1] = vertex_[2] - vertex_[1];
			lv[1] = *D3DXVec3Normalize(&D3DXVECTOR3(), &lv[1]);

			lv[2] = vertex_[0] - vertex_[2];
			lv[2] = *D3DXVec3Normalize(&D3DXVECTOR3(), &lv[2]);

			D3DXVECTOR3 cross = *D3DXVec3Cross(&D3DXVECTOR3(), &lv[0], &lv[1]);
			normal_ = *D3DXVec3Normalize(&D3DXVECTOR3(), &cross);
		}
	public:
		DxTriangle() {}
		DxTriangle(const D3DXVECTOR3 &p1, const D3DXVECTOR3 &p2, const D3DXVECTOR3 &p3) {
			vertex_[0] = p1;
			vertex_[1] = p2;
			vertex_[2] = p3;
			_Compute();
		}

		D3DXVECTOR3& GetPosition(size_t index) { return vertex_[index]; }
		D3DXVECTOR3& GetPosition1() { return vertex_[0]; }
		D3DXVECTOR3& GetPosition2() { return vertex_[1]; }
		D3DXVECTOR3& GetPosition3() { return vertex_[2]; }
		D3DXVECTOR3& GetNormal() { return normal_; }
		FLOAT GetArea() {
			D3DXVECTOR3 v_ab = vertex_[0] - vertex_[1];
			D3DXVECTOR3 v_ac = vertex_[0] - vertex_[2];
			D3DXVECTOR3 vCross;
			D3DXVec3Cross(&vCross, &v_ab, &v_ac);
			return abs(0.5f * D3DXVec3Length(&vCross));
		}
	};

	/**********************************************************
	//DxMath
	**********************************************************/
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

		//ベクトルと行列の積
		static D3DXVECTOR4 VectMatMulti(D3DXVECTOR4 v, D3DXMATRIX& mat) {
			D3DXVECTOR4 res;
			D3DXVec4Transform(&res, &v, &mat);
			return res;
		}

		//衝突判定：点－多角形
		static bool IsIntersected(D3DXVECTOR2& pos, std::vector<D3DXVECTOR2>& list);

		//衝突判定：円-円
		static bool IsIntersected(DxCircle& circle1, DxCircle& circle2);

		//衝突判定：円-直線
		static bool IsIntersected(DxCircle& circle, DxWidthLine& line);

		//衝突判定：直線-直線
		static bool IsIntersected(DxWidthLine& line1, DxWidthLine& line2);

		//衝突判定：直線：三角
		static bool IsIntersected(DxLine3D& line, std::vector<DxTriangle>& triangles, std::vector<D3DXVECTOR3>& out);

		static D3DXVECTOR4 RotatePosFromXYZFactor(D3DXVECTOR4& vec, D3DXVECTOR2* angX, D3DXVECTOR2* angY, D3DXVECTOR2* angZ);
	};

	struct RECT_D {
		double left;
		double top;
		double right;
		double bottom;
	};

	inline RECT_D GetRectD(RECT& rect) {
		RECT_D res = { (double)rect.left, (double)rect.top, (double)rect.right, (double)rect.bottom };
		return res;
	}

	inline void SetRectD(RECT_D* rect, double left, double top, double right, double bottom) {
		rect->left = left;
		rect->top = top;
		rect->right = right;
		rect->bottom = bottom;
	}
}


#endif

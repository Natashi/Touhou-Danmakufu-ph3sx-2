#pragma once

#include "../../GcLib/pch.h"

namespace directx {
	//*******************************************************************
	//DirectGraphicsListener
	//*******************************************************************
	class DirectGraphicsListener {
	public:
		virtual ~DirectGraphicsListener() {}
		virtual void ReleaseDxResource() {}
		virtual void RestoreDxResource() {}
	};

	//*******************************************************************
	//DirectGraphics
	//*******************************************************************
	typedef enum : uint8_t {
		COLOR_MODE_32BIT,
		COLOR_MODE_16BIT,
	} ColorMode;
	typedef enum : uint8_t {
		SCREENMODE_FULLSCREEN,
		SCREENMODE_WINDOW,
	} ScreenMode;
	typedef enum : uint8_t {
		MODE_BLEND_NONE,		//No blending
		MODE_BLEND_ALPHA,		//Alpha blending
		MODE_BLEND_ADD_RGB,		//Add blending, alpha ignored
		MODE_BLEND_ADD_ARGB,	//Add blending
		MODE_BLEND_MULTIPLY,	//Multiply blending
		MODE_BLEND_SUBTRACT,	//Reverse-subtract blending
		MODE_BLEND_SHADOW,		//Invert-multiply blending
		MODE_BLEND_INV_DESTRGB,	//Difference blending in Ph*tosh*p
		MODE_BLEND_ALPHA_INV,	//Alpha blending, but the source color is inverted

		RESET = 0xff,
	} BlendMode;

	struct VertexFogState {
		bool bEnable;
		D3DXVECTOR4 color;
		D3DXVECTOR2 fogDist;
	};

	//*******************************************************************
	//DirectInput
	//*******************************************************************
	typedef enum : uint8_t {
		DI_MOUSE_LEFT = 0,
		DI_MOUSE_RIGHT = 1,
		DI_MOUSE_MIDDLE = 2,
	} DIMouseButton;

	typedef enum : uint8_t {
		KEY_FREE = 0,	 //Key is free, both in the previous and current frame
		KEY_PUSH = 1,	 //Key was free in the previous frame, now pressed
		KEY_PULL = 2,	 //Key was pressed in the previous frame, now free
		KEY_HOLD = 3,	 //Key is being pressed, both in the previous and current frame
	} DIKeyState;
	
#if defined(DNH_PROJ_EXECUTOR)
	//*******************************************************************
	//DirectSound
	//*******************************************************************
	enum class SoundFileFormat : uint8_t {
		Unknown,	//Invalid
		Wave,		//WAVE RIFF
		Ogg,		//Ogg Vorbis
	};

	//*******************************************************************
	//Shader
	//*******************************************************************
	enum class ShaderParameterType : uint8_t {
		Unknown,
		Int,			//Int
		IntArray,		//Int array
		Float,			//Float
		FloatArray,		//Float array
		Vector,			//(x, y, z, w) packed float
		Matrix,			//4x4 matrix
		MatrixArray,	//4x4 matrix array
		Texture,		//IDirect3DTexture9* object
	};

	//*******************************************************************
	//DxObject
	//*******************************************************************
	enum class TypeObject : uint8_t {
		Base,

		Primitive2D,
		Sprite2D,
		SpriteList2D,
		Primitive3D,
		Sprite3D,
		Trajectory3D,

		ParticleList2D,
		ParticleList3D,

		Shader,

		Mesh,
		Text,
		Sound,

		FileText,
		FileBinary,

		//------------------------------

		Player = 128,

		SpellManage,
		Spell,

		Enemy,
		EnemyBoss,
		EnemyBossScene,

		Shot,
		LooseLaser,
		StraightLaser,
		CurveLaser,
		ShotPattern,

		Item,

		Invalid = 0xff,
	};

	//*******************************************************************
	//DxText
	//*******************************************************************
	enum class TextBorderType : uint8_t {
		None,
		Full,
		Shadow,
	};
	enum class TextTagType : uint8_t {
		Unknown,
		Ruby,
		Font
	};
	enum class TextAlignment : uint8_t {
		Left,
		Right,
		Center,
		Top,
		Bottom,
	};
#endif

	//*******************************************************************
	//Rect
	//*******************************************************************
	class DxPoint;
	template<typename T>
	class DxRect {
	public:
		T left, top, right, bottom;
	public:
		DxRect() : left(0), top(0), right(0), bottom(0) {}
		DxRect(T l, T t, T r, T b) : left(l), top(t), right(r), bottom(b) {}
		
		DxRect(const DxRect& src) :
			left(src.left), top(src.top),
			right(src.right), bottom(src.bottom) {}
		
		DxRect(const RECT& src) :
			left(src.left), top(src.top),
			right(src.right), bottom(src.bottom) {}
		
		template<typename L>
		DxRect(const DxRect<L>& src) :
			left(src.left), top(src.top),
			right(src.right), bottom(src.bottom) {}

		template<typename L>
		DxRect<L> NewAs() {
			return DxRect<L>(left, top,
				right, bottom);
		}
		void Set(T l, T t, T r, T b) {
			*this = DxRect(l, t, r, b);
		}

		RECT AsRect() const { return RECT{ left, top, right, bottom }; }
		T GetWidth() const { return right - left; }
		T GetHeight() const { return bottom - top; }

		bool IsIntersected(const DxRect<T>& other) const {
			return !(other.left > right || other.right < left
				|| other.top > bottom || other.bottom < top);
		}

#if defined(DNH_PROJ_EXECUTOR)
		bool IsPointIntersected(const DxPoint* point) const;
		bool IsPointIntersected(const gstd::Math::DVec2& point) const { return IsPointIntersected(point[0], point[1]); }
#endif
		bool IsPointIntersected(const float* point) const { return IsPointIntersected(point[0], point[1]); }
		bool IsPointIntersected(const double* point) const { return IsPointIntersected(point[0], point[1]); }
		bool IsPointIntersected(double x, double y) const {
			return (x >= left && y >= top) && (x <= right && y <= bottom);
		}
	};

#if defined(DNH_PROJ_EXECUTOR)
	//*******************************************************************
	//Shape collisions
	//*******************************************************************
	class DxShapeBase {
	public:
		DxShapeBase() = default;
		virtual ~DxShapeBase() = default;

		virtual DxRect<float> GetBounds() const = 0;
	};

	class DxPoint : public DxShapeBase {
		D3DXVECTOR2 pos_;
	public:
		DxPoint() = default;
		DxPoint(float x, float y) : pos_(x, y) {}
		
		float GetX() const { return pos_.x; }
		void SetX(float x) { pos_.x = x; }
		float GetY() const { return pos_.y; }
		void SetY(float y) { pos_.y = y; }

		DxRect<float> GetBounds() const override {
			return DxRect<float>(GetX(), GetY(), GetX(), GetY());
		}
	};
	template<typename T> bool DxRect<T>::IsPointIntersected(const DxPoint* point) const {
		return IsPointIntersected(point->GetX(), point->GetY());
	}

	class DxCircle : public DxPoint {
		float r_;
	public:
		DxCircle() = default;
		DxCircle(float x, float y, float r) :
			DxPoint(x, y), r_(r) {}
		
		float GetR() const { return r_; }
		void SetR(float r) { r_ = r; }

		DxRect<float> GetBounds() const override {
			float x = GetX();
			float y = GetY();
			return DxRect(x - r_, y - r_, x + r_, y + r_);
		}
	};
	class DxEllipse : public DxPoint {
		float a_, b_;
	public:
		DxEllipse() = default;
		DxEllipse(float x, float y, float a, float b) :
			DxPoint(x, y), a_(a), b_(b) {}

		float GetA() const { return a_; }
		void SetA(float a) { a_ = a; }
		float GetB() const { return b_; }
		void SetB(float b) { b_ = b; }

		DxRect<float> GetBounds() const override {
			float x = GetX();
			float y = GetY();
			return DxRect<float>(x - a_, y - b_, x + a_, y + b_);
		}
	};

	class DxLine : public DxShapeBase {
		DxPoint p1_, p2_;
	public:
		DxLine() = default;
		DxLine(float x1, float y1, float x2, float y2)
			: p1_(x1, y1), p2_(x2, y2) {}

		void SetX1(float x) { p1_.SetX(x); }
		float GetX1() const { return p1_.GetX(); }
		void SetY1(float y) { p1_.SetY(y); }
		float GetY1() const { return p1_.GetY(); }
		void SetX2(float x) { p2_.SetX(x); }
		float GetX2() const { return p2_.GetX(); }
		void SetY2(float y) { p2_.SetY(y); }
		float GetY2() const { return p2_.GetY(); }

		DxRect<float> GetBounds() const override {
			DxRect<float> bound(GetX1(), GetY1(), GetX2(), GetY2());
			if (bound.left > bound.right) std::swap(bound.left, bound.right);
			if (bound.top > bound.bottom) std::swap(bound.top, bound.bottom);
			return bound;
		}
	};
	class DxWidthLine : public DxLine {
		float w_;
	public:
		DxWidthLine() = default;
		DxWidthLine(float x1, float y1, float x2, float y2, float w) :
			DxLine(x1, y1, x2, y2), w_(w) {}

		void SetWidth(float w) { w_ = w; }
		float GetWidth() const { return w_; }

		DxRect<float> GetBounds() const override {
			float l = GetX1(); float t = GetY1();
			float r = GetX2(); float b = GetY2();
			if (l > r) std::swap(l, r);
			if (t > b) std::swap(t, b);
			float w2 = w_ * 0.5f;
			return DxRect<float>(l - w2, t - w2, r + w2, b + w2);
		}
	};

	class DxRegularPolygon : public DxCircle {
		size_t side_;
		float ang_;
	public:
		DxRegularPolygon() : side_(1), ang_(0) {}
		DxRegularPolygon(float x, float y, float r, size_t s, float a) :
			DxCircle(x, y, r), side_(s), ang_(a) {}

		void SetSide(size_t s) { side_ = s; }
		size_t GetSide() const { return side_; }
		void SetAngle(float a) { ang_ = a; }
		float GetAngle() const { return ang_; }
	};

	class DxLine3D : public DxShapeBase {
		std::array<D3DXVECTOR3, 2> vertex_;
	public:
		DxLine3D() = default;
		DxLine3D(const D3DXVECTOR3& p1, const D3DXVECTOR3& p2)
			: vertex_({ p1, p2 }) {}

		D3DXVECTOR3& GetPosition(size_t index) { return vertex_[index]; }
		D3DXVECTOR3& GetPosition1() { return vertex_[0]; }
		D3DXVECTOR3& GetPosition2() { return vertex_[1]; }

		DxRect<float> GetBounds() const override { return DxRect<float>(); }
	};

	class DxTriangle3D : public DxShapeBase {
		std::array<D3DXVECTOR3, 3> vertex_;
		D3DXVECTOR3 normal_;

		void Compute() {
			D3DXVECTOR3 lv[3];
			lv[0] = vertex_[1] - vertex_[0];
			D3DXVec3Normalize(&lv[0], &lv[0]);

			lv[1] = vertex_[2] - vertex_[1];
			D3DXVec3Normalize(&lv[1], &lv[1]);

			lv[2] = vertex_[0] - vertex_[2];
			D3DXVec3Normalize(&lv[2], &lv[2]);

			D3DXVECTOR3 cross;
			D3DXVec3Cross(&cross, &lv[0], &lv[1]);
			
			D3DXVec3Normalize(&normal_, &cross);
		}
	public:
		DxTriangle3D() = default;
		DxTriangle3D(const D3DXVECTOR3& p1, const D3DXVECTOR3& p2, const D3DXVECTOR3& p3)
			: vertex_({ p1, p2, p3 })
		{
			Compute();
		}

		D3DXVECTOR3& GetPosition(size_t index) { return vertex_[index]; }
		D3DXVECTOR3& GetPosition1() { return vertex_[0]; }
		D3DXVECTOR3& GetPosition2() { return vertex_[1]; }
		D3DXVECTOR3& GetPosition3() { return vertex_[2]; }
		D3DXVECTOR3& GetNormal() { return normal_; }
		
		FLOAT GetArea() {
			D3DXVECTOR3 ab = vertex_[0] - vertex_[1];
			D3DXVECTOR3 ac = vertex_[0] - vertex_[2];
			
			D3DXVECTOR3 cross;
			D3DXVec3Cross(&cross, &ab, &ac);
			
			return abs(0.5f * D3DXVec3Length(&cross));
		}

		DxRect<float> GetBounds() const override { return DxRect<float>(); }
	};
#endif
}
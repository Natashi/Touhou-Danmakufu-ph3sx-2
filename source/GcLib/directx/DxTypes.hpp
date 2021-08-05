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

	//*******************************************************************
	//DirectSound
	//*******************************************************************
	enum class SoundFileFormat : uint8_t {
		Unknown,	//Invalid
		Wave,		//WAVE RIFF
		Ogg,		//Ogg Vorbis
		Mp3,		//MPEG3
	};

	//*******************************************************************
	//Shader
	//*******************************************************************
	enum class ShaderParameterType : uint8_t {
		Unknown,
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

	//*******************************************************************
	//Rect
	//*******************************************************************
	template<typename T>
	class DxRect {
	public:
		DxRect() {
			left = (T)0;
			top = (T)0;
			right = (T)0;
			bottom = (T)0;
		}
		DxRect(T l, T t, T r, T b) : left(l), top(t), right(r), bottom(b) {}
		DxRect(const DxRect<T>& src) {
			left = src.left;
			top = src.top;
			right = src.right;
			bottom = src.bottom;
		}
		DxRect(const RECT& src) {
			left = (T)src.left;
			top = (T)src.top;
			right = (T)src.right;
			bottom = (T)src.bottom;
		}
		template<typename L>
		DxRect(const DxRect<L>& src) {
			left = (T)src.left;
			top = (T)src.top;
			right = (T)src.right;
			bottom = (T)src.bottom;
		}

		template<typename L>
		inline DxRect<L> NewAs() {
			DxRect<L> res = DxRect<L>((L)left, (L)top,
				(L)right, (L)bottom);
			return res;
		}
		inline void Set(T l, T t, T r, T b) {
			left = l;
			top = t;
			right = r;
			bottom = b;
		}

		RECT AsRect() const { return { left, top, right, bottom }; }
		T GetWidth() const { return right - left; }
		T GetHeight() const { return bottom - top; }

		bool IsIntersected(const DxRect<T>& other) const {
			return !(other.left > right || other.right < left
				|| other.top > bottom || other.bottom < top);
		}
	public:
		T left, top, right, bottom;
	};

	//*******************************************************************
	//Shape collisions
	//*******************************************************************
	class DxShapeBase {
	public:
		DxShapeBase() {};
		virtual ~DxShapeBase() {};

		virtual DxRect<float> GetBounds() const = 0;
	};

	class DxPoint : public DxShapeBase {
	private:
		D3DXVECTOR2 pos;
	public:
		DxPoint() {};
		DxPoint(float x, float y) { pos = D3DXVECTOR2(x, y); }
		
		float GetX() const { return pos.x; }
		void SetX(float x) { pos.x = x; }
		float GetY() const { return pos.y; }
		void SetY(float y) { pos.y = y; }

		virtual DxRect<float> GetBounds() const {
			return DxRect<float>(GetX(), GetY(), GetX(), GetY());
		}
	};

	class DxCircle : public DxPoint {
	private:
		float r;
	public:
		DxCircle() { r = 0; }
		DxCircle(float x, float y, float _r) : DxPoint(x, y) { r = _r; }
		
		float GetR() const { return r; }
		void SetR(float _r) { r = _r; }

		virtual DxRect<float> GetBounds() const {
			float x = GetX();
			float y = GetY();
			return DxRect<float>(x - r, y - r, x + r, y + r);
		}
	};
	class DxEllipse : public DxPoint {
	private:
		float a;
		float b;
	public:
		DxEllipse() { a = 0; b = 0; }
		DxEllipse(float x, float y, float _a, float _b) : DxPoint(x, y) { a = _a; b = _b; }

		float GetA() const { return a; }
		void SetA(float _a) { a = _a; }
		float GetB() const { return b; }
		void SetB(float _b) { b = _b; }

		virtual DxRect<float> GetBounds() const {
			float x = GetX();
			float y = GetY();
			return DxRect<float>(x - a, y - b, x + a, y + b);
		}
	};

	class DxLine : public DxShapeBase {
	private:
		DxPoint p1;
		DxPoint p2;
	public:
		DxLine() {};
		DxLine(float x1, float y1, float x2, float y2) {
			p1 = DxPoint(x1, y1); p2 = DxPoint(x2, y2);
		}

		void SetX1(float x) { p1.SetX(x); }
		float GetX1() const { return p1.GetX(); }
		void SetY1(float y) { p1.SetY(y); }
		float GetY1() const { return p1.GetY(); }
		void SetX2(float x) { p2.SetX(x); }
		float GetX2() const { return p2.GetX(); }
		void SetY2(float y) { p2.SetY(y); }
		float GetY2() const { return p2.GetY(); }

		virtual DxRect<float> GetBounds() const {
			DxRect<float> bound(GetX1(), GetY1(), GetX2(), GetY2());
			if (bound.left > bound.right) std::swap(bound.left, bound.right);
			if (bound.top > bound.bottom) std::swap(bound.top, bound.bottom);
			return bound;
		}
	};
	class DxWidthLine : public DxLine {
	private:
		float w;
	public:
		DxWidthLine() { w = 0; }
		DxWidthLine(float x1, float y1, float x2, float y2, float w_) : DxLine(x1, y1, x2, y2) {
			w = w_;
		}

		void SetWidth(float w_) { w = w_; }
		float GetWidth() const { return w; }

		virtual DxRect<float> GetBounds() const {
			float l = GetX1(); float t = GetY1();
			float r = GetX2(); float b = GetY2();
			if (l > r) std::swap(l, r);
			if (t > b) std::swap(t, b);
			float w2 = w * 0.5f;
			return DxRect<float>(l - w2, t - w2, r + w2, b + w2);
		}
	};

	class DxRegularPolygon : public DxCircle {
	private:
		size_t side;
		float ang;
	public:
		DxRegularPolygon() { side = 1; ang = 0; }
		DxRegularPolygon(float x, float y, float r, size_t s, float a) : DxCircle(x, y, r) {
			side = s; ang = a;
		}

		void SetSide(size_t s) { side = s; }
		size_t GetSide() const { return side; }
		void SetAngle(float a) { ang = a; }
		float GetAngle() const { return ang; }
	};

	class DxLine3D : public DxShapeBase {
	private:
		D3DXVECTOR3 vertex_[2];
	public:
		DxLine3D() {};
		DxLine3D(const D3DXVECTOR3& p1, const D3DXVECTOR3& p2) {
			vertex_[0] = p1;
			vertex_[1] = p2;
		}

		D3DXVECTOR3& GetPosition(size_t index) { return vertex_[index]; }
		D3DXVECTOR3& GetPosition1() { return vertex_[0]; }
		D3DXVECTOR3& GetPosition2() { return vertex_[1]; }

		virtual DxRect<float> GetBounds() const { return DxRect<float>(); }
	};

	class DxTriangle3D : public DxShapeBase {
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
		DxTriangle3D() {}
		DxTriangle3D(const D3DXVECTOR3& p1, const D3DXVECTOR3& p2, const D3DXVECTOR3& p3) {
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

		virtual DxRect<float> GetBounds() const { return DxRect<float>(); }
	};
}
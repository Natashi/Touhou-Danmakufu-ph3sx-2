#pragma once

#include "../../GcLib/pch.h"

namespace directx {
	/**********************************************************
	//DirectGraphicsListener
	**********************************************************/
	class DirectGraphicsListener {
	public:
		virtual ~DirectGraphicsListener() {}
		virtual void ReleaseDxResource() {}
		virtual void RestoreDxResource() {}
	};

	/**********************************************************
	//DirectGraphics
	**********************************************************/
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

	/**********************************************************
	//DirectInput
	**********************************************************/
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

	/**********************************************************
	//DirectSound
	**********************************************************/
	enum class SoundFileFormat : uint8_t {
		Unknown,	//Invalid
		Wave,		//WAVE RIFF
		Ogg,		//Ogg Vorbis
		Mp3,		//MPEG3
		AWave,		//.wav but mp3 actually(?)
		Midi,		//MIDI (unsupported)
	};

	/**********************************************************
	//Shader
	**********************************************************/
	enum class ShaderParameterType : uint8_t {
		Unknown,
		Float,			//Float
		FloatArray,		//Float array
		Vector,			//(x, y, z, w) packed float
		Matrix,			//4x4 matrix
		MatrixArray,	//4x4 matrix array
		Texture,		//IDirect3DTexture9* object
	};

	/**********************************************************
	//DxObject
	**********************************************************/
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

	/**********************************************************
	//DxText
	**********************************************************/
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

	/**********************************************************
	//Shape collisions
	**********************************************************/
	class DxPoint {
	public:
		DxPoint() { pos_ = D3DXVECTOR2(0, 0); }
		DxPoint(float x, float y) { pos_ = D3DXVECTOR2(x, y); }
		virtual ~DxPoint() {}
		float GetX() const { return pos_.x; }
		void SetX(float x) { pos_.x = x; }
		float GetY() const { return pos_.y; }
		void SetY(float y) { pos_.y = y; }
	private:
		D3DXVECTOR2 pos_;
	};
	class DxCircle {
	public:
		DxCircle() { cen_ = DxPoint(); r_ = 0; }
		DxCircle(float x, float y, float r) { cen_ = DxPoint(x, y); r_ = r; }
		virtual ~DxCircle() {}
		float GetX() const { return cen_.GetX(); }
		void SetX(float x) { cen_.SetX(x); }
		float GetY() const { return cen_.GetY(); }
		void SetY(float y) { cen_.SetY(y); }
		float GetR() const { return r_; }
		void SetR(float r) { r_ = r; }
	private:
		DxPoint cen_;
		float r_;
	};
	class DxWidthLine {
	public:
		DxWidthLine() { p1_ = DxPoint(); p2_ = DxPoint(); width_ = 0; }
		DxWidthLine(float x1, float y1, float x2, float y2, float width) {
			p1_ = DxPoint(x1, y1); p2_ = DxPoint(x2, y2); width_ = width;
		}
		virtual ~DxWidthLine() {}
		float GetX1() const { return p1_.GetX(); }
		float GetY1() const { return p1_.GetY(); }
		float GetX2() const { return p2_.GetX(); }
		float GetY2() const { return p2_.GetY(); }
		float GetWidth() const { return width_; }
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
		DxLine3D(const D3DXVECTOR3& p1, const D3DXVECTOR3& p2) {
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
		DxTriangle(const D3DXVECTOR3& p1, const D3DXVECTOR3& p2, const D3DXVECTOR3& p3) {
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
	//Rect
	**********************************************************/
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

		T GetWidth() const { return right - left; }
		T GetHeight() const { return bottom - top; }
	public:
		T left, top, right, bottom;
	};
}
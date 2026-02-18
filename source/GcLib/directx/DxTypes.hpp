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
}
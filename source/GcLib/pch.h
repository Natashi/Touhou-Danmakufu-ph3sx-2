#pragma once

//--------------------------Game Version Customization--------------------------

//#define GAME_VERSION_TCL
//#define GAME_VERSION_SP

//------------------------------------------------------------------------------



//----------------------------Warning Suppressions-------------------------------

#pragma warning (disable : 4786)	// __declspec attributes before linkage specification are ignored
#pragma warning (disable : 4018)	// signed/unsigned mismatch
#pragma warning (disable : 4244)	// conversion from 'x' to 'y', possible loss of data
#pragma warning (disable : 4503)	// decorated name length exceeded, name was truncated

#pragma warning (disable : 4302)	// truncation from 'x' to 'y'
#pragma warning (disable : 4305)	// initializing/argument: truncation from 'x' to 'y'
#pragma warning (disable : 4819)	// character in file can't be represented in the current code page, save file in Unicode
#pragma warning (disable : 4996)	// deprecated code

#pragma warning (disable : 26451)	// arithmetic overflow : ..... (io.2)
#pragma warning (disable : 26495)	// variable x is uninitialized
#pragma warning (disable : 26812)	// prefer 'enum class' over 'enum'.

//------------------------------------------------------------------------------

// Minimum OS -> Windows 7
#define WINVER _WIN32_WINNT_WIN7
#define _WIN32_WINNT _WIN32_WINNT_WIN7

// Force Unicode
#ifdef _MBCS
#undef _MBCS
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef STRICT
#define STRICT 1
#endif

//------------------------------Header Includes---------------------------------

// Windows

// Definition-inhibiting macros for windows.h
#define NOMINMAX
#define NOSOUND

#include <windows.h>	// Obviously
#include <commctrl.h>	// For a lot of stuff in Window.cpp

#pragma comment (lib, "comctl32.lib")

#undef GetObject

#if defined(DNH_PROJ_EXECUTOR)

	#define _WIN32_DCOM

	#include <wingdi.h>		// For font generation in DxText.cpp
	#include <pdh.h>		// For performance queries in Logger.cpp
	#include <wbemidl.h>

	#pragma comment (lib, "gdi32.lib")
	#pragma comment (lib, "pdh.lib")
	#pragma comment (lib, "wbemuuid.lib")

#endif	// defined(DNH_PROJ_EXECUTOR)

//-----------------------------------DirectX------------------------------------

#define D3D_OVERLOADS

#include <d3d9.h>
#include <d3dx9.h>
#include <DxErr.h>
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dxerr.lib")

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)

	#define DIRECTINPUT_VERSION 0x0800

	#include <dinput.h>
	#pragma comment(lib, "dinput8.lib")

#endif	// defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)

#if defined(DNH_PROJ_EXECUTOR)

	#include <mmreg.h>		// For some wave format constants
	#include <dsound.h>

	#pragma comment(lib, "d3dcompiler.lib")
	#pragma comment(lib, "dsound.lib")

#endif	// defined(DNH_PROJ_EXECUTOR)

//------------------------------------------------------------------------------

#include <cstdlib>

//debug
#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC

	#include <crtdbg.h>
#endif

#ifdef _DEBUG
	#define __L_DBG_NEW__  ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
	#define new __L_DBG_NEW__
#endif

#include <cwchar>
#include <exception>
#include <cassert>

#include <cmath>
#include <cctype>
#include <cwctype>
#include <cstdio>
#include <string>

#include <array>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <any>
#include <bitset>
#include <complex>
#include <optional>

#include <memory>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <future>

#include <fstream>
#include <sstream>

#include <regex>

//-------------------------------External stuffs--------------------------------

// zlib

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
	//#define ZLIB_WINAPI
	#include <zlib/zlib.h>

	#pragma comment(lib, "zlibstatic.lib")
	//#pragma comment(lib, "zlibdynamic.lib")
#endif

// libogg + libvorbis

#if defined(DNH_PROJ_EXECUTOR)
	#include <vorbis/codec.h>
	#include <vorbis/vorbisfile.h>

	#pragma comment(lib, "ogg_static.lib")
	#pragma comment(lib, "vorbis_static.lib")
	//#pragma comment(lib, "vorbis_dynamic.lib")
	#pragma comment(lib, "vorbisfile_static.lib")
#endif

//------------------------------------------------------------------------------

#if (!defined(__L_ENGINE_LEGACY))

	#define __L_MATH_VECTORIZE
	#define __L_USE_HWINSTANCING

#endif

//-----------------------------------Extras-------------------------------------

// Use std::filesystem for file management
#define __L_STD_FILESYSTEM
#ifdef __L_STD_FILESYSTEM
	#include <filesystem>
	namespace stdfs = std::filesystem;
	using path_t = stdfs::path;
#endif

namespace stdch = std::chrono;

//------------------------------------------------------------------------------

#include "Types.hpp"

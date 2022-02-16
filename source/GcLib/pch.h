#pragma once

//--------------------------Game Version Customization--------------------------

//#define GAME_VERSION_TCL
//#define GAME_VERSION_SP

//------------------------------------------------------------------------------



//----------------------------Warning Suppressions-------------------------------

#pragma warning (disable : 4786)	//__declspec attributes before linkage specification are ignored
#pragma warning (disable : 4018)	//signed/unsigned mismatch
#pragma warning (disable : 4244)	//conversion from 'x' to 'y', possible loss of data
#pragma warning (disable : 4503)	//decorated name length exceeded, name was truncated

#pragma warning (disable : 4302)	//truncation from 'x' to 'y'
#pragma warning (disable : 4305)	//initializing/argument: truncation from 'x' to 'y'
#pragma warning (disable : 4819)	//character in file can't be represented in the current code page, save file in Unicode
#pragma warning (disable : 4996)	//deprecated code

#pragma warning (disable : 26451)	//arithmetic overflow : ..... (io.2)
#pragma warning (disable : 26495)	//variable x is uninitialized
#pragma warning (disable : 26812)	//prefer 'enum class' over 'enum'.

//------------------------------------------------------------------------------

//Minimum OS -> Windows 7
#define WINVER _WIN32_WINNT_WIN7
#define _WIN32_WINNT _WIN32_WINNT_WIN7

//Force Unicode
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

//Windows
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <pdh.h>
#include <process.h>
#include <wingdi.h>
#include <shlwapi.h>

#include <mlang.h>
#include <psapi.h>

#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "comctl32.lib")
#pragma comment (lib, "pdh.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "psapi.lib")

//-----------------------------------DirectX------------------------------------

#define D3D_OVERLOADS

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)

#define DIRECTINPUT_VERSION 0x0800

#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <DxErr.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxerr.lib")
#pragma comment(lib, "d3dcompiler.lib")

#if defined(DNH_PROJ_EXECUTOR)

#define DIRECTSOUND_VERSION 0x0900

//for acm
#include <mmreg.h> 
#include <msacm.h>

#include <basetsd.h>
#include <dsound.h>

/*
#pragma warning(push)
#pragma warning (disable : 4838)	//require narrowing conversion
#include <xnamath.h>
#pragma warning(pop)
*/

#pragma comment(lib, "msacm32.lib")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "d3dxof.lib")

#endif	//defined(DNH_PROJ_EXECUTOR)

#endif	//defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)

//------------------------------------------------------------------------------

//debug
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif
#include <cstdlib>
#include <crtdbg.h>

#ifdef _DEBUG
#define __L_DBG_NEW__  ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new __L_DBG_NEW__
#endif

#include <cwchar>
#include <exception>

#include <cmath>
#include <cctype>
#include <cwctype>
#include <cstdio>
#include <clocale>
#include <locale>
#include <string>

#include <array>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <bitset>
#include <complex>

#include <memory>
#include <algorithm>
#include <iterator>
#include <future>

#include <fstream>
#include <sstream>

#include <cassert>

#include <regex>

//-------------------------------External stuffs--------------------------------

//OpenMP
#include <omp.h>

//zlib
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
//#define ZLIB_WINAPI
#include <zlib/zlib.h>
#pragma comment(lib, "zlibstatic.lib")
//#pragma comment(lib, "zlibdynamic.lib")
#endif

//libogg + libvorbis
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

//Use std::filesystem for file management
#define __L_STD_FILESYSTEM
#ifdef __L_STD_FILESYSTEM
#include <filesystem>
namespace stdfs = std::filesystem;
using path_t = stdfs::path;
#endif

namespace stdch = std::chrono;

//------------------------------------------------------------------------------

//Pointer utilities
template<typename T> static constexpr inline void ptr_delete(T*& ptr) {
	if (ptr) delete ptr;
	ptr = nullptr;
}
template<typename T> static constexpr inline void ptr_delete_scalar(T*& ptr) {
	if (ptr) delete[] ptr;
	ptr = nullptr;
}
template<typename T> static constexpr inline void ptr_release(T*& ptr) {
	if (ptr) ptr->Release();
	ptr = nullptr;
}
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;

using QWORD = uint64_t;		//Quad word

#undef min
#undef max

#undef GetObject

#define API_DEFINE_GET(_type, _name, _var) _type Get##_name() { return _var; }
#define API_DEFINE_SET(_type, _name, _var) void Set##_name(_type v) { _var = v; }
#define API_DEFINE_GETSET(_type, _name, _target) \
			API_DEFINE_GET(_type, _name, _target) \
			API_DEFINE_SET(_type, _name, _target)

#define LOCK_WEAK(_v, _p) if (auto _v = (_p).lock())
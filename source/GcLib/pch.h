#pragma once

//--------------------------Game Version Customization--------------------------

//#define GAME_VERSION_TCL
//#define GAME_VERSION_SP

//------------------------------------------------------------------------------

//--------------------------------Force Unicode---------------------------------

#ifdef _MBCS
#undef _MBCS
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

//------------------------------------------------------------------------------

//Minimum OS -> Windows 7
#define WINVER _WIN32_WINNT_WIN7
#define _WIN32_WINNT _WIN32_WINNT_WIN7

//------------------------------Windows Libraries-------------------------------

#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "comctl32.lib")
#pragma comment (lib, "pdh.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "psapi.lib")

//----------------------------Warning Suppressions-------------------------------

#pragma warning (disable : 4786) //__declspec attributes before linkage specification are ignored
#pragma warning (disable : 4018) //signed/unsigned mismatch
#pragma warning (disable : 4244) //conversion from 'x' to 'y', possible loss of data
#pragma warning (disable : 4503) //decorated name length exceeded, name was truncated

#pragma warning (disable : 4302) //truncation from 'x' to 'y'
#pragma warning (disable : 4305) //initializing/argument: truncation from 'x' to 'y'
#pragma warning (disable : 4819) //character in file can't be represented in the current code page, save file in Unicode
#pragma warning (disable : 4996) //deprecated code

#pragma warning (disable : 26451) //arithmetic overflow : ..... (io.2)
#pragma warning (disable : 26495) //variable x is uninitialized
#pragma warning (disable : 26812) //prefer 'enum class' over 'enum'.

//------------------------------------------------------------------------------

//define
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

//-----------------------------------DirectX------------------------------------

#define D3D_OVERLOADS

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)

#define DIRECTINPUT_VERSION 0x0800

#include <d3d9.h>
#include <dinput.h>
#include <DxErr.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxerr.lib")

#endif

#if defined(DNH_PROJ_EXECUTOR)

#define DIRECTSOUND_VERSION 0x0900

//for acm
#include <mmreg.h> 
#include <msacm.h>

#include <basetsd.h>
#include <d3dx9.h>
#include <dsound.h>
#include <dmusici.h>

#pragma comment(lib, "msacm32.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "d3dxof.lib")

#endif

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

#include <list>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <bitset>

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



//-----------------------------------Extras-------------------------------------

//Use std::filesystem for file management
#define __L_STD_FILESYSTEM
#ifdef __L_STD_FILESYSTEM
#include <filesystem>
namespace stdfs = std::filesystem;
using path_t = stdfs::path;
#endif

//Guarantee thread-safety in texture management
//#define __L_TEXTURE_THREADSAFE

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
using std::shared_ptr;
using std::weak_ptr;

#undef min
#undef max

#undef GetObject
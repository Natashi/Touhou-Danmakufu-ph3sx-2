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

//ïWèÄä÷êîëŒâûï\
//http://www1.kokusaika.jp/advisory/org/ja/win32_unicode.html

//------------------------------------------------------------------------------


//Win2000à»ç~
#define _WIN32_WINNT 0x0500


//------------------------------Windows Libraries-------------------------------

#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "comctl32.lib")
#pragma comment (lib, "pdh.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "psapi.lib")

//------------------------------------------------------------------------------



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

//debug
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif
#include <cstdlib>
#include <crtdbg.h>

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

//------------------------------------------------------------------------------



//-----------------------------------DirectX------------------------------------

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)

//lib
#pragma comment(lib, "msacm32.lib") //for acm
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "d3dxof.lib")
#pragma comment(lib, "dxerr.lib")

//define
#define D3D_OVERLOADS
#define DIRECTINPUT_VERSION 0x0800
#define DIRECTSOUND_VERSION 0x0900

//include
#include <mmreg.h> //for acm
#include <msacm.h> //for acm

#include <basetsd.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <dsound.h>
#include <dmusici.h>
#include <DxErr.h>

#endif

//------------------------------------------------------------------------------



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

//In the case crtdbg is used
#ifdef _DEBUG
#define __L_DBG_NEW__  ::new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new __L_DBG_NEW__
#endif

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
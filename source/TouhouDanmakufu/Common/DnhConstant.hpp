#ifndef __TOUHOUDANMAKUFU_DNHCONSTANT__
#define __TOUHOUDANMAKUFU_DNHCONSTANT__

#include "../../GcLib/pch.h"
#include "../../GcLib/GcLib.hpp"

using namespace gstd;
using namespace directx;

constexpr const int STANDARD_FPS = 60;

const std::wstring DNH_EXE_DEFAULT = L"th_dnh_ph3sx.exe";
const std::wstring DNH_VERSION = L"v1.10a";
constexpr const size_t DNH_VERSION_NUM = /*e*/621;	//OWO!!!!!

#if NDEBUG
typedef bool PriListBool;
#else
typedef byte PriListBool;
#endif

#endif



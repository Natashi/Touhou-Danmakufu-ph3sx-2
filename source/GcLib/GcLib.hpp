#pragma once

#include "pch.h"

#include "gstd/GstdLib.hpp"
#include "directx/DxLib.hpp"

constexpr const int STANDARD_FPS = 60;

const std::wstring DNH_EXE_DEFAULT = L"th_dnh_ph3sx.exe";
const std::wstring DNH_VERSION = L"v1.22a-pre";

constexpr const uint64_t _GAME_VERSION_RESERVED = /*e*/621;		//OWO!!!!!
constexpr const uint64_t _GAME_VERSION_MAJOR = 1;
constexpr const uint64_t _GAME_VERSION_MINOR = 20;
constexpr const uint64_t _GAME_VERSION_REVIS = 9;

//00000000 00000000 | 00000000 00000000 | 00000000 00000000 | 00000000 00000000
//<---RESERVED----> | <-----MAJOR-----> | <-----MINOR-----> | <---REVISIONS--->
constexpr const uint64_t GAME_VERSION_NUM = ((_GAME_VERSION_RESERVED & 0xffff) << 48)
	| ((_GAME_VERSION_MAJOR & 0xffff) << 32) | ((_GAME_VERSION_MINOR & 0xffff) << 16)
	| (_GAME_VERSION_REVIS & 0xffff);
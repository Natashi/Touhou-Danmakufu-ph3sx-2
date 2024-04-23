#pragma once
#include "../pch.h"

const std::wstring DNH_EXE_NAME = L"th_dnh_ph3sx.exe";
const std::wstring DNH_VERSION = L"v1.33a-pre2";

constexpr uint64_t _GAME_VERSION_RESERVED = /*e*/621;		// OWO!!!!!
constexpr uint64_t _GAME_VERSION_MAJOR = 1;
constexpr uint64_t _GAME_VERSION_SUBMAJOR = 4;
constexpr uint64_t _GAME_VERSION_MINOR = 0;
constexpr uint64_t _GAME_VERSION_REVISION = 1;

// 00000000 00000000 | 00000000 00000000 | 00000000 00000000 | 00000000 00000000
// <-RESERVED--><----MAJOR----> <-----SUBMAJOR----> <------MINOR------> <REVIS->
constexpr uint64_t GAME_VERSION_NUM = ((_GAME_VERSION_RESERVED & 0xfff) << 52)
| ((_GAME_VERSION_MAJOR & 0xfff) << 40) | ((_GAME_VERSION_SUBMAJOR & 0xffff) << 24)
| ((_GAME_VERSION_MINOR & 0xffff) << 8) | (_GAME_VERSION_REVISION & 0xff);


// Version values for each of the engine's data formats

constexpr uint32_t _GAME_VERSION_RESERVED_HIBYTE = ((uint32_t)_GAME_VERSION_RESERVED & 0xFFFF) << 16;

constexpr uint32_t DATA_VERSION_ARCHIVE = _GAME_VERSION_RESERVED_HIBYTE | 5;
constexpr uint32_t DATA_VERSION_CONFIG  = _GAME_VERSION_RESERVED_HIBYTE | 5;
constexpr uint32_t DATA_VERSION_CAREA   = _GAME_VERSION_RESERVED_HIBYTE | 5;
constexpr uint32_t DATA_VERSION_REPLAY  = _GAME_VERSION_RESERVED_HIBYTE | 5;

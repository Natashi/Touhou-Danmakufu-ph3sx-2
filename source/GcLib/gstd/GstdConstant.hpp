#pragma once
#include "../pch.h"

const std::wstring DNH_EXE_NAME = L"th_dnh_ph3sx.exe";
const std::wstring DNH_VERSION = L"v1.33a-pre2";

constexpr uint64_t _GAME_VERSION_RESERVED = /*e*/621;		// OWO!!!!!
constexpr uint64_t _GAME_VERSION_MAJOR = 1;
constexpr uint64_t _GAME_VERSION_SUBMAJOR = 4;
constexpr uint64_t _GAME_VERSION_MINOR = 0;
constexpr uint64_t _GAME_VERSION_REVISION = 1;

/**
 * [63:52] (12 bits) Reserved
 * [51:40] (12 bits) Major Version
 * [39:24] (16 bits) Submajor Version
 * [23:8 ] (16 bits) Minor Version
 * [ 7:0 ] (8 bits)  Revision
 */
constexpr uint64_t GAME_VERSION_NUM = ((_GAME_VERSION_RESERVED & 0xfff) << 52)
	| ((_GAME_VERSION_MAJOR & 0xfff) << 40) | ((_GAME_VERSION_SUBMAJOR & 0xffff) << 24)
	| ((_GAME_VERSION_MINOR & 0xffff) << 8) | (_GAME_VERSION_REVISION & 0xff);

// Version values for each of the engine's data formats

constexpr uint32_t GAME_VERSION_RESERVED_HIBYTE = (_GAME_VERSION_RESERVED & 0xffff) << 16;

constexpr uint32_t DATA_VERSION_ARCHIVE = GAME_VERSION_RESERVED_HIBYTE | 5;
constexpr uint32_t DATA_VERSION_CONFIG  = GAME_VERSION_RESERVED_HIBYTE | 5;
constexpr uint32_t DATA_VERSION_CAREA   = GAME_VERSION_RESERVED_HIBYTE | 5;
constexpr uint32_t DATA_VERSION_REPLAY  = GAME_VERSION_RESERVED_HIBYTE | 5;

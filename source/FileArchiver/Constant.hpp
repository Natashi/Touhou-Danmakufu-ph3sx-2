#pragma once

#include "../GcLib/pch.h"
#include "../GcLib/GcLib.hpp"

#pragma push_macro("new")
#undef new

#include "imgui.h"
#include "backends/imgui_impl_dx9.h"
#include "backends/imgui_impl_win32.h"

#pragma pop_macro("new")

using namespace gstd;
using namespace directx;

const std::wstring WINDOW_TITLE = L"File Archiver ph3sx";
const std::wstring PATH_ENVIRONMENT = L"settings.dat";
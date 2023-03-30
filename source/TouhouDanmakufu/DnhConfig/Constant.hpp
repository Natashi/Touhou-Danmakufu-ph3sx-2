#pragma once
#include "../../GcLib/pch.h"

#pragma push_macro("new")
#undef new

#include "imgui.h"
#include "backends/imgui_impl_dx9.h"
#include "backends/imgui_impl_win32.h"

#pragma pop_macro("new")

#include "../Common/DnhConstant.hpp"
#include "../Common/DnhCommon.hpp"

const std::wstring WINDOW_TITLE = L"Config ph3sx";
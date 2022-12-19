#pragma once

#include "../pch.h"

#include "DxConstant.hpp"
#include "DxUtility.hpp"

#if defined(DNH_PROJ_EXECUTOR)
#include "HLSL.hpp"
#endif

#include "DirectGraphics.hpp"

#if defined(DNH_PROJ_EXECUTOR)
#include "Texture.hpp"
#include "Shader.hpp"

#include "RenderObject.hpp"
#include "DxText.hpp"

//#include "ElfreinaMesh.hpp"
#include "MetasequoiaMesh.hpp"

#include "TransitionEffect.hpp"
//#include "DxWindow.hpp"
#include "DxObject.hpp"
#include "DxScript.hpp"
#include "ScriptManager.hpp"

#include "DirectSound.hpp"
#endif

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)
#include "DirectInput.hpp"
#endif
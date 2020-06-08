#include "source/GcLib/pch.h"

#include "DnhScript.hpp"
#include "DnhCommon.hpp"
#include "DnhGcLibImpl.hpp"

/**********************************************************
//DnhScript
**********************************************************/
const static function dnhFunction[] =
{
	{ "KEY_INVALID", constant<EDirectInput::KEY_INVALID>::func, 0 },

	{ "VK_LEFT", constant<EDirectInput::KEY_LEFT>::func, 0 },
	{ "VK_RIGHT", constant<EDirectInput::KEY_RIGHT>::func, 0 },
	{ "VK_UP", constant<EDirectInput::KEY_UP>::func, 0 },
	{ "VK_DOWN", constant<EDirectInput::KEY_DOWN>::func, 0 },
	{ "VK_SHOT", constant<EDirectInput::KEY_SHOT>::func, 0 },
	{ "VK_BOMB", constant<EDirectInput::KEY_BOMB>::func, 0 },
	{ "VK_SPELL", constant<EDirectInput::KEY_BOMB>::func, 0 },
	{ "VK_SLOWMOVE", constant<EDirectInput::KEY_SLOWMOVE>::func, 0 },
	{ "VK_USER1", constant<EDirectInput::KEY_USER1>::func, 0 },
	{ "VK_USER2", constant<EDirectInput::KEY_USER2>::func, 0 },
	{ "VK_OK", constant<EDirectInput::KEY_OK>::func, 0 },
	{ "VK_CANCEL", constant<EDirectInput::KEY_CANCEL>::func, 0 },
	{ "VK_PAUSE", constant<EDirectInput::KEY_PAUSE>::func, 0 },

	{ "VK_USER_ID_STAGE", constant<EDirectInput::VK_USER_ID_STAGE>::func, 0 },
	{ "VK_USER_ID_PLAYER", constant<EDirectInput::VK_USER_ID_PLAYER>::func, 0 },
};
DnhScript::DnhScript() {
	_AddFunction(dnhFunction, sizeof(dnhFunction) / sizeof(function));
}

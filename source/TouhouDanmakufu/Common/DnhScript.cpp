#include "source/GcLib/pch.h"

#include "DnhScript.hpp"
#include "DnhCommon.hpp"
#include "DnhGcLibImpl.hpp"

//*******************************************************************
//DnhScript
//*******************************************************************
static const std::vector<constant> dnhConstant = {
	//Oops! All virtual keys!
	constant("KEY_INVALID", EDirectInput::KEY_INVALID),

	constant("VK_LEFT", EDirectInput::KEY_LEFT),
	constant("VK_RIGHT", EDirectInput::KEY_RIGHT),
	constant("VK_UP", EDirectInput::KEY_UP),
	constant("VK_DOWN", EDirectInput::KEY_DOWN),
	constant("VK_SHOT", EDirectInput::KEY_SHOT),
	constant("VK_BOMB", EDirectInput::KEY_BOMB),
	constant("VK_SPELL", EDirectInput::KEY_BOMB),
	constant("VK_SLOWMOVE", EDirectInput::KEY_SLOWMOVE),
	constant("VK_USER1", EDirectInput::KEY_USER1),
	constant("VK_USER2", EDirectInput::KEY_USER2),
	constant("VK_OK", EDirectInput::KEY_OK),
	constant("VK_CANCEL", EDirectInput::KEY_CANCEL),
	constant("VK_PAUSE", EDirectInput::KEY_PAUSE),

	constant("VK_USER_ID_STAGE", EDirectInput::VK_USER_ID_STAGE),
	constant("VK_USER_ID_PLAYER", EDirectInput::VK_USER_ID_PLAYER),
};
DnhScript::DnhScript() {
	_AddConstant(&dnhConstant);
}

#include "source/GcLib/pch.h"

#include "Common.hpp"

//*******************************************************************
//KeyCodeList
//*******************************************************************
KeyCodeList::KeyCodeList() {
	mapText_[DIK_ESCAPE] = "Esc";
	mapText_[DIK_1] = "1";
	mapText_[DIK_2] = "2";
	mapText_[DIK_3] = "3";
	mapText_[DIK_4] = "4";
	mapText_[DIK_5] = "5";
	mapText_[DIK_6] = "6";
	mapText_[DIK_7] = "7";
	mapText_[DIK_8] = "8";
	mapText_[DIK_9] = "9";
	mapText_[DIK_0] = "0";
	mapText_[DIK_MINUS] = "-";
	mapText_[DIK_EQUALS] = "=";
	mapText_[DIK_BACK] = "Backspace ";
	mapText_[DIK_TAB] = "Tab";

	mapText_[DIK_Q] = "Q";
	mapText_[DIK_W] = "W";
	mapText_[DIK_E] = "E";
	mapText_[DIK_R] = "R";
	mapText_[DIK_T] = "T";
	mapText_[DIK_Y] = "Y";
	mapText_[DIK_U] = "U";
	mapText_[DIK_I] = "I";
	mapText_[DIK_O] = "O";
	mapText_[DIK_P] = "P";
	mapText_[DIK_LBRACKET] = "[";
	mapText_[DIK_RBRACKET] = "]";
	mapText_[DIK_RETURN] = "Enter";
	mapText_[DIK_LCONTROL] = "LCtrl";

	mapText_[DIK_A] = "A";
	mapText_[DIK_S] = "S";
	mapText_[DIK_D] = "D";
	mapText_[DIK_F] = "F";
	mapText_[DIK_G] = "G";
	mapText_[DIK_H] = "H";
	mapText_[DIK_J] = "J";
	mapText_[DIK_K] = "K";
	mapText_[DIK_L] = "L";
	mapText_[DIK_SEMICOLON] = ";";
	mapText_[DIK_APOSTROPHE] = "'";
	mapText_[DIK_GRAVE] = "`";
	mapText_[DIK_LSHIFT] = "LShift";
	mapText_[DIK_BACKSLASH] = "\\";

	mapText_[DIK_Z] = "Z";
	mapText_[DIK_X] = "X";
	mapText_[DIK_C] = "C";
	mapText_[DIK_V] = "V";
	mapText_[DIK_B] = "B";
	mapText_[DIK_N] = "N";
	mapText_[DIK_M] = "M";
	mapText_[DIK_COMMA] = ",";
	mapText_[DIK_PERIOD] = ".";
	mapText_[DIK_SLASH] = "/";
	mapText_[DIK_RSHIFT] = "RShift";
	mapText_[DIK_MULTIPLY] = "* (Numpad)";
	mapText_[DIK_LMENU] = "LAlt";
	mapText_[DIK_SPACE] = "Space";
	mapText_[DIK_CAPITAL] = "Caps Lock";

	mapText_[DIK_F1] = "F1";
	mapText_[DIK_F2] = "F2";
	mapText_[DIK_F3] = "F3";
	mapText_[DIK_F4] = "F4";
	mapText_[DIK_F5] = "F5";
	mapText_[DIK_F6] = "F6";
	mapText_[DIK_F7] = "F7";
	mapText_[DIK_F8] = "F8";
	mapText_[DIK_F9] = "F9";
	mapText_[DIK_F10] = "F10";

	mapText_[DIK_NUMLOCK] = "Num Lock";
	mapText_[DIK_SCROLL] = "Scroll Lock";

	mapText_[DIK_NUMPAD7] = "7 (Numpad)";
	mapText_[DIK_NUMPAD8] = "8 (Numpad)";
	mapText_[DIK_NUMPAD9] = "9 (Numpad)";
	mapText_[DIK_SUBTRACT] = "- (Numpad)";
	mapText_[DIK_NUMPAD4] = "4 (Numpad)";
	mapText_[DIK_NUMPAD5] = "5 (Numpad)";
	mapText_[DIK_NUMPAD6] = "6 (Numpad)";
	mapText_[DIK_ADD] = "+ (Numpad)";
	mapText_[DIK_NUMPAD1] = "1 (Numpad)";
	mapText_[DIK_NUMPAD2] = "2 (Numpad)";
	mapText_[DIK_NUMPAD3] = "3 (Numpad)";
	mapText_[DIK_NUMPAD0] = "0 (Numpad)";
	mapText_[DIK_DECIMAL] = ". (Numpad)";

	mapText_[DIK_F11] = "F11";
	mapText_[DIK_F12] = "F12";
	mapText_[DIK_F13] = "F13";		//NEC PC-98
	mapText_[DIK_F14] = "F14";		//NEC PC-98
	mapText_[DIK_F15] = "F15";		//NEC PC-98

	mapText_[DIK_KANA] = "Kana";				//Japanese Keyboard
	mapText_[DIK_CONVERT] = "Convert";			//Japanese Keyboard
	mapText_[DIK_NOCONVERT] = "No Convert";	//Japanese Keyboard
	mapText_[DIK_YEN] = "\\";					//Japanese Keyboard
	mapText_[DIK_NUMPADEQUALS] = "(Numpad)";	//NEC PC-98
	mapText_[DIK_CIRCUMFLEX] = "^";			//Japanese Keyboard

	mapText_[DIK_AT] = "@";			//NEC PC-98
	mapText_[DIK_COLON] = ":";			//NEC PC-98
	mapText_[DIK_UNDERLINE] = "_";		//NEC PC-98
	mapText_[DIK_KANJI] = "Kanji";		//Japanese Keyboard
	mapText_[DIK_STOP] = "Stop";		//NEC PC-98
	mapText_[DIK_AX] = "(Japan AX)";
	mapText_[DIK_UNLABELED] = "(J3100)";

	mapText_[DIK_NUMPADENTER] = "Enter (Numpad)";
	mapText_[DIK_RCONTROL] = "RCtrl";
	mapText_[DIK_NUMPADCOMMA] = ", (Numpad)";		//NEC PC-98
	mapText_[DIK_DIVIDE] = "/ (Numpad)";
	mapText_[DIK_SYSRQ] = "Sys Rq";
	mapText_[DIK_RMENU] = "RAlt";
	mapText_[DIK_PAUSE] = "Pause";
	mapText_[DIK_HOME] = "Home";
	mapText_[DIK_UP] = "↑";
	mapText_[DIK_PRIOR] = "Page Up";
	mapText_[DIK_LEFT] = "←";
	mapText_[DIK_RIGHT] = "→";
	mapText_[DIK_END] = "End";
	mapText_[DIK_DOWN] = "↓";
	mapText_[DIK_NEXT] = "Page Down";

	mapText_[DIK_INSERT] = "Insert";
	mapText_[DIK_DELETE] = "Delete";
	mapText_[DIK_LWIN] = "LWindows";
	mapText_[DIK_RWIN] = "RWindows";
	mapText_[DIK_APPS] = "Menu";
	mapText_[DIK_POWER] = "Power";
	mapText_[DIK_SLEEP] = "Sleep";
}

bool KeyCodeList::IsValidCode(int16_t code) {
	bool res = mapText_.find(code) != mapText_.end();
	return res;
}
std::string KeyCodeList::GetCodeText(int16_t code) {
	auto itr = mapText_.find(code);
	if (itr == mapText_.end())
		return "";
	return itr->second;
}

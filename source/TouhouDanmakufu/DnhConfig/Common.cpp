#include "source/GcLib/pch.h"

#include "Common.hpp"

/**********************************************************
//KeyCodeList
**********************************************************/
KeyCodeList::KeyCodeList() {
	mapText_[DIK_ESCAPE] = L"Esc";
	mapText_[DIK_1] = L"1";
	mapText_[DIK_2] = L"2";
	mapText_[DIK_3] = L"3";
	mapText_[DIK_4] = L"4";
	mapText_[DIK_5] = L"5";
	mapText_[DIK_6] = L"6";
	mapText_[DIK_7] = L"7";
	mapText_[DIK_8] = L"8";
	mapText_[DIK_9] = L"9";
	mapText_[DIK_0] = L"0";
	mapText_[DIK_MINUS] = L"-";
	mapText_[DIK_EQUALS] = L"=";
	mapText_[DIK_BACK] = L"Back Space ";
	mapText_[DIK_TAB] = L"Tab";

	mapText_[DIK_Q] = L"Q";
	mapText_[DIK_W] = L"W";
	mapText_[DIK_E] = L"E";
	mapText_[DIK_R] = L"R";
	mapText_[DIK_T] = L"T";
	mapText_[DIK_Y] = L"Y";
	mapText_[DIK_U] = L"U";
	mapText_[DIK_I] = L"I";
	mapText_[DIK_O] = L"O";
	mapText_[DIK_P] = L"P";
	mapText_[DIK_LBRACKET] = L"[";
	mapText_[DIK_RBRACKET] = L"]";
	mapText_[DIK_RETURN] = L"Enter";
	mapText_[DIK_LCONTROL] = L"Ctrl (Left)";

	mapText_[DIK_A] = L"A";
	mapText_[DIK_S] = L"S";
	mapText_[DIK_D] = L"D";
	mapText_[DIK_F] = L"F";
	mapText_[DIK_G] = L"G";
	mapText_[DIK_H] = L"H";
	mapText_[DIK_J] = L"J";
	mapText_[DIK_K] = L"K";
	mapText_[DIK_L] = L"L";
	mapText_[DIK_SEMICOLON] = L";";
	mapText_[DIK_APOSTROPHE] = L"'";
	mapText_[DIK_GRAVE] = L"`";
	mapText_[DIK_LSHIFT] = L"Shift (Left)";
	mapText_[DIK_BACKSLASH] = L"\\";

	mapText_[DIK_Z] = L"Z";
	mapText_[DIK_X] = L"X";
	mapText_[DIK_C] = L"C";
	mapText_[DIK_V] = L"V";
	mapText_[DIK_B] = L"B";
	mapText_[DIK_N] = L"N";
	mapText_[DIK_M] = L"M";
	mapText_[DIK_COMMA] = L",";
	mapText_[DIK_PERIOD] = L".";
	mapText_[DIK_SLASH] = L"/";
	mapText_[DIK_RSHIFT] = L"Shift (Right)";
	mapText_[DIK_MULTIPLY] = L"* (Numpad)";
	mapText_[DIK_LMENU] = L"Alt (Left)";
	mapText_[DIK_SPACE] = L"Space";
	mapText_[DIK_CAPITAL] = L"Caps Lock";

	mapText_[DIK_F1] = L"F1";
	mapText_[DIK_F2] = L"F2";
	mapText_[DIK_F3] = L"F3";
	mapText_[DIK_F4] = L"F4";
	mapText_[DIK_F5] = L"F5";
	mapText_[DIK_F6] = L"F6";
	mapText_[DIK_F7] = L"F7";
	mapText_[DIK_F8] = L"F8";
	mapText_[DIK_F9] = L"F9";
	mapText_[DIK_F10] = L"F10";

	mapText_[DIK_NUMLOCK] = L"Num Lock";
	mapText_[DIK_SCROLL] = L"Scroll Lock";

	mapText_[DIK_NUMPAD7] = L"7 (Numpad)";
	mapText_[DIK_NUMPAD8] = L"8 (Numpad)";
	mapText_[DIK_NUMPAD9] = L"9 (Numpad)";
	mapText_[DIK_SUBTRACT] = L"- (Numpad)";
	mapText_[DIK_NUMPAD4] = L"4 (Numpad)";
	mapText_[DIK_NUMPAD5] = L"5 (Numpad)";
	mapText_[DIK_NUMPAD6] = L"6 (Numpad)";
	mapText_[DIK_ADD] = L"+ (Numpad)";
	mapText_[DIK_NUMPAD1] = L"1 (Numpad)";
	mapText_[DIK_NUMPAD2] = L"2 (Numpad)";
	mapText_[DIK_NUMPAD3] = L"3 (Numpad)";
	mapText_[DIK_NUMPAD0] = L"0 (Numpad)";
	mapText_[DIK_DECIMAL] = L". (Numpad)";

	mapText_[DIK_F11] = L"F11";
	mapText_[DIK_F12] = L"F12";
	mapText_[DIK_F13] = L"F13";//NEC PC-98
	mapText_[DIK_F14] = L"F14";//NEC PC-98
	mapText_[DIK_F15] = L"F15";//NEC PC-98

	mapText_[DIK_KANA] = L"カナ";//日本語キーボード
	mapText_[DIK_CONVERT] = L"変換";//日本語キーボード
	mapText_[DIK_NOCONVERT] = L"無変換";//日本語キーボード
	mapText_[DIK_YEN] = L"\\";//日本語キーボード
	mapText_[DIK_NUMPADEQUALS] = L"(Numpad)";//NEC PC-98
	mapText_[DIK_CIRCUMFLEX] = L"^";//日本語キーボード

	mapText_[DIK_AT] = L"@";//NEC PC-98
	mapText_[DIK_COLON] = L":";//NEC PC-98
	mapText_[DIK_UNDERLINE] = L"_";//NEC PC-98
	mapText_[DIK_KANJI] = L"漢字";//日本語キーボード
	mapText_[DIK_STOP] = L"Stop";//NEC PC-98
	mapText_[DIK_AX] = L"(Japan AX)";
	mapText_[DIK_UNLABELED] = L"(J3100)";

	mapText_[DIK_NUMPADENTER] = L"Enter (Numpad)";
	mapText_[DIK_RCONTROL] = L"Ctrl (Right)";
	mapText_[DIK_NUMPADCOMMA] = L", (Numpad)";//NEC PC-98
	mapText_[DIK_DIVIDE] = L"/ (Numpad)";
	mapText_[DIK_SYSRQ] = L"Sys Rq";
	mapText_[DIK_RMENU] = L"Alt (Right)";
	mapText_[DIK_PAUSE] = L"Pause";
	mapText_[DIK_HOME] = L"Home";
	mapText_[DIK_UP] = L"↑";
	mapText_[DIK_PRIOR] = L"Page Up";
	mapText_[DIK_LEFT] = L"←";
	mapText_[DIK_RIGHT] = L"→";
	mapText_[DIK_END] = L"End";
	mapText_[DIK_DOWN] = L"↓";
	mapText_[DIK_NEXT] = L"Page Down";

	mapText_[DIK_INSERT] = L"Insert";
	mapText_[DIK_DELETE] = L"Delete";
	mapText_[DIK_LWIN] = L"Windows (Left)";
	mapText_[DIK_RWIN] = L"Windows (Right)";
	mapText_[DIK_APPS] = L"Menu";
	mapText_[DIK_POWER] = L"Power";
	mapText_[DIK_SLEEP] = L"Sleep";

	std::map<int, std::wstring>::iterator itr = mapText_.begin();
	for (; itr != mapText_.end(); itr++) {
		int code = itr->first;
		listValidCode_.push_back(code);
	}
}
KeyCodeList::~KeyCodeList() {
}
bool KeyCodeList::IsValidCode(int code) {
	bool res = mapText_.find(code) != mapText_.end();
	return res;
}
std::wstring KeyCodeList::GetCodeText(int code) {
	if (!IsValidCode(code))return L"";

	std::wstring res = mapText_[code];
	return res;
}

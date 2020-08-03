#pragma once
#include "../../GcLib/pch.h"

#include "../Common/DnhCommon.hpp"
#include "GcLibImpl.hpp"

/**********************************************************
//KeyCodeList
**********************************************************/
class KeyCodeList {
private:
	std::map<int16_t, std::wstring> mapText_;
public:
	KeyCodeList();
	virtual ~KeyCodeList();

	bool IsValidCode(int16_t code);
	std::wstring GetCodeText(int16_t code);
	const std::map<int16_t, std::wstring>& GetCodeMap() { return mapText_; }
};
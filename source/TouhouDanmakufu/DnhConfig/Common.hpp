#pragma once
#include "../../GcLib/pch.h"

#include "../Common/DnhCommon.hpp"
#include "GcLibImpl.hpp"

#include "ControllerMap.hpp"

//*******************************************************************
//KeyCodeList
//*******************************************************************
class KeyCodeList {
private:
	std::map<int16_t, std::string> mapText_;
public:
	KeyCodeList();

	bool IsValidCode(int16_t code);
	std::string GetCodeText(int16_t code);

	const std::map<int16_t, std::string>& GetCodeMap() { return mapText_; }
};
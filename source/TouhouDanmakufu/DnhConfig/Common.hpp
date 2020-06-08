#pragma once
#include "../../GcLib/pch.h"

#include "../Common/DnhCommon.hpp"
#include "GcLibImpl.hpp"

/**********************************************************
//KeyCodeList
**********************************************************/
class KeyCodeList {
private:
	std::map<int, std::wstring> mapText_;
	std::vector<int> listValidCode_;
public:
	KeyCodeList();
	virtual ~KeyCodeList();

	bool IsValidCode(int code);
	std::wstring GetCodeText(int code);
	std::vector<int>& GetValidCodeList() { return listValidCode_; }
};
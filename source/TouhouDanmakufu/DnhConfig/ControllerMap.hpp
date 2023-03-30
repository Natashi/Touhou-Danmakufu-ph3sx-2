#pragma once
#include "../../GcLib/pch.h"

#include "Constant.hpp"

//*******************************************************************
//ControllerMap
//*******************************************************************
class ControllerMap {
public:
	static std::vector<const ControllerMap*> listSupportedControllerMaps;
	static const ControllerMap* GetControllerMap(const DirectInput::InputDeviceHeader* pJoypad) {
		for (auto i : listSupportedControllerMaps) {
			if (i->Detect(pJoypad))
				return i;
		}
		return nullptr;
	}
public:
	const char* name;
	std::vector<uint32_t> listVidPid;

	std::unordered_map<int, const char*> mapButton;
public:
	ControllerMap() {}
	ControllerMap(const char* name, const std::initializer_list<uint32_t>& listVidPid, const char* buttonMap);
	ControllerMap(const char* name, const std::initializer_list<uint32_t>& listVidPid, const char* buttonMap, 
		const std::unordered_map<int, const char*>& extra, 
		const std::unordered_map<std::string, const char*>& rename);

	virtual bool Detect(const DirectInput::InputDeviceHeader* pJoypad) const;

	static std::string GetNameFromButton(const ControllerMap* pMap, int button);

	static void Initialize();
};

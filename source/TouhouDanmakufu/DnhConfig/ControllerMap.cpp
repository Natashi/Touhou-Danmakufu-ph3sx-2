#include "source/GcLib/pch.h"

#include "ControllerMap.hpp"

#define DEF_BUTTON(_name, _str) constexpr const char* csb_##_name = _str;
	
	DEF_BUTTON(AnalogU, "LStick Up");
	DEF_BUTTON(AnalogD, "LStick Down");
	DEF_BUTTON(AnalogL, "LStick Left");
	DEF_BUTTON(AnalogR, "LStick Right");

	DEF_BUTTON(DPadU, "D-Pad Up");
	DEF_BUTTON(DPadD, "D-Pad Down");
	DEF_BUTTON(DPadL, "D-Pad Left");
	DEF_BUTTON(DPadR, "D-Pad Right");
	
	DEF_BUTTON(A, "A");
	DEF_BUTTON(B, "B");
	DEF_BUTTON(X, "X");
	DEF_BUTTON(Y, "Y");

	DEF_BUTTON(PS_Cir, "Circle");	// A
	DEF_BUTTON(PS_Cro, "Cross");	// B
	DEF_BUTTON(PS_Tri, "Triangle");	// X
	DEF_BUTTON(PS_Sq, "Square");	// Y

	DEF_BUTTON(LB, "LButton");
	DEF_BUTTON(RB, "RButton");
	DEF_BUTTON(LT, "LTrigger");		// LT/RT on some devices aren't analog
	DEF_BUTTON(RT, "RTrigger");
	DEF_BUTTON(L3, "LThumb");
	DEF_BUTTON(R3, "RThumb");

	DEF_BUTTON(Back, "Back");
	DEF_BUTTON(Start, "Start");
	DEF_BUTTON(Select, "Select");
	DEF_BUTTON(Home, "Home");
	DEF_BUTTON(System, "System");

//*******************************************************************
//ControllerMap
//*******************************************************************
static std::unordered_map<std::string, const char*> mapDefaultName = {
	{"dpup", csb_DPadU}, {"dpdown", csb_DPadD}, {"dpleft", csb_DPadL}, {"dpright", csb_DPadR},
	{"a", csb_A}, {"b", csb_B}, {"x", csb_X}, {"y", csb_Y},
	{"leftshoulder", csb_LB}, {"rightshoulder", csb_RB},
	{"lefttrigger", csb_LT}, {"righttrigger", csb_LT},
	{"leftstick", csb_L3}, {"rightstick", csb_R3},
	{"back", csb_Back}, {"start", csb_Start},
	{"guide", "Guide"},
	{"touchpad", "Touchpad"},
};
ControllerMap::ControllerMap(const char* name, const std::initializer_list<uint32_t>& listVidPid, const char* buttonMap)
	: ControllerMap(name, listVidPid, buttonMap, {}, {}) {
}
ControllerMap::ControllerMap(const char* name, const std::initializer_list<uint32_t>& listVidPid, const char* buttonMap, 
	const std::unordered_map<int, const char*>& extra, const std::unordered_map<std::string, const char*>& rename)
{
	this->name = name;
	this->listVidPid.insert(this->listVidPid.begin(), listVidPid.begin(), listVidPid.end());

	for (auto& iButton : StringUtility::Split(buttonMap, ",")) {
		auto split = StringUtility::Split(iButton, ":");
		auto& buttonName = split[0];
		int buttonID = StringUtility::ToInteger(split[1]);

		auto itrRename = extra.find(buttonID);
		if (itrRename != extra.end()) {
			mapButton[buttonID] = itrRename->second;
		}
		else {
			auto itrName = mapDefaultName.find(buttonName);
			if (itrName != mapDefaultName.end()) {
				auto itrRename = rename.find(buttonName);
				mapButton[buttonID] = itrRename != rename.end() 
					? itrRename->second : itrName->second;
			}
			else {
				Logger::WriteTop(StringUtility::Format("Unknown button -> %d:%s", 
					buttonID, buttonName.c_str()));
			}
		}
	}
}

bool ControllerMap::Detect(const DirectInput::InputDeviceHeader* pJoypad) const {
	uint32_t guid = ((uint32_t)pJoypad->vendorID << 16) | (uint32_t)pJoypad->productID;
	for (auto& i : listVidPid) {
		if (guid == i)
			return true;
	}
	return false;
}

std::string ControllerMap::GetNameFromButton(const ControllerMap* pMap, int button) {
	if (button < 0) return "[Invalid]";
	else if (button < 4) {
		static const char* table[4] = { 
			csb_AnalogL, csb_AnalogR, csb_AnalogU, csb_AnalogD
		};
		return table[button];
	}
	else if (button < 8) {
		static const char* table[4] = {
			csb_DPadL, csb_DPadR, csb_DPadU, csb_DPadD
		};
		return table[button - 4];
	}

	if (pMap) {
		auto itr = pMap->mapButton.find(button - 8);
		if (itr != pMap->mapButton.end())
			return itr->second;
		//return StringUtility::Format("Button%02d", button - 8);
	}
	return "";
}

std::vector<const ControllerMap*> ControllerMap::listSupportedControllerMaps = {};
void ControllerMap::Initialize() {
	if (listSupportedControllerMaps.size() > 0)
		return;

	// Data obtained from:
	//    - https://github.com/gabomdq/SDL_GameControllerDB
	//    - https://github.com/pbhogan/InControl

	// TODO: Handle button renames for controllers with the "ABCXYZ" layout (like Sega Saturn)

#define ADDC(...) listSupportedControllerMaps.push_back(new ControllerMap(__VA_ARGS__))		// Fuck C++ macros

	ADDC("8BitDo F30", { 0x80001002 }, "a:0,b:1,back:10,leftshoulder:6,lefttrigger:8,rightshoulder:7,righttrigger:9,start:11,x:3,y:4");

	ADDC("8BitDo F30 Pro", { 0x2dc83810 },
		"a:1,b:0,back:10,guide:2,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");
	ADDC("8BitDo FC30 Pro", { 0x2dc89000 },
		"a:1,b:0,back:10,guide:2,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");
	ADDC("8BitDo Lite 2", { 0x2dc85112 },
		"a:1,b:0,back:10,guide:12,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");
	ADDC("8BitDo Lite SE", { 0x2dc85111 },
		"a:1,b:0,back:10,guide:12,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");

	ADDC("8BitDo M30", { 0x2dc85001 }, "a:1,b:0,back:10,rightshoulder:6,righttrigger:7,start:11,x:4,y:3");
	ADDC("8BitDo M30", { 0x2dc85101 }, "a:0,b:1,back:10,rightshoulder:6,righttrigger:7,start:11,x:3,y:4");
	ADDC("8BitDo M30", { 0x2dc85006, 0x2dc80651 },
		"a:0,b:1,back:10,guide:2,leftshoulder:8,lefttrigger:9,rightshoulder:6,righttrigger:7,start:11,x:3,y:4");

	ADDC("8BitDo N30", { 0x2dc81003, 0x2dc81080 }, "a:0,b:1,back:10,leftshoulder:6,rightshoulder:7,start:11,x:3,y:4");
	ADDC("8BitDo N30", { 0x2dc82820 },
		"a:1,b:0,back:10,guide:2,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");

	ADDC("8BitDo N30 Pro", { 0x2dc89001, 0x2dc89015, 0x2dc82865 },
		"a:1,b:0,back:10,guide:2,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");

	ADDC("8BitDo NES30", { 0x1235ab12 }, "a:2,b:1,back:6,leftshoulder:4,rightshoulder:5,start:7,x:3,y:0");
	ADDC("8BitDo NES30", { 0x2dc8ab12 }, "a:1,b:0,back:10,leftshoulder:6,rightshoulder:7,start:11,x:4,y:3");
	ADDC("8BitDo NES30 Pro", { 0x20029000, 0x38200009 },
		"a:1,b:0,back:10,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");
	ADDC("8BitDo NES30 Pro", { 0x2dc83820 },
		"a:1,b:0,back:10,guide:2,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");

	ADDC("8BitDo P30", { 0x2dc85107, 0x2dc85108 }, "a:0,b:1,back:10,leftshoulder:6,lefttrigger:8,rightshoulder:7,righttrigger:9,start:11,x:3,y:4");
	ADDC("8BitDo Pro 2", { 0x2dc86003, 0x2dc86103, 0x2dc86006 },
		"a:1,b:0,back:10,guide:12,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");
	ADDC("8BitDo Receiver", { 0x2dc83101, 0x2dc83102, 0x2dc83103, 0x2dc83104 },
		"a:1,b:0,back:10,guide:2,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");

	ADDC("8BitDo S30", { 0x2dc86728 },
		"a:0,b:1,leftshoulder:8,lefttrigger:9,rightshoulder:6,righttrigger:7,start:10,x:3,y:4");
	ADDC("8BitDo SF30", { 0x2dc83001 },
		"a:1,b:0,back:10,guide:2,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");
	ADDC("8BitDo SF30 Pro", { 0x2dc86000 },
		"a:1,b:0,back:10,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");
	ADDC("8BitDo SF30 Pro", { 0x2dc86100 },
		"a:1,b:0,back:10,guide:2,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");

	ADDC("8BitDo SFC30", { 0x28100009, 0x2dc8ab21, 0x2dc82830 },
		"a:1,b:0,back:10,leftshoulder:6,rightshoulder:7,start:11,x:4,y:3");
	ADDC("8BitDo SN30", { 0x1235ab20 }, "a:0,b:1,back:10,leftshoulder:4,lefttrigger:6,rightshoulder:5,righttrigger:7,start:11,x:4,y:3");
	ADDC("8BitDo SN30", { 0x2dc83000, 0x2dc85103, 0x2dc89012, 0x2dc82862 },
		"a:1,b:0,back:10,leftshoulder:6,rightshoulder:7,start:11,x:4,y:3");
	ADDC("8BitDo SN30", { 0x2dc8ab20, 0x2dc82840 },
		"a:1,b:0,back:10,guide:2,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");
	ADDC("8BitDo SN30 Pro", { 0x2dc82100 },
		"a:0,b:1,back:10,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:3,y:4");
	ADDC("8BitDo SN30 Pro", { 0x2dc86001, 0x2dc86101 },
		"a:1,b:0,back:10,guide:2,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");
	ADDC("8BitDo SN30 Pro Plus", { 0x2dc86002, 0x2dc86102 },
		"a:1,b:0,back:10,guide:2,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:4,y:3");

	ADDC("8BitDo Zero", { 0x05a03232 }, "a:0,b:1,back:10,leftshoulder:6,rightshoulder:7,start:11,x:3,y:4");
	ADDC("8BitDo Zero 2", { 0x2dc89018, 0x2dc83230 }, "a:1,b:0,back:10,leftshoulder:6,rightshoulder:7,start:11,x:4,y:3");

	//---------------------------------------------------------------------

	ADDC("DualShock 2", { 0x0e8f1009 },
		"a:2,b:1,back:8,leftshoulder:6,leftstick:9,lefttrigger:4,rightshoulder:7,rightstick:10,righttrigger:5,start:11,x:3,y:0");
	ADDC("DualShock 3", { 0x73310001 },
		"a:0,b:1,back:10,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:3,y:4");

	//---------------------------------------------------------------------

	ADDC("Logitech F310", { 0x046dc21d },
		"a:0,b:1,back:6,leftshoulder:4,leftstick:8,rightshoulder:5,rightstick:9,start:7,x:2,y:3");
	ADDC("Logitech F510", { 0x046dc218 },
		"a:1,b:2,back:8,leftshoulder:4,leftstick:10,lefttrigger:6,rightshoulder:5,rightstick:11,righttrigger:7,start:9,x:0,y:3");
	ADDC("Logitech F510", { 0x046dc21e },
		"a:0,b:1,back:6,leftshoulder:4,leftstick:8,rightshoulder:5,rightstick:9,start:7,x:2,y:3");
	ADDC("Logitech F710", { 0x046dc219 },
		"a:1,b:2,back:8,leftshoulder:4,leftstick:10,lefttrigger:6,rightshoulder:5,rightstick:11,righttrigger:7,start:9,x:0,y:3");
	ADDC("Logitech F710", { 0x046dc21f },
		"a:0,b:1,back:6,leftshoulder:4,leftstick:8,rightshoulder:5,rightstick:9,start:7,x:2,y:3");

	//---------------------------------------------------------------------

	ADDC("NVIDIA Controller", { 0x09557214 }, "a:11,b:10,back:13,guide:12,leftshoulder:7,leftstick:5,rightshoulder:6,rightstick:4,start:3,x:9,y:8");
	ADDC("NVIDIA Shield", { 0x09557210 }, "a:9,b:8,back:11,leftshoulder:5,leftstick:3,rightshoulder:4,rightstick:2,start:0,x:7,y:6",
		{ {12, csb_System}, {10, csb_Home} }, {});

	ADDC("OUYA Controller", { 0x28360001 },
		"a:0,b:3,dpdown:9,dpleft:10,dpright:11,dpup:8,guide:14,leftshoulder:4,leftstick:6,rightshoulder:5,rightstick:7,righttrigger:13,x:1,y:2",
		{}, { {"a", "O"}, {"b", "U"}, {"x", "Y"}, {"y", "A"} });

	//---------------------------------------------------------------------

	static const std::unordered_map<std::string, const char*> rename_PS = {
		{"a", csb_PS_Cir}, {"b", csb_PS_Cro}, {"x", csb_PS_Tri}, {"y", csb_PS_Sq}
	};
#define ADDC_PS(...) listSupportedControllerMaps.push_back(new ControllerMap(__VA_ARGS__, {}, rename_PS))

	ADDC_PS("PlayStation Classic Controller", { 0x054c0cda },
		"a:2,b:1,back:8,leftshoulder:6,lefttrigger:4,rightshoulder:7,righttrigger:5,start:9,x:3,y:0");
	ADDC_PS("PlayStation Controller", { 0x25630623 },
		"a:0,b:1,back:10,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:3,y:4");
	ADDC_PS("PlayStation Controller", { 0x25f083c1 },
		"a:1,b:2,back:8,guide:12,leftshoulder:4,leftstick:10,lefttrigger:6,rightshoulder:5,rightstick:11,righttrigger:7,start:9,x:0,y:3");

	ADDC_PS("PlayStation Vita", { 0x054c1337 }, "a:1,b:2,back:8,dpdown:13,dpleft:15,dpright:14,dpup:12,leftshoulder:4,rightshoulder:5,start:9,x:0,y:3");

	ADDC_PS("PS1 Controller", { 0x08100001 }, "a:2,b:1,back:8,leftshoulder:6,leftstick:10,lefttrigger:4,rightshoulder:7,rightstick:11,righttrigger:5,start:9,x:3,y:0");
	ADDC_PS("PS1 Controller", { 0x0e8f3075 }, "b:2,back:8,guide:12,leftshoulder:4,leftstick:10,lefttrigger:6,rightshoulder:5,rightstick:11,righttrigger:1,start:9,x:0,y:3");
	
	ADDC_PS("PS2 Controller", { 0x08100003 }, "a:2,b:1,back:8,leftshoulder:6,leftstick:10,lefttrigger:4,rightshoulder:7,rightstick:11,righttrigger:5,start:9,x:3,y:0");
	ADDC_PS("PS2 Controller", { 0x09258800, 0x09258888 }, "a:2,b:1,back:9,leftshoulder:6,leftstick:10,lefttrigger:4,rightshoulder:7,rightstick:11,righttrigger:5,start:8,x:3,y:0");
	ADDC_PS("PS2 Controller", { 0x09258868 }, "a:2,b:1,back:9,leftshoulder:5,leftstick:10,lefttrigger:4,rightshoulder:7,rightstick:11,righttrigger:6,start:8,x:3,y:0");
	ADDC_PS("PS2 Controller", { 0x146b0303 }, "a:0,b:1,back:8,guide:12,leftshoulder:4,leftstick:10,lefttrigger:6,rightshoulder:5,rightstick:11,righttrigger:7,start:9,x:2,y:3");
	ADDC_PS("PS2 Controller", { 0x0d9d3013 }, "a:0,b:1,back:8,leftshoulder:4,leftstick:10,lefttrigger:5,rightshoulder:6,rightstick:11,righttrigger:7,start:9,x:2,y:3");

	ADDC_PS("PS3 Controller", { 0x0a120001 }, "a:0,b:1,back:10,leftshoulder:6,leftstick:13,lefttrigger:8,rightshoulder:7,rightstick:14,righttrigger:9,start:11,x:3,y:4");
	ADDC_PS("PS3 Controller", { 0x0c120713, 0x0c12f11c, 0x0c120ef9 }, "a:1,b:2,back:8,leftshoulder:4,leftstick:10,rightshoulder:5,rightstick:11,start:9,x:0,y:3");
	ADDC_PS("PS3 Controller", { 0x09251801, 0x09251802 }, "a:2,b:1,back:9,leftshoulder:6,leftstick:10,lefttrigger:4,rightshoulder:7,rightstick:11,righttrigger:5,start:8,x:3,y:0");
	ADDC_PS("PS3 Controller", { 0x09250005 }, "a:2,b:1,back:9,leftshoulder:6,leftstick:10,lefttrigger:4,rightshoulder:7,rightstick:11,righttrigger:5,start:8,x:0,y:3");
	ADDC_PS("PS3 Controller", { 0x054c0268 }, "a:2,b:1,back:9,guide:12,leftshoulder:6,leftstick:10,rightshoulder:7,rightstick:11,start:8,x:3,y:0");
	ADDC_PS("PS3 Controller", { 0x1f4f0008 }, "a:1,b:2,back:8,leftshoulder:4,lefttrigger:6,rightshoulder:5,righttrigger:7,start:9,x:0,y:3");
	ADDC_PS("PS3 Controller", { 0x25630575 }, "a:2,b:1,back:8,guide:12,leftshoulder:4,leftstick:10,lefttrigger:6,rightshoulder:5,rightstick:11,righttrigger:7,start:9,x:3,y:0");
	ADDC_PS("PS3 Controller", { 0x88880308 }, "a:2,b:1,back:8,guide:12,leftshoulder:4,leftstick:9,lefttrigger:6,rightshoulder:5,rightstick:10,righttrigger:7,start:11,x:3,y:0");
	ADDC_PS("PS3 Controller", { 0x88880408 }, "a:14,b:13,back:0,dpdown:6,dpleft:7,dpright:5,dpup:4,guide:16,leftshoulder:10,leftstick:1,lefttrigger:8,rightshoulder:11,rightstick:2,righttrigger:9,start:3,x:15,y:12");
	ADDC_PS("PS3 Controller", { 0x0e8f0003 }, "a:2,b:1,back:8,leftshoulder:4,leftstick:10,lefttrigger:6,rightshoulder:5,rightstick:11,righttrigger:7,start:9,x:3,y:0");
	ADDC_PS("PS3 Controller", { 0x0e8f3114 }, "a:1,b:2,back:8,guide:12,leftshoulder:4,leftstick:10,lefttrigger:6,rightshoulder:5,rightstick:11,righttrigger:7,start:9,x:0,y:3");
	ADDC_PS("PS3 Controller", { 0x22ba1020 }, "a:0,b:1,back:8,leftshoulder:4,leftstick:10,lefttrigger:6,rightshoulder:5,rightstick:11,righttrigger:7,start:9,x:3,y:2");
	
	ADDC_PS("PS4 Controller", { 0x0c120708, 0x0c121e11, 0x0c121e12, 0x0c120e13, 0x0c120e15, 0x0c120e18, 0x0c121e18, 0x0c121e19, 0x0c120e1e, 0x0c1257a9, 0x0c1257aa, 0x0c121cf2, 0x0c121cf3, 0x0c121cf4, 0x0c121cf5, 0x0c120ef7, 0x0e120c12, 0x0e160c12, 0x1e1a0c12 }, 
		"a:1,b:2,back:8,leftshoulder:4,leftstick:10,rightshoulder:5,rightstick:11,start:9,x:0,y:3");
	ADDC_PS("PS4 Controller", { 0x054c0ba0, 0x054c09cc }, "a:1,b:2,back:8,guide:12,leftshoulder:4,leftstick:10,rightshoulder:5,rightstick:11,start:9,x:0,y:3");
	ADDC_PS("PS4 Controller", { 0x054c05c4 }, "a:1,b:2,back:8,guide:12,leftshoulder:4,leftstick:10,rightshoulder:5,rightstick:11,start:9,touchpad:13,x:0,y:3");
	
	ADDC_PS("PS5 Controller", { 0x054c0ce6, 0x054c0df2 }, "a:1,b:2,back:8,guide:12,leftshoulder:4,leftstick:10,misc1:14,rightshoulder:5,rightstick:11,start:9,touchpad:13,x:0,y:3");

	ADDC_PS("PSX", { 0x05832050 }, "a:0,b:1,back:10,leftshoulder:4,leftstick:8,lefttrigger:6,rightshoulder:5,rightstick:9,righttrigger:7,start:11,x:2,y:3");

	//---------------------------------------------------------------------

	ADDC("Sega Saturn Controller", { 0xf0000008 }, "a:1,b:2,rightshoulder:7,righttrigger:3,start:0,x:5,y:6");
	ADDC("Sega Saturn Controller", { 0x07730106 }, "a:0,b:1,leftshoulder:6,lefttrigger:7,rightshoulder:5,righttrigger:2,start:9,x:3,y:4");
	ADDC("Sega Saturn Controller", { 0x04b4010a }, "a:0,b:1,leftshoulder:6,lefttrigger:7,rightshoulder:5,righttrigger:2,start:8,x:3,y:4");
	ADDC("SNES Controller", { 0x040397c1, 0x17810a9d, 0x288b0003, 0x12925346 }, "a:0,b:4,back:2,leftshoulder:6,rightshoulder:7,start:3,x:1,y:5");
	ADDC("SNES Controller", { 0x17810a96 }, "a:4,b:0,back:2,leftshoulder:6,rightshoulder:7,start:3,x:5,y:1");

	//---------------------------------------------------------------------

	ADDC("Xbox 360 Controller", { 0x162ebeef }, "a:0,b:1,back:6,leftshoulder:4,lefttrigger:10,rightshoulder:5,righttrigger:11,start:7,x:2,y:3");
	ADDC("Xbox 360 Controller", { 0x07384716, 0x0738b726, 0x045e0719, 0x045e028e, 0x045e0291, 0x1badfd01, 0x1badf016, 0x1bad028e, 0x24c6fafd }, 
		"a:0,b:1,back:6,leftshoulder:4,leftstick:8,rightshoulder:5,rightstick:9,start:7,x:2,y:3");
	ADDC("Xbox 360 Controller", { 0x07384520 }, 
		"a:1,b:2,back:8,leftshoulder:4,leftstick:10,lefttrigger:6,rightshoulder:5,rightstick:11,righttrigger:7,start:9,x:0,y:3");
	ADDC("Xbox 360 Controller", { 0x07384426 }, 
		"a:0,b:1,back:6,leftshoulder:4,leftstick:7,lefttrigger:10,rightshoulder:5,rightstick:9,righttrigger:11,start:8,x:2,y:3");
	ADDC("Xbox 360 Controller", { 0x07384726, 0x07384736, 0x1badfd00, 0x24c65300 }, 
		"a:0,b:1,back:6,leftshoulder:4,leftstick:8,lefttrigger:10,rightshoulder:5,rightstick:9,righttrigger:11,start:7,x:2,y:3");
	
	ADDC("Xbox Controller", { 0x0c12880a }, 
		"a:0,b:1,back:7,leftshoulder:4,leftstick:8,lefttrigger:10,rightshoulder:5,rightstick:9,righttrigger:11,start:6,x:2,y:3");
	ADDC("Xbox Controller", { 0x0c128810, 0x07384586, 0x045e0285 }, 
		"a:0,b:1,back:6,leftshoulder:4,leftstick:8,rightshoulder:5,rightstick:9,start:7,x:2,y:3");
	ADDC("Xbox Controller", { 0x062a0020 }, 
		"a:0,b:1,back:13,dpdown:9,dpleft:10,dpright:11,dpup:8,leftshoulder:5,leftstick:14,lefttrigger:6,rightshoulder:4,rightstick:15,righttrigger:7,start:12,x:2,y:3");
	ADDC("Xbox Controller", { 0x0f308888 }, 
		"a:0,b:1,back:7,dpdown:13,dpleft:10,dpright:11,dpup:12,leftshoulder:5,leftstick:8,lefttrigger:10,rightshoulder:4,rightstick:9,righttrigger:11,start:6,x:2,y:3");
	ADDC("Xbox Controller", { 0x07384516, 0x07384536, 0x045e0202 }, 
		"a:0,b:1,back:7,leftshoulder:5,leftstick:8,lefttrigger:10,rightshoulder:4,rightstick:9,righttrigger:11,start:6,x:2,y:3");
	ADDC("Xbox Controller", { 0x07384526 }, 
		"a:0,b:1,back:6,leftshoulder:4,leftstick:8,lefttrigger:10,rightshoulder:5,rightstick:9,righttrigger:11,start:7,x:2,y:3");
	ADDC("Xbox Controller", { 0x045e0287 }, 
		"a:0,b:1,back:7,leftshoulder:5,leftstick:8,lefttrigger:10,rightshoulder:4,rightstick:9,righttrigger:7,start:6,x:2,y:3");
	ADDC("Xbox Controller", { 0x045e0289 }, 
		"a:0,b:1,back:7,leftshoulder:10,leftstick:8,lefttrigger:5,rightshoulder:11,rightstick:9,righttrigger:4,start:6,x:2,y:3");
	
	ADDC("Xbox One Controller", { 0x0f0d0063 }, 
		"a:0,b:1,back:6,leftshoulder:4,leftstick:8,lefttrigger:8,rightshoulder:5,rightstick:9,righttrigger:9,start:7,x:2,y:3");
	ADDC("Xbox One Controller", { 0x045e0b0c, 0x045e02d1, 0x045e02dd, 0x045e02e0, 0x045e02e3, 0x045e02ea, 0x045e02fd, 0x045e02ff, 0x0e6f02a8, 0x24c6543a }, 
		"a:0,b:1,back:6,leftshoulder:4,leftstick:8,rightshoulder:5,rightstick:9,start:7,x:2,y:3");
	ADDC("Xbox One Controller", { 0x0e6f02c8 }, 
		"a:0,b:1,back:6,leftshoulder:4,leftstick:9,rightshoulder:5,rightstick:10,start:7,x:2,y:3");

#undef ADDC_PS
#undef ADDC
}

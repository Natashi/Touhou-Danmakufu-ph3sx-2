#include "source/GcLib/pch.h"

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)

#include "DnhConfiguration.hpp"
#include "DnhGcLibImpl.hpp"

//*******************************************************************
//DnhConfiguration
//*******************************************************************
DnhConfiguration::DnhConfiguration() {
	modeScreen_ = ScreenMode::SCREENMODE_WINDOW;
	modeColor_ = ColorMode::COLOR_MODE_32BIT;

	fpsStandard_ = 60;
	fpsType_ = FPS_NORMAL;
	fastModeSpeed_ = 20;

	windowSizeIndex_ = 0;

	bVSync_ = true;
	bUseRef_ = false;
	bPseudoFullscreen_ = true;
	multiSamples_ = D3DMULTISAMPLE_NONE;

	pathExeLaunch_ = DNH_EXE_NAME;

	padIndex_ = 0;
	mapKey_[EDirectInput::KEY_LEFT] = new VirtualKey(DIK_LEFT, 0, 0);
	mapKey_[EDirectInput::KEY_RIGHT] = new VirtualKey(DIK_RIGHT, 0, 1);
	mapKey_[EDirectInput::KEY_UP] = new VirtualKey(DIK_UP, 0, 2);
	mapKey_[EDirectInput::KEY_DOWN] = new VirtualKey(DIK_DOWN, 0, 3);

	mapKey_[EDirectInput::KEY_OK] = new VirtualKey(DIK_Z, 0, 5);
	mapKey_[EDirectInput::KEY_CANCEL] = new VirtualKey(DIK_X, 0, 6);

	mapKey_[EDirectInput::KEY_SHOT] = new VirtualKey(DIK_Z, 0, 5);
	mapKey_[EDirectInput::KEY_BOMB] = new VirtualKey(DIK_X, 0, 6);
	mapKey_[EDirectInput::KEY_SLOWMOVE] = new VirtualKey(DIK_LSHIFT, 0, 7);
	mapKey_[EDirectInput::KEY_USER1] = new VirtualKey(DIK_C, 0, 8);
	mapKey_[EDirectInput::KEY_USER2] = new VirtualKey(DIK_V, 0, 9);

	mapKey_[EDirectInput::KEY_PAUSE] = new VirtualKey(DIK_ESCAPE, 0, 10);

	bLogWindow_ = false;
	bLogFile_ = false;
	bMouseVisible_ = true;

	screenWidth_ = 640;
	screenHeight_ = 480;
	windowSizeList_ = { { 640, 480 }, { 800, 600 }, { 960, 720 }, { 1280, 960 } };

	bEnableUnfocusedProcessing_ = false;

	LoadConfigFile();
	_LoadDefinitionFile();
}
DnhConfiguration::~DnhConfiguration() {}

bool DnhConfiguration::_LoadDefinitionFile() {
	PropertyFile prop;
	if (!prop.Load(PathProperty::GetModuleDirectory() + L"th_dnh.def")) return false;

	pathPackageScript_ = prop.GetString(L"package.script.main", L"");
	if (pathPackageScript_.size() > 0) {
		pathPackageScript_ = PathProperty::GetModuleDirectory() + pathPackageScript_;
		pathPackageScript_ = PathProperty::ReplaceYenToSlash(pathPackageScript_);
	}

	constexpr const LONG MIN_WD = 64;
	constexpr const LONG MIN_HT = 64;
	constexpr const LONG MAX_WD = 3840;
	constexpr const LONG MAX_HT = 2160;

	windowTitle_ = prop.GetString(L"window.title", L"");

	screenWidth_ = prop.GetInteger(L"screen.width", 640);
	screenWidth_ = std::clamp(screenWidth_, MIN_WD, MAX_WD);

	screenHeight_ = prop.GetInteger(L"screen.height", 480);
	screenHeight_ = std::clamp(screenHeight_, MIN_HT, MAX_HT);

	fpsStandard_ = prop.GetInteger(L"standard.fps", 60);
	fpsStandard_ = std::max(fpsStandard_, 1U);

	fastModeSpeed_ = prop.GetInteger(L"skip.rate", 20);
	fastModeSpeed_ = std::clamp(fastModeSpeed_, 1, 50);

	{
		std::wstring str = prop.GetString(L"unfocused.processing", L"false");
		bEnableUnfocusedProcessing_ = str == L"true" ? true : StringUtility::ToInteger(str);
	}

	{
		if (prop.HasProperty(L"window.size.list")) {
			std::wstring strList = prop.GetString(L"window.size.list", L"");
			if (strList.size() >= 3) {	//Minimum format: "0x0"
				std::wregex reg(L"([0-9]+)x([0-9]+)");
				auto itrBegin = std::wsregex_iterator(strList.begin(), strList.end(), reg);
				auto itrEnd = std::wsregex_iterator();

				if (itrBegin != itrEnd) {
					windowSizeList_.clear();

					for (auto itr = itrBegin; itr != itrEnd; ++itr) {
						const std::wsmatch& match = *itr;

						POINT size;
						size.x = wcstol(match[1].str().c_str(), nullptr, 10);
						size.y = wcstol(match[2].str().c_str(), nullptr, 10);
						size.x = std::clamp(size.x, MIN_WD, MAX_WD);
						size.y = std::clamp(size.y, MIN_HT, MAX_HT);

						windowSizeList_.push_back(size);
					}
				}
			}
		}
		if (windowSizeList_.size() == 0) {
			for (float iSizeMul : std::vector<float>({ 1, 1.25, 1.5, 2 })) {
				POINT size = { (LONG)(screenWidth_ * iSizeMul), (LONG)(screenHeight_ * iSizeMul) };
				size.x = std::clamp(size.x, MIN_WD, MAX_WD);
				size.y = std::clamp(size.y, MIN_HT, MAX_HT);

				windowSizeList_.push_back(size);
			}
		}
	}

	return true;
}

bool DnhConfiguration::LoadConfigFile() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"config.dat";

	RecordBuffer record;
	bool res = record.ReadFromFile(path, GAME_VERSION_NUM, "DNHCNFG\0", 8U);
	if (!res) return false;

	record.GetRecord<ScreenMode>("modeScreen", modeScreen_);
	record.GetRecord<ColorMode>("modeColor", modeColor_);

	record.GetRecord<int>("fpsType", fpsType_);

	record.GetRecord<size_t>("sizeWindow", windowSizeIndex_);

	record.GetRecord<bool>("bVSync", bVSync_);
	record.GetRecord<bool>("bDeviceREF", bUseRef_);
	record.GetRecord<bool>("bPseudoFullscreen", bPseudoFullscreen_);

	record.GetRecord<D3DMULTISAMPLE_TYPE>("typeMultiSamples", multiSamples_);

	pathExeLaunch_ = record.GetRecordAsStringW("pathLaunch");
	if (pathExeLaunch_.size() == 0) pathExeLaunch_ = DNH_EXE_NAME;

	if (record.IsExists("padIndex"))
		padIndex_ = record.GetRecordAsInteger("padIndex");

	{
		ByteBuffer bufKey;
		int bufKeySize = record.GetRecordAsInteger("mapKey_size");
		bufKey.SetSize(bufKeySize);
		record.GetRecord("mapKey", bufKey.GetPointer(), bufKey.GetSize());

		size_t mapKeyCount = bufKey.ReadValue<size_t>();
		if (mapKeyCount == mapKey_.size()) {
			for (size_t iKey = 0; iKey < mapKeyCount; iKey++) {
				int16_t id = bufKey.ReadShort();
				int16_t keyCode = bufKey.ReadShort();
				int16_t padIndex = bufKey.ReadShort();
				int16_t padButton = bufKey.ReadShort();

				mapKey_[id] = new VirtualKey(keyCode, padIndex, padButton);
			}
		}
	}

	bLogWindow_ = record.GetRecordAsBoolean("bLogWindow");
	bLogFile_ = record.GetRecordAsBoolean("bLogFile");
	if (record.IsExists("bMouseVisible"))
		bMouseVisible_ = record.GetRecordAsBoolean("bMouseVisible");

	return res;
}
bool DnhConfiguration::SaveConfigFile() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"config.dat";

	RecordBuffer record;

	record.SetRecord<ScreenMode>("modeScreen", modeScreen_);
	record.SetRecord<ColorMode>("modeColor", modeColor_);

	record.SetRecordAsInteger("fpsType", fpsType_);

	record.SetRecord<size_t>("sizeWindow", windowSizeIndex_);

	record.SetRecordAsBoolean("bVSync", bVSync_);
	record.SetRecordAsBoolean("bDeviceREF", bUseRef_);
	record.SetRecordAsBoolean("bPseudoFullscreen", bPseudoFullscreen_);

	record.SetRecord<D3DMULTISAMPLE_TYPE>("typeMultiSamples", multiSamples_);

	record.SetRecordAsStringW("pathLaunch", pathExeLaunch_);

	record.SetRecordAsInteger("padIndex", padIndex_);

	{
		ByteBuffer bufKey;
		bufKey.WriteValue(mapKey_.size());
		for (auto itrKey = mapKey_.begin(); itrKey != mapKey_.end(); itrKey++) {
			int16_t id = itrKey->first;
			ref_count_ptr<VirtualKey> vk = itrKey->second;

			bufKey.WriteShort(id);
			bufKey.WriteShort(vk->GetKeyCode());
			bufKey.WriteShort(padIndex_);
			bufKey.WriteShort(vk->GetPadButton());
		}
		record.SetRecordAsInteger("mapKey_size", bufKey.GetSize());
		record.SetRecord("mapKey", bufKey.GetPointer(), bufKey.GetSize());
	}

	record.SetRecordAsBoolean("bLogWindow", bLogWindow_);
	record.SetRecordAsBoolean("bLogFile", bLogFile_);
	record.SetRecordAsBoolean("bMouseVisible", bMouseVisible_);

	record.WriteToFile(path, GAME_VERSION_NUM, "DNHCNFG\0", 8U);
	return true;
}
ref_count_ptr<VirtualKey> DnhConfiguration::GetVirtualKey(int16_t id) {
	auto itr = mapKey_.find(id);
	if (itr == mapKey_.end()) return nullptr;
	return itr->second;
}

#endif
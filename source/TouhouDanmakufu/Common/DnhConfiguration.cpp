#include "source/GcLib/pch.h"

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)

#include "DnhConfiguration.hpp"
#include "DnhGcLibImpl.hpp"

//*******************************************************************
//DnhConfiguration
//*******************************************************************
const size_t DnhConfiguration::MinScreenWidth = 150;
const size_t DnhConfiguration::MinScreenHeight = 150;
const size_t DnhConfiguration::MaxScreenWidth = 3840;
const size_t DnhConfiguration::MaxScreenHeight = 2160;
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

	{
		padIndex_ = 0;
		padResponse_ = 500;

		auto _AddKey = [&](int key, VirtualKey* pVk) {
			mapKey_[key].reset(pVk);
		};

		_AddKey(EDirectInput::KEY_LEFT, new VirtualKey(DIK_LEFT, 0, 0));
		_AddKey(EDirectInput::KEY_RIGHT, new VirtualKey(DIK_RIGHT, 0, 1));
		_AddKey(EDirectInput::KEY_UP, new VirtualKey(DIK_UP, 0, 2));
		_AddKey(EDirectInput::KEY_DOWN, new VirtualKey(DIK_DOWN, 0, 3));

		_AddKey(EDirectInput::KEY_OK, new VirtualKey(DIK_Z, 0, 5));
		_AddKey(EDirectInput::KEY_CANCEL, new VirtualKey(DIK_X, 0, 6));

		_AddKey(EDirectInput::KEY_SHOT, new VirtualKey(DIK_Z, 0, 5));
		_AddKey(EDirectInput::KEY_BOMB, new VirtualKey(DIK_X, 0, 6));
		_AddKey(EDirectInput::KEY_SLOWMOVE, new VirtualKey(DIK_LSHIFT, 0, 7));
		_AddKey(EDirectInput::KEY_USER1, new VirtualKey(DIK_C, 0, 8));
		_AddKey(EDirectInput::KEY_USER2, new VirtualKey(DIK_V, 0, 9));

		_AddKey(EDirectInput::KEY_PAUSE, new VirtualKey(DIK_ESCAPE, 0, 10));
	}

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

	windowTitle_ = prop.GetString(L"window.title", L"");

	screenWidth_ = prop.GetInteger(L"screen.width", 640);
	screenWidth_ = std::clamp<LONG>(screenWidth_, MinScreenWidth, MaxScreenWidth);

	screenHeight_ = prop.GetInteger(L"screen.height", 480);
	screenHeight_ = std::clamp<LONG>(screenHeight_, MinScreenHeight, MaxScreenHeight);

	fpsStandard_ = prop.GetInteger(L"standard.fps", 60);
	fpsStandard_ = std::max(fpsStandard_, 1U);

	fastModeSpeed_ = prop.GetInteger(L"skip.rate", 20);
	fastModeSpeed_ = std::clamp(fastModeSpeed_, 1, 50);

	{
		std::wstring str = prop.GetString(L"unfocused.processing", L"false");
		bEnableUnfocusedProcessing_ = str == L"true" ? true : StringUtility::ToInteger(str);
	}

	{
		auto _AddWindowSize = [&](std::vector<POINT>& listSize, LONG width, LONG height) {
			POINT size = {
				std::clamp<LONG>(width, MinScreenWidth, MaxScreenWidth),
				std::clamp<LONG>(height, MinScreenHeight, MaxScreenHeight)
			};
			listSize.push_back(size);
		};

		if (prop.HasProperty(L"window.size.list")) {
			std::wstring strList = prop.GetString(L"window.size.list", L"");
			if (strList.size() >= 3) {	// Minimum format: "0x0"
				std::wregex reg(L"([0-9]+)x([0-9]+)");
				auto itrBegin = std::wsregex_iterator(strList.begin(), strList.end(), reg);
				auto itrEnd = std::wsregex_iterator();

				if (itrBegin != itrEnd) {
					windowSizeList_.clear();

					for (auto& itr = itrBegin; itr != itrEnd; ++itr) {
						const std::wsmatch& match = *itr;

						_AddWindowSize(windowSizeList_, 
							wcstol(match[1].str().c_str(), nullptr, 10),
							wcstol(match[2].str().c_str(), nullptr, 10));
					}
				}
			}
		}
		if (windowSizeList_.size() == 0) {
			for (float iSizeMul : { 1.0f, 1.25f, 1.5f, 2.0f }) {
				_AddWindowSize(windowSizeList_, screenWidth_ * iSizeMul, screenHeight_ * iSizeMul);
			}
		}
	}

	return true;
}

bool DnhConfiguration::LoadConfigFile() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"config.dat";

	RecordBuffer record;
	if (!record.ReadFromFile(path, DATA_VERSION_CONFIG, "DNHCNFG\0", 8U)) {
		Logger::WriteWarn("Failed to read saved config file config.dat");
		return false;
	}

	record.GetRecord<ScreenMode>("modeScreen", modeScreen_);
	record.GetRecord<ColorMode>("modeColor", modeColor_);

	record.GetRecord<int>("fpsType", fpsType_);

	record.GetRecord<size_t>("sizeWindow", windowSizeIndex_);

	record.GetRecord<bool>("bVSync", bVSync_);
	record.GetRecord<bool>("bDeviceREF", bUseRef_);
	record.GetRecord<bool>("bPseudoFullscreen", bPseudoFullscreen_);

	record.GetRecord<D3DMULTISAMPLE_TYPE>("typeMultiSamples", multiSamples_);

	if (auto data = record.GetRecordAsStringW("pathLaunch")) {
		pathExeLaunch_ = *data;
	}
	if (pathExeLaunch_.size() == 0)
		pathExeLaunch_ = DNH_EXE_NAME;

	padIndex_ = record.GetRecordOr<int>("padIndex", padIndex_);
	padResponse_ = record.GetRecordOr<int>("padResponse", padResponse_);

	if (auto data = record.GetRecordAsRecordBuffer("mapKey")) {
		auto& keyMappingRecord = *data;

		if (auto count = keyMappingRecord.GetRecordAs<uint32_t>("count")) {
			size_t mapKeyCount = *count;
			if (mapKeyCount == mapKey_.size()) {
				auto mapKey = *keyMappingRecord.GetRecordAsByteBuffer("data");

				for (size_t iKey = 0; iKey < mapKeyCount; iKey++) {
					int16_t id = mapKey.ReadShort();
					int16_t keyCode = mapKey.ReadShort();
					int16_t padIndex = mapKey.ReadShort();
					int16_t padButton = mapKey.ReadShort();

					mapKey_[id].reset(new VirtualKey(keyCode, padIndex, padButton));
				}
			}
		}
	}

	record.GetRecord("bLogWindow", bLogWindow_);
	record.GetRecordAsBoolean("bLogFile", bLogFile_);
	record.GetRecordAsBoolean("bMouseVisible", bMouseVisible_);

	return true;
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
	record.SetRecordAsInteger("padResponse", padResponse_);

	{
		RecordBuffer keyMappingRecord;

		keyMappingRecord.SetRecord<uint32_t>("count", mapKey_.size());

		ByteBuffer bufData;
		for (auto& [id, vk] : mapKey_) {
			bufData.WriteShort(id);
			bufData.WriteShort(vk->GetKeyCode());
			bufData.WriteShort(padIndex_);
			bufData.WriteShort(vk->GetPadButton());
		}

		keyMappingRecord.SetRecordAsByteBuffer("data", MOVE(bufData));

		record.SetRecordAsRecordBuffer("mapKey", keyMappingRecord);
	}

	record.SetRecordAsBoolean("bLogWindow", bLogWindow_);
	record.SetRecordAsBoolean("bLogFile", bLogFile_);
	record.SetRecordAsBoolean("bMouseVisible", bMouseVisible_);

	return record.WriteToFile(path, DATA_VERSION_CONFIG, "DNHCNFG\0", 8);
}
ref_count_ptr<VirtualKey> DnhConfiguration::GetVirtualKey(int16_t id) {
	auto itr = mapKey_.find(id);
	if (itr == mapKey_.end()) return nullptr;
	return itr->second;
}

#endif
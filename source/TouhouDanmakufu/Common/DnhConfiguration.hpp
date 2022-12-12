#pragma once

#include "../../GcLib/pch.h"

#include "DnhConstant.hpp"

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)

//*******************************************************************
//DnhConfiguration
//*******************************************************************
class DnhConfiguration : public Singleton<DnhConfiguration> {
public:
	enum {
		FPS_NORMAL = 0,	// 1/1
		FPS_1_2,		// 1/2
		FPS_1_3,		// 1/3
		FPS_VARIABLE,
	};
public:
	ScreenMode modeScreen_;
	ColorMode modeColor_;

	std::wstring windowTitle_;
	LONG screenWidth_;
	LONG screenHeight_;
	bool bEnableUnfocusedProcessing_;

	uint32_t fpsStandard_;
	int fpsType_;
	int fastModeSpeed_;

	std::vector<POINT> windowSizeList_;
	uint32_t windowSizeIndex_;

	bool bVSync_;
	bool bUseRef_;
	bool bPseudoFullscreen_;
	D3DMULTISAMPLE_TYPE multiSamples_;

	int16_t padIndex_;
	std::map<int16_t, ref_count_ptr<VirtualKey>> mapKey_;

	std::wstring pathExeLaunch_;

	bool bLogWindow_;
	bool bLogFile_;
	bool bMouseVisible_;

	std::wstring pathPackageScript_;

	bool _LoadDefinitionFile();
public:
	DnhConfiguration();
	virtual ~DnhConfiguration();

	bool LoadConfigFile();
	bool SaveConfigFile();

	ref_count_ptr<VirtualKey> GetVirtualKey(int16_t id);
};

#endif
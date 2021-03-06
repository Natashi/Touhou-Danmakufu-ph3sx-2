#include "source/GcLib/pch.h"

#include "DnhGcLibImpl.hpp"
#include "DnhCommon.hpp"

#if defined(DNH_PROJ_EXECUTOR)
//*******************************************************************
//EPathProperty
//*******************************************************************
const std::wstring& EPathProperty::GetSystemResourceDirectory() {
	static std::wstring path = GetModuleDirectory() + L"resource/";
	return path;
}
const std::wstring& EPathProperty::GetSystemImageDirectory() {
	static std::wstring path = GetSystemResourceDirectory() + L"img/";
	return path;
}
const std::wstring& EPathProperty::GetSystemBgmDirectory() {
	static std::wstring path = GetSystemResourceDirectory() + L"bgm/";
	return path;
}
const std::wstring& EPathProperty::GetSystemSeDirectory() {
	static std::wstring path = GetSystemResourceDirectory() + L"se/";
	return path;
}

const std::wstring& EPathProperty::GetStgScriptRootDirectory() {
	static std::wstring path = GetModuleDirectory() + L"script/";
	return path;
}
const std::wstring& EPathProperty::GetStgDefaultScriptDirectory() {
	static std::wstring path = GetStgScriptRootDirectory() + L"default_system/";
	return path;
}
const std::wstring& EPathProperty::GetPlayerScriptRootDirectory() {
	static std::wstring path = GetModuleDirectory() + L"script/player/";
	return path;
}
std::wstring EPathProperty::GetReplaySaveDirectory(const std::wstring& scriptPath) {
	std::wstring scriptName = PathProperty::GetFileNameWithoutExtension(scriptPath);
	std::wstring dir = PathProperty::GetFileDirectory(scriptPath) + L"replay/";
	return dir;
}
std::wstring EPathProperty::GetCommonDataPath(const std::wstring& scriptPath, const std::wstring& area) {
	std::wstring dirSave = PathProperty::GetFileDirectory(scriptPath) + L"data/";
	std::wstring nameMain = PathProperty::GetFileNameWithoutExtension(scriptPath);
	std::wstring path = dirSave + nameMain + StringUtility::Format(L"_common_%s.dat", area.c_str());
	return path;
}

//*******************************************************************
//ELogger
//*******************************************************************
ELogger::ELogger() {

}
void ELogger::Initialize(bool bFile, bool bWindow) {
	if (bFile) {
		shared_ptr<gstd::FileLogger> fileLogger(new gstd::FileLogger());
		fileLogger->Initialize(bFile);
		this->AddLogger(fileLogger);
	}

	Logger::SetTop(this);
	WindowLogger::Initialize(bWindow);

	panelCommonData_.reset(new gstd::ScriptCommonDataInfoPanel());
}
void ELogger::UpdateCommonDataInfoPanel() {
	panelCommonData_->Update();
}

//*******************************************************************
//EFpsController
//*******************************************************************
EFpsController::EFpsController() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	int fpsType = config->GetFpsType();
	if (fpsType == DnhConfiguration::FPS_NORMAL ||
		fpsType == DnhConfiguration::FPS_1_2 ||
		fpsType == DnhConfiguration::FPS_1_3) {
		StaticFpsController* controller = new StaticFpsController();
		if (fpsType == DnhConfiguration::FPS_1_2)
			controller->SetSkipRate(1);
		else if (fpsType == DnhConfiguration::FPS_1_3)
			controller->SetSkipRate(2);
		controller_ = controller;
	}
	else {
		AutoSkipFpsController* controller = new AutoSkipFpsController();
		controller_ = controller;
	}

	SetFps(STANDARD_FPS);
	fastModeKey_ = DIK_LCONTROL;
}
#endif

#if defined(DNH_PROJ_EXECUTOR)
//*******************************************************************
//ETaskManager
//*******************************************************************
ETaskManager::ETaskManager() {
	timeSpentOnRender_ = 0;
	timeSpentOnWork_ = 0;
}
bool ETaskManager::Initialize() {
	InitializeFunctionDivision(TASK_WORK_PRI_MAX, TASK_RENDER_PRI_MAX);
	return true;
}

//*******************************************************************
//ETextureManager
//*******************************************************************
bool ETextureManager::Initialize() {
	bool res = TextureManager::Initialize();
	if (!res)
		throw gstd::wexception("ETextureManager: Failed to initialize TextureManager.");

	int failedIndex = -1;
	for (size_t iRender = 0; iRender < MAX_RESERVED_RENDERTARGET; iRender++) {
		std::wstring name = GetReservedRenderTargetName(iRender);
		shared_ptr<Texture> texture(new Texture());
		if (!texture->CreateRenderTarget(name)) {
			failedIndex = iRender;
			break;
		}
		Add(name, texture);
	}

	if (failedIndex >= 0) {
		std::string err = StringUtility::Format("ETextureManager: Failed to create reserved render target %d.", failedIndex);
		throw gstd::wexception(err);
		res = false;
	}
	return res;
}
std::wstring ETextureManager::GetReservedRenderTargetName(int index) {
	return StringUtility::FormatToWide("__RESERVED_RENDER_TARGET__%d", index);
}
#endif

//*******************************************************************
//EDirectInput
//*******************************************************************
bool EDirectInput::Initialize(HWND hWnd) {
	padIndex_ = 0;

	VirtualKeyManager::Initialize(hWnd);

	ResetVirtualKeyMap();

	return true;
}
void EDirectInput::ResetVirtualKeyMap() {
	ClearKeyMap();
	
	DnhConfiguration* config = DnhConfiguration::GetInstance();

	AddKeyMap(KEY_LEFT, config->GetVirtualKey(KEY_LEFT));
	AddKeyMap(KEY_RIGHT, config->GetVirtualKey(KEY_RIGHT));
	AddKeyMap(KEY_UP, config->GetVirtualKey(KEY_UP));
	AddKeyMap(KEY_DOWN, config->GetVirtualKey(KEY_DOWN));

	AddKeyMap(KEY_OK, config->GetVirtualKey(KEY_OK));
	AddKeyMap(KEY_CANCEL, config->GetVirtualKey(KEY_CANCEL));

	AddKeyMap(KEY_SHOT, config->GetVirtualKey(KEY_SHOT));
	AddKeyMap(KEY_BOMB, config->GetVirtualKey(KEY_BOMB));
	AddKeyMap(KEY_SLOWMOVE, config->GetVirtualKey(KEY_SLOWMOVE));
	AddKeyMap(KEY_USER1, config->GetVirtualKey(KEY_USER1));
	AddKeyMap(KEY_USER2, config->GetVirtualKey(KEY_USER2));

	AddKeyMap(KEY_PAUSE, config->GetVirtualKey(KEY_PAUSE));
}

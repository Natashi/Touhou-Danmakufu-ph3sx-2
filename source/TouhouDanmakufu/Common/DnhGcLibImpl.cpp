#include "source/GcLib/pch.h"

#include "DnhGcLibImpl.hpp"
#include "DnhConfiguration.hpp"
#include "DnhCommon.hpp"

#include "../DnhExecutor/GcLibImpl.hpp"

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
ELogger::~ELogger() {
	if (threadWindow_) {
		threadWindow_->Stop();
		threadWindow_->Join(1000);
	}
}

bool ELogger::Initialize() {
	return Initialize(false, true);
}
bool ELogger::Initialize(bool bFile, bool bWindow) {
	Logger::SetTop(this);
	Logger::Initialize(bWindow);

	{
		shared_ptr<gstd::WindowLogger> logger(new gstd::WindowLogger());
		logger->Initialize("Window", bWindow);
		this->AddLogger(logger);
	}
	if (bFile) {
		shared_ptr<gstd::FileLogger> logger(new gstd::FileLogger());
		logger->Initialize("File", bFile);
		this->AddLogger(logger);
	}

	panelCommonData_.reset(new gstd::ScriptCommonDataInfoPanel());

	time_ = SystemUtility::GetCpuTime2();

	threadWindow_ = new WindowThread(this);
	threadWindow_->Start();

	return true;
}
void ELogger::UpdateCommonDataInfoPanel() {
	panelCommonData_->Update();
}

void ELogger::LoadState() {
	for (auto& logger : listLogger_)
		logger->LoadState();
}
void ELogger::SaveState() {
	for (auto& logger : listLogger_)
		logger->SaveState();
}

void ELogger::_Run() {
	while (true) {
		if (listLogger_.size() > 0) {
			uint64_t currentTime = SystemUtility::GetCpuTime2();
			uint64_t timeDelta = currentTime - time_;

			for (PanelData& iPanel : listPanel_) {
				if (iPanel.updateFreq == UINT_MAX) continue;

				bool bUpdate = false;

				bool bNowVisible = iPanel.panel->IsActive();
				if (bNowVisible && !iPanel.bPrevVisible)
					bUpdate = true;

				if (!bUpdate) {
					iPanel.timer += timeDelta;
					if (iPanel.timer >= iPanel.updateFreq)
						bUpdate = true;
				}

				if (bUpdate) {
					iPanel.panel->Update();
					iPanel.timer = 0;
				}

				iPanel.bPrevVisible = bNowVisible;
			}

			time_ = currentTime;
		}

		if (!Loop()) {
			threadWindow_->Stop();
			_Close();
			break;
		}
	}
}

bool ELogger::_AddPanel(shared_ptr<ILoggerPanel> panel, const std::wstring& name) {
	for (auto& logger : listLogger_) {
		if (auto wLogger = dynamic_cast<gstd::WindowLogger*>(logger.get())) {
			if (!wLogger->AddPanel(panel, StringUtility::ConvertWideToMulti(name)))
				return false;
		}
	}
	return true;
}
bool ELogger::EAddPanel(shared_ptr<ILoggerPanel> panel, const std::wstring& name, uint32_t updateFreq) {
	if (!_AddPanel(panel, name))
		return false;

	return EAddPanelUpdateData(panel, updateFreq);
}
bool ELogger::EAddPanelNoUpdate(shared_ptr<ILoggerPanel> panel, const std::wstring& name) {
	return EAddPanel(panel, name, UINT_MAX);
}
bool ELogger::EAddPanelUpdateData(shared_ptr<ILoggerPanel> panel, uint32_t updateFreq) {
	{
		Lock lock(GetLock());
		
		PanelData data;
		data.panel = panel;

		data.updateFreq = updateFreq;
		data.timer = 0;

		data.bPrevVisible = false;

		listPanel_.push_back(data);
	}

	return true;
}

void ELogger::InsertOpenCommandInSystemMenu(HWND hWnd) {
	HMENU hMenu = ::GetSystemMenu(hWnd, FALSE);

	MENUITEMINFO mii;
	ZeroMemory(&mii, sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);
	InsertMenuItem(hMenu, 0, 1, &mii);

	mii.fMask = MIIM_ID | MIIM_TYPE;
	mii.fType = MFT_STRING;
	mii.wID = MY_SYSCMD_OPEN;
	mii.dwTypeData = L"Show LogWindow";

	InsertMenuItem(hMenu, 0, 1, &mii);
}

void ELogger::ResetDevice() {
	_ResetDevice();
}

ELogger::WindowThread::WindowThread(ELogger* logger) {
	_SetOuter(logger);
}
void ELogger::WindowThread::_Run() {
	ELogger* logger = _GetOuter();
	logger->_Run();
}

//*******************************************************************
//EFpsController
//*******************************************************************
EFpsController::EFpsController() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	int fpsType = config->fpsType_;
	switch (fpsType) {
	case DnhConfiguration::FPS_NORMAL:
	case DnhConfiguration::FPS_1_2:
	case DnhConfiguration::FPS_1_3:
	{
		StaticFpsController* controller = new StaticFpsController();
		if (fpsType == DnhConfiguration::FPS_1_2)
			controller->SetSkipRate(2);
		else if (fpsType == DnhConfiguration::FPS_1_3)
			controller->SetSkipRate(3);
		controller_ = controller;
		break;
	}
	case DnhConfiguration::FPS_VARIABLE:
	{
		VariableFpsController* controller = new VariableFpsController();
		controller_ = controller;
	}
	}

	if (controller_ == nullptr)
		throw gstd::wexception("Invalid refresh rate mode.");

	SetFps(config->fpsStandard_);
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

	DirectGraphics* graphics = DirectGraphics::GetBase();
	{
		size_t rW = Math::GetNextPow2(graphics->GetScreenWidth());
		size_t rH = Math::GetNextPow2(graphics->GetScreenHeight());

		shared_ptr<TextureData> data;
		if (!_CreateRenderTarget_Unmanaged(data, L"__PRIMARY_BACKSURFACE__", rW, rH)) {
			throw gstd::wexception("ETextureManager: Failed to create back surface render target.");
		}

		graphics->SetDefaultBackBufferRenderTarget(data);
	}
	{
		size_t rW = Math::GetNextPow2(graphics->GetRenderScreenWidth() * 2);
		size_t rH = Math::GetNextPow2(graphics->GetRenderScreenHeight() * 2);

		std::wstring name = L"__SECONDARY_BACKSURFACE__";

		shared_ptr<Texture> texture(new Texture());
		if (!texture->CreateRenderTarget(name, rW, rH)) {
			throw gstd::wexception("ETextureManager: Failed to create secondary backbuffer.");
		}

		Add(name, texture);
		EApplication::GetInstance()->SetSecondaryBackBuffer(texture);
	}
	{
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

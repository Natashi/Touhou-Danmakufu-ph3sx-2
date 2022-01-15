#pragma once

#include "../../GcLib/pch.h"

#include "DnhConstant.hpp"

#if defined(DNH_PROJ_EXECUTOR)
//*******************************************************************
//ScriptInformation
//*******************************************************************
class ScriptInformation {
public:
	enum {
		TYPE_UNKNOWN,
		TYPE_PLAYER,
		TYPE_SINGLE,
		TYPE_PLURAL,
		TYPE_STAGE,
		TYPE_PACKAGE,

		MAX_ID = 8,
	};
	const static std::wstring DEFAULT;
	class Sort;
	struct PlayerListSort;
private:
	int type_;
	std::wstring pathArchive_;
	std::wstring pathScript_;

	std::wstring id_;
	std::wstring title_;
	std::wstring text_;
	std::wstring pathImage_;
	std::wstring pathSystem_;
	std::wstring pathBackground_;
	std::vector<std::wstring> listPlayer_;

	std::wstring replayName_;
public:
	ScriptInformation() {}
	virtual ~ScriptInformation() {}

	int GetType() { return type_; }
	void SetType(int type) { type_ = type; }
	std::wstring& GetArchivePath() { return pathArchive_; }
	void SetArchivePath(const std::wstring& path) { pathArchive_ = path; }
	std::wstring& GetScriptPath() { return pathScript_; }
	void SetScriptPath(const std::wstring& path) { pathScript_ = path; }

	std::wstring& GetID() { return id_; }
	void SetID(const std::wstring& id) { id_ = id; }
	std::wstring& GetTitle() { return title_; }
	void SetTitle(const std::wstring& title) { title_ = title; }
	std::wstring& GetText() { return text_; }
	void SetText(const std::wstring& text) { text_ = text; }
	std::wstring& GetImagePath() { return pathImage_; }
	void SetImagePath(const std::wstring& path) { pathImage_ = path; }
	std::wstring& GetSystemPath() { return pathSystem_; }
	void SetSystemPath(const std::wstring& path) { pathSystem_ = path; }
	std::wstring& GetBackgroundPath() { return pathBackground_; }
	void SetBackgroundPath(const std::wstring& path) { pathBackground_ = path; }
	std::vector<std::wstring>& GetPlayerList() { return listPlayer_; }
	void SetPlayerList(std::vector<std::wstring>& list) { listPlayer_ = list; }

	std::wstring& GetReplayName() { return replayName_; }
	void SetReplayName(const std::wstring& name) { replayName_ = name; }

	std::vector<ref_count_ptr<ScriptInformation>> CreatePlayerScriptInformationList();
public:
	static ref_count_ptr<ScriptInformation> CreateScriptInformation(const std::wstring& pathScript, 
		bool bNeedHeader = true);
	static ref_count_ptr<ScriptInformation> CreateScriptInformation(const std::wstring& pathScript, 
		const std::wstring& pathArchive, const std::string& source, bool bNeedHeader = true);

	static std::vector<ref_count_ptr<ScriptInformation>> CreateScriptInformationList(const std::wstring& path,
		bool bNeedHeader = true);
	static std::vector<ref_count_ptr<ScriptInformation>> FindPlayerScriptInformationList(const std::wstring& dir);
	static bool IsExcludeExtention(const std::wstring& ext);

private:
	static std::wstring _GetString(Scanner& scanner);
	static std::vector<std::wstring> _GetStringList(Scanner& scanner);
};

//Fuck?????
class ScriptInformation::Sort {
public:
	BOOL operator()(const ref_count_ptr<ScriptInformation>& lf, const ref_count_ptr<ScriptInformation>& rf) {
		std::wstring& lPath = lf->GetScriptPath();
		std::wstring& rPath = lf->GetScriptPath();
		int res = ::CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
			lPath.c_str(), -1, rPath.c_str(), -1);
		return res == CSTR_LESS_THAN;
	}
};

struct ScriptInformation::PlayerListSort {
	BOOL operator()(const ref_count_ptr<ScriptInformation>& lf, const ref_count_ptr<ScriptInformation>& rf) {
		std::wstring& lPath = lf->GetScriptPath();
		std::wstring& rPath = lf->GetScriptPath();
		int res = ::CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
			lPath.c_str(), -1, rPath.c_str(), -1);
		return res == CSTR_LESS_THAN;
	}
};
#endif

//*******************************************************************
//ErrorDialog
//*******************************************************************
class ErrorDialog : public ModalDialog {
protected:
	static HWND hWndParentStatic_;

	WEditBox edit_;
	WButton button_;

	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	ErrorDialog(HWND hParent);

	bool ShowModal(std::wstring msg);

	static void SetParentWindowHandle(HWND hWndParent) { hWndParentStatic_ = hWndParent; }
	static void ShowErrorDialog(std::wstring msg) { ErrorDialog dialog(hWndParentStatic_); dialog.ShowModal(msg); }
};

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
		FPS_AUTO,
	};
public:
	ScreenMode modeScreen_;
	ColorMode modeColor_;
	
	int fpsType_;

	std::vector<POINT> windowSizeList_;
	size_t sizeWindow_;
	bool bDynamicScaling_;

	int fastModeSpeed_;

	bool bVSync_;
	bool referenceRasterizer_;
	bool bPseudoFullscreen_;
	D3DMULTISAMPLE_TYPE multiSamples_;

	int16_t padIndex_;
	std::map<int16_t, ref_count_ptr<VirtualKey>> mapKey_;

	std::wstring pathExeLaunch_;

	bool bLogWindow_;
	bool bLogFile_;
	bool bMouseVisible_;

	std::wstring pathPackageScript_;
	std::wstring windowTitle_;
	LONG screenWidth_;
	LONG screenHeight_;

	bool _LoadDefinitionFile();
public:
	DnhConfiguration();
	virtual ~DnhConfiguration();

	bool LoadConfigFile();
	bool SaveConfigFile();

	ScreenMode GetScreenMode() { return modeScreen_; }
	void SetScreenMode(ScreenMode mode) { modeScreen_ = mode; }
	std::vector<POINT>& GetWindowSizeList() { return windowSizeList_; }
	size_t GetWindowSize() { return sizeWindow_; }
	void SetWindowSize(size_t size) { sizeWindow_ = size; }
	bool UseDynamicScaling() { return bDynamicScaling_; }

	int GetFpsType() { return fpsType_; }
	void SetFpsType(int type) { fpsType_ = type; }
	int GetSkipModeSpeedRate() { return fastModeSpeed_; }

	ColorMode GetColorMode() { return modeColor_; }
	void SetColorMode(ColorMode mode) { modeColor_ = mode; }
	bool IsEnableVSync() { return bVSync_; }
	void SetEnableVSync(bool b) { bVSync_ = b; }
	bool IsEnableRef() { return referenceRasterizer_; }
	void SetEnableRef(bool b) { referenceRasterizer_ = b; }
	D3DMULTISAMPLE_TYPE GetMultiSampleType() { return multiSamples_; }
	void SetMultiSampleType(D3DMULTISAMPLE_TYPE type) { multiSamples_ = type; }

	std::wstring& GetExePath() { return pathExeLaunch_; }
	void SetExePath(std::wstring str) { pathExeLaunch_ = str; }

	int16_t GetPadIndex() { return padIndex_; }
	void SetPadIndex(int16_t index) { padIndex_ = index; }
	ref_count_ptr<VirtualKey> GetVirtualKey(int16_t id);

	bool IsLogWindow() { return bLogWindow_; }
	void SetLogWindow(bool b) { bLogWindow_ = b; }
	bool IsLogFile() { return bLogFile_; }
	void SetLogFile(bool b) { bLogFile_ = b; }
	bool IsMouseVisible() { return bMouseVisible_; }
	void SetMouseVisible(bool b) { bMouseVisible_ = b; }

	std::wstring& GetPackageScriptPath() { return pathPackageScript_; }
	std::wstring& GetWindowTitle() { return windowTitle_; }
	LONG GetScreenWidth() { return screenWidth_; }
	LONG GetScreenHeight() { return screenHeight_; }
};
#endif
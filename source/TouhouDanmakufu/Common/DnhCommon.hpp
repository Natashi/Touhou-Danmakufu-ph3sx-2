#pragma once

#include "../../GcLib/pch.h"

#include "DnhConstant.hpp"
#include "DnhConfiguration.hpp"

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
public:
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
		const std::wstring& lPath = lf->pathScript_;
		const std::wstring& rPath = lf->pathScript_;
		int res = ::CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
			lPath.c_str(), -1, rPath.c_str(), -1);
		return res == CSTR_LESS_THAN;
	}
};

struct ScriptInformation::PlayerListSort {
	BOOL operator()(const ref_count_ptr<ScriptInformation>& lf, const ref_count_ptr<ScriptInformation>& rf) {
		const std::wstring& lPath = lf->pathScript_;
		const std::wstring& rPath = lf->pathScript_;
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

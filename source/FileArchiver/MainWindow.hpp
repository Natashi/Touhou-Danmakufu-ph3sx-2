#pragma once

#include "../GcLib/pch.h"
#include "Constant.hpp"

//*******************************************************************
//MainWindow
//*******************************************************************
class MainWindow : public WindowBase, public Singleton<MainWindow>, Thread {
protected:
	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	enum {
		COL_FILENAME = 0,
		COL_DIRECTORY,
		COL_FULLPATH,
	};

	WButton buttonDecompile_;
	WListView wndListFile_;
	WStatusBar wndStatus_;
	WProgressBar wndProgressBar_;

	std::wstring pathArchive_;
	std::set<std::wstring> listFile_;

	BOOL _DropFiles(WPARAM wParam, LPARAM lParam);
	void _AddFileFromDialog();

	void _AddFile(std::wstring dirBase, std::wstring path);
	void _RemoveFile();
	void _RemoveAllFile();
	bool _IsValidFilePath(const std::wstring& dirBase, const std::wstring& path);
	std::wstring _CreateKey(const std::wstring& dirBase, const std::wstring& path);
	std::wstring _CreateRelativeDirectory(const std::wstring& dirBase, const std::wstring& path);
	void _StartArchive();

	virtual void _Run();
	void _Archive();

	void _LoadEnvironment();
	void _SaveEnvironment();
public:
	MainWindow();
	~MainWindow();

	bool Initialize();
};
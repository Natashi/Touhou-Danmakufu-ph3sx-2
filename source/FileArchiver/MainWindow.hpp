#ifndef __MAIN_WINDOW__
#define __MAIN_WINDOW__

#include "../GcLib/pch.h"
#include "Constant.hpp"

/**********************************************************
//MainWindow
**********************************************************/
class MainWindow : public WindowBase, public Singleton<MainWindow>, Thread {
private:

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

	std::wstring pathArchive_;
	std::set<std::wstring> listFile_;//èdï°ìoò^ÇñhÇÆ

	BOOL _DropFiles(WPARAM wParam, LPARAM lParam);
	void _AddFileFromDialog();

	void _AddFile(std::wstring dirBase, std::wstring path);
	void _RemoveFile();
	void _RemoveAllFile();
	bool _IsValidFilePath(std::wstring dirBase, std::wstring path);
	std::wstring _CreateKey(std::wstring dirBase, std::wstring path);
	std::wstring _CreateRelativeDirectory(std::wstring dirBase, std::wstring path);
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


#endif

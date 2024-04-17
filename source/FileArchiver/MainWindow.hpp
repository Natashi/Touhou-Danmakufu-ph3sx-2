#pragma once
#include "../GcLib/pch.h"

#include "Constant.hpp"

#include "../../GcLib/directx/ImGuiWindow.hpp"

//*******************************************************************
//FileEntryInfo
//*******************************************************************
struct FileEntryInfo {
	std::wstring path;
	std::wstring displayName;
	bool bDirectory;

	bool bIncluded = true;

	FileEntryInfo* parent = nullptr;
	std::list<unique_ptr<FileEntryInfo>> children;

	FileEntryInfo* AddChild(FileEntryInfo* node) {
		node->parent = parent;
		children.push_back(unique_ptr<FileEntryInfo>(node));
		return children.back().get();
	}

	void Sort();
};

//*******************************************************************
//MainWindow
//*******************************************************************
class ArchiverThread;
class MainWindow : public directx::imgui::ImGuiBaseWindow, public Singleton<MainWindow> {
protected:
	virtual LRESULT _SubWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	enum {
		COL_FILENAME = 0,
		COL_DIRECTORY,
		COL_FULLPATH,
	};
private:
	std::wstring pathSaveBaseDir_;
	std::wstring pathSaveArchive_;
	std::wstring pathBaseDir_;
	std::wstring pathArchive_;

	unique_ptr<FileEntryInfo> nodeDirectoryRoot_;
	std::unordered_map<std::wstring, FileEntryInfo*> mapDirectoryNodes_;
	std::vector<FileEntryInfo*> listFiles_;

	bool bArchiveEnabled_;

	unique_ptr<ArchiverThread> pArchiverWorkThread_;
protected:
	virtual void _SetImguiStyle(float scale);
private:
	void _LoadEnvironment();
	void _SaveEnvironment();

	void _AddFileFromDialog();
	BOOL _AddFileFromDrop(WPARAM wParam, LPARAM lParam);

	void _AddFilesInDirectory(const std::wstring& dirBase);
	void _RemoveAllFiles();

	static std::wstring _CreateRelativeDirectory(const std::wstring& dirBase, const std::wstring& path);

	virtual void _ProcessGui();
	virtual void _ProcessGui_FileTree();

	void _StartArchive();
public:
	MainWindow();
	virtual ~MainWindow();

	virtual bool Initialize();
};

//*******************************************************************
//ArchiverThread
//*******************************************************************
class ArchiverThread : public Thread {
	//File formats excluded from compression
	static std::set<std::wstring> listCompressExclude_;
private:
	std::vector<FileEntryInfo*> listFile_;
	std::wstring pathBaseDir_;
	std::wstring pathArchive_;

	std::wstring archiverStatus_;
	float archiverProgress_;

	std::string err_;
private:
	virtual void _Run();
public:
	ArchiverThread(const std::vector<FileEntryInfo*>& listFile, 
		const std::wstring pathBaseDir, const std::wstring& pathArchive);

	const std::wstring& GetArchiverStatus() { return archiverStatus_; }
	float GetArchiverProgress() { return archiverProgress_; }

	std::string& GetError() { return err_; }
};
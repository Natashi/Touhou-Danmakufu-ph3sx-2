#pragma once

#include "../GcLib/pch.h"
#include "Constant.hpp"

//*******************************************************************
//DxGraphicsFileArchiver
//*******************************************************************
class DxGraphicsFileArchiver : public DirectGraphicsBase {
protected:
	D3DPRESENT_PARAMETERS d3dpp_;
protected:
	virtual void _ReleaseDxResource();
	virtual void _RestoreDxResource();
	virtual bool _Restore();

	virtual std::vector<std::wstring> _GetRequiredModules();
public:
	DxGraphicsFileArchiver();
	virtual ~DxGraphicsFileArchiver();

	virtual bool Initialize(HWND hWnd);
	virtual void Release();

	virtual bool BeginScene(bool bClear);
	virtual void EndScene(bool bPresent);

	virtual void ResetDeviceState() {};

	void ResetDevice();

	void SetSize(UINT wd, UINT ht) {
		d3dpp_.BackBufferWidth = wd;
		d3dpp_.BackBufferHeight = ht;
	}
	UINT GetWidth() { return d3dpp_.BackBufferWidth; }
	UINT GetHeight() { return d3dpp_.BackBufferHeight; }
};

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

	shared_ptr<gstd::WTreeView::Item> pTreeItem = nullptr;

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
class MainWindow : public WindowBase, public Singleton<MainWindow> {
protected:
	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	enum {
		COL_FILENAME = 0,
		COL_DIRECTORY,
		COL_FULLPATH,
	};
public:
	
private:
	unique_ptr<DxGraphicsFileArchiver> dxGraphics_;
	ImGuiIO* pIo_;

	std::unordered_map<std::wstring, std::string> mapSystemFontPath_;
	std::unordered_map<std::string, ImFont*> mapFont_;
	UINT dpi_;

	std::wstring pathSaveBaseDir_;
	std::wstring pathSaveArchive_;
	std::wstring pathBaseDir_;
	std::wstring pathArchive_;

	unique_ptr<FileEntryInfo> nodeDirectoryRoot_;
	std::unordered_map<std::wstring, FileEntryInfo*> mapDirectoryNodes_;
	std::vector<FileEntryInfo*> listFiles_;

	bool bInitialized_;
	volatile bool bRun_;

	bool bArchiveEnabled_;

	unique_ptr<ArchiverThread> pArchiverWorkThread_;
private:
	void _ResetDevice();
	void _ResetFont();

	void _Resize(float scale);
	void _SetImguiStyle(float scale);
private:
	void _LoadEnvironment();
	void _SaveEnvironment();

	void _AddFileFromDialog();
	BOOL _AddFileFromDrop(WPARAM wParam, LPARAM lParam);

	void _AddFilesInDirectory(const std::wstring& dirBase);
	void _RemoveAllFiles();

	static std::wstring _CreateRelativeDirectory(const std::wstring& dirBase, const std::wstring& path);

	void _ProcessGui();
	void _ProcessGui_FileTree();
	void _Update();

	//virtual void _Run();

	void _StartArchive();
public:
	MainWindow();
	~MainWindow();

	bool Initialize();
	bool Loop();
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
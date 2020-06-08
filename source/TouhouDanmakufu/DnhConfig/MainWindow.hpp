#pragma once
#include "../../GcLib/pch.h"

#include "Constant.hpp"
#include "Common.hpp"

class DevicePanel;
class KeyPanel;
class OptionPanel;
/**********************************************************
//MainWindow
**********************************************************/
class MainWindow : public WindowBase, public gstd::Singleton<MainWindow> {
protected:
	ref_count_ptr<WTabControll> wndTab_;
	ref_count_ptr<DevicePanel> panelDevice_;
	ref_count_ptr<KeyPanel> panelKey_;
	ref_count_ptr<OptionPanel> panelOption_;

	void _RunExecutor();
	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	MainWindow();
	~MainWindow();
	bool Initialize();
	bool StartUp();

	void ClearData();
	bool Load();
	bool Save();

	void UpdateKeyAssign();

	void ReadConfiguration();
	void WriteConfiguration();
};

/**********************************************************
//DevicePanel
**********************************************************/
class DevicePanel : public WPanel {
protected:
	WComboBox comboWindowSize_;
	WComboBox comboMultisample_;

	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	DevicePanel();
	~DevicePanel();
	bool Initialize(HWND hParent);

	void ReadConfiguration();
	void WriteConfiguration();
};

/**********************************************************
//KeyPanel
**********************************************************/
class KeyPanel : public WPanel {
	class KeyListView;
protected:
	enum {
		COL_ACTION,
		COL_KEY_ASSIGN,
		COL_PAD_ASSIGN,
	};

	WComboBox comboPadIndex_;
	ref_count_ptr<KeyListView> viewKey_;
	KeyCodeList listKeyCode_;
	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void _UpdateText(int row);

public:
	KeyPanel();
	~KeyPanel();
	bool Initialize(HWND hParent);
	bool StartUp();
	void UpdateKeyAssign();

	void ReadConfiguration();
	void WriteConfiguration();
};

class KeyPanel::KeyListView : public WListView {
protected:
	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

/**********************************************************
//OptionPanel
**********************************************************/
class OptionPanel : public WPanel {
protected:
	enum {
		ROW_LOG_WINDOW,
		ROW_LOG_FILE,
		ROW_MOUSE_UNVISIBLE,
	};
	ref_count_ptr<WListView> viewOption_;
	ref_count_ptr<WEditBox> exePath_;

	virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	OptionPanel();
	~OptionPanel();
	bool Initialize(HWND hParent);

	std::wstring GetExecutablePath() { return exePath_->GetText(); }

	void ReadConfiguration();
	void WriteConfiguration();
};
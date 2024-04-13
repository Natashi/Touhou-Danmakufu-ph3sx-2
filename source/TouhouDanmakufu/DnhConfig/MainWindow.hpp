#pragma once
#include "../../GcLib/pch.h"

#include "Constant.hpp"
#include "Common.hpp"

#include "../../GcLib/directx/ImGuiWindow.hpp"

//*******************************************************************
//MainWindow
//*******************************************************************
class Panel;
class MainWindow : public directx::imgui::ImGuiBaseWindow, public gstd::Singleton<MainWindow> {
protected:
	virtual LRESULT _SubWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
protected:
	std::vector<unique_ptr<Panel>> listPanels_;
	Panel* pCurrentPanel_;
	Panel* pPanelOption_;
protected:
	virtual void _SetImguiStyle(float scale);
private:
	bool _StartGame();

	void _LoadConfig();
	void _SaveConfig();
	void _ClearConfig();

	virtual void _ProcessGui();
	virtual void _Update();
public:
	MainWindow();
	virtual ~MainWindow();

	virtual bool Initialize();

	void InitializePanels();
};

//*******************************************************************
//IPanel
//*******************************************************************
class Panel {
protected:
	std::string tabName_;
public:
	virtual bool Initialize() = 0;
	virtual void DefaultSettings() = 0;

	virtual void LoadConfiguration() = 0;
	virtual void SaveConfiguration() = 0;

	virtual void Update() {}
	virtual void ProcessGui() = 0;

	const std::string& GetTabName() { return tabName_; }
};

//*******************************************************************
//DevicePanel
//*******************************************************************
class DevicePanel : public Panel {
protected:
	struct MultisampleInfo {
		D3DMULTISAMPLE_TYPE msaa;
		const char* name;
		bool bWindowed, bFullscreen;
	};
protected:
	int selectedScreenMode_;

	std::vector<POINT> listWindowSize_;
	int selectedWindowSize_;

	std::vector<MultisampleInfo> listMSAA_;
	int selectedMSAA_;

	int selectedRefreshRate_;
	int selectedColorMode_;

	bool checkEnableVSync_;
	bool checkBorderlessFullscreen_;
public:
	DevicePanel();
	~DevicePanel();

	virtual bool Initialize();
	virtual void DefaultSettings();

	virtual void LoadConfiguration();
	virtual void SaveConfiguration();

	virtual void ProcessGui();
};

//*******************************************************************
//KeyPanel
//*******************************************************************
class KeyPanel : public Panel {
	class KeyListView;
protected:
	class KeyBindData {
	public:
		enum {
			FADE_DURATION = 1000,
		};
	public:
		std::string name;		// Action name

		int16_t keyboardButton;	// Bound keyboard button

		int16_t padDevice;		// Bound pad object
		int16_t padButton;		// Bound pad button

		stdch::milliseconds fadeTimer[2];		// Fade timer

		KeyBindData() : KeyBindData("", -1, 0, -1) {}
		KeyBindData(const std::string& name) : KeyBindData(name, -1, 0, -1) {}
		KeyBindData(const std::string& name, int16_t keyboardButton, int16_t padButton)
			: KeyBindData(name, keyboardButton, 0, padButton) {}
		KeyBindData(const std::string& name, int16_t keyboardButton, int16_t padDevice, int16_t padButton) {
			this->name = name;
			SetKeys(keyboardButton, padDevice, padButton);
			fadeTimer[0] = fadeTimer[1] 
				= stdch::milliseconds(0);
		}

		void SetKeys(int16_t keyboardButton, int16_t padButton) {
			SetKeys(keyboardButton, 0, padButton);
		}
		void SetKeys(int16_t keyboardButton, int16_t padDevice, int16_t padButton) {
			this->keyboardButton = keyboardButton;
			this->padDevice = padDevice;
			this->padButton = padButton;
		}
	};
protected:
	std::vector<const DirectInput::JoypadInputDevice*> listPadDevice_;
	std::vector<const ControllerMap*> listPadDeviceKeyMap_;

	KeyCodeList listKeyCode_;

	std::map<int, KeyBindData> mapAction_;

	int selectedPadDevice_;
	int padResponse_;

	bool bPadRefreshing_;
protected:
	void _SetPadResponse();
	void _RescanDevices();
public:
	KeyPanel();
	~KeyPanel();

	virtual bool Initialize();
	virtual void DefaultSettings();

	virtual void LoadConfiguration();
	virtual void SaveConfiguration();

	virtual void Update();

	virtual void ProcessGui();
};

//*******************************************************************
//OptionPanel
//*******************************************************************
class OptionPanel : public Panel {
protected:
	bool checkShowLogWindow_;
	bool checkFileLogger_;
	bool checkHideCursor_;

	char bufTextBox_[256];
	std::wstring exePath_;
protected:
	void _SetTextBuffer();
	void _BrowseExePath();
public:
	OptionPanel();
	~OptionPanel();

	virtual bool Initialize();
	virtual void DefaultSettings();

	virtual void LoadConfiguration();
	virtual void SaveConfiguration();

	virtual void ProcessGui();

	std::wstring GetExecutablePath() { return exePath_; }
};
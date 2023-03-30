#pragma once

#include "../pch.h"
#include "DxConstant.hpp"

namespace directx {
	//*******************************************************************
	//DirectInput
	//*******************************************************************
	class DirectInput {
		static DirectInput* thisBase_;
	public:
		enum {
			MAX_JOYPAD = 4,
			MAX_KEY = 256,
			MAX_MOUSE_BUTTON = 4,
			MAX_PAD_BUTTON = 16,
			MAX_PAD_STATE = 32,

			// ------------------------------

			PAD_ANALOG_LEFT = 0,
			PAD_ANALOG_RIGHT,
			PAD_ANALOG_UP,
			PAD_ANALOG_DOWN,

			PAD_D_LEFT = 4,
			PAD_D_RIGHT,
			PAD_D_UP,
			PAD_D_DOWN,

			PAD_0 = 8,
			PAD_1,
			PAD_2,
			PAD_3,
			PAD_4,
			PAD_5,
			PAD_6,
			PAD_7,
			PAD_8,
			PAD_9,
			PAD_10,
			PAD_11,
			PAD_12,
			PAD_13,
			PAD_14,
			PAD_15,
		};
	public:
		struct InputDeviceHeader {
			LPDIRECTINPUTDEVICE8 pDevice = nullptr;
			DIDEVICEINSTANCE ddi;

			std::wstring hidPath;
			uint16_t vendorID = 0, productID = 0;

			HRESULT QueryDeviceInstance();
			HRESULT QueryHidPath();
			HRESULT QueryVidPid();

			inline void Unacquire() {
				if (pDevice) {
					pDevice->Unacquire();
					ptr_release(pDevice);
				}
			}
		};

		struct KeyboardInputDevice {
			InputDeviceHeader idh;
			BYTE state[MAX_KEY];
		};
		struct MouseInputDevice {
			InputDeviceHeader idh;
			DIMOUSESTATE2 state;
		};
		struct JoypadInputDevice {
			InputDeviceHeader idh;
			DIJOYSTATE state;
			LONG responseThreshold;
		};
	protected:
		HWND hWnd_;

		LPDIRECTINPUT8 pDirectInput_;

		KeyboardInputDevice deviceKeyboard_;
		MouseInputDevice deviceMouse_;
		std::vector<JoypadInputDevice> listDeviceJoypad_;

		DIKeyState bufKey_[MAX_KEY];					//Keyboard key states
		DIKeyState bufMouse_[MAX_MOUSE_BUTTON];			//Mouse key states
		std::vector<std::vector<DIKeyState>> bufPad_;	//Joypad key states

		void _WrapDXErr(HRESULT hr, const std::string& routine, const std::string& msg, bool bThrow = false);

		bool _InitializeKeyBoard();
		bool _InitializeMouse();
		bool _InitializeJoypad();
		BOOL static CALLBACK _GetJoypadStaticCallback(LPDIDEVICEINSTANCE lpddi, LPVOID pvRef);
		BOOL _GetJoypadCallback(LPDIDEVICEINSTANCE lpddi);

		bool _IdleKeyboard();
		bool _IdleJoypad();
		bool _IdleMouse();

		DIKeyState _GetKeyboardKey(UINT code, DIKeyState state);
		DIKeyState _GetMouseButton(int16_t button, DIKeyState state);

		DIKeyState _GetPadButton(int16_t index, int16_t buttonNo, DIKeyState state);
		DIKeyState _GetPadAnalogDirection(int16_t index, uint16_t direction, DIKeyState state);
		DIKeyState _GetPadPovHatDirection(int16_t index, int iHat, uint16_t direction, DIKeyState state);

		DIKeyState _GetStateSub(bool flag, DIKeyState state);
	public:
		DirectInput();
		virtual ~DirectInput();

		static DirectInput* GetBase() { return thisBase_; }

		virtual bool Initialize(HWND hWnd);
		virtual void Update();

		void UnacquireInputDevices();
		void RefreshInputDevices();

		DIKeyState GetKeyState(int16_t key);
		DIKeyState GetMouseState(int16_t button);
		DIKeyState GetPadState(int16_t padNo, int16_t button);

		LONG GetMouseMoveX() { return deviceMouse_.state.lX; }
		LONG GetMouseMoveY() { return deviceMouse_.state.lY; }
		LONG GetMouseMoveZ() { return deviceMouse_.state.lZ; }
		POINT GetMousePosition();

		void ResetInputState();
		void ResetMouseState();
		void ResetKeyState();
		void ResetPadState(int16_t padIndex = -1);

		const KeyboardInputDevice* GetKeyboardDevice() { return &deviceKeyboard_; }
		const MouseInputDevice* GetMouseDevice() { return &deviceMouse_; }

		size_t GetPadDeviceCount() { return listDeviceJoypad_.size(); }
		const JoypadInputDevice* GetPadDevice(int16_t padIndex);
	};

	//*******************************************************************
	//VirtualKeyManager
	//*******************************************************************
	class VirtualKeyManager;
	class VirtualKey {
		friend VirtualKeyManager;
	private:
		int16_t keyboard_;	//Keyboard key ID
		int16_t padIndex_;	//Joypad object index
		int16_t padButton_;	//Joypad key ID
		DIKeyState state_;	//Key state
	public:
		VirtualKey(int16_t keyboard, int16_t padIndex, int16_t padButton);
		virtual ~VirtualKey();

		DIKeyState GetKeyState() { return state_; }
		void SetKeyState(DIKeyState state) { state_ = state; }

		int16_t GetKeyCode() { return keyboard_; }
		void SetKeyCode(int16_t code) { keyboard_ = code; }
		int16_t GetPadIndex() { return padIndex_; }
		void SetPadIndex(int16_t index) { padIndex_ = index; }
		int16_t GetPadButton() { return padButton_; }
		void SetPadButton(int16_t button) { padButton_ = button; }
	};

	class VirtualKeyManager : public DirectInput {
	protected:
		std::map<int16_t, gstd::ref_count_ptr<VirtualKey>> mapKey_;

		DIKeyState _GetVirtualKeyState(int16_t id);
	public:
		VirtualKeyManager();
		~VirtualKeyManager();

		virtual void Update();
		void ClearKeyState();

		void AddKeyMap(int16_t id, gstd::ref_count_ptr<VirtualKey> key) { mapKey_[id] = key; }
		void DeleteKeyMap(int16_t id) { mapKey_.erase(id); };
		void ClearKeyMap() { mapKey_.clear(); };
		DIKeyState GetVirtualKeyState(int16_t id);
		gstd::ref_count_ptr<VirtualKey> GetVirtualKey(int16_t id);
		bool IsTargetKeyCode(int16_t key);
	};

#if defined(DNH_PROJ_EXECUTOR)
	//*******************************************************************
	//KeyReplayManager
	//*******************************************************************
	class KeyReplayManager {
	public:
		enum {
			STATE_RECORD,
			STATE_REPLAY,
		};
	protected:
#pragma pack(push, 2)
		struct ReplayData {
			int16_t id_;
			uint32_t frame_;
			DIKeyState state_;
		};
#pragma pack(pop)

		int state_;
		uint32_t frame_;

		std::list<ReplayData>::iterator replayDataIterator_;
		std::map<int16_t, DIKeyState> mapKeyTarget_;

		std::list<ReplayData> listReplayData_;
		VirtualKeyManager* input_;
	public:
		KeyReplayManager(VirtualKeyManager* input);
		virtual ~KeyReplayManager();

		void SetManageState(int state) { state_ = state; }
		void AddTarget(int16_t key);
		bool IsTargetKeyCode(int16_t key);

		void Update();
		void ReadRecord(gstd::RecordBuffer& record);
		void WriteRecord(gstd::RecordBuffer& record);
	};
#endif
}

//DirectInput Key Code Table
/****************************************************************************
	0x01 DIK_ESCAPE			Esc
	0x02 DIK_1				1
	0x03 DIK_2				2
	0x04 DIK_3				3
	0x05 DIK_4				4
	0x06 DIK_5				5
	0x07 DIK_6				6
	0x08 DIK_7				7
	0x09 DIK_8				8
	0x0A DIK_9				9
	0x0B DIK_0				0
	0x0C DIK_MINUS			-
	0x0D DIK_EQUALS			=
	0x0E DIK_BACK			Backspace
	0x0F DIK_TAB			Tab
	0x10 DIK_Q				Q
	0x11 DIK_W				W
	0x12 DIK_E				E
	0x13 DIK_R				R
	0x14 DIK_T				T
	0x15 DIK_Y				Y
	0x16 DIK_U				U
	0x17 DIK_I				I
	0x18 DIK_O				O
	0x19 DIK_P				P
	0x1A DIK_LBRACKET		[
	0x1B DIK_RBRACKET		]
	0x1C DIK_RETURN			Enter
	0x1D DIK_LCONTROL		Left Ctrl
	0x1E DIK_A				A
	0x1F DIK_S				S
	0x20 DIK_D				D
	0x21 DIK_F				F
	0x22 DIK_G				G
	0x23 DIK_H				H
	0x24 DIK_J				J
	0x25 DIK_K				K
	0x26 DIK_L				L
	0x27 DIK_SEMICOLON		;
	0x28 DIK_APOSTROPHE		'
	0x29 DIK_GRAVE			`
	0x2A DIK_LSHIFT			Left Shift
	0x2B DIK_BACKSLASH		\
	0x2C DIK_Z				Z
	0x2D DIK_X				X
	0x2E DIK_C				C
	0x2F DIK_V				V
	0x30 DIK_B				B
	0x31 DIK_N				N
	0x32 DIK_M				M
	0x33 DIK_COMMA			,
	0x34 DIK_PERIOD			.
	0x35 DIK_SLASH			/
	0x36 DIK_RSHIFT			Right Shift
	0x37 DIK_MULTIPLY		* (Numpad)
	0x38 DIK_LMENU			Left Alt
	0x39 DIK_SPACE			Space
	0x3A DIK_CAPITAL		Caps Lock
	0x3B DIK_F1				F1
	0x3C DIK_F2				F2
	0x3D DIK_F3				F3
	0x3E DIK_F4				F4
	0x3F DIK_F5				F5
	0x40 DIK_F6				F6
	0x41 DIK_F7				F7
	0x42 DIK_F8				F8
	0x43 DIK_F9				F9
	0x44 DIK_F10			F10
	0x45 DIK_NUMLOCK		Num Lock
	0x46 DIK_SCROLL			Scroll Lock
	0x47 DIK_NUMPAD7		7 (Numpad)
	0x48 DIK_NUMPAD8		8 (Numpad)
	0x49 DIK_NUMPAD9		9 (Numpad)
	0x4A DIK_SUBTRACT		- (Numpad)
	0x4B DIK_NUMPAD4		4 (Numpad)
	0x4C DIK_NUMPAD5		5 (Numpad)
	0x4D DIK_NUMPAD6		6 (Numpad)
	0x4E DIK_ADD			+ (Numpad)
	0x4F DIK_NUMPAD1		1 (Numpad)
	0x50 DIK_NUMPAD2		2 (Numpad)
	0x51 DIK_NUMPAD3		3 (Numpad)
	0x52 DIK_NUMPAD0		0 (Numpad)
	0x53 DIK_DECIMAL		. (Numpad)
	0x57 DIK_F11			F11
	0x58 DIK_F12			F12
	0x64 DIK_F13			F13 (NEC PC-98)
	0x65 DIK_F14			F14 (NEC PC-98)
	0x66 DIK_F15			F15 (NEC PC-98)
	0x70 DIK_KANA			Kana (Japanese Keyboard)
	0x79 DIK_CONVERT		Convert (Japanese Keyboard)
	0x7B DIK_NOCONVERT		No Convert (Japanese Keyboard)
	0x7D DIK_YEN			¥ (Japanese Keyboard, \ for English Keyboard)
	0x8D DIK_NUMPADEQUALS	= (Numpad, NEC PC-98)
	0x90 DIK_CIRCUMFLEX		^ (Japanese Keyboard)
	0x91 DIK_AT				@ (NEC PC-98)
	0x92 DIK_COLON			: (NEC PC-98)
	0x93 DIK_UNDERLINE		_ (NEC PC-98)
	0x94 DIK_KANJI			Kanji (Japanese Keyboard)
	0x95 DIK_STOP			Stop (NEC PC-98)
	0x96 DIK_AX				(Japan AX)
	0x97 DIK_UNLABELED		(J3100)
	0x9C DIK_NUMPADENTER	Enter (Numpad)
	0x9D DIK_RCONTROL		Right Ctrl
	0xB3 DIK_NUMPADCOMMA	, (Numpad, NEC PC-98)
	0xB5 DIK_DIVIDE			/ (Numpad)
	0xB7 DIK_SYSRQ			Sys Rq
	0xB8 DIK_RMENU			Right Alt
	0xC5 DIK_PAUSE			Pause
	0xC7 DIK_HOME			Home
	0xC8 DIK_UP				↑
	0xC9 DIK_PRIOR			Page Up
	0xCB DIK_LEFT			←
	0xCD DIK_RIGHT			→
	0xCF DIK_END			End
	0xD0 DIK_DOWN			↓
	0xD1 DIK_NEXT			Page Down
	0xD2 DIK_INSERT			Insert
	0xD3 DIK_DELETE			Delete
	0xDB DIK_LWIN			Left Windows
	0xDC DIK_RWIN			Right Windows
	0xDD DIK_APPS			Menu
	0xDE DIK_POWER			Power
	0xDF DIK_SLEEP			Sleep
****************************************************************************/

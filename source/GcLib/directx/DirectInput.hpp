#ifndef __DIRECTX_DIRECTINPUT__
#define __DIRECTX_DIRECTINPUT__

#include "../pch.h"
#include "DxConstant.hpp"

namespace directx {
	enum {
		DI_MOUSE_LEFT = 0,
		DI_MOUSE_RIGHT = 1,
		DI_MOUSE_MIDDLE = 2,
	};

	enum {
		KEY_FREE = 0,	 // キーが押されていない状態
		KEY_PUSH = 1,	 // キーを押した瞬間
		KEY_PULL = 2,	 // キーが離された瞬間
		KEY_HOLD = 3,	 // キーが押されている状態
	};

	/**********************************************************
	//DirectInput
	**********************************************************/
	class DirectInput {
		static DirectInput* thisBase_;
	public:
		enum {
			MAX_JOYPAD = 4,
			MAX_KEY = 256,
			MAX_MOUSE_BUTTON = 4,
			MAX_PAD_BUTTON = 16,
		};

	protected:
		HWND hWnd_;
		LPDIRECTINPUT8 pInput_;
		LPDIRECTINPUTDEVICE8 pKeyboard_;
		LPDIRECTINPUTDEVICE8 pMouse_;
		std::vector<LPDIRECTINPUTDEVICE8> pJoypad_;	// パッドデバイスオブジェクト
		BYTE stateKey_[MAX_KEY];
		DIMOUSESTATE stateMouse_;
		std::vector<DIJOYSTATE> statePad_;
		std::vector<int> padRes_;//パッドの遊び

		int bufKey_[MAX_KEY];//現フレームのキーの状態
		int bufMouse_[MAX_MOUSE_BUTTON];//現フレームのマウスの状態
		std::vector<std::vector<int> > bufPad_;//現フレームのパッドの状態			

		bool _InitializeKeyBoard();
		bool _InitializeMouse();
		bool _InitializeJoypad();
		BOOL static CALLBACK _GetJoypadStaticCallback(LPDIDEVICEINSTANCE lpddi, LPVOID pvRef);
		BOOL _GetJoypadCallback(LPDIDEVICEINSTANCE lpddi);

		bool _IdleKeyboard();
		bool _IdleJoypad();
		bool _IdleMouse();

		int _GetKey(UINT code, int state);//キー状態の取得
		int _GetMouseButton(int button, int state);//マウスのボタンの状態を取得
		int _GetPadDirection(int index, UINT code, int state);//ジョイスティック方向キーの状態取得
		int _GetPadButton(int index, int buttonNo, int state);//ジョイスティックのボタンの状態を取得
		int _GetStateSub(bool flag, int state);

	public:
		DirectInput();
		virtual ~DirectInput();
		static DirectInput* GetBase() { return thisBase_; }

		virtual bool Initialize(HWND hWnd);

		virtual void Update();//キーをセットする
		int GetKeyState(int key);
		int GetMouseState(int button);
		int GetPadState(int padNo, int button);

		int GetMouseMoveX() { return stateMouse_.lX; }//マウスの移動量を取得X
		int GetMouseMoveY() { return stateMouse_.lY; }//マウスの移動量を取得Y
		int GetMouseMoveZ() { return stateMouse_.lZ; }//マウスの移動量を取得Z
		POINT GetMousePosition();

		void ResetInputState();
		void ResetMouseState();
		void ResetKeyState();
		void ResetPadState();

		int GetPadDeviceCount() { return bufPad_.size(); }
		DIDEVICEINSTANCE GetPadDeviceInformation(int padIndex);
	};

	/**********************************************************
	//VirtualKeyManager
	**********************************************************/
	class VirtualKeyManager;
	class VirtualKey {
		friend VirtualKeyManager;
	private:
		int keyboard_;//キーボードのキー
		int padIndex_;//ジョイパッドの番号
		int padButton_;//ジョイパッドのボタン
		int state_;//現在の状態

	public:
		VirtualKey(int keyboard, int padIndex, int padButton);
		virtual ~VirtualKey();
		int GetKeyState() { return state_; }
		void SetKeyState(int state) { state_ = state; }

		int GetKeyCode() { return keyboard_; }
		void SetKeyCode(int code) { keyboard_ = code; }
		int GetPadIndex() { return padIndex_; }
		void SetPadIndex(int index) { padIndex_ = index; }
		int GetPadButton() { return padButton_; }
		void SetPadButton(int button) { padButton_ = button; }

	};

	class VirtualKeyManager : public DirectInput {
	protected:
		std::map<int, gstd::ref_count_ptr<VirtualKey> > mapKey_;

		int _GetVirtualKeyState(int id);

	public:
		VirtualKeyManager();
		~VirtualKeyManager();
		virtual void Update();//キーをセットする
		void ClearKeyState();

		void AddKeyMap(int id, gstd::ref_count_ptr<VirtualKey> key) { mapKey_[id] = key; }
		void DeleteKeyMap(int id) { mapKey_.erase(id); };
		void ClearKeyMap() { mapKey_.clear(); };
		int GetVirtualKeyState(int id);
		gstd::ref_count_ptr<VirtualKey> GetVirtualKey(int id);
		bool IsTargetKeyCode(int key);
	};

	/**********************************************************
	//KeyReplayManager
	**********************************************************/
	class KeyReplayManager {
	public:
		enum {
			STATE_RECORD,
			STATE_REPLAY,
		};
	protected:
		struct ReplayData {
			int id_;
			int frame_;
			int state_;
		};

		int state_;
		int frame_;
		std::map<int, int> mapLastKeyState_;
		std::list<int> listTarget_;
		std::list<ReplayData> listReplayData_;
		VirtualKeyManager* input_;
	public:
		KeyReplayManager(VirtualKeyManager* input);
		virtual ~KeyReplayManager();
		void SetManageState(int state) { state_ = state; }
		void AddTarget(int key);
		bool IsTargetKeyCode(int key);

		void Update();
		void ReadRecord(gstd::RecordBuffer& record);
		void WriteRecord(gstd::RecordBuffer& record);
	};
}

/* DirectInput キー識別コード表
DIK_ESCAPE 0x01 Esc
DIK_1 0x02 1
DIK_2 0x03 2
DIK_3 0x04 3
DIK_4 0x05 4
DIK_5 0x06 5
DIK_6 0x07 6
DIK_7 0x08 7
DIK_8 0x09 8
DIK_9 0x0A 9
DIK_0 0x0B 0
DIK_MINUS 0x0C -
DIK_EQUALS 0x0D =
DIK_BACK 0x0E Back Space
DIK_TAB 0x0F Tab
DIK_Q 0x10 Q
DIK_W 0x11 W
DIK_E 0x12 E
DIK_R 0x13 R
DIK_T 0x14 T
DIK_Y 0x15 Y
DIK_U 0x16 U
DIK_I 0x17 I
DIK_O 0x18 O
DIK_P 0x19 P
DIK_LBRACKET 0x1A [
DIK_RBRACKET 0x1B ]
DIK_RETURN 0x1C Enter
DIK_LCONTROL 0x1D Ctrl (Left)
DIK_A 0x1E A
DIK_S 0x1F S
DIK_D 0x20 D
DIK_F 0x21 F
DIK_G 0x22 G
DIK_H 0x23 H
DIK_J 0x24 J
DIK_K 0x25 K
DIK_L 0x26 L
DIK_SEMICOLON 0x27 ;
DIK_APOSTROPHE 0x28 '
DIK_GRAVE 0x29 `
DIK_LSHIFT 0x2A Shift (Left)
DIK_BACKSLASH 0x2B \
DIK_Z 0x2C Z
DIK_X 0x2D X
DIK_C 0x2E C
DIK_V 0x2F V
DIK_B 0x30 B
DIK_N 0x31 N
DIK_M 0x32 M
DIK_COMMA 0x33 ,
DIK_PERIOD 0x34 .
DIK_SLASH 0x35 /
DIK_RSHIFT 0x36 Shift (Right)
DIK_MULTIPLY 0x37 * (Numpad)
DIK_LMENU 0x38 Alt (Left)
DIK_SPACE 0x39 Space
DIK_CAPITAL 0x3A Caps Lock
DIK_F1 0x3B F1
DIK_F2 0x3C F2
DIK_F3 0x3D F3
DIK_F4 0x3E F4
DIK_F5 0x3F F5
DIK_F6 0x40 F6
DIK_F7 0x41 F7
DIK_F8 0x42 F8
DIK_F9 0x43 F9
DIK_F10 0x44 F10
DIK_NUMLOCK 0x45 Num Lock
DIK_SCROLL 0x46 Scroll Lock
DIK_NUMPAD7 0x47 7 (Numpad)
DIK_NUMPAD8 0x48 8 (Numpad)
DIK_NUMPAD9 0x49 9 (Numpad)
DIK_SUBTRACT 0x4A - (Numpad)
DIK_NUMPAD4 0x4B 4 (Numpad)
DIK_NUMPAD5 0x4C 5 (Numpad)
DIK_NUMPAD6 0x4D 6 (Numpad)
DIK_ADD 0x4E + (Numpad)
DIK_NUMPAD1 0x4F 1 (Numpad)
DIK_NUMPAD2 0x50 2 (Numpad)
DIK_NUMPAD3 0x51 3 (Numpad)
DIK_NUMPAD0 0x52 0 (Numpad)
DIK_DECIMAL 0x53 . (Numpad)
DIK_F11 0x57 F11
DIK_F12 0x58 F12
DIK_F13 0x64 F13 NEC PC-98
DIK_F14 0x65 F14 NEC PC-98
DIK_F15 0x66 F15 NEC PC-98
DIK_KANA 0x70 カナ 日本語キーボード
DIK_CONVERT 0x79 変換 日本語キーボード
DIK_NOCONVERT 0x7B 無変換 日本語キーボード
DIK_YEN 0x7D \ 日本語キーボード
DIK_NUMPADEQUALS 0x8D = (Numpad) NEC PC-98
DIK_CIRCUMFLEX 0x90 不明 日本語キーボード
DIK_AT 0x91 @ NEC PC-98
DIK_COLON 0x92 : NEC PC-98
DIK_UNDERLINE 0x93 _ NEC PC-98
DIK_KANJI 0x94 漢字 日本語キーボード
DIK_STOP 0x95 Stop NEC PC-98
DIK_AX 0x96 (Japan AX)
DIK_UNLABELED 0x97 (J3100)
DIK_NUMPADENTER 0x9C Enter (Numpad)
DIK_RCONTROL 0x9D Ctrl (Right)
DIK_NUMPADCOMMA 0xB3 , (Numpad) NEC PC-98
DIK_DIVIDE 0xB5 / (Numpad)
DIK_SYSRQ 0xB7 Sys Rq
DIK_RMENU 0xB8 Alt (Right)
DIK_PAUSE 0xC5 Pause
DIK_HOME 0xC7 Home
DIK_UP 0xC8 ↑
DIK_PRIOR 0xC9 Page Up
DIK_LEFT 0xCB ←
DIK_RIGHT 0xCD →
DIK_END 0xCF End
DIK_DOWN 0xD0 ↓
DIK_NEXT 0xD1 Page Down
DIK_INSERT 0xD2 Insert
DIK_DELETE 0xD3 Delete
DIK_LWIN 0xDB Windows (Left)
DIK_RWIN 0xDC Windows (Right)
DIK_APPS 0xDD Menu
DIK_POWER 0xDE Power
DIK_SLEEP 0xDF Sleep
*/

#endif

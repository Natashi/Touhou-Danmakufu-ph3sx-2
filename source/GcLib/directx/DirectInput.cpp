#include "source/GcLib/pch.h"

#include "DirectInput.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//DirectInput
**********************************************************/
DirectInput* DirectInput::thisBase_ = nullptr;
DirectInput::DirectInput() {
	hWnd_ = nullptr;
	pInput_ = nullptr;
	pKeyboard_ = nullptr;
	pMouse_ = nullptr;
}
DirectInput::~DirectInput() {
	Logger::WriteTop("DirectInput: Finalizing:");
	for (int iPad = 0; iPad < pJoypad_.size(); iPad++) {
		if (pJoypad_[iPad] == nullptr)continue;
		pJoypad_[iPad]->Unacquire();
		if (pJoypad_[iPad] != nullptr)pJoypad_[iPad]->Release();
	}

	if (pMouse_ != nullptr) {
		pMouse_->Unacquire();
		pMouse_->Release();
	}

	if (pKeyboard_ != nullptr) {
		pKeyboard_->Unacquire();
		pKeyboard_->Release();
	}

	if (pInput_ != nullptr)pInput_->Release();
	thisBase_ = nullptr;
	Logger::WriteTop("DirectInput: Finalized.");
}

bool DirectInput::Initialize(HWND hWnd) {
	if (thisBase_ != nullptr)return false;
	Logger::WriteTop("DirectInput: Initialize.");
	hWnd_ = hWnd;

	HINSTANCE hInst = ::GetModuleHandle(nullptr);
	HRESULT hrInput = DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&pInput_, nullptr);
	if (FAILED(hrInput)) {
		Logger::WriteTop("DirectInput::DirectInput8Create failure.");
		return false;  // DirectInput8の作成に失敗
	}

	_InitializeKeyBoard();
	_InitializeMouse();
	_InitializeJoypad();

	bufPad_.resize(pJoypad_.size());
	for (int iPad = 0; iPad < pJoypad_.size(); iPad++) {
		bufPad_[iPad].resize(32);
	}

	thisBase_ = this;
	Logger::WriteTop("DirectInput: Initialized.");
	return true;
}

bool DirectInput::_InitializeKeyBoard() {
	Logger::WriteTop("DirectInput: Initializing keyboard.");

	HRESULT hrDevice = pInput_->CreateDevice(GUID_SysKeyboard, &pKeyboard_, nullptr);
	if (FAILED(hrDevice)) {
		Logger::WriteTop("DirectInput: Failed to create keyboard object.");
		return false;
	}

	HRESULT hrFormat = pKeyboard_->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hrFormat)) {
		Logger::WriteTop("DirectInput: Failed to set keyboard data format.");
		return false;
	}

	HRESULT hrCoop = pKeyboard_->SetCooperativeLevel(hWnd_, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	if (FAILED(hrCoop)) {
		Logger::WriteTop("DirectInput: Failed to test keyboard cooperative level.");
		return false;
	}

	// 入力制御開始
	pKeyboard_->Acquire();

	Logger::WriteTop("DirectInput: Keyboard initialized.");

	return true;
}
bool DirectInput::_InitializeMouse() {
	Logger::WriteTop("DirectInput: Initializing mouse.");

	HRESULT hrDevice = pInput_->CreateDevice(GUID_SysMouse, &pMouse_, nullptr);
	if (FAILED(hrDevice)) {
		Logger::WriteTop("DirectInput: Failed to create mouse object.");
		return false;
	}

	HRESULT hrFormat = pMouse_->SetDataFormat(&c_dfDIMouse);
	if (FAILED(hrFormat)) {
		Logger::WriteTop("DirectInput: Failed to set mouse data format.");
		return false;
	}

	HRESULT hrCoop = pMouse_->SetCooperativeLevel(hWnd_, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	if (FAILED(hrCoop)) {
		Logger::WriteTop("DirectInput: Failed to test mouse cooperative level.");
		return false;
	}

	// 入力制御開始
	pMouse_->Acquire();

	Logger::WriteTop("DirectInput: Mouse initialized.");
	return true;
}
bool DirectInput::_InitializeJoypad() {
	Logger::WriteTop("DirectInput: Initializing joypad.");
	pInput_->EnumDevices(DI8DEVCLASS_GAMECTRL, (LPDIENUMDEVICESCALLBACK)_GetJoypadStaticCallback, this, DIEDFL_ATTACHEDONLY);
	int count = pJoypad_.size();
	if (count == 0) {
		Logger::WriteTop("DirectInput: Cannot connect to a joypad.");
		return false;	// ジョイパッドが見付からない
	}

	statePad_.resize(count);
	padRes_.resize(count);
	for (int iPad = 0; iPad < count; iPad++)
		padRes_[iPad] = 500;

	Logger::WriteTop("DirectInput: Joypad initialized.");

	return true;
}
BOOL CALLBACK DirectInput::_GetJoypadStaticCallback(LPDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
	DirectInput* input = (DirectInput*)pvRef;
	return input->_GetJoypadCallback(lpddi);
}
BOOL DirectInput::_GetJoypadCallback(LPDIDEVICEINSTANCE lpddi) {
	Logger::WriteTop("DirectInput: Connecting to a joypad.");
	LPDIRECTINPUTDEVICE8 pJoypad = nullptr;
	HRESULT hrDevice = pInput_->CreateDevice(lpddi->guidInstance, &pJoypad, nullptr);
	if (FAILED(hrDevice)) {
		Logger::WriteTop("DirectInput: Failed to create joypad object.");
		return DIENUM_CONTINUE;
	}

	// 情報表示
	{
		DIDEVICEINSTANCE State;
		ZeroMemory(&State, sizeof(State));
		State.dwSize = sizeof(State);
		pJoypad->GetDeviceInfo(&State);

		Logger::WriteTop(StringUtility::Format("Device name:%s", State.tszInstanceName));
		Logger::WriteTop(StringUtility::Format("Product name:%s", State.tszProductName));
	}

	HRESULT hrFormat = pJoypad->SetDataFormat(&c_dfDIJoystick);
	if (FAILED(hrFormat)) {
		if (pJoypad != nullptr)pJoypad->Release();
		Logger::WriteTop("DirectInput: Failed to set mouse data format.");
		return DIENUM_CONTINUE;
	}

	HRESULT hrCoop = pJoypad->SetCooperativeLevel(hWnd_, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	if (FAILED(hrCoop)) {
		if (pJoypad != nullptr)pJoypad->Release();
		Logger::WriteTop("DirectInput: Failed to test mouse cooperative level.");
		return DIENUM_CONTINUE;
	}

	// xの範囲を設定
	DIPROPRANGE diprg;
	diprg.diph.dwSize = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwObj = DIJOFS_X;
	diprg.diph.dwHow = DIPH_BYOFFSET;
	diprg.lMin = -1000;
	diprg.lMax = +1000;
	HRESULT hrRangeX = pJoypad->SetProperty(DIPROP_RANGE, &diprg.diph);
	if (FAILED(hrRangeX)) {
		if (pJoypad != nullptr)pJoypad->Release();
		Logger::WriteTop("DirectInput: Failed to set joypad X range.");
		return DIENUM_CONTINUE;
	}

	// yの範囲を設定
	diprg.diph.dwObj = DIJOFS_Y;
	HRESULT hrRangeY = pJoypad->SetProperty(DIPROP_RANGE, &diprg.diph);
	if (FAILED(hrRangeY)) {
		if (pJoypad != nullptr)pJoypad->Release();
		Logger::WriteTop("DirectInput: Failed to set joypad Y range.");
		return DIENUM_CONTINUE;
	}

	// zの範囲を設定
	diprg.diph.dwObj = DIJOFS_Z;
	HRESULT hrRangeZ = pJoypad->SetProperty(DIPROP_RANGE, &diprg.diph);
	if (FAILED(hrRangeZ)) {
		Logger::WriteTop("DirectInput: Failed to set joypad Z range.");
	}

	// xの無効ゾーンを設定
	DIPROPDWORD dipdw;
	dipdw.diph.dwSize = sizeof(dipdw);
	dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
	dipdw.diph.dwObj = DIJOFS_X;
	dipdw.diph.dwHow = DIPH_BYOFFSET;
	dipdw.dwData = 2500;
	HRESULT hrDeadX = pJoypad->SetProperty(DIPROP_DEADZONE, &dipdw.diph);
	if (FAILED(hrDeadX)) {
		if (pJoypad != nullptr)pJoypad->Release();
		Logger::WriteTop("DirectInput: Failed to set joypad X deadzone.");
		return DIENUM_CONTINUE;
	}

	// yの無効ゾーンを設定
	dipdw.diph.dwObj = DIJOFS_Y;
	HRESULT hrDeadY = pJoypad->SetProperty(DIPROP_DEADZONE, &dipdw.diph);
	if (FAILED(hrDeadY)) {
		if (pJoypad != nullptr)pJoypad->Release();
		Logger::WriteTop("DirectInput: Failed to set joypad Y deadzone.");
		return DIENUM_CONTINUE;
	}

	// Ｚの無効ゾーンを設定
	dipdw.diph.dwObj = DIJOFS_Z;
	HRESULT hrDeadZ = pJoypad->SetProperty(DIPROP_DEADZONE, &dipdw.diph);
	if (FAILED(hrDeadZ)) {
		Logger::WriteTop("DirectInput: Failed to set joypad Z deadzone.");
	}

	// 入力制御開始
	pJoypad->Acquire();

	pJoypad_.push_back(pJoypad);

	return DIENUM_CONTINUE;
}

int DirectInput::_GetKey(UINT code, int state) {
	return _GetStateSub((stateKey_[code] & 0x80) == 0x80, state);
}
int DirectInput::_GetMouseButton(int button, int state) {
	return _GetStateSub((stateMouse_.rgbButtons[button] & 0x80) == 0x80, state);
}
int DirectInput::_GetPadDirection(int index, UINT code, int state) {
	if (index >= pJoypad_.size())return KEY_FREE;
	int response = padRes_[index];

	int res = KEY_FREE;

	switch (code) {
	case DIK_UP:
		res = _GetStateSub(statePad_[index].lY < -response, state);
		break;
	case DIK_DOWN:
		res = _GetStateSub(statePad_[index].lY > response, state);
		break;
	case DIK_LEFT:
		res = _GetStateSub(statePad_[index].lX < -response, state);
		break;
	case DIK_RIGHT:
		res = _GetStateSub(statePad_[index].lX > response, state);
		break;
	}

	return res;
}
int DirectInput::_GetPadButton(int index, int buttonNo, int state) {
	return _GetStateSub((statePad_[index].rgbButtons[buttonNo] & 0x80) == 0x80, state);
}
int DirectInput::_GetStateSub(bool flag, int state) {
	int res = KEY_FREE;
	if (flag) {
		if (state == KEY_FREE)
			res = KEY_PUSH;
		else
			res = KEY_HOLD;
	}
	else {
		if (state == KEY_PUSH || state == KEY_HOLD)
			res = KEY_PULL;
		else
			res = KEY_FREE;
	}
	return res;
}

bool DirectInput::_IdleKeyboard() {
	if (!pInput_ || !pKeyboard_)
		return false;

	HRESULT hr = pKeyboard_->GetDeviceState(MAX_KEY, stateKey_);
	if (SUCCEEDED(hr)) {

	}
	else if (hr == DIERR_INPUTLOST) {
		pKeyboard_->Acquire();
	}
	return true;
}
bool DirectInput::_IdleJoypad() {
	if (pJoypad_.size() == 0)return false;
	for (int iPad = 0; iPad < pJoypad_.size(); iPad++) {
		if (!pInput_ || !pJoypad_[iPad])
			return false;

		pJoypad_[iPad]->Poll();
		HRESULT hr = pJoypad_[iPad]->GetDeviceState(sizeof(DIJOYSTATE), &statePad_[iPad]);
		if (SUCCEEDED(hr)) {

		}
		else if (hr == DIERR_INPUTLOST) {
			pJoypad_[iPad]->Acquire();
		}
	}
	return true;
}
bool DirectInput::_IdleMouse() {
	if (!pInput_ || !pMouse_)
		return false;

	HRESULT hr = pMouse_->GetDeviceState(sizeof(DIMOUSESTATE), &stateMouse_);
	if (SUCCEEDED(hr)) {

	}
	else if (hr == DIERR_INPUTLOST) {
		pMouse_->Acquire();
	}

	return true;
}

void DirectInput::Update() {
	this->_IdleKeyboard();
	this->_IdleJoypad();
	this->_IdleMouse();

	for (int iKey = 0; iKey < MAX_KEY; iKey++)
		bufKey_[iKey] = _GetKey(iKey, bufKey_[iKey]);

	for (int iButton = 0; iButton < 3; iButton++)
		bufMouse_[iButton] = _GetMouseButton(iButton, bufMouse_[iButton]);

	for (int iPad = 0; iPad < pJoypad_.size(); iPad++) {
		bufPad_[iPad][0] = _GetPadDirection(iPad, DIK_LEFT, bufPad_[iPad][0]);
		bufPad_[iPad][1] = _GetPadDirection(iPad, DIK_RIGHT, bufPad_[iPad][1]);
		bufPad_[iPad][2] = _GetPadDirection(iPad, DIK_UP, bufPad_[iPad][2]);
		bufPad_[iPad][3] = _GetPadDirection(iPad, DIK_DOWN, bufPad_[iPad][3]);

		for (int iButton = 0; iButton < MAX_PAD_BUTTON; iButton++)
			bufPad_[iPad][iButton + 4] = _GetPadButton(iPad, iButton, bufPad_[iPad][iButton + 4]);
	}
}
int DirectInput::GetKeyState(int key) {
	if (key < 0 || key >= MAX_KEY)
		return KEY_FREE;
	return bufKey_[key];
}
int DirectInput::GetMouseState(int button) {
	if (button < 0 || button >= MAX_MOUSE_BUTTON)
		return KEY_FREE;
	return bufMouse_[button];
}
int DirectInput::GetPadState(int padNo, int button) {
	int res = KEY_FREE;
	if (padNo < bufPad_.size())
		res = bufPad_[padNo][button];
	return res;
}

POINT DirectInput::GetMousePosition() {
	POINT res = { 0, 0 };
	GetCursorPos(&res);
	ScreenToClient(hWnd_, &res);
	return res;
}

void DirectInput::ResetInputState() {
	ResetMouseState();
	ResetKeyState();
	ResetPadState();
}
void DirectInput::ResetMouseState() {
	for (int iButton = 0; iButton < 3; iButton++)
		bufMouse_[iButton] = KEY_FREE;
	ZeroMemory(&stateMouse_, sizeof(stateMouse_));
}
void DirectInput::ResetKeyState() {
	for (int iKey = 0; iKey < MAX_KEY; iKey++)
		bufKey_[iKey] = KEY_FREE;
	ZeroMemory(&stateKey_, sizeof(stateKey_));
}
void DirectInput::ResetPadState() {
	for (int iPad = 0; iPad < bufPad_.size(); iPad++) {
		for (int iKey = 0; iKey < bufPad_.size(); iKey++)
			bufPad_[iPad][iKey] = KEY_FREE;
		statePad_[iPad].lX = 0;
		statePad_[iPad].lY = 0;
		for (int iButton = 0; iButton < MAX_PAD_BUTTON; iButton++) {
			statePad_[iPad].rgbButtons[iButton] = KEY_FREE;
		}
	}
}
DIDEVICEINSTANCE DirectInput::GetPadDeviceInformation(int padIndex) {
	DIDEVICEINSTANCE state;
	ZeroMemory(&state, sizeof(state));
	state.dwSize = sizeof(state);
	pJoypad_[padIndex]->GetDeviceInfo(&state);
	return state;
}


/**********************************************************
//VirtualKeyManager
**********************************************************/
//VirtualKey
VirtualKey::VirtualKey(int keyboard, int padIndex, int padButton) {
	keyboard_ = keyboard;
	padIndex_ = padIndex;
	padButton_ = padButton;
	state_ = KEY_FREE;
}
VirtualKey::~VirtualKey() {}


//VirtualKeyManager
VirtualKeyManager::VirtualKeyManager() {

}
VirtualKeyManager::~VirtualKeyManager() {

}

void VirtualKeyManager::Update() {
	DirectInput::Update();

	std::map<int, gstd::ref_count_ptr<VirtualKey> >::iterator itr = mapKey_.begin();
	for (; itr != mapKey_.end(); itr++) {
		int id = itr->first;
		gstd::ref_count_ptr<VirtualKey> key = itr->second;

		int state = _GetVirtualKeyState(id);
		key->SetKeyState(state);
	}
}
void VirtualKeyManager::ClearKeyState() {
	DirectInput::ResetInputState();
	std::map<int, gstd::ref_count_ptr<VirtualKey> >::iterator itr = mapKey_.begin();
	for (; itr != mapKey_.end(); itr++) {
		gstd::ref_count_ptr<VirtualKey> key = itr->second;
		key->SetKeyState(KEY_FREE);
	}
}
int VirtualKeyManager::_GetVirtualKeyState(int id) {
	if (mapKey_.find(id) == mapKey_.end())return KEY_FREE;

	gstd::ref_count_ptr<VirtualKey> key = mapKey_[id];

	int res = KEY_FREE;
	if (key->keyboard_ >= 0 && key->keyboard_ < MAX_KEY)
		res = bufKey_[key->keyboard_];
	if (res == KEY_FREE) {
		int indexPad = key->padIndex_;
		if (indexPad >= 0 && indexPad < pJoypad_.size()) {
			if (key->padButton_ >= 0 && key->padButton_ < bufPad_[indexPad].size())
				res = bufPad_[indexPad][key->padButton_];
		}
	}

	return res;
}

int VirtualKeyManager::GetVirtualKeyState(int id) {
	if (mapKey_.find(id) == mapKey_.end())return KEY_FREE;
	gstd::ref_count_ptr<VirtualKey> key = mapKey_[id];
	return key->GetKeyState();
}

gstd::ref_count_ptr<VirtualKey> VirtualKeyManager::GetVirtualKey(int id) {
	if (mapKey_.find(id) == mapKey_.end())return nullptr;
	return mapKey_[id];
}
bool VirtualKeyManager::IsTargetKeyCode(int key) {
	bool res = false;
	std::map<int, gstd::ref_count_ptr<VirtualKey> >::iterator itr = mapKey_.begin();
	for (; itr != mapKey_.end(); itr++) {
		gstd::ref_count_ptr<VirtualKey> vKey = itr->second;
		int keyCode = vKey->GetKeyCode();
		if (key == keyCode) {
			res = true;
			break;
		}
	}
	return res;
}

/**********************************************************
//KeyReplayManager
**********************************************************/
KeyReplayManager::KeyReplayManager(VirtualKeyManager* input) {
	frame_ = 0;
	input_ = input;
	state_ = STATE_RECORD;
}
KeyReplayManager::~KeyReplayManager() {}
void KeyReplayManager::AddTarget(int key) {
	listTarget_.push_back(key);
	mapLastKeyState_[key] = KEY_FREE;
}
void KeyReplayManager::Update() {
	if (state_ == STATE_RECORD) {
		std::list<int>::iterator itrTarget = listTarget_.begin();
		for (; itrTarget != listTarget_.end(); itrTarget++) {
			int idKey = *itrTarget;
			int keyState = input_->GetVirtualKeyState(idKey);
			bool bInsert = (frame_ == 0 || mapLastKeyState_[idKey] != keyState);
			if (bInsert) {
				ReplayData data;
				data.id_ = idKey;
				data.frame_ = frame_;
				data.state_ = keyState;
				listReplayData_.push_back(data);
			}
			mapLastKeyState_[idKey] = keyState;
		}
	}
	else if (state_ == STATE_REPLAY) {
		std::list<int>::iterator itrTarget = listTarget_.begin();
		for (; itrTarget != listTarget_.end(); itrTarget++) {
			int idKey = *itrTarget;
			std::list<ReplayData>::iterator itrData = listReplayData_.begin();
			for (; itrData != listReplayData_.end();) {
				ReplayData data = *itrData;
				if (data.frame_ > frame_)break;

				if (idKey == data.id_ && data.frame_ == frame_) {
					mapLastKeyState_[idKey] = data.state_;
					itrData = listReplayData_.erase(itrData);
				}
				else {
					itrData++;
				}
			}

			gstd::ref_count_ptr<VirtualKey> key = input_->GetVirtualKey(idKey);
			int lastKeyState = mapLastKeyState_[idKey];
			key->SetKeyState(lastKeyState);
		}
	}
	frame_++;
}
bool KeyReplayManager::IsTargetKeyCode(int key) {
	bool res = false;
	std::list<int>::iterator itrTarget = listTarget_.begin();
	for (; itrTarget != listTarget_.end(); itrTarget++) {
		int idKey = *itrTarget;
		gstd::ref_count_ptr<VirtualKey> vKey = input_->GetVirtualKey(idKey);
		int keyCode = vKey->GetKeyCode();
		if (key == keyCode) {
			res = true;
			break;
		}
	}
	return res;
}
void KeyReplayManager::ReadRecord(gstd::RecordBuffer& record) {
	int countReplayData = record.GetRecordAsInteger("count");

	ByteBuffer buffer;
	buffer.SetSize(sizeof(ReplayData) * countReplayData);
	std::string key = "data";
	record.GetRecord(key, buffer.GetPointer(), buffer.GetSize());

	for (int iRec = 0; iRec < countReplayData; iRec++) {
		ReplayData data;
		buffer.Read(&data, sizeof(ReplayData));
		listReplayData_.push_back(data);
	}
}
void KeyReplayManager::WriteRecord(gstd::RecordBuffer& record) {
	int countReplayData = listReplayData_.size();
	record.SetRecordAsInteger("count", countReplayData);

	ByteBuffer buffer;
	std::list<ReplayData>::iterator itrData = listReplayData_.begin();
	for (; itrData != listReplayData_.end(); itrData++) {
		ReplayData data = *itrData;
		buffer.Write(&data, sizeof(ReplayData));
	}

	{
		std::string key = "data";
		record.SetRecord(key, buffer.GetPointer(), buffer.GetSize());
	}
}


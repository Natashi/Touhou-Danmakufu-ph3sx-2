#include "source/GcLib/pch.h"

#include "DirectInput.hpp"

using namespace gstd;
using namespace directx;

//*******************************************************************
//DirectInput
//*******************************************************************
DirectInput* DirectInput::thisBase_ = nullptr;
DirectInput::DirectInput() {
	hWnd_ = nullptr;

	pDirectInput_ = nullptr;
}
DirectInput::~DirectInput() {
	if (this != thisBase_) return;

	Logger::WriteTop("DirectInput: Finalizing:");

	UnacquireInputDevices();

	ptr_release(pDirectInput_);

	thisBase_ = nullptr;
	Logger::WriteTop("DirectInput: Finalized.");
}

bool DirectInput::Initialize(HWND hWnd) {
	if (thisBase_) return false;
	Logger::WriteTop("DirectInput: Initialize.");
	hWnd_ = hWnd;

	HINSTANCE hInst = ::GetModuleHandle(nullptr);

	HRESULT hr = DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&pDirectInput_, nullptr);
	_WrapDXErr(hr, "Initialize", "DirectInput8Create failure.", true);

	RefreshInputDevices();

	thisBase_ = this;

	Logger::WriteTop("DirectInput: Initialized.");
	return true;
}

void DirectInput::_WrapDXErr(HRESULT hr, const std::string& routine, const std::string& msg, bool bThrow) {
	if (SUCCEEDED(hr)) return;
	std::string err = StringUtility::Format("DirectInput::%s: %s. [%s]\r\n  %s",
		routine.c_str(), msg.c_str(), DXGetErrorStringA(hr), DXGetErrorDescriptionA(hr));
	Logger::WriteTop(err);
	if (bThrow) throw wexception(err);
}

void DirectInput::UnacquireInputDevices() {
	deviceKeyboard_.idh.Unacquire();
	deviceMouse_.idh.Unacquire();
	for (auto& iPad : listDeviceJoypad_) {
		iPad.idh.Unacquire();
	}
	listDeviceJoypad_.clear();
}
void DirectInput::RefreshInputDevices() {
	UnacquireInputDevices();

	_InitializeKeyBoard();

	_InitializeMouse();

	_InitializeJoypad();
	bufPad_.resize(listDeviceJoypad_.size());
	for (int16_t iPad = 0; iPad < listDeviceJoypad_.size(); ++iPad) {
		bufPad_[iPad].resize(MAX_PAD_STATE);
	}
}

HRESULT DirectInput::InputDeviceHeader::QueryDeviceInstance() {
	if (pDevice == nullptr)
		return E_FAIL;

	ZeroMemory(&ddi, sizeof(DIDEVICEINSTANCE));
	ddi.dwSize = sizeof(DIDEVICEINSTANCE);

	HRESULT hr = pDevice->GetDeviceInfo(&ddi);

	return hr;
}
HRESULT DirectInput::InputDeviceHeader::QueryHidPath() {
	if (pDevice == nullptr)
		return E_FAIL;

	DIPROPGUIDANDPATH dip;
	dip.diph.dwSize = sizeof(dip);
	dip.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dip.diph.dwObj = 0;
	dip.diph.dwHow = DIPH_DEVICE;

	HRESULT hr = pDevice->GetProperty(DIPROP_GUIDANDPATH, &dip.diph);
	if (SUCCEEDED(hr)) {
		hidPath = dip.wszPath;
	}

	return hr;
}
HRESULT DirectInput::InputDeviceHeader::QueryVidPid() {
	if (pDevice == nullptr)
		return E_FAIL;

	DIPROPDWORD dip;
	dip.diph.dwSize = sizeof(dip);
	dip.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dip.diph.dwObj = 0;
	dip.diph.dwHow = DIPH_DEVICE;

	HRESULT hr = pDevice->GetProperty(DIPROP_VIDPID, &dip.diph);
	if (SUCCEEDED(hr)) {
		vendorID = LOWORD(dip.dwData);
		productID = HIWORD(dip.dwData);
	}

	return hr;
}

bool DirectInput::_InitializeKeyBoard() {
	Logger::WriteTop("DirectInput: Initializing keyboard device.");

	InputDeviceHeader* idh = &deviceKeyboard_.idh;
	LPDIRECTINPUTDEVICE8& device = idh->pDevice;

	HRESULT hr;
	const std::string method = "_InitializeKeyBoard";

	_WrapDXErr(hr = pDirectInput_->CreateDevice(GUID_SysKeyboard, &device, nullptr),
		method, "Failed to create device.");

	_WrapDXErr(hr = idh->QueryDeviceInstance(),
		method, "Failed to get device info.");

	_WrapDXErr(hr = idh->QueryHidPath(),
		method, "Failed to get device HID path.", false);
	_WrapDXErr(hr = idh->QueryVidPid(),
		method, "Failed to get device vendor/product IDs.", false);

	_WrapDXErr(hr = device->SetDataFormat(&c_dfDIKeyboard),
		method, "Failed to set data format.");
	_WrapDXErr(hr = device->SetCooperativeLevel(hWnd_, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND),
		method, "Failed to set cooperative level.");

	_WrapDXErr(hr = device->Acquire(),
		method, "Failed to acquire device.");

	memset(&deviceKeyboard_.state, 0, sizeof(deviceKeyboard_.state));
	Logger::WriteTop("DirectInput: Keyboard device initialized.");

	return true;
}
bool DirectInput::_InitializeMouse() {
	Logger::WriteTop("DirectInput: Initializing mouse device.");

	InputDeviceHeader* idh = &deviceMouse_.idh;
	LPDIRECTINPUTDEVICE8& device = idh->pDevice;

	HRESULT hr;
	const std::string method = "_InitializeMouse";

	_WrapDXErr(hr = pDirectInput_->CreateDevice(GUID_SysMouse, &device, nullptr),
		method, "Failed to create device.");

	_WrapDXErr(hr = idh->QueryDeviceInstance(),
		method, "Failed to get device info.");

	_WrapDXErr(hr = idh->QueryHidPath(),
		method, "Failed to get device HID path.", false);
	_WrapDXErr(hr = idh->QueryVidPid(),
		method, "Failed to get device vendor/product IDs.", false);

	_WrapDXErr(hr = device->SetDataFormat(&c_dfDIMouse2),
		method, "Failed to set data format.");
	_WrapDXErr(hr = device->SetCooperativeLevel(hWnd_, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND),
		method, "Failed to set cooperative level.");

	_WrapDXErr(device->Acquire(),
		method, "Failed to acquire device.");

	memset(&deviceMouse_.state, 0, sizeof(DIMOUSESTATE2));
	Logger::WriteTop("DirectInput: Mouse device initialized.");

	return true;
}
bool DirectInput::_InitializeJoypad() {
	Logger::WriteTop("DirectInput: Initializing pad device.");

	HRESULT hr = S_OK;

	hr = pDirectInput_->EnumDevices(DI8DEVCLASS_GAMECTRL, (LPDIENUMDEVICESCALLBACK)_GetJoypadStaticCallback, this, DIEDFL_ATTACHEDONLY);
	_WrapDXErr(hr, "_InitializeJoypad", "Failed to connect to a pad device.");

	size_t count = listDeviceJoypad_.size();
	if (count == 0U)
		Logger::WriteTop("DirectInput:_InitializeJoypad: No pad device detected.");

	Logger::WriteTop("DirectInput: Pad device initialized.");
	return true;
}
BOOL CALLBACK DirectInput::_GetJoypadStaticCallback(LPDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
	DirectInput* input = (DirectInput*)pvRef;
	return input->_GetJoypadCallback(lpddi);
}
BOOL DirectInput::_GetJoypadCallback(LPDIDEVICEINSTANCE lpddi) {
	Logger::WriteTop(StringUtility::Format(
		"DirectInput: Attempting to connect to pad device %016x...", (uint64_t)lpddi));

	JoypadInputDevice joypad;
	InputDeviceHeader* idh = &joypad.idh;
	LPDIRECTINPUTDEVICE8& device = idh->pDevice;

	try {
		HRESULT hr = S_OK;
		const std::string method = "_InitializeJoypad";

		_WrapDXErr((lpddi->dwDevType & DIDEVTYPE_HID) == 0 ? ERROR_NOT_SUPPORTED: 0, 
			method, "Device not supported (not a Human Interface Device)");

		_WrapDXErr(hr = pDirectInput_->CreateDevice(lpddi->guidInstance, &device, nullptr),
			method, "Failed to create device.");

		memcpy(&idh->ddi, lpddi, sizeof(DIDEVICEINSTANCE));

		_WrapDXErr(hr = idh->QueryHidPath(), 
			method, "Failed to get device HID path.", false);
		_WrapDXErr(hr = idh->QueryVidPid(),
			method, "Failed to get device vendor/product IDs.", false);

		_WrapDXErr(hr = device->SetDataFormat(&c_dfDIJoystick),
			method, "Failed to set data format.");
		_WrapDXErr(hr = device->SetCooperativeLevel(hWnd_, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND),
			method, "Failed to set cooperative level.");

		{
			{
				DIPROPRANGE diprg;
				diprg.diph.dwSize = sizeof(DIPROPRANGE);
				diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
				diprg.diph.dwObj = 0;
				diprg.diph.dwHow = DIPH_DEVICE;
				diprg.lMin = -1000;
				diprg.lMax = +1000;

				_WrapDXErr(hr = device->SetProperty(DIPROP_RANGE, &diprg.diph),
					method, "Failed to set pad axis range.");
			}

			{
				DIPROPDWORD dipdw;
				dipdw.diph.dwSize = sizeof(DIPROPDWORD);
				dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
				dipdw.diph.dwObj = 0;
				dipdw.diph.dwHow = DIPH_DEVICE;
				dipdw.dwData = 2000;	//20% deadzone

				_WrapDXErr(hr = device->SetProperty(DIPROP_DEADZONE, &dipdw.diph),
					method, "Failed to set pad axis deadzone.");
			}

			_WrapDXErr(hr = device->Acquire(),
				method, "Failed to acquire device.");

			Logger::WriteTop(StringUtility::Format(
				"DirectInput: Pad device connected: \n"
				"    Product name  = %s\n"
				"    Instance name = %s",
				idh->ddi.tszProductName, idh->ddi.tszInstanceName));
		}

		memset(&joypad.state, 0, sizeof(DIJOYSTATE));
		joypad.responseThreshold = 500;

		listDeviceJoypad_.push_back(joypad);
	}
	catch (wexception& err) {
		ptr_release(device);
		Logger::WriteTop("DirectInput::_GetJoypadCallback: Connection attempt failed.");
	}

#undef CHECKERR

	return DIENUM_CONTINUE;
}

DIKeyState DirectInput::_GetKeyboardKey(UINT code, DIKeyState state) {
	return _GetStateSub((deviceKeyboard_.state[code] & 0x80) == 0x80, state);
}
DIKeyState DirectInput::_GetMouseButton(int16_t button, DIKeyState state) {
	return _GetStateSub((deviceMouse_.state.rgbButtons[button] & 0x80) == 0x80, state);
}

DIKeyState DirectInput::_GetPadButton(int16_t index, int16_t buttonNo, DIKeyState state) {
	return _GetStateSub((listDeviceJoypad_[index].state.rgbButtons[buttonNo] & 0x80) == 0x80, state);
}
DIKeyState DirectInput::_GetPadAnalogDirection(int16_t index, uint16_t direction, DIKeyState state) {
	if (index < listDeviceJoypad_.size()) {
		LONG response = listDeviceJoypad_[index].responseThreshold;
		DIJOYSTATE* pState = &listDeviceJoypad_[index].state;

		switch (direction) {
		case DIK_UP:
			return _GetStateSub(pState->lY < -response, state);
		case DIK_DOWN:
			return _GetStateSub(pState->lY > response, state);
		case DIK_LEFT:
			return _GetStateSub(pState->lX < -response, state);
		case DIK_RIGHT:
			return _GetStateSub(pState->lX > response, state);
		}
	}
	return KEY_FREE;
}
DIKeyState DirectInput::_GetPadPovHatDirection(int16_t index, int iHat, uint16_t direction, DIKeyState state) {
	// TODO: Determine the amount of POV hats available in a pad
	if (index < listDeviceJoypad_.size() && iHat < 4) {
		DIJOYSTATE* pState = &listDeviceJoypad_[index].state;
		DWORD pov = pState->rgdwPOV[iHat];
		if (pov == UINT32_MAX)
			return _GetStateSub(false, state);

		pov /= 100;

		constexpr const int MARGIN = 15;
		switch (direction) {
		case DIK_UP:
			return _GetStateSub(pov < 90  - MARGIN || pov > 270 + MARGIN, state);
		case DIK_DOWN:
			return _GetStateSub(pov > 90  + MARGIN && pov < 270 - MARGIN, state);
		case DIK_LEFT:
			return _GetStateSub(pov > 180 + MARGIN && pov < 360 - MARGIN, state);
		case DIK_RIGHT:
			return _GetStateSub(pov > 0   + MARGIN && pov < 180 - MARGIN, state);
		}
	}
	return KEY_FREE;
}

DIKeyState DirectInput::_GetStateSub(bool flag, DIKeyState state) {
	DIKeyState res = KEY_FREE;
	if (flag) {
		res = state == KEY_FREE ? KEY_PUSH : KEY_HOLD;
	}
	else {
		res = (state == KEY_PUSH || state == KEY_HOLD) ? KEY_PULL : KEY_FREE;
	}
	return res;
}

bool DirectInput::_IdleKeyboard() {
	LPDIRECTINPUTDEVICE8 pDevice = deviceKeyboard_.idh.pDevice;
	if (!pDirectInput_ || !pDevice)
		return false;

	HRESULT hr = pDevice->GetDeviceState(sizeof(BYTE) * MAX_KEY, deviceKeyboard_.state);
	if (hr == DIERR_INPUTLOST)
		pDevice->Acquire();
	
	return true;
}
bool DirectInput::_IdleMouse() {
	LPDIRECTINPUTDEVICE8 pDevice = deviceMouse_.idh.pDevice;
	if (!pDirectInput_ || !pDevice)
		return false;

	HRESULT hr = pDevice->GetDeviceState(sizeof(DIMOUSESTATE2), &deviceMouse_.state);
	if (hr == DIERR_INPUTLOST)
		pDevice->Acquire();

	return true;
}
bool DirectInput::_IdleJoypad() {
	for (int16_t iPad = 0; iPad < listDeviceJoypad_.size(); ++iPad) {
		JoypadInputDevice* pJoypad = &listDeviceJoypad_[iPad];
		LPDIRECTINPUTDEVICE8 pDevice = pJoypad->idh.pDevice;
		if (!pDirectInput_ || pDevice == nullptr)
			return false;

		pDevice->Poll();

		HRESULT hr = pDevice->GetDeviceState(sizeof(DIJOYSTATE), &pJoypad->state);
		if (hr == DIERR_INPUTLOST)
			pDevice->Acquire();
	}
	return true;
}

void DirectInput::Update() {
	this->_IdleKeyboard();
	this->_IdleMouse();
	this->_IdleJoypad();

	for (int16_t iKey = 0; iKey < MAX_KEY; ++iKey)
		bufKey_[iKey] = _GetKeyboardKey(iKey, bufKey_[iKey]);

	for (int16_t iButton = 0; iButton < 3; ++iButton)
		bufMouse_[iButton] = _GetMouseButton(iButton, bufMouse_[iButton]);

	for (int16_t iPad = 0; iPad < listDeviceJoypad_.size(); ++iPad) {
		auto& pad = bufPad_[iPad];

		// 0-3 -> (Typically) Left analog stick
		pad[0] = _GetPadAnalogDirection(iPad, DIK_LEFT, pad[0]);
		pad[1] = _GetPadAnalogDirection(iPad, DIK_RIGHT, pad[1]);
		pad[2] = _GetPadAnalogDirection(iPad, DIK_UP, pad[2]);
		pad[3] = _GetPadAnalogDirection(iPad, DIK_DOWN, pad[3]);

		// 4-7 -> D-Pad
		pad[4] = _GetPadPovHatDirection(iPad, 0, DIK_LEFT, pad[4]);
		pad[5] = _GetPadPovHatDirection(iPad, 0, DIK_RIGHT, pad[5]);
		pad[6] = _GetPadPovHatDirection(iPad, 0, DIK_UP, pad[6]);
		pad[7] = _GetPadPovHatDirection(iPad, 0, DIK_DOWN, pad[7]);

		for (int16_t iButton = 0; iButton < MAX_PAD_BUTTON; ++iButton)
			pad[iButton + 8] = _GetPadButton(iPad, iButton, pad[iButton + 8]);
	}
}

DIKeyState DirectInput::GetKeyState(int16_t key) {
	if (key < 0 || key >= MAX_KEY)
		return KEY_FREE;
	return bufKey_[key];
}
DIKeyState DirectInput::GetMouseState(int16_t button) {
	if (button < 0 || button >= MAX_MOUSE_BUTTON)
		return KEY_FREE;
	return bufMouse_[button];
}
DIKeyState DirectInput::GetPadState(int16_t padNo, int16_t button) {
	DIKeyState res = KEY_FREE;
	if (padNo < bufPad_.size())
		res = bufPad_[padNo][button];
	return res;
}

#if defined(DNH_PROJ_EXECUTOR)
POINT DirectInput::GetMousePosition() {
	POINT res = { 0, 0 };
	GetCursorPos(&res);
	ScreenToClient(hWnd_, &res);
	return res;
}
#endif

void DirectInput::ResetInputState() {
	ResetKeyState();
	ResetMouseState();
	ResetPadState();
}
void DirectInput::ResetKeyState() {
	for (int16_t iKey = 0; iKey < MAX_KEY; ++iKey)
		bufKey_[iKey] = KEY_FREE;
	ZeroMemory(&deviceKeyboard_.state, sizeof(deviceKeyboard_.state));
}
void DirectInput::ResetMouseState() {
	for (int16_t iButton = 0; iButton < 3; ++iButton)
		bufMouse_[iButton] = KEY_FREE;
	ZeroMemory(&deviceMouse_.state, sizeof(deviceMouse_.state));
}
void DirectInput::ResetPadState(int16_t padIndex) {
	auto _ResetPad = [&](int16_t iPad) {
		auto& pad = bufPad_[iPad];
		auto& state = listDeviceJoypad_[iPad].state;

		for (int16_t iKey = 0; iKey < bufPad_.size(); ++iKey)
			pad[iKey] = KEY_FREE;
		state.lX = 0;
		state.lY = 0;
		for (int16_t iButton = 0; iButton < MAX_PAD_BUTTON; ++iButton)
			state.rgbButtons[iButton] = KEY_FREE;
	};

	if (padIndex < 0) {
		for (int16_t iPad = 0; iPad < listDeviceJoypad_.size(); ++iPad) {
			_ResetPad(iPad);
		}
	}
	else {
		_ResetPad(padIndex);
	}
}

const DirectInput::JoypadInputDevice* DirectInput::GetPadDevice(int16_t padIndex) {
	if (padIndex > listDeviceJoypad_.size())
		return nullptr;
	return &listDeviceJoypad_[padIndex];
}

//*******************************************************************
//VirtualKey
//*******************************************************************
VirtualKey::VirtualKey(int16_t keyboard, int16_t padIndex, int16_t padButton) {
	keyboard_ = keyboard;
	padIndex_ = padIndex;
	padButton_ = padButton;
	state_ = KEY_FREE;
}
VirtualKey::~VirtualKey() {}

//*******************************************************************
//VirtualKeyManager
//*******************************************************************
VirtualKeyManager::VirtualKeyManager() {

}
VirtualKeyManager::~VirtualKeyManager() {

}

void VirtualKeyManager::Update() {
	DirectInput::Update();
	for (auto itr = mapKey_.begin(); itr != mapKey_.end(); ++itr) {
		DIKeyState state = _GetVirtualKeyState(itr->first);
		itr->second->SetKeyState(state);
	}
}
void VirtualKeyManager::ClearKeyState() {
	DirectInput::ResetInputState();
	for (auto itr = mapKey_.begin(); itr != mapKey_.end(); ++itr) {
		itr->second->SetKeyState(KEY_FREE);
	}
}
DIKeyState VirtualKeyManager::_GetVirtualKeyState(int16_t id) {
	auto itrFind = mapKey_.find(id);
	if (itrFind == mapKey_.end()) return KEY_FREE;

	ref_count_ptr<VirtualKey> key = itrFind->second;

	DIKeyState res = KEY_FREE;
	if (key->keyboard_ >= 0 && key->keyboard_ < MAX_KEY)
		res = bufKey_[key->keyboard_];
	if (res == KEY_FREE) {
		int16_t indexPad = key->padIndex_;
		if (indexPad >= 0 && indexPad < listDeviceJoypad_.size()) {
			if (key->padButton_ >= 0 && key->padButton_ < bufPad_[indexPad].size())
				res = bufPad_[indexPad][key->padButton_];
		}
	}

	return res;
}

DIKeyState VirtualKeyManager::GetVirtualKeyState(int16_t id) {
	ref_count_ptr<VirtualKey> key = GetVirtualKey(id);
	return key ? key->GetKeyState() : KEY_FREE;
}

ref_count_ptr<VirtualKey> VirtualKeyManager::GetVirtualKey(int16_t id) {
	auto itrFind = mapKey_.find(id);
	if (itrFind == mapKey_.end()) return nullptr;
	return itrFind->second;
}
bool VirtualKeyManager::IsTargetKeyCode(int16_t key) {
	bool res = false;
	for (auto itr = mapKey_.begin(); itr != mapKey_.end(); ++itr) {
		ref_count_ptr<VirtualKey> vKey = itr->second;
		if (key == vKey->GetKeyCode()) {
			res = true;
			break;
		}
	}
	return res;
}

#if defined(DNH_PROJ_EXECUTOR)
//*******************************************************************
//KeyReplayManager
//*******************************************************************
KeyReplayManager::KeyReplayManager(VirtualKeyManager* input) {
	frame_ = 0;
	input_ = input;
	state_ = STATE_RECORD;
}
KeyReplayManager::~KeyReplayManager() {}
void KeyReplayManager::AddTarget(int16_t key) {
	mapKeyTarget_[key] = KEY_FREE;
}
void KeyReplayManager::Update() {
	if (state_ == STATE_RECORD) {
		for (auto itrTarget = mapKeyTarget_.begin(); itrTarget != mapKeyTarget_.end(); ++itrTarget) {
			int16_t idKey = itrTarget->first;
			DIKeyState keyState = input_->GetVirtualKeyState(idKey);
			
			if (frame_ == 0 || itrTarget->second != keyState) {
				ReplayData data;
				data.id_ = idKey;
				data.frame_ = frame_;
				data.state_ = keyState;
				listReplayData_.push_back(data);
			}

			itrTarget->second = keyState;
		}
	}
	else if (state_ == STATE_REPLAY) {
		if (frame_ == 0) replayDataIterator_ = listReplayData_.begin();
		for (auto itrTarget = mapKeyTarget_.begin(); itrTarget != mapKeyTarget_.end(); ++itrTarget) {
			int16_t idKey = itrTarget->first;
			DIKeyState& stateKey = itrTarget->second;

			for (auto itrData = replayDataIterator_; itrData != listReplayData_.end(); ++itrData) {
				ReplayData& data = *itrData;
				if (data.frame_ > frame_) break;

				if (idKey == data.id_ && data.frame_ == frame_) {
					stateKey = data.state_;
					++replayDataIterator_;
				}
			}

			ref_count_ptr<VirtualKey> key = input_->GetVirtualKey(idKey);
			key->SetKeyState(stateKey);
		}

		if (frame_ % 600 == 0)
			listReplayData_.erase(listReplayData_.begin(), replayDataIterator_);
	}
	++frame_;
}
bool KeyReplayManager::IsTargetKeyCode(int16_t key) {
	bool res = false;
	for (auto itrTarget = mapKeyTarget_.begin(); itrTarget != mapKeyTarget_.end(); ++itrTarget) {
		ref_count_ptr<VirtualKey> vKey = input_->GetVirtualKey(itrTarget->first);
		if (key == vKey->GetKeyCode()) {
			res = true;
			break;
		}
	}
	return res;
}
void KeyReplayManager::ReadRecord(RecordBuffer& record) {
	size_t countReplayData = record.GetRecordAs<uint32_t>("count");

	ByteBuffer buffer;
	buffer.SetSize(sizeof(ReplayData) * countReplayData);
	record.GetRecord("data", buffer.GetPointer(), buffer.GetSize());
	for (size_t iRec = 0; iRec < countReplayData; ++iRec) {
		ReplayData data;
		buffer.Read(&data, sizeof(ReplayData));
		listReplayData_.push_back(data);
	}
}
void KeyReplayManager::WriteRecord(RecordBuffer& record) {
	record.SetRecord<uint32_t>("count", listReplayData_.size());

	ByteBuffer buffer;
	for (auto itrData = listReplayData_.begin(); itrData != listReplayData_.end(); ++itrData) {
		ReplayData& data = *itrData;
		buffer.Write(&data, sizeof(ReplayData));
	}
	record.SetRecord("data", buffer.GetPointer(), buffer.GetSize());
}

#endif
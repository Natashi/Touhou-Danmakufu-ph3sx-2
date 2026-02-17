#include "source/GcLib/pch.h"

#include "GcLibImpl.hpp"
#include "MainWindow.hpp"

//*******************************************************************
//EApplication
//*******************************************************************
EApplication::EApplication() {
}
EApplication::~EApplication() {
}

bool EApplication::_Initialize() {
	MainWindow* wndMain = MainWindow::GetInstance();
	HWND hWndMain = wndMain->GetWindowHandle();

	EDirectInput* input = EDirectInput::CreateInstance();
	input->Initialize(hWndMain);

	wndMain->InitializePanels();

	return true;
}

bool EApplication::_Loop() {
	MainWindow* wndMain = MainWindow::GetInstance();
	return wndMain->Loop();
}
bool EApplication::Run() {
	while (true) {
		if (!_Loop())
			break;
	}

	bAppRun_ = false;
	return true;
}

bool EApplication::_Finalize() {
	EDirectInput::DeleteInstance();
	return true;
}

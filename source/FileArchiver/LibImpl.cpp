#include "source/GcLib/pch.h"

#include "LibImpl.hpp"

EApplication::EApplication() {
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
#include "source/GcLib/pch.h"

#include "LibImpl.hpp"
#include "MainWindow.hpp"

//*******************************************************************
//WinMain
//*******************************************************************
int APIENTRY wWinMain(HINSTANCE hInstance,
                        HINSTANCE hPrevInstance,
                        LPWSTR lpCmdLine,
                        int nCmdShow )
{
	DebugUtility::DumpMemoryLeaksOnExit();
	try {
		MainWindow* wndMain = MainWindow::CreateInstance();
		wndMain->Initialize();
		wndMain->SetWindowVisible(true);

		EApplication* app = EApplication::CreateInstance();
		app->Initialize();

		if (app->IsRun()) {
			bool bInit = app->_Initialize();
			if (bInit) app->Run();
			app->_Finalize();
		}
	}
	catch (std::exception& e) {
		std::wstring error = StringUtility::ConvertMultiToWide(e.what());
		Logger::WriteTop(error);
	}
	catch (gstd::wexception& e) {
		Logger::WriteTop(e.what());
	}

	EApplication::DeleteInstance();
	MainWindow::DeleteInstance();

	return 0;
}

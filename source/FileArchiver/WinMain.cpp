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
		{
			HRESULT hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED |
				COINIT_DISABLE_OLE1DDE);
			if (FAILED(hr))
				throw wexception("CoInitializeEx failed");
		}

		MainWindow* wndMain = MainWindow::CreateInstance();
		wndMain->Initialize();
		wndMain->SetWindowVisible(true);

		EApplication* app = EApplication::CreateInstance();
		app->Initialize();

		if (app->IsRun()) {
			bool bInit = app->_Initialize();
			if (bInit)
				app->Run();
			app->_Finalize();
		}
	}
	catch (std::exception& e) {
		::MessageBoxW(nullptr, StringUtility::ConvertMultiToWide(e.what()).c_str(),
			L"Unexpected Error", MB_ICONERROR | MB_APPLMODAL | MB_OK);
	}
	catch (gstd::wexception& e) {
		::MessageBoxW(nullptr, e.what(),
			L"Engine Error", MB_ICONERROR | MB_APPLMODAL | MB_OK);
	}

	EApplication::DeleteInstance();
	MainWindow::DeleteInstance();
	::CoUninitialize();

	return 0;
}

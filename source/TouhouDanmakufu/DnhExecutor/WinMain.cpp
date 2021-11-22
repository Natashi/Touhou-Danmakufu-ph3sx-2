#include "source/GcLib/pch.h"

#include "GcLibImpl.hpp"

//*******************************************************************
//WinMain
//*******************************************************************
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
	HWND handleWindow = nullptr;

	try {
		std::locale::global(std::locale("")); // Is this allowed?
		
		gstd::SystemUtility::TestCpuSupportSIMD();

		DnhConfiguration* config = DnhConfiguration::CreateInstance();
		ELogger* logger = ELogger::CreateInstance();
		logger->Initialize(config->IsLogFile(), config->IsLogWindow());
		EPathProperty::CreateInstance();

		EApplication* app = EApplication::CreateInstance();

		app->Initialize();

		if (app->IsRun()) {
			bool bInit = app->_Initialize();
			if (!bInit)
				throw gstd::wexception("Initialization failure.");
			handleWindow = app->GetPtrGraphics()->GetAttachedWindowHandle();

			app->Run();

			bool bFinalize = app->_Finalize();
			if (!bFinalize)
				throw gstd::wexception("Finalization failure.");
		}
	}
	catch (std::exception& e) {
		MessageBox(handleWindow, StringUtility::ConvertMultiToWide(e.what()).c_str(),
			L"Unexpected Error", MB_ICONERROR | MB_APPLMODAL | MB_OK);
	}
	catch (gstd::wexception& e) {
		MessageBox(handleWindow, e.what(), 
			L"Engine Error", MB_ICONERROR | MB_APPLMODAL | MB_OK);
	}

	EApplication::DeleteInstance();
	EPathProperty::DeleteInstance();
	ELogger::DeleteInstance();
	DnhConfiguration::DeleteInstance();

	gstd::DebugUtility::DumpMemoryLeaksOnExit();

	return 0;
}

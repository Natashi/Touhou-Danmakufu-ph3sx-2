#include "source/GcLib/pch.h"

#include "GcLibImpl.hpp"

//*******************************************************************
//WinMain
//*******************************************************************
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
	HWND handleWindow = nullptr;

	try {
		gstd::SystemUtility::InitializeCOM();
		gstd::SystemUtility::TestCpuSupportSIMD();

		directx::EDirect3D9::CreateInstance();
		DnhConfiguration* config = DnhConfiguration::CreateInstance();

		ELogger* logger = ELogger::CreateInstance();
		logger->Initialize(config->bLogFile_, config->bLogWindow_);

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
	directx::EDirect3D9::DeleteInstance();

	gstd::SystemUtility::UninitializeCOM();
	gstd::DebugUtility::DumpMemoryLeaksOnExit();

	return 0;
}

#include "source/GcLib/pch.h"

#include "Application.hpp"

#if defined(DNH_PROJ_EXECUTOR)
#include "Logger.hpp"
#endif

using namespace gstd;

/**********************************************************
//Application
**********************************************************/
Application* Application::thisBase_ = NULL;
Application::Application() {
	::InitCommonControls();
}
Application::~Application() {
	thisBase_ = NULL;
}
bool Application::Initialize() {
	if (thisBase_ != NULL)return false;
	thisBase_ = this;
	hAppInstance_ = ::GetModuleHandle(NULL);
	bAppRun_ = true;
	bAppActive_ = true;
	//return _Initialize();
	return true;
}
bool Application::Run() {
	MSG msg;
	while (true) {
		if (bAppRun_ == false)break;
		if (::PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
			if (!::GetMessage(&msg, NULL, 0, 0))break;
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		else {
			if (bAppActive_ == false) {
				Sleep(10);
				continue;
			}

			try {
				if (!_Loop()) break;
			}
			catch (std::exception& e) {
#if defined(DNH_PROJ_EXECUTOR)
				Logger::WriteTop(e.what());
				Logger::WriteTop("Runtime failure.");
#endif
				throw e;
				break;
			}
			catch (gstd::wexception& e) {
#if defined(DNH_PROJ_EXECUTOR)
				Logger::WriteTop(e.what());
				Logger::WriteTop("Runtime failure.");
#endif
				throw e;
				break;
			}
			//			catch(...)
			//			{
			//				Logger::WriteTop(L"実行中に例外が発生しました。終了します。");
			//				break;
			//			}
		}
	}

	bAppRun_ = false;
	return true;
}


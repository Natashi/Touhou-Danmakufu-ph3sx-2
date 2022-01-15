#pragma once

#include "../pch.h"

#include "GstdUtility.hpp"

namespace gstd {
	//****************************************************************************
	//Application
	//****************************************************************************
	class Application {
	private:
		static Application* thisBase_;
	protected:
		bool bAppRun_;
		bool bAppActive_;
		bool bFocused_;
		HINSTANCE hAppInstance_;
		
		Application();
	public:
		virtual ~Application();

		static Application* GetBase() { return thisBase_; }
		bool Initialize();

		virtual bool _Initialize() { return true; }
		virtual bool _Loop() { return true; }
		virtual bool _Finalize() { return true; }

		virtual bool Run();
		bool IsActive() { return this->bAppActive_; }
		void SetActive(bool b) { this->bAppActive_ = b; }
		bool IsRun() { return bAppRun_; }
		void End() { bAppRun_ = false; }

		bool IsFocused() { return bFocused_; }

		static HINSTANCE GetApplicationHandle() { return ::GetModuleHandle(NULL); }
	};
}
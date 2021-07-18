#pragma once

#include "../pch.h"

namespace gstd {
	//****************************************************************************
	//Thread
	//****************************************************************************
	class Thread {
	public:
		enum Status {
			RUN,
			STOP,
			REQUEST_STOP,
		};
	private:
		static DWORD __stdcall _StaticRun(LPVOID data);
	protected:
		volatile HANDLE hThread_;
		volatile DWORD idThread_;
		volatile Status status_;

		virtual void _Run() = 0;
	public:
		Thread();
		virtual ~Thread();

		virtual void Start();
		virtual void Stop();
		bool IsStop();
		DWORD Join(DWORD mills = INFINITE);

		Status GetStatus() { return status_; }
	};

	//****************************************************************************
	//CriticalSection
	//****************************************************************************
	class CriticalSection {
		CRITICAL_SECTION cs_;
		volatile DWORD idThread_;
		volatile int countLock_;
	public:
		CriticalSection();
		~CriticalSection();

		void Enter();
		void Leave();
	};

	//****************************************************************************
	//Lock
	//	Mutex locking based on a critical section object
	//****************************************************************************
	class Lock {
	protected:
		CriticalSection* cs_;
	public:
		Lock(CriticalSection& cs) { cs_ = &cs; cs_->Enter(); }
		Lock(CriticalSection* cs) { cs_ = cs; cs_->Enter(); }
		virtual ~Lock() { cs_->Leave(); }
	};

	//****************************************************************************
	//StaticLock
	//	Basic mutex locking
	//****************************************************************************
	class StaticLock {
	protected:
		static std::recursive_mutex* mtx_;
	public:
		StaticLock();
		~StaticLock();
	};

	//****************************************************************************
	//ThreadSignal
	//	Wrapper for thread event signaling
	//****************************************************************************
	class ThreadSignal {
		HANDLE hEvent_;
	public:
		ThreadSignal(bool bManualReset = false);
		virtual ~ThreadSignal();

		DWORD Wait(int mills = INFINITE);
		void SetSignal(bool bOn = true);
	};
}
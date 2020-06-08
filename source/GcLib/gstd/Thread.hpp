#ifndef __GSTD_THREAD__
#define __GSTD_THREAD__

#include "../pch.h"

namespace gstd {
	//================================================================
	//Thread
	class Thread {
	public:
		enum Status {
			RUN,//‰Ò“­’†
			STOP,//’âŽ~’†
			REQUEST_STOP,//’âŽ~—v‹’† 
		};
	private:
		static unsigned int __stdcall _StaticRun(LPVOID data);
	protected:
		volatile HANDLE hThread_;
		unsigned int idThread_;
		volatile Status status_;
		virtual void _Run() = 0;

	public:
		Thread();
		virtual ~Thread();
		virtual void Start();
		virtual void Stop();
		bool IsStop();
		DWORD Join(int mills = INFINITE);

		Status GetStatus() { return status_; }
	};

	//================================================================
	//CriticalSection
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

	//================================================================
	//Lock
	class Lock {
	protected:
		CriticalSection* cs_;
	public:
		Lock(CriticalSection& cs) { cs_ = &cs; cs_->Enter(); }
		virtual ~Lock() { cs_->Leave(); }
	};

	//================================================================
	//ThreadSignal
	class ThreadSignal {
		HANDLE hEvent_;
	public:
		ThreadSignal(bool bManualReset = false);
		virtual ~ThreadSignal();
		DWORD Wait(int mills = INFINITE);
		void SetSignal(bool bOn = true);
	};
}

#endif

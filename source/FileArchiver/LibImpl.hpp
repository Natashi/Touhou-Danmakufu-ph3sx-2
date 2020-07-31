#ifndef __LIBIMPL__
#define __LIBIMPL__

#include "../GcLib/pch.h"
#include "Constant.hpp"

class EApplication : public Singleton<EApplication>, public Application {
	friend Singleton<EApplication>;
private:
	EApplication() {
	}
protected:
	virtual bool _Loop() {
		Sleep(10);
		return true;
	}
public:
	~EApplication() {
	}
};

#endif

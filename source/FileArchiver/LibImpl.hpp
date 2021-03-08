#pragma once

#include "../GcLib/pch.h"
#include "Constant.hpp"

class EApplication : public Singleton<EApplication>, public Application {
	friend Singleton<EApplication>;
protected:
	virtual bool _Loop() {
		Sleep(10);
		return true;
	}
public:
	EApplication() {}
	~EApplication() {}
};
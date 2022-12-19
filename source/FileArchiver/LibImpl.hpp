#pragma once

#include "../GcLib/pch.h"
#include "Constant.hpp"

#include "MainWindow.hpp"

class EApplication : public Singleton<EApplication>, public Application {
	friend Singleton<EApplication>;
public:
	EApplication();
	~EApplication() {}

	virtual bool _Loop();
	virtual bool Run();
};
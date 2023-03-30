#pragma once
#include "../../GcLib/pch.h"

#include "Constant.hpp"
#include "../Common/DnhGcLibImpl.hpp"

//*******************************************************************
//EApplication
//*******************************************************************
class EApplication : public Singleton<EApplication>, public Application {
	friend Singleton<EApplication>;
public:
	EApplication();
	~EApplication();

	bool _Initialize();

	bool _Loop();
	virtual bool Run();

	bool _Finalize();
};
#pragma once

#include "../../GcLib/pch.h"

#include "../Common/StgSystem.hpp"

//*******************************************************************
//EStgSystemController
//*******************************************************************
class EStgSystemController : public StgSystemController {
protected:
	virtual void DoEnd();
	virtual void DoRetry();
};

//*******************************************************************
//PStgSystemController
//*******************************************************************
class PStgSystemController : public StgSystemController {
protected:
	virtual void DoEnd();
	virtual void DoRetry();
};
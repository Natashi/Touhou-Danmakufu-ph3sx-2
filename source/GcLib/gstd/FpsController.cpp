#include "source/GcLib/pch.h"

#include "FpsController.hpp"

using namespace gstd;

//*******************************************************************
//FpsController
//*******************************************************************
FpsController::FpsController() {
	fps_ = 60;
	bUseTimer_ = true;
	::timeBeginPeriod(1);
	bCriticalFrame_ = true;
	bFastMode_ = false;
	fastModeFpsRate_ = 1200;
}
FpsController::~FpsController() {
	::timeEndPeriod(1);
}
DWORD FpsController::GetCpuTime() {
	//	int res = ::timeGetTime();

	LARGE_INTEGER nFreq, nCounter;
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&nCounter);
	return (DWORD)(nCounter.QuadPart * 1000UL / nFreq.QuadPart);

	//auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch();
	//return std::chrono::duration_cast<std::chrono::milliseconds>(ms).count();
}
void FpsController::_Sleep(DWORD msec) {
	::Sleep(msec);
	/*
		int time = _GetTime();
		while(abs(_GetTime() - time) < msec)
			::Sleep(1);
	*/
}
void FpsController::RemoveFpsControlObject(ref_count_weak_ptr<FpsControlObject> obj) {
	if (obj.expired()) return;
	for (auto itr = listFpsControlObject_.begin(); itr != listFpsControlObject_.end(); itr++) {
		ref_count_weak_ptr<FpsControlObject>& tObj = *itr;
		if (obj.get() == tObj.get()) {
			listFpsControlObject_.erase(itr);
			break;
		}
	}
}
DWORD FpsController::GetControlObjectFps() {
	DWORD res = fps_;
	for (auto itr = listFpsControlObject_.begin(); itr != listFpsControlObject_.end(); ) {
		ref_count_weak_ptr<FpsControlObject>& obj = *itr;
		if (obj.IsExists()) {
			res = std::min(res, obj->GetFps());
			++itr;
		}
		else
			itr = listFpsControlObject_.erase(itr);
	}
	return res;
}

//*******************************************************************
//StaticFpsController
//*******************************************************************
StaticFpsController::StaticFpsController() {
	rateSkip_ = 0;
	fpsCurrent_ = 60;
	timePrevious_ = GetCpuTime();
	timeCurrentFpsUpdate_ = 0;
	bUseTimer_ = true;
	timeError_ = 0;
}
StaticFpsController::~StaticFpsController() {

}
void StaticFpsController::Wait() {
	DWORD fpsTarget = bFastMode_ ? fastModeFpsRate_ : std::min(fps_, GetControlObjectFps());

	DWORD timeCurrent = GetCpuTime();
	DWORD timeDelta = timeCurrent - timePrevious_;

	int frameAs1Sec = timeDelta * fpsTarget - timeError_;
	int time1Sec = CLOCKS_PER_SEC;
	DWORD sleepTime = 0;
	//timeError_ = 0;

	if (frameAs1Sec < time1Sec) {
		int secDelta = time1Sec - frameAs1Sec;
		sleepTime = std::max(secDelta / (int)fpsTarget, 0);

		if (bUseTimer_ || rateSkip_ != 0) {
			_Sleep(sleepTime);
			timeError_ = std::max<int>(secDelta % fpsTarget, 0);
		}
	}

	if (timeDelta > 0)
		listFps_.push_back(timeDelta + floor(sleepTime));

	if (timeCurrent - timeCurrentFpsUpdate_ >= 500) {
		if (listFps_.size() > 0) {
			DWORD tFpsCurrent = 0;
			for (DWORD iFps : listFps_)
				tFpsCurrent += iFps;

			fpsCurrent_ = 1000 / (tFpsCurrent / (float)listFps_.size());
			listFps_.clear();
		}
		else fpsCurrent_ = 0;

		timeCurrentFpsUpdate_ = timePrevious_;
	}
	++countSkip_;

	size_t rateSkip = rateSkip_;
	if (bFastMode_) rateSkip = fastModeFpsRate_ / 60U;
	countSkip_ %= (rateSkip + 1);
	bCriticalFrame_ = false;

	timePrevious_ = GetCpuTime();
}
bool StaticFpsController::IsSkip() {
	size_t rateSkip = rateSkip_;
	if (bFastMode_) rateSkip = fastModeFpsRate_ / 60U;
	if (rateSkip == 0 || bCriticalFrame_) return false;
	if (countSkip_ % (rateSkip + 1) != 0)
		return true;
	return false;
}

//*******************************************************************
//AutoSkipFpsController
//*******************************************************************
AutoSkipFpsController::AutoSkipFpsController() {
	timeError_ = 0;
	timePrevious_ = GetCpuTime();
	timePreviousWork_ = timePrevious_;
	timePreviousRender_ = timePrevious_;
	countSkip_ = 0;
	countSkipMax_ = 20;

	fpsCurrentWork_ = 0;
	fpsCurrentRender_ = 0;
	timeCurrentFpsUpdate_ = 0;
	timeError_ = 0;
}
AutoSkipFpsController::~AutoSkipFpsController() {

}
void AutoSkipFpsController::Wait() {
	DWORD fpsTarget = bFastMode_ ? fastModeFpsRate_ : std::min(fps_, GetControlObjectFps());

	DWORD timeCurrent = GetCpuTime();
	DWORD timeDelta = timeCurrent - timePrevious_;

	int frameAs1Sec = timeDelta * fpsTarget;
	int time1Sec = CLOCKS_PER_SEC + timeError_;
	DWORD sleepTime = 0;
	timeError_ = 0;

	if (frameAs1Sec < time1Sec || bCriticalFrame_) {
		sleepTime = (countSkip_ >= 1) ? 0 : std::max((time1Sec - frameAs1Sec) / (int)fpsTarget, 0);

		if (bUseTimer_)
			_Sleep(sleepTime);
		timeError_ = (time1Sec - frameAs1Sec) % (int)fpsTarget;
		//if(timeError_< 0 )timeError_ = 0;
	}
	else if (countSkip_ <= 0) {
		countSkip_ += std::min((timeDelta * fpsTarget / 1000U) + 1, countSkipMax_);
	}

	--countSkip_;
	bCriticalFrame_ = false;

	{
		DWORD timeCorrect = sleepTime;
		if (timeDelta > 0)
			listFpsWork_.push_back(timeDelta + timeCorrect);
		timePrevious_ = GetCpuTime();
	}
	if (countSkip_ <= 0) {
		timeCurrent = GetCpuTime();
		timeDelta = timeCurrent - timePrevious_;
		if (timeDelta > 0)
			listFpsRender_.push_back(timeDelta);
		timePreviousRender_ = GetCpuTime();
	}

	timePrevious_ = GetCpuTime();
	if (timePrevious_ - timeCurrentFpsUpdate_ >= 500) {
		if (listFpsWork_.size() != 0) {
			DWORD tFpsCurrent = 0;
			for (DWORD iFps : listFpsWork_)
				tFpsCurrent += iFps;

			fpsCurrentWork_ = 1000.0f / (tFpsCurrent / (float)listFpsWork_.size());
			listFpsWork_.clear();
		}
		else fpsCurrentWork_ = 0;

		if (listFpsRender_.size() != 0) {
			DWORD tFpsCurrent = 0;
			for (DWORD iFps : listFpsRender_)
				tFpsCurrent += iFps;

			fpsCurrentRender_ = 1000.0f / (tFpsCurrent / (float)listFpsRender_.size());
			listFpsRender_.clear();
		}
		else fpsCurrentRender_ = 0;

		timeCurrentFpsUpdate_ = GetCpuTime();
	}
}

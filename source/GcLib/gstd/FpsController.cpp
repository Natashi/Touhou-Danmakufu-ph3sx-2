#include "source/GcLib/pch.h"

#include "FpsController.hpp"

using namespace gstd;

/**********************************************************
//FpsController
**********************************************************/
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
int FpsController::_GetTime() {
	//	int res = ::timeGetTime();

	LARGE_INTEGER nFreq;
	LARGE_INTEGER nCounter;
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&nCounter);
	return (DWORD)(nCounter.QuadPart * 1000 / nFreq.QuadPart);

	//auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch();
	//return std::chrono::duration_cast<std::chrono::milliseconds>(ms).count();
}
void FpsController::_Sleep(int msec) {
	::Sleep(msec);
	/*
		int time = _GetTime();
		while(abs(_GetTime() - time) < msec)
			::Sleep(1);
	*/
}
void FpsController::RemoveFpsControlObject(ref_count_weak_ptr<FpsControlObject> obj) {
	std::list<ref_count_weak_ptr<FpsControlObject>>::iterator itr = listFpsControlObject_.begin();;
	for (; itr != listFpsControlObject_.end(); itr++) {
		ref_count_weak_ptr<FpsControlObject> tObj = (*itr);
		if (obj.GetPointer() == tObj.GetPointer()) {
			listFpsControlObject_.erase(itr);
			break;
		}
	}
}
int FpsController::GetControlObjectFps() {
	int res = fps_;
	std::list<ref_count_weak_ptr<FpsControlObject>>::iterator itr = listFpsControlObject_.begin();;
	for (; itr != listFpsControlObject_.end(); ) {
		ref_count_weak_ptr<FpsControlObject> obj = (*itr);
		if (obj.IsExists()) {
			int fps = obj->GetFps();
			res = std::min(res, fps);
			++itr;
		}
		else
			itr = listFpsControlObject_.erase(itr);
	}
	return res;
}

/**********************************************************
//StaticFpsController
**********************************************************/
StaticFpsController::StaticFpsController() {
	rateSkip_ = 0;
	fpsCurrent_ = 60;
	timePrevious_ = _GetTime();
	timeCurrentFpsUpdate_ = 0;
	bUseTimer_ = true;
	timeError_ = 0;
}
StaticFpsController::~StaticFpsController() {

}
void StaticFpsController::Wait() {
	int time = _GetTime();

	double tFps = fps_;
	tFps = std::min(tFps, (double)GetControlObjectFps());
	if (bFastMode_)tFps = fastModeFpsRate_;

	int sTime = time - timePrevious_;//前フレームとの時間差

	int frameAs1Sec = sTime * tFps;
	int time1Sec = 1000 + timeError_;
	int sleepTime = 0;
	timeError_ = 0;
	if (frameAs1Sec < time1Sec) {
		sleepTime = (time1Sec - frameAs1Sec) / tFps; //待機時間
		if (sleepTime < 0)sleepTime = 0;
		if (bUseTimer_ || rateSkip_ != 0) {
			_Sleep(sleepTime);//一定時間たつまで、sleep
			timeError_ = (time1Sec - frameAs1Sec) % (int)tFps;
		}

		if (timeError_ < 0)timeError_ = 0;
	}

	//1frameにかかった時間を保存
	double timeCorrect = (double)sleepTime;
	if (time - timePrevious_ > 0)
		listFps_.push_back(time - timePrevious_ + ceil(timeCorrect));
	timePrevious_ = _GetTime();

	if (time - timeCurrentFpsUpdate_ >= 500) {//一秒ごとに表示フレーム数を更新
		if (listFps_.size() != 0) {
			double tFpsCurrent = 0;
			std::list<int>::iterator itr;
			for (itr = listFps_.begin(); itr != listFps_.end(); itr++) {
				tFpsCurrent += (*itr);
			}
			fpsCurrent_ = (double)(1000.0) / ((double)tFpsCurrent / (double)listFps_.size());
			listFps_.clear();
		}
		else fpsCurrent_ = 0;

		timeCurrentFpsUpdate_ = _GetTime();
	}
	++countSkip_;

	int rateSkip = rateSkip_;
	if (bFastMode_)rateSkip = fastModeFpsRate_ / 60U;
	countSkip_ %= (rateSkip + 1);
	bCriticalFrame_ = false;
}
bool StaticFpsController::IsSkip() {
	int rateSkip = rateSkip_;
	if (bFastMode_)rateSkip = fastModeFpsRate_ / 60U;
	if (rateSkip == 0 || bCriticalFrame_)return false;
	if (countSkip_ % (rateSkip + 1) != 0)
		return true;
	return false;
}

/**********************************************************
//AutoSkipFpsController
**********************************************************/
AutoSkipFpsController::AutoSkipFpsController() {
	timeError_ = 0;
	timePrevious_ = _GetTime();
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
	int time = _GetTime();

	double tFps = fps_;
	tFps = std::min(tFps, (double)GetControlObjectFps());
	if (bFastMode_)tFps = fastModeFpsRate_;

	int sTime = time - timePrevious_;//前フレームとの時間差
	int frameAs1Sec = sTime * tFps;
	int time1Sec = 1000 + timeError_;
	int sleepTime = 0;
	timeError_ = 0;
	if (frameAs1Sec < time1Sec || bCriticalFrame_) {
		sleepTime = (time1Sec - frameAs1Sec) / tFps; //待機時間
		if (sleepTime < 0 || countSkip_ - 1 >= 0)sleepTime = 0;
		if (bUseTimer_)
			_Sleep(sleepTime);//一定時間たつまで、sleep

		timeError_ = (time1Sec - frameAs1Sec) % (int)tFps;
		//if(timeError_< 0 )timeError_ = 0;
	}
	else if (countSkip_ <= 0) {
		countSkip_ += (double)sTime * (double)tFps / 1000 + 1;
		if (countSkip_ > countSkipMax_)
			countSkip_ = countSkipMax_;
	}

	--countSkip_;
	bCriticalFrame_ = false;

	{
		//1Workにかかった時間を保存
		double timeCorrect = (double)sleepTime;
		if (time - timePrevious_ > 0)
			listFpsWork_.push_back(time - timePrevious_ + ceil(timeCorrect));
		timePrevious_ = _GetTime();;
	}
	if (countSkip_ <= 0) {
		//1描画にかかった時間を保存
		time = _GetTime();
		if (time - timePreviousRender_ > 0)
			listFpsRender_.push_back(time - timePreviousRender_);
		timePreviousRender_ = _GetTime();
	}

	timePrevious_ = _GetTime();
	if (time - timeCurrentFpsUpdate_ >= 500) {//一秒ごとに表示フレーム数を更新
		if (listFpsWork_.size() != 0) {
			float tFpsCurrent = 0;
			std::list<int>::iterator itr;
			for (itr = listFpsWork_.begin(); itr != listFpsWork_.end(); itr++) {
				tFpsCurrent += (*itr);
			}
			fpsCurrentWork_ = (float)(1000.0f) / ((float)tFpsCurrent / (float)listFpsWork_.size());
			listFpsWork_.clear();
		}
		else fpsCurrentWork_ = 0;

		if (listFpsRender_.size() != 0) {
			float tFpsCurrent = 0;
			std::list<int>::iterator itr;
			for (itr = listFpsRender_.begin(); itr != listFpsRender_.end(); itr++) {
				tFpsCurrent += (*itr);
			}
			fpsCurrentRender_ = (float)(1000.0f) / ((float)tFpsCurrent / (float)listFpsRender_.size());
			listFpsRender_.clear();
		}
		else fpsCurrentRender_ = 0;

		timeCurrentFpsUpdate_ = _GetTime();
	}
}

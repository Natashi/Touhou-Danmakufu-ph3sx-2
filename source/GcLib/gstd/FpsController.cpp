#include "source/GcLib/pch.h"

#include "FpsController.hpp"
#include "GstdUtility.hpp"

using namespace gstd;
using namespace stdch;

//*******************************************************************
//FpsController
//*******************************************************************
FpsController::FpsController() {
	fps_ = 60;

	bCriticalFrame_ = true;

	bFastMode_ = false;
	fastModeFpsRate_ = 1200;
}
FpsController::~FpsController() {
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
	fpsCurrent_ = 60;

	rateSkip_ = 0;

	timePrevious_ = SystemUtility::GetCpuTime();
	timeAccum_ = 0ns;

	timePreviousFpsUpdate_ = time_point<steady_clock, nanoseconds>{ 0ns };
}
StaticFpsController::~StaticFpsController() {
}
void StaticFpsController::SetCriticalFrame() {
	bCriticalFrame_ = true;
	timeAccum_ = 0ns;
	countSkip_ = 0;
}
std::array<bool, 2> StaticFpsController::Advance() {
	std::array<bool, 2> res{ false, false };

	DWORD fpsTarget = std::min<DWORD>(bFastMode_ ? fastModeFpsRate_ : std::min(fps_, GetControlObjectFps()), 1000);
	const auto targetNs = duration<double, std::nano>(std::nano::den / (double)fpsTarget);

	auto timeCurrent = SystemUtility::GetCpuTime();
	auto timeDelta = timeCurrent - timePrevious_;
	timePrevious_ = timeCurrent;

	timeAccum_ += timeDelta;
	if (timeAccum_ >= targetNs) {
		if (bCriticalFrame_ || (rateSkip_ <= 1 || countSkip_ % rateSkip_ == 0)) {
			listFps_.push_back(timeAccum_.count());
			res[0] = true;
		}
		res[1] = true;

		++countSkip_;

		const nanoseconds dCast = duration_cast<nanoseconds>(targetNs);
		timeAccum_ -= dCast;
		if (timeAccum_ > targetNs)
			timeAccum_ = dCast;
	}

	if (timeCurrent - timePreviousFpsUpdate_ >= 500ms) {
		if (listFps_.size() > 0) {
			double fpsAccum = 0;		//ms
			for (double iFps : listFps_)
				fpsAccum += iFps / 1e6;
			fpsAccum /= listFps_.size();

			fpsCurrent_ = 1000 / fpsAccum;

			listFps_.clear();
		}
		else fpsCurrent_ = 0;

		timePreviousFpsUpdate_ = timePrevious_;
	}

	bCriticalFrame_ = false;
	return res;
}

//*******************************************************************
//VariableFpsController
//*******************************************************************
static constexpr size_t MAX_SKIP = 5;	// Can only skip 5 frames consecutively
VariableFpsController::VariableFpsController() {
	fpsCurrentUpdate_ = 0;
	fpsCurrentRender_ = 0;
	bFrameRendered_ = false;

	countSkip_ = 0;

	timePrevious_ = SystemUtility::GetCpuTime();
	timePreviousUpdate_ = timePrevious_;
	timePreviousRender_ = timePrevious_;

	timeAccumUpdate_ = 0ns;
	timeAccumRender_ = 0ns;

	timePreviousFpsUpdate_ = time_point<steady_clock, nanoseconds>{ 0ns };
}
VariableFpsController::~VariableFpsController() {
}
void VariableFpsController::SetCriticalFrame() {
	bCriticalFrame_ = true;
	countSkip_ = 0;
	timeAccumUpdate_ = 0ns;
	timeAccumRender_ = 0ns;
}
std::array<bool, 2> VariableFpsController::Advance() {
	std::array<bool, 2> res{ false, false };

	DWORD fpsTarget = std::min<DWORD>(bFastMode_ ? fastModeFpsRate_ : std::min(fps_, GetControlObjectFps()), 1000);
	const auto targetNs = duration<double, std::nano>(std::nano::den / (double)fpsTarget);

	auto timeCurrent = SystemUtility::GetCpuTime();
	auto timeDelta = timeCurrent - timePrevious_;
	timePrevious_ = timeCurrent;

	timeAccumUpdate_ += timeDelta;
	timeAccumRender_ += timeDelta;

	if (timeAccumUpdate_ >= targetNs) {
		listFpsUpdate_.push_back(timeAccumUpdate_.count());
		res[1] = true;

		const nanoseconds dCast = duration_cast<nanoseconds>(targetNs);
		timeAccumUpdate_ -= dCast;
		if (timeAccumUpdate_ > targetNs)
			timeAccumUpdate_ = dCast;

		timeAccumRender_ += timeAccumUpdate_;
		bFrameRendered_ = false;
	}

	if (bCriticalFrame_ || (!bFrameRendered_ && (countSkip_ >= MAX_SKIP || timeAccumRender_ < targetNs))) {
		auto timeDeltaRender = timeCurrent - timePreviousRender_;

		listFpsRender_.push_back(timeDeltaRender.count());
		res[0] = true;

		timePreviousRender_ = timeCurrent;
		bFrameRendered_ = true;
		countSkip_ = 0;
	}
	else if (!bFrameRendered_) {
		++countSkip_;
	}
	timeAccumRender_ = 0ns;

	if (timeCurrent - timePreviousFpsUpdate_ >= 500ms) {
		if (listFpsUpdate_.size() > 0) {
			double fpsAccum = 0;		//ms
			for (double iFps : listFpsUpdate_)
				fpsAccum += iFps / 1e6;
			fpsAccum /= listFpsUpdate_.size();

			fpsCurrentUpdate_ = 1000 / fpsAccum;
		}
		else fpsCurrentUpdate_ = 0;

		/*
		if (listFpsRender_.size() > 0) {
			double fpsAccum = 0;		//ms
			for (double iFps : listFpsRender_)
				fpsAccum += iFps / 1e6;
			fpsAccum /= listFpsRender_.size();

			fpsCurrentRender_ = std::max(1000 / fpsAccum - 2, 0.0);
		}
		else fpsCurrentRender_ = 0;
		*/
		fpsCurrentRender_ = listFpsUpdate_.size();	//TODO: Figure out how to calculate this

		listFpsUpdate_.clear();
		listFpsRender_.clear();
		timePreviousFpsUpdate_ = timePrevious_;
	}

	bCriticalFrame_ = false;
	return res;
	/*
	DWORD fpsTarget = bFastMode_ ? fastModeFpsRate_ : std::min(fps_, GetControlObjectFps());
	const double targetMs = 1000.0 / fpsTarget;
	const auto targetMsCh = duration<double, std::milli>(targetMs);

	auto timeCurrent = GetCpuTime();
	auto timeDelta = timeCurrent - timePrevious_;

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
	*/
}

#include "source/GcLib/pch.h"

#include "FpsController.hpp"
#include "GstdUtility.hpp"

using namespace gstd;
using namespace stdch;

//*******************************************************************
//FpsController
//*******************************************************************
FpsController::FpsController() : 
	fps_(60), fastModeFps_(1000),
	bCriticalFrame_(true), bFastMode_(false) { }

void FpsController::RemoveFpsControlObject(FpsControlObject* obj) {
	listFpsControlObject_.remove_if([&](const auto& x) { return x.get() == obj; });
}
uint32_t FpsController::GetControlObjectFps() {
	uint32_t res = fps_;
	for (auto& pControl : listFpsControlObject_) {
		res = std::min(res, pControl->GetFps());
	}
	return res;
}

constexpr duration<double, std::nano> FpsToNs(uint32_t fps) {
	return duration<double, std::nano>(std::nano::den / (double)fps);
}

duration<double, std::nano> FpsController::GetTargetFrameDurationNs() {
	auto targetFps = std::min<uint32_t>(
		bFastMode_ ? fastModeFps_ :
		std::min(fps_, GetControlObjectFps()),
		MAX_FPS);
	return FpsToNs(targetFps);
}

double FpsController::TimeList::CalculateFps() const {
	if (listTime.size() > 0) {
		// in seconds
		double fpsAccum = std::reduce(listTime.begin(), listTime.end(),
			0.0, [](double s, double x) { return s + x / 1e9; });
		fpsAccum /= listTime.size();

		return 1 / fpsAccum;
	}
	return 0;
}

//*******************************************************************
//StaticFpsController
//*******************************************************************

constexpr auto FPS_CALC_INTERVAL = 500ms;

StaticFpsController::StaticFpsController() : 
	rateSkip_(0), countSkip_(0),
	timePreviousFpsUpdate_(0ns),
	timePreviousUpdate_(SystemUtility::GetCpuTime())
{
	StaticFpsController::SetCriticalFrame();
	SetSkipRate(0);
}

void StaticFpsController::SetCriticalFrame() {
	bCriticalFrame_ = true;
	countSkip_ = 0;

	tc_.accum = 0ns;
}

void StaticFpsController::Advance() {
	// get current target frame timing
	const auto targetNs = GetTargetFrameDurationNs();
	const auto targetNsDuration = duration_cast<nanoseconds>(targetNs);
	
	const auto timeCurrent = SystemUtility::GetCpuTime();
	const auto timeDelta = timeCurrent - tc_.prev;
	tc_.prev = timeCurrent;

	tc_.accum += timeDelta;

	if (tc_.accum >= targetNs) {
		// do update frame
		{
			auto now = SystemUtility::GetCpuTime();
			auto delta = now - timePreviousUpdate_;

			tlUpdate_.AddTime(delta);
			timePreviousUpdate_ = now;
			
			callbackFrameUpdate_();
		}

		// do render frame (unless skipping)
		if (bCriticalFrame_ || countSkip_ >= rateSkip_) {
			callbackFrameRender_();

			bCriticalFrame_ = false;
			countSkip_ = 0;
		}
		++countSkip_;

		tc_.accum = std::min(tc_.accum - targetNsDuration, targetNsDuration);
	}

	// periodically update FPS stats
	if (timeCurrent - timePreviousFpsUpdate_ >= FPS_CALC_INTERVAL) {
		tlUpdate_.fps = tlUpdate_.CalculateFps();
		tlUpdate_.listTime.clear();

		// same as update fps
		tlRender_.fps = tlUpdate_.fps;

		timePreviousFpsUpdate_ = timeCurrent;
	}
}

//*******************************************************************
//VariableFpsController
//*******************************************************************
VariableFpsController::VariableFpsController() :
timePreviousFpsUpdate_(0ns),
	timePreviousUpdate_(SystemUtility::GetCpuTime()),
	timePreviousRender_(SystemUtility::GetCpuTime())
{
	VariableFpsController::SetCriticalFrame();
}

void VariableFpsController::SetCriticalFrame() {
	bCriticalFrame_ = true;

	tc_.accum = 0ns;
}

void VariableFpsController::Advance() {
	// get current target frame timing
	const auto targetNs = GetTargetFrameDurationNs();
	const auto targetNsDuration = duration_cast<nanoseconds>(targetNs);

	// can only frameskip for 0.1s consecutively
	constexpr auto MAX_SKIP = 0.1s;
	
	auto timeCurrent = SystemUtility::GetCpuTime();
	auto timeDelta = timeCurrent - tc_.prev;
	tc_.prev = timeCurrent;

	tc_.accum += timeDelta;

	while (tc_.accum >= targetNs) {
		// prevent spiral of death
		if (SystemUtility::GetCpuTime() - timePreviousRender_ >= MAX_SKIP) {
			tc_.accum = std::min(targetNsDuration, tc_.accum);
			break;
		}
		
		// do update frame
		{
			auto now = SystemUtility::GetCpuTime();
			auto delta = now - timePreviousUpdate_;

			tlUpdate_.AddTime(delta);
			timePreviousUpdate_ = now;
			
			callbackFrameUpdate_();
		}

		tc_.accum -= targetNsDuration;
	}

	// do render frame
	{
		auto now = SystemUtility::GetCpuTime();
		auto delta = now - timePreviousRender_;

		tlRender_.AddTime(delta);
		timePreviousRender_ = now;
        
		callbackFrameRender_();
	}

	if (timeCurrent - timePreviousFpsUpdate_ >= FPS_CALC_INTERVAL) {
		tlUpdate_.fps = tlUpdate_.CalculateFps();
		tlUpdate_.listTime.clear();

		tlRender_.fps = tlRender_.CalculateFps();
		tlRender_.listTime.clear();

		timePreviousFpsUpdate_ = timeCurrent;
	}
}

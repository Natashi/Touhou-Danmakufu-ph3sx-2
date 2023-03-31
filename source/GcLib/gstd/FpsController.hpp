#pragma once

#include "../pch.h"

#include "SmartPointer.hpp"

namespace gstd {
	class FpsControlObject;
	//*******************************************************************
	//FpsController
	//*******************************************************************
	class FpsController {
	protected:
		DWORD fps_;

		bool bCriticalFrame_;
		bool bFastMode_;

		size_t fastModeFpsRate_;

		std::list<ref_count_weak_ptr<FpsControlObject>> listFpsControlObject_;
	public:
		FpsController();
		virtual ~FpsController();

		virtual void SetFps(DWORD fps) { fps_ = fps; }
		virtual DWORD GetFps() { return fps_; }

		virtual std::array<bool, 2> Advance() = 0;

		virtual bool IsRenderFrame() { return false; }
		virtual bool IsUpdateFrame() { return false; }

		virtual void SetCriticalFrame() { bCriticalFrame_ = true; }

		virtual float GetCurrentFps() = 0;
		virtual float GetCurrentWorkFps() { return GetCurrentFps(); }
		virtual float GetCurrentRenderFps() { return GetCurrentFps(); }

		bool IsFastMode() { return bFastMode_; }
		void SetFastMode(bool b) { bFastMode_ = b; }

		void SetFastModeRate(size_t fpsRate) { fastModeFpsRate_ = fpsRate; }

		void AddFpsControlObject(ref_count_weak_ptr<FpsControlObject> obj) {
			listFpsControlObject_.push_back(obj);
		}
		void RemoveFpsControlObject(ref_count_weak_ptr<FpsControlObject> obj);
		DWORD GetControlObjectFps();
	};

	//*******************************************************************
	//StaticFpsController
	//*******************************************************************
	class StaticFpsController : public FpsController {
	protected:
		float fpsCurrent_;

		size_t rateSkip_;
		size_t countSkip_;

		stdch::steady_clock::time_point timePrevious_;
		stdch::steady_clock::time_point timePreviousRender_;
		stdch::nanoseconds timeAccum_;

		stdch::steady_clock::time_point timePreviousFpsUpdate_;
		std::list<double> listFps_;
	public:
		StaticFpsController();
		virtual ~StaticFpsController();

		virtual std::array<bool, 2> Advance();

		virtual void SetCriticalFrame();

		void SetSkipRate(size_t value) {
			rateSkip_ = value;
			countSkip_ = 0;
		}
		virtual float GetCurrentFps() { return fpsCurrent_; }
		virtual float GetCurrentWorkFps() { return fpsCurrent_; }
		virtual float GetCurrentRenderFps() { return GetCurrentFps(); }
	};

	//*******************************************************************
	//VariableFpsController
	//*******************************************************************
	class VariableFpsController : public FpsController {
	protected:
		float fpsCurrentUpdate_;
		float fpsCurrentRender_;
		bool bFrameRendered_;

		size_t countSkip_;

		stdch::steady_clock::time_point timePrevious_;
		stdch::steady_clock::time_point timePreviousUpdate_;
		stdch::steady_clock::time_point timePreviousRender_;

		stdch::nanoseconds timeAccumUpdate_;
		stdch::nanoseconds timeAccumRender_;

		stdch::steady_clock::time_point timePreviousFpsUpdate_;
		std::list<double> listFpsUpdate_;
		std::list<double> listFpsRender_;
	public:
		VariableFpsController();
		virtual ~VariableFpsController();

		virtual std::array<bool, 2> Advance();

		virtual void SetCriticalFrame();

		virtual float GetCurrentFps() { return GetCurrentWorkFps(); }
		float GetCurrentWorkFps() { return fpsCurrentUpdate_; };
		float GetCurrentRenderFps() { return fpsCurrentRender_; };
	};

	//*******************************************************************
	//FpsControlObject
	//*******************************************************************
	class FpsControlObject {
	public:
		FpsControlObject() {}
		virtual ~FpsControlObject() {}

		virtual DWORD GetFps() = 0;
	};
}
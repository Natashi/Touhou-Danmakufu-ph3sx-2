#pragma once

#include "../pch.h"

#include "GstdUtility.hpp"

namespace gstd {
	class FpsControlObject;
	
	//*******************************************************************
	//FpsController
	//*******************************************************************
	class FpsController {
	protected:
		static constexpr uint32_t MAX_FPS = 3000;
	protected:
		struct TimeCounter {
			stdch::steady_clock::time_point prev;
			stdch::nanoseconds accum;
		};
		struct TimeList {
			std::vector<double> listTime;
			double fps;

			void AddTime(stdch::nanoseconds ns) { listTime.push_back(ns.count()); }
			double CalculateFps() const;

			TimeList() : fps(0) {}
		};
	protected:
		uint32_t fps_;
		uint32_t fastModeFps_;

		bool bCriticalFrame_;
		bool bFastMode_;

		std::function<void()> callbackFrameUpdate_;
		std::function<void()> callbackFrameRender_;

		std::list<unique_ptr<FpsControlObject>> listFpsControlObject_;
	public:
		FpsController();
		virtual ~FpsController() = default;

		void SetFps(uint32_t fps) { fps_ = fps; }
		uint32_t GetFps() { return fps_; }

		virtual void SetCriticalFrame() { bCriticalFrame_ = true; }

		virtual void Advance() = 0;

		virtual float GetCurrentUpdateFps() = 0;
		virtual float GetCurrentRenderFps() = 0;

		bool IsFastMode() { return bFastMode_; }
		void SetFastMode(bool b) { bFastMode_ = b; }

		void SetFastModeRate(size_t fpsRate) { fastModeFps_ = fpsRate; }

		void AddFpsControlObject(unique_ptr<FpsControlObject> obj) {
			listFpsControlObject_.push_back(MOVE(obj));
		}
		void RemoveFpsControlObject(FpsControlObject* obj);
		uint32_t GetControlObjectFps();

		stdch::duration<double, std::nano> GetTargetFrameDurationNs();

		void SetUpdateCallback(const std::function<void()>& fn) { callbackFrameUpdate_ = fn; }
		void SetRenderCallback(const std::function<void()>& fn) { callbackFrameRender_ = fn; }
	};

	//*******************************************************************
	//StaticFpsController
	//*******************************************************************
	class StaticFpsController : public FpsController, NonCopyable, NonMovable {
		size_t rateSkip_;
		size_t countSkip_;

		stdch::steady_clock::time_point timePreviousFpsUpdate_;
		
		stdch::steady_clock::time_point timePreviousUpdate_;

		TimeCounter tc_;
		TimeList tlUpdate_, tlRender_;
	public:
		StaticFpsController();

		void SetSkipRate(size_t value) {
			rateSkip_ = value;
			countSkip_ = 0;
		}

		void SetCriticalFrame() override;

		void Advance() override;

		float GetCurrentUpdateFps() override { return tlUpdate_.fps; }
		float GetCurrentRenderFps() override { return tlRender_.fps; }
	};

	//*******************************************************************
	//VariableFpsController
	//*******************************************************************
	class VariableFpsController : public FpsController, NonCopyable, NonMovable {
	protected:
		stdch::steady_clock::time_point timePreviousFpsUpdate_;

		stdch::steady_clock::time_point timePreviousUpdate_;
		stdch::steady_clock::time_point timePreviousRender_;

		TimeCounter tc_;
		TimeList tlUpdate_, tlRender_;
	public:
		VariableFpsController();

		void SetCriticalFrame() override;

		void Advance() override;

		float GetCurrentUpdateFps() override { return tlUpdate_.fps; }
		float GetCurrentRenderFps() override { return tlRender_.fps; }
	};

	//*******************************************************************
	//FpsControlObject
	//*******************************************************************
	class FpsControlObject {
	public:
		virtual ~FpsControlObject() = default;

		virtual uint32_t GetFps() = 0;
	};
}
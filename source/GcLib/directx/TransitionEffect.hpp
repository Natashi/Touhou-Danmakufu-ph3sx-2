#pragma once

#include "../pch.h"

#include "RenderObject.hpp"

namespace directx {
	//*******************************************************************
	//TransitionEffect
	//*******************************************************************
	class TransitionEffect {
	protected:

	public:
		TransitionEffect();
		virtual ~TransitionEffect();

		virtual void Work() = 0;
		virtual void Render() = 0;
		virtual bool IsEnd() { return true; }
	};

	//*******************************************************************
	//TransitionEffect_FadeOut
	//*******************************************************************
	class TransitionEffect_FadeOut : public TransitionEffect {
	protected:
		double diffAlpha_;
		double alpha_;
		shared_ptr<Sprite2D> sprite_;
	public:
		virtual void Work();
		virtual void Render();
		virtual bool IsEnd();
		void Initialize(int frame, shared_ptr<Texture> texture);
	};

	//*******************************************************************
	//TransitionEffectTask
	//*******************************************************************
	class TransitionEffectTask : public gstd::TaskBase {
	protected:
		shared_ptr<TransitionEffect> effect_;
	public:
		TransitionEffectTask();
		~TransitionEffectTask();

		void SetTransition(shared_ptr<TransitionEffect> effect);
		virtual void Work();
		virtual void Render();
	};
}
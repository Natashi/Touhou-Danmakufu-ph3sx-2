#ifndef __DIRECTX_TRANSITIONEFFECT__
#define __DIRECTX_TRANSITIONEFFECT__

#include "../pch.h"

#include "RenderObject.hpp"

namespace directx {
	/**********************************************************
	//TransitionEffect
	**********************************************************/
	class TransitionEffect {
	protected:

	public:
		TransitionEffect();
		virtual ~TransitionEffect();
		virtual void Work() = 0;
		virtual void Render() = 0;
		virtual bool IsEnd() { return true; }
	};

	/**********************************************************
	//TransitionEffect_FadeOut
	**********************************************************/
	class TransitionEffect_FadeOut : public TransitionEffect {
	protected:
		double diffAlpha_;
		double alpha_;
		gstd::ref_count_ptr<Sprite2D> sprite_;
	public:
		virtual void Work();
		virtual void Render();
		virtual bool IsEnd();
		void Initialize(int frame, shared_ptr<Texture> texture);

	};

	/**********************************************************
	//TransitionEffectTask
	**********************************************************/
	class TransitionEffectTask : public gstd::TaskBase {
	protected:
		gstd::ref_count_ptr<TransitionEffect> effect_;
	public:
		TransitionEffectTask();
		~TransitionEffectTask();

		void SetTransition(gstd::ref_count_ptr<TransitionEffect> effect);
		virtual void Work();
		virtual void Render();
	};
}

#endif
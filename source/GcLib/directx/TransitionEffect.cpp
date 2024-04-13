#include "source/GcLib/pch.h"

#include "TransitionEffect.hpp"

using namespace gstd;
using namespace directx;

//*******************************************************************
//TransitionEffect
//*******************************************************************
TransitionEffect::TransitionEffect() {

}
TransitionEffect::~TransitionEffect() {

}

//*******************************************************************
//TransitionEffect_FadeOut
//*******************************************************************
void TransitionEffect_FadeOut::Work() {
	if (sprite_ == nullptr) return;
	alpha_ -= diffAlpha_;
	alpha_ = std::max(alpha_, 0.0);
	sprite_->SetAlpha((int)alpha_);
}
void TransitionEffect_FadeOut::Render() {
	if (sprite_ == nullptr) return;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetBlendMode(MODE_BLEND_ALPHA);
	graphics->SetZBufferEnable(false);
	graphics->SetZWriteEnable(false);
	sprite_->Render();
}
bool TransitionEffect_FadeOut::IsEnd() {
	bool res = (alpha_ <= 0);
	return res;
}
void TransitionEffect_FadeOut::Initialize(int frame, shared_ptr<Texture> texture) {
	diffAlpha_ = 255.0 / frame;
	alpha_ = 255.0;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG width = graphics->GetScreenWidth();
	LONG height = graphics->GetScreenHeight();

	DxRect<int> rect(0, 0, width, height);

	sprite_.reset(new Sprite2D());
	sprite_->SetTexture(texture);
	sprite_->SetVertex(rect, rect, D3DCOLOR_ARGB((int)alpha_, 255, 255, 255));
}

//*******************************************************************
//TransitionEffectTask
//*******************************************************************
TransitionEffectTask::TransitionEffectTask() {
}
TransitionEffectTask::~TransitionEffectTask() {
}
void TransitionEffectTask::SetTransition(shared_ptr<TransitionEffect> effect) {
	effect_ = effect;
}
void TransitionEffectTask::Work() {
	if (effect_)
		effect_->Work();
}
void TransitionEffectTask::Render() {
	if (effect_)
		effect_->Render();
}

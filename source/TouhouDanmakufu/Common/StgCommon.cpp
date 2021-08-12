#include "source/GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgSystem.hpp"

//****************************************************************************
//StgMoveObject
//****************************************************************************
StgMoveObject::StgMoveObject(StgStageController* stageController) {
	posX_ = 0;
	posY_ = 0;
	framePattern_ = 0;
	stageController_ = stageController;

	pattern_ = nullptr;
	bEnableMovement_ = true;
}
StgMoveObject::~StgMoveObject() {
	pattern_ = nullptr;
}
void StgMoveObject::_Move() {
	if (pattern_ == nullptr || !bEnableMovement_) return;

	if (mapPattern_.size() > 0) {
		auto itr = mapPattern_.begin();
		while (framePattern_ >= itr->first) {
			for (auto& ipPattern : itr->second)
				_AttachReservedPattern(ipPattern);
			itr = mapPattern_.erase(itr);
			if (mapPattern_.size() == 0) break;
		}
	}

	pattern_->Move();
	++framePattern_;
}
void StgMoveObject::_AttachReservedPattern(ref_unsync_ptr<StgMovePattern> pattern) {
	if (pattern_ == nullptr)
		pattern_ = new StgMovePattern_Angle(this);

	pattern->_Activate(pattern_.get());
	pattern_ = pattern;
}
void StgMoveObject::AddPattern(int frameDelay, ref_unsync_ptr<StgMovePattern> pattern, bool bForceMap) {
	if (frameDelay == 0 && !bForceMap)
		_AttachReservedPattern(pattern);
	else {
		int frame = frameDelay + framePattern_;
		mapPattern_[frame].push_back(pattern);
	}
}
double StgMoveObject::GetSpeed() {
	if (pattern_ == nullptr) return 0;
	double res = pattern_->GetSpeed();
	return res;
}
void StgMoveObject::SetSpeed(double speed) {
	if (pattern_ == nullptr || pattern_->GetType() != StgMovePattern::TYPE_ANGLE) {
		pattern_ = new StgMovePattern_Angle(this);
	}
	StgMovePattern_Angle* pattern = dynamic_cast<StgMovePattern_Angle*>(pattern_.get());
	pattern->SetSpeed(speed);
}
double StgMoveObject::GetDirectionAngle() {
	if (pattern_ == nullptr) return 0;
	double res = pattern_->GetDirectionAngle();
	return res;
}
void StgMoveObject::SetDirectionAngle(double angle) {
	if (pattern_ == nullptr || pattern_->GetType() != StgMovePattern::TYPE_ANGLE) {
		pattern_ = new StgMovePattern_Angle(this);
	}
	StgMovePattern_Angle* pattern = dynamic_cast<StgMovePattern_Angle*>(pattern_.get());
	pattern->SetDirectionAngle(angle);
}
void StgMoveObject::SetSpeedX(double speedX) {
	if (pattern_ == nullptr || pattern_->GetType() != StgMovePattern::TYPE_XY) {
		pattern_ = new StgMovePattern_XY(this);
	}
	StgMovePattern_XY* pattern = dynamic_cast<StgMovePattern_XY*>(pattern_.get());
	pattern->SetSpeedX(speedX);
}
void StgMoveObject::SetSpeedY(double speedY) {
	if (pattern_ == nullptr || pattern_->GetType() != StgMovePattern::TYPE_XY) {
		pattern_ = new StgMovePattern_XY(this);
	}
	StgMovePattern_XY* pattern = dynamic_cast<StgMovePattern_XY*>(pattern_.get());
	pattern->SetSpeedY(speedY);
}

//****************************************************************************
//StgMovePattern
//****************************************************************************
StgMovePattern::StgMovePattern(StgMoveObject* target) {
	target_ = target;
	idShotData_ = NO_CHANGE;
	frameWork_ = 0;
	typeMove_ = TYPE_OTHER;
	c_ = 1;
	s_ = 0;
}
ref_unsync_ptr<StgMoveObject> StgMovePattern::_GetMoveObject(int id) {
	ref_unsync_ptr<DxScriptObjectBase> base = _GetStageController()->GetMainRenderObject(id);
	if (base == nullptr || base->IsDeleted()) return nullptr;

	return ref_unsync_ptr<StgMoveObject>::Cast(base);
}

//****************************************************************************
//StgMovePattern_Angle
//****************************************************************************
StgMovePattern_Angle::StgMovePattern_Angle(StgMoveObject* target) : StgMovePattern(target) {
	typeMove_ = TYPE_ANGLE;
	speed_ = 0;
	angDirection_ = 0;
	acceleration_ = 0;
	maxSpeed_ = 0;
	angularVelocity_ = 0;
	objRelative_ = ref_unsync_weak_ptr<StgMoveObject>();
}
void StgMovePattern_Angle::Move() {
	double angle = angDirection_;

	if (acceleration_ != 0) {
		speed_ += acceleration_;
		if (acceleration_ > 0)
			speed_ = std::min(speed_, maxSpeed_);
		if (acceleration_ < 0)
			speed_ = std::max(speed_, maxSpeed_);
	}
	if (angularVelocity_ != 0) {
		SetDirectionAngle(angle + angularVelocity_);
	}

	target_->SetPositionX(fma(speed_, c_, target_->GetPositionX()));
	target_->SetPositionY(fma(speed_, s_, target_->GetPositionY()));

	++frameWork_;
}
void StgMovePattern_Angle::_Activate(StgMovePattern* _src) {
	double newSpeed = 0;
	double newAngle = 0;
	double newAccel = 0;
	double newAgVel = 0;
	double newMaxSp = 0;
	if (_src->GetType() == TYPE_ANGLE) {
		StgMovePattern_Angle* src = (StgMovePattern_Angle*)_src;
		newSpeed = src->speed_;
		newAngle = src->angDirection_;
		newAccel = src->acceleration_;
		newAgVel = src->angularVelocity_;
		newMaxSp = src->maxSpeed_;
	}
	else if (_src->GetType() == TYPE_XY) {
		StgMovePattern_XY* src = (StgMovePattern_XY*)_src;
		newSpeed = src->GetSpeed();
		newAngle = src->GetDirectionAngle();
		newAccel = hypot(src->GetAccelerationX(), src->GetAccelerationY());
		newMaxSp = hypot(src->GetMaxSpeedX(), src->GetMaxSpeedY());
	}

	bool bMaxSpeed2 = false;
	for (auto& pairCmd : listCommand_) {
		double& arg = pairCmd.second;
		switch (pairCmd.first) {
		case SET_ZERO:
			newAccel = 0;
			newAgVel = 0;
			newMaxSp = 0;
			break;
		case SET_SPEED:
			newSpeed = arg;
			break;
		case SET_ANGLE:
			newAngle = arg;
			break;
		case SET_ACCEL:
			newAccel = arg;
			break;
		case SET_AGVEL:
			newAgVel = arg;
			break;
		case SET_SPMAX:
			newMaxSp = arg;
			break;
		case SET_SPMAX2:
			newMaxSp = arg;
			bMaxSpeed2 = true;
			break;
		case ADD_SPEED:
			newSpeed += arg;
			break;
		case ADD_ANGLE:
			newAngle += arg;
			break;
		case ADD_ACCEL:
			newAccel += arg;
			break;
		case ADD_AGVEL:
			newAgVel += arg;
			break;
		case ADD_SPMAX:
			newMaxSp += arg;
			break;
		}
	}

	if (objRelative_) {
		double dx = objRelative_->GetPositionX() - target_->GetPositionX();
		double dy = objRelative_->GetPositionY() - target_->GetPositionY();
		newAngle += atan2(dy, dx);
	}
	speed_ = newSpeed;
	//angDirection_ = newAngle;
	SetDirectionAngle(newAngle);
	acceleration_ = newAccel;
	angularVelocity_ = newAgVel;
	maxSpeed_ = newMaxSp + (bMaxSpeed2 ? speed_ : 0);
}
void StgMovePattern_Angle::SetDirectionAngle(double angle) {
	if (angle != StgMovePattern::NO_CHANGE) {
		angle = Math::NormalizeAngleRad(angle);
		c_ = cos(angle);
		s_ = sin(angle);
	}
	angDirection_ = angle;
}

//****************************************************************************
//StgMovePattern_XY
//****************************************************************************
StgMovePattern_XY::StgMovePattern_XY(StgMoveObject* target) : StgMovePattern(target) {
	typeMove_ = TYPE_XY;
	c_ = 0;
	s_ = 0;
	accelerationX_ = 0;
	accelerationY_ = 0;
	maxSpeedX_ = INT_MAX;
	maxSpeedY_ = INT_MAX;
}
void StgMovePattern_XY::Move() {
	if (accelerationX_ != 0) {
		c_ += accelerationX_;
		if (accelerationX_ > 0)
			c_ = std::min(c_, maxSpeedX_);
		if (accelerationX_ < 0)
			c_ = std::max(c_, maxSpeedX_);
	}
	if (accelerationY_ != 0) {
		s_ += accelerationY_;
		if (accelerationY_ > 0)
			s_ = std::min(s_, maxSpeedY_);
		if (accelerationY_ < 0)
			s_ = std::max(s_, maxSpeedY_);
	}

	target_->SetPositionX(target_->GetPositionX() + c_);
	target_->SetPositionY(target_->GetPositionY() + s_);

	++frameWork_;
}
void StgMovePattern_XY::_Activate(StgMovePattern* _src) {
	if (_src->GetType() == TYPE_XY) {
		StgMovePattern_XY* src = (StgMovePattern_XY*)_src;
		c_ = src->c_;
		s_ = src->s_;
		accelerationX_ = src->accelerationX_;
		accelerationY_ = src->accelerationY_;
		maxSpeedX_ = src->maxSpeedX_;
		maxSpeedY_ = src->maxSpeedY_;
	}
	else if (_src->GetType() == TYPE_ANGLE) {
		StgMovePattern_Angle* src = (StgMovePattern_Angle*)_src;
		c_ = src->GetSpeedX() / src->speed_;
		s_ = src->GetSpeedY() / src->speed_;
		{
			accelerationX_ = c_ * src->acceleration_;
			accelerationY_ = s_ * src->acceleration_;
			maxSpeedX_ = c_ * src->maxSpeed_;
			maxSpeedY_ = s_ * src->maxSpeed_;
		}
	}

	for (auto& pairCmd : listCommand_) {
		double& arg = pairCmd.second;
		switch (pairCmd.first) {
		case SET_ZERO:
			accelerationX_ = 0;
			accelerationY_ = 0;
			maxSpeedX_ = 0;
			maxSpeedY_ = 0;
			break;
		case SET_S_X:
			c_ = arg;
			break;
		case SET_S_Y:
			s_ = arg;
			break;
		case SET_A_X:
			accelerationX_ = arg;
			break;
		case SET_A_Y:
			accelerationY_ = arg;
			break;
		case SET_M_X:
			maxSpeedX_ = arg;
			break;
		case SET_M_Y:
			maxSpeedY_ = arg;
			break;
		}
	}
}

//****************************************************************************
//StgMovePattern_Line
//****************************************************************************
StgMovePattern_Line::StgMovePattern_Line(StgMoveObject* target) : StgMovePattern(target) {
	typeMove_ = TYPE_LINE;
	typeLine_ = TYPE_NONE;
	maxFrame_ = -1;
	speed_ = 0;
	angDirection_ = 0;
	memset(iniPos_, 0x00, sizeof(iniPos_));
	memset(targetPos_, 0x00, sizeof(targetPos_));
}
void StgMovePattern_Line::Move() {
	if (frameWork_ < maxFrame_) {
		target_->SetPositionX(fma(speed_, c_, target_->GetPositionX()));
		target_->SetPositionY(fma(speed_, s_, target_->GetPositionY()));
	}
	else {
		speed_ = 0;
	}

	++frameWork_;
}

StgMovePattern_Line_Speed::StgMovePattern_Line_Speed(StgMoveObject* target) : StgMovePattern_Line(target) {
	typeLine_ = TYPE_SPEED;
}
void StgMovePattern_Line_Speed::SetAtSpeed(double tx, double ty, double speed) {
	iniPos_[0] = target_->GetPositionX();
	iniPos_[1] = target_->GetPositionY();
	targetPos_[0] = tx;
	targetPos_[1] = ty;

	double dx = targetPos_[0] - iniPos_[0];
	double dy = targetPos_[1] - iniPos_[1];
	double dist = hypot(dx, dy);

	//speed_ = speed;
	angDirection_ = atan2(dy, dx);
	maxFrame_ = std::floor(dist / speed + 0.001);
	speed_ = dist / maxFrame_;	//Speed correction to reach the destination in integer frames

	c_ = dx / dist;
	s_ = dy / dist;
}

StgMovePattern_Line_Frame::StgMovePattern_Line_Frame(StgMoveObject* target) : StgMovePattern_Line(target) {
	typeLine_ = TYPE_FRAME;
	speedRate_ = 0.0;
	moveLerpFunc = Math::Lerp::Linear<double, double>;
	diffLerpFunc = Math::Lerp::DifferentialLinear<double>;
}
void StgMovePattern_Line_Frame::SetAtFrame(double tx, double ty, int frame, lerp_func lerpFunc, lerp_diff_func diffFunc) {
	iniPos_[0] = target_->GetPositionX();
	iniPos_[1] = target_->GetPositionY();
	targetPos_[0] = tx;
	targetPos_[1] = ty;

	moveLerpFunc = lerpFunc;
	diffLerpFunc = diffFunc;

	maxFrame_ = std::max(frame, 1);

	double dx = targetPos_[0] - iniPos_[0];
	double dy = targetPos_[1] - iniPos_[1];
	double dist = hypot(dx, dy);

	speedRate_ = dist / (double)frame;

	speed_ = diffLerpFunc(0.0) * speedRate_;
	angDirection_ = atan2(dy, dx);

	c_ = dx / dist;
	s_ = dy / dist;
}
void StgMovePattern_Line_Frame::Move() {
	if (frameWork_ < maxFrame_) {
		double tmp_line = (frameWork_ + 1) / (double)maxFrame_;

		speed_ = diffLerpFunc(tmp_line) * speedRate_;

		double nx = moveLerpFunc(iniPos_[0], targetPos_[0], tmp_line);
		double ny = moveLerpFunc(iniPos_[1], targetPos_[1], tmp_line);
		target_->SetPositionX(nx);
		target_->SetPositionY(ny);
	}
	else {
		speed_ = 0;
	}

	++frameWork_;
}

StgMovePattern_Line_Weight::StgMovePattern_Line_Weight(StgMoveObject* target) : StgMovePattern_Line(target) {
	typeLine_ = TYPE_WEIGHT;
	dist_ = 0;
	weight_ = 0;
	maxSpeed_ = 0;
}
void StgMovePattern_Line_Weight::SetAtWeight(double tx, double ty, double weight, double maxSpeed) {
	iniPos_[0] = target_->GetPositionX();
	iniPos_[1] = target_->GetPositionY();
	targetPos_[0] = tx;
	targetPos_[1] = ty;

	weight_ = weight;
	maxSpeed_ = maxSpeed;

	double dx = targetPos_[0] - iniPos_[0];
	double dy = targetPos_[1] - iniPos_[1];
	dist_ = hypot(dx, dy);

	speed_ = maxSpeed_;
	angDirection_ = atan2(dy, dx);

	c_ = dx / dist_;
	s_ = dy / dist_;
}
void StgMovePattern_Line_Weight::Move() {
	if (dist_ < 0.1) {
		speed_ = 0;
	}
	else {
		speed_ = dist_ / weight_;
		if (speed_ > maxSpeed_)
			speed_ = maxSpeed_;

		target_->SetPositionX(fma(speed_, c_, target_->GetPositionX()));
		target_->SetPositionY(fma(speed_, s_, target_->GetPositionY()));

		dist_ -= speed_;
	}

	++frameWork_;
}
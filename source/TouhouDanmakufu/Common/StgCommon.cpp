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
	if (!bEnableMovement_) return;
	
	if (mapPattern_.size() > 0) {
		auto itr = mapPattern_.begin();
		while (framePattern_ >= itr->first) {
			for (auto& ipPattern : itr->second)
				_AttachReservedPattern(ipPattern);
			itr = mapPattern_.erase(itr);
			if (mapPattern_.size() == 0) break;
		}
		if (pattern_ == nullptr)
			pattern_ = new StgMovePattern_Angle(this);
	}
	else if (pattern_ == nullptr) return;

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
	angularAcceleration_ = 0;
	angularMaxVelocity_ = 0;
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
	if (angularAcceleration_ != 0) {
		angularVelocity_ += angularAcceleration_;
		if (angularAcceleration_ > 0)
			angularVelocity_ = std::min(angularVelocity_, angularMaxVelocity_);
		if (angularAcceleration_ < 0)
			angularVelocity_ = std::max(angularVelocity_, angularMaxVelocity_);
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
	double newAgAcc = 0;
	double newAgMax = 0;
	if (_src->GetType() == TYPE_ANGLE) {
		StgMovePattern_Angle* src = (StgMovePattern_Angle*)_src;
		newSpeed = src->speed_;
		newAngle = src->angDirection_;
		newAccel = src->acceleration_;
		newAgVel = src->angularVelocity_;
		newMaxSp = src->maxSpeed_;
		newAgAcc = src->angularAcceleration_;
		newAgMax = src->angularMaxVelocity_;
	}
	else if (_src->GetType() == TYPE_XY) {
		StgMovePattern_XY* src = (StgMovePattern_XY*)_src;
		newSpeed = src->GetSpeed();
		newAngle = src->GetDirectionAngle();
		newAccel = hypot(src->GetAccelerationX(), src->GetAccelerationY());
		newMaxSp = hypot(src->GetMaxSpeedX(), src->GetMaxSpeedY());
	}
	else if (_src->GetType() == TYPE_XY_ANG) {
		StgMovePattern_XY_Angle* src = (StgMovePattern_XY_Angle*)_src;
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
			newAgAcc = 0;
			newAgMax = 0;
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
		case SET_AGACC:
			newAgAcc = arg;
			break;
		case SET_AGMAX:
			newAgMax = arg;
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
		case ADD_AGACC:
			newAgAcc += arg;
			break;
		case ADD_AGMAX:
			newAgMax += arg;
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
	angularAcceleration_ = newAgAcc;
	angularMaxVelocity_ = newAgMax;
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
	maxSpeedX_ = 0;
	maxSpeedY_ = 0;
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
		c_ = src->GetSpeedX();
		s_ = src->GetSpeedY();
		{
			double c = c_ / src->speed_;
			double s = s_ / src->speed_;
			accelerationX_ = c * src->acceleration_;
			accelerationY_ = s * src->acceleration_;
			maxSpeedX_ = c * src->maxSpeed_;
			maxSpeedY_ = s * src->maxSpeed_;
		}
	}
	if (_src->GetType() == TYPE_XY_ANG) {
		StgMovePattern_XY_Angle* src = (StgMovePattern_XY_Angle*)_src;
		double sc[2];
		Math::DoSinCos(src->angOff_, sc);

		double c = src->c_;
		double s = src->s_;
		double ax = src->accelerationX_;
		double ay = src->accelerationY_;
		double mx = src->maxSpeedX_;
		double my = src->maxSpeedY_;

		c_ = c * sc[1] - s * sc[0];
		s_ = c * sc[0] + s * sc[1];
		accelerationX_ = ax * sc[1] - ay * sc[0];
		accelerationY_ = ax * sc[0] + ay * sc[1];
		maxSpeedX_ = mx * sc[1] - my * sc[0];
		maxSpeedY_ = mx * sc[0] + my * sc[1];
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
//StgMovePattern_XY_Angle
//****************************************************************************
StgMovePattern_XY_Angle::StgMovePattern_XY_Angle(StgMoveObject* target) : StgMovePattern(target) {
	typeMove_ = TYPE_XY_ANG;
	c_ = 0;
	s_ = 0;
	accelerationX_ = 0;
	accelerationY_ = 0;
	maxSpeedX_ = 0;
	maxSpeedY_ = 0;
	angOff_ = 0;
	angOffVelocity_ = 0;
	angOffAcceleration_ = 0;
	angOffMaxVelocity_ = 0;
}
void StgMovePattern_XY_Angle::Move() {
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
	if (angOffVelocity_ != 0) {
		angOff_ += angOffVelocity_;
	}

	double sc[2];
	Math::DoSinCos(angOff_, sc);

	target_->SetPositionX(target_->GetPositionX() + c_ * sc[1] - s_ * sc[0]);
	target_->SetPositionY(target_->GetPositionY() + c_ * sc[0] + s_ * sc[1]);

	++frameWork_;
}
void StgMovePattern_XY_Angle::_Activate(StgMovePattern* _src) {
	if (_src->GetType() == TYPE_XY) {
		StgMovePattern_XY* src = (StgMovePattern_XY*)_src;
		c_ = src->c_;
		s_ = src->s_;
		accelerationX_ = src->accelerationX_;
		accelerationY_ = src->accelerationY_;
		maxSpeedX_ = src->maxSpeedX_;
		maxSpeedY_ = src->maxSpeedY_;
		angOff_ = 0;
		angOffVelocity_ = 0;
		angOffAcceleration_ = 0;
		angOffMaxVelocity_ = 0;
	}
	else if (_src->GetType() == TYPE_ANGLE) {
		StgMovePattern_Angle* src = (StgMovePattern_Angle*)_src;
		c_ = src->GetSpeedX();
		s_ = src->GetSpeedY();
		{
			double c = c_ / src->speed_;
			double s = s_ / src->speed_;
			accelerationX_ = c * src->acceleration_;
			accelerationY_ = s * src->acceleration_;
			maxSpeedX_ = c * src->maxSpeed_;
			maxSpeedY_ = s * src->maxSpeed_;
			angOff_ = 0;
			angOffVelocity_ = 0;
			angOffAcceleration_ = 0;
			angOffMaxVelocity_ = 0;
		}
	}
	if (_src->GetType() == TYPE_XY_ANG) {
		StgMovePattern_XY_Angle* src = (StgMovePattern_XY_Angle*)_src;
		c_ = src->c_;
		s_ = src->s_;
		accelerationX_ = src->accelerationX_;
		accelerationY_ = src->accelerationY_;
		maxSpeedX_ = src->maxSpeedX_;
		maxSpeedY_ = src->maxSpeedY_;
		angOff_ = src->angOff_;
		angOffVelocity_ = src->angOffVelocity_;
		angOffAcceleration_ = src->angOffAcceleration_;
		angOffMaxVelocity_ = src->angOffMaxVelocity_;
	}

	for (auto& pairCmd : listCommand_) {
		double& arg = pairCmd.second;
		switch (pairCmd.first) {
		case SET_ZERO:
			accelerationX_ = 0;
			accelerationY_ = 0;
			maxSpeedX_ = 0;
			maxSpeedY_ = 0;
			angOff_ = 0;
			angOffVelocity_ = 0;
			angOffAcceleration_ = 0;
			angOffMaxVelocity_ = 0;
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
		case SET_ANGLE:
			angOff_ = arg;
			break;
		case SET_AGVEL:
			angOffVelocity_ = arg;
			break;
		case SET_AGACC:
			angOffAcceleration_ = arg;
			break;
		case SET_AGMAX:
			angOffMaxVelocity_ = arg;
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
	double tPos[2];
	if (dist_ < 0.1) {
		speed_ = 0;

		tPos[0] = targetPos_[0];
		tPos[1] = targetPos_[1];
	}
	else {
		speed_ = std::min(dist_ / weight_, maxSpeed_);
		dist_ -= speed_;

		tPos[0] = fma(speed_, c_, target_->GetPositionX());
		tPos[1] = fma(speed_, s_, target_->GetPositionY());
	}

	target_->SetPositionX(tPos[0]);
	target_->SetPositionY(tPos[1]);
	++frameWork_;
}
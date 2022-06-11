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
	frameMove_ = 0;
}
StgMoveObject::~StgMoveObject() {
	pattern_ = nullptr;
}
void StgMoveObject::_Move() {
	if (!bEnableMovement_) return;
	++frameMove_;

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
	pattern->Activate(pattern_.get());
	pattern_ = pattern;
}
void StgMoveObject::AddPattern(uint32_t frameDelay, ref_unsync_ptr<StgMovePattern> pattern, bool bForceMap) {
	if (frameDelay == 0 && !bForceMap)
		_AttachReservedPattern(pattern);
	else {
		uint32_t frame = frameDelay + framePattern_;
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
	if (id == DxScript::ID_INVALID) return nullptr;

	ref_unsync_ptr<DxScriptObjectBase> base = _GetStageController()->GetMainRenderObject(id);
	if (base == nullptr || base->IsDeleted()) return nullptr;

	return ref_unsync_ptr<StgMoveObject>::Cast(base);
}
void StgMovePattern::_RegisterShotDataID() {
	if (target_ == nullptr || idShotData_ == NO_CHANGE) return;

	if (auto pObjShot = dynamic_cast<StgShotObject*>(target_)) {
		pObjShot->SetShotDataID(idShotData_);
	}
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
		if (maxSpeed_ != UNCAPPED) {
			if (acceleration_ > 0)
				speed_ = std::min(speed_, maxSpeed_);
			if (acceleration_ < 0)
				speed_ = std::max(speed_, maxSpeed_);
		}
	}
	if (angularAcceleration_ != 0) {
		angularVelocity_ += angularAcceleration_;
		if (angularMaxVelocity_ != UNCAPPED) {
			if (angularAcceleration_ > 0)
				angularVelocity_ = std::min(angularVelocity_, angularMaxVelocity_);
			if (angularAcceleration_ < 0)
				angularVelocity_ = std::max(angularVelocity_, angularMaxVelocity_);
		}
	}
	if (angularVelocity_ != 0) {
		SetDirectionAngle(angle + angularVelocity_);
	}

	target_->SetPositionX(fma(speed_, c_, target_->GetPositionX()));
	target_->SetPositionY(fma(speed_, s_, target_->GetPositionY()));

	++frameWork_;
}
void StgMovePattern_Angle::Activate(StgMovePattern* _src) {
	angularVelocity_ = 0;
	angularAcceleration_ = 0;
	angularMaxVelocity_ = 0;
	if (_src) {
		if (_src->GetType() == TYPE_ANGLE) {
			StgMovePattern_Angle* src = (StgMovePattern_Angle*)_src;
			speed_ = src->speed_;
			angDirection_ = src->angDirection_;
			acceleration_ = src->acceleration_;
			angularVelocity_ = src->angularVelocity_;
			maxSpeed_ = src->maxSpeed_;
			angularAcceleration_ = src->angularAcceleration_;
			angularMaxVelocity_ = src->angularMaxVelocity_;
		}
		else if (_src->GetType() == TYPE_XY) {
			StgMovePattern_XY* src = (StgMovePattern_XY*)_src;
			angDirection_ = src->GetDirectionAngle();

			speed_ = src->GetSpeed();

			double _ax = src->GetAccelerationX(), _ay = src->GetAccelerationY();
			double _mx = src->GetMaxSpeedX(), _my = src->GetMaxSpeedY();
			acceleration_ = hypot(_ax, _ay) * StgMovePattern_XY::GetDirectionSignRelative(angDirection_, _ax, _ay);
			maxSpeed_ = hypot(_mx, _my) * StgMovePattern_XY::GetDirectionSignRelative(angDirection_, _mx, _my);
		}
		else if (_src->GetType() == TYPE_XY_ANG) {
			StgMovePattern_XY_Angle* src = (StgMovePattern_XY_Angle*)_src;
			angDirection_ = src->GetDirectionAngle();

			speed_ = src->GetSpeed();

			double _ax = src->GetAccelerationX(), _ay = src->GetAccelerationY();
			double _mx = src->GetMaxSpeedX(), _my = src->GetMaxSpeedY();
			acceleration_ = hypot(_ax, _ay) * StgMovePattern_XY::GetDirectionSignRelative(angDirection_, _ax, _ay);
			maxSpeed_ = hypot(_mx, _my) * StgMovePattern_XY::GetDirectionSignRelative(angDirection_, _mx, _my);
		}
		else if (_src->GetType() == TYPE_LINE) {
			StgMovePattern_Line* src = dynamic_cast<StgMovePattern_Line*>(_src);
			speed_ = src->GetSpeed();
			angDirection_ = src->GetDirectionAngle();
		}
	}

	bool bMaxSpeed2 = false;
	for (auto& [cmd, arg] : listCommand_) {
		switch (cmd) {
		case SET_ZERO:
			acceleration_ = 0;
			angularVelocity_ = 0;
			maxSpeed_ = 0;
			angularAcceleration_ = 0;
			angularMaxVelocity_ = 0;
			break;
		case SET_SPEED:
			speed_ = arg;
			break;
		case SET_ANGLE:
			angDirection_ = arg;
			break;
		case SET_ACCEL:
			acceleration_ = arg;
			break;
		case SET_AGVEL:
			angularVelocity_ = arg;
			break;
		case SET_SPMAX:
			maxSpeed_ = arg;
			break;
		case SET_SPMAX2:
			maxSpeed_ = arg;
			bMaxSpeed2 = true;
			break;
		case SET_AGACC:
			angularAcceleration_ = arg;
			break;
		case SET_AGMAX:
			angularMaxVelocity_ = arg;
			break;
		case ADD_SPEED:
			speed_ += arg;
			break;
		case ADD_ANGLE:
			angDirection_ += arg;
			break;
		case ADD_ACCEL:
			acceleration_ += arg;
			break;
		case ADD_AGVEL:
			angularVelocity_ += arg;
			break;
		case ADD_SPMAX:
			maxSpeed_ += arg;
			break;
		case ADD_AGACC:
			angularAcceleration_ += arg;
			break;
		case ADD_AGMAX:
			angularMaxVelocity_ += arg;
			break;
		}
	}

	if (objRelative_) {
		double dx = objRelative_->GetPositionX() - target_->GetPositionX();
		double dy = objRelative_->GetPositionY() - target_->GetPositionY();
		angDirection_ += atan2(dy, dx);
	}

	SetDirectionAngle(angDirection_);
	if (bMaxSpeed2)
		maxSpeed_ += speed_;

	_RegisterShotDataID();
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
	angDirection_ = 0;
	accelerationX_ = 0;
	accelerationY_ = 0;
	maxSpeedX_ = 0;
	maxSpeedY_ = 0;
}
void StgMovePattern_XY::Move() {
	if (accelerationX_ != 0) {
		c_ += accelerationX_;
		if (maxSpeedX_ != UNCAPPED) {
			if (accelerationX_ > 0)
				c_ = std::min(c_, maxSpeedX_);
			if (accelerationX_ < 0)
				c_ = std::max(c_, maxSpeedX_);
		}
	}
	if (accelerationY_ != 0) {
		s_ += accelerationY_;
		if (maxSpeedY_ != UNCAPPED) {
			if (accelerationY_ > 0)
				s_ = std::min(s_, maxSpeedY_);
			if (accelerationY_ < 0)
				s_ = std::max(s_, maxSpeedY_);
		}
	}

	target_->SetPositionX(target_->GetPositionX() + c_);
	target_->SetPositionY(target_->GetPositionY() + s_);

	if ((c_ != 0 || s_ != 0)) angDirection_ = atan2(s_, c_);

	++frameWork_;
}
void StgMovePattern_XY::Activate(StgMovePattern* _src) {
	if (_src) {
		if (_src->GetType() == TYPE_XY) {
			StgMovePattern_XY* src = (StgMovePattern_XY*)_src;
			c_ = src->c_;
			s_ = src->s_;
			angDirection_ = src->angDirection_;
			accelerationX_ = src->accelerationX_;
			accelerationY_ = src->accelerationY_;
			maxSpeedX_ = src->maxSpeedX_;
			maxSpeedY_ = src->maxSpeedY_;
		}
		else if (_src->GetType() == TYPE_ANGLE) {
			StgMovePattern_Angle* src = (StgMovePattern_Angle*)_src;
			c_ = src->GetSpeedX();
			s_ = src->GetSpeedY();
			angDirection_ = src->angDirection_;
			{
				double c = c_ / src->speed_;
				double s = s_ / src->speed_;
				accelerationX_ = c * src->acceleration_;
				accelerationY_ = s * src->acceleration_;
				maxSpeedX_ = c * src->maxSpeed_;
				maxSpeedY_ = s * src->maxSpeed_;
			}
		}
		else if (_src->GetType() == TYPE_XY_ANG) {
			StgMovePattern_XY_Angle* src = (StgMovePattern_XY_Angle*)_src;

			Math::DVec2 sc;
			Math::DoSinCos(src->angOff_, sc);

			Math::DVec2 tmpS = { src->c_, src->s_ },
				tmpAcc = { src->accelerationX_, src->accelerationY_ },
				tmpMS = { src->maxSpeedX_, src->maxSpeedY_ };
			Math::Rotate2D(tmpS, sc);
			Math::Rotate2D(tmpAcc, sc);
			Math::Rotate2D(tmpMS, sc);

			c_ = tmpS[0]; s_ = tmpS[1];
			angDirection_ = src->angDirection_;
			accelerationX_ = tmpAcc[0]; accelerationY_ = tmpAcc[1];
			maxSpeedX_ = tmpMS[0]; maxSpeedY_ = tmpMS[1];
		}
		else if (_src->GetType() == TYPE_LINE) {
			StgMovePattern_Line* src = dynamic_cast<StgMovePattern_Line*>(_src);
			c_ = src->GetSpeedX();
			s_ = src->GetSpeedY();
			angDirection_ = src->GetDirectionAngle();
		}
	}

	for (auto& [cmd, arg] : listCommand_) {
		switch (cmd) {
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

	_RegisterShotDataID();
}

double StgMovePattern_XY::GetDirectionSignRelative(double baseAngle, double sx, double sy) {
	double ang2 = (sx != 0 && sy != 0) ? atan2(sy, sx) : 0;;
	double angDist = Math::AngleDifferenceRad(baseAngle, ang2);
	return cos(angDist) > 0 ? 1 : -1;
}

//****************************************************************************
//StgMovePattern_XY_Angle
//****************************************************************************
StgMovePattern_XY_Angle::StgMovePattern_XY_Angle(StgMoveObject* target) : StgMovePattern(target) {
	typeMove_ = TYPE_XY_ANG;
	c_ = 0;
	s_ = 0;
	angDirection_ = 0;
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
		if (maxSpeedX_ != UNCAPPED) {
			if (accelerationX_ > 0)
				c_ = std::min(c_, maxSpeedX_);
			if (accelerationX_ < 0)
				c_ = std::max(c_, maxSpeedX_);
		}
	}
	if (accelerationY_ != 0) {
		s_ += accelerationY_;
		if (maxSpeedY_ != UNCAPPED) {
			if (accelerationY_ > 0)
				s_ = std::min(s_, maxSpeedY_);
			if (accelerationY_ < 0)
				s_ = std::max(s_, maxSpeedY_);
		}
	}
	if (angOffAcceleration_ != 0) {
		angOffVelocity_ += angOffAcceleration_;
		if (angOffMaxVelocity_ != UNCAPPED) {
			if (angOffAcceleration_ > 0)
				angOffVelocity_ = std::min(angOffVelocity_, angOffMaxVelocity_);
			if (angOffAcceleration_ < 0)
				angOffVelocity_ = std::max(angOffVelocity_, angOffMaxVelocity_);
		}
	}
	if (angOffVelocity_ != 0) {
		angOff_ += angOffVelocity_;
	}

	Math::DVec2 speed{ c_, s_ };
	Math::Rotate2D(speed, angOff_, 0, 0);

	target_->SetPositionX(target_->GetPositionX() + speed[0]);
	target_->SetPositionY(target_->GetPositionY() + speed[1]);

	if ((c_ != 0 || s_ != 0)) angDirection_ = atan2(s_, c_) + angOff_;

	++frameWork_;
}
void StgMovePattern_XY_Angle::Activate(StgMovePattern* _src) {
	if (_src) {
		if (_src->GetType() == TYPE_XY) {
			StgMovePattern_XY* src = (StgMovePattern_XY*)_src;
			c_ = src->c_;
			s_ = src->s_;
			angDirection_ = src->angDirection_;
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
			angDirection_ = src->angDirection_;
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
		else if (_src->GetType() == TYPE_XY_ANG) {
			StgMovePattern_XY_Angle* src = (StgMovePattern_XY_Angle*)_src;
			c_ = src->c_;
			s_ = src->s_;
			angDirection_ = src->angDirection_;
			accelerationX_ = src->accelerationX_;
			accelerationY_ = src->accelerationY_;
			maxSpeedX_ = src->maxSpeedX_;
			maxSpeedY_ = src->maxSpeedY_;
			angOff_ = src->angOff_;
			angOffVelocity_ = src->angOffVelocity_;
			angOffAcceleration_ = src->angOffAcceleration_;
			angOffMaxVelocity_ = src->angOffMaxVelocity_;
		}
		else if (_src->GetType() == TYPE_LINE) {
			StgMovePattern_Line* src = dynamic_cast<StgMovePattern_Line*>(_src);
			c_ = src->GetSpeedX();
			s_ = src->GetSpeedY();
		}
	}

	for (auto& [cmd, arg] : listCommand_) {
		switch (cmd) {
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

	_RegisterShotDataID();
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
void StgMovePattern_Line::Activate(StgMovePattern* src) {
	double tx = 0, ty = 0;

	for (auto& [cmd, arg] : listCommand_) {
		switch (cmd) {
		case SET_DX:
			tx = arg;
			break;
		case SET_DY:
			ty = arg;
			break;
		}
	}

	if (auto patternS = dynamic_cast<StgMovePattern_Line_Speed*>(this)) {
		double speed = 0;
		for (auto& [cmd, arg] : listCommand_) {
			if (cmd == SET_SP) speed = arg;
		}
		patternS->SetAtSpeed(tx, ty, speed);
	}
	else if (auto patternF = dynamic_cast<StgMovePattern_Line_Frame*>(this)) {
		uint32_t frame = 0;
		Math::Lerp::Type lerpMode = Math::Lerp::LINEAR;
		for (auto& [cmd, arg] : listCommand_) {
			switch (cmd) {
			case SET_FR:
				frame = (uint32_t)arg;
				break;
			case SET_LP:
				lerpMode = (Math::Lerp::Type)arg;
				break;
			}
		}
		patternF->SetAtFrame(tx, ty, frame, Math::Lerp::GetFunc<double, double>(lerpMode), Math::Lerp::GetFuncDifferential<double>(lerpMode));
	}
	else if (auto patternW = dynamic_cast<StgMovePattern_Line_Weight*>(this)) {
		double weight = 0;
		double maxSpeed = 0;
		for (auto& [cmd, arg] : listCommand_) {
			switch (cmd) {
			case SET_WG:
				weight = arg;
				break;
			case SET_MS:
				maxSpeed = arg;
				break;
			}
		}
		patternW->SetAtWeight(tx, ty, weight, maxSpeed);
	}

	_RegisterShotDataID();
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
void StgMovePattern_Line_Frame::SetAtFrame(double tx, double ty, uint32_t frame, lerp_func lerpFunc, lerp_diff_func diffFunc) {
	iniPos_[0] = target_->GetPositionX();
	iniPos_[1] = target_->GetPositionY();
	targetPos_[0] = tx;
	targetPos_[1] = ty;

	moveLerpFunc = lerpFunc;
	diffLerpFunc = diffFunc;

	maxFrame_ = std::max(frame, 1U);

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
	Math::DVec2 tPos;
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
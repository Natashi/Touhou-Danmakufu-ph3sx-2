#include "source/GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgSystem.hpp"

//****************************************************************************
//StgMoveObject
//****************************************************************************
StgMoveObject::StgMoveObject(StgStageController* stageController) : StgObjectBase(stageController),
	enableMovement(true),
	position({}),
	framePattern_(0), frameMove_(0) {}

void StgMoveObject::Copy(StgMoveObject* src) {
	// Copy object then re-parent target
	auto clonePattern = [this](const StgMovePattern* srcPattern) {
		auto cloned = srcPattern->Clone();

		// Take ownership of pointer
		auto pattern = unique_ptr<StgMovePattern>(cloned);
		pattern->target_ = this;
		
		return pattern;
	};
	
	position = src->position;

	pattern_ = clonePattern(src->pattern_.get());
	frameMove_ = src->frameMove_;
	framePattern_ = src->framePattern_;

	scheduledPatterns_.clear();
	for (auto& [frame, patterns] : src->scheduledPatterns_) {
		std::list<unique_ptr<StgMovePattern>> listPattern;
		
		for (auto& pattern : patterns) {
			listPattern.push_back(clonePattern(pattern.get()));
		}
		
		scheduledPatterns_[frame] = MOVE(listPattern);
	}
}

void StgMoveObject::_Move() {
	if (!enableMovement)
		return;
	++frameMove_;

	if (scheduledPatterns_.size() > 0) {
		auto itr = scheduledPatterns_.begin();

		// Process all patterns scheduled to be executed this frame
		while (framePattern_ >= itr->first) {
			for (auto& pattern : itr->second) {
				AttachPattern(MOVE(pattern));
			}

			itr = scheduledPatterns_.erase(itr);
			if (scheduledPatterns_.size() == 0)
				break;
		}

		// If pattern somehow became null, set to Angle as fallback
		if (pattern_ == nullptr)
			pattern_.reset(new StgMovePattern_Angle(this));
	}
	
	if (pattern_)
		pattern_->Move();
	
	++framePattern_;
}

void StgMoveObject::AttachPattern(unique_ptr<StgMovePattern> pattern) {
	// Transfer movement from previous pattern
	pattern->Activate(pattern_.get());

	// Then set as the current pattern
	pattern_ = MOVE(pattern);
}

void StgMoveObject::AddPattern(uint32_t frameDelay, unique_ptr<StgMovePattern> pattern,
	bool mustSchedule
) {
	if (frameDelay == 0 && !mustSchedule) {
		AttachPattern(MOVE(pattern));
	}
	else {
		uint32_t frame = frameDelay + framePattern_;
		scheduledPatterns_[frame].push_back(MOVE(pattern));
	}
}

double StgMoveObject::GetSpeed() {
	return pattern_ ? pattern_->GetSpeed() : 0;
}
double StgMoveObject::GetDirectionAngle() {
	return pattern_ ? pattern_->GetDirectionAngle() : 0;
}

void StgMoveObject::SetSpeed(double speed) {
	auto pattern = dynamic_cast<StgMovePattern_Angle*>(pattern_.get());
	if (pattern == nullptr) {
		pattern = new StgMovePattern_Angle(this);
		pattern_.reset(pattern);
	}
	
	pattern->speed = speed;
}
void StgMoveObject::SetDirectionAngle(double angle) {
	auto pattern = dynamic_cast<StgMovePattern_Angle*>(pattern_.get());
	if (pattern == nullptr) {
		pattern = new StgMovePattern_Angle(this);
		pattern_.reset(pattern);
	}
	
	pattern->SetDirectionAngle(angle);
}

void StgMoveObject::SetSpeedX(double speedX) {
	auto pattern = dynamic_cast<StgMovePattern_XY*>(pattern_.get());
	if (pattern == nullptr) {
		pattern = new StgMovePattern_XY(this);
		pattern_.reset(pattern);
	}
	
	pattern->SetSpeedX(speedX);
}
void StgMoveObject::SetSpeedY(double speedY) {
	auto pattern = dynamic_cast<StgMovePattern_XY*>(pattern_.get());
	if (pattern == nullptr) {
		pattern = new StgMovePattern_XY(this);
		pattern_.reset(pattern);
	}
	
	pattern->SetSpeedY(speedY);
}

//****************************************************************************
//StgMovePattern
//****************************************************************************
StgMovePattern::StgMovePattern(StgMoveObject* target) :
	target_(target),
	frameWork_(0),
	c_(0), s_(0),
	direction_(0),
	shotDataId(NO_CHANGE) {}

ref_unsync_ptr<StgMoveObject> StgMovePattern::_GetMoveObject(int id) {
	if (id == DxScript::ID_INVALID) return nullptr;

	ref_unsync_ptr<DxScriptObjectBase> base = _GetStageController()->GetMainRenderObject(id);
	if (base == nullptr || base->IsDeleted()) return nullptr;

	return ref_unsync_ptr<StgMoveObject>::Cast(base);
}
void StgMovePattern::_RegisterShotDataID() {
	if (target_ == nullptr || shotDataId == NO_CHANGE) return;

	if (auto objShot = dynamic_cast<StgShotObject*>(target_)) {
		objShot->SetShotDataID(shotDataId);
	}
}

//****************************************************************************
//StgMovePattern_Angle
//****************************************************************************
StgMovePattern_Angle::StgMovePattern_Angle(StgMoveObject* target) :
	StgMovePattern(target),
	speed(0), acceleration(0), maxSpeed(0),
	angularVelocity(0),
	angularAcceleration(0),
	angularMaxVelocity(0) {}

void StgMovePattern_Angle::Move() {
	double angle = direction_;

	if (acceleration != 0) {
		speed += acceleration;

		if (maxSpeed != UNCAPPED) {
			if (acceleration > 0)
				speed = std::min(speed, maxSpeed);
			if (acceleration < 0)
				speed = std::max(speed, maxSpeed);
		}
	}
	if (angularAcceleration != 0) {
		angularVelocity += angularAcceleration;

		if (angularMaxVelocity != UNCAPPED) {
			if (angularAcceleration > 0)
				angularVelocity = std::min(angularVelocity, angularMaxVelocity);
			if (angularAcceleration < 0)
				angularVelocity = std::max(angularVelocity, angularMaxVelocity);
		}
	}
	if (angularVelocity != 0) {
		SetDirectionAngle(angle + angularVelocity);
	}

	target_->position = {
		fma(speed, c_, target_->position[0]),
		fma(speed, s_, target_->position[1]),
	};

	++frameWork_;
}
void StgMovePattern_Angle::Activate(StgMovePattern* _src) {
	angularVelocity = 0;
	angularAcceleration = 0;
	angularMaxVelocity = 0;
	
	if (_src) {
		if (_src->GetType() == MovePatternType::Angle) {
			auto src = dcast(StgMovePattern_Angle*, _src);

			direction_ = src->direction_;
			
			speed = src->speed;
			acceleration = src->acceleration;
			maxSpeed = src->maxSpeed;
			angularVelocity = src->angularVelocity;
			angularAcceleration = src->angularAcceleration;
			angularMaxVelocity = src->angularMaxVelocity;
		}
		else if (_src->GetType() == MovePatternType::XY) {
			auto src = dcast(StgMovePattern_XY*, _src);
			
			speed = _src->GetSpeed();
			direction_ = _src->GetDirectionAngle();

			double ax = src->accelerationX, ay = src->accelerationY;
			double mx = src->maxSpeedX, my = src->maxSpeedY;
			
			double aSign = StgMovePattern_XY::GetDirectionSignRelative(direction_, ax, ay);
			acceleration = hypot(ax, ay) * aSign;
			
			double mSign = StgMovePattern_XY::GetDirectionSignRelative(direction_, mx, my);
			maxSpeed = hypot(mx, my) * mSign;
		}
		else if (_src->GetType() == MovePatternType::XY_WithAngle) {
			auto src = dcast(StgMovePattern_XY_Angle*, _src);
			
			speed = _src->GetSpeed();
			direction_ = _src->GetDirectionAngle();

			double ax = src->accelerationX, ay = src->accelerationY;
			double mx = src->maxSpeedX, my = src->maxSpeedY;
			
			double aSign = StgMovePattern_XY::GetDirectionSignRelative(direction_, ax, ay);
			acceleration = hypot(ax, ay) * aSign;
			
			double mSign = StgMovePattern_XY::GetDirectionSignRelative(direction_, mx, my);
			maxSpeed = hypot(mx, my) * mSign;
		}
		else if (_src->GetType() == MovePatternType::Line) {
			speed = _src->GetSpeed();
			direction_ = _src->GetDirectionAngle();
		}
	}

	bool bMaxSpeed2 = false;
	for (auto& [cmd, arg] : listCommand_) {
		switch (cmd) {
		case SET_ZERO:
			acceleration = 0;
			angularVelocity = 0;
			maxSpeed = 0;
			angularAcceleration = 0;
			angularMaxVelocity = 0;
			break;
		case SET_SPEED:
			speed = arg;
			break;
		case SET_ANGLE:
			direction_ = arg;
			break;
		case SET_ACCEL:
			acceleration = arg;
			break;
		case SET_AGVEL:
			angularVelocity = arg;
			break;
		case SET_SPMAX:
			maxSpeed = arg;
			break;
		case SET_SPMAX2:
			maxSpeed = arg;
			bMaxSpeed2 = true;
			break;
		case SET_AGACC:
			angularAcceleration = arg;
			break;
		case SET_AGMAX:
			angularMaxVelocity = arg;
			break;
		case ADD_SPEED:
			speed += arg;
			break;
		case ADD_ANGLE:
			direction_ += arg;
			break;
		case ADD_ACCEL:
			acceleration += arg;
			break;
		case ADD_AGVEL:
			angularVelocity += arg;
			break;
		case ADD_SPMAX:
			maxSpeed += arg;
			break;
		case ADD_AGACC:
			angularAcceleration += arg;
			break;
		case ADD_AGMAX:
			angularMaxVelocity += arg;
			break;
		}
	}

	if (objRelative_) {
		double dx = objRelative_->position[0] - target_->position[0];
		double dy = objRelative_->position[1] - target_->position[1];
		direction_ += atan2(dy, dx);
	}

	SetDirectionAngle(direction_);
	if (bMaxSpeed2)
		maxSpeed += speed;

	_RegisterShotDataID();
}
void StgMovePattern_Angle::SetDirectionAngle(double angle) {
	if (angle != StgMovePattern::NO_CHANGE) {
		angle = Math::NormalizeAngleRad(angle);
		c_ = cos(angle);
		s_ = sin(angle);
	}
	direction_ = angle;
}

//****************************************************************************
//StgMovePattern_XY
//****************************************************************************
StgMovePattern_XY::StgMovePattern_XY(StgMoveObject* target) :
	StgMovePattern(target),
	accelerationX(0),
	accelerationY(0),
	maxSpeedX(0),
	maxSpeedY(0) {}

void StgMovePattern_XY::Move() {
	if (accelerationX != 0) {
		c_ += accelerationX;

		if (maxSpeedX != UNCAPPED) {
			if (accelerationX > 0)
				c_ = std::min(c_, maxSpeedX);
			if (accelerationX < 0)
				c_ = std::max(c_, maxSpeedX);
		}
	}
	if (accelerationY != 0) {
		s_ += accelerationY;

		if (maxSpeedY != UNCAPPED) {
			if (accelerationY > 0)
				s_ = std::min(s_, maxSpeedY);
			if (accelerationY < 0)
				s_ = std::max(s_, maxSpeedY);
		}
	}
	
	target_->position = {
		target_->position[0] + c_,
		target_->position[1] + s_,
	};

	++frameWork_;
}
void StgMovePattern_XY::Activate(StgMovePattern* _src) {
	if (_src) {
		if (_src->GetType() == MovePatternType::XY) {
			auto src = dcast(StgMovePattern_XY*, _src);

			c_ = src->c_;
			s_ = src->s_;
			
			accelerationX = src->accelerationX;
			accelerationY = src->accelerationY;
			maxSpeedX = src->maxSpeedX;
			maxSpeedY = src->maxSpeedY;
		}
		else if (_src->GetType() == MovePatternType::Angle) {
			auto src = dcast(StgMovePattern_Angle*, _src);
			
			c_ = src->c_;
			s_ = src->s_;
			
			double cNorm = c_ / src->speed;
			double sNorm = s_ / src->speed;
			accelerationX = cNorm * src->acceleration;
			accelerationY = sNorm * src->acceleration;
			maxSpeedX = cNorm * src->maxSpeed;
			maxSpeedY = sNorm * src->maxSpeed;
		}
		else if (_src->GetType() == MovePatternType::XY_WithAngle) {
			auto src = dcast(StgMovePattern_XY_Angle*, _src);

			Math::DVec2 sc;
			Math::DoSinCos(src->angOffset, sc);

			Math::DVec2 tmpS = { src->c_, src->s_ },
				tmpAcc = { src->accelerationX, src->accelerationY },
				tmpMs = { src->maxSpeedX, src->maxSpeedY };
			Math::Rotate2D(tmpS, sc);
			Math::Rotate2D(tmpAcc, sc);
			Math::Rotate2D(tmpMs, sc);

			c_ = tmpS[0]; s_ = tmpS[1];
			accelerationX = tmpAcc[0]; accelerationY = tmpAcc[1];
			maxSpeedX = tmpMs[0]; maxSpeedY = tmpMs[1];
		}
		else if (_src->GetType() == MovePatternType::Line) {
			auto src = dcast(StgMovePattern_Line*, _src);
			
			c_ = src->GetSpeedX();
			s_ = src->GetSpeedY();
		}
	}

	for (auto& [cmd, arg] : listCommand_) {
		switch (cmd) {
		case SET_ZERO:
			accelerationX = 0;
			accelerationY = 0;
			maxSpeedX = 0;
			maxSpeedY = 0;
			break;
		case SET_S_X:
			c_ = arg;
			break;
		case SET_S_Y:
			s_ = arg;
			break;
		case SET_A_X:
			accelerationX = arg;
			break;
		case SET_A_Y:
			accelerationY = arg;
			break;
		case SET_M_X:
			maxSpeedX = arg;
			break;
		case SET_M_Y:
			maxSpeedY = arg;
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
StgMovePattern_XY_Angle::StgMovePattern_XY_Angle(StgMoveObject* target) :
	StgMovePattern_XY(target),
	angOffset(0),
	angOffsetVelocity(0),
	angOffsetAcceleration(0),
	angOffsetMaxVelocity(0) {}

void StgMovePattern_XY_Angle::Move() {
	if (accelerationX != 0) {
		c_ += accelerationX;

		if (maxSpeedX != UNCAPPED) {
			if (accelerationX > 0)
				c_ = std::min(c_, maxSpeedX);
			if (accelerationX < 0)
				c_ = std::max(c_, maxSpeedX);
		}
	}
	if (accelerationY != 0) {
		s_ += accelerationY;

		if (maxSpeedY != UNCAPPED) {
			if (accelerationY > 0)
				s_ = std::min(s_, maxSpeedY);
			if (accelerationY < 0)
				s_ = std::max(s_, maxSpeedY);
		}
	}

	if (angOffsetAcceleration != 0) {
		angOffsetVelocity += angOffsetAcceleration;

		if (angOffsetMaxVelocity != UNCAPPED) {
			if (angOffsetAcceleration > 0)
				angOffsetVelocity = std::min(angOffsetVelocity, angOffsetMaxVelocity);
			if (angOffsetAcceleration < 0)
				angOffsetVelocity = std::max(angOffsetVelocity, angOffsetMaxVelocity);
		}
	}
	if (angOffsetVelocity != 0) {
		angOffset += angOffsetVelocity;
	}

	Math::DVec2 speed{ c_, s_ };
	Math::Rotate2D(speed, angOffset, 0, 0);

	target_->position = {
		target_->position[0] + speed[0],
		target_->position[1] + speed[1],
	};

	++frameWork_;
}
void StgMovePattern_XY_Angle::Activate(StgMovePattern* _src) {
	if (_src) {
		if (_src->GetType() == MovePatternType::XY_WithAngle) {
			auto src = dcast(StgMovePattern_XY_Angle*, _src);

			c_ = src->c_;
			s_ = src->s_;
			
			accelerationX = src->accelerationX;
			accelerationY = src->accelerationY;
			maxSpeedX = src->maxSpeedX;
			maxSpeedY = src->maxSpeedY;
			
			angOffset = src->angOffset;
			angOffsetVelocity = src->angOffsetVelocity;
			angOffsetAcceleration = src->angOffsetAcceleration;
			angOffsetMaxVelocity = src->angOffsetMaxVelocity;
		}
		else if (_src->GetType() == MovePatternType::Angle) {
			auto src = dcast(StgMovePattern_Angle*, _src);
			
			c_ = src->c_;
			s_ = src->s_;
			
			double cNorm = c_ / src->speed;
			double sNorm = s_ / src->speed;
			accelerationX = cNorm * src->acceleration;
			accelerationY = sNorm * src->acceleration;
			maxSpeedX = cNorm * src->maxSpeed;
			maxSpeedY = sNorm * src->maxSpeed;
		}
		else if (_src->GetType() == MovePatternType::XY) {
			auto src = dcast(StgMovePattern_XY*, _src);
			
			c_ = src->c_;
			s_ = src->s_;
			accelerationX = src->accelerationX;
			accelerationY = src->accelerationY;
			maxSpeedX = src->maxSpeedX;
			maxSpeedY = src->maxSpeedY;
		}
		else if (_src->GetType() == MovePatternType::Line) {
			auto src = dcast(StgMovePattern_Line*, _src);
			
			c_ = src->GetSpeedX();
			s_ = src->GetSpeedY();
		}
	}

	for (auto& [cmd, arg] : listCommand_) {
		switch (cmd) {
		case SET_ZERO:
			accelerationX = 0;
			accelerationY = 0;
			maxSpeedX = 0;
			maxSpeedY = 0;
			angOffset = 0;
			angOffsetVelocity = 0;
			angOffsetAcceleration = 0;
			angOffsetMaxVelocity = 0;
			break;
		case SET_S_X:
			c_ = arg;
			break;
		case SET_S_Y:
			s_ = arg;
			break;
		case SET_A_X:
			accelerationX = arg;
			break;
		case SET_A_Y:
			accelerationY = arg;
			break;
		case SET_M_X:
			maxSpeedX = arg;
			break;
		case SET_M_Y:
			maxSpeedY = arg;
			break;
		case SET_ANGLE:
			angOffset = arg;
			break;
		case SET_AGVEL:
			angOffsetVelocity = arg;
			break;
		case SET_AGACC:
			angOffsetAcceleration = arg;
			break;
		case SET_AGMAX:
			angOffsetMaxVelocity = arg;
			break;
		}
	}

	_RegisterShotDataID();
}

//****************************************************************************
//StgMovePattern_Line
//****************************************************************************
StgMovePattern_Line::StgMovePattern_Line(StgMoveObject* target) :
	StgMovePattern(target),
	maxFrame_(-1),
	speed_(0),
	iniPos_({}), targetPos_({}) {}

void StgMovePattern_Line::Move() {
	if (frameWork_ < maxFrame_) {
		target_->position = {
			fma(speed_, c_, target_->position[0]),
			fma(speed_, s_, target_->position[1]),
		};
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

	switch (GetLineType()) {
	case LineType::Speed:
		if (auto pattern = dynamic_cast<StgMovePattern_Line_Speed*>(this)) {
			double speed = 0;
			for (auto& [cmd, arg] : listCommand_) {
				if (cmd == SET_SP) speed = arg;
			}
			pattern->SetAtSpeed(tx, ty, speed);
		}
		break;
	case LineType::Frame:
		if (auto pattern = dynamic_cast<StgMovePattern_Line_Frame*>(this)) {
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
			pattern->SetAtFrame(tx, ty, frame,
				Math::Lerp::GetFunc<double, double>(lerpMode),
				Math::Lerp::GetFuncDifferential<double>(lerpMode));
		}
		break;
	case LineType::Weight:
		if (auto pattern = dynamic_cast<StgMovePattern_Line_Weight*>(this)) {
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
			pattern->SetAtWeight(tx, ty, weight, maxSpeed);
		}
		break;
	}

	_RegisterShotDataID();
}

StgMovePattern_Line_Speed::StgMovePattern_Line_Speed(StgMoveObject* target)
	: StgMovePattern_Line(target) {}

void StgMovePattern_Line_Speed::SetAtSpeed(double tx, double ty, double speed) {
	iniPos_ = target_->position;
	targetPos_ = { tx, ty };

	double dx = targetPos_[0] - iniPos_[0];
	double dy = targetPos_[1] - iniPos_[1];
	double dist = hypot(dx, dy);

	//speed_ = speed;
	direction_ = atan2(dy, dx);
	maxFrame_ = std::floor(dist / speed + 0.001);
	speed_ = dist / maxFrame_;	//Speed correction to reach the destination in integer frames

	c_ = dx / dist;
	s_ = dy / dist;
}

StgMovePattern_Line_Frame::StgMovePattern_Line_Frame(StgMoveObject* target) :
	StgMovePattern_Line(target),
	speedRate_(0),
	moveLerpFunc(Math::Lerp::Linear<double, double>),
	diffLerpFunc(Math::Lerp::DifferentialLinear<double>) {}

void StgMovePattern_Line_Frame::SetAtFrame(double tx, double ty, uint32_t frame, lerp_func lerpFunc, lerp_diff_func diffFunc) {
	iniPos_ = target_->position;
	targetPos_ = { tx, ty };

	moveLerpFunc = lerpFunc;
	diffLerpFunc = diffFunc;

	maxFrame_ = std::max(frame, 1U);

	double dx = targetPos_[0] - iniPos_[0];
	double dy = targetPos_[1] - iniPos_[1];
	double dist = hypot(dx, dy);

	speedRate_ = dist / (double)frame;

	speed_ = diffLerpFunc(0.0) * speedRate_;
	direction_ = atan2(dy, dx);

	c_ = dx / dist;
	s_ = dy / dist;
}
void StgMovePattern_Line_Frame::Move() {
	if (frameWork_ < maxFrame_) {
		double rate = (frameWork_ + 1) / (double)maxFrame_;

		speed_ = diffLerpFunc(rate) * speedRate_;
		
		target_->position = {
			moveLerpFunc(iniPos_[0], targetPos_[0], rate),
			moveLerpFunc(iniPos_[1], targetPos_[1], rate),
		};
	}
	else {
		speed_ = 0;
	}

	++frameWork_;
}

StgMovePattern_Line_Weight::StgMovePattern_Line_Weight(StgMoveObject* target) :
	StgMovePattern_Line(target),
	dist_(0),
	weight_(0),
	maxSpeed_(0) {}

void StgMovePattern_Line_Weight::SetAtWeight(double tx, double ty, double weight, double maxSpeed) {
	iniPos_ = target_->position;
	targetPos_ = { tx, ty };

	weight_ = weight;
	maxSpeed_ = maxSpeed;

	double dx = targetPos_[0] - iniPos_[0];
	double dy = targetPos_[1] - iniPos_[1];
	dist_ = hypot(dx, dy);

	speed_ = maxSpeed_;
	direction_ = atan2(dy, dx);

	c_ = dx / dist_;
	s_ = dy / dist_;
}
void StgMovePattern_Line_Weight::Move() {
	if (dist_ < 0.1) {
		speed_ = 0;

		target_->position = targetPos_;
	}
	else {
		speed_ = std::min(dist_ / weight_, maxSpeed_);
		dist_ -= speed_;

		target_->position = {
			fma(speed_, c_, target_->position[0]),
			fma(speed_, s_, target_->position[1]),
		};
	}
	
	++frameWork_;
}
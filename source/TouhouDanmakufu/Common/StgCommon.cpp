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

	parent_ = nullptr;
	offX_ = 0;
	offY_ = 0;
}
StgMoveObject::~StgMoveObject() {
	parent_ = nullptr;
	pattern_ = nullptr;

	if (listOwnedParent_.size() > 0) {
		for (auto& iPar : listOwnedParent_) {
			if (iPar && !iPar->bAutoDelete_) {
				// If parent exists and is not set to auto-delete, remove target
				iPar->target_ = nullptr;
				iPar->UpdatePosition();
			}
		}
	}
}
void StgMoveObject::Move() {
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
	if (pattern_ == nullptr) return;
	pattern_->Move();
	++framePattern_;
}
void StgMoveObject::_Move() {	
	if (parent_) return; // Objects with parents cannot move without parental supervision
	if (bEnableMovement_) Move();

	if (listOwnedParent_.size() > 0) {
		for (auto& iPar : listOwnedParent_) {
			if (iPar) iPar->UpdateChildren();
		}
	}
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
double StgMoveObject::GetSpeedX() {
	if (pattern_ == nullptr) return 0;
	double res = pattern_->GetSpeedX();
	return res;
}
double StgMoveObject::GetSpeedY() {
	if (pattern_ == nullptr) return 0;
	double res = pattern_->GetSpeedY();
	return res;
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
void StgMoveObject::RemoveParent(ref_unsync_weak_ptr<StgMoveObject> self, bool bErase) {
	if (parent_) {
		if (bErase) {
			auto& vec = parent_->listChild_;
			vec.erase(std::remove(vec.begin(), vec.end(), self), vec.end());
		}

		parent_ = nullptr;
		offX_ = posX_;
		offY_ = posY_;
	}
}
void StgMoveObject::UpdateRelativePosition() { // Optimize later I guess?
	if (parent_) {
		// If parent exists, reverse its transformation
		double pX = parent_->posX_ + parent_->offX_;
		double pY = parent_->posY_ + parent_->offY_;
		double pScaX = parent_->scaX_;
		double pScaY = parent_->scaY_;
		double pRotZ = parent_->rotZ_;

		double cX = posX_ - pX;
		double cY = posY_ - pY;

		double sc[2]{ 0, 1 };
		if (pRotZ != 0) Math::DoSinCos(Math::DegreeToRadian(-pRotZ), sc);

		if (pScaX != pScaY && parent_->transOrder_ == StgMoveParent::ORDER_SCALE_ANGLE) {
			offX_ = sc[1] * cX / pScaX - sc[0] * cY / pScaY;
			offY_ = sc[0] * cX / pScaX + sc[1] * cY / pScaY;
		}
		else {
			offX_ = (sc[1] * cX - sc[0] * cY) / pScaX;
			offY_ = (sc[0] * cX + sc[1] * cY) / pScaY;
		}
	}
	else {
		// Otherwise, just use global coordinates
		offX_ = posX_;
		offY_ = posY_;
	}
}
double StgMoveObject::GetDistanceFromParent() {
	double x0 = 0, y0 = 0;
	double x1 = posX_;
	double y1 = posY_;
	if (auto& par = parent_) {
		x0 = par->offX_;
		y0 = par->offY_;
		if (auto& tar = par->target_) {
			x0 += tar->posX_;
			y0 += tar->posY_;
		}
	}
	return hypot(x1 - x0, y1 - y0);
}
double StgMoveObject::GetAngleFromParent() {
	double x0 = 0, y0 = 0;
	double x1 = posX_;
	double y1 = posY_;
	if (auto& par = parent_) {
		x0 = par->offX_;
		y0 = par->offY_;
		if (auto& tar = par->target_) {
			x0 += tar->posX_;
			y0 += tar->posY_;
		}
	}
	return Math::NormalizeAngleDeg(Math::RadianToDegree(atan2(y1 - y0, x1 - x0)));
}

//****************************************************************************
//StgMoveParent
//****************************************************************************
StgMoveParent::StgMoveParent(StgStageController* stageController) {
	typeObject_ = TypeObject::MoveParent;
	stageController_ = stageController;

	target_ = nullptr;
	typeAngle_ = ANGLE_FIXED;
	transOrder_ = ORDER_ANGLE_SCALE;
	bAutoDelete_ = false;
	bMoveChild_ = true;
	bRotateLaser_ = false;

	posX_ = 0;
	posY_ = 0;
	offX_ = 0;
	offY_ = 0;
	scaX_ = 1;
	scaY_ = 1;
	rotZ_ = 0;
}
StgMoveParent::~StgMoveParent() {
	if (listChild_.size() > 0) {
		auto iter = listChild_.begin();
		while (iter != listChild_.end()) {
			auto& child = *iter;
			if (child)
				child->RemoveParent(child, false);

			iter = listChild_.erase(iter);
		}
	}
	target_ = nullptr;
}
void StgMoveParent::Work() {
	if (target_ != nullptr || bAutoDelete_) return;
	
	// If there's no target object, update the children here instead
	UpdateChildren();
}
void StgMoveParent::CleanUp() {
	if (target_ == nullptr && bAutoDelete_) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
		return;
	}
	if (listChild_.size() > 0) {
		auto iter = listChild_.begin();
		while (iter != listChild_.end()) {
			auto& child = *iter;
			if (child == nullptr)
				iter = listChild_.erase(iter);
			else
				++iter;
		}
	}
}
void StgMoveParent::SetParentObject(ref_unsync_weak_ptr<StgMoveParent> self, ref_unsync_weak_ptr<StgMoveObject> parent) {
	if (target_ == parent) return; // Exit if target is the same

	if (target_ != nullptr) { // Remove reference from previous target object
		auto& vec = target_->listOwnedParent_;
		vec.erase(std::remove(vec.begin(), vec.end(), self), vec.end());
	}

	if (parent != nullptr) { // Add reference to target object's "owned parents" list
		parent->listOwnedParent_.push_back(self);
		target_ = parent;
	}
	else {
		target_ = nullptr;
	}
	UpdatePosition();
}
void StgMoveParent::AddChild(ref_unsync_weak_ptr<StgMoveParent> self, ref_unsync_weak_ptr<StgMoveObject> child) {
	// Parenting the player object is an astoundingly stupid idea.
	if (dynamic_cast<StgPlayerObject*>(child.get())) return;
	if (std::find(listChild_.begin(), listChild_.end(), child) != listChild_.end()) return; // Exit if child is already added
	if (child->parent_ != nullptr) { // Remove from previous parent
		auto& vec = child->parent_->listChild_;
		vec.erase(std::remove(vec.begin(), vec.end(), child), vec.end());
	}
	child->SetParent(self);
	listChild_.push_back(child);
	child->UpdateRelativePosition(); // Children's relative positions are initialized relative to the parent
}
void StgMoveParent::RemoveChildren() {
	if (listChild_.size() > 0) {
		auto iter = listChild_.begin();
		while (iter != listChild_.end()) {
			auto& child = *iter;
			if (child)
				child->RemoveParent(child, false);

			iter = listChild_.erase(iter);
		}
	}
}
void StgMoveParent::SetTransformAngle(double z) {
	if (typeAngle_ == ANGLE_ROTATE) {
		double diff = Math::DegreeToRadian(z - rotZ_);
		for (auto& child : listChild_) {
			if (child) {
				child->SetDirectionAngle(child->GetDirectionAngle() + diff);
				if (bRotateLaser_) {
					StgStraightLaserObject* laser = dynamic_cast<StgStraightLaserObject*>(child.get());
					if (laser) laser->SetLaserAngle(laser->GetLaserAngle() + diff);
				}
			}
		}
	}
	rotZ_ = z;
}
/*
double StgMoveParent::GetRadiusAtAngle(double angle) {
	double rad = 1;
	if (scaX_ == scaY_) 
		rad = scaX_;
	else {
		if (transOrder_ == ORDER_SCALE_ANGLE)
			angle -= rotZ_;

		angle = Math::DegreeToRadian(angle);
		rad = (scaX_ * scaY_) / hypot(scaX_ * cos(angle), scaY_ * sin(angle));
	}
	return rad;
}
*/
void StgMoveParent::ApplyTransformation() {
	scaX_ = 1;
	scaY_ = 1;
	rotZ_ = 0;
	for (auto& child : listChild_) {
		if (child) child->UpdateRelativePosition();
	}
}
void StgMoveParent::MoveChild(StgMoveObject* child) {
	double pX = posX_ + offX_;
	double pY = posY_ + offY_;
	double pScaX = scaX_;
	double pScaY = scaY_;
	double pRotZ = rotZ_;

	double cX = child->offX_;
	double cY = child->offY_;

	double sc[2]{ 0, 1 };
	if (pRotZ != 0) Math::DoSinCos(Math::DegreeToRadian(pRotZ), sc);

	double x1 = pX;
	double y1 = pY;

	if (transOrder_ == ORDER_SCALE_ANGLE && pScaX != pScaY) {
		x1 += sc[1] * cX * pScaX - sc[0] * cY * pScaY;
		y1 += sc[0] * cX * pScaX + sc[1] * cY * pScaY;
	}
	else {
		x1 += (sc[1] * cX - sc[0] * cY) * pScaX;
		y1 += (sc[0] * cX + sc[1] * cY) * pScaY;
	}

	child->SetPositionX(x1);
	child->SetPositionY(y1);

	double dir = StgMovePattern::NO_CHANGE;
	if (typeAngle_ == ANGLE_OUTWARD)
		dir = atan2(y1 - pY, x1 - pX);
	else if (typeAngle_ == ANGLE_INWARD)
		dir = atan2(pY - y1, pX - x1);
	
	if (dir != StgMovePattern::NO_CHANGE) {
		child->SetDirectionAngle(dir);
		if (bRotateLaser_) {
			StgStraightLaserObject* laser = dynamic_cast<StgStraightLaserObject*>(child);
			if (laser) laser->SetLaserAngle(dir);
		}
	}
	
	// Yes, children can have children too.
	if (child->listOwnedParent_.size() > 0) {
		for (auto& iPar : child->listOwnedParent_) {
			if (iPar) iPar->UpdateChildren();
		}
	}
}
void StgMoveParent::UpdateChildren() {
	// This looks stupid but please have faith in me
	double px0 = posX_;
	double py0 = posY_;
	UpdatePosition();
	double px1 = posX_;
	double py1 = posY_;

	auto& list = listChild_;
	if (listChild_.size() > 0) {
		
		auto iter = listChild_.begin();
		while (iter != listChild_.end()) {
			auto child = (*iter).get();
			if (child == nullptr) {
				iter = list.erase(iter);
			}
			else {
				if (target_ && typeAngle_ == ANGLE_FOLLOW) {
					child->SetDirectionAngle(target_->GetDirectionAngle());
					if (bRotateLaser_) {
						StgStraightLaserObject* laser = dynamic_cast<StgStraightLaserObject*>(child);
						if (laser) laser->SetLaserAngle(target_->GetDirectionAngle());
					}
				}
				double x0 = child->posX_;
				double y0 = child->posY_;
				
				MoveChild(child);
				if (bMoveChild_) {
					child->Move();
					if (child->GetSpeed() != 0) child->UpdateRelativePosition();
				}

				if (typeAngle_ == ANGLE_ABSOLUTE || typeAngle_ == ANGLE_RELATIVE) {
					double x1 = child->posX_;
					double y1 = child->posY_;

					if (typeAngle_ == ANGLE_RELATIVE) {
						x0 -= px0;
						y0 -= py0;
						x1 -= px1;
						y1 -= py1;
					}
					
					double x = x1 - x0;
					double y = y1 - y0;

					// This is to prevent, uh, angular seizures
					if (hypot(x, y) > 0.0001) {
						double dir = atan2(y, x);
						child->SetDirectionAngle(dir);
						if (bRotateLaser_) {
							StgStraightLaserObject* laser = dynamic_cast<StgStraightLaserObject*>(child);
							if (laser) laser->SetLaserAngle(dir);
						}
					}
				}

				++iter;
			}
		}
	}
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
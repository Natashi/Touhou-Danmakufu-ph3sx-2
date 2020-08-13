#pragma once

#include "../../GcLib/pch.h"

#include "DnhCommon.hpp"
#include "DnhGcLibImpl.hpp"
#include "DnhReplay.hpp"
#include "DnhScript.hpp"

class StgSystemController;
class StgSystemInformation;
class StgStageController;
class StgPackageController;
class StgStageInformation;
class StgSystemInformation;
class StgMovePattern;

/**********************************************************
//StgMoveObject
**********************************************************/
class StgMoveObject {
	friend StgMovePattern;
private:
	StgStageController* stageController_;
protected:
	double posX_;
	double posY_;
	shared_ptr<StgMovePattern> pattern_;

	bool bEnableMovement_;

	int framePattern_;
	std::map<int, std::list<shared_ptr<StgMovePattern>>> mapPattern_;
	virtual void _Move();
	void _AttachReservedPattern(std::shared_ptr<StgMovePattern> pattern);
public:
	StgMoveObject(StgStageController* stageController);
	virtual ~StgMoveObject();

	void SetEnableMovement(bool b) { bEnableMovement_ = b; }
	bool IsEnableMovement() { return bEnableMovement_; }

	double GetPositionX() { return posX_; }
	void SetPositionX(double pos) { posX_ = pos; }
	double GetPositionY() { return posY_; }
	void SetPositionY(double pos) { posY_ = pos; }

	double GetSpeed();
	void SetSpeed(double speed);
	double GetDirectionAngle();
	void SetDirectionAngle(double angle);

	void SetSpeedX(double speedX);
	void SetSpeedY(double speedY);

	std::shared_ptr<StgMovePattern> GetPattern() { return pattern_; }
	void SetPattern(std::shared_ptr<StgMovePattern> pattern) {
		pattern_ = pattern;
	}
	void AddPattern(int frameDelay, std::shared_ptr<StgMovePattern> pattern, bool bForceMap = false);
};

/**********************************************************
//StgMovePattern
**********************************************************/
class StgMovePattern {
	friend StgMoveObject;
public:
	enum {
		TYPE_OTHER,
		TYPE_ANGLE,
		TYPE_XY,
		TYPE_LINE,

		NO_CHANGE = -0x1000000,
		TOPLAYER_CHANGE = 0x1000000,
		SET_ZERO = -1,
	};
protected:
	int typeMove_;
	StgMoveObject* target_;

	int frameWork_;
	int idShotData_;

	double c_;
	double s_;

	std::list<std::pair<int8_t, double>> listCommand_;

	StgStageController* _GetStageController() { return target_->stageController_; }
	shared_ptr<StgMoveObject> _GetMoveObject(int id);
	virtual void _Activate(StgMovePattern* src) {}
public:
	StgMovePattern(StgMoveObject* target);
	virtual ~StgMovePattern() {}
	virtual void Move() = 0;

	void AddCommand(std::pair<uint8_t, double> cmd) { listCommand_.push_back(cmd); }
	int GetType() { return typeMove_; }

	virtual inline double GetSpeed() = 0;
	virtual inline double GetDirectionAngle() = 0;
	int GetShotDataID() { return idShotData_; }
	void SetShotDataID(int id) { idShotData_ = id; }

	virtual double GetSpeedX() { return c_; }
	virtual double GetSpeedY() { return s_; }
};

class StgMovePattern_XY;
class StgMovePattern_Angle : public StgMovePattern {
	friend class StgMoveObject;
	friend class StgMovePattern_XY;
public:
	enum : int8_t {
		SET_SPEED,
		SET_ANGLE,
		SET_ACCEL,
		SET_AGVEL,
		SET_SPMAX,
		ADD_SPEED,
		ADD_ANGLE,
		ADD_ACCEL,
		ADD_AGVEL,
		ADD_SPMAX,
	};
protected:
	double speed_;
	double angDirection_;
	double acceleration_;
	double maxSpeed_;
	double angularVelocity_;
	weak_ptr<StgMoveObject> idRelative_;

	virtual void _Activate(StgMovePattern* src);
public:
	StgMovePattern_Angle(StgMoveObject* target);
	virtual void Move();

	virtual inline double GetSpeed() { return speed_; }
	virtual inline double GetDirectionAngle() { return angDirection_; }

	void SetSpeed(double speed) { speed_ = speed; }
	void SetDirectionAngle(double angle);
	void SetAcceleration(double accel) { acceleration_ = accel; }
	void SetMaxSpeed(double max) { maxSpeed_ = max; }
	void SetAngularVelocity(double av) { angularVelocity_ = av; }
	void SetRelativeObjectID(shared_ptr<StgMoveObject> obj) { idRelative_ = obj; }
	void SetRelativeObjectID(int id) { idRelative_ = _GetMoveObject(id); }

	virtual inline double GetSpeedX() {
		return (speed_ * c_);
	}
	virtual inline double GetSpeedY() {
		return (speed_ * s_);
	}
};

class StgMovePattern_XY : public StgMovePattern {
	friend class StgMoveObject;
	friend class StgMovePattern_Angle;
public:
	enum : int8_t {
		SET_S_X,
		SET_S_Y,
		SET_A_X,
		SET_A_Y,
		SET_M_X,
		SET_M_Y,
	};
protected:
	double accelerationX_;
	double accelerationY_;
	double maxSpeedX_;
	double maxSpeedY_;

	virtual void _Activate(StgMovePattern* src);
public:
	StgMovePattern_XY(StgMoveObject* target);
	virtual void Move();

	virtual inline double GetSpeed() { return hypot(c_, s_); }
	virtual inline double GetDirectionAngle() { return atan2(s_, c_); }

	virtual double GetSpeedX() { return c_; }
	virtual double GetSpeedY() { return s_; }
	void SetSpeedX(double value) { c_ = value; }
	void SetSpeedY(double value) { s_ = value; }
	void SetAccelerationX(double value) { accelerationX_ = value; }
	void SetAccelerationY(double value) { accelerationY_ = value; }
	void SetMaxSpeedX(double value) { maxSpeedX_ = value; }
	void SetMaxSpeedY(double value) { maxSpeedY_ = value; }

	double GetAccelerationX() { return accelerationX_; }
	double GetAccelerationY() { return accelerationY_; }
	double GetMaxSpeedX() { return maxSpeedX_; }
	double GetMaxSpeedY() { return maxSpeedY_; }
};

class StgMovePattern_Line : public StgMovePattern {
	friend class StgMoveObject;
protected:
	enum {
		TYPE_SPEED,
		TYPE_FRAME,
		TYPE_WEIGHT,
		TYPE_NONE,
	};

	int typeLine_;
	int maxFrame_;
	double speed_;
	double angDirection_;
	
	D3DXVECTOR2 iniPos_;
	D3DXVECTOR2 targetPos_;
public:
	StgMovePattern_Line(StgMoveObject* target);

	virtual void Move();

	virtual inline double GetSpeed() { return speed_; }
	virtual inline double GetDirectionAngle() { return angDirection_; }

	virtual double GetSpeedX() { return c_; }
	virtual double GetSpeedY() { return s_; }
};
class StgMovePattern_Line_Speed : public StgMovePattern_Line {
	friend class StgMoveObject;
public:
	StgMovePattern_Line_Speed(StgMoveObject* target);

	void SetAtSpeed(float tx, float ty, double speed);
};
class StgMovePattern_Line_Frame : public StgMovePattern_Line {
	friend class StgMoveObject;
public:
	typedef float (*lerp_func)(float, float, float);
protected:
	D3DXVECTOR2 lastPos_;
	lerp_func moveLerpFunc;
public:
	StgMovePattern_Line_Frame(StgMoveObject* target);

	virtual void Move();

	void SetAtFrame(float tx, float ty, int frame, lerp_func lerpFunc);
};
class StgMovePattern_Line_Weight : public StgMovePattern_Line {
	friend class StgMoveObject;
protected:
	double dist_;
	double weight_;
	double maxSpeed_;
public:
	StgMovePattern_Line_Weight(StgMoveObject* target);

	virtual void Move();

	void SetAtWeight(float tx, float ty, double weight, double maxSpeed);
};
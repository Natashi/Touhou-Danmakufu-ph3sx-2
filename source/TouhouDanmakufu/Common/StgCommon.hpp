#pragma once

#include "../../GcLib/pch.h"

#include "StgObjectBase.hpp"

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

//*******************************************************************
//StgMoveObject
//*******************************************************************
class StgMoveObject : public StgObjectBase {
	friend StgMovePattern;
protected:
	unique_ptr<StgMovePattern> pattern_;
	
	int frameMove_;

	uint32_t framePattern_;
	std::map<uint32_t, std::list<unique_ptr<StgMovePattern>>> scheduledPatterns_;

	void _Move();
	void AttachPattern(unique_ptr<StgMovePattern> pattern);
public:
	Math::DVec2 position;

	bool enableMovement;
public:
	StgMoveObject(StgStageController* stageController);
	virtual ~StgMoveObject() = default;

	virtual void Copy(StgMoveObject* src);

	double GetSpeed();
	void SetSpeed(double speed);
	double GetDirectionAngle();
	void SetDirectionAngle(double angle);

	void SetSpeedX(double speedX);
	void SetSpeedY(double speedY);

	StgMovePattern* GetPattern() { return pattern_.get(); }
	void SetPattern(unique_ptr<StgMovePattern> pattern) {
		pattern_ = MOVE(pattern);
	}
	
	void AddPattern(uint32_t frameDelay,
		unique_ptr<StgMovePattern> pattern,
		bool mustSchedule = false);
	
	int GetMoveFrame() const { return frameMove_; }
};

//*******************************************************************
//StgMovePattern
//*******************************************************************
enum class MovePatternType {
	None,

	Angle,
	XY,
	XY_WithAngle,
	Line,
	Item,

	Other = -1,
};

class StgMovePattern {
	friend StgMoveObject;
public:
	enum {
		NO_CHANGE = -0x1000000,
		TOPLAYER_CHANGE = 0x1000000,
		UNCAPPED = TOPLAYER_CHANGE,
		SET_ZERO = -1,
	};
protected:
	StgMoveObject* target_;

	uint32_t frameWork_;

	double c_;
	double s_;
	double direction_;

	std::list<std::pair<int8_t, double>> listCommand_;

	StgStageController* _GetStageController() { return target_->GetStageController(); }
	ref_unsync_ptr<StgMoveObject> _GetMoveObject(int id);
	void _RegisterShotDataID();
public:
	int shotDataId;
public:
	StgMovePattern(StgMoveObject* target);
	virtual ~StgMovePattern() = default;
	
	virtual StgMovePattern* Clone() const = 0;

	virtual void Activate(StgMovePattern* src) = 0;
	virtual void Move() = 0;

	virtual MovePatternType GetType() const { return MovePatternType::None; }

	void AddCommand(uint8_t command, double arg) {
		listCommand_.push_back({ command, arg });
	}
	void AddCommandChecked(uint8_t command, double arg) {
		if (static_cast<int>(arg) != NO_CHANGE)
			listCommand_.push_back({ command, arg });
	}
	void AddCommandChecked(uint8_t command, double argCheck, double arg) {
		if (static_cast<int>(argCheck) != NO_CHANGE)
			listCommand_.push_back({ command, arg });
	}

	virtual double GetSpeed() const = 0;
	virtual double GetDirectionAngle() const { return direction_; }

	virtual double GetSpeedX() const { return c_; }
	virtual double GetSpeedY() const { return s_; }
};

class StgMovePattern_Angle;
class StgMovePattern_XY;
class StgMovePattern_XY_Angle;
class StgMovePattern_Line;

class StgMovePattern_Angle : public StgMovePattern {
	friend StgMoveObject;
	friend StgMovePattern_XY;
	friend StgMovePattern_XY_Angle;
public:
	enum : int8_t {
		SET_SPEED,
		SET_ANGLE,
		SET_ACCEL,
		SET_AGVEL,
		SET_SPMAX,
		SET_SPMAX2,
		SET_AGACC,
		SET_AGMAX,
		ADD_SPEED,
		ADD_ANGLE,
		ADD_ACCEL,
		ADD_AGVEL,
		ADD_SPMAX,
		ADD_AGACC,
		ADD_AGMAX
	};
public:
	double speed;
	double acceleration;
	double maxSpeed;
	double angularVelocity;
	double angularAcceleration;
	double angularMaxVelocity;
protected:
	ref_unsync_weak_ptr<StgMoveObject> objRelative_;
public:
	StgMovePattern_Angle(StgMoveObject* target);

	StgMovePattern* Clone() const override {
		return new StgMovePattern_Angle(*this);
	}

	void Activate(StgMovePattern* src) override;
	void Move() override;

	MovePatternType GetType() const override { return MovePatternType::Angle; }

	double GetSpeed() const override { return speed; }
	
	void SetDirectionAngle(double angle);

	void SetRelativeObject(ref_unsync_weak_ptr<StgMoveObject> obj) { objRelative_ = obj; }
	void SetRelativeObject(int id) { objRelative_ = _GetMoveObject(id); }

	double GetSpeedX() const override { return speed * c_; }
	double GetSpeedY() const override { return speed * s_; }
};

class StgMovePattern_XY : public StgMovePattern {
	friend StgMoveObject;
	friend StgMovePattern_Angle;
	friend StgMovePattern_XY_Angle;
public:
	enum : int8_t {
		SET_S_X,
		SET_S_Y,
		SET_A_X,
		SET_A_Y,
		SET_M_X,
		SET_M_Y,
	};
public:
	double accelerationX;
	double accelerationY;
	double maxSpeedX;
	double maxSpeedY;
public:
	StgMovePattern_XY(StgMoveObject* target);

	StgMovePattern* Clone() const override {
		return new StgMovePattern_XY(*this);
	}

	void Activate(StgMovePattern* src) override;
	void Move() override;

	MovePatternType GetType() const override { return MovePatternType::XY; }

	double GetSpeed() const override { 
		return hypot(c_, s_);
	}
	double GetDirectionAngle() const override {
		return c_ != 0 || s_ != 0
			? atan2(s_, c_)
			: direction_;
	}

	double GetSpeedX() const override { return c_; }
	double GetSpeedY() const override { return s_; }
	
	void SetSpeedX(double x) { c_ = x; }
	void SetSpeedY(double y) { s_ = y; }
	virtual void SetSpeedXY(double x, double y) {
		SetSpeedX(x);
		SetSpeedY(x);
	}

	static double GetDirectionSignRelative(double baseAngle, double sx, double sy);
};

class StgMovePattern_XY_Angle : public StgMovePattern_XY {
	friend StgMoveObject;
	friend StgMovePattern_Angle;
public:
	enum : int8_t {
		SET_S_X,
		SET_S_Y,
		SET_A_X,
		SET_A_Y,
		SET_M_X,
		SET_M_Y,
		SET_ANGLE,
		SET_AGVEL,
		SET_AGACC,
		SET_AGMAX,
	};
public:
	double angOffset;
	double angOffsetVelocity;
	double angOffsetAcceleration;
	double angOffsetMaxVelocity;
public:
	StgMovePattern_XY_Angle(StgMoveObject* target);

	StgMovePattern* Clone() const override {
		return new StgMovePattern_XY_Angle(*this);
	}

	void Activate(StgMovePattern* src) override;
	void Move() override;
	
	MovePatternType GetType() const override { return MovePatternType::XY_WithAngle; }

	double GetDirectionAngle() const override {
		return c_ != 0 || s_ != 0
			? atan2(s_, c_) + angOffset
			: direction_;
	}

	double GetSpeedX() const override { 
		return c_ * cos(angOffset) - s_ * sin(angOffset);
	}
	double GetSpeedY() const override { 
		return c_ * sin(angOffset) + s_ * cos(angOffset);
	}

	void SetSpeedXY(double x, double y) override {	// For proper de-rotation
		double c = cos(-angOffset), s = sin(-angOffset);
		c_ = x * c - y * s;
		s_ = x * s + y * c;
	}
};

class StgMovePattern_Line : public StgMovePattern {
	friend StgMoveObject;
public:
	enum : int8_t {
		SET_DX,
		SET_DY,
		SET_SP,
		SET_FR,
		SET_WG,
		SET_MS,
		SET_LP,
	};

	enum class LineType {
		Speed,
		Frame,
		Weight,
		Other,
	};
protected:
	uint32_t maxFrame_;
	double speed_;
	
	Math::DVec2 iniPos_;
	Math::DVec2 targetPos_;
public:
	StgMovePattern_Line(StgMoveObject* target);

	void Activate(StgMovePattern* src) override;
	void Move() override;

	MovePatternType GetType() const override { return MovePatternType::Line; }
	virtual LineType GetLineType() const = 0;

	double GetSpeed() const override { return speed_; }
	// virtual inline double GetDirectionAngle() { return angDirection_; }

	double GetSpeedX() const override { return speed_ * c_; }
	double GetSpeedY() const override { return speed_ * s_; }
};

class StgMovePattern_Line_Speed : public StgMovePattern_Line {
	friend StgMoveObject;
public:
	StgMovePattern_Line_Speed(StgMoveObject* target);

	StgMovePattern* Clone() const override {
		return new StgMovePattern_Line_Speed(*this);
	}

	LineType GetLineType() const override { return LineType::Speed; }

	void SetAtSpeed(double tx, double ty, double speed);
};

class StgMovePattern_Line_Frame : public StgMovePattern_Line {
	friend StgMoveObject;
public:
	using lerp_func = Math::Lerp::funcLerp<double, double>;
	using lerp_diff_func = Math::Lerp::funcLerpDiff<double>;
protected:
	double speedRate_;
	lerp_func moveLerpFunc;
	lerp_diff_func diffLerpFunc;
public:
	StgMovePattern_Line_Frame(StgMoveObject* target);

	StgMovePattern* Clone() const override {
		return new StgMovePattern_Line_Frame(*this);
	}

	void Move() override;
	
	LineType GetLineType() const override { return LineType::Frame; }

	void SetAtFrame(double tx, double ty, uint32_t frame, lerp_func lerpFunc, lerp_diff_func diffFunc);
};

class StgMovePattern_Line_Weight : public StgMovePattern_Line {
	friend StgMoveObject;
protected:
	double dist_;
	double weight_;
	double maxSpeed_;
public:
	StgMovePattern_Line_Weight(StgMoveObject* target);

	StgMovePattern* Clone() const override {
		return new StgMovePattern_Line_Weight(*this);
	}

	void Move() override;

	LineType GetLineType() const override { return LineType::Weight; }

	void SetAtWeight(double tx, double ty, double weight, double maxSpeed);
};
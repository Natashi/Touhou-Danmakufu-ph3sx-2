#pragma once
#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgStageScript.hpp"
#include "StgIntersection.hpp"

class StgPlayerObject;
class StgPlayerInformation;
class StgPlayerSpellManageObject;
class StgPlayerSpellObject;

//*******************************************************************
//StgPlayerObject
//*******************************************************************
class StgPlayerInformation {
	friend StgPlayerObject;
private:
	double life_;
	double countBomb_;
	double power_;
	int frameRebirth_;
public:
	StgPlayerInformation() {}
	virtual ~StgPlayerInformation() {}

	double GetLife() { return life_; }
	void SetLife(double life) { life_ = life; }
	double GetSpell() { return countBomb_; }
	void SetSpell(double spell) { countBomb_ = spell; }
	double GetPower() { return power_; }
	void SetPower(double power) { power_ = power; }

	int GetRebirthFrame() { return frameRebirth_; }
	void SetRebirthFrame(int frame) { frameRebirth_ = frame; }
};

class StgPlayerObject : public DxScriptSpriteObject2D, public StgMoveObject, public StgIntersectionObject {
public:
	enum {
		STATE_NORMAL,
		STATE_HIT,
		STATE_DOWN,
		STATE_END,

		REBIRTH_DEFAULT = -99999,
	};
protected:
	StgStagePlayerScript* script_;

	ref_count_ptr<StgPlayerInformation> infoPlayer_;
	ref_unsync_ptr<StgPlayerSpellManageObject> objSpell_;

	double speedFast_;
	double speedSlow_;
	DxRect<LONG> rcClip_;

	int state_;
	int frameState_;
	int frameRebirthMax_;	//Maximum deathbomb frame
	int frameRebirthDiff_;	//Deathbomb frame reduction per hit
	int frameStateDown_;

	std::list<ref_unsync_weak_ptr<StgIntersectionObject>> listGrazedShot_;
	int hitObjectID_;

	double itemCircle_;
	int frameInvincibility_;
	bool bForbidShot_;
	bool bForbidSpell_;
	int yAutoItemCollect_;

	bool enableStateEnd_;
	bool enableGrazeInvincible_;
	bool enableShootdownEvent_;
	bool enableDeleteShotOnHit_;

	double rebirthX_;
	double rebirthY_;

	void _InitializeRebirth();
	void _Move();
	void _AddIntersection();
	bool _IsValidSpell();
public:
	StgPlayerObject(StgStageController* stageController);
	virtual ~StgPlayerObject();

	virtual void Clone(DxScriptObjectBase* src);

	void Clear() { ClearIntersectionRelativeTarget(); }

	void SetScript(StgStagePlayerScript* script) { script_ = script; }

	virtual void Work();
	void Move();
	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);
	void CallSpell();
	void KillSelf(int hitObj);

	void SendGrazeEvent();

	virtual void SetX(double x) { posX_ = x; DxScriptRenderObject::SetX(x); }
	virtual void SetY(double y) { posY_ = y; DxScriptRenderObject::SetY(y); }

	ref_count_ptr<StgPlayerInformation> GetPlayerInformation() { return infoPlayer_; }
	void SetPlayerInforamtion(ref_count_ptr<StgPlayerInformation> info) { infoPlayer_ = info; }
	ref_unsync_ptr<StgPlayerSpellManageObject> GetSpellManageObject() { return objSpell_; }

	StgStagePlayerScript* GetPlayerScript() { return script_; }
	ref_unsync_ptr<StgPlayerObject> GetOwnObject();
	double GetX() { return posX_; }
	double GetY() { return posY_; }
	double GetFastSpeed() { return speedFast_; }
	void SetFastSpeed(double speed) { speedFast_ = speed; }
	double GetSlowSpeed() { return speedSlow_; }
	void SetSlowSpeed(double speed) { speedSlow_ = speed; }

	DxRect<LONG>* GetClip() { return &rcClip_; }
	void SetClip(const DxRect<LONG>& rect) { rcClip_ = rect; }

	int GetState() { return state_; }
	int GetDownStateFrame() { return frameStateDown_; }
	void SetDownStateFrame(int frame) { frameStateDown_ = frame; }
	int GetRebirthFrame() { return infoPlayer_->GetRebirthFrame(); }
	void SetRebirthFrameMax(int frame) { frameRebirthMax_ = frame; }
	void SetRebirthFrame(int frame) { infoPlayer_->SetRebirthFrame(frame); }
	void SetRebirthLossFrame(int frame) { frameRebirthDiff_ = frame; }
	double GetItemIntersectionRadius() { return itemCircle_; }
	void SetItemIntersectionRadius(double r) { itemCircle_ = r; }
	double GetLife() { return infoPlayer_->GetLife(); }
	void SetLife(double life) { infoPlayer_->SetLife(life); }
	double GetSpell() { return infoPlayer_->GetSpell(); }
	void SetSpell(double spell) { infoPlayer_->SetSpell(spell); }
	double GetPower() { return infoPlayer_->GetPower(); }
	void SetPower(double power) { infoPlayer_->SetPower(power); }
	int GetInvincibilityFrame() { return frameInvincibility_; }
	void SetInvincibilityFrame(int frame) { frameInvincibility_ = frame; }
	int GetAutoItemCollectY() { return yAutoItemCollect_; }
	void SetAutoItemCollectY(int y) { yAutoItemCollect_ = y; }

	void SetRebirthPosition(double x, double y) { rebirthX_ = x; rebirthY_ = y; }

	bool IsPermitShot();
	void SetForbidShot(bool bForbid) { bForbidShot_ = bForbid; }
	bool IsPermitSpell();
	void SetForbidSpell(bool bForbid) { bForbidSpell_ = bForbid; }
	bool IsWaitLastSpell();

	void SetEnableInvincibleGraze(bool bEnable) { enableGrazeInvincible_ = bEnable; }
	void SetDeleteShotOnHit(bool bEnable) { enableDeleteShotOnHit_ = bEnable; }
	void SetEnableStateEnd(bool bEnable) { enableStateEnd_ = bEnable; }
	void SetEnableShootdownEvent(bool bEnable) { enableShootdownEvent_ = bEnable; }
};


class StgIntersectionTarget_Player : public StgIntersectionTarget_Circle {
	bool bGraze_;
public:
	StgIntersectionTarget_Player(bool bGraze) { typeTarget_ = TYPE_PLAYER; bGraze_ = bGraze; }
	
	bool IsGraze() { return bGraze_; }
};

//*******************************************************************
//StgPlayerSpellManageObject
//*******************************************************************
class StgPlayerSpellManageObject : public DxScriptObjectBase, public StgObjectBase {
public:
	StgPlayerSpellManageObject(StgStageController* stageController) : StgObjectBase(stageController) {
		bVisible_ = false;
	}
	
	virtual void Render() {}
	virtual void SetRenderState() {}
};

//*******************************************************************
//StgPlayerSpellObject
//*******************************************************************
class StgPlayerSpellObject : public DxScriptPrimitiveObject2D, public StgObjectBase, public StgIntersectionObject {
protected:
	double damage_;
	bool bEraseShot_;
	double life_;
public:
	StgPlayerSpellObject(StgStageController* stageController);

	virtual void Clone(DxScriptObjectBase* src);
	
	virtual void Work();
	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);

	double GetDamage() { return damage_; }
	void SetDamage(double damage) { damage_ = damage; }
	bool IsEraseShot() { return bEraseShot_; }
	void SetEraseShot(bool b) { bEraseShot_ = b; }
	double GetLife() { return life_; }
	void SetLife(double life) { life_ = life; }
};
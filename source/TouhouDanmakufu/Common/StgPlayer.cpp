#include "source/GcLib/pch.h"

#include "StgPlayer.hpp"
#include "StgSystem.hpp"

//*******************************************************************
//StgPlayerObject
//*******************************************************************
StgPlayerObject::StgPlayerObject(StgStageController* stageController) : StgMoveObject(stageController) {
	stageController_ = stageController;
	typeObject_ = TypeObject::Player;

	infoPlayer_ = new StgPlayerInformation();

	SetRenderPriorityI(30);
	speedFast_ = 4;
	speedSlow_ = 1.6;
	{
		DxRect<LONG>* rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();
		LONG stgWidth = rcStgFrame->right - rcStgFrame->left;
		LONG stgHeight = rcStgFrame->bottom - rcStgFrame->top;
		rcClip_.Set(0, 0, stgWidth, stgHeight);
	}

	state_ = STATE_NORMAL;
	frameStateDown_ = 120;
	frameRebirthMax_ = 15;
	infoPlayer_->frameRebirth_ = frameRebirthMax_;
	frameRebirthDiff_ = 0;

	infoPlayer_->life_ = 2;
	infoPlayer_->countBomb_ = 3;
	infoPlayer_->power_ = 1.0;

	itemCircle_ = 24;
	frameInvincibility_ = 0;
	bForbidShot_ = false;
	bForbidSpell_ = false;
	yAutoItemCollect_ = -256 * 256;

	enableGrazeInvincible_ = true;
	enableStateEnd_ = true;
	enableShootdownEvent_ = true;
	enableDeleteShotOnHit_ = true;

	rebirthX_ = REBIRTH_DEFAULT;
	rebirthY_ = REBIRTH_DEFAULT;

	hitObjectID_ = DxScript::ID_INVALID;

	_InitializeRebirth();
}
StgPlayerObject::~StgPlayerObject() {}
void StgPlayerObject::_InitializeRebirth() {
	DxRect<LONG>* rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();

	SetX((rebirthX_ == REBIRTH_DEFAULT) ? (rcStgFrame->right - rcStgFrame->left) / 2.0 : rebirthX_);
	SetY((rebirthY_ == REBIRTH_DEFAULT) ? rcStgFrame->bottom - 48 : rebirthY_);
}
void StgPlayerObject::Work() {
	EDirectInput* input = EDirectInput::GetInstance();
	auto scriptManager = stageController_->GetScriptManager();
	StgEnemyManager* enemyManager = stageController_->GetEnemyManager();

	//Refresh player intersection
	ClearIntersected();

	switch (state_) {
	case STATE_NORMAL:
	{
		//_Move();
		if (input->GetVirtualKeyState(EDirectInput::KEY_BOMB) == KEY_PUSH)
			CallSpell();

		_AddIntersection();
		break;
	}
	case STATE_HIT:
		//Check deathbomb
		if (input->GetVirtualKeyState(EDirectInput::KEY_BOMB) == KEY_PUSH)
			CallSpell();

		frameState_--;
		if (frameState_ < 0) {
			//Player is killed

			bool bEnemyLastSpell = false;

			ref_unsync_ptr<StgEnemyBossSceneObject> objBossScene = enemyManager->GetBossSceneObject();
			if (objBossScene) {
				objBossScene->AddPlayerShootDownCount();
				if (objBossScene->GetActiveData()->IsLastSpell())
					bEnemyLastSpell = true;
			}
			if (!bEnemyLastSpell)
				infoPlayer_->life_--;

			//Reset deathbomb frame
			infoPlayer_->frameRebirth_ = frameRebirthMax_;
			if (enableShootdownEvent_)
				scriptManager->RequestEventAll(StgStagePlayerScript::EV_PLAYER_SHOOTDOWN);

			if (infoPlayer_->life_ >= 0 || !enableStateEnd_) {
				bVisible_ = false;
				state_ = STATE_DOWN;
				frameState_ = frameStateDown_;
			}
			else {
				state_ = STATE_END;
			}

			//Also prevents STATE_END and STATE_DOWN
			if (!enableShootdownEvent_) {
				frameState_ = 0;
				bVisible_ = true;
				_InitializeRebirth();
				state_ = STATE_NORMAL;
			}
		}
		break;
	case STATE_DOWN:
		frameState_--;
		if (frameState_ <= 0) {
			bVisible_ = true;
			_InitializeRebirth();
			state_ = STATE_NORMAL;
			scriptManager->RequestEventAll(StgStageScript::EV_PLAYER_REBIRTH);
		}
		break;
	case STATE_END:
		bVisible_ = false;
		break;
	}

	--frameInvincibility_;
	frameInvincibility_ = std::max(frameInvincibility_, 0);
	hitObjectID_ = DxScript::ID_INVALID;
}
void StgPlayerObject::Move() {
	if (state_ == STATE_NORMAL && bEnableMovement_) {
		++frameMove_;
		_Move();
	}
	else {
		SetSpeed(0);
	}

	//Just in case the player is the parent of anything
	if (listOwnedParent_.size() > 0) {
		for (auto& iPar : listOwnedParent_) {
			if (iPar) iPar->UpdateChildren();
		}
	}
}
void StgPlayerObject::_Move() {
	EDirectInput* input = EDirectInput::GetInstance();
	DIKeyState keyLeft = input->GetVirtualKeyState(EDirectInput::KEY_LEFT);
	DIKeyState keyRight = input->GetVirtualKeyState(EDirectInput::KEY_RIGHT);
	DIKeyState keyUp = input->GetVirtualKeyState(EDirectInput::KEY_UP);
	DIKeyState keyDown = input->GetVirtualKeyState(EDirectInput::KEY_DOWN);
	DIKeyState keySlow = input->GetVirtualKeyState(EDirectInput::KEY_SLOWMOVE);

	double sx = 0;
	double sy = 0;
	double speed = keySlow == KEY_PUSH || keySlow == KEY_HOLD ? speedSlow_ : speedFast_;

	bool bKeyLeft = keyLeft == KEY_PUSH || keyLeft == KEY_HOLD;
	bool bKeyRight = keyRight == KEY_PUSH || keyRight == KEY_HOLD;
	bool bKeyUp = keyUp == KEY_PUSH || keyUp == KEY_HOLD;
	bool bKeyDown = keyDown == KEY_PUSH || keyDown == KEY_HOLD;
	if (bKeyLeft && !bKeyRight) sx -= speed;
	if (!bKeyLeft && bKeyRight) sx += speed;
	if (bKeyUp && !bKeyDown) sy -= speed;
	if (!bKeyUp && bKeyDown) sy += speed;

	SetSpeed(speed);
	if (sx != 0 && sy != 0) {
		constexpr double diagFactor = 1.0 / gstd::GM_SQRT2;
		sx *= diagFactor;
		sy *= diagFactor;
		if (sx > 0) SetDirectionAngle(sy > 0 ? Math::DegreeToRadian(45) : Math::DegreeToRadian(315));
		else SetDirectionAngle(sy > 0 ? Math::DegreeToRadian(135) : Math::DegreeToRadian(225));
	}
	else if (sx != 0 || sy != 0) {
		if (sx != 0) SetDirectionAngle(sx > 0 ? Math::DegreeToRadian(0) : Math::DegreeToRadian(180));
		else if (sy != 0) SetDirectionAngle(sy > 0 ? Math::DegreeToRadian(90) : Math::DegreeToRadian(270));
	}
	else {
		SetSpeed(0);
	}

	//Add and clip player position
	{
		double px = posX_ + sx;
		double py = posY_ + sy;
		SetX(std::clamp<double>(px, rcClip_.left, rcClip_.right));
		SetY(std::clamp<double>(py, rcClip_.top, rcClip_.bottom));
	}
}
void StgPlayerObject::_AddIntersection() {
	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();

	UpdateIntersectionRelativeTarget(posX_, posY_, 0);
	RegistIntersectionRelativeTarget(intersectionManager);
}
bool StgPlayerObject::_IsValidSpell() {
	bool res = true;
	res &= (state_ == STATE_NORMAL) || (state_ == STATE_HIT && frameState_ > 0);	//Normal or deathbombing -> yes
	res &= (objSpell_ == nullptr) || (objSpell_->IsDeleted());		//A spell is already active -> no
	return res;
}
void StgPlayerObject::CallSpell() {
	if (!_IsValidSpell()) return;
	if (!IsPermitSpell()) return;

	auto objectManager = stageController_->GetMainObjectManager();
	objSpell_ = new StgPlayerSpellManageObject();
	int idSpell = objectManager->AddObject(objSpell_);

	gstd::value vUse = script_->RequestEvent(StgStagePlayerScript::EV_REQUEST_SPELL);
	if (!StgStagePlayerScript::IsBooleanValue(vUse))
		throw gstd::wexception(L"@Event(EV_REQUEST_SPELL) must return a boolean value.");
	else if (!vUse.as_boolean()) {
		objectManager->DeleteObject(objSpell_);
		objSpell_ = nullptr;
		return;
	}

	//Restore state from deathbombing to normal
	if (state_ == STATE_HIT) {
		state_ = STATE_NORMAL;
		infoPlayer_->frameRebirth_ = std::max(infoPlayer_->frameRebirth_ - frameRebirthDiff_, 0);
	}

	StgEnemyManager* enemyManager = stageController_->GetEnemyManager();
	ref_unsync_ptr<StgEnemyBossSceneObject> objBossScene = enemyManager->GetBossSceneObject();
	if (objBossScene) objBossScene->AddPlayerSpellCount();

	auto scriptManager = stageController_->GetScriptManager();
	scriptManager->RequestEventAll(StgStageScript::EV_PLAYER_SPELL);
}
void StgPlayerObject::KillSelf(int hitObj) {
	if (state_ == STATE_NORMAL && frameInvincibility_ <= 0) {
		state_ = STATE_HIT;
		frameState_ = infoPlayer_->frameRebirth_;

		gstd::value valueHitObjectID = script_->CreateIntValue(hitObj);
		script_->RequestEvent(StgStagePlayerScript::EV_HIT, &valueHitObjectID, 1);
	}
}
void StgPlayerObject::SendGrazeEvent() {
	if (listGrazedShot_.size() == 0U) return;

	std::vector<value> listValPos;
	std::vector<int> listShotID;

	stageController_->GetStageInformation()->AddGraze(listGrazedShot_.size());

	size_t iValidGraze = 0;
	for (auto iObj = listGrazedShot_.begin(); iObj != listGrazedShot_.end(); ++iObj) {
		if (auto& wObj = *iObj) {
			//No need to check for a nullptr, listGrazedShot_ only contains StgShotObject* anyway
			StgShotObject* objShot = dynamic_cast<StgShotObject*>(wObj.get());
			if (!objShot->IsDeleted()) {
				double listShotPos[2] = { objShot->GetPositionX(), objShot->GetPositionY() };
				listValPos.push_back(script_->CreateRealArrayValue(listShotPos, 2U));

				listShotID.push_back(objShot->GetObjectID());

				++iValidGraze;
			}
		}
	}
	listGrazedShot_.clear();

	if (iValidGraze == 0) return;

	value listScriptValue[3];
	listScriptValue[0] = script_->CreateIntValue(iValidGraze);
	listScriptValue[1] = script_->CreateIntArrayValue(listShotID);
	listScriptValue[2] = script_->CreateValueArrayValue(listValPos);
	script_->RequestEvent(StgStagePlayerScript::EV_GRAZE, listScriptValue, 3);

	auto stageScriptManager = stageController_->GetScriptManager();
	LOCK_WEAK(itemScript, stageScriptManager->GetItemScript())
		itemScript->RequestEvent(StgStagePlayerScript::EV_GRAZE, listScriptValue, 3);
}

void StgPlayerObject::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	StgIntersectionTarget_Player* tPlayer = dynamic_cast<StgIntersectionTarget_Player*>(ownTarget);
	if (tPlayer == nullptr) return;

	auto wObj = otherTarget->GetObject();
	switch (otherTarget->GetTargetType()) {
	case StgIntersectionTarget::TYPE_ENEMY_SHOT:
	{
		auto objShot = dynamic_cast<StgShotObject*>(wObj.get());
		if (!tPlayer->IsGraze()) {
			int hitID = DxScript::ID_INVALID;
			if (objShot) {
				hitID = objShot->GetObjectID();
				KillSelf(hitID);

				if (enableDeleteShotOnHit_ && !objShot->IsSpellResist() &&
					objShot->GetObjectType() == TypeObject::Shot)
					objShot->DeleteImmediate();
			}
		}
		else {
			if (objShot) {
				if (objShot->IsValidGraze() && (enableGrazeInvincible_ || frameInvincibility_ <= 0))
					listGrazedShot_.push_back(wObj);
			}
		}
		break;
	}
	case StgIntersectionTarget::TYPE_ENEMY:
	{
		if (!tPlayer->IsGraze()) {
			if (auto objEnemy = dynamic_cast<StgEnemyObject*>(wObj.get()))
				KillSelf(objEnemy->GetObjectID());
		}
		break;
	}
	}
}
ref_unsync_ptr<StgPlayerObject> StgPlayerObject::GetOwnObject() {
	return ref_unsync_ptr<StgPlayerObject>::Cast(stageController_->GetMainRenderObject(idObject_));
}
bool StgPlayerObject::IsPermitShot() {
	return !bForbidShot_;
}
bool StgPlayerObject::IsPermitSpell() {
	StgEnemyManager* enemyManager = stageController_->GetEnemyManager();
	bool bEnemyLastSpell = false;
	ref_unsync_ptr<StgEnemyBossSceneObject> objBossScene = enemyManager->GetBossSceneObject();
	if (objBossScene) {
		shared_ptr<StgEnemyBossSceneData> data = objBossScene->GetActiveData();
		if (data != nullptr && data->IsLastSpell())
			bEnemyLastSpell = true;
	}

	return !bEnemyLastSpell && !bForbidSpell_;
}
bool StgPlayerObject::IsWaitLastSpell() {
	bool res = IsPermitSpell() && state_ == STATE_HIT;
	return res;
}

//*******************************************************************
//StgPlayerSpellObject
//*******************************************************************
StgPlayerSpellObject::StgPlayerSpellObject(StgStageController* stageController) {
	stageController_ = stageController;
	damage_ = 0;
	bEraseShot_ = true;
	life_ = 256 * 256 * 256;
}
void StgPlayerSpellObject::Work() {
	if (IsDeleted()) return;
	if (life_ <= 0) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgPlayerSpellObject::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	double damage = 0;
	StgIntersectionTarget::Type otherType = otherTarget->GetTargetType();
	switch (otherType) {
	case StgIntersectionTarget::TYPE_ENEMY:
	case StgIntersectionTarget::TYPE_ENEMY_SHOT:
	{
		damage = 1;
		break;
	}
	}
	life_ -= damage;
	life_ = std::max(life_, 0.0);
}



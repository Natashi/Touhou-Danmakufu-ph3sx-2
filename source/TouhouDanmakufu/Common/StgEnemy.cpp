#include "source/GcLib/pch.h"

#include "StgEnemy.hpp"
#include "StgSystem.hpp"

//*******************************************************************
//StgEnemyManager
//*******************************************************************
StgEnemyManager::StgEnemyManager(StgStageController* stageController) {
	stageController_ = stageController;
}
StgEnemyManager::~StgEnemyManager() {
	for (ref_unsync_ptr<StgEnemyObject>& obj : listObj_) {
		if (obj)
			obj->ClearEnemyObject();
	}
}
void StgEnemyManager::Work() {
	for (auto itr = listObj_.begin(); itr != listObj_.end();) {
		ref_unsync_ptr<StgEnemyObject>& obj = (*itr);
		if (obj->IsDeleted()) {
			obj->ClearEnemyObject();
			itr = listObj_.erase(itr);
		}
		else ++itr;
	}
}
void StgEnemyManager::RegistIntersectionTarget() {
	for (ref_unsync_ptr<StgEnemyObject>& obj : listObj_) {
		if (!obj->IsDeleted()) {
			obj->ClearIntersectedIdList();
			obj->RegistIntersectionTarget();
		}
	}
}
void StgEnemyManager::SetBossSceneObject(ref_unsync_ptr<StgEnemyBossSceneObject> obj) {
	if (objBossScene_ != nullptr && !objBossScene_->IsDeleted())
		throw gstd::wexception("ObjEnemyBossScene already exists.");
	objBossScene_ = obj;
}
ref_unsync_ptr<StgEnemyBossSceneObject> StgEnemyManager::GetBossSceneObject() {
	ref_unsync_ptr<StgEnemyBossSceneObject> res;
	if (objBossScene_ != nullptr && !objBossScene_->IsDeleted())
		res = objBossScene_;
	return res;
}

//*******************************************************************
//StgEnemyObject
//*******************************************************************
StgEnemyObject::StgEnemyObject(StgStageController* stageController) : StgMoveObject(stageController) {
	stageController_ = stageController;
	typeObject_ = TypeObject::Enemy;

	SetRenderPriorityI(40);

	life_ = 0;
	rateDamageShot_ = 100;
	rateDamageSpell_ = 100;
	intersectedPlayerShotCount_ = 0U;

	bEnableGetIntersectionPositionFetch_ = true;
}
StgEnemyObject:: ~StgEnemyObject() {
}
void StgEnemyObject::Work() {
	ClearIntersected();
	intersectedPlayerShotCount_ = 0U;

	ptrIntersectionToShot_.clear();
	ptrIntersectionToPlayer_.clear();

	_Move();
}
void StgEnemyObject::_Move() {
	StgMoveObject::_Move();
	SetX(posX_);
	SetY(posY_);
}
void StgEnemyObject::_AddRelativeIntersection() {
	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();

	UpdateIntersectionRelativeTarget(posX_, posY_, 0);
	RegistIntersectionRelativeTarget(intersectionManager);
}
void StgEnemyObject::Activate() {}
void StgEnemyObject::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	double damage = 0;
	if (auto ptrObj = otherTarget->GetObject()) {
		if (otherTarget->GetTargetType() == StgIntersectionTarget::TYPE_PLAYER_SHOT) {
			if (StgShotObject* shot = dynamic_cast<StgShotObject*>(ptrObj.get())) {
				damage = shot->GetDamage() * (shot->IsSpellFactor() ? rateDamageSpell_ : rateDamageShot_) / 100.0;
				++intersectedPlayerShotCount_;
			}
		}
		else if (otherTarget->GetTargetType() == StgIntersectionTarget::TYPE_PLAYER_SPELL) {
			if (StgPlayerSpellObject* spell = dynamic_cast<StgPlayerSpellObject*>(ptrObj.get()))
				damage = spell->GetDamage() * rateDamageSpell_ / 100;
		}
	}
	life_ = std::max(life_ - damage, 0.0);
}
void StgEnemyObject::RegistIntersectionTarget() {
	_AddRelativeIntersection();
}
ref_unsync_ptr<StgEnemyObject> StgEnemyObject::GetOwnObject() {
	return ref_unsync_ptr<StgEnemyObject>::Cast(stageController_->GetMainRenderObject(idObject_));
}
void StgEnemyObject::AddReferenceToShotIntersection(ref_unsync_ptr<StgIntersectionTarget> target) {
	ptrIntersectionToShot_.push_back(target);
}
void StgEnemyObject::AddReferenceToPlayerIntersection(ref_unsync_ptr<StgIntersectionTarget> target) {
	ptrIntersectionToPlayer_.push_back(target);
}

//*******************************************************************
//StgEnemyBossObject
//*******************************************************************
StgEnemyBossObject::StgEnemyBossObject(StgStageController* stageController) : StgEnemyObject(stageController) {
	typeObject_ = TypeObject::EnemyBoss;
}

//*******************************************************************
//StgEnemyBossSceneObject
//*******************************************************************
StgEnemyBossSceneObject::StgEnemyBossSceneObject(StgStageController* stageController) {
	stageController_ = stageController;
	typeObject_ = TypeObject::EnemyBossScene;

	bVisible_ = false;
	bLoad_ = false;
	dataStep_ = 0;
	dataIndex_ = -1;

	bEnableUnloadCache_ = false;
}
StgEnemyBossSceneObject::~StgEnemyBossSceneObject() {
	if (bEnableUnloadCache_) {
		auto pCache = stageController_->GetSystemController()->GetScriptEngineCache();
		for (std::vector<ref_unsync_ptr<StgEnemyBossSceneData>>& iStep : listData_) {
			for (ref_unsync_ptr<StgEnemyBossSceneData>& pData : iStep) {
				pCache->RemoveCache(pData->GetPath());
			}
		}
	}
}
bool StgEnemyBossSceneObject::_NextStep() {
	if (dataStep_ >= listData_.size()) return false;

	auto scriptManager = stageController_->GetScriptManager();

	if (activeData_)
		scriptManager->RequestEventAll(StgStageScript::EV_END_BOSS_STEP);

	dataIndex_++;
	if (dataIndex_ >= listData_[dataStep_].size()) {
		dataIndex_ = 0;
		while (true) {
			dataStep_++;
			if (dataStep_ >= listData_.size()) return false;
			if (listData_[dataStep_].size() > 0) break;
		}
	}

	ref_unsync_ptr<StgEnemyBossSceneData> oldActiveData = activeData_;

	StgStageScriptObjectManager* objectManager = stageController_->GetMainObjectManager();

	activeData_ = listData_[dataStep_][dataIndex_];
	std::vector<ref_unsync_ptr<StgEnemyBossObject>>& listEnemy = activeData_->GetEnemyObjectList();
	std::vector<double>& listLife = activeData_->GetLifeList();
	for (size_t iEnemy = 0; iEnemy < listEnemy.size(); ++iEnemy) {
		ref_unsync_ptr<StgEnemyBossObject> obj = listEnemy[iEnemy];
		obj->SetLife(listLife[iEnemy]);
		if (oldActiveData) {
			std::vector<ref_unsync_ptr<StgEnemyBossObject>>& listOldEnemyObject = oldActiveData->GetEnemyObjectList();
			if (iEnemy < listOldEnemyObject.size()) {
				ref_unsync_ptr<StgEnemyBossObject>& objOld = listOldEnemyObject[iEnemy];
				obj->SetPositionX(objOld->GetPositionX());
				obj->SetPositionY(objOld->GetPositionY());
			}
		}
		objectManager->ActivateObject(obj->GetObjectID(), true);
	}

	LOCK_WEAK(pScript, activeData_->GetScriptPointer()) {
		if (!pScript->IsLoad()) {
			throw gstd::wexception(StringUtility::Format(L"_NextStep: Script wasn't loaded or has been unloaded. [%d, %d]",
				dataStep_, dataIndex_));
		}
		else {
			scriptManager->StartScript(pScript);
		}
	}

	scriptManager->RequestEventAll(StgStageScript::EV_START_BOSS_STEP);

	return true;
}
void StgEnemyBossSceneObject::Work() {
	if (activeData_->IsReadyNext()) {
		bool bEnemyExists = false;
		for (ref_unsync_ptr<StgEnemyBossObject>& iEnemy : activeData_->GetEnemyObjectList())
			bEnemyExists |= !iEnemy->IsDeleted();

		if (!bEnemyExists) {
			bool bNext = _NextStep();
			if (!bNext) {
				StgEnemyManager* enemyManager = stageController_->GetEnemyManager();
				StgStageScriptObjectManager* objectManager = stageController_->GetMainObjectManager();
				objectManager->DeleteObject(idObject_);
				enemyManager->SetBossSceneObject(nullptr);
				return;
			}
		}

	}
	else if (!activeData_->IsReadyNext()) {
		bool bZeroTimer = false;
		int timer = activeData_->GetSpellTimer();
		if (timer > 0) {
			timer--;
			activeData_->SetSpellTimer(timer);
			if (timer == 0) {
				bZeroTimer = true;
			}
		}

		bool bEndLastSpell = false;
		if (activeData_->IsLastSpell()) {
			bEndLastSpell = activeData_->GetPlayerShootDownCount() > 0;
		}

		if (bZeroTimer || bEndLastSpell) {
			for (ref_unsync_ptr<StgEnemyBossObject>& iEnemy : activeData_->GetEnemyObjectList())
				iEnemy->SetLife(0);

			if (bZeroTimer) {
				auto scriptManager = stageController_->GetScriptManager();
				scriptManager->RequestEventAll(StgStageScript::EV_TIMEOUT);
			}
		}

		std::vector<ref_unsync_ptr<StgEnemyBossObject>>& listEnemy = activeData_->GetEnemyObjectList();
		bool bReadyNext = true;
		if (activeData_->IsRequireAllDown()) {
			for (auto itr = listEnemy.begin(); itr != listEnemy.end(); ++itr) {
				if ((*itr)->GetLife() > 0)
					bReadyNext = false;
			}
		}
		else {
			bReadyNext = false;
			for (auto itr = listEnemy.begin(); itr != listEnemy.end(); ++itr) {
				if ((*itr)->GetLife() <= 0) {
					bReadyNext = true;
					break;
				}
			}
		}

		if (bReadyNext) {
			if (activeData_->IsSpellCard()) {
				bool bGain = (activeData_->IsDurable() || activeData_->GetSpellTimer() > 0)
					&& (activeData_->GetPlayerShootDownCount() == 0 && activeData_->GetPlayerSpellCount() == 0);

				if (bGain) {
					auto scriptManager = stageController_->GetScriptManager();
					int64_t score = activeData_->GetCurrentSpellScore();
					scriptManager->RequestEventAll(StgStageScript::EV_GAIN_SPELL);
				}
			}
			activeData_->SetReadyNext();
		}

	}
}
void StgEnemyBossSceneObject::Activate() {
	if (!bLoad_)
		LoadAllScriptInThread();

	auto scriptManager = stageController_->GetScriptManager();
	StgStageScriptObjectManager* objectManager = stageController_->GetMainObjectManager();

	for (std::vector<ref_unsync_ptr<StgEnemyBossSceneData>>& iStep : listData_) {
		size_t iData = 0;
		for (ref_unsync_ptr<StgEnemyBossSceneData>& pData : iStep) {
			shared_ptr<ManagedScript> pScript = pData->GetScriptPointer().lock();

			if (pScript == nullptr)
				throw gstd::wexception(StringUtility::Format(L"Cannot load script: %s", pData->GetPath().c_str()));
			if (!pScript->IsLoad()) {
				DWORD count = 0;
				while (!pScript->IsLoad()) {
					if (count % 1000 == 0) {
						Logger::WriteTop(StringUtility::Format(L"EnemyBossScene: Script is still loading... [%s]",
							pScript->GetPath().c_str()));
					}
					::Sleep(5);
					++count;
				}
			}

			if (stageController_->GetSystemInformation()->IsError()) continue;

			std::vector<double> listLife;
			gstd::value vLife = pScript->RequestEvent(StgStageScript::EV_REQUEST_LIFE);
			if (vLife.has_data()) {
				if (pScript->IsArrayValue(vLife)) {
					for (size_t iLife = 0; iLife < vLife.length_as_array(); ++iLife) {
						double life = vLife.index_as_array(iLife).as_real();
						listLife.push_back(life);
					}
				}
				else {
					double life = vLife.as_real();
					listLife.push_back(life);
				}
			}

			if (listLife.size() == 0)
				throw gstd::wexception(StringUtility::Format(L"EV_REQUEST_LIFE must return a value. (%s)",
					pData->GetPath().c_str()));
			pData->SetLifeList(listLife);

			gstd::value vTimer = pScript->RequestEvent(StgStageScript::EV_REQUEST_TIMER);
			if (vTimer.has_data())
				pData->SetOriginalSpellTimer(vTimer.as_real() * STANDARD_FPS);

			gstd::value vSpell = pScript->RequestEvent(StgStageScript::EV_REQUEST_IS_SPELL);
			if (vSpell.has_data())
				pData->SetSpellCard(vSpell.as_boolean());

			{
				gstd::value vScore = pScript->RequestEvent(StgStageScript::EV_REQUEST_SPELL_SCORE);
				if (vScore.has_data()) pData->SetSpellScore(vScore.as_real());

				gstd::value vLast = pScript->RequestEvent(StgStageScript::EV_REQUEST_IS_LAST_SPELL);
				if (vLast.has_data()) pData->SetLastSpell(vLast.as_boolean());

				gstd::value vDurable = pScript->RequestEvent(StgStageScript::EV_REQUEST_IS_DURABLE_SPELL);
				if (vDurable.has_data()) pData->SetDurable(vDurable.as_boolean());

				gstd::value vAllDown = pScript->RequestEvent(StgStageScript::EV_REQUEST_REQUIRE_ALL_DOWN);
				if (vAllDown.has_data()) pData->SetRequireAllDown(vAllDown.as_boolean());
			}

			std::vector<ref_unsync_ptr<StgEnemyBossObject>> listEnemyObject;
			for (size_t iEnemy = 0; iEnemy < listLife.size(); iEnemy++) {
				ref_unsync_ptr<StgEnemyBossObject> obj = new StgEnemyBossObject(stageController_);
				int idEnemy = objectManager->AddObject(obj, false);
				listEnemyObject.push_back(obj);
			}
			pData->SetEnemyObjectList(listEnemyObject);

			++iData;
		}
	}

	_NextStep();
}
void StgEnemyBossSceneObject::AddData(int step, ref_unsync_ptr<StgEnemyBossSceneData> data) {
	if (listData_.size() <= step)
		listData_.resize(step + 1);
	listData_[step].push_back(data);
}
void StgEnemyBossSceneObject::LoadAllScriptInThread() {
	auto scriptManager = stageController_->GetScriptManager();
	for (std::vector<ref_unsync_ptr<StgEnemyBossSceneData>>& iStep : listData_) {
		for (ref_unsync_ptr<StgEnemyBossSceneData>& pData : iStep) {
			auto script = scriptManager->LoadScriptInThread(pData->GetPath(), StgStageScript::TYPE_STAGE);
			//auto script = scriptManager->LoadScript(pData->GetPath(), StgStageScript::TYPE_STAGE);
			pData->SetScriptPointer(script);
		}
	}
	bLoad_ = true;
}
size_t StgEnemyBossSceneObject::GetRemainStepCount() {
	int res = listData_.size() - dataStep_ - 1;
	res = std::max(res, 0);
	return res;
}
size_t StgEnemyBossSceneObject::GetActiveStepLifeCount() {
	if (dataStep_ >= listData_.size()) return 0;
	return listData_[dataStep_].size();
}
double StgEnemyBossSceneObject::GetActiveStepTotalMaxLife() {
	if (dataStep_ >= listData_.size()) return 0;
	double res = 0;
	for (ref_unsync_ptr<StgEnemyBossSceneData>& pData : listData_[dataStep_]) {
		for (double iLife : pData->GetLifeList())
			res = res + iLife;
	}
	return res;
}
double StgEnemyBossSceneObject::GetActiveStepTotalLife() {
	if (dataStep_ >= listData_.size()) return 0;
	double res = 0;
	for (size_t iData = dataIndex_; iData < listData_[dataStep_].size(); iData++) {
		res = res + GetActiveStepLife(iData);
	}
	return res;
}
double StgEnemyBossSceneObject::GetActiveStepLife(size_t index) {
	if (dataStep_ >= listData_.size()) return 0;
	if (index < dataIndex_) return 0;

	double res = 0;
	ref_unsync_ptr<StgEnemyBossSceneData>& data = listData_[dataStep_][index];
	if (index == dataIndex_) {
		for (auto& iEnemyObj : data->GetEnemyObjectList())
			res += iEnemyObj->GetLife();
	}
	else {
		for (double iLife : data->GetLifeList())
			res += iLife;
	}
	return res;
}
std::vector<double> StgEnemyBossSceneObject::GetActiveStepLifeRateList() {
	std::vector<double> res;
	res.resize(GetActiveStepLifeCount());

	const double total = GetActiveStepTotalMaxLife();
	double rate = 0;

	size_t iData = 0;
	for (ref_unsync_ptr<StgEnemyBossSceneData>& pSceneData : listData_[dataStep_]) {
		double life = 0;
		
		for (double iLife : pSceneData->GetLifeList())
			life += iLife;

		rate += life / total;
		res[iData++] = rate;
	}

	return res;
}
void StgEnemyBossSceneObject::AddPlayerShootDownCount() {
	if (activeData_ == nullptr) return;
	activeData_->AddPlayerShootDownCount();
}
void StgEnemyBossSceneObject::AddPlayerSpellCount() {
	if (activeData_ == nullptr) return;
	activeData_->AddPlayerSpellCount();
}

//StgEnemyBossSceneData
StgEnemyBossSceneData::StgEnemyBossSceneData() {
	countCreate_ = 0;
	bReadyNext_ = false;

	scoreSpell_ = 0;
	timerSpell_ = -1;
	bSpell_ = false;
	bLastSpell_ = false;
	bDurable_ = false;
	bRequireAllShotdown_ = true;

	countPlayerShootDown_ = 0;
	countPlayerSpell_ = 0;
}
int StgEnemyBossSceneData::GetEnemyBossIdInCreate() {
	if (countCreate_ >= listEnemyObject_.size()) {
		std::string log = StringUtility::Format("Cannot create any more boss objects [Max=%d]\r\n"
			"**Use an array in EV_REQUEST_LIFE to create multiple bosses.", countCreate_);
		throw gstd::wexception(log);
	}

	ref_unsync_ptr<StgEnemyBossObject> obj = listEnemyObject_[countCreate_];
	++countCreate_;

	return obj->GetObjectID();
}
int64_t StgEnemyBossSceneData::GetCurrentSpellScore() {
	int64_t res = scoreSpell_;
	if (!bDurable_) {
		double rate = (double)timerSpell_ / (double)timerSpellOrg_;
		res = scoreSpell_ * rate;
	}
	return res;
}
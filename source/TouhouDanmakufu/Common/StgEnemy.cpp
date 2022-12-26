#include "source/GcLib/pch.h"

#include "StgEnemy.hpp"
#include "StgSystem.hpp"

//*******************************************************************
//StgEnemyManager
//*******************************************************************
StgEnemyManager::StgEnemyManager(StgStageController* stageController) {
	bLoadThreadCancel_ = false;
	countLoad_ = 0;

	stageController_ = stageController;

	rcDeleteClip_ = DxRect<LONG>(-64, -64, 64, 64);

	FileManager::GetBase()->AddLoadThreadListener(this);
}
StgEnemyManager::~StgEnemyManager() {
	for (ref_unsync_ptr<StgEnemyObject>& obj : listEnemy_) {
		if (obj)
			obj->ClearEnemyObject();
	}

	this->WaitForCancel();
	FileManager::GetBase()->RemoveLoadThreadListener(this);
}
void StgEnemyManager::Work() {
	for (auto itr = listEnemy_.begin(); itr != listEnemy_.end();) {
		ref_unsync_ptr<StgEnemyObject>& obj = (*itr);
		if (obj->IsDeleted()) {
			obj->ClearEnemyObject();
			itr = listEnemy_.erase(itr);
		}
		else ++itr;
	}
}
void StgEnemyManager::RegistIntersectionTarget() {
	for (ref_unsync_ptr<StgEnemyObject>& obj : listEnemy_) {
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

class _ListBossSceneData : public FileManager::LoadObject {
public:
	std::vector<weak_ptr<StgEnemyBossSceneData>> listData;
	size_t cData;
public:
	_ListBossSceneData(std::vector<shared_ptr<StgEnemyBossSceneData>>* listStepData) {
		cData = listStepData->size();
		listData.resize(cData);
		for (size_t i = 0; i < cData; ++i)
			listData[i] = listStepData->at(i);
	}
};
void StgEnemyManager::LoadBossSceneScriptsInThread(std::vector<shared_ptr<StgEnemyBossSceneData>>* listStepData) {
	Lock lock(lock_);
	{
		shared_ptr<FileManager::LoadObject> pListData;
		pListData.reset(new _ListBossSceneData(listStepData));

		shared_ptr<FileManager::LoadThreadEvent> event(new FileManager::LoadThreadEvent(this, L"", pListData));
		FileManager::GetBase()->AddLoadThreadEvent(event);
	}
}
void StgEnemyManager::CallFromLoadThread(shared_ptr<FileManager::LoadThreadEvent> event) {
	shared_ptr<_ListBossSceneData> pListData = std::dynamic_pointer_cast<_ListBossSceneData>(event->GetSource());
	if (pListData == nullptr) return;

	++countLoad_;

	{
		weak_ptr<StgStageScriptManager> scriptManagerWeak = stageController_->GetScriptManagerRef();
		StgStageScriptObjectManager* objectManager = stageController_->GetMainObjectManager();

		for (size_t i = 0; i < pListData->cData; ++i) {
			shared_ptr<StgEnemyBossSceneData> pData = pListData->listData[i].lock();
			if (pData == nullptr || pData->bLoad_) continue;

			if (!bLoadThreadCancel_) {
				auto pManager = scriptManagerWeak.lock();
				if (pManager && pData->objBossSceneParent_.IsExists()) {
					try {
						auto script = pManager->LoadScript(pData->GetPath(), StgStageScript::TYPE_STAGE);
						if (script == nullptr)
							throw gstd::wexception(StringUtility::Format(L"Cannot load script: %s", pData->GetPath().c_str()));

						pData->SetScriptPointer(script);
						pData->bLoad_ = true;
					}
					catch (gstd::wexception& e) {
						Logger::WriteTop(e.what());
						pManager->SetError(e.what());

						pData->SetScriptPointer(weak_ptr<ManagedScript>());
						pData->bLoad_ = true;

						goto lab_cancel_all;
					}
					continue;
				}
			}
			
lab_cancel_all:
			//Cancels loading of all remaining scripts
			for (size_t j = 0; j < pListData->cData; ++j) {
				if (auto pData_ = pListData->listData[i].lock()) {
					pData_->bLoad_ = true;
				}
			}
			break;
		}
	}

	--countLoad_;
}

//*******************************************************************
//StgEnemyObject
//*******************************************************************
StgEnemyObject::StgEnemyObject(StgStageController* stageController) : StgMoveObject(stageController) {
	typeObject_ = TypeObject::Enemy;

	SetRenderPriorityI(40);

	life_ = 0;
	lifePrev_ = 0;
	rateDamageShot_ = 1;
	rateDamageSpell_ = 1;

	maximumDamage_ = 256 * 256 * 256;
	damageAccumFrame_ = 0;

	intersectedPlayerShotCount_ = 0U;

	bAutoDelete_ = false;
	frameAutoDelete_ = INT_MAX;

	bEnableGetIntersectionPositionFetch_ = true;
}
StgEnemyObject::~StgEnemyObject() {
}

void StgEnemyObject::Clone(DxScriptObjectBase* _src) {
	DxScriptSpriteObject2D::Clone(_src);

	auto src = (StgEnemyObject*)_src;
	StgMoveObject::Copy((StgMoveObject*)src);
	StgIntersectionObject::Copy((StgIntersectionObject*)src);

	life_ = src->life_;
	lifePrev_ = src->lifePrev_;
	lifeDelta_ = src->lifeDelta_;

	rateDamageShot_ = src->rateDamageShot_;
	rateDamageSpell_ = src->rateDamageSpell_;
	maximumDamage_ = src->maximumDamage_;
	damageAccumFrame_ = src->damageAccumFrame_;
	intersectedPlayerShotCount_ = src->intersectedPlayerShotCount_;

	bAutoDelete_ = src->bAutoDelete_;
	frameAutoDelete_ = src->frameAutoDelete_;

	bEnableGetIntersectionPositionFetch_ = src->bEnableGetIntersectionPositionFetch_;
	ptrIntersectionToShot_ = src->ptrIntersectionToShot_;
	ptrIntersectionToPlayer_ = src->ptrIntersectionToPlayer_;
	mapShotDamageRate_ = src->mapShotDamageRate_;
}

void StgEnemyObject::Work() {
	ClearIntersected();
	intersectedPlayerShotCount_ = 0U;

	ptrIntersectionToShot_.clear();
	ptrIntersectionToPlayer_.clear();

	lifeDelta_ = lifePrev_ - life_;
	lifePrev_ = life_;
	damageAccumFrame_ = 0;

	_Move();

	_DeleteInAutoClip();
	_DeleteInAutoDeleteFrame();
}
void StgEnemyObject::_DeleteInAutoClip() {
	if (!bAutoDelete_ || IsDeleted()) return;

	DxRect<LONG>* const rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();
	DxRect<LONG>* const rcClipBase = stageController_->GetEnemyManager()->GetEnemyDeleteClip();
	DxRect<LONG> rcDeleteClip(rcClipBase->left, rcClipBase->top,
		rcStgFrame->GetWidth() + rcClipBase->right,
		rcStgFrame->GetHeight() + rcClipBase->bottom);

	if (!rcDeleteClip.IsPointIntersected(posX_, posY_)) {
		stageController_->GetMainObjectManager()->DeleteObject(this);
	}
}
void StgEnemyObject::_DeleteInAutoDeleteFrame() {
	if (IsDeleted()) return;

	if (frameAutoDelete_ <= 0)
		stageController_->GetMainObjectManager()->DeleteObject(this);
	else --frameAutoDelete_;
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
				if (ref_unsync_weak_ptr<StgEnemyObject> self = ownTarget->GetObject()) {
					//Register intersection only if the enemy is off hit cooldown
					if (!shot->CheckEnemyHitCooldownExists(self)) {
						damage = shot->GetDamage() * (shot->IsSpellFactor() ? rateDamageSpell_ : rateDamageShot_) 
							* GetShotDamageRateByShotDataID(shot->GetShotDataID());
						++intersectedPlayerShotCount_;

						uint32_t frame = shot->GetEnemyIntersectionInvalidFrame();
						if (frame > 0)
							shot->AddEnemyHitCooldown(self, frame);
					}
				}
			}
		}
		else if (otherTarget->GetTargetType() == StgIntersectionTarget::TYPE_PLAYER_SPELL) {
			if (StgPlayerSpellObject* spell = dynamic_cast<StgPlayerSpellObject*>(ptrObj.get()))
				damage = spell->GetDamage() * rateDamageSpell_;
		}
	}
	AddLife2(-damage);
}
void StgEnemyObject::RegistIntersectionTarget() {
	_AddRelativeIntersection();
}
ref_unsync_ptr<StgEnemyObject> StgEnemyObject::GetOwnObject() {
	return ref_unsync_ptr<StgEnemyObject>::Cast(stageController_->GetMainRenderObject(idObject_));
}
void StgEnemyObject::AddLife(double inc) { 
	life_ = std::max(life_ + inc, 0.0); 
}
void StgEnemyObject::AddLife2(double inc) {
	if (damageAccumFrame_ - inc > maximumDamage_)
		inc = -(maximumDamage_ - damageAccumFrame_);
	damageAccumFrame_ -= inc;
	AddLife(inc);
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

void StgEnemyBossObject::Clone(DxScriptObjectBase* _src) {
	StgEnemyObject::Clone(_src);

	auto src = (StgEnemyBossObject*)_src;

	throw new wexception("Object cannot be cloned: ObjEnemyBoss");
}

//*******************************************************************
//StgEnemyBossSceneObject
//*******************************************************************
StgEnemyBossSceneObject::StgEnemyBossSceneObject(StgStageController* stageController) : StgObjectBase(stageController) {
	typeObject_ = TypeObject::EnemyBossScene;

	bScriptsLoaded_ = false;
	bEnableUnloadCache_ = false;

	dataStep_ = 0;
	dataIndex_ = -1;
}
StgEnemyBossSceneObject::~StgEnemyBossSceneObject() {
	if (bEnableUnloadCache_) {
		auto pCache = stageController_->GetSystemController()->GetScriptEngineCache();
		for (std::vector<shared_ptr<StgEnemyBossSceneData>>& iStep : listData_) {
			for (shared_ptr<StgEnemyBossSceneData>& pData : iStep) {
				pCache->RemoveCache(pData->GetPath());
			}
		}
	}
}

void StgEnemyBossSceneObject::Clone(DxScriptObjectBase* _src) {
	DxScriptObjectBase::Clone(_src);

	auto src = (StgEnemyBossObject*)_src;

	throw new wexception("Object cannot be cloned: ObjEnemyBossScene");
}

void StgEnemyBossSceneObject::_WaitForStepLoad(int iStep) {
	auto scriptManager = stageController_->GetScriptManager();

	for (shared_ptr<StgEnemyBossSceneData> pData : listData_[iStep]) {
		if (!pData->IsLoad()) {
			DWORD count = 0;
			while (!pData->IsLoad()) {
				if (count % 100 == 0) {
					Logger::WriteTop(StringUtility::Format(L"_NextScript: Script is still loading... [%s]",
						PathProperty::GetFileName(pData->GetPath()).c_str()));
				}
				::Sleep(10);
				++count;
			}
		}

		scriptManager->TryThrowError();
		pData->LoadSceneEvents(stageController_);
	}
}
bool StgEnemyBossSceneObject::_NextScript() {
	if (dataStep_ >= listData_.size()) return false;

	auto scriptManager = stageController_->GetScriptManager();
	StgStageScriptObjectManager* objectManager = stageController_->GetMainObjectManager();

	shared_ptr<StgEnemyBossSceneData> oldActiveData = activeData_;

	if (oldActiveData)
		scriptManager->RequestEventAll(StgStageScript::EV_END_BOSS_STEP);
	else if (dataIndex_ == -1) {
		_WaitForStepLoad(0);
	}

	dataIndex_++;
	if (dataIndex_ >= listData_[dataStep_].size()) {
		dataIndex_ = 0;
		while (true) {
			dataStep_++;
			if (dataStep_ >= listData_.size()) return false;
			if (listData_[dataStep_].size() > 0) {
				_WaitForStepLoad(dataStep_);
				break;
			}
		}
	}

	activeData_ = listData_[dataStep_][dataIndex_];

	LOCK_WEAK(pScript, activeData_->GetScriptPointer()) {
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

		scriptManager->StartScript(pScript);
		scriptManager->RequestEventAll(StgStageScript::EV_START_BOSS_STEP);
	}
	else {
		throw gstd::wexception(StringUtility::Format(L"_NextScript: Invalid script at step %d, index %d",
			dataStep_, dataIndex_));
	}

	return true;
}
void StgEnemyBossSceneObject::Work() {
	if (activeData_->IsReadyNext()) {
		bool bEnemyExists = false;
		for (ref_unsync_ptr<StgEnemyBossObject>& iEnemy : activeData_->GetEnemyObjectList())
			bEnemyExists |= !iEnemy->IsDeleted();

		if (!bEnemyExists) {
			bool bNext = _NextScript();
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
	if (!bScriptsLoaded_)
		LoadAllScriptInThread();

	_NextScript();
}
void StgEnemyBossSceneObject::AddData(int step, shared_ptr<StgEnemyBossSceneData> data) {
	if (listData_.size() <= step)
		listData_.resize(step + 1);
	listData_[step].push_back(data);
}
void StgEnemyBossSceneObject::LoadAllScriptInThread() {
	for (std::vector<shared_ptr<StgEnemyBossSceneData>>& iStep : listData_) {
		stageController_->GetEnemyManager()->LoadBossSceneScriptsInThread(&iStep);
	}

	bScriptsLoaded_ = true;
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
	for (shared_ptr<StgEnemyBossSceneData> pData : listData_[dataStep_]) {
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

	shared_ptr<StgEnemyBossSceneData> data = listData_[dataStep_][index];

	double res = 0;
	
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
	for (shared_ptr<StgEnemyBossSceneData> pSceneData : listData_[dataStep_]) {
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
	bLoad_ = false;

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

void StgEnemyBossSceneData::LoadSceneEvents(StgStageController* stageController) {
	auto scriptManager = stageController->GetScriptManager();
	StgStageScriptObjectManager* objectManager = stageController->GetMainObjectManager();

	LOCK_WEAK(script, ptrScript_) {
		std::vector<double> listLife;
		gstd::value vLife = script->RequestEvent(StgStageScript::EV_REQUEST_LIFE);
		if (vLife.has_data()) {
			if (script->IsArrayValue(vLife)) {
				for (size_t iLife = 0; iLife < vLife.length_as_array(); ++iLife) {
					double life = vLife[iLife].as_float();
					listLife.push_back(life);
				}
			}
			else {
				double life = vLife.as_float();
				listLife.push_back(life);
			}
		}

		if (listLife.size() == 0)
			throw gstd::wexception(StringUtility::Format(L"EV_REQUEST_LIFE must return a value. (%s)",
				GetPath().c_str()));
		SetLifeList(listLife);

		gstd::value vTimer = script->RequestEvent(StgStageScript::EV_REQUEST_TIMER);
		if (vTimer.has_data())
			SetOriginalSpellTimer(vTimer.as_float() * DnhConfiguration::GetInstance()->fpsStandard_);

		gstd::value vSpell = script->RequestEvent(StgStageScript::EV_REQUEST_IS_SPELL);
		if (vSpell.has_data())
			SetSpellCard(vSpell.as_boolean());

		{
			gstd::value vScore = script->RequestEvent(StgStageScript::EV_REQUEST_SPELL_SCORE);
			if (vScore.has_data()) SetSpellScore(vScore.as_float());

			gstd::value vLast = script->RequestEvent(StgStageScript::EV_REQUEST_IS_LAST_SPELL);
			if (vLast.has_data()) SetLastSpell(vLast.as_boolean());

			gstd::value vDurable = script->RequestEvent(StgStageScript::EV_REQUEST_IS_DURABLE_SPELL);
			if (vDurable.has_data()) SetDurable(vDurable.as_boolean());

			gstd::value vAllDown = script->RequestEvent(StgStageScript::EV_REQUEST_REQUIRE_ALL_DOWN);
			if (vAllDown.has_data()) SetRequireAllDown(vAllDown.as_boolean());
		}

		std::vector<ref_unsync_ptr<StgEnemyBossObject>> listEnemyObject;
		for (size_t iEnemy = 0; iEnemy < listLife.size(); iEnemy++) {
			ref_unsync_ptr<StgEnemyBossObject> obj = new StgEnemyBossObject(stageController);
			int idEnemy = objectManager->AddObject(obj, false);
			listEnemyObject.push_back(obj);
		}
		SetEnemyObjectList(listEnemyObject);
	}
}
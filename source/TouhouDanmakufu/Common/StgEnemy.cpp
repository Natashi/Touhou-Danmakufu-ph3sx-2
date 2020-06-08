#include "source/GcLib/pch.h"

#include "StgEnemy.hpp"
#include "StgSystem.hpp"

/**********************************************************
//StgEnemyManager
**********************************************************/
StgEnemyManager::StgEnemyManager(StgStageController* stageController) {
	stageController_ = stageController;
}
StgEnemyManager::~StgEnemyManager() {
	std::list<shared_ptr<StgEnemyObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); itr++) {
		shared_ptr<StgEnemyObject> obj = (*itr);
		if (obj != nullptr) {
			obj->ClearEnemyObject();
		}
	}
}
void StgEnemyManager::Work() {
	std::list<shared_ptr<StgEnemyObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); ) {
		shared_ptr<StgEnemyObject> obj = (*itr);
		if (obj->IsDeleted()) {
			obj->ClearEnemyObject();
			itr = listObj_.erase(itr);
		}
		else itr++;
	}

}
void StgEnemyManager::RegistIntersectionTarget() {
	std::list<shared_ptr<StgEnemyObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); itr++) {
		shared_ptr<StgEnemyObject> obj = (*itr);
		if (!obj->IsDeleted()) {
			obj->ClearIntersectedIdList();
			obj->RegistIntersectionTarget();
		}
	}
}
void StgEnemyManager::SetBossSceneObject(shared_ptr<StgEnemyBossSceneObject> obj) {
	if (objBossScene_ != nullptr && !objBossScene_->IsDeleted())
		throw gstd::wexception("ObjEnemyBossScene already exists.");
	objBossScene_ = obj;
}
shared_ptr<StgEnemyBossSceneObject> StgEnemyManager::GetBossSceneObject() {
	shared_ptr<StgEnemyBossSceneObject> res = nullptr;
	if (objBossScene_ != nullptr && !objBossScene_->IsDeleted())
		res = objBossScene_;
	return res;
}

/**********************************************************
//StgEnemyObject
**********************************************************/
StgEnemyObject::StgEnemyObject(StgStageController* stageController) : StgMoveObject(stageController) {
	stageController_ = stageController;
	typeObject_ = TypeObject::OBJ_ENEMY;

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
void StgEnemyObject::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	double damage = 0;
	if (auto ptrObj = otherTarget->GetObject().lock()) {
		if (otherTarget->GetTargetType() == StgIntersectionTarget::TYPE_PLAYER_SHOT) {
			StgShotObject* shot = (StgShotObject*)ptrObj.get();
			if (shot) {
				damage = shot->GetDamage() * (shot->IsSpellFactor() ? rateDamageSpell_ : rateDamageShot_) / 100.0;
				++intersectedPlayerShotCount_;
			}
		}
		else if (otherTarget->GetTargetType() == StgIntersectionTarget::TYPE_PLAYER_SPELL) {
			StgPlayerSpellObject* spell = (StgPlayerSpellObject*)ptrObj.get();
			if (spell) 
				damage = spell->GetDamage() * rateDamageSpell_ / 100;
		}
	}
	life_ = std::max(life_ - damage, 0.0);
}
void StgEnemyObject::RegistIntersectionTarget() {
	_AddRelativeIntersection();
}
shared_ptr<StgEnemyObject> StgEnemyObject::GetOwnObject() {
	return std::dynamic_pointer_cast<StgEnemyObject>(stageController_->GetMainRenderObject(idObject_));
}
void StgEnemyObject::AddReferenceToShotIntersection(StgIntersectionTarget::ptr pointer) {
	ptrIntersectionToShot_.push_back(pointer);
}
void StgEnemyObject::AddReferenceToPlayerIntersection(StgIntersectionTarget::ptr pointer) {
	ptrIntersectionToPlayer_.push_back(pointer);
}

/**********************************************************
//StgEnemyBossObject
**********************************************************/
StgEnemyBossObject::StgEnemyBossObject(StgStageController* stageController) : StgEnemyObject(stageController) {
	typeObject_ = TypeObject::OBJ_ENEMY_BOSS;
}

/**********************************************************
//StgEnemyBossSceneObject
**********************************************************/
StgEnemyBossSceneObject::StgEnemyBossSceneObject(StgStageController* stageController) {
	stageController_ = stageController;
	typeObject_ = TypeObject::OBJ_ENEMY_BOSS_SCENE;

	bVisible_ = false;
	bLoad_ = false;
	dataStep_ = 0;
	dataIndex_ = -1;
}
bool StgEnemyBossSceneObject::_NextStep() {
	if (dataStep_ >= listData_.size())return false;

	auto scriptManager = stageController_->GetScriptManager();

	//現ステップ終了通知
	if (activeData_ != nullptr) {
		scriptManager->RequestEventAll(StgStageScript::EV_END_BOSS_STEP);
	}

	dataIndex_++;
	if (dataIndex_ >= listData_[dataStep_].size()) {
		dataIndex_ = 0;
		while (true) {
			dataStep_++;
			if (dataStep_ >= listData_.size())return false;
			if (listData_[dataStep_].size() > 0)break;
		}
	}

	shared_ptr<StgEnemyBossSceneData> oldActiveData = activeData_;

	//敵登録
	StgStageScriptObjectManager* objectManager = stageController_->GetMainObjectManager();
	activeData_ = listData_[dataStep_][dataIndex_];
	std::vector<shared_ptr<StgEnemyBossObject>>& listEnemy = activeData_->GetEnemyObjectList();
	std::vector<double>& listLife = activeData_->GetLifeList();
	for (size_t iEnemy = 0; iEnemy < listEnemy.size(); iEnemy++) {
		shared_ptr<StgEnemyBossObject> obj = listEnemy[iEnemy];
		obj->SetLife(listLife[iEnemy]);
		if (oldActiveData != nullptr) {
			std::vector<shared_ptr<StgEnemyBossObject>> listOldEnemyObject = oldActiveData->GetEnemyObjectList();
			if (iEnemy < listOldEnemyObject.size()) {
				shared_ptr<StgEnemyBossObject> objOld = listOldEnemyObject[iEnemy];
				obj->SetPositionX(objOld->GetPositionX());
				obj->SetPositionY(objOld->GetPositionY());
			}
		}
		objectManager->ActivateObject(obj->GetObjectID(), true);
	}

	//スクリプト開始
	int64_t idScript = activeData_->GetScriptID();
	scriptManager->StartScript(idScript);

	//新ステップ開始通知
	scriptManager->RequestEventAll(StgStageScript::EV_START_BOSS_STEP);

	return true;
}
void StgEnemyBossSceneObject::Work() {
	if (activeData_->IsReadyNext()) {
		//次ステップ遷移可能
		bool bEnemyExists = false;
		std::vector<shared_ptr<StgEnemyBossObject>> listEnemy = activeData_->GetEnemyObjectList();
		for (size_t iEnemy = 0; iEnemy < listEnemy.size(); iEnemy++) {
			shared_ptr<StgEnemyBossObject> obj = listEnemy[iEnemy];
			bEnemyExists |= (!obj->IsDeleted());
		}

		if (!bEnemyExists) {
			bool bNext = _NextStep();
			if (!bNext) {
				//終了
				StgEnemyManager* enemyManager = stageController_->GetEnemyManager();
				StgStageScriptObjectManager* objectManager = stageController_->GetMainObjectManager();
				objectManager->DeleteObject(idObject_);
				enemyManager->SetBossSceneObject(nullptr);
				return;
			}
		}

	}
	else if (!activeData_->IsReadyNext()) {
		//タイマー監視
		bool bZeroTimer = false;
		int timer = activeData_->GetSpellTimer();
		if (timer > 0) {
			timer--;
			activeData_->SetSpellTimer(timer);
			if (timer == 0) {
				bZeroTimer = true;
			}
		}

		//ラストスペル監視
		bool bEndLastSpell = false;
		if (activeData_->IsLastSpell()) {
			bEndLastSpell = activeData_->GetPlayerShootDownCount() > 0;
		}

		if (bZeroTimer || bEndLastSpell) {
			//タイマー0なら敵のライフを0にする
			std::vector<shared_ptr<StgEnemyBossObject>>& listEnemy = activeData_->GetEnemyObjectList();
			for (size_t iEnemy = 0; iEnemy < listEnemy.size(); iEnemy++) {
				shared_ptr<StgEnemyBossObject> obj = listEnemy[iEnemy];
				obj->SetLife(0);
			}

			if (bZeroTimer) {
				//タイムアウト通知
				auto scriptManager = stageController_->GetScriptManager();
				scriptManager->RequestEventAll(StgStageScript::EV_TIMEOUT);
			}
		}

		//次シーンへの遷移フラグ設定
		std::vector<shared_ptr<StgEnemyBossObject>>& listEnemy = activeData_->GetEnemyObjectList();
		bool bReadyNext = true;
		if (activeData_->IsRequireAllDown()) {
			for (auto itr = listEnemy.begin(); itr != listEnemy.end(); ++itr) {
				if ((*itr)->GetLife() > 0) bReadyNext = false;
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
				//スペルカード取得
				//・タイマー0／スペル使用／被弾時は取得不可
				//・耐久の場合はタイマー0でも取得可能
				bool bGain = true;
				bGain &= (activeData_->GetPlayerShootDownCount() == 0);
				bGain &= (activeData_->GetPlayerSpellCount() == 0);
				bGain &= (activeData_->IsDurable() || activeData_->GetSpellTimer() > 0);

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
	//スクリプトを読み込んでいなかったら読み込む。
	if (!bLoad_)
		LoadAllScriptInThread();

	auto scriptManager = stageController_->GetScriptManager();
	StgStageScriptObjectManager* objectManager = stageController_->GetMainObjectManager();

	for (size_t iStep = 0; iStep < listData_.size(); iStep++) {
		for (size_t iData = 0; iData < listData_[iStep].size(); iData++) {
			shared_ptr<StgEnemyBossSceneData> data = listData_[iStep][iData];

			int64_t idScript = data->GetScriptID();
			shared_ptr<ManagedScript> script = scriptManager->GetScript(idScript);

			if (script == nullptr)
				throw gstd::wexception(StringUtility::Format(L"読み込まれていないスクリプト：%s", data->GetPath().c_str()).c_str());
			if (!script->IsLoad()) {
				int count = 0;
				while (!script->IsLoad()) {
					if (count % 1000 == 999) {
						std::wstring log = StringUtility::Format(L"Waiting for script load: [%d, %d] %s", iStep, iData, data->GetPath().c_str());
						Logger::WriteTop(log);
					}
					Sleep(1);
					count++;
				}
			}

			if (stageController_->GetSystemInformation()->IsError())continue;

			//ライフ読み込み
			std::vector<double> listLife;
			gstd::value vLife = script->RequestEvent(StgStageScript::EV_REQUEST_LIFE);
			if (script->IsRealValue(vLife)) {
				double life = vLife.as_real();
				listLife.push_back(life);
			}
			else if (script->IsRealArrayValue(vLife)) {
				size_t count = vLife.length_as_array();
				for (size_t iLife = 0; iLife < count; iLife++) {
					double life = vLife.index_as_array(iLife).as_real();
					listLife.push_back(life);
				}
			}

			if (listLife.size() == 0)
				throw gstd::wexception(StringUtility::Format(L"EV_REQUEST_LIFE must return a value. (%s)", data->GetPath().c_str()));
			data->SetLifeList(listLife);

			//タイマー読み込み
			gstd::value vTimer = script->RequestEvent(StgStageScript::EV_REQUEST_TIMER);
			data->SetOriginalSpellTimer(vTimer.as_real() * STANDARD_FPS);

			//スペル
			gstd::value vSpell = script->RequestEvent(StgStageScript::EV_REQUEST_IS_SPELL);
			data->SetSpellCard(vSpell.as_boolean());

			{
				//スコア、ラストスペル、耐久スペルを読み込む
				gstd::value vScore = script->RequestEvent(StgStageScript::EV_REQUEST_SPELL_SCORE);
				data->SetSpellScore(vScore.as_real());

				gstd::value vLast = script->RequestEvent(StgStageScript::EV_REQUEST_IS_LAST_SPELL);
				data->SetLastSpell(vLast.as_boolean());

				gstd::value vDurable = script->RequestEvent(StgStageScript::EV_REQUEST_IS_DURABLE_SPELL);
				data->SetDurable(vDurable.as_boolean());

				gstd::value vAllDown = script->RequestEvent(StgStageScript::EV_REQUEST_REQUIRE_ALL_DOWN);
				data->SetRequireAllDown(vAllDown.as_boolean());
			}

			//敵オブジェクト作成
			std::vector<shared_ptr<StgEnemyBossObject>> listEnemyObject;
			for (size_t iEnemy = 0; iEnemy < listLife.size(); iEnemy++) {
				shared_ptr<StgEnemyBossObject> obj = shared_ptr<StgEnemyBossObject>(new StgEnemyBossObject(stageController_));
				int idEnemy = objectManager->AddObject(obj, false);
				listEnemyObject.push_back(obj);
			}
			data->SetEnemyObjectList(listEnemyObject);
		}
	}

	//登録
	_NextStep();

}
void StgEnemyBossSceneObject::AddData(int step, shared_ptr<StgEnemyBossSceneData> data) {
	if (listData_.size() <= step)
		listData_.resize(step + 1);
	listData_[step].push_back(data);
}
void StgEnemyBossSceneObject::LoadAllScriptInThread() {
	auto scriptManager = stageController_->GetScriptManager();
	for (size_t iStep = 0; iStep < listData_.size(); iStep++) {
		for (size_t iData = 0; iData < listData_[iStep].size(); iData++) {
			shared_ptr<StgEnemyBossSceneData> data = listData_[iStep][iData];
			std::wstring path = data->GetPath();

			auto script = scriptManager->LoadScriptInThread(path, StgStageScript::TYPE_SYSTEM);
			data->SetScriptID(script->GetScriptID());
		}
	}
	bLoad_ = true;
}
int StgEnemyBossSceneObject::GetRemainStepCount() {
	int res = listData_.size() - dataStep_ - 1;
	res = std::max(res, 0);
	return res;
}
int StgEnemyBossSceneObject::GetActiveStepLifeCount() {
	if (dataStep_ >= listData_.size())return 0;
	return listData_[dataStep_].size();
}
double StgEnemyBossSceneObject::GetActiveStepTotalMaxLife() {
	if (dataStep_ >= listData_.size())return 0;
	double res = 0;
	for (size_t iData = 0; iData < listData_[dataStep_].size(); iData++) {
		shared_ptr<StgEnemyBossSceneData> data = listData_[dataStep_][iData];
		std::vector<double>& listLife = data->GetLifeList();
		for (size_t iLife = 0; iLife < listLife.size(); iLife++)
			res = res + listLife[iLife];
	}
	return res;
}
double StgEnemyBossSceneObject::GetActiveStepTotalLife() {
	if (dataStep_ >= listData_.size())return 0;
	double res = 0;
	for (size_t iData = dataIndex_; iData < listData_[dataStep_].size(); iData++) {
		res = res + GetActiveStepLife(iData);
	}
	return res;
}
double StgEnemyBossSceneObject::GetActiveStepLife(int index) {
	if (dataStep_ >= listData_.size())return 0;
	if (index < dataIndex_)return 0;

	double res = 0;
	shared_ptr<StgEnemyBossSceneData> data = listData_[dataStep_][index];
	if (index == dataIndex_) {
		std::vector<shared_ptr<StgEnemyBossObject>>& listEnemyObject = data->GetEnemyObjectList();
		for (size_t iEnemy = 0; iEnemy < listEnemyObject.size(); iEnemy++) {
			shared_ptr<StgEnemyBossObject> obj = listEnemyObject[iEnemy];
			res += obj->GetLife();
		}
	}
	else {
		std::vector<double>& listLife = data->GetLifeList();
		for (size_t iLife = 0; iLife < listLife.size(); iLife++)
			res += listLife[iLife];
	}
	return res;
}
std::vector<double> StgEnemyBossSceneObject::GetActiveStepLifeRateList() {
	std::vector<double> res;
	res.resize(GetActiveStepLifeCount());

	const double total = GetActiveStepTotalMaxLife();

	for (size_t iData = 0; iData < res.size(); iData++) {
		shared_ptr<StgEnemyBossSceneData> data = listData_[dataStep_][iData];

		double life = 0;
		std::vector<double>& listLife = data->GetLifeList();
		for (size_t iLife = 0; iLife < listLife.size(); iLife++) {
			life += listLife[iLife];
		}

		res[iData] = life / total;
	}

	return res;
}
void StgEnemyBossSceneObject::AddPlayerShootDownCount() {
	if (activeData_ == nullptr)return;
	activeData_->AddPlayerShootDownCount();
}
void StgEnemyBossSceneObject::AddPlayerSpellCount() {
	if (activeData_ == nullptr)return;
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
			"**Return an array in EV_REQUEST_LIFE to create multiple bosses.", countCreate_);
		throw gstd::wexception(log);
	}

	shared_ptr<StgEnemyBossObject> obj = listEnemyObject_[countCreate_];
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



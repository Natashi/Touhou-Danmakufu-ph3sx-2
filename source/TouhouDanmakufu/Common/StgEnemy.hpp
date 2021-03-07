#pragma once

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgIntersection.hpp"

class StgEnemyObject;
class StgEnemyBossSceneObject;
/**********************************************************
//StgEnemyManager
**********************************************************/
class StgEnemyManager {
	StgStageController* stageController_;
	std::list<ref_unsync_ptr<StgEnemyObject>> listObj_;
	ref_unsync_ptr<StgEnemyBossSceneObject> objBossScene_;
public:
	StgEnemyManager(StgStageController* stageController);
	virtual ~StgEnemyManager();

	void Work();
	void RegistIntersectionTarget();

	void AddEnemy(ref_unsync_ptr<StgEnemyObject> obj) { listObj_.push_back(obj); }
	size_t GetEnemyCount() { return listObj_.size(); }

	void SetBossSceneObject(ref_unsync_ptr<StgEnemyBossSceneObject> obj);
	ref_unsync_ptr<StgEnemyBossSceneObject> GetBossSceneObject();
	std::list<ref_unsync_ptr<StgEnemyObject>>& GetEnemyList() { return listObj_; }
};

/**********************************************************
//StgEnemyObject
**********************************************************/
class StgEnemyObject : public DxScriptSpriteObject2D, public StgMoveObject, public StgIntersectionObject {
protected:
	StgStageController* stageController_;

	double life_;
	double rateDamageShot_;
	double rateDamageSpell_;
	size_t intersectedPlayerShotCount_;

	bool bEnableGetIntersectionPositionFetch_;

	std::vector<ref_unsync_weak_ptr<StgIntersectionTarget>> ptrIntersectionToShot_;
	std::vector<ref_unsync_weak_ptr<StgIntersectionTarget>> ptrIntersectionToPlayer_;

	virtual void _Move();
	virtual void _AddRelativeIntersection();
public:
	StgEnemyObject(StgStageController* stageController);
	virtual ~StgEnemyObject();

	virtual void Work();
	virtual void Activate();
	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);
	virtual void ClearEnemyObject() { ClearIntersectionRelativeTarget(); }
	virtual void RegistIntersectionTarget();

	virtual void SetX(double x) { posX_ = x; DxScriptRenderObject::SetX(x); }
	virtual void SetY(double y) { posY_ = y; DxScriptRenderObject::SetY(y); }

	ref_unsync_ptr<StgEnemyObject> GetOwnObject();
	double GetLife() { return life_; }
	void SetLife(double life) { life_ = life; }
	void AddLife(double inc) { life_ += inc; life_ = std::max(life_, 0.0); }
	void SetDamageRate(double rateShot, double rateSpell) { rateDamageShot_ = rateShot; rateDamageSpell_ = rateSpell; }
	double GetShotDamageRate() { return rateDamageShot_; }
	double GetSpellDamageRate() { return rateDamageSpell_; }
	size_t GetIntersectedPlayerShotCount() { return intersectedPlayerShotCount_; }

	void SetEnableGetIntersectionPosition(bool flg) { bEnableGetIntersectionPositionFetch_ = flg; }
	bool GetEnableGetIntersectionPosition() { return bEnableGetIntersectionPositionFetch_; }

	void AddReferenceToShotIntersection(ref_unsync_ptr<StgIntersectionTarget> target);
	void AddReferenceToPlayerIntersection(ref_unsync_ptr<StgIntersectionTarget> target);
	std::vector<ref_unsync_weak_ptr<StgIntersectionTarget>>* GetIntersectionListShot() { return &ptrIntersectionToShot_; }
	std::vector<ref_unsync_weak_ptr<StgIntersectionTarget>>* GetIntersectionListPlayer() { return &ptrIntersectionToPlayer_; }
};

/**********************************************************
//StgEnemyBossObject
**********************************************************/
class StgEnemyBossObject : public StgEnemyObject {
private:
	int timeSpellCard_;
public:
	StgEnemyBossObject(StgStageController* stageController);
};

/**********************************************************
//StgEnemyBossSceneObject
**********************************************************/
class StgEnemyBossSceneData;
class StgEnemyBossSceneObject : public DxScriptObjectBase {
private:
	StgStageController* stageController_;
	volatile bool bLoad_;

	int dataStep_;
	int dataIndex_;
	ref_unsync_ptr<StgEnemyBossSceneData> activeData_;
	std::vector<std::vector<ref_unsync_ptr<StgEnemyBossSceneData>>> listData_;

	bool _NextStep();
public:
	StgEnemyBossSceneObject(StgStageController* stageController);

	virtual void Work();
	virtual void Activate();
	virtual void Render() {}
	virtual void SetRenderState() {}

	void AddData(int step, ref_unsync_ptr<StgEnemyBossSceneData> data);
	ref_unsync_ptr<StgEnemyBossSceneData> GetActiveData() { return activeData_; }
	void LoadAllScriptInThread();

	size_t GetRemainStepCount();
	size_t GetActiveStepLifeCount();
	double GetActiveStepTotalMaxLife();
	double GetActiveStepTotalLife();
	double GetActiveStepLife(size_t index);
	std::vector<double> GetActiveStepLifeRateList();
	int GetDataStep() { return dataStep_; }
	int GetDataIndex() { return dataIndex_; }

	void AddPlayerShootDownCount();
	void AddPlayerSpellCount();
};

class StgEnemyBossSceneData {
private:
	std::wstring path_;
	weak_ptr<ManagedScript> ptrScript_;

	std::vector<double> listLife_;
	std::vector<ref_unsync_ptr<StgEnemyBossObject>> listEnemyObject_;
	int countCreate_;			//The maximum amount of bosses allowed in listEnemyObject_
	bool bReadyNext_;

	int64_t scoreSpell_;

	bool bSpell_;				//Currently a spell?
	bool bLastSpell_;			//Last spell? (th8-style)
	bool bDurable_;				//Survival spell
	bool bRequireAllShotdown_;	//End the step only when all boss objects are dead?

	int timerSpellOrg_;			//Original timer
	int timerSpell_;			//Current timer
	int countPlayerShootDown_;	//Player death count
	int countPlayerSpell_;		//Player bomb count
public:
	StgEnemyBossSceneData();
	virtual ~StgEnemyBossSceneData() {}

	std::wstring& GetPath() { return path_; }
	void SetPath(const std::wstring& path) { path_ = path; }

	weak_ptr<ManagedScript> GetScriptPointer() { return ptrScript_; }
	void SetScriptPointer(weak_ptr<ManagedScript> id) { ptrScript_ = id; }

	std::vector<double>& GetLifeList() { return listLife_; }
	void SetLifeList(std::vector<double>& list) { listLife_ = list; }
	std::vector<ref_unsync_ptr<StgEnemyBossObject>>& GetEnemyObjectList() { return listEnemyObject_; }
	void SetEnemyObjectList(std::vector<ref_unsync_ptr<StgEnemyBossObject>>& list) { listEnemyObject_ = list; }
	int GetEnemyBossIdInCreate();

	bool IsReadyNext() { return bReadyNext_; }
	void SetReadyNext() { bReadyNext_ = true; }

	int64_t GetCurrentSpellScore();
	int64_t GetSpellScore() { return scoreSpell_; }
	void SetSpellScore(int64_t score) { scoreSpell_ = score; }

	int GetSpellTimer() { return timerSpell_; }
	void SetSpellTimer(int timer) { timerSpell_ = timer; }
	int GetOriginalSpellTimer() { return timerSpellOrg_; }
	void SetOriginalSpellTimer(int timer) { timerSpellOrg_ = timer; timerSpell_ = timer; }
	bool IsSpellCard() { return bSpell_; }
	void SetSpellCard(bool b) { bSpell_ = b; }
	bool IsLastSpell() { return bLastSpell_; }
	void SetLastSpell(bool b) { bLastSpell_ = b; }
	bool IsDurable() { return bDurable_; }
	void SetDurable(bool b) { bDurable_ = b; }
	bool IsRequireAllDown() { return bRequireAllShotdown_; }
	void SetRequireAllDown(bool b) { bRequireAllShotdown_ = b; }

	void AddPlayerShootDownCount() { countPlayerShootDown_++; }
	int GetPlayerShootDownCount() { return countPlayerShootDown_; }
	void AddPlayerSpellCount() { countPlayerSpell_++; }
	int GetPlayerSpellCount() { return countPlayerSpell_; }
};
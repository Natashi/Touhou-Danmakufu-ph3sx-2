#pragma once

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgIntersection.hpp"

class StgEnemyObject;
class StgEnemyBossSceneObject;
class StgEnemyBossSceneData;
//*******************************************************************
//StgEnemyManager
//*******************************************************************
class StgEnemyManager : public FileManager::LoadThreadListener {
private:
	CriticalSection lock_;
	std::atomic_bool bLoadThreadCancel_;
	std::atomic_int countLoad_;

	StgStageController* stageController_;

	std::list<ref_unsync_ptr<StgEnemyObject>> listEnemy_;

	ref_unsync_ptr<StgEnemyBossSceneObject> objBossScene_;
protected:
	DxRect<LONG> rcDeleteClip_;
public:
	StgEnemyManager(StgStageController* stageController);
	virtual ~StgEnemyManager();

	virtual void CancelLoad() { bLoadThreadCancel_ = true; }
	virtual bool CancelLoadComplete() { return countLoad_ <= 0; }

	CriticalSection& GetLock() { return lock_; }

	void Work();
	void RegistIntersectionTarget();

	void AddEnemy(ref_unsync_ptr<StgEnemyObject> obj) { listEnemy_.push_back(obj); }
	size_t GetEnemyCount() { return listEnemy_.size(); }

	void SetEnemyDeleteClip(const DxRect<LONG>& clip) { rcDeleteClip_ = clip; }
	DxRect<LONG>* GetEnemyDeleteClip() { return &rcDeleteClip_; }

	void SetBossSceneObject(ref_unsync_ptr<StgEnemyBossSceneObject> obj);
	ref_unsync_ptr<StgEnemyBossSceneObject> GetBossSceneObject();
	std::list<ref_unsync_ptr<StgEnemyObject>>& GetEnemyList() { return listEnemy_; }

	void LoadBossSceneScriptsInThread(std::vector<shared_ptr<StgEnemyBossSceneData>>* listStepData);
	virtual void CallFromLoadThread(shared_ptr<FileManager::LoadThreadEvent> event);
};

//*******************************************************************
//StgEnemyObject
//*******************************************************************
class StgEnemyObject : public DxScriptSpriteObject2D, public StgMoveObject, public StgIntersectionObject {
protected:
	StgStageController* stageController_;

	double life_;
	double lifePrev_;
	double lifeDelta_;
	double rateDamageShot_;
	double rateDamageSpell_;

	double maximumDamage_;
	double damageAccumFrame_;

	size_t intersectedPlayerShotCount_;

	bool bAutoDelete_;
	int frameAutoDelete_;

	bool bEnableGetIntersectionPositionFetch_;

	std::vector<ref_unsync_weak_ptr<StgIntersectionTarget>> ptrIntersectionToShot_;
	std::vector<ref_unsync_weak_ptr<StgIntersectionTarget>> ptrIntersectionToPlayer_;

	std::unordered_map<int, double> mapShotDamageRate_;

	void _DeleteInAutoClip();
	void _DeleteInAutoDeleteFrame();
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

	void SetAutoDelete(bool b) { bAutoDelete_ = b; }
	void SetAutoDeleteFrame(int frame) { frameAutoDelete_ = frame; }

	double GetLife() { return life_; }
	double GetLifeDelta() { return lifeDelta_; }
	void SetLife(double life) { life_ = lifePrev_ = life; }
	void AddLife(double inc);
	void AddLife2(double inc);

	void SetDamageRate(double rateShot, double rateSpell) { rateDamageShot_ = rateShot; rateDamageSpell_ = rateSpell; }
	double GetShotDamageRate() { return rateDamageShot_; }
	double GetSpellDamageRate() { return rateDamageSpell_; }
	void SetShotDamageRateByShotDataID(int id, double rate) { mapShotDamageRate_[id] = rate; }
	double GetShotDamageRateByShotDataID(int id) {
		return mapShotDamageRate_.size() > 0 ? 
			(mapShotDamageRate_.count(id) > 0 ? mapShotDamageRate_[id] : 1) : 1;
	}

	void SetMaximumDamage(double dmg) { maximumDamage_ = dmg; }
	double GetMaximumDamage() { return maximumDamage_; }

	size_t GetIntersectedPlayerShotCount() { return intersectedPlayerShotCount_; }
	void SetEnableGetIntersectionPosition(bool flg) { bEnableGetIntersectionPositionFetch_ = flg; }
	bool GetEnableGetIntersectionPosition() { return bEnableGetIntersectionPositionFetch_; }

	void AddReferenceToShotIntersection(ref_unsync_ptr<StgIntersectionTarget> target);
	void AddReferenceToPlayerIntersection(ref_unsync_ptr<StgIntersectionTarget> target);
	std::vector<ref_unsync_weak_ptr<StgIntersectionTarget>>* GetIntersectionListShot() { return &ptrIntersectionToShot_; }
	std::vector<ref_unsync_weak_ptr<StgIntersectionTarget>>* GetIntersectionListPlayer() { return &ptrIntersectionToPlayer_; }
};

//*******************************************************************
//StgEnemyBossObject
//*******************************************************************
class StgEnemyBossObject : public StgEnemyObject {
private:
	int timeSpellCard_;
public:
	StgEnemyBossObject(StgStageController* stageController);
};

//*******************************************************************
//StgEnemyBossSceneObject
//*******************************************************************
class StgEnemyBossSceneObject : public DxScriptObjectBase {
private:
	StgStageController* stageController_;

	bool bScriptsLoaded_;
	bool bEnableUnloadCache_;

	int dataStep_;
	int dataIndex_;
	shared_ptr<StgEnemyBossSceneData> activeData_;
	std::vector<std::vector<shared_ptr<StgEnemyBossSceneData>>> listData_;

	void _WaitForStepLoad(int iStep);
	bool _NextScript();
public:
	StgEnemyBossSceneObject(StgStageController* stageController);
	~StgEnemyBossSceneObject();

	virtual void Work();
	virtual void Activate();
	virtual void Render() {}
	virtual void SetRenderState() {}

	void AddData(int step, shared_ptr<StgEnemyBossSceneData> data);
	shared_ptr<StgEnemyBossSceneData> GetActiveData() { return activeData_; }
	void LoadAllScriptInThread();

	void SetUnloadCache(bool b) { bEnableUnloadCache_ = b; }

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
	friend class StgEnemyManager;
private:
	volatile bool bLoad_;

	std::wstring path_;
	weak_ptr<ManagedScript> ptrScript_;

	ref_unsync_weak_ptr<StgEnemyBossSceneObject> objBossSceneParent_;

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

	void LoadSceneEvents(StgStageController* stageController);

	bool IsLoad() { return bLoad_; }

	std::wstring& GetPath() { return path_; }
	void SetPath(const std::wstring& path) { path_ = path; }

	void SetParent(ref_unsync_ptr<StgEnemyBossSceneObject> parent) { objBossSceneParent_ = parent; }

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
#ifndef __TOUHOUDANMAKUFU_DNHSTG_ENEMY__
#define __TOUHOUDANMAKUFU_DNHSTG_ENEMY__

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
	std::list<shared_ptr<StgEnemyObject>> listObj_;
	shared_ptr<StgEnemyBossSceneObject> objBossScene_;
public:
	StgEnemyManager(StgStageController* stageController);
	virtual ~StgEnemyManager();
	void Work();
	void RegistIntersectionTarget();

	void AddEnemy(shared_ptr<StgEnemyObject> obj) { listObj_.push_back(obj); }
	size_t GetEnemyCount() { return listObj_.size(); }

	void SetBossSceneObject(shared_ptr<StgEnemyBossSceneObject> obj);
	shared_ptr<StgEnemyBossSceneObject> GetBossSceneObject();
	std::list<shared_ptr<StgEnemyObject>>& GetEnemyList() { return listObj_; }
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

	std::vector<std::weak_ptr<StgIntersectionTarget>> ptrIntersectionToShot_;
	std::vector<std::weak_ptr<StgIntersectionTarget>> ptrIntersectionToPlayer_;

	virtual void _Move();
	virtual void _AddRelativeIntersection();
public:
	StgEnemyObject(StgStageController* stageController);
	virtual ~StgEnemyObject();

	virtual void Work();
	virtual void Activate();
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);
	virtual void ClearEnemyObject() { ClearIntersectionRelativeTarget(); }
	virtual void RegistIntersectionTarget();

	virtual void SetX(double x) { posX_ = x; DxScriptRenderObject::SetX(x); }
	virtual void SetY(double y) { posY_ = y; DxScriptRenderObject::SetY(y); }

	shared_ptr<StgEnemyObject> GetOwnObject();
	double GetLife() { return life_; }
	void SetLife(double life) { life_ = life; }
	void AddLife(double inc) { life_ += inc; life_ = std::max(life_, 0.0); }
	void SetDamageRate(double rateShot, double rateSpell) { rateDamageShot_ = rateShot; rateDamageSpell_ = rateSpell; }
	double GetShotDamageRate() { return rateDamageShot_; }
	double GetSpellDamageRate() { return rateDamageSpell_; }
	size_t GetIntersectedPlayerShotCount() { return intersectedPlayerShotCount_; }

	void SetEnableGetIntersectionPosition(bool flg) { bEnableGetIntersectionPositionFetch_ = flg; }
	bool GetEnableGetIntersectionPosition() { return bEnableGetIntersectionPositionFetch_; }

	void AddReferenceToShotIntersection(StgIntersectionTarget::ptr pointer);
	void AddReferenceToPlayerIntersection(StgIntersectionTarget::ptr pointer);
	std::vector<weak_ptr<StgIntersectionTarget>>* GetIntersectionListShot() { return &ptrIntersectionToShot_; }
	std::vector<weak_ptr<StgIntersectionTarget>>* GetIntersectionListPlayer() { return &ptrIntersectionToPlayer_; }
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
	shared_ptr<StgEnemyBossSceneData> activeData_;
	std::vector<std::vector<shared_ptr<StgEnemyBossSceneData>>> listData_;

	bool _NextStep();
public:
	StgEnemyBossSceneObject(StgStageController* stageController);
	virtual void Work();
	virtual void Activate();
	virtual void Render() {}//何もしない
	virtual void SetRenderState() {}//何もしない

	void AddData(int step, shared_ptr<StgEnemyBossSceneData> data);
	shared_ptr<StgEnemyBossSceneData> GetActiveData() { return activeData_; }
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
	std::vector<shared_ptr<StgEnemyBossObject>> listEnemyObject_;
	int countCreate_;//ボス生成数。listEnemyObject_を超えて生成しようとしたらエラー。
	bool bReadyNext_;

	bool bSpell_;//スペルカード
	bool bLastSpell_;//ラストスペル
	bool bDurable_;//耐久スペル
	bool bRequireAllShotdown_;	//Whether the step will end when all boss objects are dead
	int64_t scoreSpell_;
	int timerSpellOrg_;//初期タイマー フレーム単位 -1で無効
	int timerSpell_;//タイマー フレーム単位 -1で無効
	int countPlayerShootDown_;//自機撃破数
	int countPlayerSpell_;//自機スペル使用数
public:
	StgEnemyBossSceneData();
	virtual ~StgEnemyBossSceneData() {}
	std::wstring& GetPath() { return path_; }
	void SetPath(std::wstring path) { path_ = path; }
	weak_ptr<ManagedScript> GetScriptPointer() { return ptrScript_; }
	void SetScriptPointer(weak_ptr<ManagedScript> id) { ptrScript_ = id; }
	std::vector<double>& GetLifeList() { return listLife_; }
	void SetLifeList(std::vector<double>& list) { listLife_ = list; }
	std::vector<shared_ptr<StgEnemyBossObject>>& GetEnemyObjectList() { return listEnemyObject_; }
	void SetEnemyObjectList(std::vector<shared_ptr<StgEnemyBossObject>>& list) { listEnemyObject_ = list; }
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

#endif

#include "source/GcLib/pch.h"

#include "StgIntersection.hpp"
#include "StgShot.hpp"
#include "StgPlayer.hpp"
#include "StgEnemy.hpp"

/**********************************************************
//StgIntersectionManager
**********************************************************/
StgIntersectionManager::StgIntersectionManager() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	int screenWidth = graphics->GetScreenWidth();
	int screenHeight = graphics->GetScreenWidth();

	omp_init_lock(&lock_);

	//_CreatePool(2);
	listSpace_.resize(3);
	for (size_t iSpace = 0; iSpace < listSpace_.size(); iSpace++) {
		StgIntersectionSpace* space = new StgIntersectionSpace();
		space->Initialize(-100, -100, screenWidth + 100, screenHeight + 100);
		listSpace_[iSpace] = space;
	}
}
StgIntersectionManager::~StgIntersectionManager() {
	for (auto& itr : listSpace_) {
		ptr_delete(itr);
	}
	listSpace_.clear();

	omp_destroy_lock(&lock_);
}
void StgIntersectionManager::Work() {
	listEnemyTargetPoint_ = listEnemyTargetPointNext_;
	listEnemyTargetPointNext_.clear();

	size_t totalCheck = 0;
	size_t totalTarget = 0;
	std::vector<StgIntersectionSpace*>::iterator itr = listSpace_.begin();
	for (; itr != listSpace_.end(); itr++) {
		StgIntersectionSpace* space = *itr;
		StgIntersectionCheckList* listCheck = space->CreateIntersectionCheckList(this, totalTarget);

		size_t countCheck = listCheck->GetCheckCount();

//#pragma omp parallel for
		for (size_t iCheck = 0; iCheck < countCheck; iCheck++) {
			//Getは1回しか使用できません
			StgIntersectionTarget::ptr targetA = listCheck->GetTargetA(iCheck);
			StgIntersectionTarget::ptr targetB = listCheck->GetTargetB(iCheck);

			if (targetA == nullptr || targetB == nullptr) continue;

			bool bIntersected = IsIntersected(targetA, targetB);
			if (!bIntersected) continue;

			//Grazeの関係で、先に自機の当たり判定をする必要がある。
			weak_ptr<StgIntersectionObject> objA = targetA->GetObject();
			weak_ptr<StgIntersectionObject> objB = targetB->GetObject();
			auto ptrA = objA.lock();
			auto ptrB = objB.lock();

			{
				//omp_set_lock(&lock_);

				if (ptrA) {
					ptrA->Intersect(targetA, targetB);
					ptrA->SetIntersected();
					if (ptrB) ptrA->AddIntersectedId(ptrB);
				}
				if (ptrB) {
					ptrB->Intersect(targetB, targetA);
					ptrB->SetIntersected();
					if (ptrA) ptrB->AddIntersectedId(ptrA);
				}

				//omp_unset_lock(&lock_);
			}
		}

		totalCheck += countCheck;
		space->ClearTarget();
	}

	//_ArrangePool();

	ELogger* logger = ELogger::GetInstance();
	if (logger->IsWindowVisible()) {
		/*
		int countUsed = GetUsedPoolObjectCount();
		int countCache = GetCachePoolObjectCount();
		logger->SetInfo(9, L"Intersection count",
			StringUtility::Format(L"Used=%4d, Cached=%4d, Total=%4d, Check=%4d", countUsed, countCache, countUsed + countCache, totalCheck));
		*/
		logger->SetInfo(9, L"Intersection count",
			StringUtility::Format(L"Total=%4d, Check=%4d", totalTarget, totalCheck));
	}
}
void StgIntersectionManager::AddTarget(StgIntersectionTarget::ptr target) {
	if (shared_ptr<StgIntersectionObject> obj = target->GetObject().lock()) {
		int type = target->GetTargetType();
		switch (type) {
		case StgIntersectionTarget::TYPE_PLAYER:
		{
			listSpace_[SPACE_PLAYER_ENEMY]->RegistTargetA(target);
			break;
		}
		case StgIntersectionTarget::TYPE_PLAYER_SHOT:
		case StgIntersectionTarget::TYPE_PLAYER_SPELL:
		{
			listSpace_[SPACE_PLAYERSHOT_ENEMY]->RegistTargetA(target);

			bool bEraseShot = false;

			if (obj) {
				//弾消し能力付加なら
				if (type == StgIntersectionTarget::TYPE_PLAYER_SHOT) {
					StgShotObject* shot = (StgShotObject*)obj.get();
					if (shot)
						bEraseShot = shot->IsEraseShot();
				}
				else if (type == StgIntersectionTarget::TYPE_PLAYER_SPELL) {
					StgPlayerSpellObject* spell = (StgPlayerSpellObject*)obj.get();
					if (spell)
						bEraseShot = spell->IsEraseShot();
				}
			}

			if (bEraseShot) {
				listSpace_[SPACE_PLAYERSHOT_ENEMYSHOT]->RegistTargetA(target);
			}
			break;
		}
		case StgIntersectionTarget::TYPE_ENEMY:
		{
			listSpace_[SPACE_PLAYER_ENEMY]->RegistTargetB(target);
			listSpace_[SPACE_PLAYERSHOT_ENEMY]->RegistTargetB(target);

			StgIntersectionTarget_Circle::ptr circle = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);
			if (circle) {
				shared_ptr<StgEnemyObject> objEnemy = std::dynamic_pointer_cast<StgEnemyObject>(obj);
				if (objEnemy) {
					POINT pos = { (int)circle->GetCircle().GetX(), (int)circle->GetCircle().GetY() };
					StgIntersectionTargetPoint tp;
					tp.SetObjectRef(objEnemy);
					tp.SetPoint(pos);
					listEnemyTargetPointNext_.push_back(tp);
				}
			}

			break;
		}
		case StgIntersectionTarget::TYPE_ENEMY_SHOT:
		{
			listSpace_[SPACE_PLAYER_ENEMY]->RegistTargetB(target);
			listSpace_[SPACE_PLAYERSHOT_ENEMYSHOT]->RegistTargetB(target);
			break;
		}
		}
	}
}
void StgIntersectionManager::AddEnemyTargetToShot(StgIntersectionTarget::ptr target) {
	//target->SetMortonNumber(-1);
	//target->ClearObjectIntersectedIdList();

	int type = target->GetTargetType();
	switch (type) {
	case StgIntersectionTarget::TYPE_ENEMY:
	{
		listSpace_[SPACE_PLAYERSHOT_ENEMY]->RegistTargetB(target);

		StgIntersectionTarget_Circle::ptr circle = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);
		if (circle) {
			if (shared_ptr<StgIntersectionObject> obj = target->GetObject().lock()) {
				shared_ptr<StgEnemyObject> objEnemy = std::dynamic_pointer_cast<StgEnemyObject>(obj);
				if (objEnemy) {
					POINT pos = { (int)circle->GetCircle().GetX(), (int)circle->GetCircle().GetY() };
					StgIntersectionTargetPoint tp;
					tp.SetObjectRef(objEnemy);
					tp.SetPoint(pos);
					listEnemyTargetPointNext_.push_back(tp);
				}
			}
		}

		break;
	}
	}
}
void StgIntersectionManager::AddEnemyTargetToPlayer(StgIntersectionTarget::ptr target) {
	//target->SetMortonNumber(-1);
	//target->ClearObjectIntersectedIdList();

	int type = target->GetTargetType();
	switch (type) {
	case StgIntersectionTarget::TYPE_ENEMY:
	{
		listSpace_[SPACE_PLAYER_ENEMY]->RegistTargetB(target);
		break;
	}
	}
}

bool StgIntersectionManager::IsIntersected(StgIntersectionTarget::ptr& target1, StgIntersectionTarget::ptr& target2) {
	bool res = false;
	int shape1 = target1->GetShape();
	int shape2 = target2->GetShape();
	StgIntersectionTarget* p1 = target1.get();
	StgIntersectionTarget* p2 = target2.get();
	if (p1 == nullptr || p2 == nullptr) return false;
	if (shape1 == StgIntersectionTarget::SHAPE_CIRCLE && shape2 == StgIntersectionTarget::SHAPE_CIRCLE) {
		StgIntersectionTarget_Circle* c1 = dynamic_cast<StgIntersectionTarget_Circle*>(p1);
		StgIntersectionTarget_Circle* c2 = dynamic_cast<StgIntersectionTarget_Circle*>(p2);
		res = DxMath::IsIntersected(c1->GetCircle(), c2->GetCircle());
	}
	else if ((shape1 == StgIntersectionTarget::SHAPE_CIRCLE && shape2 == StgIntersectionTarget::SHAPE_LINE) ||
		(shape1 == StgIntersectionTarget::SHAPE_LINE && shape2 == StgIntersectionTarget::SHAPE_CIRCLE)) {
		StgIntersectionTarget_Circle* c = nullptr;
		StgIntersectionTarget_Line* l = nullptr;
		if (shape1 == StgIntersectionTarget::SHAPE_CIRCLE && shape2 == StgIntersectionTarget::SHAPE_LINE) {
			c = dynamic_cast<StgIntersectionTarget_Circle*>(p1);
			l = dynamic_cast<StgIntersectionTarget_Line*>(p2);
		}
		else {
			c = dynamic_cast<StgIntersectionTarget_Circle*>(p2);
			l = dynamic_cast<StgIntersectionTarget_Line*>(p1);
		}

		res = DxMath::IsIntersected(c->GetCircle(), l->GetLine());
	}
	else if (shape1 == StgIntersectionTarget::SHAPE_LINE && shape2 == StgIntersectionTarget::SHAPE_LINE) {
		StgIntersectionTarget_Line* l1 = dynamic_cast<StgIntersectionTarget_Line*>(p1);
		StgIntersectionTarget_Line* l2 = dynamic_cast<StgIntersectionTarget_Line*>(p2);
		res = DxMath::IsIntersected(l1->GetLine(), l2->GetLine());
	}
	return res;
}

/*
void StgIntersectionManager::_ResetPoolObject(StgIntersectionTarget::ptr& obj) {
	//	ELogger::WriteTop(StringUtility::Format("_ResetPoolObject:start:%s)", obj->GetInfoAsString().c_str()));
	obj->obj_ = NULL;
	//	ELogger::WriteTop("_ResetPoolObject:end");
}
ref_count_ptr<StgIntersectionTarget>::unsync StgIntersectionManager::_CreatePoolObject(int type) {
	StgIntersectionTarget::ptr res = nullptr;
	switch (type) {
	case StgIntersectionTarget::SHAPE_CIRCLE:
		res = new StgIntersectionTarget_Circle();
		break;
	case StgIntersectionTarget::SHAPE_LINE:
		res = new StgIntersectionTarget_Line();
		break;
	}
	return res;
}

void StgIntersectionManager::CheckDeletedObject(std::string funcName) {
	int countType = listUsedPool_.size();
	for (int iType = 0; iType < countType; iType++) {
		std::list<StgIntersectionTarget::ptr>* listUsed = &listUsedPool_[iType];
		std::vector<StgIntersectionTarget::ptr>* listCache = &listCachePool_[iType];

		std::list<StgIntersectionTarget::ptr>::iterator itr = listUsed->begin();
		for (; itr != listUsed->end(); itr++) {
			StgIntersectionTarget::ptr& target = (*itr);
			ref_count_weak_ptr<DxScriptObjectBase>::unsync dxObj =
				ref_count_weak_ptr<DxScriptObjectBase>::unsync::DownCast(target->GetObject());
			if (dxObj != NULL && dxObj->IsDeleted()) {
				ELogger::WriteTop(StringUtility::Format(L"%s(deleted):%s", funcName.c_str(), target->GetInfoAsString().c_str()));
			}
		}
	}
}
*/

/**********************************************************
//StgIntersectionSpace
**********************************************************/
StgIntersectionSpace::StgIntersectionSpace() {
	spaceLeft_ = 0;
	spaceTop_ = 0;
	spaceWidth_ = 0;
	spaceHeight_ = 0;

	listCheck_ = new StgIntersectionCheckList();
}
StgIntersectionSpace::~StgIntersectionSpace() {
	ptr_delete(listCheck_);
}
bool StgIntersectionSpace::Initialize(int left, int top, int right, int bottom) {
	listCell_.resize(2);

	spaceLeft_ = left;
	spaceTop_ = top;
	spaceWidth_ = right - left;
	spaceHeight_ = bottom - top;

	return true;
}
bool StgIntersectionSpace::RegistTarget(int type, StgIntersectionTarget::ptr& target) {
	RECT& rect = target->GetIntersectionSpaceRect();
	if (rect.right < spaceLeft_ || rect.bottom < spaceTop_ ||
		rect.left >(spaceLeft_ + spaceWidth_) || rect.top >(spaceTop_ + spaceHeight_))
		return false;

	listCell_[type].push_back(target);
	return true;
}

size_t StgIntersectionSpace::_WriteIntersectionCheckList(StgIntersectionManager* manager, StgIntersectionCheckList*& listCheck)
//	std::vector<std::vector<StgIntersectionTarget*>> &listStack) 
{
	std::atomic<size_t> count{0};

	std::vector<StgIntersectionTarget::ptr>& listTargetA = listCell_[0];
	std::vector<StgIntersectionTarget::ptr>& listTargetB = listCell_[1];
	auto itrA = listTargetA.begin();
	auto itrB = listTargetB.begin();

	omp_lock_t* ompLock = manager->GetLock();

#pragma omp for
	for (int iA = 0; iA < listTargetA.size(); ++iA) {
		StgIntersectionTarget::ptr targetA = listTargetA[iA];

		for (size_t iB = 0; iB < listTargetB.size(); ++iB) {
			StgIntersectionTarget::ptr targetB = listTargetB[iB];

			RECT& rc1 = targetA->GetIntersectionSpaceRect();
			RECT& rc2 = targetB->GetIntersectionSpaceRect();

			++count;

			bool bIntersectX = (rc1.left >= rc2.right && rc2.left >= rc1.right)
				|| (rc1.left <= rc2.right && rc2.left <= rc1.right);
			if (!bIntersectX) continue;
			bool bIntersectY = (rc1.bottom >= rc2.top && rc2.bottom >= rc1.top) 
				|| (rc1.bottom <= rc2.top && rc2.bottom <= rc1.top);
			if (!bIntersectY) continue;

			{
				omp_set_lock(ompLock);
				listCheck->Add(targetA, targetB);
				omp_unset_lock(ompLock);
			}
		}
	}

	return count;

	//I have no idea what all these below were meant to do, but removing them resulted in 
	//	both no unfortunate issues and faster performance.

	/*
	std::vector<std::vector<StgIntersectionTarget*>>& listCell = listCell_[indexSpace];
	int typeCount = listCell.size();
	for (int iType1 = 0; iType1 < typeCount; ++iType1) {
		std::vector<StgIntersectionTarget*>& list1 = listCell[iType1];
		int iType2 = 0;
		for (iType2 = iType1 + 1; iType2 < typeCount; ++iType2) {
			std::vector<StgIntersectionTarget*>& list2 = listCell[iType2];

			// ① 空間内のオブジェクト同士の衝突リスト作成
			std::vector<StgIntersectionTarget*>::iterator itr1 = list1.begin();
			for (; itr1 != list1.end(); ++itr1) {
				std::vector<StgIntersectionTarget*>::iterator itr2 = list2.begin();
				for (; itr2 != list2.end(); ++itr2) {
					StgIntersectionTarget*& target1 = (*itr1);
					StgIntersectionTarget*& target2 = (*itr2);
					listCheck->Add(target1, target2);
				}
			}

		}

		std::vector<StgIntersectionTarget*>& stack = listStack[iType1];
		for (iType2 = 0; iType2 < typeCount; ++iType2) {
			if (iType1 == iType2)continue;
			std::vector<StgIntersectionTarget*>& list2 = listCell[iType2];

			// ② 衝突スタックとの衝突リスト作成
			std::vector<StgIntersectionTarget*>::iterator itrStack = stack.begin();
			for (; itrStack != stack.end(); ++itrStack) {
				std::vector<StgIntersectionTarget*>::iterator itr2 = list2.begin();
				for (; itr2 != list2.end(); ++itr2) {
					StgIntersectionTarget*& target2 = (*itr2);
					StgIntersectionTarget*& targetStack = (*itrStack);
					if (iType1 < iType2)
						listCheck->Add(targetStack, target2);
					else
						listCheck->Add(target2, targetStack);
				}
			}
		}
	}

	//空間内のオブジェクトをスタックに追加
	int iType = 0;
	for (iType = 0; iType < typeCount; ++iType) {
		std::vector<StgIntersectionTarget*>& list = listCell[iType];
		std::vector<StgIntersectionTarget*>& stack = listStack[iType];
		std::vector<StgIntersectionTarget*>::iterator itr = list.begin();
		for (; itr != list.end(); ++itr) {
			StgIntersectionTarget* target = (*itr);
			stack.push_back(target);
		}
	}

	//スタックから解除
	for (iType = 0; iType < typeCount; ++iType) {
		std::vector<StgIntersectionTarget*>& list = listCell[iType];
		std::vector<StgIntersectionTarget*>& stack = listStack[iType];
		int count = list.size();
		for (int iCount = 0; iCount < count; ++iCount) {
			stack.pop_back();
		}
	}
	*/
}

/*
unsigned int StgIntersectionSpace::_GetMortonNumber(float left, float top, float right, float bottom) {
	// 座標から空間番号を算出
	// 最小レベルにおける各軸位置を算出
	unsigned int  LT = _GetPointElem(left, top);
	unsigned int  RB = _GetPointElem(right, bottom);

	// 空間番号の排他的論理和から
	// 所属レベルを算出
	unsigned int def = RB ^ LT;
	unsigned int hiLevel = 0;
	for (int iLevel = 0; iLevel < unitLevel_; ++iLevel) {
		DWORD Check = (def >> (iLevel * 2)) & 0x3;
		if (Check != 0)
			hiLevel = iLevel + 1;
	}
	DWORD spaceIndex = RB >> (hiLevel * 2);
	DWORD addIndex = (listCountLevel_[unitLevel_ - hiLevel] - 1) / 3;
	spaceIndex += addIndex;

	if (spaceIndex > countCell_)
		return 0xffffffff;

	return spaceIndex;
}
unsigned int StgIntersectionSpace::_BitSeparate32(unsigned int n) {
	// ビット分割関数
	n = (n | (n << 8)) & 0x00ff00ff;
	n = (n | (n << 4)) & 0x0f0f0f0f;
	n = (n | (n << 2)) & 0x33333333;
	return (n | (n << 1)) & 0x55555555;
}
unsigned short StgIntersectionSpace::_Get2DMortonNumber(unsigned short x, unsigned short y) {
	// 2Dモートン空間番号算出関数
	return (unsigned short)(_BitSeparate32(x) | (_BitSeparate32(y) << 1));
}
unsigned int  StgIntersectionSpace::_GetPointElem(float pos_x, float pos_y) {
	// 座標→線形4分木要素番号変換関数
	float val1 = std::max(pos_x - spaceLeft_, 0.0);
	float val2 = std::max(pos_y - spaceTop_, 0.0);
	return _Get2DMortonNumber(
		(unsigned short)(val1 / unitWidth_), (unsigned short)(val2 / unitHeight_));
}
*/

//StgIntersectionObject
void StgIntersectionObject::ClearIntersectionRelativeTarget() {
	for (auto itr = listRelativeTarget_.begin(); itr != listRelativeTarget_.end(); ++itr)
		(*itr)->SetObject(weak_ptr<StgIntersectionObject>());
	listRelativeTarget_.clear();
}
void StgIntersectionObject::AddIntersectionRelativeTarget(StgIntersectionTarget::ptr target) {
	listRelativeTarget_.push_back(target);
	int shape = target->GetShape();
	if (shape == StgIntersectionTarget::SHAPE_CIRCLE) {
		StgIntersectionTarget_Circle::ptr tTarget = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);
		if (tTarget)
			listOrgCircle_.push_back(tTarget->GetCircle());
	}
	else if (shape == StgIntersectionTarget::SHAPE_LINE) {
		StgIntersectionTarget_Line::ptr tTarget = std::dynamic_pointer_cast<StgIntersectionTarget_Line>(target);
		if (tTarget)
			listOrgLine_.push_back(tTarget->GetLine());
	}
}
void StgIntersectionObject::UpdateIntersectionRelativeTarget(int posX, int posY, double angle) {
	size_t iCircle = 0;
	size_t iLine = 0;

	for (auto itr = listRelativeTarget_.begin(); itr != listRelativeTarget_.end(); ++itr) {
		StgIntersectionTarget::ptr target = *itr;

		int shape = target->GetShape();
		if (shape == StgIntersectionTarget::SHAPE_CIRCLE) {
			StgIntersectionTarget_Circle::ptr tTarget = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);
			if (tTarget) {
				DxCircle org = listOrgCircle_[iCircle];
				int px = org.GetX() + posX;
				int py = org.GetY() + posY;

				DxCircle circle = tTarget->GetCircle();
				circle.SetX(px);
				circle.SetY(py);
				tTarget->SetCircle(circle);
			}
			++iCircle;
		}
		else if (shape == StgIntersectionTarget::SHAPE_LINE) {
			//StgIntersectionTarget_Line::ptr tTarget = StgIntersectionTarget_Line::ptr::DownCast(target);
			++iLine;
		}
	}
}
void StgIntersectionObject::RegistIntersectionRelativeTarget(StgIntersectionManager* manager) {
	for (auto itr = listRelativeTarget_.begin(); itr != listRelativeTarget_.end(); ++itr)
		manager->AddTarget(*itr);
}
int StgIntersectionObject::GetDxScriptObjectID() {
	int res = DxScript::ID_INVALID;
	StgEnemyObject* objEnemy = dynamic_cast<StgEnemyObject*>(this);
	if (objEnemy)
		res = objEnemy->GetObjectID();
	return res;
}

/**********************************************************
//StgIntersectionTarget
**********************************************************/
StgIntersectionTarget::StgIntersectionTarget() {
	//mortonNo_ = -1;
	ZeroMemory(&intersectionSpace_, sizeof(RECT));
}
std::wstring StgIntersectionTarget::GetInfoAsString() {
	std::wstring res;
	res += L"type[";
	switch (typeTarget_) {
	case TYPE_PLAYER:res += L"PLAYER"; break;
	case TYPE_PLAYER_SHOT:res += L"PLAYER_SHOT"; break;
	case TYPE_PLAYER_SPELL:res += L"PLAYER_SPELL"; break;
	case TYPE_ENEMY:res += L"ENEMY"; break;
	case TYPE_ENEMY_SHOT:res += L"ENEMY_SHOT"; break;
	}
	res += L"] ";

	res += L"shape[";
	switch (shape_) {
	case SHAPE_CIRCLE:res += L"CIRCLE"; break;
	case SHAPE_LINE:res += L"LINE"; break;
	}
	res += L"] ";

	res += StringUtility::Format(L"address[%08x] ", (int)this);

	res += L"obj[";
	if (obj_.expired()) {
		res += L"NULL";
	}
	else {
		shared_ptr<DxScriptObjectBase> dxObj = std::dynamic_pointer_cast<DxScriptObjectBase>(obj_.lock());

		if (dxObj == nullptr)
			res += L"UNKNOWN";
		else {
			int address = (int)dxObj.get();
			char* className = (char*)typeid(*this).name();
			res += StringUtility::Format(L"ref=%d, " L"delete=%s, active=%s, class=%s[%08x]",
				dxObj.use_count(),
				dxObj->IsDeleted() ? L"true" : L"false",
				dxObj->IsActive() ? L"true" : L"false",
				className, address);
		}
	}
	res += L"] ";

	return res;
}


#include "source/GcLib/pch.h"

#include "StgIntersection.hpp"
#include "StgShot.hpp"
#include "StgPlayer.hpp"
#include "StgEnemy.hpp"
#include "StgSystem.hpp"

/**********************************************************
//StgIntersectionManager
**********************************************************/
StgIntersectionManager::StgIntersectionManager() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG screenWidth = graphics->GetScreenWidth();
	LONG screenHeight = graphics->GetScreenWidth();

	//_CreatePool(2);
	listSpace_.resize(3);
	for (size_t iSpace = 0; iSpace < listSpace_.size(); iSpace++) {
		StgIntersectionSpace* space = new StgIntersectionSpace();
		space->Initialize(-100, -100, screenWidth + 100, screenHeight + 100);
		listSpace_[iSpace] = space;
	}

	{
		{
			ShaderManager* shaderManager = ShaderManager::GetBase();
			RenderShaderLibrary* shaderLib = shaderManager->GetRenderLib();

			shaderVisualizerCircle_ = shaderManager->CreateUnmanagedFromEffect(shaderLib->GetIntersectVisualShader1());
			shaderVisualizerLine_ = shaderManager->CreateUnmanagedFromEffect(shaderLib->GetIntersectVisualShader2());
		}

		{
			objIntersectionVisualizerCircle_ = std::make_shared<DxScriptParticleListObject2D>();
			objIntersectionVisualizerLine_ = std::make_shared<DxScriptPrimitiveObject2D>();

			countCircleInstance_ = 0U;
			countLineVertex_ = 0U;
			bRenderIntersection_ = false;

			{
				ParticleRenderer2D* objParticleCircle = objIntersectionVisualizerCircle_->GetParticlePointer();

				objIntersectionVisualizerCircle_->SetPrimitiveType(D3DPT_TRIANGLESTRIP);
				objIntersectionVisualizerCircle_->SetShader(shaderVisualizerCircle_);

				uint16_t numEdge = 48ui16;
				objIntersectionVisualizerCircle_->SetVertexCount(numEdge + 1U);
				{
					std::vector<uint16_t> index;
					index.resize(numEdge * 2U);
					for (uint16_t i = 0; i < numEdge; ++i) {
						index[i * 2U + 0] = i;
						index[i * 2U + 1] = 0;
					}
					index.push_back(1);
					index.push_back(0);
					objParticleCircle->SetVertexIndices(index);
				}

				VERTEX_TLX vert;
				vert.position = D3DXVECTOR4(0, 0, 1, 1);
				vert.texcoord = D3DXVECTOR2(0, 0);
				vert.diffuse_color = 0x80ffffff;
				objParticleCircle->RenderObjectTLX::SetVertex(0, vert);
				for (size_t i = 0; i < numEdge; ++i) {
					float angle = i / (float)numEdge * (float)GM_PI_X2;
					vert.position = D3DXVECTOR4(cosf(angle), sinf(angle), 1, 1);
					objParticleCircle->RenderObjectTLX::SetVertex(i + 1, vert);
				}

				/*
				objIntersectionVisualizerCircle_->SetVertexCount(4U);
				objParticleCircle->SetVertexIndices({ 0, 1, 2, 3 });

				VERTEX_TLX vert;
				vert.position = D3DXVECTOR4(-8, -8, 1, 1);
				vert.texcoord = D3DXVECTOR2(0, 0);
				vert.diffuse_color = 0xffffffff;
				objParticleCircle->RenderObjectTLX::SetVertex(0, vert);
				vert.position = D3DXVECTOR4(8, -8, 1, 1);
				objParticleCircle->RenderObjectTLX::SetVertex(1, vert);
				vert.position = D3DXVECTOR4(-8, 8, 1, 1);
				objParticleCircle->RenderObjectTLX::SetVertex(2, vert);
				vert.position = D3DXVECTOR4(8, 8, 1, 1);
				objParticleCircle->RenderObjectTLX::SetVertex(3, vert);
				*/
			}

			{
				objIntersectionVisualizerLine_->SetPrimitiveType(D3DPT_TRIANGLELIST);
				objIntersectionVisualizerLine_->SetVertexShaderRendering(true);
				objIntersectionVisualizerLine_->SetVertexCount(65536U);		//10922 max renders

				objIntersectionVisualizerLine_->SetShader(shaderVisualizerLine_);
			}
		}
	}
}
StgIntersectionManager::~StgIntersectionManager() {
	for (auto& itr : listSpace_) {
		ptr_delete(itr);
	}
	listSpace_.clear();
}
void StgIntersectionManager::Work() {
	objIntersectionVisualizerCircle_->CleanUp();
	objIntersectionVisualizerLine_->CleanUp();
	{
		RenderObjectTLX* objParticleLine = objIntersectionVisualizerLine_->GetObjectPointer();
		VERTEX_TLX* ptrVert = objParticleLine->GetVertex(0);
		memset(ptrVert, 0x00, sizeof(VERTEX_TLX) * countLineVertex_);
	}
	countCircleInstance_ = 0U;
	countLineVertex_ = 0U;

	listEnemyTargetPoint_ = listEnemyTargetPointNext_;
	listEnemyTargetPointNext_.clear();

	size_t totalCheck = 0;
	size_t totalTarget = 0;
	for (auto itr = listSpace_.begin(); itr != listSpace_.end(); itr++) {
		StgIntersectionSpace* space = *itr;

		size_t currentCheck = 0;
		auto listCheck = space->CreateIntersectionCheckList(this, currentCheck);

		for (size_t iCheck = 0; iCheck < currentCheck; iCheck++) {
			auto& cTargetPair = listCheck->at(iCheck);

			StgIntersectionTarget* targetA = cTargetPair.first;
			StgIntersectionTarget* targetB = cTargetPair.second;
			if (targetA == nullptr || targetB == nullptr) continue;

			if (IsIntersected(targetA, targetB)) {
				weak_ptr<StgIntersectionObject> objA = targetA->GetObject();
				weak_ptr<StgIntersectionObject> objB = targetB->GetObject();
				auto ptrA = objA.lock();
				auto ptrB = objB.lock();

				{
					if (ptrA) {
						ptrA->Intersect(targetA, targetB);
						ptrA->SetIntersected();
						if (ptrB) ptrA->AddIntersectedId(objB);
					}
					if (ptrB) {
						ptrB->Intersect(targetB, targetA);
						ptrB->SetIntersected();
						if (ptrA) ptrB->AddIntersectedId(objA);
					}
				}
			}
		}

		totalCheck += currentCheck;
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
void StgIntersectionManager::RenderVisualizer() {
	if (!bRenderIntersection_) return;
	if (countCircleInstance_ > 0U)
		objIntersectionVisualizerCircle_->Render();
	if (countLineVertex_ > 0U)
		objIntersectionVisualizerLine_->Render();
}
void StgIntersectionManager::AddTarget(shared_ptr<StgIntersectionTarget> target) {
	if (target == nullptr) return;
	if (shared_ptr<StgIntersectionObject> obj = target->GetObject().lock()) {
		StgIntersectionTarget::Type type = target->GetTargetType();
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
				//’eÁ‚µ”\—Í•t‰Á‚È‚ç
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

			shared_ptr<StgIntersectionTarget_Circle> circle = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);
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
void StgIntersectionManager::AddEnemyTargetToShot(shared_ptr<StgIntersectionTarget> target) {
	//target->SetMortonNumber(-1);
	//target->ClearObjectIntersectedIdList();

	StgIntersectionTarget::Type type = target->GetTargetType();
	switch (type) {
	case StgIntersectionTarget::TYPE_ENEMY:
	{
		listSpace_[SPACE_PLAYERSHOT_ENEMY]->RegistTargetB(target);

		shared_ptr<StgIntersectionTarget_Circle> circle = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);
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
void StgIntersectionManager::AddEnemyTargetToPlayer(shared_ptr<StgIntersectionTarget> target) {
	//target->SetMortonNumber(-1);
	//target->ClearObjectIntersectedIdList();

	StgIntersectionTarget::Type type = target->GetTargetType();
	switch (type) {
	case StgIntersectionTarget::TYPE_ENEMY:
	{
		listSpace_[SPACE_PLAYER_ENEMY]->RegistTargetB(target);
		break;
	}
	}
}

bool StgIntersectionManager::IsIntersected(StgIntersectionTarget* p1, StgIntersectionTarget* p2) {
	if (p1 != nullptr && p2 != nullptr) {
		StgIntersectionTarget::Shape shape1 = p1->GetShape();
		StgIntersectionTarget::Shape shape2 = p2->GetShape();
		if (shape1 == StgIntersectionTarget::SHAPE_CIRCLE && shape2 == StgIntersectionTarget::SHAPE_CIRCLE) {
			StgIntersectionTarget_Circle* c1 = dynamic_cast<StgIntersectionTarget_Circle*>(p1);
			StgIntersectionTarget_Circle* c2 = dynamic_cast<StgIntersectionTarget_Circle*>(p2);
			if (c1 == nullptr || c2 == nullptr) goto lab_fail;
			return DxMath::IsIntersected(c1->GetCircle(), c2->GetCircle());
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

			if (c == nullptr || l == nullptr) goto lab_fail;
			return DxMath::IsIntersected(c->GetCircle(), l->GetLine());
		}
		else if (shape1 == StgIntersectionTarget::SHAPE_LINE && shape2 == StgIntersectionTarget::SHAPE_LINE) {
			StgIntersectionTarget_Line* l1 = dynamic_cast<StgIntersectionTarget_Line*>(p1);
			StgIntersectionTarget_Line* l2 = dynamic_cast<StgIntersectionTarget_Line*>(p2);
			if (l1 == nullptr || l2 == nullptr) goto lab_fail;
			return DxMath::IsIntersected(l1->GetLine(), l2->GetLine());
		}
	}
lab_fail:
	return false;
}

void StgIntersectionManager::AddVisualization(shared_ptr<StgIntersectionTarget>& target) {
	if (!bRenderIntersection_ || target == nullptr) return;

	ParticleRenderer2D* objParticleCircle = objIntersectionVisualizerCircle_->GetParticlePointer();
	RenderObjectTLX* objParticleLine = objIntersectionVisualizerLine_->GetObjectPointer();

	D3DCOLOR color = 0xffffffff;
	switch (target->GetTargetType()) {
	case StgIntersectionTarget::TYPE_PLAYER:
	{
		if (dynamic_cast<StgIntersectionTarget_Player*>(target.get())->IsGraze())
			color = D3DCOLOR_ARGB(192, 48, 212, 48);
		else
			color = D3DCOLOR_XRGB(0, 255, 0);
		break;
	}
	case StgIntersectionTarget::TYPE_PLAYER_SHOT:
		color = D3DCOLOR_XRGB(32, 32, 255);
		break;
	case StgIntersectionTarget::TYPE_PLAYER_SPELL:
		color = D3DCOLOR_XRGB(24, 224, 255);
		break;
	case StgIntersectionTarget::TYPE_ENEMY:
		color = D3DCOLOR_XRGB(255, 240, 16);
		break;
	case StgIntersectionTarget::TYPE_ENEMY_SHOT:
		color = D3DCOLOR_XRGB(255, 16, 16);
		break;
	}

	switch (target->GetShape()) {
	case StgIntersectionTarget::SHAPE_CIRCLE:
	{
		StgIntersectionTarget_Circle* pTarget = dynamic_cast<StgIntersectionTarget_Circle*>(target.get());
		DxCircle& circle = pTarget->GetCircle();

		objParticleCircle->SetInstancePosition(circle.GetX(), circle.GetY(), 0.0f);
		objParticleCircle->SetInstanceScale(circle.GetR(), 1.0f, 1.0f);
		objParticleCircle->SetInstanceColor(color);

		objParticleCircle->AddInstance();
		++countCircleInstance_;

		break;
	}
	case StgIntersectionTarget::SHAPE_LINE:
	{
		if (countLineVertex_ >= (65536U / 6U) * 6U) break;

		StgIntersectionTarget_Line* pTarget = dynamic_cast<StgIntersectionTarget_Line*>(target.get());
		DxWidthLine& line = pTarget->GetLine();

		DxWidthLine splitLines[2];
		size_t countSplit = DxMath::SplitWidthLine(splitLines, &line, 1.0f, true);

		if (countSplit > 0U) {
			D3DXVECTOR2 posList[4] = {
				D3DXVECTOR2(splitLines[0].GetX1(), splitLines[0].GetY1()),
				D3DXVECTOR2(splitLines[0].GetX2(), splitLines[0].GetY2()),
				D3DXVECTOR2(splitLines[1].GetX1(), splitLines[1].GetY1()),
				D3DXVECTOR2(splitLines[1].GetX2(), splitLines[1].GetY2())
			};

			VERTEX_TLX verts[4];
			for (size_t i = 0; i < 4; ++i) {
				verts[i].position = D3DXVECTOR4(posList[i].x, posList[i].y, 1, 1);
				verts[i].texcoord = D3DXVECTOR2(0, 0);
				verts[i].diffuse_color = 0x80000000 | (color & 0x00ffffff);
			}
			objParticleLine->SetVertex(countLineVertex_ + 0, verts[0]);
			objParticleLine->SetVertex(countLineVertex_ + 1, verts[1]);
			objParticleLine->SetVertex(countLineVertex_ + 2, verts[2]);
			objParticleLine->SetVertex(countLineVertex_ + 3, verts[1]);
			objParticleLine->SetVertex(countLineVertex_ + 4, verts[2]);
			objParticleLine->SetVertex(countLineVertex_ + 5, verts[3]);
			countLineVertex_ += 6U;
		}

		break;
	}
	}
}

/*
void StgIntersectionManager::_ResetPoolObject(shared_ptr<StgIntersectionTarget>& obj) {
	//	ELogger::WriteTop(StringUtility::Format("_ResetPoolObject:start:%s)", obj->GetInfoAsString().c_str()));
	obj->obj_ = NULL;
	//	ELogger::WriteTop("_ResetPoolObject:end");
}
ref_count_ptr<StgIntersectionTarget>::unsync StgIntersectionManager::_CreatePoolObject(int type) {
	shared_ptr<StgIntersectionTarget> res = nullptr;
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
		std::list<shared_ptr<StgIntersectionTarget>>* listUsed = &listUsedPool_[iType];
		std::vector<shared_ptr<StgIntersectionTarget>>* listCache = &listCachePool_[iType];

		std::list<shared_ptr<StgIntersectionTarget>>::iterator itr = listUsed->begin();
		for (; itr != listUsed->end(); itr++) {
			shared_ptr<StgIntersectionTarget>& target = (*itr);
			ref_count_weak_ptr<DxScriptObjectBase>::unsync dxObj =
				ref_count_weak_ptr<DxScriptObjectBase>::unsync::Cast(target->GetObject());
			if (dxObj != NULL && dxObj->IsDeleted()) {
				ELogger::WriteTop(StringUtility::Format(L"%s(deleted):%s", funcName.c_str(), target->GetInfoAsString().c_str()));
			}
		}
	}
}
*/

/**********************************************************
//StgIntersectionCheckList
**********************************************************/
StgIntersectionCheckList::StgIntersectionCheckList() { 
	count_ = 0; 
}
StgIntersectionCheckList::~StgIntersectionCheckList() {
}
void StgIntersectionCheckList::AddTargetPair(shared_ptr<StgIntersectionTarget>& targetA, shared_ptr<StgIntersectionTarget>& targetB) {
	auto pair = std::make_pair(targetA, targetB);
	if (listTargetPair_.size() <= count_) {
		listTargetPair_.push_back(pair);
	}
	else {
		listTargetPair_[count_] = pair;
	}
	++count_;
}
shared_ptr<StgIntersectionTarget> StgIntersectionCheckList::GetTargetA(size_t index) {
	shared_ptr<StgIntersectionTarget> target = listTargetPair_[index].first;
	listTargetPair_[index].first = nullptr;
	return target;
}
shared_ptr<StgIntersectionTarget> StgIntersectionCheckList::GetTargetB(size_t index) {
	shared_ptr<StgIntersectionTarget> target = listTargetPair_[index].second;
	listTargetPair_[index].second = nullptr;
	return target;
}

/**********************************************************
//StgIntersectionSpace
**********************************************************/
StgIntersectionSpace::StgIntersectionSpace() {
	spaceRect_ = DxRect<double>(0, 0, 0, 0);
	previousCheckCreated_ = 0;
}
StgIntersectionSpace::~StgIntersectionSpace() {
}
bool StgIntersectionSpace::Initialize(double left, double top, double right, double bottom) {
	spaceRect_ = DxRect<double>(left, top, right, bottom);
	pooledCheckList_.resize(64U);
	return true;
}
bool StgIntersectionSpace::RegistTarget(ListTarget* pVec, shared_ptr<StgIntersectionTarget>& target) {
	if (!spaceRect_.IsIntersected(target->GetIntersectionSpaceRect()))
		return false;
	pVec->push_back(target);
	return true;
}
void StgIntersectionSpace::ClearTarget() {
	pairTargetList_.first.clear();
	pairTargetList_.second.clear();
	for (size_t i = 0; i < pooledCheckList_.size(); ++i) {
		if (i >= previousCheckCreated_) break;
		pooledCheckList_[i].first = nullptr;
		pooledCheckList_[i].second = nullptr;
	}
}

std::vector<StgIntersectionSpace::TargetCheckListPair>* StgIntersectionSpace::CreateIntersectionCheckList(
	StgIntersectionManager* manager, size_t& total) 
{
	ListTarget* pListTargetA = &pairTargetList_.first;
	ListTarget* pListTargetB = &pairTargetList_.second;

	CriticalSection& criticalSection = manager->GetLock();
	volatile unsigned int count = 0;

	if (manager->IsRenderIntersection()) {
		/*
		ParallelFor(pListTargetA->size(), [&](size_t i) {
			manager->AddVisualization(pListTargetA->at(i));
		});
		ParallelFor(pListTargetB->size(), [&](size_t i) {
			manager->AddVisualization(pListTargetB->at(i));
		});
		*/
		for (auto& pTarget : *pListTargetA)
			manager->AddVisualization(pTarget);
		for (auto& pTarget : *pListTargetB)
			manager->AddVisualization(pTarget);
	}

	if (pListTargetA->size() > 0 && pListTargetB->size() > 0) {
		auto CheckSpaceRect = [&](StgIntersectionTarget* targetA, StgIntersectionTarget* targetB) {
			if (targetA == nullptr || targetB == nullptr) return;
			const DxRect<LONG>& boundA = targetA->GetIntersectionSpaceRect();
			const DxRect<LONG>& boundB = targetB->GetIntersectionSpaceRect();
			if (boundA.IsIntersected(boundB)) {
				if ((size_t)count >= pooledCheckList_.size()) {
					Lock lock(criticalSection);
					pooledCheckList_.resize(pooledCheckList_.size() * 2);
				}
				pooledCheckList_[(size_t)count] = std::make_pair(targetA, targetB);
				InterlockedIncrement(&count);
			}
		};

		//Attempt to most efficiently utilize multithreading
		if (pListTargetA->size() >= pListTargetB->size()) {
			ParallelFor(pListTargetA->size(), [&](size_t iA) {
				StgIntersectionTarget* pTargetA = pListTargetA->at(iA).get();
				for (auto itrB = pListTargetB->begin(); itrB != pListTargetB->end(); ++itrB) {
					StgIntersectionTarget* pTargetB = itrB->get();
					CheckSpaceRect(pTargetA, pTargetB);
				}
			});
		}
		else {
			ParallelFor(pListTargetB->size(), [&](size_t iB) {
				StgIntersectionTarget* pTargetB = pListTargetB->at(iB).get();
				for (auto itrA = pListTargetA->begin(); itrA != pListTargetA->end(); ++itrA) {
					StgIntersectionTarget* pTargetA = itrA->get();
					CheckSpaceRect(pTargetA, pTargetB);
				}
			});
		}
	}

	total = (size_t)count;
	previousCheckCreated_ = total;
	return &pooledCheckList_;
}

/**********************************************************
//StgIntersectionObject
**********************************************************/
void StgIntersectionObject::AddIntersectionRelativeTarget(shared_ptr<StgIntersectionTarget> target) {
	IntersectionRelativeTarget newTarget;
	{
		DxShapeBase* pOrgShape = nullptr;
		switch (target->GetShape()) {
		case StgIntersectionTarget::SHAPE_CIRCLE:
			if (StgIntersectionTarget_Circle* pTarget = dynamic_cast<StgIntersectionTarget_Circle*>(target.get()))
				pOrgShape = new DxCircle(pTarget->GetCircle());
			break;
		case StgIntersectionTarget::SHAPE_LINE:
			if (StgIntersectionTarget_Line* pTarget = dynamic_cast<StgIntersectionTarget_Line*>(target.get()))
				pOrgShape = new DxWidthLine(pTarget->GetLine());
			break;
		}
		newTarget.orgShape = pOrgShape;		//Original
	}
	newTarget.orgIntersectionRect = target->GetIntersectionSpaceRect();
	newTarget.relTarget = target;			//Relative
	listRelativeTarget_.push_back(newTarget);
}
void StgIntersectionObject::ClearIntersectionRelativeTarget() {
	for (auto& iTargetPair : listRelativeTarget_)
		delete iTargetPair.orgShape;
	listRelativeTarget_.clear();
}
void StgIntersectionObject::UpdateIntersectionRelativeTarget(float posX, float posY, double angle) {
	//TODO: Make angle do something
	for (auto& iTargetList : listRelativeTarget_) {
		const DxShapeBase* pShapeOrg = iTargetList.orgShape;
		const DxRect<LONG>& pRectOrg = iTargetList.orgIntersectionRect;
		StgIntersectionTarget* targetRel = iTargetList.relTarget.get();

		StgIntersectionTarget::Shape shape = targetRel->GetShape();
		if (shape == StgIntersectionTarget::SHAPE_CIRCLE) {
			const DxCircle* pCircleOrg = dynamic_cast<const DxCircle*>(pShapeOrg);
			DxCircle& shapeRel = dynamic_cast<StgIntersectionTarget_Circle*>(targetRel)->GetCircle();

			shapeRel.SetX(pCircleOrg->GetX() + posX);
			shapeRel.SetY(pCircleOrg->GetY() + posY);
		}
		else if (shape == StgIntersectionTarget::SHAPE_LINE) {
			const DxWidthLine* pLineOrg = dynamic_cast<const DxWidthLine*>(pShapeOrg);
			DxWidthLine& shapeRel = dynamic_cast<StgIntersectionTarget_Line*>(targetRel)->GetLine();

			shapeRel.SetX1(pLineOrg->GetX1() + posX);
			shapeRel.SetY1(pLineOrg->GetY1() + posY);
			shapeRel.SetX2(pLineOrg->GetX2() + posX);
			shapeRel.SetY2(pLineOrg->GetY2() + posY);
		}
		else continue;

		targetRel->SetIntersectionSpace(DxRect<LONG>(pRectOrg.left + posX, pRectOrg.top + posY,
			pRectOrg.right + posX, pRectOrg.bottom + posY));
	}
}
void StgIntersectionObject::RegistIntersectionRelativeTarget(StgIntersectionManager* manager) {
	for (auto& iTargetList : listRelativeTarget_) {
		if (iTargetList.orgShape)
			manager->AddTarget(iTargetList.relTarget);
	}
}
int StgIntersectionObject::GetDxScriptObjectID() {
	int res = DxScript::ID_INVALID;
	DxScriptObjectBase* objBase = dynamic_cast<DxScriptObjectBase*>(this);
	if (objBase) res = objBase->GetObjectID();
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

void StgIntersectionTarget_Circle::SetIntersectionSpace() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	constexpr LONG margin = 16L;
	LONG screenWidth = graphics->GetScreenWidth() + margin;
	LONG screenHeight = graphics->GetScreenWidth() + margin;

	LONG x = circle_.GetX();
	LONG y = circle_.GetY();
	LONG r = circle_.GetR();

	LONG x1 = std::clamp(x - r, -margin, screenWidth);
	LONG x2 = std::clamp(x + r, -margin, screenWidth);
	LONG y1 = std::clamp(y - r, -margin, screenHeight);
	LONG y2 = std::clamp(y + r, -margin, screenHeight);
	intersectionSpace_ = { x1, y1, x2, y2 };
}
void StgIntersectionTarget_Line::SetIntersectionSpace() {
	float l = line_.GetX1();
	float t = line_.GetY1();
	float r = line_.GetX2();
	float b = line_.GetY2();
	float width = line_.GetWidth();

	if (l > r) std::swap(l, r);
	if (t > b) std::swap(t, b);
	l -= width;
	t -= width;
	r += width;
	b += width;

	DirectGraphics* graphics = DirectGraphics::GetBase();

	constexpr LONG margin = 16L;
	LONG screenWidth = graphics->GetScreenWidth() + margin;
	LONG screenHeight = graphics->GetScreenWidth() + margin;

	intersectionSpace_ = { 
		std::clamp((LONG)l, -margin, screenWidth),
		std::clamp((LONG)t, -margin, screenWidth),
		std::clamp((LONG)r, -margin, screenHeight),
		std::clamp((LONG)b, -margin, screenHeight)
	};
}
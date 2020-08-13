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

	omp_init_lock(&lock_);

	//_CreatePool(2);
	listSpace_.resize(3);
	for (size_t iSpace = 0; iSpace < listSpace_.size(); iSpace++) {
		StgIntersectionSpace* space = new StgIntersectionSpace();
		space->Initialize(-100, -100, screenWidth + 100, screenHeight + 100);
		listSpace_[iSpace] = space;
	}

	{
		{
			const std::string HLSL_VISUALIZER_CIRCLE =
				"sampler samp0_ : register(s0);"
				"float4x4 g_mWorldViewProj : WORLDVIEWPROJ : register(c0);"

				"struct VS_INPUT {"
					"float4 position : POSITION;"
					"float4 diffuse : COLOR0;"
					"float2 texCoord : TEXCOORD0;"
			
					"float4 i_color : COLOR1;"
					"float4 i_xyzpos_xsc : TEXCOORD1;"
					"float4 i_yzsc_xyang : TEXCOORD2;"
					"float4 i_zang_usdat : TEXCOORD3;"
				"};"
				"struct VS_OUTPUT {"
					"float4 position : POSITION;"
					"float4 diffuse : COLOR0;"
				"};"

				"VS_OUTPUT mainVS(VS_INPUT inVs) {"
					"VS_OUTPUT outVs;"
			
					"float t_scale = inVs.i_xyzpos_xsc.w;"

					"float4x4 instanceMat = float4x4("
						"float4(t_scale, 0, 0, 0),"
						"float4(0, t_scale, 0, 0),"
						"(float4)0,"
						"float4(inVs.i_xyzpos_xsc.xyz, 1)"
					");"

					"outVs.diffuse = inVs.diffuse * inVs.i_color;"
					"outVs.position = mul(inVs.position, instanceMat);"
					"outVs.position = mul(outVs.position, g_mWorldViewProj);"
					"outVs.position.z = 1.0f;"

					"return outVs;"
				"}"

				"float4 mainPS(VS_OUTPUT inPs) : COLOR0 {"
					"return inPs.diffuse;"
				"}"

				"technique Render {"
					"pass P0 {"
						"VertexShader = compile vs_2_0 mainVS();"
						"PixelShader = compile ps_2_0 mainPS();"
					"}"
				"}";
			const std::string HLSL_VISUALIZER_LINE =
				"sampler samp0_ : register(s0);"
				"float4x4 g_mWorld : WORLD : register(c0);"
				"float4x4 g_ViewProj : VIEWPROJECTION : register(c4);"

				"struct VS_INPUT {"
					"float4 position : POSITION;"
					"float4 diffuse : COLOR0;"
					"float2 texCoord : TEXCOORD0;"
				"};"
				"struct VS_OUTPUT {"
					"float4 position : POSITION;"
					"float4 diffuse : COLOR0;"
				"};"

				"VS_OUTPUT mainVS(VS_INPUT inVs) {"
					"VS_OUTPUT outVs;"

					"outVs.diffuse = inVs.diffuse;"
					"outVs.position = mul(inVs.position, g_mWorld);"
					"outVs.position = mul(outVs.position, g_ViewProj);"
					"outVs.position.z = 1.0f;"

					"return outVs;"
				"}"

				"float4 mainPS(VS_OUTPUT inPs) : COLOR0 {"
					"return inPs.diffuse;"
				"}"

				"technique Render {"
					"pass P0 {"
						"VertexShader = compile vs_2_0 mainVS();"
						"PixelShader = compile ps_2_0 mainPS();"
					"}"
				"}";

			ShaderManager* manager = ShaderManager::GetBase();
			shaderVisualizerCircle_ = manager->CreateFromText(HLSL_VISUALIZER_CIRCLE);
			shaderVisualizerLine_ = manager->CreateFromText(HLSL_VISUALIZER_LINE);
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

	omp_destroy_lock(&lock_);
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
		StgIntersectionCheckList* listCheck = space->CreateIntersectionCheckList(this, totalTarget);

		size_t countCheck = listCheck->GetCheckCount();

		for (size_t iCheck = 0; iCheck < countCheck; iCheck++) {
			StgIntersectionTarget::ptr targetA = listCheck->GetTargetA(iCheck);
			StgIntersectionTarget::ptr targetB = listCheck->GetTargetB(iCheck);

			if (targetA == nullptr || targetB == nullptr) continue;

			bool bIntersected = IsIntersected(targetA, targetB);
			if (!bIntersected) continue;

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
void StgIntersectionManager::RenderVisualizer() {
	if (!bRenderIntersection_) return;
	if (countCircleInstance_ > 0U)
		objIntersectionVisualizerCircle_->Render();
	if (countLineVertex_ > 0U)
		objIntersectionVisualizerLine_->Render();
}
void StgIntersectionManager::AddTarget(StgIntersectionTarget::ptr target) {
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

	StgIntersectionTarget::Type type = target->GetTargetType();
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

	StgIntersectionTarget::Type type = target->GetTargetType();
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
	StgIntersectionTarget::Shape shape1 = target1->GetShape();
	StgIntersectionTarget::Shape shape2 = target2->GetShape();
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

void StgIntersectionManager::AddVisualization(StgIntersectionTarget::ptr& target) {
	//if (!bRenderIntersection_) return;

	ParticleRenderer2D* objParticleCircle = objIntersectionVisualizerCircle_->GetParticlePointer();
	RenderObjectTLX* objParticleLine = objIntersectionVisualizerLine_->GetObjectPointer();

	D3DCOLOR color = 0xffffffff;
	switch (target->GetTargetType()) {
	case StgIntersectionTarget::TYPE_PLAYER:
		color = D3DCOLOR_XRGB(16, 255, 16);
		break;
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
		DxMath::SplitWidthLine(splitLines, &line);

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
		
		break;
	}
	}
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
//StgIntersectionCheckList
**********************************************************/
StgIntersectionCheckList::StgIntersectionCheckList() { 
	count_ = 0; 
}
StgIntersectionCheckList::~StgIntersectionCheckList() {
}
void StgIntersectionCheckList::AddTargetPair(StgIntersectionTarget::ptr& targetA, StgIntersectionTarget::ptr& targetB) {
	auto pair = std::make_pair(targetA, targetB);
	if (listTargetPair_.size() <= count_) {
		listTargetPair_.push_back(pair);
	}
	else {
		listTargetPair_[count_] = pair;
	}
	++count_;
}
StgIntersectionTarget::ptr StgIntersectionCheckList::GetTargetA(size_t index) {
	shared_ptr<StgIntersectionTarget> target = listTargetPair_[index].first;
	listTargetPair_[index].first = nullptr;
	return target;
}
StgIntersectionTarget::ptr StgIntersectionCheckList::GetTargetB(size_t index) {
	shared_ptr<StgIntersectionTarget> target = listTargetPair_[index].second;
	listTargetPair_[index].second = nullptr;
	return target;
}

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
void StgIntersectionSpace::ClearTarget() {
	listCell_[0].clear();
	listCell_[1].clear();
}

StgIntersectionCheckList* StgIntersectionSpace::CreateIntersectionCheckList(StgIntersectionManager* manager, size_t& total) {
	listCheck_->Clear();
	total += _WriteIntersectionCheckList(manager);
	return listCheck_;
}
size_t StgIntersectionSpace::_WriteIntersectionCheckList(StgIntersectionManager* manager) {
	std::atomic<size_t> count{0};

	std::vector<StgIntersectionTarget::ptr>& listTargetA = listCell_[0];
	std::vector<StgIntersectionTarget::ptr>& listTargetB = listCell_[1];
	auto itrA = listTargetA.begin();
	auto itrB = listTargetB.begin();

	omp_lock_t* ompLock = manager->GetLock();

	if (manager->IsRenderIntersection()) {
#pragma omp for
		for (int i = 0; i < listTargetA.size(); ++i) {
			StgIntersectionTarget::ptr& target = listTargetA[i];
			manager->AddVisualization(target);
		}
#pragma omp for
		for (int i = 0; i < listTargetB.size(); ++i) {
			StgIntersectionTarget::ptr& target = listTargetB[i];
			manager->AddVisualization(target);
		}
	}

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
				listCheck_->AddTargetPair(targetA, targetB);
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
			if (iType1 == iType2) continue;
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
	listRelativeTarget_.clear();
}
void StgIntersectionObject::AddIntersectionRelativeTarget(StgIntersectionTarget::ptr target) {
	listRelativeTarget_.push_back(target);
	StgIntersectionTarget::Shape shape = target->GetShape();
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

	for (auto& target : listRelativeTarget_) {
		StgIntersectionTarget::Shape shape = target->GetShape();
		if (shape == StgIntersectionTarget::SHAPE_CIRCLE) {
			StgIntersectionTarget_Circle* tTarget = dynamic_cast<StgIntersectionTarget_Circle*>(target.get());
			if (tTarget) {
				DxCircle& org = listOrgCircle_[iCircle];
				int px = (int)org.GetX() + posX;
				int py = (int)org.GetY() + posY;

				DxCircle& circle = tTarget->GetCircle();
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
	float x1 = line_.GetX1();
	float y1 = line_.GetY1();
	float x2 = line_.GetX2();
	float y2 = line_.GetY2();
	float width = line_.GetWidth();
	if (x1 > x2)
		std::swap(x1, x2);
	if (y1 > y2)
		std::swap(y1, y2);
	x1 -= width;
	x2 += width;
	y1 -= width;
	y2 += width;

	DirectGraphics* graphics = DirectGraphics::GetBase();

	constexpr LONG margin = 16L;
	LONG screenWidth = graphics->GetScreenWidth() + margin;
	LONG screenHeight = graphics->GetScreenWidth() + margin;

	LONG _x1 = std::clamp((LONG)x1, -margin, screenWidth);
	LONG _x2 = std::clamp((LONG)x2, -margin, screenWidth);
	LONG _y1 = std::clamp((LONG)y1, -margin, screenHeight);
	LONG _y2 = std::clamp((LONG)y2, -margin, screenHeight);
	intersectionSpace_ = { _x1, _y1, _x2, _y2 };
}
#pragma once

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgIntersection.hpp"

class StgItemDataList;
class StgItemObject;
class StgItemData;
class StgItemRenderer;
//*******************************************************************
//StgItemManager
//*******************************************************************
class StgItemManager {
	friend class StgItemRenderer;

	StgStageController* stageController_;

	SpriteList2D* listSpriteItem_;
	SpriteList2D* listSpriteDigit_;
	StgItemDataList* listItemData_;

	std::list<ref_unsync_ptr<StgItemObject>> listObj_;
	std::vector<std::pair<size_t, std::vector<StgItemObject*>>> listRenderQueue_;

	std::list<DxCircle> listCircleToPlayer_;

	DxRect<LONG> rcDeleteClip_;

	bool bAllItemToPlayer_;
	bool bCancelToPlayer_;
	bool bDefaultBonusItemEnable_;

	ID3DXEffect* effectLayer_;
	D3DXHANDLE handleEffectWorld_;
public:
	enum {
		ITEM_MAX = 8192 + 1024 + 256,
	};
public:
	StgItemManager(StgStageController* stageController);
	virtual ~StgItemManager();

	void Work();
	void Render(int targetPriority);
	void LoadRenderQueue();

	void AddItem(ref_unsync_ptr<StgItemObject> obj) {
		listObj_.push_back(obj); 
	}
	size_t GetItemCount() { return listObj_.size(); }

	SpriteList2D* GetItemRenderer() { return listSpriteItem_; }
	SpriteList2D* GetDigitRenderer() { return listSpriteDigit_; }

	StgItemDataList* GetItemDataList() { return listItemData_; }
	bool LoadItemData(const std::wstring& path, bool bReload = false);

	ref_unsync_ptr<StgItemObject> CreateItem(int type);

	void SetItemDeleteClip(const DxRect<LONG>& clip) { rcDeleteClip_ = clip; }
	DxRect<LONG>* GetItemDeleteClip() { return &rcDeleteClip_; }

	void CollectItemsAll();
	void CollectItemsInCircle(const DxCircle& circle);
	void CancelCollectItems();

	std::vector<int> GetItemIdInCircle(int cx, int cy, int* radius, int* itemType);

	bool IsDefaultBonusItemEnable() { return bDefaultBonusItemEnable_; }
	void SetDefaultBonusItemEnable(bool bEnable) { bDefaultBonusItemEnable_ = bEnable; }
};

//*******************************************************************
//StgItemDataList
//*******************************************************************
class StgItemDataList {
public:
	enum {
		RENDER_TYPE_COUNT = 3,
	};
private:
	std::set<std::wstring> listReadPath_;
	std::vector<shared_ptr<Texture>> listTexture_;
	std::vector<std::vector<StgItemRenderer*>> listRenderer_;
	std::vector<StgItemData*> listData_;

	void _ScanItem(std::vector<StgItemData*>& listData, Scanner& scanner);
	static void _ScanAnimation(StgItemData* itemData, Scanner& scanner);
public:
	StgItemDataList();
	virtual ~StgItemDataList();

	size_t GetTextureCount() { return listTexture_.size(); }
	shared_ptr<Texture> GetTexture(size_t index) { return listTexture_[index]; }
	StgItemRenderer* GetRenderer(size_t index, size_t typeRender) { return listRenderer_[typeRender][index]; }
	std::vector<StgItemRenderer*>& GetRendererList(size_t typeRender) { return listRenderer_[typeRender]; }

	StgItemData* GetData(int id) { return (id >= 0 && id < listData_.size()) ? listData_[id] : nullptr; }

	bool AddItemDataList(const std::wstring& path, bool bReload);
};

class StgItemData {
	friend StgItemDataList;
public:
	struct AnimationData {
		DxRect<LONG> rcSrc_;
		DxRect<float> rcDst_;
		size_t frame_;

		DxRect<LONG>* GetSource() { return &rcSrc_; }
		DxRect<float>* GetDest() { return &rcDst_; }

		static DxRect<float> SetDestRect(DxRect<LONG>* src);
	};
private:
	StgItemDataList* listItemData_;

	int indexTexture_;
	D3DXVECTOR2 textureSize_;

	int typeItem_;
	BlendMode typeRender_;

	int alpha_;

	AnimationData out_;

	std::vector<AnimationData> listAnime_;
	size_t totalAnimeFrame_;
public:
	StgItemData(StgItemDataList* listItemData);
	virtual ~StgItemData();

	int GetTextureIndex() { return indexTexture_; }
	D3DXVECTOR2& GetTextureSize() { return textureSize_; }

	int GetItemType() { return typeItem_; }
	BlendMode GetRenderType() { return typeRender_; }

	int GetAlpha() { return alpha_; }
	
	DxRect<LONG>* GetOutRect() { return &out_.rcSrc_; }
	DxRect<float>* GetOutDest() { return &out_.rcDst_; }

	AnimationData* GetData(size_t frame);
	size_t GetFrameCount() { return listAnime_.size(); }

	shared_ptr<Texture> GetTexture() { return listItemData_->GetTexture(indexTexture_); }
	StgItemRenderer* GetRenderer() { return GetRenderer(typeRender_); }
	StgItemRenderer* GetRenderer(BlendMode type);
};

//*******************************************************************
//StgItemRenderer
//*******************************************************************
class StgItemRenderer : public RenderObjectTLX {
	friend class StgItemManager;

	size_t countRenderVertex_;
	size_t countMaxVertex_;
public:
	StgItemRenderer();
	~StgItemRenderer();
	
	virtual void Render(StgItemManager* manager);
	void AddVertex(VERTEX_TLX& vertex);
	void AddSquareVertex(VERTEX_TLX* listVertex) {
		AddVertex(listVertex[0]);
		AddVertex(listVertex[2]);
		AddVertex(listVertex[1]);
		AddVertex(listVertex[1]);
		AddVertex(listVertex[2]);
		AddVertex(listVertex[3]);
	}

	virtual size_t GetVertexCount() {
		return std::min(countRenderVertex_, vertex_.size() / strideVertexStreamZero_);
	}
	virtual void SetVertexCount(size_t count) {
		vertex_.resize(count * strideVertexStreamZero_);
	}
};

//*******************************************************************
//StgItemObject
//*******************************************************************
class StgItemObject : public DxScriptRenderObject, public StgMoveObject, public StgIntersectionObject {
	friend class StgItemManager;
public:
	enum {
		ITEM_1UP = -256 * 256,
		ITEM_1UP_S,
		ITEM_SPELL,
		ITEM_SPELL_S,
		ITEM_POWER,
		ITEM_POWER_S,
		ITEM_POINT,
		ITEM_POINT_S,

		ITEM_SCORE,
		ITEM_BONUS,

		ITEM_USER = 0,

		COLLECT_PLAYER_SCOPE = 0,
		COLLECT_PLAYER_LINE,
		COLLECT_IN_CIRCLE,
		COLLECT_ALL,
		COLLECT_SINGLE,

		CANCEL_PLAYER_DOWN = 0,
		CANCEL_ALL,
		CANCEL_SINGLE,
	};
protected:
	StgStageController* stageController_;
	int typeItem_;
	//D3DCOLOR color_;

	int frameWork_;

	int64_t score_;

	bool bMoveToPlayer_;		//Is the item supposed to be homing in on the player?
	bool bPermitMoveToPlayer_;	//Can the item home in on the player?

	bool bDefaultScoreText_;

	bool bAutoDelete_;
	bool bIntersectEnable_;
	uint32_t itemIntersectRadius_;

	bool bDefaultCollectionMove_;
	bool bRoundingPosition_;

	void _DeleteInAutoClip();
	void _CreateScoreItem();
	void _NotifyEventToPlayerScript(gstd::value* listValue, size_t count);
	void _NotifyEventToItemScript(gstd::value* listValue, size_t count);
public:
	StgItemObject(StgStageController* stageController);

	virtual bool HasNormalRendering() { return false; }

	virtual void Work();
	virtual void Render() {}
	virtual void RenderOnItemManager();
	virtual void SetRenderState() {}
	virtual void Activate() {}

	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) = 0;

	virtual void SetX(float x) { posX_ = x; DxScriptRenderObject::SetX(x); }
	virtual void SetY(float y) { posY_ = y; DxScriptRenderObject::SetY(y); }
	virtual void SetColor(int r, int g, int b);
	virtual void SetAlpha(int alpha);
	void SetToPosition(D3DXVECTOR2& pos);

	int GetFrameWork() { return frameWork_; }

	int GetItemType() { return typeItem_; }
	void SetItemType(int type) { typeItem_ = type; }

	int64_t GetScore() { return score_; }
	void SetScore(int64_t score) { score_ = score; }

	bool IsMoveToPlayer() { return bMoveToPlayer_; }
	void SetMoveToPlayer(bool b) { bMoveToPlayer_ = b; }
	bool IsPermitMoveToPlayer() { return bPermitMoveToPlayer_; }
	void SetPermitMoveToPlayer(bool bPermit) { bPermitMoveToPlayer_ = bPermit; }

	void SetDefaultScoreText(bool b) { bDefaultScoreText_ = b; }

	void SetAutoDelete(bool b) { bAutoDelete_ = b; }
	void SetIntersectionEnable(bool b) { bIntersectEnable_ = b; }
	void SetIntersectionRadius(int r) { itemIntersectRadius_ = r * r; }

	bool IsIntersectionEnable() { return bIntersectEnable_;  }

	bool IsDefaultCollectionMovement() { return bDefaultCollectionMove_; }
	void SetDefaultCollectionMovement(bool b) { bDefaultCollectionMove_ = b; }

	void SetPositionRounding(bool b) { bRoundingPosition_ = b; }

	int GetMoveType();
	void SetMoveType(int type);

	void NotifyItemCollectEvent(int type, uint64_t eventParam);
	void NotifyItemCancelEvent(int type);

	StgStageController* GetStageController() { return stageController_; }
};

class StgItemObject_1UP : public StgItemObject {
public:
	StgItemObject_1UP(StgStageController* stageController);
	
	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);
};

class StgItemObject_Bomb : public StgItemObject {
public:
	StgItemObject_Bomb(StgStageController* stageController);
	
	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);
};

class StgItemObject_Power : public StgItemObject {
public:
	StgItemObject_Power(StgStageController* stageController);
	
	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);
};

class StgItemObject_Point : public StgItemObject {
public:
	StgItemObject_Point(StgStageController* stageController);
	
	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);
};

class StgItemObject_Bonus : public StgItemObject {
public:
	StgItemObject_Bonus(StgStageController* stageController);
	
	virtual void Work();
	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);
};

class StgItemObject_Score : public StgItemObject {
	int frameDelete_;
public:
	StgItemObject_Score(StgStageController* stageController);
	
	virtual void Work();
	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);
};

class StgItemObject_User : public StgItemObject {
	int frameWork_;
	int idImage_;

	StgItemData* _GetItemData();
	void _SetVertexPosition(VERTEX_TLX& vertex, float x, float y, float z = 1.0f, float w = 1.0f);
	void _SetVertexUV(VERTEX_TLX& vertex, float u, float v);
	void _SetVertexColorARGB(VERTEX_TLX& vertex, D3DCOLOR color);
public:
	StgItemObject_User(StgStageController* stageController);

	virtual void Work();
	virtual void RenderOnItemManager();

	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);

	void SetImageID(int id);
};

//*******************************************************************
//StgMovePattern_Item
//*******************************************************************
class StgMovePattern_Item : public StgMovePattern {
public:
	enum {
		MOVE_NONE,
		MOVE_TOPOSITION_A,	//Move to the specified target position
		MOVE_DOWN,			//Downwards, default
		MOVE_TOPLAYER,		//Yeet to player
		MOVE_SCORE,			//Yeet to player but score
	};

protected:
	int frame_;
	int typeMove_;
	double speed_;
	double angDirection_;

	D3DXVECTOR2 posTo_;
public:
	StgMovePattern_Item(StgMoveObject* target);

	virtual void Move();

	int GetType() { return TYPE_OTHER; }
	virtual double GetSpeed() { return speed_; }
	virtual double GetDirectionAngle() { return angDirection_; }
	void SetToPosition(D3DXVECTOR2& pos) { posTo_ = pos; }

	int GetItemMoveType() { return typeMove_; }
	void SetItemMoveType(int type) { typeMove_ = type; }
};

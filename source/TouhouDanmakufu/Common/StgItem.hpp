#ifndef __TOUHOUDANMAKUFU_DNHSTG_ITEM__
#define __TOUHOUDANMAKUFU_DNHSTG_ITEM__

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgIntersection.hpp"

class StgItemDataList;
class StgItemObject;
class StgItemData;
class StgItemRenderer;
/**********************************************************
//StgItemManager
**********************************************************/
class StgItemManager {
	friend class StgItemRenderer;

	StgStageController* stageController_;

	SpriteList2D* listSpriteItem_;
	SpriteList2D* listSpriteDigit_;
	StgItemDataList* listItemData_;

	std::list<shared_ptr<StgItemObject>> listObj_;
	std::set<int> listItemTypeToPlayer_;
	std::list<DxCircle> listCircleToPlayer_;
	bool bAllItemToPlayer_;
	bool bCancelToPlayer_;
	bool bDefaultBonusItemEnable_;

	ID3DXEffect* effectLayer_;
	D3DXHANDLE handleEffectWorld_;
public:
	enum {
		ITEM_MAX = 8192 + 1024 + 256,
	};

	StgItemManager(StgStageController* stageController);
	virtual ~StgItemManager();
	void Work();
	void Render(int targetPriority);

	void AddItem(shared_ptr<StgItemObject> obj) { 
		listObj_.push_back(obj); 
	}
	size_t GetItemCount() { return listObj_.size(); }

	SpriteList2D* GetItemRenderer() { return listSpriteItem_; }
	SpriteList2D* GetDigitRenderer() { return listSpriteDigit_; }
	void GetValidRenderPriorityList(std::vector<PriListBool>& list);

	StgItemDataList* GetItemDataList() { return listItemData_; }
	bool LoadItemData(std::wstring path, bool bReload = false);

	shared_ptr<StgItemObject> CreateItem(int type);

	void CollectItemsAll();
	void CollectItemsByType(int type);
	void CollectItemsInCircle(DxCircle circle);
	void CancelCollectItems();

	bool IsDefaultBonusItemEnable() { return bDefaultBonusItemEnable_; }
	void SetDefaultBonusItemEnable(bool bEnable) { bDefaultBonusItemEnable_ = bEnable; }
};

/**********************************************************
//StgItemDataList
**********************************************************/
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
	void _ScanAnimation(StgItemData* itemData, Scanner& scanner);
	std::vector<std::wstring> _GetArgumentList(Scanner& scanner);
public:
	StgItemDataList();
	virtual ~StgItemDataList();

	size_t GetTextureCount() { return listTexture_.size(); }
	shared_ptr<Texture> GetTexture(size_t index) { return listTexture_[index]; }
	StgItemRenderer* GetRenderer(size_t index, size_t typeRender) { return listRenderer_[typeRender][index]; }
	std::vector<StgItemRenderer*>& GetRendererList(size_t typeRender) { return listRenderer_[typeRender]; }

	StgItemData* GetData(int id) { return (id >= 0 && id < listData_.size()) ? listData_[id] : nullptr; }

	bool AddItemDataList(std::wstring path, bool bReload);
};

class StgItemData {
	friend StgItemDataList;
public:
	struct AnimationData {
		RECT rcSrc_;
		RECT rcDest_;
		int frame_;
	};
private:
	StgItemDataList* listItemData_;

	int indexTexture_;
	D3DXVECTOR2 textureSize_;

	int typeItem_;
	int typeRender_;
	RECT rcOutSrc_;
	RECT rcOutDest_;
	int alpha_;

	std::vector<AnimationData> listAnime_;
	int totalAnimeFrame_;
public:
	StgItemData(StgItemDataList* listItemData);
	virtual ~StgItemData();

	int GetTextureIndex() { return indexTexture_; }
	D3DXVECTOR2& GetTextureSize() { return textureSize_; }
	int GetItemType() { return typeItem_; }
	int GetRenderType() { return typeRender_; }
	AnimationData* GetData(int frame);
	RECT* GetOutSrc() { return &rcOutSrc_; }
	RECT* GetOutDest() { return &rcOutDest_; }
	int GetAlpha() { return alpha_; }

	shared_ptr<Texture> GetTexture() { return listItemData_->GetTexture(indexTexture_); }
	StgItemRenderer* GetRenderer() { return GetRenderer(typeRender_); }
	StgItemRenderer* GetRenderer(int type);
};

/**********************************************************
//StgItemRenderer
**********************************************************/
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



/**********************************************************
//StgItemObject
**********************************************************/
class StgItemObject : public DxScriptRenderObject, public StgMoveObject, public StgIntersectionObject {
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
	};
protected:
	StgStageController* stageController_;
	int typeItem_;
	//D3DCOLOR color_;

	int64_t score_;
	bool bMoveToPlayer_; //自機移動フラグ
	bool bPermitMoveToPlayer_; //自機自動回収許可
	bool bChangeItemScore_;

	int frameWork_;

	void _DeleteInAutoClip();
	void _CreateScoreItem();
	void _NotifyEventToPlayerScript(std::vector<float>& listValue);
	void _NotifyEventToItemScript(std::vector<float>& listValue);
public:
	StgItemObject(StgStageController* stageController);
	virtual void Work();
	virtual void Render() {}//一括で描画するためオブジェクト管理での描画はしない
	virtual void RenderOnItemManager();
	virtual void SetRenderState() {}
	virtual void Activate() {}

	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) = 0;

	virtual void SetX(float x) { posX_ = x; DxScriptRenderObject::SetX(x); }
	virtual void SetY(float y) { posY_ = y; DxScriptRenderObject::SetY(y); }
	virtual void SetColor(int r, int g, int b);
	virtual void SetAlpha(int alpha);
	void SetToPosition(POINT pos);

	int64_t GetScore() { return score_; }
	void SetScore(int64_t score) { score_ = score; }
	bool IsMoveToPlayer() { return bMoveToPlayer_; }
	void SetMoveToPlayer(bool b) { bMoveToPlayer_ = b; }
	bool IsPermitMoveToPlayer() { return bPermitMoveToPlayer_; }
	void SetPermitMoveToPlayer(bool bPermit) { bPermitMoveToPlayer_ = bPermit; }
	void SetChangeItemScore(bool b) { bChangeItemScore_ = b; }

	int GetMoveType();
	void SetMoveType(int type);

	int GetFrameWork() { return frameWork_; }

	int GetItemType() { return typeItem_; }
	void SetItemType(int type) { typeItem_ = type; }
	StgStageController* GetStageController() { return stageController_; }
};

class StgItemObject_1UP : public StgItemObject {
public:
	StgItemObject_1UP(StgStageController* stageController);
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);
};

class StgItemObject_Bomb : public StgItemObject {
public:
	StgItemObject_Bomb(StgStageController* stageController);
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);
};

class StgItemObject_Power : public StgItemObject {
public:
	StgItemObject_Power(StgStageController* stageController);
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);
};

class StgItemObject_Point : public StgItemObject {
public:
	StgItemObject_Point(StgStageController* stageController);
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);
};

class StgItemObject_Bonus : public StgItemObject {
public:
	StgItemObject_Bonus(StgStageController* stageController);
	virtual void Work();
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);
};

class StgItemObject_Score : public StgItemObject {
	int frameDelete_;
public:
	StgItemObject_Score(StgStageController* stageController);
	virtual void Work();
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);
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
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);

	void SetImageID(int id);
};
#pragma region StgItemObject_User_impl
inline void StgItemObject_User::_SetVertexPosition(VERTEX_TLX& vertex, float x, float y, float z, float w) {
	constexpr float bias = -0.5f;
	vertex.position.x = x + bias;
	vertex.position.y = y + bias;
	vertex.position.z = z;
	vertex.position.w = w;
}
inline void StgItemObject_User::_SetVertexUV(VERTEX_TLX& vertex, float u, float v) {
	vertex.texcoord.x = u;
	vertex.texcoord.y = v;
}
inline void StgItemObject_User::_SetVertexColorARGB(VERTEX_TLX& vertex, D3DCOLOR color) {
	vertex.diffuse_color = color;
}
#pragma endregion StgItemObject_User_impl

/**********************************************************
//StgMovePattern_Item
**********************************************************/
class StgMovePattern_Item : public StgMovePattern {
public:
	enum {
		MOVE_NONE,
		MOVE_TOPOSITION_A,//指定ポイントへの移動(60フレーム)
		MOVE_DOWN,//下降
		MOVE_TOPLAYER,//自機へ移動
		MOVE_SCORE,//得点(上昇)
	};

protected:
	int frame_;
	int typeMove_;
	double speed_;
	double angDirection_;

	POINT posTo_;
public:
	StgMovePattern_Item(StgMoveObject* target);
	virtual void Move();
	int GetType() { return TYPE_OTHER; }
	virtual double GetSpeed() { return speed_; }
	virtual double GetDirectionAngle() { return angDirection_; }
	void SetToPosition(POINT pos) { posTo_ = pos; }

	int GetItemMoveType() { return typeMove_; }
	void SetItemMoveType(int type) { typeMove_ = type; }
};


#endif


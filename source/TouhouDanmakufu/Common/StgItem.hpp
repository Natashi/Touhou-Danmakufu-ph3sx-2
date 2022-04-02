#pragma once

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgIntersection.hpp"
//#include "StgShot.hpp"

class StgShotVertexBufferContainer;

class StgItemDataList;
class StgItemData;
struct StgItemDataFrame;
class StgItemObject;
//*******************************************************************
//StgItemManager
//*******************************************************************
class StgItemManager {
	friend class StgItemRenderer;
public:
	enum {
		ITEM_MAX = 10000,

		BLEND_COUNT = 8,
	};
protected:
	static std::array<BlendMode, BLEND_COUNT> blendTypeRenderOrder;
	struct RenderQueue {
		size_t count;
		std::vector<StgItemObject*> listItem;
	};
protected:
	StgStageController* stageController_;

	unique_ptr<SpriteList2D> listSpriteItem_;
	unique_ptr<SpriteList2D> listSpriteDigit_;

	unique_ptr<StgItemDataList> listItemData_;

	std::list<ref_unsync_ptr<StgItemObject>> listObj_;
	std::vector<RenderQueue> listRenderQueue_;		//one for each render pri

	std::list<DxCircle> listCircleToPlayer_;

	DxRect<LONG> rcDeleteClip_;

	D3DTEXTUREFILTERTYPE filterMin_;
	D3DTEXTUREFILTERTYPE filterMag_;

	bool bAllItemToPlayer_;
	bool bCancelToPlayer_;
	bool bDefaultBonusItemEnable_;

	ID3DXEffect* effectItem_;
	D3DXMATRIX matProj_;
public:
	IDirect3DTexture9* pLastTexture_;
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

	ID3DXEffect* GetEffect() { return effectItem_; }
	D3DXMATRIX* GetProjectionMatrix() { return &matProj_; }

	SpriteList2D* GetItemRenderer() { return listSpriteItem_.get(); }
	SpriteList2D* GetDigitRenderer() { return listSpriteDigit_.get(); }

	StgItemDataList* GetItemDataList() { return listItemData_.get(); }

	bool LoadItemData(const std::wstring& path, bool bReload = false);

	ref_unsync_ptr<StgItemObject> CreateItem(int type);

	void SetItemDeleteClip(const DxRect<LONG>& clip) { rcDeleteClip_ = clip; }
	DxRect<LONG>* GetItemDeleteClip() { return &rcDeleteClip_; }

	void SetTextureFilter(D3DTEXTUREFILTERTYPE min, D3DTEXTUREFILTERTYPE mag) {
		filterMin_ = min;
		filterMag_ = mag;
	}

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
	//Repurpose StgShotVertexBufferContainer for this, since it'd have been the same code anyway
	using VBContainerList = std::list<unique_ptr<StgShotVertexBufferContainer>>;
private:
	std::map<std::wstring, VBContainerList> mapVertexBuffer_;	//<shot data file, vb list>
	std::vector<unique_ptr<StgItemData>> listData_;

	void _ScanItem(std::map<int, unique_ptr<StgItemData>>& mapData, Scanner& scanner);
	static void _ScanAnimation(StgItemData* itemData, Scanner& scanner);

	void _LoadVertexBuffers(std::map<std::wstring, VBContainerList>::iterator placement,
		shared_ptr<Texture> texture, std::vector<StgItemData*>& listAddData);
public:
	StgItemDataList();
	virtual ~StgItemDataList();

	StgItemData* GetData(int id) { return (id >= 0 && id < listData_.size()) ? listData_[id].get() : nullptr; }

	bool AddItemDataList(const std::wstring& path, bool bReload);
};

//*******************************************************************
//StgItemData
//*******************************************************************
class StgItemData {
	friend StgItemDataList;
private:
	StgItemDataList* listItemData_;

	int typeItem_;
	BlendMode typeRender_;

	int alpha_;

	unique_ptr<StgItemData> dataOut_;

	std::vector<StgItemDataFrame> listFrame_;
	size_t totalFrame_;
public:
	StgItemData(StgItemDataList* listItemData);
	virtual ~StgItemData();

	int GetItemType() { return typeItem_; }
	BlendMode GetRenderType() { return typeRender_; }

	int GetAlpha() { return alpha_; }
	
	StgItemData* GetOutData() { return dataOut_.get(); }

	StgItemDataFrame* GetFrame(size_t frame);
	size_t GetFrameCount() { return listFrame_.size(); }
};
struct StgItemDataFrame {
	StgItemDataList* listItemData_;

	StgShotVertexBufferContainer* pVertexBuffer_;
	DWORD vertexOffset_;

	DxRect<LONG> rcSrc_;
	DxRect<float> rcDst_;

	size_t frame_;
public:
	StgItemDataFrame();

	DxRect<LONG>* GetSourceRect() { return &rcSrc_; }
	DxRect<float>* GetDestRect() { return &rcDst_; }
	StgShotVertexBufferContainer* GetVertexBufferContainer() {
		return pVertexBuffer_;
	}

	static DxRect<float> LoadDestRect(DxRect<LONG>* src);
};

//*******************************************************************
//StgItemObject
//*******************************************************************
class StgItemObject : public DxScriptShaderObject, public StgMoveObject, public StgIntersectionObject {
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
	enum {
		FLAG_MOVETOPL_NONE				= 0x0,
		FLAG_MOVETOPL_PLAYER_SCOPE		= 0x1,
		FLAG_MOVETOPL_COLLECT_ALL		= 0x2,
		FLAG_MOVETOPL_POC_LINE			= 0x4,
		FLAG_MOVETOPL_COLLECT_CIRCLE	= 0x8,
		FLAG_MOVETOPL_ALL				= 0x1 | 0x2 | 0x4 | 0x8,
	};
protected:
	StgStageController* stageController_;
	int typeItem_;
	//D3DCOLOR color_;

	int frameWork_;

	int64_t score_;

	bool bMoveToPlayer_;		//Is the item supposed to be homing in on the player right now?
	int moveToPlayerFlags_;		//MoveToPlayer permissions

	bool bDefaultScoreText_;

	bool bAutoDelete_;
	bool bIntersectEnable_;
	uint32_t itemIntersectRadius_;

	bool bDefaultCollectionMove_;
	bool bRoundingPosition_;
protected:
	void _DeleteInAutoClip();
	void _CreateScoreItem();
	void _NotifyEventToPlayerScript(gstd::value* listValue, size_t count);
	void _NotifyEventToItemScript(gstd::value* listValue, size_t count);
public:
	StgItemObject(StgStageController* stageController);

	virtual bool HasNormalRendering() { return false; }

	virtual void Work();
	virtual void Activate() {}
	
	virtual void SetRenderState() {}
	virtual void Render() {};
	virtual void Render(BlendMode targetBlend) {};
	virtual void RenderOnItemManager();

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
	int GetMoveToPlayerEnableFlags() { return moveToPlayerFlags_; }
	void SetMoveToPlayerEnableFlags(int moveFlags) { moveToPlayerFlags_ = moveFlags; }

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

class StgItemObject_ScoreText : public StgItemObject {
	int frameDelete_;
public:
	StgItemObject_ScoreText(StgStageController* stageController);
	
	virtual void Work();
	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);
};

class StgItemObject_User : public StgItemObject {
	int frameWork_;
	int idImage_;

	weak_ptr<Texture> renderTarget_;
protected:
	inline StgItemData* _GetItemData();
public:
	StgItemObject_User(StgStageController* stageController);

	virtual void Work();

	virtual void Render(BlendMode targetBlend);
	virtual void RenderOnItemManager() {};

	virtual void SetRenderTarget(shared_ptr<Texture> texture) { renderTarget_ = texture; }

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

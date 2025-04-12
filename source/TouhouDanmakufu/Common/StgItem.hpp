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

enum class ItemType {
	OneUp = -256 * 256,
	OneUpSmall,
	Spell,
	SpellSmall,
	Power,
	PowerSmall,
	Point,
	PointSmall,

	ScoreText,	// Default score text objects
	Bonus,		// Default bullet cancel items

	User = 0,
};

//*******************************************************************
//StgItemManager
//*******************************************************************
class StgItemManager {
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

	ID3DXEffect* effectItem_;
	D3DXMATRIX matProj_;
public:
	bool bDefaultBonusItemEnable_;

	IDirect3DTexture9* pLastTexture_;
public:
	StgItemManager(StgStageController* stageController);

	void Work();
	void Render(int targetPriority);
	void LoadRenderQueue();

	void AddItem(const ref_unsync_ptr<StgItemObject>& obj) {
		listObj_.push_back(obj); 
	}
	_NODISCARD size_t GetItemCount() const { return listObj_.size(); }

	_NODISCARD ID3DXEffect* GetEffect() const { return effectItem_; }
	_NODISCARD D3DXMATRIX* GetProjectionMatrix() { return &matProj_; }

	_NODISCARD SpriteList2D* GetItemRenderer() const { return listSpriteItem_.get(); }
	_NODISCARD SpriteList2D* GetDigitRenderer() const { return listSpriteDigit_.get(); }

	_NODISCARD StgItemDataList* GetItemDataList() const { return listItemData_.get(); }

	bool LoadItemData(const std::wstring& path, bool bReload = false);

	ref_unsync_ptr<StgItemObject> CreateItem(ItemType type);

	void SetItemDeleteClip(const DxRect<LONG>& clip) { rcDeleteClip_ = clip; }
	DxRect<LONG>* GetItemDeleteClip() { return &rcDeleteClip_; }

	void SetTextureFilter(D3DTEXTUREFILTERTYPE min, D3DTEXTUREFILTERTYPE mag) {
		filterMin_ = min;
		filterMag_ = mag;
	}

	void CollectItemsAll();
	void CollectItemsInCircle(const DxCircle& circle);
	void CancelCollectItems();

	std::vector<int> GetItemIdInCircle(int cx, int cy,
		optional<int> radius, optional<ItemType> itemType);
};

//*******************************************************************
//StgItemDataList
//*******************************************************************
class StgItemDataList {
private:
	std::map<std::wstring, std::list<unique_ptr<StgShotVertexBufferContainer>>> mapVertexBuffer_;
	std::vector<unique_ptr<StgItemData>> listData_;

	void _ScanItem(std::map<int, unique_ptr<StgItemData>>& mapData, Scanner& scanner);
	static void _ScanAnimation(StgItemData* itemData, Scanner& scanner);

	void _LoadVertexBuffers(const std::wstring& name, shared_ptr<Texture> texture,
		const std::vector<StgItemData*>& listAddData);
public:
	StgItemData* GetData(int id) {
		return id >= 0 && id < listData_.size()
			? listData_[id].get()
			: nullptr;
	}

	bool AddItemDataList(const std::wstring& path, bool bReload);
};

//*******************************************************************
//StgItemData
//*******************************************************************
class StgItemData {
	friend StgItemDataList;
private:
	StgItemDataList* listItemData_;

	//int typeItem_;
	BlendMode typeRender_;

	int alpha_;

	unique_ptr<StgItemData> dataOut_;

	std::vector<StgItemDataFrame> listFrame_;
	size_t totalFrame_;
public:
	StgItemData(StgItemDataList* listItemData);

	//int GetItemType() { return typeItem_; }
	BlendMode GetRenderType() const { return typeRender_; }

	int GetAlpha() const { return alpha_; }
	
	StgItemData* GetOutData() { return dataOut_.get(); }

	StgItemDataFrame* GetFrame(size_t frame);
	size_t GetFrameCount() const { return listFrame_.size(); }
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
//StgMovePattern_Item
//*******************************************************************
class StgMovePattern_Item : public StgMovePattern {
public:
	enum class ItemMoveType {
		None,
		ToPosition,				// Move to the specified target position
		Down,					// Downwards, default
		ToPlayer,				// Yeet to player
		ScoreText,				// Yeet to player but score
	};

	ItemMoveType itemMoveType;
protected:
	int frame_;
	
	double speed_;
	double angDirection_;

	Math::DVec2 posTo_;
public:
	StgMovePattern_Item(StgMoveObject* target);

	StgMovePattern* Clone() const override {
		return new StgMovePattern_Item(*this);
	}

	void Activate(StgMovePattern* src) override {}
	void Move() override;

	double GetSpeed() const override { return speed_; }
	double GetDirectionAngle() const override { return angDirection_; }
	
	MovePatternType GetType() const override { return MovePatternType::Item; }
	
	void SetToPosition(const Math::DVec2& pos) { posTo_ = pos; }
};

//*******************************************************************
//StgItemObject
//*******************************************************************
class StgItemObject : public DxScriptShaderObject, public StgMoveObject, public StgIntersectionObject {
	friend StgItemManager;
public:
	enum class CollectType {
		PlayerScope,
		PlayerPoc,
		InCircle,
		Single,
		CollectAll,
	};
	enum class CancelType {
		PlayerDead,
		Single,
		CancelAll,
	};

	static constexpr int MoveToPlayerFlag_None = 0;
	static constexpr int MoveToPlayerFlag_CollectAllItems = 1 << 0;
	static constexpr int MoveToPlayerFlag_PlayerScope = 1 << 1;
	static constexpr int MoveToPlayerFlag_PlayerPoc = 1 << 2;
	static constexpr int MoveToPlayerFlag_Circle = 1 << 3;
	static constexpr int MoveToPlayerFlag_All =
		MoveToPlayerFlag_CollectAllItems |
		MoveToPlayerFlag_PlayerScope |
		MoveToPlayerFlag_PlayerPoc |
		MoveToPlayerFlag_Circle;
	
protected:
	int frameWork_;
public:
	ItemType itemType;
	
	int64_t score;
	bool useDefaultScoreText;

	bool isMovingToPlayer;		// Is the item supposed to be homing in on the player right now?
	int moveToPlayerFlags;		// MoveToPlayer permissions

	bool canAutoDelete;
	bool isIntersectEnable;
	uint32_t itemIntersectRadius;

	bool isDefaultCollectionMove;
	bool isRoundingPosition;
protected:
	void _DeleteInAutoClip();
	void _CreateScoreItem();
	void _NotifyEventToPlayerScript(gstd::value* listValue, size_t count);
	void _NotifyEventToItemScript(gstd::value* listValue, size_t count);
public:
	StgItemObject(StgStageController* stageController);

	void Clone(DxScriptObjectBase* src) override;

	bool HasNormalRendering() override { return false; }

	void Work() override;
	
	void SetRenderState() override {}
	void Render() override {};
	
	virtual void Render(BlendMode targetBlend) {};
	virtual void RenderOnItemManager();

	void SetX(float x) override { position[0] = x; DxScriptRenderObject::SetX(x); }
	void SetY(float y) override { position[1] = y; DxScriptRenderObject::SetY(y); }
	void SetColor(int r, int g, int b) override;
	void SetAlpha(int alpha) override;
	
	void SetToPosition(const Math::DVec2& pos);

	int GetFrameWork() const { return frameWork_; }

	StgMovePattern_Item::ItemMoveType GetMoveType();
	void SetMoveType(StgMovePattern_Item::ItemMoveType type);

	void NotifyItemCollectEvent(CollectType type, uint64_t eventParam);
	void NotifyItemCancelEvent(CancelType type);
};

class StgItemObject_1UP : public StgItemObject {
public:
	StgItemObject_1UP(StgStageController* stageController);
	
	void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) override;
};

class StgItemObject_Bomb : public StgItemObject {
public:
	StgItemObject_Bomb(StgStageController* stageController);
	
	void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) override;
};

class StgItemObject_Power : public StgItemObject {
public:
	StgItemObject_Power(StgStageController* stageController);
	
	void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) override;
};

class StgItemObject_Point : public StgItemObject {
public:
	StgItemObject_Point(StgStageController* stageController);
	
	void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) override;
};

class StgItemObject_Bonus : public StgItemObject {
public:
	StgItemObject_Bonus(StgStageController* stageController);
	
	void Work() override;
	void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) override;
};

class StgItemObject_ScoreText : public StgItemObject {
	int frameDelete_;
public:
	StgItemObject_ScoreText(StgStageController* stageController);
	
	void Work() override;
	void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) override;
};

class StgItemObject_User : public StgItemObject {
	int idImage_;

	weak_ptr<Texture> renderTarget_;
protected:
	inline StgItemData* _GetItemData();
public:
	StgItemObject_User(StgStageController* stageController);

	void Clone(DxScriptObjectBase* src) override;

	void Work() override;

	void Render(BlendMode targetBlend) override;
	void RenderOnItemManager() override {};

	void SetRenderTarget(shared_ptr<Texture> texture) override { renderTarget_ = texture; }

	void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) override;

	void SetImageID(int id);
};

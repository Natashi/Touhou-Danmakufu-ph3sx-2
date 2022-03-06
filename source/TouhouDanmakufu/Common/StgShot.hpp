#pragma once

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgIntersection.hpp"

class StgShotDataList;
class StgShotData;
struct StgShotDataFrame;
class StgShotVertexBufferContainer;
class StgShotObject;
//*******************************************************************
//StgShotManager
//*******************************************************************
class StgShotManager {
	friend class StgShotVertexBufferContainer;
public:
	enum {
		DEL_TYPE_ALL,
		DEL_TYPE_SHOT,
		DEL_TYPE_CHILD,
		TO_TYPE_IMMEDIATE,
		TO_TYPE_FADE,
		TO_TYPE_ITEM,
	};

	enum {
		BIT_EV_DELETE_IMMEDIATE = 1,
		BIT_EV_DELETE_TO_ITEM,
		BIT_EV_DELETE_FADE,
		BIT_EV_DELETE_COUNT,
	};

	enum {
		SHOT_MAX = 10000,

		BLEND_COUNT = 8,
	};
protected:
	static std::array<BlendMode, BLEND_COUNT> blendTypeRenderOrder;
	struct RenderQueue {
		size_t count;
		std::vector<StgShotObject*> listShot;
	};
protected:
	StgStageController* stageController_;

	unique_ptr<StgShotDataList> listPlayerShotData_;
	unique_ptr<StgShotDataList> listEnemyShotData_;

	std::list<ref_unsync_ptr<StgShotObject>> listObj_;
	std::vector<RenderQueue> listRenderQueuePlayer_;		//one for each render pri
	std::vector<RenderQueue> listRenderQueueEnemy_;			//one for each render pri

	std::bitset<BIT_EV_DELETE_COUNT> listDeleteEventEnable_;

	DxRect<LONG> rcDeleteClip_;

	D3DTEXTUREFILTERTYPE filterMin_;
	D3DTEXTUREFILTERTYPE filterMag_;

	ID3DXEffect* effectShot_;
	D3DXMATRIX matProj_;
public:
	StgShotManager(StgStageController* stageController);
	virtual ~StgShotManager();

	void Work();
	void Render(int targetPriority);
	void LoadRenderQueue();

	void RegistIntersectionTarget();

	void AddShot(ref_unsync_ptr<StgShotObject> obj);

	ID3DXEffect* GetEffect() { return effectShot_; }
	D3DXMATRIX* GetProjectionMatrix() { return &matProj_; }

	StgShotDataList* GetPlayerShotDataList() { return listPlayerShotData_.get(); }
	StgShotDataList* GetEnemyShotDataList() { return listEnemyShotData_.get(); }

	bool LoadPlayerShotData(const std::wstring& path, bool bReload = false);
	bool LoadEnemyShotData(const std::wstring& path, bool bReload = false);

	void SetShotDeleteClip(const DxRect<LONG>& clip) { rcDeleteClip_ = clip; }
	DxRect<LONG>* GetShotDeleteClip() { return &rcDeleteClip_; }

	void SetTextureFilter(D3DTEXTUREFILTERTYPE min, D3DTEXTUREFILTERTYPE mag) {
		filterMin_ = min;
		filterMag_ = mag;
	}

	void DeleteInCircle(int typeDelete, int typeTo, int typeOwner, int cx, int cy, int* radius);
	std::vector<int> GetShotIdInCircle(int typeOwner, int cx, int cy, int* radius);
	size_t GetShotCount(int typeOwner);
	size_t GetShotCountAll() { return listObj_.size(); }

	void SetDeleteEventEnableByType(int type, bool bEnable);
	bool IsDeleteEventEnable(int bit) { return listDeleteEventEnable_[bit]; }
};

//*******************************************************************
//StgShotDataList
//*******************************************************************
class StgShotDataList {
public:
	using VBContainerList = std::list<unique_ptr<StgShotVertexBufferContainer>>;
protected:
	std::map<std::wstring, VBContainerList> mapVertexBuffer_;	//<shot data file, vb list>
	std::vector<unique_ptr<StgShotData>> listData_;

	int defaultDelayData_;
	D3DCOLOR defaultDelayColor_;

	void _ScanShot(std::map<int, unique_ptr<StgShotData>>& mapData, Scanner& scanner);
	static void _ScanAnimation(StgShotData* shotData, Scanner& scanner);

	void _LoadVertexBuffers(std::map<std::wstring, VBContainerList>::iterator placement, 
		shared_ptr<Texture> texture, std::vector<StgShotData*>& listAddData);
public:
	StgShotDataList();
	virtual ~StgShotDataList();

	StgShotData* GetData(int id) { return (id >= 0 && id < listData_.size()) ? listData_[id].get() : nullptr; }

	bool AddShotDataList(const std::wstring& path, bool bReload);
};

//*******************************************************************
//StgShotData
//*******************************************************************
class StgShotData {
	friend StgShotDataList;
private:
	StgShotDataList* listShotData_;

	BlendMode typeRender_;
	BlendMode typeDelayRender_;

	int alpha_;

	int idDefaultDelay_;
	D3DCOLOR colorDelay_;

	std::vector<StgShotDataFrame> listFrame_;
	size_t totalFrame_;

	std::vector<DxCircle> listCol_;

	double angularVelocityMin_;
	double angularVelocityMax_;
	bool bFixedAngle_;
public:
	StgShotData(StgShotDataList* listShotData);
	virtual ~StgShotData();

	StgShotDataList* GetShotDataList() { return listShotData_; }

	BlendMode GetRenderType() { return typeRender_; }
	BlendMode GetDelayRenderType() { return typeDelayRender_; }

	int GetAlpha() { return alpha_; }

	int GetDefaultDelayID() { return idDefaultDelay_; }
	D3DCOLOR GetDelayColor() { return colorDelay_; }

	StgShotDataFrame* GetFrame(size_t frame);
	size_t GetFrameCount() { return listFrame_.size(); }

	const std::vector<DxCircle>& GetIntersectionCircleList() { return listCol_; }

	double GetAngularVelocityMin() { return angularVelocityMin_; }
	double GetAngularVelocityMax() { return angularVelocityMax_; }
	bool IsFixedAngle() { return bFixedAngle_; }
};
struct StgShotDataFrame {
	StgShotDataList* listShotData_;

	StgShotVertexBufferContainer* pVertexBuffer_;
	DWORD vertexOffset_;

	DxRect<LONG> rcSrc_;
	DxRect<float> rcDst_;

	size_t frame_;
public:
	StgShotDataFrame();

	DxRect<LONG>* GetSourceRect() { return &rcSrc_; }
	DxRect<float>* GetDestRect() { return &rcDst_; }
	StgShotVertexBufferContainer* GetVertexBufferContainer() {
		return pVertexBuffer_;
	}

	static DxRect<float> LoadDestRect(DxRect<LONG>* src);
};

//*******************************************************************
//StgShotVertexBufferContainer
//*******************************************************************
class StgShotVertexBufferContainer {
	friend class StgShotDataList;
public:
	enum {
		MAX_DATA = 2048,
		STRIDE = 4 * sizeof(VERTEX_TLX),	//Approx 230kB per buffer object max
	};
private:
	FixedVertexBuffer* pVertexBuffer_;
	size_t countData_;

	shared_ptr<Texture> texture_;
public:
	StgShotVertexBufferContainer();
	~StgShotVertexBufferContainer();

	HRESULT LoadData(const std::vector<VERTEX_TLX>& data, size_t countFrame);

	FixedVertexBuffer* GetBufferObject() { return pVertexBuffer_; }
	IDirect3DVertexBuffer9* GetD3DBuffer() { return pVertexBuffer_ ? pVertexBuffer_->GetBuffer() : nullptr; }
	size_t GetDataCount() { return countData_; }

	void SetTexture(shared_ptr<Texture> texture) { texture_ = texture; }
	shared_ptr<Texture> GetTexture() { return texture_; }
	IDirect3DTexture9* GetD3DTexture() { return texture_ ? texture_->GetD3DTexture() : nullptr; }
};

//*******************************************************************
//StgShotObject
//*******************************************************************
struct StgPatternShotTransform;
class StgShotObject : public DxScriptShaderObject, public StgMoveObject, public StgIntersectionObject {
	friend StgPatternShotTransform;
public:
	enum {
		OWNER_PLAYER = 0,
		OWNER_ENEMY,
		OWNER_NULL,

		FRAME_FADEDELETE = 30,
		FRAME_FADEDELETE_LASER = 30,
	};
public:
	struct DelayParameter {
		using lerp_func = Math::Lerp::funcLerp<float, float>;
		enum {
			DELAY_DEFAULT,
			DELAY_LERP,
		};

		int time;
		int id;
		BlendMode blend;
		D3DXVECTOR3 scale;	//[end, start, factor]
		D3DXVECTOR3 alpha;	//[end, start, factor]
		D3DCOLOR colorRep;
		bool colorMix;
		D3DXVECTOR2 angle;	//[angle, spin]

		uint8_t type;		//0 = default danmakufu, 1 = ZUN-like
		lerp_func scaleLerpFunc;	//Scale interpolation
		lerp_func alphaLerpFunc;	//Alpha interpolation

		DelayParameter() : time(0), id(-1), blend(MODE_BLEND_NONE), type(0), colorMix(false) {
			scale = D3DXVECTOR3(0.5f, 2.0f, 15.0f);
			alpha = D3DXVECTOR3(1.0f, 1.0f, 15.0f);
			scaleLerpFunc = Math::Lerp::Linear<float, float>;
			alphaLerpFunc = Math::Lerp::Linear<float, float>;
			colorRep = 0x00000000;
			angle = D3DXVECTOR2(0, 0);
		}
		DelayParameter(float sMin, float sMax, float rate) : time(0), id(-1), blend(MODE_BLEND_NONE), type(0), colorMix(false) {
			scale = D3DXVECTOR3(sMin, sMax, rate);
			alpha = D3DXVECTOR3(1.0f, 1.0f, 15.0f);
			scaleLerpFunc = Math::Lerp::Linear<float, float>;
			alphaLerpFunc = Math::Lerp::Linear<float, float>;
			colorRep = 0x00000000;
			angle = D3DXVECTOR2(0, 0);
		}
		DelayParameter& operator=(const DelayParameter& source) = default;

		inline float GetScale() { return _CalculateValue(&scale, scaleLerpFunc); }
		inline float GetAlpha() { return _CalculateValue(&alpha, alphaLerpFunc); }
		float _CalculateValue(D3DXVECTOR3* param, lerp_func func);

	};
protected:
	StgStageController* stageController_;

	ref_unsync_weak_ptr<StgShotObject> pOwnReference_;

	int frameWork_;
	int idShotData_;
	int typeOwner_;

	D3DXVECTOR2 move_;	//[cos, sin]
	double lastAngle_;

	D3DXVECTOR2 hitboxScale_;

	DelayParameter delay_;

	int frameGrazeInvalid_;
	int frameGrazeInvalidStart_;
	int frameFadeDelete_;

	bool bPenetrateShot_; // Translation: Does The Shot Lose Penetration Points Upon Colliding With Another Shot And Not An Enemy

	weak_ptr<Texture> renderTarget_;
private:
	struct _WeakPtrHasher {
		std::size_t operator()(const ref_unsync_weak_ptr<StgEnemyObject>& k) const {
			return std::hash<StgEnemyObject*>{}(k.get());
		}
	};
public:
	uint32_t frameEnemyHitInvalid_;
	std::unordered_map<ref_unsync_weak_ptr<StgEnemyObject>, uint32_t, _WeakPtrHasher> mapEnemyHitCooldown_;
	
	bool bRequestedPlayerDeleteEvent_;
	double damage_;
	double life_;

	bool bAutoDelete_;
	bool bEraseShot_;
	bool bSpellFactor_;
	bool bSpellResist_;
	int frameAutoDelete_;
	
	IntersectionListType listIntersectionTarget_;
	bool bUserIntersectionMode_;
	bool bIntersectionEnable_;
	bool bChangeItemEnable_;

	bool bEnableMotionDelay_;
	bool bRoundingPosition_;
	double roundingAngle_;
public:
	StgShotData* _GetShotData() { return _GetShotData(idShotData_); }
	inline StgShotData* _GetShotData(int id);

	static void _SetVertexPosition(VERTEX_TLX* vertex, float x, float y, float z = 1.0f, float w = 1.0f);
	static void _SetVertexUV(VERTEX_TLX* vertex, float u, float v);
	static void _SetVertexColorARGB(VERTEX_TLX* vertex, D3DCOLOR color);
protected:
	virtual void _DeleteInLife();
	virtual void _DeleteInAutoClip();
	virtual void _DeleteInFadeDelete();
	virtual void _DeleteInAutoDeleteFrame();
	void _CommonWorkTask();

	virtual void _Move();

	virtual void _SendDeleteEvent(int type) {}
	void _RequestPlayerDeleteEvent(int hitObjectID);

	inline void _DefaultShotRender(StgShotData* shotData, StgShotDataFrame* shotFrame, const D3DXMATRIX& matWorld, D3DCOLOR color);
protected:
	std::list<StgPatternShotTransform> listTransformationShotAct_;
	int timerTransform_;
	int timerTransformNext_;

	void _ProcessTransformAct();
public:
	StgShotObject(StgStageController* stageController);
	virtual ~StgShotObject();

	virtual bool HasNormalRendering() { return false; }

	virtual void Work();
	virtual void Activate() {}

	virtual void Render() {};
	virtual void Render(BlendMode targetBlend) = 0;

	virtual void SetRenderTarget(shared_ptr<Texture> texture) { renderTarget_ = texture; }

	virtual void DeleteImmediate();
	virtual void ConvertToItem();

	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget);
	virtual void ClearShotObject() { ClearIntersectionRelativeTarget(); }
	virtual void RegistIntersectionTarget() = 0;

	virtual void SetX(float x) { posX_ = x; DxScriptRenderObject::SetX(x); }
	virtual void SetY(float y) { posY_ = y; DxScriptRenderObject::SetY(y); }
	virtual void SetColor(int r, int g, int b);
	virtual void SetAlpha(int alpha);
	virtual void SetRenderState() {}

	void SetTransformList(const std::list<StgPatternShotTransform>& listTransform) {
		listTransformationShotAct_ = listTransform;
	}

	void SetOwnObjectReference();

	int GetShotDataID() { return idShotData_; }
	virtual void SetShotDataID(int id) { idShotData_ = id; }
	int GetOwnerType() { return typeOwner_; }
	void SetOwnerType(int type) { typeOwner_ = type; }

	void SetGrazeInvalidFrame(int frame) { frameGrazeInvalidStart_ = frame; }
	int GetGrazeInvalidFrame() { return frameGrazeInvalidStart_; }
	void SetGrazeFrame(int frame) { frameGrazeInvalid_ = frame; }
	bool IsValidGraze() { return frameGrazeInvalid_ <= 0; }

	void SetPenetrateShotEnable(bool enable) { bPenetrateShot_ = enable; }
	bool GetPenetrateShotEnable() { return bPenetrateShot_; }

	void SetEnemyIntersectionInvalidFrame(uint32_t frame) { frameEnemyHitInvalid_ = frame; }
	uint32_t GetEnemyIntersectionInvalidFrame() { return frameEnemyHitInvalid_;  }

	//Returns true if obj is on hit cooldown
	bool CheckEnemyHitCooldownExists(ref_unsync_weak_ptr<StgEnemyObject> obj);
	void AddEnemyHitCooldown(ref_unsync_weak_ptr<StgEnemyObject> obj, uint32_t time);

	int GetDelay() { return delay_.time; }
	void SetDelay(int delay) { delay_.time = delay; }
	int GetShotDataDelayID() { return delay_.id; }
	void SetShotDataDelayID(int id) { delay_.id = id; }
	BlendMode GetDelayBlendType() { return delay_.blend; }
	void SetDelayBlendType(BlendMode type) { delay_.blend = type; }
	DelayParameter* GetDelayParameter() { return &delay_; }
	void SetDelayParameter(DelayParameter& param) { delay_ = param; }
	void SetEnableDelayMotion(bool b) { bEnableMotionDelay_ = b; }
	void SetDelayAngularVelocity(float av) { delay_.angle.y = av; }

	double GetLife() { return life_; }
	void SetLife(double life) { life_ = life; }
	double GetDamage() { return damage_; }
	void SetDamage(double damage) { damage_ = damage; }
	virtual void SetFadeDelete() { if (frameFadeDelete_ < 0) frameFadeDelete_ = FRAME_FADEDELETE; }
	bool IsAutoDelete() { return bAutoDelete_; }
	void SetAutoDelete(bool b) { bAutoDelete_ = b; }
	void SetAutoDeleteFrame(int frame) { frameAutoDelete_ = frame; }
	bool IsEraseShot() { return bEraseShot_; }
	void SetEraseShot(bool bErase) { bEraseShot_ = bErase; }
	bool IsSpellFactor() { return bSpellFactor_; }
	void SetSpellFactor(bool bSpell) { bSpellFactor_ = bSpell; }
	bool IsSpellResist() { return bSpellResist_; }
	void SetSpellResist(bool bSpell) { bSpellResist_ = bSpell; }

	void SetUserIntersectionMode(bool b) { bUserIntersectionMode_ = b; }
	void SetIntersectionEnable(bool b) { bIntersectionEnable_ = b; }
	bool IsIntersectionEnable() { return bIntersectionEnable_; }
	void SetItemChangeEnable(bool b) { bChangeItemEnable_ = b; }

	void SetPositionRounding(bool b) { bRoundingPosition_ = b; }
	void SetAngleRounding(double a) { roundingAngle_ = a; }

	void SetHitboxScale(D3DXVECTOR2& sc) { hitboxScale_ = sc; }
	void SetHitboxScaleX(float x) { hitboxScale_.x = x; }
	void SetHitboxScaleY(float y) { hitboxScale_.y = y; }
	float GetHitboxScaleX() { return hitboxScale_.x; }
	float GetHitboxScaleY() { return hitboxScale_.y; }
};

//*******************************************************************
//StgNormalShotObject
//*******************************************************************
class StgNormalShotObject : public StgShotObject {
	friend class StgShotObject;
protected:
	double angularVelocity_;
	bool bFixedAngle_;

	void _AddIntersectionRelativeTarget();
	virtual void _SendDeleteEvent(int type);
public:
	StgNormalShotObject(StgStageController* stageController);
	virtual ~StgNormalShotObject();

	virtual void Work();
	virtual void Render(BlendMode targetBlend);

	virtual void ClearShotObject() {
		ClearIntersectionRelativeTarget();
	}
	virtual void RegistIntersectionTarget() {
		if (!bUserIntersectionMode_)
			_AddIntersectionRelativeTarget();
	}
	virtual IntersectionListType GetIntersectionTargetList();
	virtual bool GetIntersectionTargetList_NoVector(StgShotData* shotData);

	virtual void SetShotDataID(int id);
	void SetGraphicAngularVelocity(double agv) { angularVelocity_ = agv; }
	void SetFixedAngle(bool fix) { bFixedAngle_ = fix; }
};

//*******************************************************************
//StgLaserObject(レーザー基本部)
//*******************************************************************
class StgLaserObject : public StgShotObject {
protected:
	int length_;

	//Edge-to-edge
	int widthRender_;
	int widthIntersection_;

	//[0.0, 1.0] from laser head to middle
	float invalidLengthStart_;
	//[0.0, 1.0] from laser tail to middle
	float invalidLengthEnd_;
	float itemDistance_;

	void _AddIntersectionRelativeTarget();
public:
	StgLaserObject(StgStageController* stageController);

	virtual void ClearShotObject() {
		ClearIntersectionRelativeTarget();
	}
	virtual void RegistIntersectionTarget() {
		if (!bUserIntersectionMode_)
			_AddIntersectionRelativeTarget();
	}

	virtual IntersectionListType GetIntersectionTargetList();
	virtual bool GetIntersectionTargetList_NoVector(StgShotData* shotData) { return false; }

	int GetLength() { return length_; }
	void SetLength(int length) { length_ = length; }
	int GetRenderWidth() { return widthRender_; }
	void SetRenderWidth(int width) {
		width = std::max(width, 0);
		widthRender_ = width;
		if (widthIntersection_ < 0) widthIntersection_ = width / 4;
	}
	int GetIntersectionWidth() { return widthIntersection_; }
	void SetIntersectionWidth(int width) { widthIntersection_ = std::max(width, 0); }

	void SetInvalidLength(float start, float end) { invalidLengthStart_ = start; invalidLengthEnd_ = end; }

	void SetItemDistance(float dist) { itemDistance_ = std::max(dist, 0.1f); }
};

//*******************************************************************
//StgLooseLaserObject
//*******************************************************************
class StgLooseLaserObject : public StgLaserObject {
protected:
	Math::DVec2 posTail_;
	D3DXVECTOR2 posOrigin_;

	double currentLength_;

	virtual void _DeleteInAutoClip();
	virtual void _Move();
	virtual void _SendDeleteEvent(int type);
public:
	StgLooseLaserObject(StgStageController* stageController);

	virtual void Work();
	virtual void Render(BlendMode targetBlend);

	virtual bool GetIntersectionTargetList_NoVector(StgShotData* shotData);

	virtual void SetX(float x) { StgShotObject::SetX(x); posTail_[0] = x; }
	virtual void SetY(float y) { StgShotObject::SetY(y); posTail_[1] = y; }
};

//*******************************************************************
//StgStraightLaserObject (as opposed to StgGayLaserObject)
//*******************************************************************
class StgStraightLaserObject : public StgLaserObject {
protected:
	double angLaser_;

	bool bUseSouce_;
	bool bUseEnd_;
	int idImageEnd_;

	D3DXVECTOR2 delaySize_;

	float scaleX_;

	bool bLaserExpand_;

	virtual void _DeleteInAutoClip();
	virtual void _DeleteInAutoDeleteFrame();
	virtual void _SendDeleteEvent(int type);
public:
	StgStraightLaserObject(StgStageController* stageController);

	virtual void Work();
	virtual void Render(BlendMode targetBlend);

	virtual bool GetIntersectionTargetList_NoVector(StgShotData* shotData);

	double GetLaserAngle() { return angLaser_; }
	void SetLaserAngle(double angle) { angLaser_ = angle; }
	void SetFadeDelete() { if (frameFadeDelete_ < 0) frameFadeDelete_ = FRAME_FADEDELETE_LASER; }

	void SetSourceEnable(bool bEnable) { bUseSouce_ = bEnable; }
	void SetEndEnable(bool bEnable) { bUseEnd_ = bEnable; }
	void SetEndGraphic(int gr) { idImageEnd_ = gr; }

	void SetSourceEndScale(const D3DXVECTOR2& s) { delaySize_ = s; }

	void SetLaserExpand(bool b) { bLaserExpand_ = b; }
	bool GetLaserExpand() { return bLaserExpand_; }
};

//*******************************************************************
//StgCurveLaserObject (curvy lasers)
//*******************************************************************
class StgCurveLaserObject : public StgLaserObject {
public:
	struct LaserNode {
		StgCurveLaserObject* parent;
		D3DXVECTOR2 pos;
		D3DXVECTOR2 vertOff[2];
		D3DCOLOR color;
		float widthMul = 1.0f;
	};
	enum {
		MAP_NORMAL,
		MAP_CAPPED
	};
protected:
	std::list<LaserNode> listPosition_;
	std::vector<VERTEX_TLX> vertexData_;
	std::vector<float> listRectIncrement_;

	float posXO_;
	float posYO_;

	float tipDecrement_;
	bool bCap_;

	D3DXVECTOR2 posOrigin_;

	virtual void _DeleteInAutoClip();
	virtual void _Move();
	virtual void _SendDeleteEvent(int type);
public:
	StgCurveLaserObject(StgStageController* stageController);

	virtual void Work();
	virtual void Render(BlendMode targetBlend);

	virtual bool GetIntersectionTargetList_NoVector(StgShotData* shotData);

	void SetTipDecrement(float dec) { tipDecrement_ = dec; }
	void SetTipCapping(bool enable) { bCap_ = enable; }

	LaserNode CreateNode(const D3DXVECTOR2& pos, const D3DXVECTOR2& rFac, float widthMul, D3DCOLOR col = 0xffffffff);
	bool GetNode(size_t indexNode, std::list<LaserNode>::iterator& res);
	void GetNodePointerList(std::vector<LaserNode*>* listRes);
	std::list<LaserNode>::iterator PushNode(const LaserNode& node);
};


//*******************************************************************
//StgPatternShotObjectGenerator (ECL-style bullets firing)
//*******************************************************************
class StgPatternShotObjectGenerator : public DxScriptObjectBase {
public:
	enum {
		PATTERN_TYPE_FAN = 0,
		PATTERN_TYPE_FAN_AIMED,
		PATTERN_TYPE_RING,
		PATTERN_TYPE_RING_AIMED,
		PATTERN_TYPE_ARROW,
		PATTERN_TYPE_ARROW_AIMED,
		PATTERN_TYPE_POLYGON,
		PATTERN_TYPE_POLYGON_AIMED,
		PATTERN_TYPE_ELLIPSE,
		PATTERN_TYPE_ELLIPSE_AIMED,
		PATTERN_TYPE_SCATTER_ANGLE,
		PATTERN_TYPE_SCATTER_SPEED,
		PATTERN_TYPE_SCATTER,
        PATTERN_TYPE_LINE,
        PATTERN_TYPE_LINE_AIMED,
		PATTERN_TYPE_ROSE,
		PATTERN_TYPE_ROSE_AIMED,

		BASEPOINT_RESET = -256 * 256,
	};
private:
	ref_unsync_weak_ptr<StgMoveObject> parent_;

	int idShotData_;
	int typeOwner_;
	TypeObject typeShot_;
	int typePattern_;
	BlendMode iniBlendType_;

	size_t shotWay_;
	size_t shotStack_;

	//Calculate the sets in order-------------------------------------
	//Set 1
	float basePointX_;
	float basePointY_;
	//Set 2
	float basePointOffsetX_;
	float basePointOffsetY_;
	//Set 4
	float fireRadiusOffset_;
	//-----------------------------------------------------------------

	double speedBase_;
	double speedArgument_;
	double angleBase_;
	double angleArgument_;

	int delay_;
	//bool delayMove_;

	int laserWidth_;
	int laserLength_;

	std::vector<StgPatternShotTransform> listTransformation_;
public:
	StgPatternShotObjectGenerator();
	~StgPatternShotObjectGenerator();

	virtual void Render() {}
	virtual void SetRenderState() {}

	void CopyFrom(ref_unsync_ptr<StgPatternShotObjectGenerator> other) {
		StgPatternShotObjectGenerator::CopyFrom(other.get());
	}
	void CopyFrom(StgPatternShotObjectGenerator* other);

	void AddTransformation(StgPatternShotTransform& entry) { listTransformation_.push_back(entry); }
	void SetTransformation(size_t off, StgPatternShotTransform& entry);
	void ClearTransformation() { listTransformation_.clear(); }

	void SetParent(ref_unsync_ptr<StgMoveObject> obj) { parent_ = obj; }

	void FireSet(void* scriptData, StgStageController* controller, std::vector<int>* idVector);

	void SetGraphic(int id) { idShotData_ = id; }
	void SetTypeOwner(int type) { typeOwner_ = type; }
	void SetTypePattern(int type) { typePattern_ = type; }
	void SetTypeShot(TypeObject type) { typeShot_ = type; }
	void SetBlendType(BlendMode type) { iniBlendType_ = type; }

	void SetWayStack(size_t way, size_t stack) {
		shotWay_ = way;
		shotStack_ = stack;
	};

	void SetBasePoint(float bx, float by) {
		basePointX_ = bx;
		basePointY_ = by;
	}
	void SetOffsetFromBasePoint(float ox, float oy) {
		basePointOffsetX_ = ox;
		basePointOffsetY_ = oy;
	}
	void SetRadiusFromFirePoint(float r) { fireRadiusOffset_ = r; }

	void SetSpeed(double base, double arg) {
		speedBase_ = base;
		speedArgument_ = arg;
	}
	void SetAngle(double base, double arg) {
		angleBase_ = base;
		angleArgument_ = arg;
	}

	void SetDelay(int delay) { delay_ = delay; }
	//void SetDelayMotion(bool b) { delayMove_ = b; }
	void SetLaserArgument(int width, int length) {
		laserWidth_ = width;
		laserLength_ = length;
	}
};
struct StgPatternShotTransform {
	enum : uint8_t {
		TRANSFORM_WAIT,
		TRANSFORM_ADD_SPEED_ANGLE,
		TRANSFORM_ANGULAR_MOVE,
		TRANSFORM_N_DECEL_CHANGE,
		TRANSFORM_GRAPHIC_CHANGE,
		TRANSFORM_BLEND_CHANGE,
		TRANSFORM_TO_SPEED_ANGLE,
		TRANSFORM_ADDPATTERN_A1,
		TRANSFORM_ADDPATTERN_A2,
		TRANSFORM_ADDPATTERN_B1,
		TRANSFORM_ADDPATTERN_B2,
		TRANSFORM_ADDPATTERN_C1,
		TRANSFORM_ADDPATTERN_C2,
		//TRANSFORM_,
	};
	uint8_t act = 0xff;
	double param[8];
};
#ifndef __TOUHOUDANMAKUFU_DNHSTG_SHOT__
#define __TOUHOUDANMAKUFU_DNHSTG_SHOT__

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgIntersection.hpp"

class StgShotDataList;
class StgShotData;
class StgShotRenderer;
class StgShotObject;
/**********************************************************
//StgShotManager
**********************************************************/
class StgShotManager {
	//uint16_t RTARGET_VERT_INDICES[6];
	//size_t vertRTw;
	//size_t vertRTh;
	friend class StgShotRenderer;
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
		SHOT_MAX = 8192,
	};
protected:
	StgStageController* stageController_;
	StgShotDataList* listPlayerShotData_;
	StgShotDataList* listEnemyShotData_;
	std::list<shared_ptr<StgShotObject>> listObj_;

	std::bitset<BIT_EV_DELETE_COUNT> listDeleteEventEnable_;

	ID3DXEffect* effectLayer_;
	D3DXHANDLE handleEffectWorld_;
public:
	StgShotManager(StgStageController* stageController);
	virtual ~StgShotManager();
	void Work();
	void Render(int targetPriority);
	void RegistIntersectionTarget();

	void AddShot(shared_ptr<StgShotObject> obj);

	StgShotDataList* GetPlayerShotDataList() { return listPlayerShotData_; }
	StgShotDataList* GetEnemyShotDataList() { return listEnemyShotData_; }

	bool LoadPlayerShotData(std::wstring path, bool bReload = false);
	bool LoadEnemyShotData(std::wstring path, bool bReload = false);

	RECT GetShotAutoDeleteClipRect();

	void DeleteInCircle(int typeDelete, int typeTo, int typeOnwer, float cx, float cy, float radius);
	std::vector<int> GetShotIdInCircle(int typeOnwer, float cx, float cy, float radius);
	size_t GetShotCount(int typeOnwer);
	size_t GetShotCountAll() { return listObj_.size(); }

	void GetValidRenderPriorityList(std::vector<PriListBool>& list);

	void SetDeleteEventEnableByType(int type, bool bEnable);
	bool IsDeleteEventEnable(int bit) { return listDeleteEventEnable_[bit]; }
};

/**********************************************************
//StgShotDataList
**********************************************************/
class StgShotDataList {
public:
	enum {
		RENDER_TYPE_COUNT = 8,
	};
private:
	std::set<std::wstring> listReadPath_;
	std::vector<shared_ptr<Texture>> listTexture_;
	std::vector<std::vector<StgShotRenderer*>> listRenderer_;
	std::vector<StgShotData*> listData_;

	D3DCOLOR defaultDelayColor_;

	void _ScanShot(std::vector<StgShotData*>& listData, Scanner& scanner);
	void _ScanAnimation(StgShotData*& shotData, Scanner& scanner);
	std::vector<std::wstring> _GetArgumentList(Scanner& scanner);
public:
	StgShotDataList();
	virtual ~StgShotDataList();

	size_t GetTextureCount() { return listTexture_.size(); }
	shared_ptr<Texture> GetTexture(int index) { return listTexture_[index]; }
	StgShotRenderer* GetRenderer(int index, int typeRender) { return listRenderer_[typeRender][index]; }
	std::vector<StgShotRenderer*>& GetRendererList(int typeRender) { return listRenderer_[typeRender]; }

	StgShotData* GetData(int id) { return (id >= 0 && id < listData_.size()) ? listData_[id] : NULL; }

	bool AddShotDataList(std::wstring path, bool bReload);
};

class StgShotData {
	friend StgShotDataList;
public:
	struct AnimationData {
		RECT rcSrc_;
		RECT rcDst_;
		int frame_;

		RECT* GetSource() { return &rcSrc_; }
		RECT* GetDest() { return &rcDst_; }
	};
private:
	StgShotDataList* listShotData_;

	int indexTexture_;
	D3DXVECTOR2 textureSize_;

	int typeRender_;
	int typeDelayRender_;

	RECT rcDelay_;
	RECT rcDstDelay_;

	int alpha_;
	D3DCOLOR colorDelay_;
	DxCircle listCol_;

	std::vector<AnimationData> listAnime_;
	int totalAnimeFrame_;

	double angularVelocityMin_;
	double angularVelocityMax_;
	bool bFixedAngle_;
public:
	StgShotData(StgShotDataList* listShotData);
	virtual ~StgShotData();

	int GetTextureIndex() { return indexTexture_; }
	D3DXVECTOR2& GetTextureSize() { return textureSize_; }
	int GetRenderType() { return typeRender_; }
	int GetDelayRenderType() { return typeDelayRender_; }
	AnimationData* GetData(int frame);
	RECT* GetDelayRect() { return &rcDelay_; }
	RECT* GetDelayDest() { return &rcDstDelay_; }
	int GetAlpha() { return alpha_; }
	D3DCOLOR GetDelayColor() { return colorDelay_; }
	DxCircle* GetIntersectionCircleList() { return &listCol_; }
	double GetAngularVelocityMin() { return angularVelocityMin_; }
	double GetAngularVelocityMax() { return angularVelocityMax_; }
	bool IsFixedAngle() { return bFixedAngle_; }

	int GetFrameCount() { return listAnime_.size(); }

	shared_ptr<Texture> GetTexture() { return listShotData_->GetTexture(indexTexture_); }
	StgShotRenderer* GetRenderer() { return GetRenderer(typeRender_); }
	StgShotRenderer* GetRenderer(int type);
};

/**********************************************************
//StgShotRenderer
**********************************************************/
class StgShotRenderer : public RenderObjectTLX {
	friend class StgShotManager;

	std::vector<uint32_t> vecIndex_;

	size_t countMaxVertex_;
	size_t countRenderVertex_;
	size_t countMaxIndex_;
	size_t countRenderIndex_;
public:
	StgShotRenderer();
	~StgShotRenderer();

	virtual void Render(StgShotManager* manager);
	void AddSquareVertex(VERTEX_TLX* listVertex);
	void AddSquareVertex_CurveLaser(VERTEX_TLX* listVertex, bool bAddIndex);

	virtual size_t GetVertexCount() {
		return std::min(countRenderVertex_, vertex_.size() / strideVertexStreamZero_);
	}
	virtual void SetVertexCount(size_t count) {
		vertex_.resize(count * strideVertexStreamZero_);
	}
private:
	inline void AddVertex(VERTEX_TLX& vertex) {
		VERTEX_TLX* data = (VERTEX_TLX*)&vertex_[0];
		memcpy((VERTEX_TLX*)(data + countRenderVertex_), &vertex, strideVertexStreamZero_);
		++countRenderVertex_;
	}
	void TryExpandVertex(size_t chk) {
		//Expands the vertex buffer if its size is insufficient for the next batch
		if (chk < countMaxVertex_ - 6U) return;
		countMaxVertex_ *= 2U;
		SetVertexCount(countMaxVertex_);
	}
	void TryExpandIndex(size_t chk) {
		//Expands the index buffer if its size is insufficient for the next batch
		if (chk < countMaxIndex_ - 6U) return;
		countMaxIndex_ *= 2U;
		vecIndex_.resize(countMaxIndex_);
	}
};

/**********************************************************
//StgShotObject
**********************************************************/
struct StgPatternShotTransform;
class StgShotObject : public DxScriptRenderObject, public StgMoveObject, public StgIntersectionObject {
	friend StgPatternShotTransform;
public:
	enum {
		OWNER_PLAYER = 0,
		OWNER_ENEMY,
		OWNER_NULL,

		FRAME_FADEDELETE = 30,
		FRAME_FADEDELETE_LASER = 30,

		LIFE_SPELL_UNREGIST = 5,
		LIFE_SPELL_REGIST = 256 * 256 * 256,
	};
	class ReserveShotList;
	class ReserveShotListData;
protected:
	StgStageController* stageController_;

	weak_ptr<StgShotObject> pOwnReference_;

	int frameWork_;
	int idShotData_;
	int idShotDelay_;
	int typeOwner_;

	float c_;
	float s_;
	double lastAngle_;

	D3DXVECTOR2 hitboxScale_;

	int delay_;
	int typeSourceBlend_;
	int frameGrazeInvalid_;
	int frameFadeDelete_;

	double damage_;
	double life_;

	bool bAutoDelete_;
	bool bEraseShot_;
	bool bSpellFactor_;
	int frameAutoDelete_;
	ref_count_ptr<ReserveShotList>::unsync listReserveShot_;

	shared_ptr<StgIntersectionTarget> pShotIntersectionTarget_;
	bool bUserIntersectionMode_;//ユーザ定義あたり判定モード
	bool bIntersectionEnable_;
	bool bChangeItemEnable_;

	bool bEnableMotionDelay_;
	bool bRoundingPosition_;

	StgShotData* _GetShotData();
	void _SetVertexPosition(VERTEX_TLX& vertex, float x, float y, float z = 1.0f, float w = 1.0f);
	void _SetVertexUV(VERTEX_TLX& vertex, float u, float v);
	void _SetVertexColorARGB(VERTEX_TLX& vertex, D3DCOLOR color);
	virtual void _DeleteInLife();
	virtual void _DeleteInAutoClip();
	virtual void _DeleteInFadeDelete();
	virtual void _DeleteInAutoDeleteFrame();
	virtual void _Move();
	void _AddReservedShotWork();
	virtual void _AddReservedShot(shared_ptr<StgShotObject> obj, ReserveShotListData* data);
	virtual void _ConvertToItemAndSendEvent(bool flgPlayerCollision) {}
	virtual void _SendDeleteEvent(int bit);

	std::list<StgPatternShotTransform> listTransformationShotAct_;
	int timerTransform_;
	int timerTransformNext_;
	void _ProcessTransformAct();
public:
	StgShotObject(StgStageController* stageController);
	virtual ~StgShotObject();

	virtual void Work();
	virtual void Render() {}//一括で描画するためオブジェクト管理での描画はしない
	virtual void Activate() {}
	virtual void RenderOnShotManager() {}
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);
	virtual void ClearShotObject() { ClearIntersectionRelativeTarget(); }
	virtual void RegistIntersectionTarget() = 0;

	virtual void SetX(float x) { posX_ = x; DxScriptRenderObject::SetX(x); }
	virtual void SetY(float y) { posY_ = y; DxScriptRenderObject::SetY(y); }
	virtual void SetColor(int r, int g, int b);
	virtual void SetAlpha(int alpha);
	virtual void SetRenderState() {}

	void SetTransformList(std::list<StgPatternShotTransform>& listTransform) {
		listTransformationShotAct_ = listTransform;
	}

	void SetOwnObjectReference();

	int GetShotDataID() { return idShotData_; }
	virtual void SetShotDataID(int id) { idShotData_ = id; }
	int GetShotDataDelayID() { return idShotDelay_; }
	void SetShotDataDelayID(int id) { idShotDelay_ = id; }
	int GetOwnerType() { return typeOwner_; }
	void SetOwnerType(int type) { typeOwner_ = type; }

	bool IsValidGraze() { return frameGrazeInvalid_ <= 0; }
	int GetDelay() { return delay_; }
	void SetDelay(int delay) { delay_ = delay; }
	int GetSourceBlendType() { return typeSourceBlend_; }
	void SetSourceBlendType(int type) { typeSourceBlend_ = type; }
	double GetLife() { return life_; }
	void SetLife(double life) { life_ = life; }
	double GetDamage() { return damage_; }
	void SetDamage(double damage) { damage_ = damage; }
	virtual void SetFadeDelete() { if (frameFadeDelete_ < 0)frameFadeDelete_ = FRAME_FADEDELETE; }
	bool IsAutoDelete() { return bAutoDelete_; }
	void SetAutoDelete(bool b) { bAutoDelete_ = b; }
	void SetAutoDeleteFrame(int frame) { frameAutoDelete_ = frame; }
	bool IsEraseShot() { return bEraseShot_; }
	void SetEraseShot(bool bErase) { bEraseShot_ = bErase; }
	bool IsSpellFactor() { return bSpellFactor_; }
	void SetSpellFactor(bool bSpell) { bSpellFactor_ = bSpell; }
	void SetUserIntersectionMode(bool b) { bUserIntersectionMode_ = b; }
	void SetIntersectionEnable(bool b) { bIntersectionEnable_ = b; }
	bool IsIntersectionEnable() { return bIntersectionEnable_; }
	void SetItemChangeEnable(bool b) { bChangeItemEnable_ = b; }

	void SetEnableDelayMotion(bool b) { bEnableMotionDelay_ = b; }
	void SetPositionRounding(bool b) { bRoundingPosition_ = b; }

	void SetHitboxScale(D3DXVECTOR2& sc) { hitboxScale_ = sc; }
	void SetHitboxScaleX(float x) { hitboxScale_.x = x; }
	void SetHitboxScaleY(float y) { hitboxScale_.y = y; }
	float GetHitboxScaleX() { return hitboxScale_.x; }
	float GetHitboxScaleY() { return hitboxScale_.y; }

	virtual void DeleteImmediate();
	virtual void ConvertToItem(bool flgPlayerCollision);
	void AddShot(int frame, int idShot, float radius, double angle);
};

class StgShotObject::ReserveShotListData {
	friend ReserveShotList;
	int idShot_;//対象ID
	float radius_;//出現位置距離
	double angle_;//出現位置角度
public:
	ReserveShotListData() { idShot_ = DxScript::ID_INVALID; radius_ = 0; angle_ = 0; }
	virtual ~ReserveShotListData() {}
	int GetShotID() { return idShot_; }
	float GetRadius() { return radius_; }
	double GetAngle() { return angle_; }
};

class StgShotObject::ReserveShotList {
public:
	class ListElement {
		std::list<ReserveShotListData> list_;
	public:
		ListElement() {}
		virtual ~ListElement() {}
		void Add(ReserveShotListData& data) { list_.push_back(data); }
		std::list<ReserveShotListData>* GetDataList() { return &list_; }
	};
private:
	int frame_;
	std::unordered_map<int, ref_count_ptr<ListElement>::unsync> mapData_;
public:
	ReserveShotList() { frame_ = 0; }
	virtual ~ReserveShotList() {}
	ref_count_ptr<ListElement>::unsync GetNextFrameData();
	void AddData(int frame, int idShot, float radius, double angle);
	void Clear(StgStageController* stageController);
};

/**********************************************************
//StgNormalShotObject
**********************************************************/
class StgNormalShotObject : public StgShotObject {
	friend class StgShotObject;
protected:
	double angularVelocity_;
	void _AddIntersectionRelativeTarget();
	virtual void _ConvertToItemAndSendEvent(bool flgPlayerCollision);
public:
	StgNormalShotObject(StgStageController* stageController);
	virtual ~StgNormalShotObject();
	virtual void Work();
	virtual void RenderOnShotManager();

	virtual void ClearShotObject() {
		ClearIntersectionRelativeTarget();
	}
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);

	virtual void RegistIntersectionTarget() {
		if (!bUserIntersectionMode_) _AddIntersectionRelativeTarget();
	}

	virtual std::vector<StgIntersectionTarget::ptr> GetIntersectionTargetList();
	virtual void SetShotDataID(int id);
};

/**********************************************************
//StgLaserObject(レーザー基本部)
**********************************************************/
class StgLaserObject : public StgShotObject {
protected:
	int length_;
	int widthRender_;
	int widthIntersection_;
	float invalidLengthStart_;
	float invalidLengthEnd_;
	int frameGrazeInvalidStart_;
	float itemDistance_;
	void _AddIntersectionRelativeTarget();
public:
	StgLaserObject(StgStageController* stageController);
	virtual void ClearShotObject() {
		ClearIntersectionRelativeTarget();
	}
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);

	int GetLength() { return length_; }
	void SetLength(int length) { length_ = length; }
	int GetRenderWidth() { return widthRender_; }
	void SetRenderWidth(int width) {
		widthRender_ = width;
		if (widthIntersection_ < 0)widthIntersection_ = width / 4;
	}
	int GetIntersectionWidth() { return widthIntersection_; }
	void SetIntersectionWidth(int width) { widthIntersection_ = width; }
	void SetInvalidLength(float start, float end) { invalidLengthStart_ = start; invalidLengthEnd_ = end; }
	void SetGrazeInvalidFrame(int frame) { frameGrazeInvalidStart_ = frame; }
	void SetItemDistance(float dist) { itemDistance_ = std::max(dist, 0.1f); }
};

/**********************************************************
//StgLooseLaserObject(射出型レーザー)
**********************************************************/
class StgLooseLaserObject : public StgLaserObject {
protected:
	float posXE_;//後方x
	float posYE_;//後方y

	virtual void _DeleteInAutoClip();
	virtual void _Move();
	virtual void _ConvertToItemAndSendEvent(bool flgPlayerCollision);
public:
	StgLooseLaserObject(StgStageController* stageController);
	virtual void Work();
	virtual void RenderOnShotManager();

	virtual void RegistIntersectionTarget() {
		if (!bUserIntersectionMode_) _AddIntersectionRelativeTarget();
	}
	virtual std::vector<StgIntersectionTarget::ptr> GetIntersectionTargetList();
	virtual void SetX(float x) { StgShotObject::SetX(x); posXE_ = x; }
	virtual void SetY(float y) { StgShotObject::SetY(y); posYE_ = y; }
};

/**********************************************************
//StgStraightLaserObject(設置型レーザー)
**********************************************************/
class StgStraightLaserObject : public StgLaserObject {
protected:
	double angLaser_;
	bool bUseSouce_;
	float scaleX_;

	bool bLaserExpand_;

	virtual void _DeleteInAutoClip();
	virtual void _DeleteInAutoDeleteFrame();
	virtual void _AddReservedShot(shared_ptr<StgShotObject> obj, ReserveShotListData* data);
	virtual void _ConvertToItemAndSendEvent(bool flgPlayerCollision);
public:
	StgStraightLaserObject(StgStageController* stageController);
	virtual void Work();
	virtual void RenderOnShotManager();
	virtual void RegistIntersectionTarget() {
		if (!bUserIntersectionMode_) _AddIntersectionRelativeTarget();
	}
	virtual std::vector<StgIntersectionTarget::ptr> GetIntersectionTargetList();

	double GetLaserAngle() { return angLaser_; }
	void SetLaserAngle(double angle) { angLaser_ = angle; }
	void SetFadeDelete() { if (frameFadeDelete_ < 0)frameFadeDelete_ = FRAME_FADEDELETE_LASER; }
	void SetSourceEnable(bool bEnable) { bUseSouce_ = bEnable; }

	void SetLaserExpand(bool b) { bLaserExpand_ = b; }
	bool GetLaserExpand() { return bLaserExpand_; }
};

/**********************************************************
//StgCurveLaserObject(曲がる型レーザー)
**********************************************************/
class StgCurveLaserObject : public StgLaserObject {
protected:
	struct LaserNode {
		DxPoint pos;
		DxPoint vertOff[2];
	};

	std::list<LaserNode> listPosition_;
	float tipDecrement_;

	virtual void _DeleteInAutoClip();
	virtual void _Move();
	virtual void _ConvertToItemAndSendEvent(bool flgPlayerCollision);
public:
	StgCurveLaserObject(StgStageController* stageController);
	virtual void Work();
	virtual void RenderOnShotManager();
	virtual void RegistIntersectionTarget() {
		if (!bUserIntersectionMode_) _AddIntersectionRelativeTarget();
	}
	virtual std::vector<StgIntersectionTarget::ptr> GetIntersectionTargetList();
	void SetTipDecrement(float dec) { tipDecrement_ = dec; }
};


/**********************************************************
//StgPatternShotObjectGenerator (ECL-style bullets firing) [Under construction]
**********************************************************/
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

		BASEPOINT_RESET = -256 * 256,
	};
private:
	shared_ptr<StgMoveObject> parent_;

	int idShotData_;
	int typeOwner_;
	TypeObject typeShot_;
	int typePattern_;
	int iniBlendType_;

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

	float speedBase_;
	float speedArgument_;
	float angleBase_;
	float angleArgument_;

	int delay_;

	int laserWidth_;
	int laserLength_;

	std::vector<StgPatternShotTransform> listTransformation_;
public:
	StgPatternShotObjectGenerator();
	~StgPatternShotObjectGenerator();

	virtual void Render() {}
	virtual void SetRenderState() {}

	void CopyFrom(shared_ptr<StgPatternShotObjectGenerator> other) {
		StgPatternShotObjectGenerator::CopyFrom(other.get());
	}
	void CopyFrom(StgPatternShotObjectGenerator* other);

	void AddTransformation(StgPatternShotTransform& entry) { listTransformation_.push_back(entry); }
	void SetTransformation(size_t off, StgPatternShotTransform& entry);
	void ClearTransformation() { listTransformation_.clear(); }

	void SetParent(shared_ptr<StgMoveObject> obj) { parent_ = obj; }

	void FireSet(void* scriptData, StgStageController* controller, std::vector<int>* idVector);

	void SetGraphic(int id) { idShotData_ = id; }
	void SetTypeOwner(int type) { typeOwner_ = type; }
	void SetTypePattern(int type) { typePattern_ = type; }
	void SetTypeShot(TypeObject type) { typeShot_ = type; }
	void SetBlendType(int type) { iniBlendType_ = type; }

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

	void SetSpeed(float base, float arg) {
		speedBase_ = base;
		speedArgument_ = arg;
	}
	void SetAngle(float base, float arg) {
		angleBase_ = base;
		angleArgument_ = arg;
	}

	void SetDelay(int delay) { delay_ = delay; }
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
		TRANSFORM_ADDPATTERNA1,
		TRANSFORM_ADDPATTERNA2,
		//TRANSFORM_,
	};
	uint8_t act = 0xff;
	int param_s[3];
	double param_d[3];
};

/**********************************************************
//Inline function implementations
**********************************************************/
inline bool StgShotManager::LoadPlayerShotData(std::wstring path, bool bReload) {
	return listPlayerShotData_->AddShotDataList(path, bReload);
}
inline bool StgShotManager::LoadEnemyShotData(std::wstring path, bool bReload) {
	return listEnemyShotData_->AddShotDataList(path, bReload);
}

#pragma region StgShotObject_impl
inline void StgShotObject::_SetVertexPosition(VERTEX_TLX& vertex, float x, float y, float z, float w) {
	constexpr float bias = -0.5f;
	vertex.position.x = x + bias;
	vertex.position.y = y + bias;
	vertex.position.z = z;
	vertex.position.w = w;
}
inline void StgShotObject::_SetVertexUV(VERTEX_TLX& vertex, float u, float v) {
	vertex.texcoord.x = u;
	vertex.texcoord.y = v;
}
inline void StgShotObject::_SetVertexColorARGB(VERTEX_TLX& vertex, D3DCOLOR color) {
	vertex.diffuse_color = color;
}
inline void StgShotObject::SetAlpha(int alpha) {
	ColorAccess::ClampColor(alpha);
	color_ = (color_ & 0x00ffffff) | ((byte)alpha << 24);
}
inline void StgShotObject::SetColor(int r, int g, int b) {
	ColorAccess::ClampColor(r);
	ColorAccess::ClampColor(g);
	ColorAccess::ClampColor(b);
	D3DCOLOR dc = D3DCOLOR_ARGB(0, r, g, b);
	color_ = (color_ & 0xff000000) | (dc & 0x00ffffff);
}
#pragma endregion StgShotObject_impl

#endif
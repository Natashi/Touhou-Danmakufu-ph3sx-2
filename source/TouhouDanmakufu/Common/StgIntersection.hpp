#ifndef __TOUHOUDANMAKUFU_DNHSTG_INTERSECTION__
#define __TOUHOUDANMAKUFU_DNHSTG_INTERSECTION__

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"

class StgIntersectionManager;
class StgIntersectionSpace;
class StgIntersectionCheckList;

class StgIntersectionObject;

/**********************************************************
//StgIntersectionTarget
**********************************************************/
class StgIntersectionTarget : public IStringInfo {
	friend StgIntersectionManager;
public:
	typedef enum : uint8_t {
		SHAPE_CIRCLE = 0,
		SHAPE_LINE = 1,
	} Shape;
	typedef enum : uint8_t {
		TYPE_PLAYER,
		TYPE_PLAYER_SHOT,
		TYPE_PLAYER_SPELL,
		TYPE_ENEMY,
		TYPE_ENEMY_SHOT,
	} Type;
protected:
	//int mortonNo_;
	Type typeTarget_;
	Shape shape_;
	weak_ptr<StgIntersectionObject> obj_;

	RECT intersectionSpace_;
public:
	StgIntersectionTarget();
	virtual ~StgIntersectionTarget() {}

	RECT& GetIntersectionSpaceRect() { return intersectionSpace_; }
	virtual void SetIntersectionSpace() = 0;

	Type GetTargetType() { return typeTarget_; }
	void SetTargetType(Type type) { typeTarget_ = type; }
	Shape GetShape() { return shape_; }
	weak_ptr<StgIntersectionObject> GetObject() { return obj_; }
	void SetObject(weak_ptr<StgIntersectionObject> obj) {
		if (!obj.expired()) obj_ = obj;
	}

	//int GetMortonNumber() { return mortonNo_; }
	//void SetMortonNumber(int no) { mortonNo_ = no; }
	void ClearObjectIntersectedIdList();

	virtual std::wstring GetInfoAsString();
};

class StgIntersectionTarget_Circle : public StgIntersectionTarget {
	friend StgIntersectionManager;
	DxCircle circle_;
public:
	StgIntersectionTarget_Circle() { shape_ = Shape::SHAPE_CIRCLE; }
	virtual ~StgIntersectionTarget_Circle() {}

	virtual void SetIntersectionSpace();

	DxCircle& GetCircle() { return circle_; }
	void SetCircle(DxCircle& circle) { 
		circle_ = circle;
		SetIntersectionSpace();
	}
};

class StgIntersectionTarget_Line : public StgIntersectionTarget {
	friend StgIntersectionManager;
	DxWidthLine line_;
public:
	StgIntersectionTarget_Line() { shape_ = Shape::SHAPE_LINE; }
	virtual ~StgIntersectionTarget_Line() {}

	virtual void SetIntersectionSpace();

	DxWidthLine& GetLine() { return line_; }
	void SetLine(DxWidthLine& line) { 
		line_ = line; 
		SetIntersectionSpace();
	}
};

class StgIntersectionTargetPoint;

/**********************************************************
//StgIntersectionManager
//下記を参考
//http://marupeke296.com/COL_2D_No8_QuadTree.html
**********************************************************/
class StgIntersectionManager {
private:
	enum {
		SPACE_PLAYER_ENEMY = 0,
		SPACE_PLAYERSHOT_ENEMY,
		SPACE_PLAYERSHOT_ENEMYSHOT,
	};
	
	std::vector<StgIntersectionSpace*> listSpace_;
	std::vector<StgIntersectionTargetPoint> listEnemyTargetPoint_;
	std::vector<StgIntersectionTargetPoint> listEnemyTargetPointNext_;

	shared_ptr<DxScriptParticleListObject2D> objIntersectionVisualizerCircle_;
	shared_ptr<DxScriptPrimitiveObject2D> objIntersectionVisualizerLine_;
	std::atomic<size_t> countCircleInstance_;
	std::atomic<size_t> countLineVertex_;
	bool bRenderIntersection_;

	shared_ptr<Shader> shaderVisualizerCircle_;
	shared_ptr<Shader> shaderVisualizerLine_;

	omp_lock_t lock_;
public:
	StgIntersectionManager();
	virtual ~StgIntersectionManager();

	void Work();
	void RenderVisualizer();

	void SetRenderIntersection(bool b) { bRenderIntersection_ = b; }
	bool IsRenderIntersection() { return bRenderIntersection_; }

	void AddTarget(shared_ptr<StgIntersectionTarget> target);
	void AddEnemyTargetToShot(shared_ptr<StgIntersectionTarget> target);
	void AddEnemyTargetToPlayer(shared_ptr<StgIntersectionTarget> target);
	std::vector<StgIntersectionTargetPoint>* GetAllEnemyTargetPoint() { return &listEnemyTargetPoint_; }

	//void CheckDeletedObject(std::string funcName);

	static bool IsIntersected(shared_ptr<StgIntersectionTarget>& target1, shared_ptr<StgIntersectionTarget>& target2);

	omp_lock_t* GetLock() { return &lock_; }

	void AddVisualization(shared_ptr<StgIntersectionTarget>& target);
};

/**********************************************************
//StgIntersectionSpace
//以下サイトを参考
//　○×（まるぺけ）つくろーどっとコム
//　http://marupeke296.com/
**********************************************************/
/*
class StgIntersectionSpace {
	enum {
		MAX_LEVEL = 9,
		TYPE_A = 0,
		TYPE_B = 1,
	};
protected:
	//Cell TARGETA/B listTarget
	std::vector<std::vector<std::vector<StgIntersectionTarget*>>> listCell_;
	int listCountLevel_[MAX_LEVEL + 1];	// 各レベルのセル数
	double spaceWidth_; // 領域のX軸幅
	double spaceHeight_; // 領域のY軸幅
	double spaceLeft_; // 領域の左側（X軸最小値）
	double spaceTop_; // 領域の上側（Y軸最小値）
	double unitWidth_; // 最小レベル空間の幅単位
	double unitHeight_; // 最小レベル空間の高単位
	int countCell_; // 空間の数
	int unitLevel_; // 最下位レベル
	StgIntersectionCheckList* listCheck_;

	unsigned int _GetMortonNumber(float left, float top, float right, float bottom);
	unsigned int  _BitSeparate32(unsigned int  n);
	unsigned short _Get2DMortonNumber(unsigned short x, unsigned short y);
	unsigned int  _GetPointElem(float pos_x, float pos_y);
	void _WriteIntersectionCheckList(int indexSpace, StgIntersectionCheckList*& listCheck, 
		std::vector<std::vector<StgIntersectionTarget*>> &listStack);
public:
	StgIntersectionSpace();
	virtual ~StgIntersectionSpace();
	bool Initialize(int level, int left, int top, int right, int bottom);
	bool RegistTarget(int type, StgIntersectionTarget*& target);
	bool RegistTargetA(StgIntersectionTarget*& target) { return RegistTarget(TYPE_A, target); }
	bool RegistTargetB(StgIntersectionTarget*& target) { return RegistTarget(TYPE_B, target); }
	void ClearTarget();
	StgIntersectionCheckList* CreateIntersectionCheckList();
};
*/

class StgIntersectionCheckList {
	size_t count_;
	//std::vector<StgIntersectionTarget*> listTargetA_;
	//std::vector<StgIntersectionTarget*> listTargetB_;
	std::vector<std::pair<shared_ptr<StgIntersectionTarget>, shared_ptr<StgIntersectionTarget>>> listTargetPair_;
public:
	StgIntersectionCheckList();
	virtual ~StgIntersectionCheckList();

	void Clear() { count_ = 0; }
	size_t GetCheckCount() { return count_; }

	void AddTargetPair(shared_ptr<StgIntersectionTarget>& targetA, shared_ptr<StgIntersectionTarget>& targetB);
	shared_ptr<StgIntersectionTarget> GetTargetA(size_t index);
	shared_ptr<StgIntersectionTarget> GetTargetB(size_t index);
};

class StgIntersectionSpace {
	enum {
		TYPE_A = 0,
		TYPE_B = 1,
	};
protected:
	std::vector<std::vector<shared_ptr<StgIntersectionTarget>>> listCell_;

	double spaceWidth_; // 領域のX軸幅
	double spaceHeight_; // 領域のY軸幅
	double spaceLeft_; // 領域の左側（X軸最小値）
	double spaceTop_; // 領域の上側（Y軸最小値）

	StgIntersectionCheckList* listCheck_;

	size_t _WriteIntersectionCheckList(StgIntersectionManager* manager);
public:
	StgIntersectionSpace();
	virtual ~StgIntersectionSpace();
	bool Initialize(int left, int top, int right, int bottom);
	bool RegistTarget(int type, shared_ptr<StgIntersectionTarget>& target);
	bool RegistTargetA(shared_ptr<StgIntersectionTarget>& target) { return RegistTarget(TYPE_A, target); }
	bool RegistTargetB(shared_ptr<StgIntersectionTarget>& target) { return RegistTarget(TYPE_B, target); }
	void ClearTarget();
	StgIntersectionCheckList* CreateIntersectionCheckList(StgIntersectionManager* manager, size_t& total);
};

class StgIntersectionObject {
protected:
	bool bIntersected_;//衝突判定
	size_t intersectedCount_;
	std::vector<shared_ptr<StgIntersectionTarget>> listRelativeTarget_;
	std::vector<DxCircle> listOrgCircle_;
	std::vector<DxWidthLine> listOrgLine_;
	std::vector<weak_ptr<StgIntersectionObject>> listIntersectedID_;
public:
	StgIntersectionObject() { bIntersected_ = false; intersectedCount_ = 0; }
	virtual ~StgIntersectionObject() {}

	virtual void Intersect(shared_ptr<StgIntersectionTarget> ownTarget, shared_ptr<StgIntersectionTarget> otherTarget) = 0;

	void ClearIntersected() { bIntersected_ = false; intersectedCount_ = 0; }
	bool IsIntersected() { return bIntersected_; }
	void SetIntersected() { bIntersected_ = true; intersectedCount_++; }

	size_t GetIntersectedCount() { return intersectedCount_; }
	void ClearIntersectedIdList() { listIntersectedID_.clear(); }
	void AddIntersectedId(weak_ptr<StgIntersectionObject> obj) { listIntersectedID_.push_back(obj); }
	std::vector<weak_ptr<StgIntersectionObject>>& GetIntersectedIdList() { return listIntersectedID_; }

	void ClearIntersectionRelativeTarget();
	void AddIntersectionRelativeTarget(shared_ptr<StgIntersectionTarget> target);
	shared_ptr<StgIntersectionTarget> GetIntersectionRelativeTarget(size_t index) { return listRelativeTarget_[index]; }

	void UpdateIntersectionRelativeTarget(int posX, int posY, double angle);
	void RegistIntersectionRelativeTarget(StgIntersectionManager* manager);
	size_t GetIntersectionRelativeTargetCount() { return listRelativeTarget_.size(); }
	int GetDxScriptObjectID();

	virtual std::vector<shared_ptr<StgIntersectionTarget>> GetIntersectionTargetList() { 
		return std::vector<shared_ptr<StgIntersectionTarget>>();
	}
};

inline void StgIntersectionTarget::ClearObjectIntersectedIdList() { 
	if (auto ptr = obj_.lock()) 
		ptr->ClearIntersectedIdList();
}

/**********************************************************
//StgIntersectionTargetPoint
**********************************************************/
class StgEnemyObject;
class StgIntersectionTargetPoint {
private:
	POINT pos_;
	weak_ptr<StgEnemyObject> ptrObject_;
public:
	POINT& GetPoint() { return pos_; }
	void SetPoint(POINT& pos) { pos_ = pos; }
	weak_ptr<StgEnemyObject> GetObjectRef() { return ptrObject_; }
	void SetObjectRef(weak_ptr<StgEnemyObject> id) { ptrObject_ = id; }
};

#endif


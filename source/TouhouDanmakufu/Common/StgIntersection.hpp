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

	DxRect<LONG> intersectionSpace_;
public:
	StgIntersectionTarget();
	virtual ~StgIntersectionTarget() {}

	const DxRect<LONG>& GetIntersectionSpaceRect() const { return intersectionSpace_; }
	void SetIntersectionSpace(const DxRect<LONG>& rect) { intersectionSpace_ = rect; }
	virtual void SetIntersectionSpace() = 0;

	Type GetTargetType() const { return typeTarget_; }
	void SetTargetType(Type type) { typeTarget_ = type; }
	Shape GetShape() const { return shape_; }
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
	void SetCircle(const DxCircle& circle) { 
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
	void SetLine(const DxWidthLine& line) { 
		line_ = line; 
		SetIntersectionSpace();
	}
};

class StgIntersectionTargetPoint;

/**********************************************************
//StgIntersectionManager
//â∫ãLÇéQçl
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

	CriticalSection lock_;
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

	static bool IsIntersected(StgIntersectionTarget* target1, StgIntersectionTarget* target2);

	CriticalSection& GetLock() { return lock_; }

	void AddVisualization(shared_ptr<StgIntersectionTarget>& target);
};

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
public:
	typedef std::vector<shared_ptr<StgIntersectionTarget>> ListTarget;
	typedef std::pair<StgIntersectionTarget*, StgIntersectionTarget*> TargetCheckListPair;
protected:
	DxRect<double> spaceRect_;

	size_t previousCheckCreated_;
	std::pair<ListTarget, ListTarget> pairTargetList_;
	std::vector<TargetCheckListPair> pooledCheckList_;
public:
	StgIntersectionSpace();
	virtual ~StgIntersectionSpace();

	bool Initialize(double left, double top, double right, double bottom);

	bool RegistTarget(ListTarget* pVec, shared_ptr<StgIntersectionTarget>& target);
	bool RegistTargetA(shared_ptr<StgIntersectionTarget>& target) { return RegistTarget(&pairTargetList_.first, target); }
	bool RegistTargetB(shared_ptr<StgIntersectionTarget>& target) { return RegistTarget(&pairTargetList_.second, target); }
	void ClearTarget();

	std::vector<TargetCheckListPair>* CreateIntersectionCheckList(StgIntersectionManager* manager, size_t& total);
};

class StgIntersectionObject {
private:
	struct IntersectionRelativeTarget {
		DxShapeBase* orgShape;
		DxRect<LONG> orgIntersectionRect;
		shared_ptr<StgIntersectionTarget> relTarget;
	};
protected:
	bool bIntersected_;
	size_t intersectedCount_;
	std::vector<weak_ptr<StgIntersectionObject>> listIntersectedID_;

	std::vector<IntersectionRelativeTarget> listRelativeTarget_;
public:
	StgIntersectionObject() { bIntersected_ = false; intersectedCount_ = 0; }
	virtual ~StgIntersectionObject() {}

	virtual void Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) = 0;

	void ClearIntersected() { bIntersected_ = false; intersectedCount_ = 0; }
	bool IsIntersected() { return bIntersected_; }
	void SetIntersected() { bIntersected_ = true; intersectedCount_++; }

	size_t GetIntersectedCount() { return intersectedCount_; }
	void ClearIntersectedIdList() { listIntersectedID_.clear(); }
	void AddIntersectedId(weak_ptr<StgIntersectionObject> obj) { listIntersectedID_.push_back(obj); }
	std::vector<weak_ptr<StgIntersectionObject>>& GetIntersectedIdList() { return listIntersectedID_; }

	void AddIntersectionRelativeTarget(shared_ptr<StgIntersectionTarget> target);
	void ClearIntersectionRelativeTarget();
	shared_ptr<StgIntersectionTarget> GetIntersectionRelativeTarget(size_t index) { return listRelativeTarget_[index].relTarget; }

	void UpdateIntersectionRelativeTarget(float posX, float posY, double angle);
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


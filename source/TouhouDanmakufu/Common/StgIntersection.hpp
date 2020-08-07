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
	using ptr = std::shared_ptr<StgIntersectionTarget>;

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
	using ptr = std::shared_ptr<StgIntersectionTarget_Circle>;

	StgIntersectionTarget_Circle() { shape_ = Shape::SHAPE_CIRCLE; }
	virtual ~StgIntersectionTarget_Circle() {}

	virtual void SetIntersectionSpace() {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		LONG screenWidth = graphics->GetScreenWidth();
		LONG screenHeight = graphics->GetScreenWidth();

		constexpr LONG margin = 16L;
		LONG x = circle_.GetX();
		LONG y = circle_.GetY();
		LONG r = circle_.GetR();

		LONG x1 = std::clamp(x - r, -margin, screenWidth + margin);
		LONG x2 = std::clamp(x + r, -margin, screenWidth + margin);
		LONG y1 = std::clamp(y - r, -margin, screenHeight + margin);
		LONG y2 = std::clamp(y + r, -margin, screenHeight + margin);
		intersectionSpace_ = { x1, y1, x2, y2 };
	}

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
	using ptr = std::shared_ptr<StgIntersectionTarget_Line>;

	StgIntersectionTarget_Line() { shape_ = Shape::SHAPE_LINE; }
	virtual ~StgIntersectionTarget_Line() {}

	virtual void SetIntersectionSpace() {
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
		LONG screenWidth = graphics->GetScreenWidth();
		LONG screenHeight = graphics->GetScreenWidth();

		LONG _x1 = std::clamp((LONG)x1, -margin, screenWidth + margin);
		LONG _x2 = std::clamp((LONG)x2, -margin, screenWidth + margin);
		LONG _y1 = std::clamp((LONG)y1, -margin, screenHeight + margin);
		LONG _y2 = std::clamp((LONG)y2, -margin, screenHeight + margin);
		intersectionSpace_ = { _x1, _y1, _x2, _y2 };
	}

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

	omp_lock_t lock_;
public:
	StgIntersectionManager();
	virtual ~StgIntersectionManager();
	void Work();

	void AddTarget(StgIntersectionTarget::ptr target);
	void AddEnemyTargetToShot(StgIntersectionTarget::ptr target);
	void AddEnemyTargetToPlayer(StgIntersectionTarget::ptr target);
	std::vector<StgIntersectionTargetPoint>* GetAllEnemyTargetPoint() { return &listEnemyTargetPoint_; }

	//void CheckDeletedObject(std::string funcName);

	static bool IsIntersected(StgIntersectionTarget::ptr& target1, StgIntersectionTarget::ptr& target2);

	omp_lock_t* GetLock() { return &lock_; }
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
	std::vector<std::pair<StgIntersectionTarget::ptr, StgIntersectionTarget::ptr>> listTargetPair_;
public:
	StgIntersectionCheckList() { count_ = 0; }
	virtual ~StgIntersectionCheckList() {}

	void Clear() { count_ = 0; }
	size_t GetCheckCount() { return count_; }
	void Add(StgIntersectionTarget::ptr& targetA, StgIntersectionTarget::ptr& targetB) {
		std::pair<StgIntersectionTarget::ptr, StgIntersectionTarget::ptr> pair = { targetA, targetB };
		if (listTargetPair_.size() <= count_) {
			listTargetPair_.push_back(pair);
		}
		else {
			listTargetPair_[count_] = pair;
		}
		++count_;
	}
	StgIntersectionTarget::ptr GetTargetA(int index) {
		std::pair<StgIntersectionTarget::ptr, StgIntersectionTarget::ptr> pair = listTargetPair_[index];
		listTargetPair_[index].first = nullptr;
		return pair.first; 
	}
	StgIntersectionTarget::ptr GetTargetB(int index) {
		std::pair<StgIntersectionTarget::ptr, StgIntersectionTarget::ptr> pair = listTargetPair_[index];
		listTargetPair_[index].second = nullptr;
		return pair.second;
	}
};

class StgIntersectionSpace {
	enum {
		TYPE_A = 0,
		TYPE_B = 1,
	};
protected:
	std::vector<std::vector<StgIntersectionTarget::ptr>> listCell_;

	double spaceWidth_; // 領域のX軸幅
	double spaceHeight_; // 領域のY軸幅
	double spaceLeft_; // 領域の左側（X軸最小値）
	double spaceTop_; // 領域の上側（Y軸最小値）

	StgIntersectionCheckList* listCheck_;

	size_t _WriteIntersectionCheckList(StgIntersectionManager* manager, StgIntersectionCheckList*& listCheck);
//		std::vector<std::vector<StgIntersectionTarget*>> &listStack);
public:
	StgIntersectionSpace();
	virtual ~StgIntersectionSpace();
	bool Initialize(int left, int top, int right, int bottom);
	bool RegistTarget(int type, StgIntersectionTarget::ptr& target);
	bool RegistTargetA(StgIntersectionTarget::ptr& target) { return RegistTarget(TYPE_A, target); }
	bool RegistTargetB(StgIntersectionTarget::ptr& target) { return RegistTarget(TYPE_B, target); }
	void ClearTarget() {
		listCell_[0].clear();
		listCell_[1].clear();
	}
	StgIntersectionCheckList* CreateIntersectionCheckList(StgIntersectionManager* manager, size_t& total) {
		StgIntersectionCheckList* res = listCheck_;
		res->Clear();

		total += _WriteIntersectionCheckList(manager, res);
		return res;
	}
};

class StgIntersectionObject {
protected:
	bool bIntersected_;//衝突判定
	size_t intersectedCount_;
	std::vector<StgIntersectionTarget::ptr> listRelativeTarget_;
	std::vector<DxCircle> listOrgCircle_;
	std::vector<DxWidthLine> listOrgLine_;
	std::vector<weak_ptr<StgIntersectionObject>> listIntersectedID_;
public:
	StgIntersectionObject() { bIntersected_ = false; intersectedCount_ = 0; }
	virtual ~StgIntersectionObject() {}
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) = 0;
	void ClearIntersected() { bIntersected_ = false; intersectedCount_ = 0; }
	bool IsIntersected() { return bIntersected_; }
	void SetIntersected() { bIntersected_ = true; intersectedCount_++; }
	size_t GetIntersectedCount() { return intersectedCount_; }
	void ClearIntersectedIdList() { listIntersectedID_.clear(); }
	void AddIntersectedId(weak_ptr<StgIntersectionObject> obj) { listIntersectedID_.push_back(obj); }
	std::vector<weak_ptr<StgIntersectionObject>>& GetIntersectedIdList() { return listIntersectedID_; }

	void ClearIntersectionRelativeTarget();
	void AddIntersectionRelativeTarget(StgIntersectionTarget::ptr target);
	StgIntersectionTarget::ptr GetIntersectionRelativeTarget(size_t index) { return listRelativeTarget_[index]; }

	void UpdateIntersectionRelativeTarget(int posX, int posY, double angle);
	void RegistIntersectionRelativeTarget(StgIntersectionManager* manager);
	size_t GetIntersectionRelativeTargetCount() { return listRelativeTarget_.size(); }
	int GetDxScriptObjectID();

	virtual std::vector<StgIntersectionTarget::ptr> GetIntersectionTargetList() { 
		return std::vector<StgIntersectionTarget::ptr>();
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


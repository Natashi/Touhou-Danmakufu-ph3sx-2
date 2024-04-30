#pragma once

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"

//*******************************************************************
//ScriptCommonDataArea
//*******************************************************************
class ScriptCommonDataArea {
public:
	static constexpr const char* HEADER_SAVED_DATA = "DNHCDR\0\0";
	static constexpr size_t HEADER_SAVED_DATA_SIZE = 8U;
	static constexpr size_t DATA_HASH = 0xabcdef69u;

protected:
	volatile size_t verifHash_;

	std::map<std::string, gstd::value> mapValue_;

public:
	using KeyValue = std::pair<std::string, gstd::value>;
	using MapIter = decltype(mapValue_)::iterator;

protected:
	gstd::value _ReadRecord(gstd::ByteBuffer& buffer);
	void _WriteRecord(gstd::ByteBuffer& buffer, const gstd::value& comValue);

public:
	ScriptCommonDataArea();
	virtual ~ScriptCommonDataArea();

	void Clear();

	optional<MapIter> GetValue(const std::string& name);

	gstd::value* GetValueRef(const std::string& name);
	gstd::value* GetValueRef(MapIter where);

	void SetValue(const std::string& name, gstd::value v);
	void SetValue(MapIter where, gstd::value v);

	void DeleteValue(const std::string& name);
	void Copy(ScriptCommonDataArea* source);

	auto begin() const { return mapValue_.cbegin(); }
	auto end() const { return mapValue_.cend(); }

	void ReadRecord(gstd::RecordBuffer& record);
	void WriteRecord(gstd::RecordBuffer& record);

	volatile bool HashValid() const { return verifHash_ == DATA_HASH; }
};

struct ScriptCommonData_Pointer {
	ScriptCommonDataArea* area = nullptr;
	gstd::value* data = nullptr;

	ScriptCommonData_Pointer() : area(nullptr), data(nullptr) {}
	ScriptCommonData_Pointer(ScriptCommonDataArea* area, gstd::value* data) : area(area), data(data) {}

	static optional<ScriptCommonData_Pointer> From(uint64_t ptr);
	explicit operator uint64_t() const;

	bool IsValid() const;
};

//*******************************************************************
//ScriptCommonDataManager
//*******************************************************************
class ScriptCommonDataManager {
public:
	using DataArea = ScriptCommonDataArea;
	using DataArea_UPtr = unique_ptr<DataArea>;
	using DataArea_Map = std::map<std::string, DataArea_UPtr>;
public:
	static inline const std::string DEFAULT_AREA_NAME = "";
protected:
	gstd::CriticalSection lock_;

	DataArea_Map mapData_;
	DataArea* defaultArea_;
public:
	ScriptCommonDataManager();
	virtual ~ScriptCommonDataManager();

	gstd::CriticalSection& GetLock() { return lock_; }

	void Clear();
	void Erase(const std::string& name);

	DataArea* GetDefaultArea() { return defaultArea_; }

	DataArea* CreateArea(const std::string& name);
	DataArea* CreateArea(const std::string& name, const DataArea& source);
	DataArea* CreateArea(const std::string& name, DataArea_UPtr&& source);
	void CopyArea(const std::string& nameDest, const std::string& nameSrc);

	DataArea* GetArea(const std::string& name);
	void SetArea(const std::string& name, const DataArea& source);
	void SetArea(const std::string& name, DataArea_UPtr&& source);

	auto begin() const { return mapData_.cbegin(); }
	auto end() const { return mapData_.cend(); }
};

//*******************************************************************
//ScriptCommonDataInfoPanel
//*******************************************************************
class ScriptCommonDataInfoPanel : public ILoggerPanel {
	class CommonDataDisplay {
	public:
		struct DataPair {
			std::string key;
			std::string value;
		};
	public:
		ScriptCommonDataArea* data;
		std::string name;

		CommonDataDisplay(ScriptCommonDataArea* data, const std::string& name);
	public:
		// Lazy-loaded as they're only visible upon selecting an area
		optional<std::vector<DataPair>> dataValues;
		void LoadValues();
	public:
		/*
		static const ImGuiTableSortSpecs* imguiSortSpecs;
		static bool IMGUI_CDECL Compare(const CommonDataDisplay& a, const CommonDataDisplay& b);
		*/
	};
protected:
	std::vector<CommonDataDisplay> listDisplay_;
	size_t selectedData_;
public:
	ScriptCommonDataInfoPanel();

	virtual void Initialize(const std::string& name);

	virtual void Update();
	virtual void ProcessGui();
};

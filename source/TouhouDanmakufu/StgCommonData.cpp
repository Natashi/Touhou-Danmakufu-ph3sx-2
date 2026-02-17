#include "source/GcLib/pch.h"

#include "StgCommonData.hpp"
#include "StgSystem.hpp"

//****************************************************************************
//ScriptCommonDataManager
//****************************************************************************
ScriptCommonDataManager::ScriptCommonDataManager() 
	: defaultArea_(CreateArea(DEFAULT_AREA_NAME))
{
}
ScriptCommonDataManager::~ScriptCommonDataManager() {
	mapData_.clear();
}

void ScriptCommonDataManager::Clear() {
	mapData_.clear();
}
void ScriptCommonDataManager::Erase(const std::string& name) {
	auto itr = mapData_.find(name);
	if (itr != mapData_.end()) {
		itr->second->Clear();
		mapData_.erase(itr);
	}
}

ScriptCommonDataManager::DataArea* ScriptCommonDataManager::CreateArea(const std::string& name) {
	return CreateArea(name, make_unique<ScriptCommonDataArea>());
}
ScriptCommonDataManager::DataArea* ScriptCommonDataManager::CreateArea(
	const std::string& name, const DataArea& source)
{
	return CreateArea(name, make_unique<ScriptCommonDataArea>(source));
}
ScriptCommonDataManager::DataArea* ScriptCommonDataManager::CreateArea(
	const std::string& name, DataArea_UPtr&& area)
{
	auto itr = mapData_.find(name);
	if (itr != mapData_.end()) {
		auto warn = STR_FMT("ScriptCommonDataManager: Area \"%s\" already exists.", name.c_str());
		Logger::WriteWarn(warn);
		return itr->second.get();
	}

	auto& inserted = (mapData_[name] = MOVE(area));
	return inserted.get();
}
void ScriptCommonDataManager::CopyArea(const std::string& nameDest, const std::string& nameSrc) {
	auto& dataSrc = mapData_[nameSrc];
	auto dataDest = make_unique<ScriptCommonDataArea>();

	dataDest->Copy(dataSrc.get());

	CreateArea(nameDest, MOVE(dataDest));
}

ScriptCommonDataManager::DataArea* ScriptCommonDataManager::GetArea(const std::string& name) {
	auto itr = mapData_.find(name);
	if (itr != mapData_.end()) {
		return itr->second.get();
	}
	return nullptr;
}
void ScriptCommonDataManager::SetArea(const std::string& name, DataArea_UPtr&& source) {
	mapData_[name] = MOVE(source);
}
void ScriptCommonDataManager::SetArea(const std::string& name, const DataArea& source) {
	if (auto area = GetArea(name)) {
		*area = source;
	}
	else {
		CreateArea(name, source);
	}
}

//****************************************************************************
//ScriptCommonDataArea
//****************************************************************************
ScriptCommonDataArea::ScriptCommonDataArea() : verifHash_(DATA_HASH) {
}
ScriptCommonDataArea::~ScriptCommonDataArea() {}

void ScriptCommonDataArea::Clear() {
	mapValue_.clear();
}

optional<ScriptCommonDataArea::MapIter> ScriptCommonDataArea::GetValue(const std::string& name) {
	auto itr = mapValue_.find(name);
	if (itr != mapValue_.end())
		return itr;
	return {};
}

value* ScriptCommonDataArea::GetValueRef(const std::string& name) {
	auto itr = mapValue_.find(name);
	return GetValueRef(itr);
}
value* ScriptCommonDataArea::GetValueRef(MapIter where) {
	if (where == mapValue_.end())
		return nullptr;
	return &where->second;
}

void ScriptCommonDataArea::SetValue(const std::string& name, value v) {
	mapValue_[name] = v;
}
void ScriptCommonDataArea::SetValue(MapIter where, value v) {
	if (where == mapValue_.end())
		return;
	where->second = v;
}

void ScriptCommonDataArea::DeleteValue(const std::string& name) {
	mapValue_.erase(name);
}
void ScriptCommonDataArea::Copy(ScriptCommonDataArea* source) {
	// Copy assign
	mapValue_ = source->mapValue_;
}

void ScriptCommonDataArea::ReadRecord(RecordBuffer& record) {
	mapValue_.clear();

	for (const std::string& key : record.GetKeyList()) {
		RecordEntry* entry = record.GetEntry(key);
		auto& buffer = entry->GetBufferRef();

		value stored;

		if (buffer.GetSize() > 0) {
			buffer.Seek(0);
			stored = _ReadRecord(buffer);
		}

		mapValue_[key] = stored;
	}
}
value ScriptCommonDataArea::_ReadRecord(ByteBuffer& buffer) {
	script_type_manager* scriptTypeManager = script_type_manager::get_instance();

	uint8_t kind{};
	buffer.Read(&kind, sizeof(uint8_t));

	switch ((type_data::type_kind)kind) {
	case type_data::type_kind::tk_int:
	{
		int64_t data = buffer.ReadInteger64();
		return value(scriptTypeManager->get_int_type(), data);
	}
	case type_data::type_kind::tk_float:
	{
		double data = buffer.ReadDouble();
		return value(scriptTypeManager->get_float_type(), data);
	}
	case type_data::type_kind::tk_char:
	{
		wchar_t data = buffer.ReadValue<wchar_t>();
		return value(scriptTypeManager->get_char_type(), data);
	}
	case type_data::type_kind::tk_boolean:
	{
		bool data = buffer.ReadBoolean();
		return value(scriptTypeManager->get_boolean_type(), data);
	}
	case type_data::type_kind::tk_array:
	{
		uint32_t arrayLength = buffer.ReadValue<uint32_t>();
		if (arrayLength > 0U) {
			std::vector<value> v;
			v.resize(arrayLength);
			for (uint32_t iArray = 0; iArray < arrayLength; iArray++) {
				v[iArray] = _ReadRecord(buffer);
			}
			value res;
			res.reset(scriptTypeManager->get_array_type(v[0].get_type()), v);
			return res;
		}
		return value(scriptTypeManager->get_null_array_type(), std::wstring());
	}
	}

	return value();
}
void ScriptCommonDataArea::WriteRecord(gstd::RecordBuffer& record) {
	for (auto& [key, value] : mapValue_) {
		gstd::ByteBuffer buffer;
		if (value.has_data()) {
			_WriteRecord(buffer, value);
		}

		record.SetRecordAsByteBuffer(key, MOVE(buffer));
	}
}
void ScriptCommonDataArea::_WriteRecord(gstd::ByteBuffer& buffer, const gstd::value& comValue) {
	type_data::type_kind kind = comValue.get_type()->get_kind();

	buffer.WriteValue<uint8_t>((uint8_t)kind);
	switch (kind) {
	case type_data::type_kind::tk_int:
		buffer.WriteInteger64(comValue.as_int());
		break;
	case type_data::type_kind::tk_float:
		buffer.WriteDouble(comValue.as_float());
		break;
	case type_data::type_kind::tk_char:
		buffer.WriteValue(comValue.as_char());
		break;
	case type_data::type_kind::tk_boolean:
		buffer.WriteBoolean(comValue.as_boolean());
		break;
	case type_data::type_kind::tk_array:
	{
		uint32_t arrayLength = comValue.length_as_array();
		buffer.WriteValue(arrayLength);
		for (size_t iArray = 0; iArray < arrayLength; iArray++) {
			const value& arrayValue = comValue[iArray];
			_WriteRecord(buffer, arrayValue);
		}
		break;
	}
	}
}

//****************************************************************************
//ScriptCommonData_Pointer
//****************************************************************************
optional<ScriptCommonData_Pointer> ScriptCommonData_Pointer::From(uint64_t ptr) {
	// TODO: Make this bitness-invariant

	//Pointer format:
	//	63    32				31     0
	//	xxxxxxxx				xxxxxxxx
	//	(ScriptCommonDataArea*) (value*)

	ScriptCommonDataArea* area = (ScriptCommonDataArea*)(ptr >> 32);
	value* data = (value*)(ptr & 0xffffffff);

	ScriptCommonData_Pointer res(area, data);

	if (res.IsValid())
		return res;
	return {};
}
ScriptCommonData_Pointer::operator uint64_t() const {
	// TODO: Make this bitness-invariant

	if (IsValid())
		return ((uint64_t)area << 32) | (uint64_t)data;
	return 0;
}

bool ScriptCommonData_Pointer::IsValid() const {
	return area != nullptr && data != nullptr && area->HashValid();
}

//****************************************************************************
//ScriptCommonDataPanel
//****************************************************************************
ScriptCommonDataInfoPanel::ScriptCommonDataInfoPanel()
	: selectedData_(UINT_MAX) { }

void ScriptCommonDataInfoPanel::Initialize(const std::string& name) {
	ILoggerPanel::Initialize(name);
}

void ScriptCommonDataInfoPanel::Update() {
	listDisplay_.clear();

	ETaskManager* taskManager = ETaskManager::GetInstance();
	if (taskManager == nullptr) {
		return;
	}

	{
		Lock lock(Logger::GetTop()->GetLock());

		for (auto& task : taskManager->GetTaskList()) {
			if (auto& systemController = dptr_cast(StgSystemController, task)) {
				auto cdataManager = systemController->GetCommonDataManager();

				for (auto& [name, area] : *cdataManager) {
					listDisplay_.push_back(CommonDataDisplay(area.get(), name));
				}
			}
		}
	}
}
void ScriptCommonDataInfoPanel::ProcessGui() {
	Logger* parent = Logger::GetTop();

	const ImGuiStyle& style = ImGui::GetStyle();

	float wd = ImGui::GetContentRegionAvail().x;
	float ht = ImGui::GetContentRegionAvail().y;
	float iniDivider = std::max(200.0f, ht * 0.3f);

	if (ImGui::BeginChild("pcdata_child_disp", ImVec2(0, ht), false, ImGuiWindowFlags_HorizontalScrollbar)) {
		{
			if (ImGui::BeginListBox("##pcdata_list", ImVec2(wd, iniDivider))) {
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				float itemHeight = ImGui::GetCurrentContext()->FontSize + style.ItemSpacing.y;

				for (size_t i = 0; i < listDisplay_.size(); ++i) {
					bool selected = (selectedData_ == i);

					{
						const ImVec2 p = ImGui::GetCursorScreenPos();
						float py = p.y + itemHeight / 2;
						const float size = 5;
						drawList->AddTriangleFilled(ImVec2(p.x + size, py),
							ImVec2(p.x, py - size), ImVec2(p.x, py + size),
							ImColor(16, 16, 128));
					}

					const float GAP = 24;

					ImGui::Indent(GAP);
					if (ImGui::Selectable(STR_FMT("%s##item%u", listDisplay_[i].name.c_str(), i).c_str(), selected))
						selectedData_ = i;
					ImGui::Unindent(GAP);

					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndListBox();
			}
		}

		{
			ImGuiTableFlags flags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable
				| ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX
				| ImGuiTableFlags_RowBg;

			if (ImGui::BeginTable("pcdata_table", 2, flags)) {
				ImGui::TableSetupScrollFreeze(0, 1);

				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 100);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 300);

				ImGui::TableHeadersRow();

				if (selectedData_ < listDisplay_.size()) {
					CommonDataDisplay* dataDisplay = &listDisplay_[selectedData_];
					if (!dataDisplay->dataValues.has_value())
						dataDisplay->LoadValues();

					ImGuiListClipper clipper;
					clipper.Begin(dataDisplay->dataValues->size());
					while (clipper.Step()) {
						for (size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
							auto& item = dataDisplay->dataValues->at(i);

							ImGui::TableNextRow();

							ImGui::TableSetColumnIndex(0);
							ImGui::Text(item.key.c_str());

							ImGui::TableSetColumnIndex(1);
							ImGui::Text(item.value.c_str());
						}
					}
				}

				ImGui::EndTable();
			}
		}
	}
	ImGui::EndChild();
}

ScriptCommonDataInfoPanel::CommonDataDisplay::CommonDataDisplay(
	ScriptCommonDataArea* data, const std::string& name)
	: data(data), name(name) { }

void ScriptCommonDataInfoPanel::CommonDataDisplay::LoadValues() {
	if (dataValues.has_value()) return;
	{
		Lock lock(Logger::GetTop()->GetLock());

		{
			std::vector<DataPair> res;

			// Sorted alphabetically by key
			for (auto& [key, value] : *data) {
				res.push_back({
					key,
					STR_MULTI(value.as_string())
				});
			}

			dataValues = res;
		}
	}
}

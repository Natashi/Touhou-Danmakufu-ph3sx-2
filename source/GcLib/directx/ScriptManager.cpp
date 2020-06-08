#include "source/GcLib/pch.h"

#include "ScriptManager.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//ScriptManager
**********************************************************/
int64_t ScriptManager::idScript_ = 0;
ScriptManager::ScriptManager() {
	mainThreadID_ = GetCurrentThreadId();

	bHasCloseScriptWork_ = false;

	FileManager::GetBase()->AddLoadThreadListener(this);
}
ScriptManager::~ScriptManager() {
	FileManager::GetBase()->RemoveLoadThreadListener(this);
	FileManager::GetBase()->WaitForThreadLoadComplete();
}

void ScriptManager::Work() {
	Work(ManagedScript::TYPE_ALL);
}
void ScriptManager::Work(int targetType) {
	bHasCloseScriptWork_ = false;
	std::list<shared_ptr<ManagedScript>>::iterator itr = listScriptRun_.begin();
	for (; itr != listScriptRun_.end(); ) {
		shared_ptr<ManagedScript> script = (*itr);
		int type = script->GetScriptType();
		if (script->IsPaused() || (targetType != ManagedScript::TYPE_ALL && targetType != type)) {
			itr++;
			continue;
		}

		if (script->IsEndScript()) {
			std::map<std::string, script_engine::block*>::iterator itrEvent;
			if (script->IsEventExists("Finalize", itrEvent))
				script->Run(itrEvent);
			itr = listScriptRun_.erase(itr);
			bHasCloseScriptWork_ |= true;
		}
		else {
			std::map<std::string, script_engine::block*>::iterator itrEvent;
			if (script->IsEventExists("MainLoop", itrEvent))
				script->Run(itrEvent);
			itr++;

			bHasCloseScriptWork_ |= script->IsEndScript();
		}

	}

}
void ScriptManager::Render() {
	//ここではオブジェクトの描画を行わない。
}
shared_ptr<ManagedScript> ScriptManager::GetScript(int64_t id) {
	shared_ptr<ManagedScript> res = nullptr;
	{
		Lock lock(lock_);

		auto itr = mapScriptLoad_.find(id);
		if (itr != mapScriptLoad_.end()) {
			res = itr->second;
		}
		else {
			std::list<shared_ptr<ManagedScript>>::iterator itr = listScriptRun_.begin();
			for (; itr != listScriptRun_.end(); itr++) {
				if ((*itr)->GetScriptID() == id) {
					res = (*itr);
					break;
				}
			}
		}
	}
	return res;
}
void ScriptManager::StartScript(int64_t id) {
	shared_ptr<ManagedScript> script = nullptr;
	std::map<int64_t, shared_ptr<ManagedScript>>::iterator itrMap;
	{
		Lock lock(lock_);

		itrMap = mapScriptLoad_.find(id);
		if (itrMap == mapScriptLoad_.end()) return;
		script = itrMap->second;
	}

	if (!script->IsLoad()) {
		int count = 0;
		while (!script->IsLoad()) {
			if (count % 1000 == 999)
				Logger::WriteTop(StringUtility::Format(L"読み込み完了待機(ScriptManager)：%s", script->GetPath().c_str()));
			Sleep(1);
			count++;
		}
	}

	{
		Lock lock(lock_);
		mapScriptLoad_.erase(itrMap);
		listScriptRun_.push_back(script);
	}

	if (script != nullptr && !IsError()) {
		std::map<std::string, script_engine::block*>::iterator itrEvent;
		if (script->IsEventExists("Initialize", itrEvent))
			script->Run(itrEvent);
	}
}
void ScriptManager::StartScript(shared_ptr<ManagedScript> id) {
	if (!id->IsLoad()) {
		int count = 0;
		while (!id->IsLoad()) {
			if (count % 1000 == 999)
				Logger::WriteTop(StringUtility::Format(L"読み込み完了待機(ScriptManager)：%s", id->GetPath().c_str()));
			Sleep(1);
			count++;
		}
	}

	{
		Lock lock(lock_);
		mapScriptLoad_.erase(id->GetScriptID());
		listScriptRun_.push_back(id);
	}

	if (id != nullptr && !IsError()) {
		std::map<std::string, script_engine::block*>::iterator itrEvent;
		if (id->IsEventExists("Initialize", itrEvent))
			id->Run(itrEvent);
	}
}
void ScriptManager::CloseScript(int64_t id) {
	std::list<shared_ptr<ManagedScript>>::iterator itr = listScriptRun_.begin();
	for (; itr != listScriptRun_.end(); itr++) {
		shared_ptr<ManagedScript> script = (*itr);
		if (script->GetScriptID() == id) {
			script->SetEndScript();

			mapClosedScriptResult_[id] = script->GetResultValue();
			if (mapClosedScriptResult_.size() > MAX_CLOSED_SCRIPT_RESULT) {
				int64_t targetID = mapClosedScriptResult_.begin()->first;
				mapClosedScriptResult_.erase(targetID);
			}

			if (script->IsAutoDeleteObject())
				script->GetObjectManager()->DeleteObjectByScriptID(id);

			break;
		}
	}
}
void ScriptManager::CloseScript(shared_ptr<ManagedScript> id) {
	id->SetEndScript();

	mapClosedScriptResult_[id->GetScriptID()] = id->GetResultValue();
	if (mapClosedScriptResult_.size() > MAX_CLOSED_SCRIPT_RESULT) {
		int64_t targetID = mapClosedScriptResult_.begin()->first;
		mapClosedScriptResult_.erase(targetID);
	}

	if (id->IsAutoDeleteObject())
		id->GetObjectManager()->DeleteObjectByScriptID(id->GetScriptID());
}
void ScriptManager::CloseScriptOnType(int type) {
	std::list<shared_ptr<ManagedScript>>::iterator itr = listScriptRun_.begin();
	for (; itr != listScriptRun_.end(); itr++) {
		shared_ptr<ManagedScript> script = (*itr);
		if (script->GetScriptType() == type) {
			script->SetEndScript();

			int64_t id = script->GetScriptID();
			mapClosedScriptResult_[id] = script->GetResultValue();
			if (mapClosedScriptResult_.size() > MAX_CLOSED_SCRIPT_RESULT) {
				int64_t targetID = mapClosedScriptResult_.begin()->first;
				mapClosedScriptResult_.erase(targetID);
			}

			if (script->IsAutoDeleteObject())
				script->GetObjectManager()->DeleteObjectByScriptID(id);
		}
	}
}
bool ScriptManager::IsCloseScript(int64_t id) {
	shared_ptr<ManagedScript> script = GetScript(id);
	bool res = script == nullptr || script->IsEndScript();
	return res;
}
int ScriptManager::GetAllScriptThreadCount() {
	int res = 0;
	{
		Lock lock(lock_);
		std::list<shared_ptr<ManagedScript>>::iterator itr = listScriptRun_.begin();
		for (; itr != listScriptRun_.end(); itr++) {
			res += (*itr)->GetThreadCount();
		}
	}
	return res;
}
void ScriptManager::TerminateScriptAll(std::wstring message) {
	{
		Lock lock(lock_);
		std::list<shared_ptr<ManagedScript>>::iterator itr = listScriptRun_.begin();
		for (; itr != listScriptRun_.end(); itr++) {
			(*itr)->Terminate(message);
		}
	}
}
int64_t ScriptManager::_LoadScript(std::wstring path, shared_ptr<ManagedScript> script) {
	int64_t res = 0;

	res = script->GetScriptID();

	script->SetSourceFromFile(path);
	script->Compile();

	std::map<std::string, script_engine::block*>::iterator itrEvent;
	if (script->IsEventExists("Loading", itrEvent))
		script->Run(itrEvent);

	{
		Lock lock(lock_);
		script->bLoad_ = true;
		mapScriptLoad_[res] = script;
	}

	return res;
}
int64_t ScriptManager::LoadScript(std::wstring path, shared_ptr<ManagedScript> script) {
	int64_t res = 0;
	{
		Lock lock(lock_);
		res = _LoadScript(path, script);
		mapScriptLoad_[res] = script;
	}
	return res;
}
shared_ptr<ManagedScript> ScriptManager::LoadScript(std::wstring path, int type) {
	shared_ptr<ManagedScript> script = Create(type);
	this->LoadScript(path, script);
	return script;
}
int64_t ScriptManager::LoadScriptInThread(std::wstring path, shared_ptr<ManagedScript> script) {
	int64_t res = 0;
	{
		Lock lock(lock_);

		res = script->GetScriptID();
		mapScriptLoad_[res] = script;

		shared_ptr<FileManager::LoadThreadEvent> event(new FileManager::LoadThreadEvent(this, path, script));
		FileManager::GetBase()->AddLoadThreadEvent(event);
	}
	return res;
}
shared_ptr<ManagedScript> ScriptManager::LoadScriptInThread(std::wstring path, int type) {
	shared_ptr<ManagedScript> script = Create(type);
	LoadScriptInThread(path, script);
	return script;
}
void ScriptManager::CallFromLoadThread(shared_ptr<gstd::FileManager::LoadThreadEvent> event) {
	std::wstring path = event->GetPath();

	shared_ptr<ManagedScript> script = std::dynamic_pointer_cast<ManagedScript>(event->GetSource());
	if (script == nullptr || script->IsLoad())return;

	try {
		_LoadScript(path, script);
	}
	catch (gstd::wexception& e) {
		Logger::WriteTop(e.what());
		script->bLoad_ = true;
		SetError(e.what());
	}
}
void ScriptManager::RequestEventAll(int type, std::vector<gstd::value>& listValue) {
	{
		std::list<shared_ptr<ManagedScript>>::iterator itrScript = listScriptRun_.begin();
		for (; itrScript != listScriptRun_.end(); itrScript++) {
			shared_ptr<ManagedScript> script = (*itrScript);
			if (script->IsEndScript())continue;

			script->RequestEvent(type, listValue);
		}
	}

	if (listRelativeManager_.size() > 0) {
		std::list<weak_ptr<ScriptManager>>::iterator itrManager = listRelativeManager_.begin();
		for (; itrManager != listRelativeManager_.end(); ) {
			if (auto manager = (*itrManager).lock()) {
				std::list<shared_ptr<ManagedScript>>::iterator itrScript = manager->listScriptRun_.begin();
				for (; itrScript != manager->listScriptRun_.end(); itrScript++) {
					shared_ptr<ManagedScript> script = (*itrScript);
					if (script->IsEndScript()) continue;

					script->RequestEvent(type, listValue);
				}
				itrManager++;
			}
			else {
				itrManager = listRelativeManager_.erase(itrManager);
			}
		}
	}
}
gstd::value ScriptManager::GetScriptResult(int64_t idScript) {
	gstd::value res;
	shared_ptr<ManagedScript> script = GetScript(idScript);
	if (script) {
		res = script->GetResultValue();
	}
	else {
		auto itr = mapClosedScriptResult_.find(idScript);
		if (itr != mapClosedScriptResult_.end()) {
			res = itr->second;
		}
	}

	return res;
}
void ScriptManager::AddRelativeScriptManagerMutual(weak_ptr<ScriptManager> manager1, weak_ptr<ScriptManager> manager2) {
	auto lManager1 = manager1.lock();
	auto lManager2 = manager2.lock();
	if (lManager1 != nullptr && lManager2 != nullptr) {
		lManager1->AddRelativeScriptManager(manager2);
		lManager2->AddRelativeScriptManager(manager1);
	}
}

/**********************************************************
//ManagedScript
**********************************************************/
const function commonFunction[] =
{
	//関数：

	//制御共通関数：スクリプト操作
	{ "LoadScript", ManagedScript::Func_LoadScript, 1 },
	{ "LoadScriptInThread", ManagedScript::Func_LoadScriptInThread, 1 },
	{ "StartScript", ManagedScript::Func_StartScript, 1 },
	{ "CloseScript", ManagedScript::Func_CloseScript, 1 },
	{ "IsCloseScript", ManagedScript::Func_IsCloseScript, 1 },
	{ "GetOwnScriptID", ManagedScript::Func_GetOwnScriptID, 0 },
	{ "GetEventType", ManagedScript::Func_GetEventType, 0 },
	{ "GetEventArgument", ManagedScript::Func_GetEventArgument, 1 },
	{ "SetScriptArgument", ManagedScript::Func_SetScriptArgument, 3 },
	{ "GetScriptResult", ManagedScript::Func_GetScriptResult, 1 },
	{ "SetAutoDeleteObject", ManagedScript::Func_SetAutoDeleteObject, 1 },
	{ "NotifyEvent", ManagedScript::Func_NotifyEvent, 3 },
	{ "NotifyEventOwn", ManagedScript::Func_NotifyEventOwn, 2 },
	{ "NotifyEventAll", ManagedScript::Func_NotifyEventAll, 2 },
	{ "PauseScript", ManagedScript::Func_PauseScript, 2 },

};
ManagedScript::ManagedScript() {
	scriptManager_ = nullptr;
	_AddFunction(commonFunction, sizeof(commonFunction) / sizeof(function));

	bLoad_ = false;
	bEndScript_ = false;
	bAutoDeleteObject_ = false;

	bPaused_ = false;

	typeEvent_ = -1;
}
void ManagedScript::SetScriptManager(ScriptManager* manager) {
	scriptManager_ = manager;
	mainThreadID_ = scriptManager_->GetMainThreadID();
	idScript_ = scriptManager_->IssueScriptID();
}
gstd::value ManagedScript::RequestEvent(int type, std::vector<gstd::value>& listValue) {
	gstd::value res;
	std::map<std::string, script_engine::block*>::iterator itrEvent;
	if (!IsEventExists("Event", itrEvent)) {
		return res;
	}

	//Run() will overwrite these if it invokes another RequestEvent
	int tEventType = typeEvent_;
	std::vector<gstd::value> tArgs = listValueEvent_;
	gstd::value tValue = valueRes_;

	typeEvent_ = type;
	listValueEvent_ = listValue;
	valueRes_ = gstd::value();

	Run(itrEvent);
	res = GetResultValue();

	typeEvent_ = tEventType;
	listValueEvent_ = tArgs;
	valueRes_ = tValue;

	return res;
}



//STG制御共通関数：スクリプト操作
gstd::value ManagedScript::Func_LoadScript(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	std::wstring path = argv[0].as_string();
	int type = script->GetScriptType();
	shared_ptr<ManagedScript> target = scriptManager->Create(type);
	target->scriptParam_ = script->scriptParam_;

	int64_t res = scriptManager->LoadScript(path, target);
	return value(machine->get_engine()->get_real_type(), (float)res);
}
gstd::value ManagedScript::Func_LoadScriptInThread(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	std::wstring path = argv[0].as_string();
	int type = script->GetScriptType();
	shared_ptr<ManagedScript> target = scriptManager->Create(type);
	target->scriptParam_ = script->scriptParam_;
	int64_t res = scriptManager->LoadScriptInThread(path, target);
	return value(machine->get_engine()->get_real_type(), (float)res);
}
gstd::value ManagedScript::Func_StartScript(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = (int64_t)argv[0].as_real();
	scriptManager->StartScript(idScript);
	return value();
}
gstd::value ManagedScript::Func_CloseScript(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = (int64_t)argv[0].as_real();
	scriptManager->CloseScript(idScript);
	return value();
}
gstd::value ManagedScript::Func_IsCloseScript(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = (int64_t)argv[0].as_real();
	bool res = scriptManager->IsCloseScript(idScript);

	return value(machine->get_engine()->get_boolean_type(), res);
}

gstd::value ManagedScript::Func_GetOwnScriptID(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	int64_t res = script->GetScriptID();
	return value(machine->get_engine()->get_real_type(), (float)res);
}
gstd::value ManagedScript::Func_GetEventType(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	int res = script->typeEvent_;
	return value(machine->get_engine()->get_real_type(), (float)res);
}
gstd::value ManagedScript::Func_GetEventArgument(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	int index = (int)argv[0].as_real();
	if (index < 0 || index >= script->listValueEvent_.size())
		throw gstd::wexception("Invalid event argument index.");
	return script->listValueEvent_[index];
}
gstd::value ManagedScript::Func_SetScriptArgument(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = (int64_t)argv[0].as_real();
	shared_ptr<ManagedScript> target = scriptManager->GetScript(idScript);
	if (target) {
		int index = (int)argv[1].as_real();
		target->SetArgumentValue(argv[2], index);
	}
	return value();
}
gstd::value ManagedScript::Func_GetScriptResult(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = (int64_t)argv[0].as_real();
	gstd::value res = scriptManager->GetScriptResult(idScript);
	return res;
}
gstd::value ManagedScript::Func_SetAutoDeleteObject(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;

	bool bAutoDelete = argv[0].as_boolean();
	script->SetAutoDeleteObject(bAutoDelete);
	return value();
}
gstd::value ManagedScript::Func_NotifyEvent(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	script->CheckRunInMainThread();
	auto scriptManager = script->scriptManager_;

	gstd::value res;
	int64_t idScript = (int64_t)argv[0].as_real();
	shared_ptr<ManagedScript> target = scriptManager->GetScript(idScript);
	if (target) {
		int type = (int)argv[1].as_real();
		std::vector<gstd::value> listArg;
		listArg.push_back(argv[2]);
		res = target->RequestEvent(type, listArg);
	}
	return res;
}
gstd::value ManagedScript::Func_NotifyEventOwn(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	script->CheckRunInMainThread();

	int type = (int)argv[0].as_real();
	std::vector<gstd::value> listArg;
	listArg.push_back(argv[1]);

	gstd::value res = script->RequestEvent(type, listArg);
	return res;
}
gstd::value ManagedScript::Func_NotifyEventAll(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	script->CheckRunInMainThread();

	auto scriptManager = script->scriptManager_;

	int type = (int)argv[0].as_real();
	std::vector<gstd::value> listArg;
	listArg.push_back(argv[1]);
	scriptManager->RequestEventAll(type, listArg);

	return value();
}
gstd::value ManagedScript::Func_PauseScript(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	script->CheckRunInMainThread();

	auto scriptManager = script->scriptManager_;

	int64_t idScript = (int64_t)argv[0].as_real();
	bool state = argv[1].as_boolean();
	if (idScript == script->GetScriptID())
		throw gstd::wexception("A script is not allowed to pause itself.");

	shared_ptr<ManagedScript> target = scriptManager->GetScript(idScript);
	if (target) target->bPaused_ = state;

	return value();
}

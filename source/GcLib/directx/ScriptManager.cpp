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
	
	for (auto itr = listScriptRun_.begin(); itr != listScriptRun_.end(); ) {
		shared_ptr<ManagedScript> script = *itr;
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
	//What?
}
shared_ptr<ManagedScript> ScriptManager::GetScript(int64_t id, bool bSearchRelative) {
	shared_ptr<ManagedScript> res = nullptr;
	{
		Lock lock(lock_);

		auto itr = mapScriptLoad_.find(id);
		if (itr != mapScriptLoad_.end()) {
			res = itr->second;
		}
		else {
			for (auto pScriptRun : listScriptRun_) {
				if (pScriptRun->GetScriptID() == id) {
					res = pScriptRun;
					break;
				}
			}

			if (res == nullptr && bSearchRelative) {
				for (auto pWeakManager : listRelativeManager_) {
					if (auto pManager = pWeakManager.lock()) {
						for (auto pScript : pManager->listScriptRun_) {
							if (pScript->GetScriptID() == id) {
								res = pScript;
								break;
							}
						}
					}
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
	for (auto pScript : listScriptRun_) {
		if (pScript->GetScriptID() == id) {
			pScript->SetEndScript();

			mapClosedScriptResult_[id] = pScript->GetResultValue();
			if (mapClosedScriptResult_.size() > MAX_CLOSED_SCRIPT_RESULT) {
				int64_t targetID = mapClosedScriptResult_.begin()->first;
				mapClosedScriptResult_.erase(targetID);
			}

			if (pScript->IsAutoDeleteObject())
				pScript->GetObjectManager()->DeleteObjectByScriptID(id);

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
	for (auto pScript : listScriptRun_) {
		if (pScript->GetScriptType() == type) {
			pScript->SetEndScript();

			int64_t id = pScript->GetScriptID();
			mapClosedScriptResult_[id] = pScript->GetResultValue();
			if (mapClosedScriptResult_.size() > MAX_CLOSED_SCRIPT_RESULT) {
				int64_t targetID = mapClosedScriptResult_.begin()->first;
				mapClosedScriptResult_.erase(targetID);
			}

			if (pScript->IsAutoDeleteObject())
				pScript->GetObjectManager()->DeleteObjectByScriptID(id);
		}
	}
}
bool ScriptManager::IsCloseScript(int64_t id) {
	shared_ptr<ManagedScript> script = GetScript(id);
	bool res = script == nullptr || script->IsEndScript();
	return res;
}
size_t ScriptManager::GetAllScriptThreadCount() {
	size_t res = 0;
	{
		Lock lock(lock_);
		for (auto pScript : listScriptRun_) {
			res += pScript->GetThreadCount();
		}
	}
	return res;
}
void ScriptManager::TerminateScriptAll(const std::wstring& message) {
	{
		Lock lock(lock_);
		for (auto pScript : listScriptRun_) {
			pScript->Terminate(message);
		}
	}
}
int64_t ScriptManager::_LoadScript(const std::wstring& path, shared_ptr<ManagedScript> script) {
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
int64_t ScriptManager::LoadScript(const std::wstring& path, shared_ptr<ManagedScript> script) {
	int64_t res = 0;
	{
		Lock lock(lock_);
		res = _LoadScript(path, script);
		mapScriptLoad_[res] = script;
	}
	return res;
}
shared_ptr<ManagedScript> ScriptManager::LoadScript(const std::wstring& path, int type) {
	shared_ptr<ManagedScript> script = Create(type);
	this->LoadScript(path, script);
	return script;
}
int64_t ScriptManager::LoadScriptInThread(const std::wstring& path, shared_ptr<ManagedScript> script) {
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
shared_ptr<ManagedScript> ScriptManager::LoadScriptInThread(const std::wstring& path, int type) {
	shared_ptr<ManagedScript> script = Create(type);
	LoadScriptInThread(path, script);
	return script;
}
void ScriptManager::CallFromLoadThread(shared_ptr<gstd::FileManager::LoadThreadEvent> event) {
	std::wstring path = event->GetPath();

	shared_ptr<ManagedScript> script = std::dynamic_pointer_cast<ManagedScript>(event->GetSource());
	if (script == nullptr || script->IsLoad()) return;

	try {
		_LoadScript(path, script);
	}
	catch (gstd::wexception& e) {
		Logger::WriteTop(e.what());
		script->bLoad_ = true;
		SetError(e.what());
	}
}
void ScriptManager::RequestEventAll(int type, const gstd::value* listValue, size_t countArgument) {
	{
		for (auto pScript : listScriptRun_) {
			if (pScript->IsEndScript() /*|| pScript->IsPaused()*/) continue;
			pScript->RequestEvent(type, listValue, countArgument);
		}
	}

	for (auto itrManager = listRelativeManager_.begin(); itrManager != listRelativeManager_.end(); ) {
		if (auto manager = itrManager->lock()) {
			for (auto pScript : manager->listScriptRun_) {
				if (pScript->IsEndScript() /*|| pScript->IsPaused()*/) continue;
				pScript->RequestEvent(type, listValue, countArgument);
			}
			itrManager++;
		}
		else {
			itrManager = listRelativeManager_.erase(itrManager);
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
	{ "GetEventArgumentCount", ManagedScript::Func_GetEventArgumentCount, 0 },
	{ "SetScriptArgument", ManagedScript::Func_SetScriptArgument, 3 },
	{ "GetScriptResult", ManagedScript::Func_GetScriptResult, 1 },
	{ "SetAutoDeleteObject", ManagedScript::Func_SetAutoDeleteObject, 1 },
	{ "NotifyEvent", ManagedScript::Func_NotifyEvent, -4 },			//2 fixed + ... -> 3 minimum
	{ "NotifyEventOwn", ManagedScript::Func_NotifyEventOwn, -3 },	//1 fixed + ... -> 2 minimum
	{ "NotifyEventAll", ManagedScript::Func_NotifyEventAll, -3 },	//1 fixed + ... -> 2 minimum
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
	listValueEvent_ = nullptr;
	listValueEventSize_ = 0;
}
ManagedScript::~ManagedScript() {
	//listValueEvent_ shouldn't be delete'd, that's the job of whatever was calling RequestEvent,
	//	doing so will cause a memory corruption + crash if there was a script error in Run().

	//ptr_delete_scalar(listValueEvent_);
	listValueEventSize_ = 0;
}
void ManagedScript::SetScriptManager(ScriptManager* manager) {
	scriptManager_ = manager;
	mainThreadID_ = scriptManager_->GetMainThreadID();
	idScript_ = scriptManager_->IssueScriptID();
}
gstd::value ManagedScript::RequestEvent(int type) {
	return RequestEvent(type, nullptr, 0);
}
gstd::value ManagedScript::RequestEvent(int type, const gstd::value* listValue, size_t countArgument) {
	gstd::value res;
	std::map<std::string, script_engine::block*>::iterator itrEvent;
	if (!IsEventExists("Event", itrEvent)) {
		return res;
	}

	//Run() may overwrite these if it invokes another RequestEvent
	int prevEventType = typeEvent_;
	gstd::value* prevArgv = listValueEvent_;
	size_t prevArgc = listValueEventSize_;
	gstd::value prevValue = valueRes_;

	typeEvent_ = type;
	listValueEvent_ = const_cast<value*>(listValue);
	listValueEventSize_ = countArgument;
	valueRes_ = gstd::value();

	Run(itrEvent);
	res = GetResultValue();

	//Restore previous values
	typeEvent_ = prevEventType;
	listValueEvent_ = prevArgv;
	listValueEventSize_ = prevArgc;
	valueRes_ = prevValue;

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
	return script->CreateRealValue(res);
}
gstd::value ManagedScript::Func_LoadScriptInThread(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	std::wstring path = argv[0].as_string();
	int type = script->GetScriptType();
	shared_ptr<ManagedScript> target = scriptManager->Create(type);
	target->scriptParam_ = script->scriptParam_;

	int64_t res = scriptManager->LoadScriptInThread(path, target);
	return script->CreateRealValue(res);
}
gstd::value ManagedScript::Func_StartScript(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = argv[0].as_int();
	scriptManager->StartScript(idScript);
	return value();
}
gstd::value ManagedScript::Func_CloseScript(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = argv[0].as_int();
	scriptManager->CloseScript(idScript);
	return value();
}
gstd::value ManagedScript::Func_IsCloseScript(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = argv[0].as_int();
	bool res = scriptManager->IsCloseScript(idScript);

	return script->CreateBooleanValue(res);
}

gstd::value ManagedScript::Func_GetOwnScriptID(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	int64_t res = script->GetScriptID();
	return script->CreateRealValue(res);
}
gstd::value ManagedScript::Func_GetEventType(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	int res = script->typeEvent_;
	return script->CreateRealValue(res);
}
gstd::value ManagedScript::Func_GetEventArgument(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	int index = argv[0].as_int();
	if (index < 0 || index >= script->listValueEventSize_) {
		script->RaiseError(StringUtility::Format("Invalid event argument index: %d. [max=%d]",
			index, script->listValueEventSize_));
	}
	return script->listValueEvent_[index];
}
gstd::value ManagedScript::Func_GetEventArgumentCount(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	return script->CreateRealValue(script->listValueEventSize_);
}
gstd::value ManagedScript::Func_SetScriptArgument(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = argv[0].as_int();
	shared_ptr<ManagedScript> target = scriptManager->GetScript(idScript, true);
	if (target) {
		int index = argv[1].as_int();
		target->SetArgumentValue(argv[2], index);
	}
	return value();
}
gstd::value ManagedScript::Func_GetScriptResult(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = argv[0].as_int();
	gstd::value res = scriptManager->GetScriptResult(idScript);
	return res;
}
gstd::value ManagedScript::Func_SetAutoDeleteObject(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	script->SetAutoDeleteObject(argv[0].as_boolean());
	return value();
}
gstd::value ManagedScript::Func_NotifyEvent(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	script->CheckRunInMainThread();
	auto scriptManager = script->scriptManager_;

	gstd::value res;

	int64_t idScript = argv[0].as_int();
	shared_ptr<ManagedScript> target = scriptManager->GetScript(idScript, true);
	if (target) {
		//(script id, event type, ...)
		int type = argv[1].as_int();
		res = target->RequestEvent(type, (const value*)(argv + 2), argc - 2);
	}

	return res;
}
gstd::value ManagedScript::Func_NotifyEventOwn(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	script->CheckRunInMainThread();

	//(event type, ...)
	int type = argv[0].as_int();
	return script->RequestEvent(type, (const value*)(argv + 1), argc - 1);
}
gstd::value ManagedScript::Func_NotifyEventAll(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	script->CheckRunInMainThread();

	//(event type, ...)
	int type = argv[0].as_int();
	script->scriptManager_->RequestEventAll(type, (const value*)(argv + 1), argc - 1);

	return value();
}
gstd::value ManagedScript::Func_PauseScript(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	script->CheckRunInMainThread();

	auto scriptManager = script->scriptManager_;

	int64_t idScript = argv[0].as_int();
	bool state = argv[1].as_boolean();
	if (idScript == script->GetScriptID())
		script->RaiseError("A script is not allowed to pause itself.");

	shared_ptr<ManagedScript> target = scriptManager->GetScript(idScript, true);
	if (target) target->bPaused_ = state;

	return value();
}

#include "source/GcLib/pch.h"

#include "ScriptManager.hpp"

using namespace gstd;
using namespace directx;

//*******************************************************************
//ScriptManager
//*******************************************************************
std::atomic<int64_t> ScriptManager::idScript_ = 0;
ScriptManager::ScriptManager() {
	mainThreadID_ = GetCurrentThreadId();

	bCancelLoad_ = false;
	nActiveScriptLoad_ = 0;

	bHasCloseScriptWork_ = false;

	FileManager::GetBase()->AddLoadThreadListener(this);
}
ScriptManager::~ScriptManager() {
	//this->WaitForCancel();
	FileManager::GetBase()->RemoveLoadThreadListener(this);
}

void ScriptManager::Work() {
	Work(ManagedScript::TYPE_ALL);
}
void ScriptManager::Work(int targetType) {
	bHasCloseScriptWork_ = false;

	LARGE_INTEGER startTime, endTime;
	LARGE_INTEGER timeFreq;
	QueryPerformanceFrequency(&timeFreq);
	
	for (auto itr = listScriptRun_.begin(); itr != listScriptRun_.end(); ) {
		shared_ptr<ManagedScript> script = *itr;
		int type = script->GetScriptType();

		if (script->IsPaused())
			script->runTime_ = 0;
		if (script->IsPaused() || (targetType != ManagedScript::TYPE_ALL && targetType != type)) {
			++itr;
			continue;
		}

		QueryPerformanceCounter(&startTime);
		if (script->IsEndScript()) {
			std::map<std::string, script_block*>::iterator itrEvent;
			if (script->IsEventExists("Finalize", itrEvent))
				script->Run(itrEvent);

			bHasCloseScriptWork_ |= true;
			itr = listScriptRun_.erase(itr);
		}
		else {
			std::map<std::string, script_block*>::iterator itrEvent;
			if (script->IsEventExists("MainLoop", itrEvent))
				script->Run(itrEvent);

			bHasCloseScriptWork_ |= script->IsEndScript();
			++itr;
		}
		QueryPerformanceCounter(&endTime);

		script->runTime_ = (endTime.QuadPart - startTime.QuadPart) * 1000000ULL / timeFreq.QuadPart;
	}
}
void ScriptManager::Render() {
	//What?
}
shared_ptr<ManagedScript> ScriptManager::GetScript(int64_t id, bool bSearchInLoad, bool bSearchRelative) {
	shared_ptr<ManagedScript> res = nullptr;
	{
		Lock lock(lock_);

		if (bSearchInLoad) {
			auto itr = mapScriptLoad_.find(id);
			if (itr != mapScriptLoad_.end()) {
				res = itr->second;
			}
		}
		if (res == nullptr) {
			for (auto pScriptRun : listScriptRun_) {
				if (pScriptRun && pScriptRun->GetScriptID() == id) {
					res = pScriptRun;
					goto get_script_res;
				}
			}

			if (bSearchRelative) {
				for (auto pWeakManager : listRelativeManager_) {
					LOCK_WEAK(pManager, pWeakManager) {
						for (auto pScript : pManager->listScriptRun_) {
							if (pScript && pScript->GetScriptID() == id) {
								res = pScript;
								goto get_script_res;
							}
						}
					}
				}
			}
		}
get_script_res:;
	}
	return res;
}
void ScriptManager::StartScript(int64_t id, bool bUnload) {
	std::map<int64_t, shared_ptr<ManagedScript>>::iterator itrMap;
	{
		Lock lock(lock_);

		itrMap = mapScriptLoad_.find(id);
		if (itrMap == mapScriptLoad_.end()) return;

		StartScript(itrMap->second, bUnload);
	}
}
void ScriptManager::StartScript(shared_ptr<ManagedScript> script, bool bUnload) {
	if (!script->IsLoad()) {
		DWORD count = 0;
		while (!script->IsLoad()) {
			if (count % 100 == 0) {
				Logger::WriteTop(StringUtility::Format(L"ScriptManager: Script is still loading... [%s]",
					PathProperty::ReduceModuleDirectory(script->GetPath()).c_str()));
			}
			::Sleep(10);
			++count;
		}
	}

	{
		Lock lock(lock_);

		for (auto& iScript : listScriptRun_) {
			if (iScript->GetScriptID() == script->GetScriptID()) {
				Logger::WriteTop(StringUtility::Format(
					L"ScriptManager: Cannot run multiple instances of the same loaded script simultaneously. [%s]\r\n",
					PathProperty::ReduceModuleDirectory(script->GetPath()).c_str()));
				return;
			}
		}

		if (bUnload)
			mapScriptLoad_.erase(script->GetScriptID());
		listScriptRun_.push_back(script);
	}

	if (script) {
		TryThrowError();	//Rethrows the error (for scripts loaded in the load thread)

		script->Reset();
		script->bRunning_ = true;

		script->Run();	//Execute code in the global scope

		std::map<std::string, script_block*>::iterator itrEvent;
		if (script->IsEventExists("Initialize", itrEvent))
			script->Run(itrEvent);
	}
}
void ScriptManager::CloseScript(int64_t id) {
	for (auto& pScript : listScriptRun_) {
		if (pScript->GetScriptID() == id) {
			CloseScript(pScript);
			break;
		}
	}
}
void ScriptManager::CloseScript(shared_ptr<ManagedScript> id) {
	if (id == nullptr) return;

	id->SetEndScript();
	id->bRunning_ = false;

	mapClosedScriptResult_[id->GetScriptID()] = id->GetResultValue();
	if (mapClosedScriptResult_.size() > MAX_CLOSED_SCRIPT_RESULT)
		mapClosedScriptResult_.erase(mapClosedScriptResult_.begin());

	if (id->IsAutoDeleteObject())
		id->GetObjectManager()->DeleteObjectByScriptID(id->GetScriptID());
	
	// id->GetObjectManager()->OrphanObjectByScriptID(id->GetScriptID());
}
void ScriptManager::CloseScriptOnType(int type) {
	for (auto& pScript : listScriptRun_) {
		if (pScript->GetScriptType() == type) {
			CloseScript(pScript);
		}
	}
}
bool ScriptManager::IsCloseScript(int64_t id) {
	shared_ptr<ManagedScript> script = GetScript(id, true, true);
	bool res = script == nullptr || script->IsEndScript();
	return res;
}
size_t ScriptManager::GetAllScriptThreadCount() {
	size_t res = 0;
	{
		//Lock lock(lock_);
		for (auto& pScript : listScriptRun_) {
			res += pScript->GetThreadCount();
		}
	}
	return res;
}
void ScriptManager::TerminateScriptAll(const std::wstring& message) {
	{
		Lock lock(lock_);
		for (auto& pScript : listScriptRun_) {
			pScript->Terminate(message);
		}
	}
}
void ScriptManager::OrphanAllScripts() {
	{
		Lock lock(lock_);

		mapScriptLoad_.clear();
		listScriptRun_.clear();

		/*
		for (auto itr = listRelativeManager_.begin(); itr != listRelativeManager_.end(); ++itr) {
			if (auto pManager = itr->lock())
				pManager->OrphanAllScripts();
		}
		*/
	}
}

int64_t ScriptManager::_LoadScript(const std::wstring& path, shared_ptr<ManagedScript> script) {
	++nActiveScriptLoad_;
	int64_t res = script->GetScriptID();

	script->bBeginLoad_ = true;

	script->SetSourceFromFile(path);
	script->Compile();

	std::map<std::string, script_block*>::iterator itrEvent;
	if (script->IsEventExists("Loading", itrEvent))
		script->Run(itrEvent);

	script->bLoad_ = true;
	script->bRunning_ = false;
	{
		//Lock lock(lock_);
		StaticLock lock = StaticLock();
		mapScriptLoad_[res] = script;
	}

	--nActiveScriptLoad_;
	return res;
}
int64_t ScriptManager::LoadScript(const std::wstring& path, shared_ptr<ManagedScript> script) {
	int64_t res = _LoadScript(path, script);
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

		script->SetPath(path);

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
	const std::wstring& path = event->GetPath();

	shared_ptr<ManagedScript> script = std::dynamic_pointer_cast<ManagedScript>(event->GetSource());
	if (script == nullptr || script->IsLoad()) return;

	if (bCancelLoad_) {
		script->bLoad_ = true;
		return;
	}

	try {
		_LoadScript(path, script);
	}
	catch (gstd::wexception& e) {
		Logger::WriteTop(e.what());
		script->bLoad_ = true;
		SetError(e.what());
	}
}
void ScriptManager::UnloadScript(int64_t id) {
	Lock lock(lock_);
	mapScriptLoad_.erase(id);
}
void ScriptManager::UnloadScript(shared_ptr<ManagedScript> script) {
	UnloadScript(script->GetScriptID());
}

void ScriptManager::RequestEventAll(int type, const gstd::value* listValue, size_t countArgument) {
	{
		for (auto& pScript : listScriptRun_) {
			if (pScript->IsEndScript() /*|| pScript->IsPaused()*/) continue;
			pScript->RequestEvent(type, listValue, countArgument);
		}
	}

	for (auto itrManager = listRelativeManager_.begin(); itrManager != listRelativeManager_.end(); ) {
		if (auto manager = itrManager->lock()) {
			for (auto& pScript : manager->listScriptRun_) {
				if (pScript->IsEndScript() /*|| pScript->IsPaused()*/) continue;
				pScript->RequestEvent(type, listValue, countArgument);
			}
			++itrManager;
		}
		else {
			itrManager = listRelativeManager_.erase(itrManager);
		}
	}
}
gstd::value ScriptManager::GetScriptResult(int64_t idScript) {
	gstd::value res;
	shared_ptr<ManagedScript> script = GetScript(idScript, false, true);
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

//*******************************************************************
//ManagedScript
//*******************************************************************
static const std::vector<function> managedScriptFunction = {
	{ "LoadScript", ManagedScript::Func_LoadScript, 1 },
	{ "LoadScriptInThread", ManagedScript::Func_LoadScriptInThread, 1 },
	{ "UnloadScript", ManagedScript::Func_UnloadScript, 1 },
	{ "UnloadScriptFromCache", ManagedScript::Func_UnloadScriptFromCache, 1 },

	{ "StartScript", ManagedScript::Func_StartScript, 1 },
	{ "StartScript", ManagedScript::Func_StartScript, 2 },	//Overloaded

	{ "CloseScript", ManagedScript::Func_CloseScript, 1 },

	{ "IsCloseScript", ManagedScript::Func_IsCloseScript, 1 },
	{ "GetOwnScriptID", ManagedScript::Func_GetOwnScriptID, 0 },

	{ "GetEventType", ManagedScript::Func_GetEventType, 0 },
	{ "GetEventArgument", ManagedScript::Func_GetEventArgument, 1 },
	{ "GetEventArgumentCount", ManagedScript::Func_GetEventArgumentCount, 0 },
	{ "SetScriptArgument", ManagedScript::Func_SetScriptArgument, 3 },
	{ "GetScriptResult", ManagedScript::Func_GetScriptResult, 1 },
	{ "SetAutoDeleteObject", ManagedScript::Func_SetAutoDeleteObject, 1 },
	{ "GetAllObjectIdInScript", ManagedScript::Func_GetAllObjectIdInScript, 0 },
	{ "GetAllObjectIdInScript", ManagedScript::Func_GetAllObjectIdInScript, 1 }, //Overloaded
	{ "GetAllObjectIdInPool", ManagedScript::Func_GetAllObjectIdInPool, 0 },

	{ "NotifyEvent", ManagedScript::Func_NotifyEvent, -3 },			//2 fixed (+ ...) -> 2 minimum
	{ "NotifyEventOwn", ManagedScript::Func_NotifyEventOwn, -2 },	//1 fixed (+ ...) -> 1 minimum
	{ "NotifyEventAll", ManagedScript::Func_NotifyEventAll, -2 },	//1 fixed (+ ...) -> 1 minimum
	{ "PauseScript", ManagedScript::Func_PauseScript, 2 },

	{ "GetScriptStatus", ManagedScript::Func_GetScriptStatus, 1 },
};
static const std::vector<constant> managedScriptConstant = {
	constant("STATUS_INVALID", ManagedScript::STATUS_INVALID),
	constant("STATUS_LOADED", ManagedScript::STATUS_LOADED),
	constant("STATUS_LOADING", ManagedScript::STATUS_LOADING),
	constant("STATUS_RUNNING", ManagedScript::STATUS_RUNNING),
	constant("STATUS_PAUSED", ManagedScript::STATUS_PAUSED),
	constant("STATUS_CLOSING", ManagedScript::STATUS_CLOSING),
};

ManagedScript::ManagedScript() {
	scriptManager_ = nullptr;

	_AddFunction(&managedScriptFunction);
	_AddConstant(&managedScriptConstant);

	bBeginLoad_ = false;
	bLoad_ = false;

	bEndScript_ = false;
	bAutoDeleteObject_ = false;
	bRunning_ = false;
	bPaused_ = false;

	runTime_ = 0;

	typeEvent_ = -1;
	listValueEvent_ = nullptr;
	listValueEventSize_ = 0;
}
ManagedScript::~ManagedScript() {
	//listValueEvent_ shouldn't be delete'd, that's the job of whatever was calling RequestEvent,
	//	doing so will cause a memory corruption + crash if there was a script error in Run().

	//ptr_delete_scalar(listValueEvent_);
	listValueEventSize_ = 0;

	/*
	Logger::WriteTop(StringUtility::Format(
		L"Released script: %s (Manager=%08x, Type=%d)",
		PathProperty::GetFileName(GetPath()).c_str(), (int)scriptManager_, typeScript_));
	*/
}

void ManagedScript::Reset() {
	ScriptClientBase::Reset();

	bEndScript_ = false;
	bRunning_ = false;
	bPaused_ = false;
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
	std::map<std::string, script_block*>::iterator itrEvent;
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
	return script->CreateIntValue(res);
}
gstd::value ManagedScript::Func_LoadScriptInThread(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	std::wstring path = argv[0].as_string();
	int type = script->GetScriptType();
	shared_ptr<ManagedScript> target = scriptManager->Create(type);
	target->scriptParam_ = script->scriptParam_;

	int64_t res = scriptManager->LoadScriptInThread(path, target);
	return script->CreateIntValue(res);
}
gstd::value ManagedScript::Func_UnloadScript(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = argv[0].as_int();
	scriptManager->UnloadScript(idScript);

	return value();
}
gstd::value ManagedScript::Func_UnloadScriptFromCache(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;

	auto cache = script->GetScriptEngineCache();
	cache->RemoveCache(argv[0].as_string());

	return value();
}
gstd::value ManagedScript::Func_StartScript(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	bool bUnload = true;
	if (argc == 2)
		bUnload = argv[1].as_boolean();

	int64_t idScript = argv[0].as_int();
	scriptManager->StartScript(idScript, bUnload);

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
	return script->CreateIntValue(res);
}
gstd::value ManagedScript::Func_GetEventType(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	int res = script->typeEvent_;
	return script->CreateIntValue(res);
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
	return script->CreateIntValue(script->listValueEventSize_);
}
gstd::value ManagedScript::Func_SetScriptArgument(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int64_t idScript = argv[0].as_int();
	shared_ptr<ManagedScript> target = scriptManager->GetScript(idScript, true, true);
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
gstd::value ManagedScript::Func_GetAllObjectIdInScript(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	int64_t idScript = script->idScript_;
	if (argc == 1) idScript = argv[0].as_int();

	std::vector<int> res = script->GetObjectManager()->GetObjectByScriptID(idScript);

	return script->CreateIntArrayValue(res);
}
gstd::value ManagedScript::Func_GetAllObjectIdInPool(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;

	std::vector<int> res = script->GetObjectManager()->GetValidObjectIdentifier();

	return script->CreateIntArrayValue(res);
}
gstd::value ManagedScript::Func_NotifyEvent(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	script->CheckRunInMainThread();
	auto scriptManager = script->scriptManager_;

	gstd::value res;

	int64_t idScript = argv[0].as_int();
	shared_ptr<ManagedScript> target = scriptManager->GetScript(idScript, false, true);
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

	shared_ptr<ManagedScript> target = scriptManager->GetScript(idScript, false, true);
	if (target)
		target->bPaused_ = state;

	return value();
}
gstd::value ManagedScript::Func_GetScriptStatus(script_machine* machine, int argc, const value* argv) {
	ManagedScript* script = (ManagedScript*)machine->data;
	auto scriptManager = script->scriptManager_;

	int res = STATUS_INVALID;
	int64_t idScript = argv[0].as_int();

	shared_ptr<ManagedScript> pScript = scriptManager->GetScript(idScript, true, true);
	if (pScript) {
		if (pScript->bEndScript_)
			res = STATUS_CLOSING;
		else if (pScript->bRunning_)
			res = pScript->bPaused_ ? STATUS_PAUSED : STATUS_RUNNING;
		else if (pScript->bBeginLoad_)
			res = pScript->bLoad_ ? STATUS_LOADED : STATUS_LOADING;
	}

	return CreateIntValue(res);
}

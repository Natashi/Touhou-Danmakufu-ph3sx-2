#include "source/GcLib/pch.h"

#include "Task.hpp"

using namespace gstd;

//****************************************************************************
//TaskBase
//****************************************************************************
TaskBase::TaskBase() {
	indexTask_ = -1;
	idTask_ = TASK_FREE_ID;
	idTaskGroup_ = TASK_GROUP_FREE_ID;
}
TaskBase::~TaskBase() {
}

//****************************************************************************
//TaskManager
//****************************************************************************
gstd::CriticalSection TaskManager::lockStatic_;
TaskManager::TaskManager() {
	indexTaskManager_ = 0;

	timeSpentLastCall_ = 0;
}
TaskManager::~TaskManager() {
	this->Clear();
	panelInfo_ = nullptr;
}
void TaskManager::_CheckInvalidFunctionDivision(int divFunc) {
	if (mapFunc_.find(divFunc) == mapFunc_.end())
		throw gstd::wexception("TaskManager: Invalid function division");
}
void TaskManager::ArrangeTask() {
	// Erase dead tasks
	for (auto itrTask = listTask_.begin(); itrTask != listTask_.end();) {
		if (*itrTask == nullptr)
			itrTask = listTask_.erase(itrTask);
		else ++itrTask;
	}

	// Erase finished functions
	for (auto& [division, funcs] : mapFunc_) {
		for (auto& iListFunc : funcs) {
			for (auto itrFunc = iListFunc.begin(); itrFunc != iListFunc.end();) {
				if (*itrFunc == nullptr)
					itrFunc = iListFunc.erase(itrFunc);
				else ++itrFunc;
			}
		}
	}

	//if (panelInfo_) panelInfo_->Update(this);
}
void TaskManager::Clear() {
	listTask_.clear();
	mapFunc_.clear();
}
void TaskManager::ClearTask() {
	listTask_.clear();
	for (auto& [_, funcs] : mapFunc_)
		funcs.clear();
}
void TaskManager::AddTask(shared_ptr<TaskBase> task) {
	for (auto& iTask : listTask_) {
		if (iTask == task) return;
	}
//	task->mTask_ = this;
	task->indexTask_ = indexTaskManager_++;

	listTask_.push_back(task);
}
shared_ptr<TaskBase> TaskManager::GetTask(int idTask) {
	for (auto& iTask : listTask_) {
		if (iTask == nullptr) continue;
		if (iTask->idTask_ != idTask) continue;
		return iTask;
	}
	return nullptr;
}
shared_ptr<TaskBase> TaskManager::GetTask(const std::type_info& info) {
	for (auto& iTask : listTask_) {
		if (iTask == nullptr) continue;
		const std::type_info& tInfo = typeid(*(iTask.get()));
		if (info != tInfo) continue;
		return iTask;
	}
	return nullptr;
}
void TaskManager::RemoveTask(TaskBase* task) {
	for (auto& iTask : listTask_) {
		if (iTask == nullptr) continue;
		if (iTask.get() != task) continue;
		if (iTask->idTask_ != task->idTask_) continue;
		this->RemoveFunction(task);
		iTask = nullptr;
		break;
	}
}
void TaskManager::RemoveTask(int idTask) {
	for (auto& iTask : listTask_) {
		if (iTask == nullptr) continue;
		if (iTask->idTask_ != idTask) continue;
		this->RemoveFunction(iTask.get());
		iTask = nullptr;
		break;
	}
}
void TaskManager::RemoveTaskGroup(int idGroup) {
	for (auto& iTask : listTask_) {
		if (iTask == nullptr) continue;
		if (iTask->idTaskGroup_ != idGroup) continue;
		this->RemoveFunction(iTask.get());
		iTask = nullptr;
	}
}
void TaskManager::RemoveTask(const std::type_info& info) {
	for (auto& iTask : listTask_) {
		if (iTask == nullptr) continue;
		const std::type_info& tInfo = typeid(*(iTask.get()));
		if (info != tInfo) continue;
		this->RemoveFunction(iTask.get());
		iTask = nullptr;
	}
}
void TaskManager::RemoveTaskWithoutTypeInfo(std::set<const std::type_info*> listInfo) {
	for (auto& iTask : listTask_) {
		if (iTask == nullptr) continue;
		const std::type_info& tInfo = typeid(*(iTask.get()));
		if (listInfo.find(&tInfo) != listInfo.end()) continue;
		this->RemoveFunction(iTask.get());
		iTask = nullptr;
	}
}
void TaskManager::InitializeFunctionDivision(int divFunc, int maxPri) {
	if (mapFunc_.find(divFunc) != mapFunc_.end())
		throw gstd::wexception("TaskManager: Function Division already exists.");
	std::vector<std::list<shared_ptr<TaskFunction>>> vectPri;
	vectPri.resize(maxPri);
	mapFunc_[divFunc] = vectPri;
}
void TaskManager::CallFunction(int divFunc) {
	_CheckInvalidFunctionDivision(divFunc);

	timeSpentLastCall_ = 0;

	auto itrDiv = mapFunc_.find(divFunc);
	if (itrDiv != mapFunc_.end()) {
		auto timePrev = std::chrono::system_clock::now();
		for (auto& iListFunc : itrDiv->second) {
			for (auto& iFunc : iListFunc) {
				if (iFunc == nullptr || !iFunc->bEnable_) continue;
				if (iFunc->GetDelay() > 0) {
					iFunc->SetDelay(iFunc->GetDelay() - 1);
					continue;
				}
				iFunc->Call();
			}
		}
		timeSpentLastCall_ = (std::chrono::system_clock::now() - timePrev).count();
	}
}
void TaskManager::AddFunction(int divFunc, shared_ptr<TaskFunction> func, int pri, int idFunc) {
	auto itrDiv = mapFunc_.find(divFunc);
	if (itrDiv == mapFunc_.end())
		throw gstd::wexception(L"TaskManager: Division does not exist.");
	std::vector<std::list<shared_ptr<TaskFunction>>>& vectPri = itrDiv->second;
	func->id_ = idFunc;
	vectPri[pri].push_back(func);
}
void TaskManager::RemoveFunction(TaskBase* task) {
	for (auto& [name, div] : mapFunc_) {
		for (auto& listFunc : div) {
			for (auto& iFunc : listFunc) {
				if (iFunc == nullptr) continue;
				if (iFunc->task_.get() != task) continue;
				if (iFunc->task_->idTask_ != task->idTask_) continue;
				iFunc = nullptr;
			}
		}
	}
}
void TaskManager::RemoveFunction(TaskBase* task, int divFunc, int idFunc) {
	_CheckInvalidFunctionDivision(divFunc);

	auto itrDiv = mapFunc_.find(divFunc);
	if (itrDiv != mapFunc_.end()) {
		for (auto& iListFunc : itrDiv->second) {
			for (auto& iFunc : iListFunc) {
				if (iFunc == nullptr) continue;
				if (iFunc->id_ != idFunc) continue;
				if (iFunc->task_->idTask_ != task->idTask_) continue;
				iFunc = nullptr;
			}
		}
	}
}
void TaskManager::RemoveFunction(const std::type_info& info) {
	for (auto& [name, div] : mapFunc_) {
		for (auto& listFunc : div) {
			for (auto& iFunc : listFunc) {
				if (iFunc == nullptr) continue;
				const std::type_info& tInfo = typeid(*(iFunc->task_));
				if (info != tInfo) continue;
				iFunc = nullptr;
			}
		}
	}
}
void TaskManager::SetFunctionEnable(bool bEnable) {
	for (auto& [name, div] : mapFunc_) {
		for (auto& listFunc : div) {
			for (auto& iFunc : listFunc) {
				if (iFunc)
					iFunc->bEnable_ = bEnable;
			}
		}
	}
}
void TaskManager::SetFunctionEnable(bool bEnable, int divFunc) {
	_CheckInvalidFunctionDivision(divFunc);

	auto itrDiv = mapFunc_.find(divFunc);
	if (itrDiv != mapFunc_.end()) {
		for (auto& iListFunc : itrDiv->second) {
			for (auto& iFunc : iListFunc) {
				if (iFunc)
					iFunc->bEnable_ = bEnable;
			}
		}
	}
}
void TaskManager::SetFunctionEnable(bool bEnable, int idTask, int divFunc) {
	shared_ptr<TaskBase> task = this->GetTask(idTask);
	if (task == nullptr) return;
	this->SetFunctionEnable(bEnable, task.get(), divFunc);
}
void TaskManager::SetFunctionEnable(bool bEnable, int idTask, int divFunc, int idFunc) {
	shared_ptr<TaskBase> task = this->GetTask(idTask);
	if (task == nullptr) return;
	this->SetFunctionEnable(bEnable, task.get(), divFunc, idFunc);
}
void TaskManager::SetFunctionEnable(bool bEnable, TaskBase* task, int divFunc) {
	_CheckInvalidFunctionDivision(divFunc);

	auto itrDiv = mapFunc_.find(divFunc);
	if (itrDiv != mapFunc_.end()) {
		for (auto& iListFunc : itrDiv->second) {
			for (auto& iFunc : iListFunc) {
				if (iFunc == nullptr) continue;
				if (iFunc->task_.get() != task) continue;
				if (iFunc->task_->idTask_ != task->idTask_) continue;
				iFunc->bEnable_ = bEnable;
			}
		}
	}
}
void TaskManager::SetFunctionEnable(bool bEnable, TaskBase* task, int divFunc, int idFunc) {
	_CheckInvalidFunctionDivision(divFunc);

	auto itrDiv = mapFunc_.find(divFunc);
	if (itrDiv != mapFunc_.end()) {
		for (auto& iListFunc : itrDiv->second) {
			for (auto& iFunc : iListFunc) {
				if (iFunc == nullptr) continue;
				if (iFunc->task_.get() != task) continue;
				if (iFunc->task_->idTask_ != task->idTask_) continue;
				if (iFunc->id_ != idFunc) continue;
				iFunc->bEnable_ = bEnable;
			}
		}
	}
}
void TaskManager::SetFunctionEnable(bool bEnable, const std::type_info& info, int divFunc) {
	_CheckInvalidFunctionDivision(divFunc);

	auto itrDiv = mapFunc_.find(divFunc);
	if (itrDiv != mapFunc_.end()) {
		for (auto& iListFunc : itrDiv->second) {
			for (auto& iFunc : iListFunc) {
				if (iFunc == nullptr) continue;
				const std::type_info& tInfo = typeid(*(iFunc->task_));
				if (info != tInfo) continue;
				iFunc->bEnable_ = bEnable;
			}
		}
	}
}

//****************************************************************************
//TaskInfoPanel
//****************************************************************************
TaskInfoPanel::TaskInfoPanel() {
	//addressLastFindManager_ = 0;
	//timeLastUpdate_ = 0;
	//timeUpdateInterval_ = 2000;
}

void TaskInfoPanel::Initialize(const std::string& name) {
	ILoggerPanel::Initialize(name);
}

void TaskInfoPanel::Update() {
	
}
void TaskInfoPanel::ProcessGui() {

}

//****************************************************************************
//WorkRenderTaskManager
//****************************************************************************
WorkRenderTaskManager::WorkRenderTaskManager() {

}
WorkRenderTaskManager::~WorkRenderTaskManager() {

}
void WorkRenderTaskManager::InitializeFunctionDivision(int maxPriWork, int maxPriRender) {
	this->TaskManager::InitializeFunctionDivision(DIV_FUNC_WORK, maxPriWork);
	this->TaskManager::InitializeFunctionDivision(DIV_FUNC_RENDER, maxPriRender);
}

void WorkRenderTaskManager::CallWorkFunction() {
	CallFunction(DIV_FUNC_WORK);
}
void WorkRenderTaskManager::AddWorkFunction(shared_ptr<TaskFunction> func, int pri, int idFunc) {
	//?????????????
	func->SetDelay(1);
	AddFunction(DIV_FUNC_WORK, func, pri, idFunc);
}
void WorkRenderTaskManager::RemoveWorkFunction(TaskBase* task, int idFunc) {
	RemoveFunction(task, DIV_FUNC_WORK, idFunc);
}
void WorkRenderTaskManager::SetWorkFunctionEnable(bool bEnable) {
	SetFunctionEnable(bEnable, DIV_FUNC_WORK);
}
void WorkRenderTaskManager::SetWorkFunctionEnable(bool bEnable, int idTask) {
	SetFunctionEnable(bEnable, idTask, DIV_FUNC_WORK);
}
void WorkRenderTaskManager::SetWorkFunctionEnable(bool bEnable, int idTask, int idFunc) {
	SetFunctionEnable(bEnable, idTask, DIV_FUNC_WORK, idFunc);
}
void WorkRenderTaskManager::SetWorkFunctionEnable(bool bEnable, TaskBase* task) {
	SetFunctionEnable(bEnable, task, DIV_FUNC_WORK);
}
void WorkRenderTaskManager::SetWorkFunctionEnable(bool bEnable, TaskBase* task, int idFunc) {
	SetFunctionEnable(bEnable, task, DIV_FUNC_WORK, idFunc);
}
void WorkRenderTaskManager::SetWorkFunctionEnable(bool bEnable, const std::type_info& info) {
	SetFunctionEnable(bEnable, info, DIV_FUNC_WORK);
}

void WorkRenderTaskManager::CallRenderFunction() {
	CallFunction(DIV_FUNC_RENDER);
}
void WorkRenderTaskManager::AddRenderFunction(shared_ptr<TaskFunction> func, int pri, int idFunc) {
	AddFunction(DIV_FUNC_RENDER, func, pri, idFunc);
}
void WorkRenderTaskManager::RemoveRenderFunction(TaskBase* task, int idFunc) {
	RemoveFunction(task, DIV_FUNC_RENDER, idFunc);
}
void WorkRenderTaskManager::SetRenderFunctionEnable(bool bEnable) {
	SetFunctionEnable(bEnable, DIV_FUNC_RENDER);
}
void WorkRenderTaskManager::SetRenderFunctionEnable(bool bEnable, int idTask) {
	SetFunctionEnable(bEnable, idTask, DIV_FUNC_RENDER);
}
void WorkRenderTaskManager::SetRenderFunctionEnable(bool bEnable, int idTask, int idFunc) {
	SetFunctionEnable(bEnable, idTask, DIV_FUNC_RENDER, idFunc);
}
void WorkRenderTaskManager::SetRenderFunctionEnable(bool bEnable, TaskBase* task) {
	SetFunctionEnable(bEnable, task, DIV_FUNC_RENDER);
}
void WorkRenderTaskManager::SetRenderFunctionEnable(bool bEnable, TaskBase* task, int idFunc) {
	SetFunctionEnable(bEnable, task, DIV_FUNC_RENDER, idFunc);
}
void WorkRenderTaskManager::SetRenderFunctionEnable(bool bEnable, const std::type_info& info) {
	SetFunctionEnable(bEnable, info, DIV_FUNC_RENDER);
}
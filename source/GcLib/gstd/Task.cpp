#include "source/GcLib/pch.h"

#include "Task.hpp"

using namespace gstd;

//****************************************************************************
//TaskFunction
//****************************************************************************
std::wstring TaskFunction::GetInfoAsString() {
	return task_->GetInfoAsString();
}

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
	//Erase dead tasks
	for (auto itrTask = listTask_.begin(); itrTask != listTask_.end();) {
		if (*itrTask == nullptr)
			itrTask = listTask_.erase(itrTask);
		else ++itrTask;
	}

	//Erase finished functions
	for (auto itrDiv = mapFunc_.begin(); itrDiv != mapFunc_.end(); ++itrDiv) {
		for (auto& iListFunc : itrDiv->second) {
			for (auto itrFunc = iListFunc.begin(); itrFunc != iListFunc.end();) {
				if (*itrFunc == nullptr) itrFunc = iListFunc.erase(itrFunc);
				else ++itrFunc;
			}
		}
	}

	if (panelInfo_) panelInfo_->Update(this);
}
void TaskManager::Clear() {
	listTask_.clear();
	mapFunc_.clear();
}
void TaskManager::ClearTask() {
	listTask_.clear();
	for (auto itrDiv = mapFunc_.begin(); itrDiv != mapFunc_.end(); ++itrDiv)
		itrDiv->second.clear();
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
	for (auto itrDiv = mapFunc_.begin(); itrDiv != mapFunc_.end(); ++itrDiv) {
		for (auto& iListFunc : itrDiv->second) {
			for (auto& iFunc : iListFunc) {
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
	for (auto itrDiv = mapFunc_.begin(); itrDiv != mapFunc_.end(); ++itrDiv) {
		for (auto& iListFunc : itrDiv->second) {
			for (auto& iFunc : iListFunc) {
				if (iFunc == nullptr) continue;
				const std::type_info& tInfo = typeid(*(iFunc->task_));
				if (info != tInfo) continue;
				iFunc = nullptr;
			}
		}
	}
}
void TaskManager::SetFunctionEnable(bool bEnable) {
	for (auto itrDiv = mapFunc_.begin(); itrDiv != mapFunc_.end(); ++itrDiv) {
		for (auto& iListFunc : itrDiv->second) {
			for (auto& iFunc : iListFunc) {
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
	addressLastFindManager_ = 0;
	timeLastUpdate_ = 0;
	timeUpdateInterval_ = 2000;
}
bool TaskInfoPanel::_AddedLogger(HWND hTab) {
	Create(hTab);

	gstd::WTreeView::Style styleTreeView;
	styleTreeView.SetStyle(WS_CHILD | WS_VISIBLE |
		TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT);
	styleTreeView.SetStyleEx(WS_EX_CLIENTEDGE);
	wndTreeView_.Create(hWnd_, styleTreeView);

	WTreeView::ItemStyle treeIteSmtyle;
	treeIteSmtyle.SetMask(TVIF_TEXT | TVIF_PARAM);
	wndTreeView_.CreateRootItem(treeIteSmtyle);

	gstd::WListView::Style styleListView;
	styleListView.SetStyle(WS_CHILD | WS_VISIBLE |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER);
	styleListView.SetStyleEx(WS_EX_CLIENTEDGE);
	styleListView.SetListViewStyleEx(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	wndListView_.Create(hWnd_, styleListView);
	wndListView_.AddColumn(64, ROW_FUNC_ADDRESS, L"Address");
	wndListView_.AddColumn(280, ROW_FUNC_CLASS, L"Class");
	wndListView_.AddColumn(32, ROW_FUNC_ID, L"ID");
	wndListView_.AddColumn(52, ROW_FUNC_DIVISION, L"Division");
	wndListView_.AddColumn(48, ROW_FUNC_PRIORITY, L"Priority");
	wndListView_.AddColumn(48, ROW_FUNC_ENABLE, L"Enable");
	wndListView_.AddColumn(256, ROW_FUNC_INFO, L"Info");

	wndSplitter_.Create(hWnd_, WSplitter::TYPE_HORIZONTAL);
	wndSplitter_.SetRatioY(0.25f);

	SetWindowVisible(false);

	return true;
}
void TaskInfoPanel::LocateParts() {
	int wx = GetClientX();
	int wy = GetClientY();
	int wWidth = GetClientWidth();
	int wHeight = GetClientHeight();

	int ySplitter = (int)((float)wHeight * wndSplitter_.GetRatioY());
	int heightSplitter = 6;

	wndSplitter_.SetBounds(wx, ySplitter, wWidth, heightSplitter);
	wndTreeView_.SetBounds(wx, wy, wWidth, ySplitter);
	wndListView_.SetBounds(wx, ySplitter + heightSplitter, wWidth, wHeight - ySplitter - heightSplitter);
}
void TaskInfoPanel::Update(TaskManager* taskManager) {
	if (!IsWindowVisible()) return;

	int time = timeGetTime();
	if (abs(time - timeLastUpdate_) < timeUpdateInterval_) return;
	timeLastUpdate_ = time;

	shared_ptr<WTreeView::Item> itemRoot = wndTreeView_.GetRootItem();
	itemRoot->SetText(taskManager->GetInfoAsString());
	itemRoot->SetParam((LPARAM)taskManager);
	_UpdateTreeView(taskManager, itemRoot);

	int addressManager = 0;
	shared_ptr<WTreeView::Item> itemSelected = wndTreeView_.GetSelectedItem();
	if (itemSelected) {
		addressManager = itemSelected->GetParam();
	}
	_UpdateListView((TaskManager*)addressManager);
}
void TaskInfoPanel::_UpdateTreeView(TaskManager* taskManager, shared_ptr<WTreeView::Item> item) {
	std::set<int> setAddress;
	{
		std::list<shared_ptr<TaskBase>> listTask = taskManager->GetTaskList();
		
		for (auto itrTask = listTask.begin(); itrTask != listTask.end(); ++itrTask) {
			if (*itrTask == nullptr) continue;
			TaskManager* task = dynamic_cast<TaskManager*>(itrTask->get());
			if (task == nullptr) continue;

			int address = (int)task;
			shared_ptr<WTreeView::Item> itemChild = nullptr;
			std::list<shared_ptr<WTreeView::Item>> listChild = item->GetChildList();
			for (auto itrChild = listChild.begin(); itrChild != listChild.end(); ++itrChild) {
				shared_ptr<WTreeView::Item>& iItem = *itrChild;
				LPARAM param = iItem->GetParam();
				if (param != address) continue;
				itemChild = iItem;
			}

			if (itemChild == nullptr) {
				WTreeView::ItemStyle treeIteSmtyle;
				treeIteSmtyle.SetMask(TVIF_TEXT | TVIF_PARAM);
				itemChild = item->CreateChild(treeIteSmtyle);
			}
			itemChild->SetText(task->GetInfoAsString());
			itemChild->SetParam(address);
			_UpdateTreeView(task, itemChild);
			setAddress.insert(address);
		}
	}

	{
		std::list<shared_ptr<WTreeView::Item>> listChild = item->GetChildList();
		for (auto itrChild = listChild.begin(); itrChild != listChild.end(); ++itrChild) {
			shared_ptr<WTreeView::Item>& iItem = *itrChild;
			LPARAM param = iItem->GetParam();
			if (setAddress.find(param) == setAddress.end()) iItem->Delete();
		}
	}
}
void TaskInfoPanel::_UpdateListView(TaskManager* taskManager) {
	if (addressLastFindManager_ != (int)taskManager) {
		wndListView_.Clear();
	}

	if (taskManager == 0) {
		wndListView_.Clear();
		return;
	}

	std::set<std::wstring> setKey;
	TaskManager::function_map mapFunc = taskManager->GetFunctionMap();
	
	for (auto itrType = mapFunc.begin(); itrType != mapFunc.end(); ++itrType) {
		int division = itrType->first;
		int priority = 0;
		std::vector<std::list<shared_ptr<TaskFunction>>>* vectPri = &itrType->second;

		for (auto itrPri = vectPri->begin(); itrPri != vectPri->end(); ++itrPri) {
			std::list<shared_ptr<TaskFunction>>& listFunc = *itrPri;

			for (auto itrFunc = listFunc.begin(); itrFunc != listFunc.end(); ++itrFunc) {
				if (*itrFunc == nullptr) continue;
				std::string keyList;

				TaskFunction* func = itrFunc->get();
				int address = (int)func;
				std::wstring key = StringUtility::Format(L"%08x", address);
				int index = wndListView_.GetIndexInColumn(key, ROW_FUNC_ADDRESS);
				if (index == -1) {
					index = wndListView_.GetRowCount();
					wndListView_.SetText(index, ROW_FUNC_ADDRESS, key);
				}

				std::wstring className = StringUtility::ConvertMultiToWide(typeid(*func).name());
				wndListView_.SetText(index, ROW_FUNC_CLASS, className);
				wndListView_.SetText(index, ROW_FUNC_ID, StringUtility::Format(L"%d", func->GetID()));
				wndListView_.SetText(index, ROW_FUNC_DIVISION, StringUtility::Format(L"%d", division));
				wndListView_.SetText(index, ROW_FUNC_PRIORITY, StringUtility::Format(L"%d", priority));
				wndListView_.SetText(index, ROW_FUNC_ENABLE, func->IsEnable() ? L"Yes" : L"No");
				wndListView_.SetText(index, ROW_FUNC_INFO, func->GetInfoAsString());

				setKey.insert(key);
			}
			++priority;
		}
	}

	for (int iRow = 0; iRow < wndListView_.GetRowCount();) {
		std::wstring key = wndListView_.GetText(iRow, ROW_FUNC_ADDRESS);
		if (setKey.find(key) != setKey.end()) ++iRow;
		else wndListView_.DeleteRow(iRow);
	}

	addressLastFindManager_ = (int)taskManager;
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
#pragma once

#include "../pch.h"

#include "GstdUtility.hpp"
#include "Logger.hpp"

namespace gstd {
	class TaskBase;
	class TaskFunction;
	class TaskManager;

	enum {
		TASK_FREE_ID = 0xffffffff,
		TASK_GROUP_FREE_ID = 0xffffffff,
	};

	//****************************************************************************
	//TaskFunction
	//****************************************************************************
	class TaskFunction : public IStringInfo {
		friend TaskManager;
	protected:
		shared_ptr<TaskBase> task_;
		int id_;
		bool bEnable_;
		int delay_;
	public:
		TaskFunction() : task_(nullptr), id_(TASK_FREE_ID), bEnable_(true), delay_(0) {}
		virtual ~TaskFunction() {}

		virtual void Call() = 0;

		shared_ptr<TaskBase> GetTask() { return task_; }
		int GetID() { return id_; }
		bool IsEnable() { return bEnable_; }

		int GetDelay() { return delay_; }
		void SetDelay(int delay) { delay_ = delay; }
		bool IsDelay() { return delay_ > 0; }

		virtual std::wstring GetInfoAsString();
	};

	template <class T>
	class TTaskFunction : public TaskFunction {
	public:
		typedef void (T::*Function)();
	protected:
		Function pFunc;
	public:
		TTaskFunction(shared_ptr<T> task, Function func) { task_ = task; pFunc = func; }

		virtual void Call() {
			if (task_) ((T*)task_.get()->*pFunc)();
		}

		static shared_ptr<TaskFunction> Create(shared_ptr<TaskBase> task, Function func) {
			shared_ptr<TaskFunction> dTask = std::dynamic_pointer_cast<TaskFunction>(task);
			return shared_ptr<TTaskFunction<T>>::Create(dTask, func);
		}
		static shared_ptr<TaskFunction> Create(shared_ptr<T> task, Function func) {
			return shared_ptr<TTaskFunction<T>>(new TTaskFunction<T>(task, func));
		}
	};

	//****************************************************************************
	//TaskBase
	//****************************************************************************
	class TaskBase : public IStringInfo {
		friend TaskManager;
	protected:
		int64_t indexTask_;
		int idTask_;
		int idTaskGroup_;
	public:
		TaskBase();
		virtual ~TaskBase();

		int GetTaskID() { return idTask_; }
		int64_t GetTaskIndex() { return indexTask_; }
	};

	//****************************************************************************
	//TaskManager
	//****************************************************************************
	class TaskInfoPanel;
	class TaskManager : public TaskBase {
		friend TaskInfoPanel;
	public:
		typedef std::map<int, std::vector<std::list<shared_ptr<TaskFunction>>>> function_map;
	protected:
		static gstd::CriticalSection lockStatic_;

		std::list<shared_ptr<TaskBase>> listTask_;
		function_map mapFunc_;			//Map of <division, vector<priority, functions>>
		int64_t indexTaskManager_;		//Unique ID for created tasks
		shared_ptr<TaskInfoPanel> panelInfo_;

		uint64_t timeSpentLastCall_;

		void _CheckInvalidFunctionDivision(int divFunc);
	public:
		TaskManager();
		virtual ~TaskManager();

		void ArrangeTask();				//Erase finished tasks

		void Clear();
		void ClearTask();

		shared_ptr<TaskBase> GetTask(int idTask);
		shared_ptr<TaskBase> GetTask(const std::type_info& info);
		std::list<shared_ptr<TaskBase>>& GetTaskList() { return listTask_; }

		void AddTask(shared_ptr<TaskBase> task);
		void RemoveTask(TaskBase* task);
		void RemoveTask(int idTask);
		void RemoveTaskGroup(int idGroup);
		void RemoveTask(const std::type_info& info);
		void RemoveTaskWithoutTypeInfo(std::set<const std::type_info*> listInfo);

		void InitializeFunctionDivision(int divFunc, int maxPri);
		void CallFunction(int divFunc);
		void AddFunction(int divFunc, shared_ptr<TaskFunction> func, int pri, int idFunc = TASK_FREE_ID);
		void RemoveFunction(TaskBase* task);
		void RemoveFunction(TaskBase* task, int divFunc, int idFunc);
		void RemoveFunction(const std::type_info& info);
		function_map GetFunctionMap() { return mapFunc_; }

		void SetFunctionEnable(bool bEnable);
		void SetFunctionEnable(bool bEnable, int divFunc);
		void SetFunctionEnable(bool bEnable, int idTask, int divFunc);
		void SetFunctionEnable(bool bEnable, int idTask, int divFunc, int idFunc);
		void SetFunctionEnable(bool bEnable, TaskBase* task, int divFunc);
		void SetFunctionEnable(bool bEnable, TaskBase* task, int divFunc, int idFunc);
		void SetFunctionEnable(bool bEnable, const std::type_info& info, int divFunc);

		void SetInfoPanel(shared_ptr<TaskInfoPanel> panel) { panelInfo_ = panel; }
		gstd::CriticalSection& GetStaticLock() { return lockStatic_; }

		uint32_t GetTimeSpentOnLastFuncCall() { return timeSpentLastCall_; }
	};

	//****************************************************************************
	//TaskInfoPanel
	//****************************************************************************
	class TaskInfoPanel : public WindowLogger::Panel {
	protected:
		enum {
			ROW_FUNC_ADDRESS = 0,
			ROW_FUNC_CLASS,
			ROW_FUNC_ID,
			ROW_FUNC_DIVISION,
			ROW_FUNC_PRIORITY,
			ROW_FUNC_ENABLE,
			ROW_FUNC_INFO,
		};
		WSplitter wndSplitter_;
		WTreeView wndTreeView_;
		WListView wndListView_;

		uint64_t timeLastUpdate_;
		uint64_t timeUpdateInterval_;
		int addressLastFindManager_;

		virtual bool _AddedLogger(HWND hTab);
		void _UpdateTreeView(TaskManager* taskManager, shared_ptr<WTreeView::Item> item);
		void _UpdateListView(TaskManager* taskManager);
	public:
		TaskInfoPanel();

		void SetUpdateInterval(int time) { timeUpdateInterval_ = time; }

		virtual void LocateParts();
		virtual void Update(TaskManager* taskManager);
	};

	//****************************************************************************
	//WorkRenderTaskManager
	//	Manages update and render loops
	//****************************************************************************
	class WorkRenderTaskManager : public TaskManager {
		enum {
			DIV_FUNC_WORK,
			DIV_FUNC_RENDER,
		};
	public:
		WorkRenderTaskManager();
		~WorkRenderTaskManager();

		virtual void InitializeFunctionDivision(int maxPriWork, int maxPriRender);

		void CallWorkFunction();
		void AddWorkFunction(shared_ptr<TaskFunction> func, int pri, int idFunc = TASK_FREE_ID);
		void RemoveWorkFunction(TaskBase* task, int idFunc);
		void SetWorkFunctionEnable(bool bEnable);
		void SetWorkFunctionEnable(bool bEnable, int idTask);
		void SetWorkFunctionEnable(bool bEnable, int idTask, int idFunc);
		void SetWorkFunctionEnable(bool bEnable, TaskBase* task);
		void SetWorkFunctionEnable(bool bEnable, TaskBase* task, int idFunc);
		void SetWorkFunctionEnable(bool bEnable, const std::type_info& info);

		void CallRenderFunction();
		void AddRenderFunction(shared_ptr<TaskFunction> func, int pri, int idFunc = TASK_FREE_ID);
		void RemoveRenderFunction(TaskBase* task, int idFunc);
		void SetRenderFunctionEnable(bool bEnable);
		void SetRenderFunctionEnable(bool bEnable, int idTask);
		void SetRenderFunctionEnable(bool bEnable, int idTask, int idFunc);
		void SetRenderFunctionEnable(bool bEnable, TaskBase* task);
		void SetRenderFunctionEnable(bool bEnable, TaskBase* task, int idFunc);
		void SetRenderFunctionEnable(bool bEnable, const std::type_info& info);
	};
}
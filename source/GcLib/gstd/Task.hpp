#ifndef __GSTD_TASK__
#define __GSTD_TASK__

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
	/**********************************************************
	//TaskFunction
	**********************************************************/
	class TaskFunction : public IStringInfo {
		friend TaskManager;
	protected:
		shared_ptr<TaskBase> task_;	//タスクへのポインタ
		int id_;//id
		bool bEnable_;
		int delay_;
	public:
		TaskFunction() { task_ = nullptr; id_ = TASK_FREE_ID; bEnable_ = true; delay_ = 0; }
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
		typedef void (T::* Function)();
	protected:

		Function pFunc;//メンバ関数ポインタ
	public:
		TTaskFunction(shared_ptr<T> task, Function func) { task_ = task; pFunc = func; }
		virtual void Call() {
			if (task_ != nullptr) ((T*)task_.get()->*pFunc)();
		}

		static shared_ptr<TaskFunction> Create(shared_ptr<TaskBase> task, Function func) {
			shared_ptr<TaskFunction> dTask = std::dynamic_pointer_cast<TaskFunction>(task);
			return shared_ptr<TTaskFunction<T>>::Create(dTask, func);
		}
		static shared_ptr<TaskFunction> Create(shared_ptr<T> task, Function func) {
			return shared_ptr<TTaskFunction<T>>(new TTaskFunction<T>(task, func));
		}
	};

	/**********************************************************
	//TaskBase
	**********************************************************/
	class TaskBase : public IStringInfo {
		friend TaskManager;
	protected:
		int64_t indexTask_;//TaskManagerによってつけられる一意のインデックス
		int idTask_;//ID
		int idTaskGroup_;//グループID
	public:
		TaskBase();
		virtual ~TaskBase();
		int GetTaskID() { return idTask_; }
		int64_t GetTaskIndex() { return indexTask_; }
	};

	/**********************************************************
	//TaskManager
	**********************************************************/
	class TaskInfoPanel;
	class TaskManager : public TaskBase {
		friend TaskInfoPanel;
	public:
		typedef std::map<int, std::vector<std::list<shared_ptr<TaskFunction>>>> function_map;
	protected:
		static gstd::CriticalSection lockStatic_;
		std::list<shared_ptr<TaskBase>> listTask_;//タスクの元クラス
		function_map mapFunc_;//タスク機能のリスト(divFunc, priority, func)
		int64_t indexTaskManager_;//一意のインデックス
		ref_count_ptr<TaskInfoPanel> panelInfo_;

		void _ArrangeTask();//必要のなくなった領域削除
		void _CheckInvalidFunctionDivision(int divFunc);
	public:
		TaskManager();
		virtual ~TaskManager();
		void Clear();//全タスク削除
		void ClearTask();
		void AddTask(shared_ptr<TaskBase> task);//タスクを追加
		shared_ptr<TaskBase> GetTask(int idTask);//指定したIDのタスクを取得
		shared_ptr<TaskBase> GetTask(const std::type_info& info);
		void RemoveTask(TaskBase* task);//指定したタスクを削除
		void RemoveTask(int idTask);//タスク元IDで削除
		void RemoveTaskGroup(int idGroup);//タスクをグループで削除
		void RemoveTask(const std::type_info& info);//クラス型で削除
		void RemoveTaskWithoutTypeInfo(std::set<const std::type_info*> listInfo);//クラス型以外のタスクを削除
		std::list<shared_ptr<TaskBase>> GetTaskList() { return listTask_; }

		void InitializeFunctionDivision(int divFunc, int maxPri);
		void CallFunction(int divFunc);//タスク機能実行
		void AddFunction(int divFunc, shared_ptr<TaskFunction> func, int pri, int idFunc = TASK_FREE_ID);//タスク機能追加
		void RemoveFunction(TaskBase* task);//タスク機能削除
		void RemoveFunction(TaskBase* task, int divFunc, int idFunc);//タスク機能削除
		void RemoveFunction(const std::type_info& info);//タスク機能削除
		function_map GetFunctionMap() { return mapFunc_; }

		void SetFunctionEnable(bool bEnable);//全タスク機能の状態を切り替える
		void SetFunctionEnable(bool bEnable, int divFunc);//タスク機能の状態を切り替える
		void SetFunctionEnable(bool bEnable, int idTask, int divFunc);//タスク機能の状態を切り替える
		void SetFunctionEnable(bool bEnable, int idTask, int divFunc, int idFunc);//タスク機能の状態を切り替える
		void SetFunctionEnable(bool bEnable, TaskBase* task, int divFunc);//タスク機能の状態を切り替える
		void SetFunctionEnable(bool bEnable, TaskBase* task, int divFunc, int idFunc);//タスク機能の状態を切り替える
		void SetFunctionEnable(bool bEnable, const std::type_info& info, int divFunc);//タスク機能の状態を切り替える

		void SetInfoPanel(ref_count_ptr<TaskInfoPanel> panel) { panelInfo_ = panel; }
		gstd::CriticalSection& GetStaticLock() { return lockStatic_; }
	};

	/**********************************************************
	//TaskInfoPanel
	**********************************************************/
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
		int timeLastUpdate_;
		int timeUpdateInterval_;
		int addressLastFindManager_;

		virtual bool _AddedLogger(HWND hTab);
		void _UpdateTreeView(TaskManager* taskManager, ref_count_ptr<WTreeView::Item> item);
		void _UpdateListView(TaskManager* taskManager);
	public:
		TaskInfoPanel();
		void SetUpdateInterval(int time) { timeUpdateInterval_ = time; }
		virtual void LocateParts();
		virtual void Update(TaskManager* taskManager);
	};

	/**********************************************************
	//WorkRenderTaskManager
	//動作、描画機能を保持するTaskManager
	**********************************************************/
	class WorkRenderTaskManager : public TaskManager {
		enum {
			DIV_FUNC_WORK,//動作
			DIV_FUNC_RENDER,//描画
		};
	public:
		WorkRenderTaskManager();
		~WorkRenderTaskManager();
		virtual void InitializeFunctionDivision(int maxPriWork, int maxPriRender);

		//動作機能
		void CallWorkFunction();
		void AddWorkFunction(shared_ptr<TaskFunction> func, int pri, int idFunc = TASK_FREE_ID);
		void RemoveWorkFunction(TaskBase* task, int idFunc);
		void SetWorkFunctionEnable(bool bEnable);
		void SetWorkFunctionEnable(bool bEnable, int idTask);
		void SetWorkFunctionEnable(bool bEnable, int idTask, int idFunc);
		void SetWorkFunctionEnable(bool bEnable, TaskBase* task);
		void SetWorkFunctionEnable(bool bEnable, TaskBase* task, int idFunc);
		void SetWorkFunctionEnable(bool bEnable, const std::type_info& info);

		//描画機能
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

#endif

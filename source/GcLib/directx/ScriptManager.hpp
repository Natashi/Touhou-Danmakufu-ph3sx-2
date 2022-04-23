#ifndef __DIRECTX_SCRIPTMANAGER__
#define __DIRECTX_SCRIPTMANAGER__

#include "../pch.h"
#include "DxScript.hpp"

namespace directx {
	class ManagedScript;
	//*******************************************************************
	//ScriptManager
	//*******************************************************************
	class ScriptManager : public gstd::FileManager::LoadThreadListener {
	public:
		enum {
			MAX_CLOSED_SCRIPT_RESULT = 100,
			ID_INVALID = -1,
		};
	protected:
		static std::atomic<int64_t> idScript_;

		gstd::CriticalSection lock_;

		std::atomic_bool bCancelLoad_;
		std::atomic_int nActiveScriptLoad_;
		
		bool bHasCloseScriptWork_;

		std::wstring error_;
		std::map<int64_t, shared_ptr<ManagedScript>> mapScriptLoad_;
		std::list<shared_ptr<ManagedScript>> listScriptRun_;
		std::map<int64_t, gstd::value> mapClosedScriptResult_;
		std::list<weak_ptr<ScriptManager>> listRelativeManager_;

		int mainThreadID_;

		int64_t _LoadScript(const std::wstring& path, shared_ptr<ManagedScript> script);
	public:
		ScriptManager();
		virtual ~ScriptManager();

		virtual void CancelLoad() { bCancelLoad_ = true; }
		virtual bool CancelLoadComplete() { return nActiveScriptLoad_ <= 0; }

		virtual void Work();
		virtual void Work(int targetType);
		virtual void Render();

		virtual void SetError(const std::wstring& error) { error_ = error; }
		virtual bool IsError() { return error_.size() > 0; }
		void TryThrowError() {
			if (IsError()) throw gstd::wexception(error_);
		}

		int GetMainThreadID() { return mainThreadID_; }
		int64_t IssueScriptID() { return ++idScript_; }

		std::map<int64_t, shared_ptr<ManagedScript>>& GetMapScriptLoad() { return mapScriptLoad_; }
		std::list<shared_ptr<ManagedScript>>& GetRunningScriptList() { return listScriptRun_; }
		std::list<weak_ptr<ScriptManager>>& GetRelativeManagerList() { return listRelativeManager_; }

		shared_ptr<ManagedScript> GetScript(int64_t id, bool bSearchInLoad, bool bSearchRelative);
		void StartScript(int64_t id, bool bUnload = true);
		void StartScript(shared_ptr<ManagedScript> id, bool bUnload = true);
		void CloseScript(int64_t id);
		void CloseScript(shared_ptr<ManagedScript> id);
		void CloseScriptOnType(int type);
		bool IsCloseScript(int64_t id);
		bool HasCloseScriptWork() { return bHasCloseScriptWork_; }
		size_t GetAllScriptThreadCount();
		void TerminateScriptAll(const std::wstring& message);

		void OrphanAllScripts();

		int64_t LoadScript(const std::wstring& path, shared_ptr<ManagedScript> script);
		shared_ptr<ManagedScript> LoadScript(const std::wstring& path, int type);
		int64_t LoadScriptInThread(const std::wstring& path, shared_ptr<ManagedScript> script);
		shared_ptr<ManagedScript> LoadScriptInThread(const std::wstring& path, int type);
		virtual void CallFromLoadThread(shared_ptr<gstd::FileManager::LoadThreadEvent> event);

		void UnloadScript(int64_t id);
		void UnloadScript(shared_ptr<ManagedScript> script);

		virtual shared_ptr<ManagedScript> Create(int type) = 0;

		virtual void RequestEventAll(int type, const gstd::value* listValue = nullptr, size_t countArgument = 0);

		gstd::value GetScriptResult(int64_t idScript);

		void AddRelativeScriptManager(weak_ptr<ScriptManager> manager) { listRelativeManager_.push_back(manager); }
		static void AddRelativeScriptManagerMutual(weak_ptr<ScriptManager> manager1, weak_ptr<ScriptManager> manager2);
	};


	//*******************************************************************
	//ManagedScript
	//*******************************************************************
	class ManagedScriptParameter {
	public:
		ManagedScriptParameter() {}
		virtual ~ManagedScriptParameter() {}
	};

	class ManagedScript : public DxScript, public gstd::FileManager::LoadObject {
		friend ScriptManager;
	public:
		enum {
			TYPE_ALL = -1,

			STATUS_INVALID = 0,
			STATUS_LOADING,
			STATUS_LOADED,
			STATUS_RUNNING,
			STATUS_PAUSED,
			STATUS_CLOSING,
		};
	protected:
		ScriptManager* scriptManager_;

		std::atomic_bool bBeginLoad_;
		std::atomic_bool bLoad_;

		int typeScript_;
		shared_ptr<ManagedScriptParameter> scriptParam_;
		
		bool bEndScript_;
		bool bAutoDeleteObject_;
		std::atomic_bool bRunning_;
		std::atomic_bool bPaused_;

		uint64_t runTime_;

		int typeEvent_;
		gstd::value* listValueEvent_;
		size_t listValueEventSize_;
	public:
		ManagedScript();
		~ManagedScript();

		virtual void Reset();

		virtual void SetScriptManager(ScriptManager* manager);
		virtual void SetScriptParameter(shared_ptr<ManagedScriptParameter> param) { scriptParam_ = param; }
		shared_ptr<ManagedScriptParameter> GetScriptParameter() { return scriptParam_; }

		ScriptManager* GetScriptManager() { return scriptManager_; }

		bool IsBeginLoad() { return bBeginLoad_; }
		bool IsLoad() { return bLoad_; }

		int GetScriptType() { return typeScript_; }
		bool IsEndScript() { return bEndScript_; }
		void SetEndScript() { bEndScript_ = true; }
		bool IsAutoDeleteObject() { return bAutoDeleteObject_; }
		void SetAutoDeleteObject(bool bEnable) { bAutoDeleteObject_ = bEnable; }
		bool IsRunning() { return bRunning_; }
		bool IsPaused() { return bPaused_; }

		uint64_t GetScriptRunTime() { return runTime_; }

		gstd::value RequestEvent(int type);
		gstd::value RequestEvent(int type, const gstd::value* listValue, size_t countArgument);

		//制御共通関数：共通データ
		static gstd::value Func_SaveCommonDataAreaA1(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_LoadCommonDataAreaA1(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//制御共通関数：スクリプト操作
		static gstd::value Func_LoadScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_LoadScriptInThread(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_UnloadScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_UnloadScriptFromCache(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_StartScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_CloseScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_IsCloseScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetOwnScriptID(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetEventType(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetEventArgument(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_GetEventArgumentCount);
		static gstd::value Func_SetScriptArgument(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetScriptResult(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetAutoDeleteObject(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetAllObjectIdInScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetAllObjectIdInPool(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_NotifyEvent(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_NotifyEventOwn);
		static gstd::value Func_NotifyEventAll(gstd::script_machine* machine, int argc, const gstd::value* argv);
		DNH_FUNCAPI_DECL_(Func_PauseScript);

		DNH_FUNCAPI_DECL_(Func_GetScriptStatus);
	};
}

#endif

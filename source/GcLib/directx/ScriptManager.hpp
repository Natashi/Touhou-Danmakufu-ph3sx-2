#ifndef __DIRECTX_SCRIPTMANAGER__
#define __DIRECTX_SCRIPTMANAGER__

#include "../pch.h"
#include "DxScript.hpp"

namespace directx {
	class ManagedScript;
	/**********************************************************
	//ScriptManager
	**********************************************************/
	class ScriptManager : public gstd::FileManager::LoadThreadListener {
	public:
		enum {
			MAX_CLOSED_SCRIPT_RESULT = 100,
			ID_INVALID = -1,
		};

	protected:
		gstd::CriticalSection lock_;
		static int64_t idScript_;
		bool bHasCloseScriptWork_;

		std::wstring error_;
		std::map<int64_t, shared_ptr<ManagedScript>> mapScriptLoad_;
		std::list<shared_ptr<ManagedScript>> listScriptRun_;
		std::map<int64_t, gstd::value> mapClosedScriptResult_;
		std::list<std::weak_ptr<ScriptManager>> listRelativeManager_;

		int mainThreadID_;

		int64_t _LoadScript(std::wstring path, shared_ptr<ManagedScript> script);
	public:
		ScriptManager();
		virtual ~ScriptManager();
		virtual void Work();
		virtual void Work(int targetType);
		virtual void Render();

		virtual void SetError(std::wstring error) { error_ = error; }
		virtual bool IsError() { return error_ != L""; }

		int GetMainThreadID() { return mainThreadID_; }
		int64_t IssueScriptID() { { gstd::Lock lock(lock_); idScript_++; return idScript_; } }

		shared_ptr<ManagedScript> GetScript(int64_t id, bool bSearchRelative = false);
		void StartScript(int64_t id);
		void StartScript(shared_ptr<ManagedScript> id);
		void CloseScript(int64_t id);
		void CloseScript(shared_ptr<ManagedScript> id);
		void CloseScriptOnType(int type);
		bool IsCloseScript(int64_t id);
		int IsHasCloseScliptWork() { return bHasCloseScriptWork_; }
		size_t GetAllScriptThreadCount();
		void TerminateScriptAll(std::wstring message);

		int64_t LoadScript(std::wstring path, shared_ptr<ManagedScript> script);
		shared_ptr<ManagedScript> LoadScript(std::wstring path, int type);
		int64_t LoadScriptInThread(std::wstring path, shared_ptr<ManagedScript> script);
		shared_ptr<ManagedScript> LoadScriptInThread(std::wstring path, int type);
		virtual void CallFromLoadThread(shared_ptr<gstd::FileManager::LoadThreadEvent> event);

		virtual shared_ptr<ManagedScript> Create(int type) = 0;
		virtual void RequestEventAll(int type, const gstd::value* listValue = nullptr, size_t countArgument = 0);
		gstd::value GetScriptResult(int64_t idScript);
		void AddRelativeScriptManager(std::weak_ptr<ScriptManager> manager) { listRelativeManager_.push_back(manager); }
		static void AddRelativeScriptManagerMutual(std::weak_ptr<ScriptManager> manager1, 
			std::weak_ptr<ScriptManager> manager2);
	};


	/**********************************************************
	//ManagedScript
	**********************************************************/
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
		};

	protected:
		ScriptManager* scriptManager_;

		int typeScript_;
		shared_ptr<ManagedScriptParameter> scriptParam_;
		volatile bool bLoad_;
		bool bEndScript_;
		bool bAutoDeleteObject_;
		bool bPaused_;

		int typeEvent_;
		gstd::value* listValueEvent_;
		size_t listValueEventSize_;
	public:
		ManagedScript();
		~ManagedScript();

		virtual void SetScriptManager(ScriptManager* manager);
		virtual void SetScriptParameter(shared_ptr<ManagedScriptParameter> param) { scriptParam_ = param; }
		shared_ptr<ManagedScriptParameter> GetScriptParameter() { return scriptParam_; }

		int GetScriptType() { return typeScript_; }
		bool IsLoad() { return bLoad_; }
		bool IsEndScript() { return bEndScript_; }
		void SetEndScript() { bEndScript_ = true; }
		bool IsAutoDeleteObject() { return bAutoDeleteObject_; }
		void SetAutoDeleteObject(bool bEneble) { bAutoDeleteObject_ = bEneble; }
		bool IsPaused() { return bPaused_; }

		gstd::value RequestEvent(int type);
		gstd::value RequestEvent(int type, const gstd::value* listValue, size_t countArgument);

		//制御共通関数：共通データ
		static gstd::value Func_SaveCommonDataAreaA1(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_LoadCommonDataAreaA1(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//制御共通関数：スクリプト操作
		static gstd::value Func_LoadScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_LoadScriptInThread(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_StartScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_CloseScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_IsCloseScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetOwnScriptID(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetEventType(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetEventArgument(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetScriptArgument(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetScriptResult(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetAutoDeleteObject(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_NotifyEvent(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_NotifyEventOwn(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_NotifyEventAll(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_PauseScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
	};


}

#endif

#pragma once

#include "../pch.h"

#include "GstdUtility.hpp"
#include "Script.hpp"
#include "RandProvider.hpp"
#include "Thread.hpp"
#include "File.hpp"
#include "Logger.hpp"

namespace gstd {
	class ScriptFileLineMap;
	class ScriptCommonDataManager;
	/**********************************************************
	//ScriptException
	**********************************************************/
	class ScriptException : public gstd::wexception {
	public:
		ScriptException() : gstd::wexception(L"") {};
		ScriptException(std::wstring str) : gstd::wexception(str.c_str()) {}
	};

	/**********************************************************
	//ScriptEngineData
	**********************************************************/
	class ScriptEngineData {
	protected:
		std::wstring path_;
		int encoding_;
		std::vector<char> source_;
		gstd::ref_count_ptr<script_engine> engine_;
		gstd::ref_count_ptr<ScriptFileLineMap> mapLine_;
	public:
		ScriptEngineData();
		virtual ~ScriptEngineData();

		void SetPath(std::wstring path) { path_ = path; }
		std::wstring GetPath() { return path_; }
		void SetSource(std::vector<char>& source);
		std::vector<char>& GetSource() { return source_; }
		int GetEncoding() { return encoding_; }
		void SetEngine(gstd::ref_count_ptr<script_engine> engine) { engine_ = engine; }
		gstd::ref_count_ptr<script_engine> GetEngine() { return engine_; }
		gstd::ref_count_ptr<ScriptFileLineMap> GetScriptFileLineMap() { return mapLine_; }
		void SetScriptFileLineMap(gstd::ref_count_ptr<ScriptFileLineMap> mapLine) { mapLine_ = mapLine; }
	};

	/**********************************************************
	//ScriptEngineCache
	**********************************************************/
	class ScriptEngineCache {
	protected:
		std::map<std::wstring, ref_count_ptr<ScriptEngineData> > cache_;
	public:
		ScriptEngineCache();
		virtual ~ScriptEngineCache();
		void Clear();

		void AddCache(std::wstring name, ref_count_ptr<ScriptEngineData> data);
		ref_count_ptr<ScriptEngineData> GetCache(std::wstring name);
		bool IsExists(std::wstring name);
	};

	/**********************************************************
	//ScriptBase
	**********************************************************/
	class ScriptClientBase {
	public:
		enum {
			ID_SCRIPT_FREE = -1,
		};
	protected:
		script_type_manager* pTypeManager_;

		bool bError_;
		gstd::ref_count_ptr<ScriptEngineCache> cache_;
		gstd::ref_count_ptr<ScriptEngineData> engine_;
		script_machine* machine_;

		std::vector<function> func_;
		shared_ptr<RandProvider> mt_;
		shared_ptr<RandProvider> mtEffect_;
		gstd::ref_count_ptr<ScriptCommonDataManager> commonDataManager_;
		int mainThreadID_;
		int64_t idScript_;

		gstd::CriticalSection criticalSection_;

		std::vector<gstd::value> listValueArg_;
		gstd::value valueRes_;

		void _AddFunction(const char* name, callback f, size_t arguments);
		void _AddFunction(const function* f, size_t count);
		void _RaiseErrorFromEngine();
		void _RaiseErrorFromMachine();
		void _RaiseError(int line, std::wstring message);
		std::wstring _GetErrorLineSource(int line);
		virtual std::vector<char> _Include(std::vector<char>& source);
		virtual bool _CreateEngine();
		std::wstring _ExtendPath(std::wstring path);

	public:
		ScriptClientBase();
		virtual ~ScriptClientBase();
		void SetScriptEngineCache(gstd::ref_count_ptr<ScriptEngineCache> cache) { cache_ = cache; }
		gstd::ref_count_ptr<ScriptEngineData> GetEngine() { return engine_; }
		virtual bool SetSourceFromFile(std::wstring path);
		virtual void SetSource(std::vector<char>& source);
		virtual void SetSource(std::string source);

		script_type_manager* GetDefaultScriptTypeManager() { return pTypeManager_; }

		std::wstring GetPath() { return engine_->GetPath(); }
		void SetPath(std::wstring path) { engine_->SetPath(path); }

		virtual void Compile();
		virtual bool Run();
		virtual bool Run(std::string target);
		virtual bool Run(std::map<std::string, script_engine::block*>::iterator target);
		bool IsEventExists(std::string name, std::map<std::string, script_engine::block*>::iterator& res);
		void RaiseError(std::wstring error) { _RaiseError(machine_->get_error_line(), error); }
		void RaiseError(std::string error) { 
			_RaiseError(machine_->get_error_line(), 
				StringUtility::ConvertMultiToWide(error));
		}
		void Terminate(std::wstring error) { machine_->terminate(error); }
		int64_t GetScriptID() { return idScript_; }
		size_t GetThreadCount();

		void AddArgumentValue(value v) { listValueArg_.push_back(v); }
		void SetArgumentValue(value v, int index = 0);
		value GetResultValue() { return valueRes_; }

		value CreateRealValue(double r);
		value CreateBooleanValue(bool b);
		value CreateStringValue(std::string s);
		value CreateStringValue(std::wstring s);
		value CreateRealArrayValue(std::vector<float>& list);
		value CreateRealArrayValue(std::vector<double>& list);
		value CreateStringArrayValue(std::vector<std::string>& list);
		value CreateStringArrayValue(std::vector<std::wstring>& list);
		value CreateValueArrayValue(std::vector<value>& list);
		bool IsRealValue(value& v);
		bool IsBooleanValue(value& v);
		bool IsStringValue(value& v);
		bool IsRealArrayValue(value& v);

		static void IsMatrix(script_machine*& machine, const value& v);
		static void IsVector(script_machine*& machine, const value& v, size_t count);

		void CheckRunInMainThread();
		ScriptCommonDataManager* GetCommonDataManager() { return commonDataManager_.GetPointer(); }

		//共通関数：スクリプト引数結果
		static value Func_GetScriptArgument(script_machine* machine, int argc, const value* argv);
		static value Func_GetScriptArgumentCount(script_machine* machine, int argc, const value* argv);
		static value Func_SetScriptResult(script_machine* machine, int argc, const value* argv);

		//共通関数：数学系
		static value Func_Min(script_machine* machine, int argc, const value* argv);
		static value Func_Max(script_machine* machine, int argc, const value* argv);
		static value Func_Log(script_machine* machine, int argc, const value* argv);
		static value Func_Log10(script_machine* machine, int argc, const value* argv);

		static value Func_Cos(script_machine* machine, int argc, const value* argv);
		static value Func_Sin(script_machine* machine, int argc, const value* argv);
		static value Func_Tan(script_machine* machine, int argc, const value* argv);
		static value Func_SinCos(script_machine* machine, int argc, const value* argv);
		static value Func_RCos(script_machine* machine, int argc, const value* argv);
		static value Func_RSin(script_machine* machine, int argc, const value* argv);
		static value Func_RTan(script_machine* machine, int argc, const value* argv);
		static value Func_RSinCos(script_machine* machine, int argc, const value* argv);

		static value Func_Acos(script_machine* machine, int argc, const value* argv);
		static value Func_Asin(script_machine* machine, int argc, const value* argv);
		static value Func_Atan(script_machine* machine, int argc, const value* argv);
		static value Func_Atan2(script_machine* machine, int argc, const value* argv);
		static value Func_RAcos(script_machine* machine, int argc, const value* argv);
		static value Func_RAsin(script_machine* machine, int argc, const value* argv);
		static value Func_RAtan(script_machine* machine, int argc, const value* argv);
		static value Func_RAtan2(script_machine* machine, int argc, const value* argv);

		static value Func_Exp(script_machine* machine, int argc, const value* argv);

		static value Func_Rand(script_machine* machine, int argc, const value* argv);
		static value Func_RandEff(script_machine* machine, int argc, const value* argv);
		static value Func_Sqrt(script_machine* machine, int argc, const value* argv);

		static value Func_ToDegrees(script_machine* machine, int argc, const value* argv);
		static value Func_ToRadians(script_machine* machine, int argc, const value* argv);
		static value Func_NormalizeAngle(script_machine* machine, int argc, const value* argv);
		static value Func_RNormalizeAngle(script_machine* machine, int argc, const value* argv);

		static value Func_Interpolate_Linear(script_machine* machine, int argc, const value* argv);
		static value Func_Interpolate_Smooth(script_machine* machine, int argc, const value* argv);
		static value Func_Interpolate_Smoother(script_machine* machine, int argc, const value* argv);
		static value Func_Interpolate_Accelerate(script_machine* machine, int argc, const value* argv);
		static value Func_Interpolate_Decelerate(script_machine* machine, int argc, const value* argv);
		static value Func_Interpolate_Modulate(script_machine* machine, int argc, const value* argv);
		static value Func_Interpolate_Overshoot(script_machine* machine, int argc, const value* argv);
		static value Func_Interpolate_QuadraticBezier(script_machine* machine, int argc, const value* argv);
		static value Func_Interpolate_CubicBezier(script_machine* machine, int argc, const value* argv);

		static value Func_TypeOf(script_machine* machine, int argc, const value* argv);
		static value Func_FTypeOf(script_machine* machine, int argc, const value* argv);

		//共通関数：文字列操作
		static value Func_ToString(script_machine* machine, int argc, const value* argv);
		static value Func_ItoA(script_machine* machine, int argc, const value* argv);
		static value Func_RtoA(script_machine* machine, int argc, const value* argv);
		static value Func_RtoS(script_machine* machine, int argc, const value* argv);
		static value Func_VtoS(script_machine* machine, int argc, const value* argv);
		static value Func_AtoI(script_machine* machine, int argc, const value* argv);
		static value Func_AtoR(script_machine* machine, int argc, const value* argv);
		static value Func_TrimString(script_machine* machine, int argc, const value* argv);
		static value Func_SplitString(script_machine* machine, int argc, const value* argv);

		static value Func_RegexMatch(script_machine* machine, int argc, const value* argv);
		static value Func_RegexMatchRepeated(script_machine* machine, int argc, const value* argv);
		static value Func_RegexReplace(script_machine* machine, int argc, const value* argv);

		//共通関数：パス関連
		static value Func_GetModuleDirectory(script_machine* machine, int argc, const value* argv);
		static value Func_GetParentScriptDirectory(script_machine* machine, int argc, const value* argv);
		static value Func_GetCurrentScriptDirectory(script_machine* machine, int argc, const value* argv);
		static value Func_GetFileDirectory(script_machine* machine, int argc, const value* argv);
		static value Func_GetFilePathList(script_machine* machine, int argc, const value* argv);
		static value Func_GetDirectoryList(script_machine* machine, int argc, const value* argv);

		//共通関数：時刻関連
		static value Func_GetCurrentDateTimeS(script_machine* machine, int argc, const value* argv);

		//共通関数：デバッグ関連
		static value Func_WriteLog(script_machine* machine, int argc, const value* argv);
		static value Func_RaiseError(script_machine* machine, int argc, const value* argv);

		//共通関数：共通データ
		static value Func_SetCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_GetCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_ClearCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_DeleteCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_SetAreaCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_GetAreaCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_ClearAreaCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_DeleteAreaCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_DeleteWholeAreaCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_CreateCommonDataArea(script_machine* machine, int argc, const value* argv);
		static value Func_CopyCommonDataArea(script_machine* machine, int argc, const value* argv);
		static value Func_IsCommonDataAreaExists(script_machine* machine, int argc, const value* argv);
		static value Func_GetCommonDataAreaKeyList(script_machine* machine, int argc, const value* argv);
		static value Func_GetCommonDataValueKeyList(script_machine* machine, int argc, const value* argv);
	};

	/**********************************************************
	//ScriptFileLineMap
	**********************************************************/
	class ScriptFileLineMap {
	public:
		struct Entry {
			int lineStart_;
			int lineEnd_;
			int lineStartOriginal_;
			int lineEndOriginal_;
			std::wstring path_;
		};
	protected:
		std::list<Entry> listEntry_;
	public:
		ScriptFileLineMap();
		virtual ~ScriptFileLineMap();
		void AddEntry(std::wstring path, int lineAdd, int lineCount);
		Entry GetEntry(int line);
		std::wstring GetPath(int line);
		std::list<Entry> GetEntryList() { return listEntry_; }
	};

	/**********************************************************
	//ScriptCommonData
	**********************************************************/
	class ScriptCommonData {
	protected:
		std::map<std::string, gstd::value> mapValue_;

		gstd::value _ReadRecord(gstd::ByteBuffer& buffer);
		void _WriteRecord(gstd::ByteBuffer& buffer, gstd::value& comValue);
	public:
		ScriptCommonData();
		virtual ~ScriptCommonData();

		void Clear();
		std::pair<bool, std::map<std::string, gstd::value>::iterator> IsExists(std::string name);

		gstd::value GetValue(std::string name);
		gstd::value GetValue(std::map<std::string, gstd::value>::iterator itr);
		void SetValue(std::string name, gstd::value v);
		void SetValue(std::map<std::string, gstd::value>::iterator itr, gstd::value v);
		void DeleteValue(std::string name);
		void Copy(shared_ptr<ScriptCommonData>& dataSrc);

		std::map<std::string, gstd::value>::iterator MapBegin() { return mapValue_.begin(); }
		std::map<std::string, gstd::value>::iterator MapEnd() { return mapValue_.end(); }

		void ReadRecord(gstd::RecordBuffer& record);
		void WriteRecord(gstd::RecordBuffer& record);
	};

	/**********************************************************
	//ScriptCommonDataManager
	**********************************************************/
	class ScriptCommonDataManager {
	public:
		using CommonDataMap = std::map<std::string, shared_ptr<ScriptCommonData>>;
	protected:
		gstd::CriticalSection lock_;
		CommonDataMap mapData_;
		CommonDataMap::iterator defaultAreaIterator_;
	public:
		static const std::string nameAreaDefault_;

		ScriptCommonDataManager();
		virtual ~ScriptCommonDataManager();

		void Clear();
		void Erase(std::string name);

		std::string GetDefaultAreaName() { return nameAreaDefault_; }
		CommonDataMap::iterator GetDefaultAreaIterator() { return defaultAreaIterator_; }

		std::pair<bool, CommonDataMap::iterator> IsExists(std::string name);
		CommonDataMap::iterator CreateArea(std::string name);
		void CopyArea(std::string nameDest, std::string nameSrc);
		shared_ptr<ScriptCommonData> GetData(std::string name);
		shared_ptr<ScriptCommonData> GetData(CommonDataMap::iterator itr);
		void SetData(std::string name, shared_ptr<ScriptCommonData> commonData);
		void SetData(CommonDataMap::iterator itr, shared_ptr<ScriptCommonData> commonData);

		CommonDataMap::iterator MapBegin() { return mapData_.begin(); }
		CommonDataMap::iterator MapEnd() { return mapData_.end(); }

		gstd::CriticalSection& GetLock() { return lock_; }
	};

	/**********************************************************
	//ScriptCommonDataInfoPanel
	**********************************************************/
	class ScriptCommonDataInfoPanel : public WindowLogger::Panel {
	protected:
		enum {
			COL_AREA = 0,
			COL_KEY = 0,
			COL_VALUE,
		};

		gstd::ref_count_ptr<ScriptCommonDataManager> commonDataManager_;
		std::vector<std::map<std::string, shared_ptr<ScriptCommonData>>::iterator> vecMapItr_;

		gstd::CriticalSection lock_;

		WSplitter wndSplitter_;
		WListView wndListViewArea_;
		WListView wndListViewValue_;
		int timeLastUpdate_;
		int timeUpdateInterval_;

		virtual bool _AddedLogger(HWND hTab);

		void _UpdateAreaView();
		void _UpdateValueView();
	public:
		ScriptCommonDataInfoPanel();
		void SetUpdateInterval(int time) { timeUpdateInterval_ = time; }
		virtual void LocateParts();
		virtual void Update(gstd::ref_count_ptr<ScriptCommonDataManager> commonDataManager);
	};

}

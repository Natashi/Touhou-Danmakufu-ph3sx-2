#pragma once

#include "../pch.h"

#include "GstdUtility.hpp"
#include "Script/Script.hpp"
#include "RandProvider.hpp"
#include "Thread.hpp"
#include "File.hpp"
#include "Logger.hpp"

namespace gstd {
	//*******************************************************************
	//ScriptFileLineMap
	//*******************************************************************
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

		void AddEntry(const std::wstring& path, int lineAdd, int lineCount);
		Entry* GetEntry(int line);
		std::wstring& GetPath(int line);
		std::list<Entry>& GetEntryList() { return listEntry_; }

		void Clear() { listEntry_.clear(); }
	};

	//*******************************************************************
	//ScriptEngineData
	//*******************************************************************
	class ScriptEngineData {
	protected:
		std::wstring path_;

		Encoding::Type encoding_;
		std::vector<char> source_;

		unique_ptr<script_engine> engine_;
		ScriptFileLineMap mapLine_;
	public:
		ScriptEngineData();
		virtual ~ScriptEngineData();

		void SetPath(const std::wstring& path) { path_ = path; }
		std::wstring& GetPath() { return path_; }

		void SetSource(std::vector<char>& source);
		std::vector<char>& GetSource() { return source_; }
		Encoding::Type GetEncoding() { return encoding_; }

		void SetEngine(unique_ptr<script_engine>&& engine) { engine_ = std::move(engine); }
		unique_ptr<script_engine>& GetEngine() { return engine_; }

		ScriptFileLineMap* GetScriptFileLineMap() { return &mapLine_; }
	};

	//*******************************************************************
	//ScriptEngineCache
	//*******************************************************************
	class ScriptEngineCache {
	protected:
		std::map<std::wstring, shared_ptr<ScriptEngineData>> cache_;
	public:
		ScriptEngineCache();

		void Clear();

		void AddCache(const std::wstring& name, shared_ptr<ScriptEngineData> data);
		void RemoveCache(const std::wstring& name);
		shared_ptr<ScriptEngineData> GetCache(const std::wstring& name);

		const std::map<std::wstring, shared_ptr<ScriptEngineData>>& GetMap() { return cache_; }

		bool IsExists(const std::wstring& name);
	};

	//*******************************************************************
	//ScriptClientBase
	//*******************************************************************
	class ScriptLoader;
	class ScriptClientBase {
		friend ScriptLoader;
		static unique_ptr<script_type_manager> pTypeManager_;
	public:
		enum {
			ID_SCRIPT_FREE = -1,
		};
		static uint64_t randCalls_;
		static uint64_t prandCalls_;
	protected:
		bool bError_;

		ScriptEngineCache* cache_;

		shared_ptr<ScriptEngineData> engineData_;
		unique_ptr<script_machine> machine_;

		std::vector<gstd::function> func_;
		std::vector<gstd::constant> const_;
		std::map<std::wstring, std::wstring> definedMacro_;

		shared_ptr<RandProvider> mt_;
		shared_ptr<RandProvider> mtEffect_;

		gstd::CriticalSection criticalSection_;

		int mainThreadID_;
		int64_t idScript_;

		std::vector<gstd::value> listValueArg_;
		gstd::value valueRes_;
	protected:
		void _AddFunction(const char* name, dnh_func_callback_t f, size_t arguments);
		void _AddFunction(const std::vector<gstd::function>* f);
		void _AddConstant(const std::vector<gstd::constant>* c);

		void _RaiseErrorFromEngine();
		void _RaiseErrorFromMachine();
		void _RaiseError(int line, const std::wstring& message);
		std::wstring _GetErrorLineSource(int line);

		virtual std::vector<char> _ParseScriptSource(std::vector<char>& source);
		virtual bool _CreateEngine();

		std::wstring _ExtendPath(std::wstring path);
	public:
		ScriptClientBase();
		virtual ~ScriptClientBase();

		static script_type_manager* GetDefaultScriptTypeManager() { return pTypeManager_.get(); }

		void SetScriptEngineCache(ScriptEngineCache* cache) { cache_ = cache; }
		ScriptEngineCache* GetScriptEngineCache() { return cache_; }

		shared_ptr<ScriptEngineData> GetEngineData() { return engineData_; }

		shared_ptr<RandProvider> GetRand() { return mt_; }

		virtual bool SetSourceFromFile(std::wstring path);
		virtual void SetSource(std::vector<char>& source);
		virtual void SetSource(const std::string& source);

		std::wstring& GetPath() { return engineData_->GetPath(); }
		void SetPath(const std::wstring& path) { engineData_->SetPath(path); }

		virtual void Compile();
		virtual void Reset();
		virtual bool Run();
		virtual bool Run(const std::string& target);
		virtual bool Run(std::map<std::string, script_block*>::iterator target);
		bool IsEventExists(const std::string& name, std::map<std::string, script_block*>::iterator& res);
		void RaiseError(const std::wstring& error) { _RaiseError(machine_->get_error_line(), error); }
		void RaiseError(const std::string& error) {
			_RaiseError(machine_->get_error_line(), 
				StringUtility::ConvertMultiToWide(error));
		}
		void Terminate(const std::wstring& error) { machine_->terminate(error); }
		int64_t GetScriptID() { return idScript_; }
		size_t GetThreadCount();

		void AddArgumentValue(value v) { listValueArg_.push_back(v); }
		void SetArgumentValue(value v, int index = 0);
		value GetResultValue() { return valueRes_; }

		static inline value CreateFloatValue(double r);
		static inline value CreateIntValue(int64_t r);
		static inline value CreateBooleanValue(bool b);
		static inline value CreateStringValue(const std::wstring& s);
		static inline value CreateStringValue(const std::string& s);
		template<typename T> static inline value CreateFloatArrayValue(const std::vector<T>& list);
		template<typename T> static value CreateFloatArrayValue(const T* ptrList, size_t count);
		template<size_t N> static inline value CreateFloatArrayValue(const Math::DVec<N>& arr);
		template<typename T> static inline value CreateIntArrayValue(const std::vector<T>& list);
		template<typename T> static value CreateIntArrayValue(const T* ptrList, size_t count);
		static value CreateStringArrayValue(const std::vector<std::string>& list);
		static value CreateStringArrayValue(const std::vector<std::wstring>& list);
		value CreateValueArrayValue(const std::vector<value>& list);

		static bool IsFloatValue(value& v);
		static bool IsIntValue(value& v);
		static bool IsBooleanValue(value& v);
		static bool IsStringValue(value& v);
		static bool IsArrayValue(value& v);
		static bool IsArrayValue(value& v, type_data* element);
		static bool IsFloatArrayValue(value& v);
		static bool IsIntArrayValue(value& v);

		void CheckRunInMainThread();

		//-------------------------------------------------------------------------

		//Script functions
		static value Func_GetScriptArgument(script_machine* machine, int argc, const value* argv);
		static value Func_GetScriptArgumentCount(script_machine* machine, int argc, const value* argv);
		static value Func_SetScriptResult(script_machine* machine, int argc, const value* argv);

		//Floating point functions
		DNH_FUNCAPI_DECL_(Float_Classify);
		DNH_FUNCAPI_DECL_(Float_IsNan);
		DNH_FUNCAPI_DECL_(Float_IsInf);
		DNH_FUNCAPI_DECL_(Float_GetSign);
		DNH_FUNCAPI_DECL_(Float_CopySign);

		//Maths functions
		static value Func_Min(script_machine* machine, int argc, const value* argv);
		static value Func_Max(script_machine* machine, int argc, const value* argv);
		static value Func_Clamp(script_machine* machine, int argc, const value* argv);

		static value Func_Log(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_Log2);
		static value Func_Log10(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_LogN);
		DNH_FUNCAPI_DECL_(Func_ErF);
		DNH_FUNCAPI_DECL_(Func_Gamma);

		static value Func_Cos(script_machine* machine, int argc, const value* argv);
		static value Func_Sin(script_machine* machine, int argc, const value* argv);
		static value Func_Tan(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_SinCos);
		DNH_FUNCAPI_DECL_(Func_RCos);
		DNH_FUNCAPI_DECL_(Func_RSin);
		DNH_FUNCAPI_DECL_(Func_RTan);
		DNH_FUNCAPI_DECL_(Func_RSinCos);

		static value Func_Acos(script_machine* machine, int argc, const value* argv);
		static value Func_Asin(script_machine* machine, int argc, const value* argv);
		static value Func_Atan(script_machine* machine, int argc, const value* argv);
		static value Func_Atan2(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_RAcos);
		DNH_FUNCAPI_DECL_(Func_RAsin);
		DNH_FUNCAPI_DECL_(Func_RAtan);
		DNH_FUNCAPI_DECL_(Func_RAtan2);

		DNH_FUNCAPI_DECL_(Func_Exp);
		DNH_FUNCAPI_DECL_(Func_Sqrt);
		DNH_FUNCAPI_DECL_(Func_Cbrt);
		DNH_FUNCAPI_DECL_(Func_NRoot);
		DNH_FUNCAPI_DECL_(Func_Hypot);
		DNH_FUNCAPI_DECL_(Func_Distance);
		DNH_FUNCAPI_DECL_(Func_DistanceSq);
		template<bool USE_RAD>
		DNH_FUNCAPI_DECL_(Func_GapAngle);

		//Math functions; random
		static value Func_Rand(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_RandI);
		DNH_FUNCAPI_DECL_(Func_RandEff);
		DNH_FUNCAPI_DECL_(Func_RandEffI);
		DNH_FUNCAPI_DECL_(Func_RandEffSet);
		DNH_FUNCAPI_DECL_(Func_GetRandCount);
		DNH_FUNCAPI_DECL_(Func_GetRandEffCount);
		DNH_FUNCAPI_DECL_(Func_ResetRandCount);
		DNH_FUNCAPI_DECL_(Func_ResetRandEffCount);

		//Math functions; angle helper
		DNH_FUNCAPI_DECL_(Func_ToDegrees);
		DNH_FUNCAPI_DECL_(Func_ToRadians);
		template<bool USE_RAD>
		DNH_FUNCAPI_DECL_(Func_NormalizeAngle);
		template<bool USE_RAD>
		DNH_FUNCAPI_DECL_(Func_AngularDistance);
		template<bool USE_RAD>
		DNH_FUNCAPI_DECL_(Func_ReflectAngle);

		//Math functions; interpolation
		template<double (*func)(double, double, double)>
		DNH_FUNCAPI_DECL_(Func_Interpolate);
		DNH_FUNCAPI_DECL_(Func_Interpolate_Modulate);
		DNH_FUNCAPI_DECL_(Func_Interpolate_Overshoot);
		DNH_FUNCAPI_DECL_(Func_Interpolate_QuadraticBezier);
		DNH_FUNCAPI_DECL_(Func_Interpolate_CubicBezier);
		DNH_FUNCAPI_DECL_(Func_Interpolate_Hermite);

		DNH_FUNCAPI_DECL_(Func_Interpolate_X);
		DNH_FUNCAPI_DECL_(Func_Interpolate_X_Packed);
		template<bool USE_RAD>
		DNH_FUNCAPI_DECL_(Func_Interpolate_X_Angle);
		DNH_FUNCAPI_DECL_(Func_Interpolate_X_Array);

		//Math functions; rotation
		DNH_FUNCAPI_DECL_(Func_Rotate2D);
		DNH_FUNCAPI_DECL_(Func_Rotate3D);

		//String manipulations
		static value Func_ToString(script_machine* machine, int argc, const value* argv);
		static value Func_ItoA(script_machine* machine, int argc, const value* argv);
		static value Func_RtoA(script_machine* machine, int argc, const value* argv);
		static value Func_RtoS(script_machine* machine, int argc, const value* argv);
		static value Func_VtoS(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_StringFormat);
		static value Func_AtoI(script_machine* machine, int argc, const value* argv);
		static value Func_AtoR(script_machine* machine, int argc, const value* argv);
		static value Func_TrimString(script_machine* machine, int argc, const value* argv);
		static value Func_SplitString(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_SplitString2);

		//String manipulations; regular expressions
		DNH_FUNCAPI_DECL_(Func_RegexMatch);
		DNH_FUNCAPI_DECL_(Func_RegexMatchRepeated);
		DNH_FUNCAPI_DECL_(Func_RegexReplace);

		//Path utility
		static value Func_GetParentScriptDirectory(script_machine* machine, int argc, const value* argv);
		static value Func_GetCurrentScriptDirectory(script_machine* machine, int argc, const value* argv);
		static value Func_GetFilePathList(script_machine* machine, int argc, const value* argv);
		static value Func_GetDirectoryList(script_machine* machine, int argc, const value* argv);

		DNH_FUNCAPI_DECL_(Func_GetWorkingDirectory);
		DNH_FUNCAPI_DECL_(Func_GetModuleName);
		DNH_FUNCAPI_DECL_(Func_GetModuleDirectory);
		DNH_FUNCAPI_DECL_(Func_GetFileDirectory);
		DNH_FUNCAPI_DECL_(Func_GetFileDirectoryFromModule);
		DNH_FUNCAPI_DECL_(Func_GetFileTopDirectory);
		DNH_FUNCAPI_DECL_(Func_GetFileName);
		DNH_FUNCAPI_DECL_(Func_GetFileNameWithoutExtension);
		DNH_FUNCAPI_DECL_(Func_GetFileExtension);
		DNH_FUNCAPI_DECL_(Func_IsFileExists);
		DNH_FUNCAPI_DECL_(Func_IsDirectoryExists);

		//System time
		DNH_FUNCAPI_DECL_(Func_GetSystemTimeMilliS);
		DNH_FUNCAPI_DECL_(Func_GetSystemTimeNanoS);
		static value Func_GetCurrentDateTimeS(script_machine* machine, int argc, const value* argv);

		//Debugging
		static value Func_WriteLog(script_machine* machine, int argc, const value* argv);
		static value Func_RaiseError(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_RaiseMessageWindow);
	};

#pragma region ScriptClientBase_impl
	value ScriptClientBase::CreateFloatValue(double r) {
		return value(script_type_manager::get_float_type(), r);
	}
	value ScriptClientBase::CreateIntValue(int64_t r) {
		return value(script_type_manager::get_int_type(), r);
	}
	value ScriptClientBase::CreateBooleanValue(bool b) {
		return value(script_type_manager::get_boolean_type(), b);
	}
	value ScriptClientBase::CreateStringValue(const std::wstring& s) {
		return value(script_type_manager::get_string_type(), s);
	}
	value ScriptClientBase::CreateStringValue(const std::string& s) {
		return CreateStringValue(StringUtility::ConvertMultiToWide(s));
	}

	template<typename T> value ScriptClientBase::CreateFloatArrayValue(const std::vector<T>& list) {
		return CreateFloatArrayValue(list.data(), list.size());
	}
	template<typename T>
	value ScriptClientBase::CreateFloatArrayValue(const T* ptrList, size_t count) {
		type_data* type_float = script_type_manager::get_float_type();
		type_data* type_arr = script_type_manager::get_float_array_type();
		if (ptrList && count > 0) {
			std::vector<value> res_arr;
			res_arr.resize(count);
			for (size_t iVal = 0U; iVal < count; ++iVal) {
				res_arr[iVal] = value(type_float, (double)(ptrList[iVal]));
			}

			value res;
			res.reset(type_arr, res_arr);
			return res;
		}
		return value(type_arr, std::wstring());
	}
	template<size_t N>
	value ScriptClientBase::CreateFloatArrayValue(const Math::DVec<N>& arr) {
		return CreateFloatArrayValue((double*)arr.data(), N);
	}

	template<typename T> value ScriptClientBase::CreateIntArrayValue(const std::vector<T>& list) {
		return CreateIntArrayValue(list.data(), list.size());
	}
	template<typename T>
	value ScriptClientBase::CreateIntArrayValue(const T* ptrList, size_t count) {
		type_data* type_int = script_type_manager::get_int_type();
		type_data* type_arr = script_type_manager::get_int_array_type();
		if (ptrList && count > 0) {
			std::vector<value> res_arr;
			res_arr.resize(count);
			for (size_t iVal = 0U; iVal < count; ++iVal) {
				res_arr[iVal] = value(type_int, (int64_t)(ptrList[iVal]));
			}

			value res;
			res.reset(type_arr, res_arr);
			return res;
		}
		return value(type_arr, std::wstring());
	}
#pragma endregion ScriptClientBase_impl

	//*******************************************************************
	//ScriptLoader
	//*******************************************************************
	class ScriptLoader {
	protected:
		ScriptClientBase* script_;

		std::wstring pathSource_;
		std::vector<char> src_;
		Encoding::Type encoding_;
		size_t charSize_;

		unique_ptr<Scanner> scanner_;

		ScriptFileLineMap* mapLine_;
		std::set<std::wstring> setIncludedPath_;
	protected:
		void _RaiseError(int line, const std::wstring& err);
		void _DumpRes();

		void _ResetScanner(size_t iniReadPos);
		void _AssertNewline();
		bool _SkipToNextValidLine();

		void _ParseInclude();
		void _ParseIfElse();

		void _ConvertToEncoding(Encoding::Type targetEncoding);
	public:
		ScriptLoader(ScriptClientBase* script, const std::wstring& path, 
			std::vector<char>& source, ScriptFileLineMap* mapLine);

		void Parse();

		std::vector<char>& GetResult() { return src_; }
		ScriptFileLineMap* GetLineMap() { return mapLine_; }
	};
}

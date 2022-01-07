#pragma once

#include "../pch.h"

#include "GstdUtility.hpp"
#include "Script/Script.hpp"
#include "RandProvider.hpp"
#include "Thread.hpp"
#include "File.hpp"
#include "Logger.hpp"

namespace gstd {
	class ScriptFileLineMap;
	class ScriptCommonDataManager;

	//*******************************************************************
	//ScriptEngineData
	//*******************************************************************
	class ScriptEngineData {
	protected:
		std::wstring path_;

		Encoding::Type encoding_;
		std::vector<char> source_;

		ref_count_ptr<script_engine> engine_;
		ref_count_ptr<ScriptFileLineMap> mapLine_;
	public:
		ScriptEngineData();
		virtual ~ScriptEngineData();

		void SetPath(const std::wstring& path) { path_ = path; }
		std::wstring& GetPath() { return path_; }

		void SetSource(std::vector<char>& source);
		std::vector<char>& GetSource() { return source_; }
		Encoding::Type GetEncoding() { return encoding_; }

		void SetEngine(ref_count_ptr<script_engine>& engine) { engine_ = engine; }
		ref_count_ptr<script_engine> GetEngine() { return engine_; }

		ref_count_ptr<ScriptFileLineMap> GetScriptFileLineMap() { return mapLine_; }
		void SetScriptFileLineMap(ref_count_ptr<ScriptFileLineMap>& mapLine) { mapLine_ = mapLine; }
	};

	//*******************************************************************
	//ScriptEngineCache
	//*******************************************************************
	class ScriptEngineCache {
	protected:
		std::map<std::wstring, ref_count_ptr<ScriptEngineData>> cache_;
	public:
		ScriptEngineCache();

		void Clear();

		void AddCache(const std::wstring& name, ref_count_ptr<ScriptEngineData>& data);
		void RemoveCache(const std::wstring& name);
		ref_count_ptr<ScriptEngineData> GetCache(const std::wstring& name);

		const std::map<std::wstring, ref_count_ptr<ScriptEngineData>>& GetMap() { return cache_; }

		bool IsExists(const std::wstring& name);
	};

	//*******************************************************************
	//ScriptClientBase
	//*******************************************************************
	class ScriptLoader;
	class ScriptClientBase {
		friend class ScriptLoader;
		static script_type_manager* pTypeManager_;
	public:
		enum {
			ID_SCRIPT_FREE = -1,
		};
		static uint64_t randCalls_;
		static uint64_t prandCalls_;
	protected:
		bool bError_;

		ref_count_ptr<ScriptEngineCache> cache_;

		ref_count_ptr<ScriptEngineData> engine_;
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

		static script_type_manager* GetDefaultScriptTypeManager() { return pTypeManager_; }

		void SetScriptEngineCache(ref_count_ptr<ScriptEngineCache>& cache) { cache_ = cache; }
		ref_count_ptr<ScriptEngineCache> GetScriptEngineCache() { return cache_; }

		ref_count_ptr<ScriptEngineData> GetEngine() { return engine_; }

		shared_ptr<RandProvider> GetRand() { return mt_; }

		virtual bool SetSourceFromFile(std::wstring path);
		virtual void SetSource(std::vector<char>& source);
		virtual void SetSource(const std::string& source);

		std::wstring& GetPath() { return engine_->GetPath(); }
		void SetPath(const std::wstring& path) { engine_->SetPath(path); }

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

		static inline value CreateRealValue(double r);
		static inline value CreateIntValue(int64_t r);
		static inline value CreateBooleanValue(bool b);
		static inline value CreateCharValue(wchar_t c);
		static inline value CreateStringValue(const std::wstring& s);
		static inline value CreateStringValue(const std::string& s);
		template<typename T> static inline value CreateRealArrayValue(const std::vector<T>& list);
		template<typename T> static value CreateRealArrayValue(const T* ptrList, size_t count);
		template<size_t N> static inline value CreateRealArrayValue(const Math::DVec<N>& arr);
		template<typename T> static inline value CreateIntArrayValue(const std::vector<T>& list);
		template<typename T> static value CreateIntArrayValue(const T* ptrList, size_t count);
		static value CreateStringArrayValue(const std::vector<std::string>& list);
		static value CreateStringArrayValue(const std::vector<std::wstring>& list);
		value CreateValueArrayValue(const std::vector<value>& list);

		static bool IsRealValue(value& v);
		static bool IsIntValue(value& v);
		static bool IsBooleanValue(value& v);
		static bool IsStringValue(value& v);
		static bool IsArrayValue(value& v);
		static bool IsArrayValue(value& v, type_data* element);
		static bool IsRealArrayValue(value& v);
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
		DNH_FUNCAPI_DECL_(Func_CosSin);
		DNH_FUNCAPI_DECL_(Func_RCos);
		DNH_FUNCAPI_DECL_(Func_RSin);
		DNH_FUNCAPI_DECL_(Func_RTan);
		DNH_FUNCAPI_DECL_(Func_RSinCos);
		DNH_FUNCAPI_DECL_(Func_RCosSin);

		DNH_FUNCAPI_DECL_(Func_Sec);
		DNH_FUNCAPI_DECL_(Func_Csc);
		DNH_FUNCAPI_DECL_(Func_Cot);
		DNH_FUNCAPI_DECL_(Func_RSec);
		DNH_FUNCAPI_DECL_(Func_RCsc);
		DNH_FUNCAPI_DECL_(Func_RCot);


		static value Func_Acos(script_machine* machine, int argc, const value* argv);
		static value Func_Asin(script_machine* machine, int argc, const value* argv);
		static value Func_Atan(script_machine* machine, int argc, const value* argv);
		static value Func_Atan2(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_RAcos);
		DNH_FUNCAPI_DECL_(Func_RAsin);
		DNH_FUNCAPI_DECL_(Func_RAtan);
		DNH_FUNCAPI_DECL_(Func_RAtan2);
		DNH_FUNCAPI_DECL_(Func_Asec);
		DNH_FUNCAPI_DECL_(Func_Acsc);
		DNH_FUNCAPI_DECL_(Func_Acot);
		DNH_FUNCAPI_DECL_(Func_RAsec);
		DNH_FUNCAPI_DECL_(Func_RAcsc);
		DNH_FUNCAPI_DECL_(Func_RAcot);

		DNH_FUNCAPI_DECL_(Func_Cas);
		DNH_FUNCAPI_DECL_(Func_RCas);

		DNH_FUNCAPI_DECL_(Func_CosH);
		DNH_FUNCAPI_DECL_(Func_SinH);
		DNH_FUNCAPI_DECL_(Func_TanH);
		DNH_FUNCAPI_DECL_(Func_AcosH);
		DNH_FUNCAPI_DECL_(Func_AsinH);
		DNH_FUNCAPI_DECL_(Func_AtanH);
		DNH_FUNCAPI_DECL_(Func_SecH);
		DNH_FUNCAPI_DECL_(Func_CscH);
		DNH_FUNCAPI_DECL_(Func_CotH);
		DNH_FUNCAPI_DECL_(Func_AsecH);
		DNH_FUNCAPI_DECL_(Func_AcscH);
		DNH_FUNCAPI_DECL_(Func_AcotH);

		DNH_FUNCAPI_DECL_(Func_Triangular);
		DNH_FUNCAPI_DECL_(Func_Tetrahedral);
		DNH_FUNCAPI_DECL_(Func_NSimplex);

		DNH_FUNCAPI_DECL_(Func_Exp);
		DNH_FUNCAPI_DECL_(Func_Sqrt);
		DNH_FUNCAPI_DECL_(Func_Cbrt);
		DNH_FUNCAPI_DECL_(Func_NRoot);
		DNH_FUNCAPI_DECL_(Func_Hypot);
		DNH_FUNCAPI_DECL_(Func_Distance);
		DNH_FUNCAPI_DECL_(Func_DistanceSq);
		template<bool USE_RAD>
		DNH_FUNCAPI_DECL_(Func_GapAngle);
		template<bool USE_RAD>
		DNH_FUNCAPI_DECL_(Func_PolarToCartesian);
		template<bool USE_RAD>
		DNH_FUNCAPI_DECL_(Func_CartesianToPolar);

		//Math functions; random
		static value Func_Rand(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_RandI);
		DNH_FUNCAPI_DECL_(Func_RandEff);
		DNH_FUNCAPI_DECL_(Func_RandEffI);
		DNH_FUNCAPI_DECL_(Func_RandArray);
		DNH_FUNCAPI_DECL_(Func_RandEffArray);
		DNH_FUNCAPI_DECL_(Func_RandArrayI);
		DNH_FUNCAPI_DECL_(Func_RandEffArrayI);
		DNH_FUNCAPI_DECL_(Func_Choose);
		DNH_FUNCAPI_DECL_(Func_ChooseEff);
		DNH_FUNCAPI_DECL_(Func_Shuffle);
		DNH_FUNCAPI_DECL_(Func_ShuffleEff);
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

		//Math functions; kinematics
		template<double (*funcKinematic)(double, double, double)>
		DNH_FUNCAPI_DECL_(Func_Kinematic);

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
		template<wint_t (*func)(wint_t)>
		static value Func_RecaseString(script_machine* machine, int argc, const value* argv);
		template<int (*func)(wint_t)>
		static value Func_ClassifyString(script_machine* machine, int argc, const value* argv);
		static value Func_TrimString(script_machine* machine, int argc, const value* argv);
		static value Func_SplitString(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_SplitString2);

		//String manipulations; regular expressions
		DNH_FUNCAPI_DECL_(Func_RegexMatch);
		DNH_FUNCAPI_DECL_(Func_RegexMatchRepeated);
		DNH_FUNCAPI_DECL_(Func_RegexReplace);

		//Digit functions
		DNH_FUNCAPI_DECL_(Func_DigitToArray);
		DNH_FUNCAPI_DECL_(Func_GetDigitCount);

		//Point lists
		DNH_FUNCAPI_DECL_(Func_GetPoints_Line);
		DNH_FUNCAPI_DECL_(Func_GetPoints_Circle);
		DNH_FUNCAPI_DECL_(Func_GetPoints_Ellipse);
		DNH_FUNCAPI_DECL_(Func_GetPoints_EquidistantEllipse);
		DNH_FUNCAPI_DECL_(Func_GetPoints_RegularPolygon);

		//Path utility
		static value Func_GetParentScriptDirectory(script_machine* machine, int argc, const value* argv);
		static value Func_GetCurrentScriptDirectory(script_machine* machine, int argc, const value* argv);
		static value Func_GetFilePathList(script_machine* machine, int argc, const value* argv);
		static value Func_GetDirectoryList(script_machine* machine, int argc, const value* argv);

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
		DNH_FUNCAPI_DECL_(Func_GetSystemTimeMicroS);
		DNH_FUNCAPI_DECL_(Func_GetSystemTimeNanoS);
		static value Func_GetCurrentDateTimeS(script_machine* machine, int argc, const value* argv);

		//Debugging
		static value Func_WriteLog(script_machine* machine, int argc, const value* argv);
		static value Func_RaiseError(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_RaiseMessageWindow);

		//Script common data
		static value Func_SetCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_GetCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_ClearCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_DeleteCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_SetAreaCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_GetAreaCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_ClearAreaCommonData(script_machine* machine, int argc, const value* argv);
		static value Func_DeleteAreaCommonData(script_machine* machine, int argc, const value* argv);
		DNH_FUNCAPI_DECL_(Func_DeleteWholeAreaCommonData);
		static value Func_CreateCommonDataArea(script_machine* machine, int argc, const value* argv);
		static value Func_CopyCommonDataArea(script_machine* machine, int argc, const value* argv);
		static value Func_IsCommonDataAreaExists(script_machine* machine, int argc, const value* argv);
		static value Func_GetCommonDataAreaKeyList(script_machine* machine, int argc, const value* argv);
		static value Func_GetCommonDataValueKeyList(script_machine* machine, int argc, const value* argv);

		DNH_FUNCAPI_DECL_(Func_LoadCommonDataValuePointer);
		DNH_FUNCAPI_DECL_(Func_LoadAreaCommonDataValuePointer);
		DNH_FUNCAPI_DECL_(Func_IsValidCommonDataValuePointer);
		DNH_FUNCAPI_DECL_(Func_SetCommonDataPtr);
		DNH_FUNCAPI_DECL_(Func_GetCommonDataPtr);
	};

#pragma region ScriptClientBase_impl
	value ScriptClientBase::CreateRealValue(double r) {
		return value(script_type_manager::get_real_type(), r);
	}
	value ScriptClientBase::CreateIntValue(int64_t r) {
		return value(script_type_manager::get_int_type(), r);
	}
	value ScriptClientBase::CreateBooleanValue(bool b) {
		return value(script_type_manager::get_boolean_type(), b);
	}
	value ScriptClientBase::CreateCharValue(wchar_t c) {
		return value(script_type_manager::get_char_type(), c);
	}
	value ScriptClientBase::CreateStringValue(const std::wstring& s) {
		return value(script_type_manager::get_string_type(), s);
	}
	value ScriptClientBase::CreateStringValue(const std::string& s) {
		return CreateStringValue(StringUtility::ConvertMultiToWide(s));
	}

	template<typename T> value ScriptClientBase::CreateRealArrayValue(const std::vector<T>& list) {
		return CreateRealArrayValue(list.data(), list.size());
	}
	template<typename T>
	value ScriptClientBase::CreateRealArrayValue(const T* ptrList, size_t count) {
		type_data* type_real = script_type_manager::get_real_type();
		type_data* type_arr = script_type_manager::get_real_array_type();
		if (ptrList && count > 0) {
			std::vector<value> res_arr;
			res_arr.resize(count);
			for (size_t iVal = 0U; iVal < count; ++iVal) {
				res_arr[iVal] = value(type_real, (double)(ptrList[iVal]));
			}

			value res;
			res.reset(type_arr, res_arr);
			return res;
		}
		return value(type_arr, std::wstring());
	}
	template<size_t N>
	value ScriptClientBase::CreateRealArrayValue(const Math::DVec<N>& arr) {
		return CreateRealArrayValue((double*)arr.data(), N);
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

		gstd::ref_count_ptr<ScriptFileLineMap> mapLine_;
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
		ScriptLoader(ScriptClientBase* script, const std::wstring& path, std::vector<char>& source);
		~ScriptLoader();

		void Parse();

		std::vector<char>& GetResult() { return src_; }
		gstd::ref_count_ptr<ScriptFileLineMap> GetLineMap() { return mapLine_; }
	};

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
	};

	//*******************************************************************
	//ScriptCommonData
	//*******************************************************************
	class ScriptCommonData {
	public:
		static constexpr const char* HEADER_SAVED_DATA = "DNHCDR\0\0";
		static constexpr size_t HEADER_SAVED_DATA_SIZE = 8U;
		static constexpr size_t DATA_HASH = 0xabcdef69u;

		struct _Script_PointerData {
			ScriptCommonData* pArea = nullptr;
			gstd::value* pData = nullptr;
		};
	protected:
		volatile size_t verifHash_;
		std::map<std::string, gstd::value> mapValue_;

		gstd::value _ReadRecord(gstd::ByteBuffer& buffer);
		void _WriteRecord(gstd::ByteBuffer& buffer, const gstd::value& comValue);
	public:
		ScriptCommonData();
		virtual ~ScriptCommonData();

		void Clear();
		std::pair<bool, std::map<std::string, gstd::value>::iterator> IsExists(const std::string& name);

		gstd::value* GetValueRef(const std::string& name);
		gstd::value* GetValueRef(std::map<std::string, gstd::value>::iterator itr);
		gstd::value GetValue(const std::string& name);
		gstd::value GetValue(std::map<std::string, gstd::value>::iterator itr);
		void SetValue(const std::string& name, gstd::value v);
		void SetValue(std::map<std::string, gstd::value>::iterator itr, gstd::value v);
		void DeleteValue(const std::string& name);
		void Copy(shared_ptr<ScriptCommonData>& dataSrc);

		std::map<std::string, gstd::value>::iterator MapBegin() { return mapValue_.begin(); }
		std::map<std::string, gstd::value>::iterator MapEnd() { return mapValue_.end(); }

		void ReadRecord(gstd::RecordBuffer& record);
		void WriteRecord(gstd::RecordBuffer& record);

		bool CheckHash() { return verifHash_ == DATA_HASH; }
		static bool Script_DecomposePtr(uint64_t val, _Script_PointerData* dst);
	};

	//*******************************************************************
	//ScriptCommonDataManager
	//*******************************************************************
	class ScriptCommonDataManager {
		static ScriptCommonDataManager* inst_;
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

		static ScriptCommonDataManager* GetInstance() { return inst_; }

		void Clear();
		void Erase(const std::string& name);

		const std::string& GetDefaultAreaName() { return nameAreaDefault_; }
		CommonDataMap::iterator GetDefaultAreaIterator() { return defaultAreaIterator_; }

		std::pair<bool, CommonDataMap::iterator> IsExists(const std::string& name);
		CommonDataMap::iterator CreateArea(const std::string& name);
		void CopyArea(const std::string& nameDest, const std::string& nameSrc);
		shared_ptr<ScriptCommonData> GetData(const std::string& name);
		shared_ptr<ScriptCommonData> GetData(CommonDataMap::iterator itr);
		void SetData(const std::string& name, shared_ptr<ScriptCommonData> commonData);
		void SetData(CommonDataMap::iterator itr, shared_ptr<ScriptCommonData> commonData);

		CommonDataMap::iterator MapBegin() { return mapData_.begin(); }
		CommonDataMap::iterator MapEnd() { return mapData_.end(); }

		gstd::CriticalSection& GetLock() { return lock_; }
	};

	//*******************************************************************
	//ScriptCommonDataInfoPanel
	//*******************************************************************
	class ScriptCommonDataInfoPanel : public WindowLogger::Panel {
	protected:
		enum {
			COL_AREA = 0,
			COL_KEY = 0,
			COL_VALUE,
		};

		std::vector<std::map<std::string, shared_ptr<ScriptCommonData>>::iterator> vecMapItr_;

		gstd::CriticalSection lock_;

		ScriptCommonDataManager* commonDataManager_;

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
		virtual void Update();
	};

}

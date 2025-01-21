#include "source/GcLib/pch.h"

#include "ScriptClient.hpp"
#include "File.hpp"
#include "Logger.hpp"

using namespace gstd;

//****************************************************************************
//ScriptEngineData
//****************************************************************************
ScriptEngineData::ScriptEngineData() {
	encoding_ = Encoding::UNKNOWN;
}
ScriptEngineData::~ScriptEngineData() {}
void ScriptEngineData::SetSource(std::vector<char>& source) {
	encoding_ = Encoding::Detect(source.data(), source.size());
	//if (encoding_ == Encoding::UTF8BOM) encoding_ = Encoding::UTF8;
	source_ = source;
}

//****************************************************************************
//ScriptEngineCache
//****************************************************************************
ScriptEngineCache::ScriptEngineCache() {
}
void ScriptEngineCache::Clear() {
	cache_.clear();
}
void ScriptEngineCache::AddCache(const std::wstring& name, shared_ptr<ScriptEngineData> data) {
	auto& res = (cache_[name] = MOVE(data));
}
void ScriptEngineCache::RemoveCache(const std::wstring& name) {
	auto itrFind = cache_.find(name);
	if (cache_.find(name) != cache_.end())
		cache_.erase(itrFind);
}
shared_ptr<ScriptEngineData> ScriptEngineCache::GetCache(const std::wstring& name) {
	auto itrFind = cache_.find(name);
	if (cache_.find(name) == cache_.end()) return nullptr;
	return itrFind->second;
}
bool ScriptEngineCache::IsExists(const std::wstring& name) {
	return cache_.find(name) != cache_.end();
}

//****************************************************************************
//ScriptClientBase
//****************************************************************************
static const std::vector<function> commonFunction = {
	//Script functions
	{ "GetScriptArgument", ScriptClientBase::Func_GetScriptArgument, 1 },
	{ "GetScriptArgumentCount", ScriptClientBase::Func_GetScriptArgumentCount, 0 },
	{ "SetScriptResult", ScriptClientBase::Func_SetScriptResult, 1 },

	//Floating point functions
	{ "Float_Classify", ScriptClientBase::Float_Classify, 1 },
	{ "Float_IsNan", ScriptClientBase::Float_IsNan, 1 },
	{ "Float_IsInf", ScriptClientBase::Float_IsInf, 1 },
	{ "Float_GetSign", ScriptClientBase::Float_GetSign, 1 },
	{ "Float_CopySign", ScriptClientBase::Float_CopySign, 2 },

	//Math functions
	{ "min", ScriptClientBase::Func_Min, 2 },
	{ "max", ScriptClientBase::Func_Max, 2 },
	{ "clamp", ScriptClientBase::Func_Clamp, 3 },

	{ "log", ScriptClientBase::Func_Log, 1 },
	{ "log2", ScriptClientBase::Func_Log2, 1 },
	{ "log10", ScriptClientBase::Func_Log10, 1 },
	{ "logn", ScriptClientBase::Func_LogN, 2 },
	{ "erf", ScriptClientBase::Func_ErF, 1 },
	{ "gamma", ScriptClientBase::Func_Gamma, 1 },

	//Math functions: Trigonometry
	{ "cos", ScriptClientBase::Func_Cos, 1 },
	{ "sin", ScriptClientBase::Func_Sin, 1 },
	{ "tan", ScriptClientBase::Func_Tan, 1 },
	{ "sincos", ScriptClientBase::Func_SinCos, 1 },
	{ "rcos", ScriptClientBase::Func_RCos, 1 },
	{ "rsin", ScriptClientBase::Func_RSin, 1 },
	{ "rtan", ScriptClientBase::Func_RTan, 1 },
	{ "rsincos", ScriptClientBase::Func_RSinCos, 1 },

	{ "acos", ScriptClientBase::Func_Acos, 1 },
	{ "asin", ScriptClientBase::Func_Asin, 1 },
	{ "atan", ScriptClientBase::Func_Atan, 1 },
	{ "atan2", ScriptClientBase::Func_Atan2, 2 },
	{ "racos", ScriptClientBase::Func_RAcos, 1 },
	{ "rasin", ScriptClientBase::Func_RAsin, 1 },
	{ "ratan", ScriptClientBase::Func_RAtan, 1 },
	{ "ratan2", ScriptClientBase::Func_RAtan2, 2 },

	//Math functions: Angles
	{ "ToDegrees", ScriptClientBase::Func_ToDegrees, 1 },
	{ "ToRadians", ScriptClientBase::Func_ToRadians, 1 },
	{ "NormalizeAngle", ScriptClientBase::Func_NormalizeAngle<false>, 1 },
	{ "NormalizeAngleR", ScriptClientBase::Func_NormalizeAngle<true>, 1 },
	{ "AngularDistance", ScriptClientBase::Func_AngularDistance<false>, 2 },
	{ "AngularDistanceR", ScriptClientBase::Func_AngularDistance<true>, 2 },
	{ "ReflectAngle", ScriptClientBase::Func_ReflectAngle<false>, 2 },
	{ "ReflectAngleR", ScriptClientBase::Func_ReflectAngle<true>, 2 },

	//Math functions: Extra
	{ "exp", ScriptClientBase::Func_Exp, 1 },
	{ "sqrt", ScriptClientBase::Func_Sqrt, 1 },
	{ "cbrt", ScriptClientBase::Func_Cbrt, 1 },
	{ "nroot", ScriptClientBase::Func_NRoot, 2 },
	{ "hypot", ScriptClientBase::Func_Hypot, 2 },
	{ "distance", ScriptClientBase::Func_Distance, 4 },
	{ "distancesq", ScriptClientBase::Func_DistanceSq, 4 },
	{ "dottheta", ScriptClientBase::Func_GapAngle<false>, 4 },
	{ "rdottheta", ScriptClientBase::Func_GapAngle<true>, 4 },

	//Random
	{ "rand", ScriptClientBase::Func_Rand, 2 },
	{ "rand_int", ScriptClientBase::Func_RandI, 2 },
	{ "prand", ScriptClientBase::Func_RandEff, 2 },
	{ "prand_int", ScriptClientBase::Func_RandEffI, 2 },
	{ "psrand", ScriptClientBase::Func_RandEffSet, 1 },
	{ "count_rand", ScriptClientBase::Func_GetRandCount, 0 },
	{ "count_prand", ScriptClientBase::Func_GetRandEffCount, 0 },
	{ "reset_count_rand", ScriptClientBase::Func_ResetRandCount, 0 },
	{ "reset_count_prand", ScriptClientBase::Func_ResetRandEffCount, 0 },

	//Interpolation
	{ "Interpolate_Linear", ScriptClientBase::Func_Interpolate<Math::Lerp::Linear>, 3 },
	{ "Interpolate_Smooth", ScriptClientBase::Func_Interpolate<Math::Lerp::Smooth>, 3 },
	{ "Interpolate_Smoother", ScriptClientBase::Func_Interpolate<Math::Lerp::Smoother>, 3 },
	{ "Interpolate_Accelerate", ScriptClientBase::Func_Interpolate<Math::Lerp::Accelerate>, 3 },
	{ "Interpolate_Decelerate", ScriptClientBase::Func_Interpolate<Math::Lerp::Decelerate>, 3 },
	{ "Interpolate_Modulate", ScriptClientBase::Func_Interpolate_Modulate, 4 },
	{ "Interpolate_Overshoot", ScriptClientBase::Func_Interpolate_Overshoot, 4 },
	{ "Interpolate_QuadraticBezier", ScriptClientBase::Func_Interpolate_QuadraticBezier, 4 },
	{ "Interpolate_CubicBezier", ScriptClientBase::Func_Interpolate_CubicBezier, 5 },
	{ "Interpolate_Hermite", ScriptClientBase::Func_Interpolate_Hermite, 9 },
	{ "Interpolate_X", ScriptClientBase::Func_Interpolate_X, 4 },
	{ "Interpolate_X_PackedInt", ScriptClientBase::Func_Interpolate_X_Packed, 4 },
	{ "Interpolate_X_Angle", ScriptClientBase::Func_Interpolate_X_Angle<false>, 4 },
	{ "Interpolate_X_AngleR", ScriptClientBase::Func_Interpolate_X_Angle<true>, 4 },
    { "Interpolate_X_Array", ScriptClientBase::Func_Interpolate_X_Array, 3 },

	//Rotation
	{ "Rotate2D", ScriptClientBase::Func_Rotate2D, 3 },
	{ "Rotate2D", ScriptClientBase::Func_Rotate2D, 5 },
	{ "Rotate3D", ScriptClientBase::Func_Rotate3D, 6 },
	{ "Rotate3D", ScriptClientBase::Func_Rotate3D, 9 },

	//String functions
	{ "ToString", ScriptClientBase::Func_ToString, 1 },
	{ "IntToString", ScriptClientBase::Func_ItoA, 1 },
	{ "itoa", ScriptClientBase::Func_ItoA, 1 },
	{ "rtoa", ScriptClientBase::Func_RtoA, 1 },
	{ "rtos", ScriptClientBase::Func_RtoS, 2 },
	{ "vtos", ScriptClientBase::Func_VtoS, 2 },
	{ "StringFormat", ScriptClientBase::Func_StringFormat, -4 },	//2 fixed + ... -> 3 minimum
	{ "atoi", ScriptClientBase::Func_AtoI, 1 },
	{ "atoi", ScriptClientBase::Func_AtoI, 2 },		//Overloaded
	{ "ator", ScriptClientBase::Func_AtoR, 1 },
	{ "TrimString", ScriptClientBase::Func_TrimString, 1 },
	{ "SplitString", ScriptClientBase::Func_SplitString, 2 },
	{ "SplitString2", ScriptClientBase::Func_SplitString2, 2 },

	{ "RegexMatch", ScriptClientBase::Func_RegexMatch, 2 },
	{ "RegexMatchRepeated", ScriptClientBase::Func_RegexMatchRepeated, 2 },
	{ "RegexReplace", ScriptClientBase::Func_RegexReplace, 3 },

	//Path utilities
	{ "GetParentScriptDirectory", ScriptClientBase::Func_GetParentScriptDirectory, 0 },
	{ "GetCurrentScriptDirectory", ScriptClientBase::Func_GetCurrentScriptDirectory, 0 },
	{ "GetFilePathList", ScriptClientBase::Func_GetFilePathList, 1 },
	{ "GetDirectoryList", ScriptClientBase::Func_GetDirectoryList, 1 },

	{ "GetWorkingDirectory", ScriptClientBase::Func_GetWorkingDirectory, 0 },
	{ "GetModuleName", ScriptClientBase::Func_GetModuleName, 0 },
	{ "GetModuleDirectory", ScriptClientBase::Func_GetModuleDirectory, 0 },
	{ "GetFileDirectory", ScriptClientBase::Func_GetFileDirectory, 1 },
	{ "GetFileDirectoryFromModule", ScriptClientBase::Func_GetFileDirectoryFromModule, 1 },
	{ "GetFileTopDirectory", ScriptClientBase::Func_GetFileTopDirectory, 1 },
	{ "GetFileName", ScriptClientBase::Func_GetFileName, 1 },
	{ "GetFileNameWithoutExtension", ScriptClientBase::Func_GetFileNameWithoutExtension, 1 },
	{ "GetFileExtension", ScriptClientBase::Func_GetFileExtension, 1 },
	{ "IsFileExists", ScriptClientBase::Func_IsFileExists, 1 },
	{ "IsDirectoryExists", ScriptClientBase::Func_IsDirectoryExists, 1 },

	//System time
	{ "GetSystemTimeMilliS", ScriptClientBase::Func_GetSystemTimeMilliS, 0 },
	{ "GetSystemTimeNanoS", ScriptClientBase::Func_GetSystemTimeNanoS, 0 },
	{ "GetCurrentDateTimeS", ScriptClientBase::Func_GetCurrentDateTimeS, 0 },

	//Debug stuff
	{ "WriteLog", ScriptClientBase::Func_WriteLog, -2 },		//0 fixed + ... -> 1 minimum
	{ "RaiseError", ScriptClientBase::Func_RaiseError, 1 },
	{ "RaiseMessageWindow", ScriptClientBase::Func_RaiseMessageWindow, 2 },
	{ "RaiseMessageWindow", ScriptClientBase::Func_RaiseMessageWindow, 3 },	//Overloaded
};
static const std::vector<constant> commonConstant = {
	constant("NULL", 0i64),

	constant("INF", INFINITY),
	constant("NAN", NAN),

	//Types for Float_Classify
	constant("FLOAT_TYPE_SUBNORMAL", FP_SUBNORMAL),
	constant("FLOAT_TYPE_NORMAL", FP_NORMAL),
	constant("FLOAT_TYPE_ZERO", FP_ZERO),
	constant("FLOAT_TYPE_INFINITY", FP_INFINITE),
	constant("FLOAT_TYPE_NAN", FP_NAN),

	//Types for typeof and ftypeof
	constant("VAR_INT", type_data::tk_int),
	constant("VAR_FLOAT", type_data::tk_float),
	constant("VAR_CHAR", type_data::tk_char),
	constant("VAR_BOOL", type_data::tk_boolean),
	constant("VAR_ARRAY", type_data::tk_array),
	constant("VAR_STRING", type_data::tk_string),

	//Interpolation modes
	constant("LERP_LINEAR", Math::Lerp::LINEAR),
	constant("LERP_SMOOTH", Math::Lerp::SMOOTH),
	constant("LERP_SMOOTHER", Math::Lerp::SMOOTHER),
	constant("LERP_ACCELERATE", Math::Lerp::ACCELERATE),
	constant("LERP_DECELERATE", Math::Lerp::DECELERATE),

	//Math constants
	constant("M_PI", GM_PI),
	constant("M_PI_2", GM_PI_2),
	constant("M_PI_4", GM_PI_4),
	constant("M_PI_X2", GM_PI_X2),
	constant("M_PI_X4", GM_PI_X4),
	constant("M_1_PI", GM_1_PI),
	constant("M_2_PI", GM_2_PI),
	constant("M_SQRTPI", GM_SQRTP),
	constant("M_1_SQRTPI", GM_1_SQRTP),
	constant("M_2_SQRTPI", GM_2_SQRTP),
	constant("M_SQRT2", GM_SQRT2),
	constant("M_SQRT2_2", GM_SQRT2_2),
	constant("M_SQRT2_X2", GM_SQRT2_X2),
	constant("M_E", GM_E),
	constant("M_LOG2E", GM_LOG2E),
	constant("M_LOG10E", GM_LOG10E),
	constant("M_LN2", GM_LN2),
	constant("M_LN10", GM_LN10),
	constant("M_PHI", GM_PHI),
	constant("M_1_PHI", GM_1_PHI),
};

unique_ptr<script_type_manager> ScriptClientBase::pTypeManager_ = unique_ptr<script_type_manager>(new script_type_manager());
uint64_t ScriptClientBase::randCalls_ = 0;
uint64_t ScriptClientBase::prandCalls_ = 0;
ScriptClientBase::ScriptClientBase() {
	bError_ = false;

	engineData_.reset(new ScriptEngineData());
	machine_ = nullptr;

	mainThreadID_ = -1;
	idScript_ = ID_SCRIPT_FREE;

	//commonDataManager_.reset(new ScriptCommonDataManager());

	{
		DWORD seed = SystemUtility::GetCpuTime2();

		mt_ = make_shared<RandProvider>();
		mt_->Initialize(seed ^ 0xc3c3c3c3);

		mtEffect_ = make_shared<RandProvider>();
		mtEffect_->Initialize(((seed ^ 0xf27ea021) << 11) ^ ((seed ^ 0x8b56c1b5) >> 11));
	}

	_AddFunction(&commonFunction);
	_AddConstant(&commonConstant);
	{
		definedMacro_[L"_DNH_PH3SX_"] = L"";
	}

	Reset();
}
ScriptClientBase::~ScriptClientBase() {
}

void ScriptClientBase::_AddFunction(const char* name, dnh_func_callback_t f, size_t arguments) {
	function tFunc(name, f, arguments);
	func_.push_back(tFunc);
}
void ScriptClientBase::_AddFunction(const std::vector<gstd::function>* f) {
	func_.insert(func_.end(), f->cbegin(), f->cend());
}
void ScriptClientBase::_AddConstant(const std::vector<gstd::constant>* c) {
	const_.insert(const_.end(), c->cbegin(), c->cend());
}

void ScriptClientBase::_RaiseError(int line, const std::wstring& message) {
	bError_ = true;
	std::wstring errorPos = _GetErrorLineSource(line);

	ScriptFileLineMap* mapLine = engineData_->GetScriptFileLineMap();
	ScriptFileLineMap::Entry* entry = mapLine->GetEntry(line);

	int lineOriginal = -1;
	std::wstring entryPath = engineData_->GetPath();
	if (entry) {
		lineOriginal = entry->lineEndOriginal_ - (entry->lineEnd_ - line);
		entryPath = entry->path_;
	}

	std::wstring fileName = PathProperty::GetFileName(entryPath);
	std::wstring str = StringUtility::Format(L"%s\r\n%s" "\r\n[%s (main=%s)] " "line-> %d\r\n\r\n↓\r\n%s\r\n～～～",
		message.c_str(),
		entryPath.c_str(),
		fileName.c_str(), PathProperty::GetFileName(engineData_->GetPath()).c_str(),
		lineOriginal, errorPos.c_str());
	throw wexception(str);
}
void ScriptClientBase::_RaiseErrorFromEngine() {
	int line = engineData_->GetEngine()->get_error_line();
	_RaiseError(line, engineData_->GetEngine()->get_error_message());
}
void ScriptClientBase::_RaiseErrorFromMachine() {
	int line = machine_->get_error_line();
	_RaiseError(line, machine_->get_error_message());
}
std::wstring ScriptClientBase::_GetErrorLineSource(int line) {
	if (line == 0) return L"";

	Encoding::Type encoding = engineData_->GetEncoding();
	std::vector<char>& source = engineData_->GetSource();

	int tLine = 1;

	char* pStr = source.data();
	char* pEnd = pStr + source.size();
	while (pStr < pEnd) {
		if (tLine == line)
			break;

		if (Encoding::BytesToWChar(pStr, encoding) == L'\n')
			++tLine;
		pStr += Encoding::GetCharSize(encoding);
	}

	constexpr size_t DISP_MAX = 256;
	size_t size = std::min(DISP_MAX, (size_t)(pEnd - pStr));
	return Encoding::BytesToWString(pStr, size, encoding);
}
std::vector<char> ScriptClientBase::_ParseScriptSource(std::vector<char>& source) {
	ScriptFileLineMap* lineMap = engineData_->GetScriptFileLineMap();

	lineMap->Clear();
	ScriptLoader scriptLoader(this, engineData_->GetPath(), source, lineMap);

	scriptLoader.Parse();

	return scriptLoader.GetResult();
}
bool ScriptClientBase::_CreateEngine() {
	unique_ptr<script_engine> engine(new script_engine(engineData_->GetSource(), &func_, &const_));
	engineData_->SetEngine(std::move(engine));
	return !engineData_->GetEngine()->get_error();
}
bool ScriptClientBase::SetSourceFromFile(std::wstring path) {
	path = PathProperty::GetUnique(path);

	if (auto pFindCache = cache_->GetCache(path)) {
		engineData_ = pFindCache;
		return true;
	}

	// Script not found in cache, create a new entry

	cache_->AddCache(path, engineData_);
	engineData_->SetPath(path);
	
	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr || !reader->Open())
		throw gstd::wexception(L"SetScriptFileSource: " + ErrorUtility::GetFileNotFoundErrorMessage(path, true));

	size_t size = reader->GetFileSize();

	std::vector<char> source;
	source.resize(size);
	reader->Read(&source[0], size);

	SetSource(source);

	return true;
}
void ScriptClientBase::SetSource(const std::string& source) {
	std::vector<char> vect;
	vect.resize(source.size());
	memcpy(&vect[0], &source[0], source.size());
	this->SetSource(vect);
}
void ScriptClientBase::SetSource(std::vector<char>& source) {
	engineData_->SetSource(source);
	ScriptFileLineMap* mapLine = engineData_->GetScriptFileLineMap();
	mapLine->AddEntry(engineData_->GetPath(), 1, StringUtility::CountCharacter(source, '\n') + 1);
}
void ScriptClientBase::Compile() {
	if (engineData_->GetEngine() == nullptr) {
		std::vector<char> source = _ParseScriptSource(engineData_->GetSource());
		engineData_->SetSource(source);

		bool bCreateSuccess = _CreateEngine();
		if (!bCreateSuccess) {
			bError_ = true;
			_RaiseErrorFromEngine();
		}
	}

	machine_.reset(new script_machine(engineData_->GetEngine().get()));
	if (machine_->get_error()) {
		bError_ = true;
		_RaiseErrorFromMachine();
	}
	machine_->data = this;
}

void ScriptClientBase::Reset() {
	if (machine_)
		machine_->reset();
	valueRes_ = value();
}
bool ScriptClientBase::Run() {
	if (bError_) return false;
	machine_->run();
	if (machine_->get_error()) {
		bError_ = true;
		_RaiseErrorFromMachine();
	}
	return true;
}
bool ScriptClientBase::Run(const std::string& target) {
	if (bError_) return false;

	std::map<std::string, script_block*>::iterator itrEvent;
	if (!machine_->has_event(target, itrEvent)) {
		_RaiseError(0, StringUtility::FormatToWide("Event doesn't exist. [%s]", target.c_str()));
	}

	return Run(itrEvent);
}
bool ScriptClientBase::Run(std::map<std::string, script_block*>::iterator target) {
	if (bError_) return false;

	//Run();
	machine_->call(target);

	if (machine_->get_error()) {
		bError_ = true;
		_RaiseErrorFromMachine();
	}
	return true;
}
bool ScriptClientBase::IsEventExists(const std::string& name, std::map<std::string, script_block*>::iterator& res) {
	if (bError_) {
		if (machine_ && machine_->get_error())
			_RaiseErrorFromMachine();
		else if (engineData_->GetEngine()->get_error())
			_RaiseErrorFromEngine();
		return false;
	}
	return machine_->has_event(name, res);
}
size_t ScriptClientBase::GetThreadCount() {
	if (machine_ == nullptr) return 0;
	return machine_->get_thread_count();
}
void ScriptClientBase::SetArgumentValue(value v, int index) {
	if (listValueArg_.size() <= index) {
		listValueArg_.resize(index + 1);
	}
	listValueArg_[index] = v;
}

value ScriptClientBase::CreateStringArrayValue(const std::vector<std::string>& list) {
	script_type_manager* typeManager = script_type_manager::get_instance();
	type_data* type_arr = typeManager->get_array_type(typeManager->get_string_type());

	if (list.size() > 0) {
		std::vector<value> res_arr;
		res_arr.resize(list.size());
		for (size_t iVal = 0U; iVal < list.size(); ++iVal) {
			value data = CreateStringValue(list[iVal]);
			res_arr[iVal] = data;
		}

		value res;
		res.reset(type_arr, res_arr);
		return res;
	}

	return value(type_arr, std::wstring());
}
value ScriptClientBase::CreateStringArrayValue(const std::vector<std::wstring>& list) {
	script_type_manager* typeManager = script_type_manager::get_instance();
	type_data* type_arr = typeManager->get_array_type(typeManager->get_string_type());

	if (list.size() > 0) {
		std::vector<value> res_arr;
		res_arr.resize(list.size());
		for (size_t iVal = 0U; iVal < list.size(); ++iVal) {
			value data = CreateStringValue(list[iVal]);
			res_arr[iVal] = data;
		}

		value res;
		res.reset(type_arr, res_arr);
		return res;
	}

	return value(type_arr, std::wstring());
}
value ScriptClientBase::CreateValueArrayValue(const std::vector<value>& list) {
	script_type_manager* typeManager = script_type_manager::get_instance();

	if (list.size() > 0) {
		type_data* type_array = typeManager->get_array_type(list[0].get_type());

		std::vector<value> res_arr;
		res_arr.resize(list.size());
		for (size_t iVal = 0U; iVal < list.size(); ++iVal) {
			BaseFunction::_append_check_no_convert(machine_.get(), type_array, list[iVal].get_type());
			res_arr[iVal] = list[iVal];
		}

		value res;
		res.reset(type_array, res_arr);
		return res;
	}

	return value(typeManager->get_null_array_type(), std::wstring());
}
bool ScriptClientBase::IsFloatValue(value& v) {
	if (!v.has_data()) return false;
	return v.get_type() == script_type_manager::get_float_type();
}
bool ScriptClientBase::IsIntValue(value& v) {
	if (!v.has_data()) return false;
	return v.get_type() == script_type_manager::get_int_type();
}
bool ScriptClientBase::IsBooleanValue(value& v) {
	if (!v.has_data()) return false;
	return v.get_type() == script_type_manager::get_boolean_type();
}
bool ScriptClientBase::IsStringValue(value& v) {
	if (!v.has_data()) return false;
	return v.get_type() == script_type_manager::get_string_type();
}
bool ScriptClientBase::IsArrayValue(value& v) {
	if (!v.has_data()) return false;
	return v.get_type()->get_kind() == type_data::tk_array;
}
bool ScriptClientBase::IsArrayValue(value& v, type_data* element) {
	if (!v.has_data()) return false;
	return v.get_type() == script_type_manager::get_instance()->get_array_type(element);
}
bool ScriptClientBase::IsFloatArrayValue(value& v) {
	if (!v.has_data()) return false;
	return v.get_type() == script_type_manager::get_float_array_type();
}
bool ScriptClientBase::IsIntArrayValue(value& v) {
	if (!v.has_data()) return false;
	return v.get_type() == script_type_manager::get_int_array_type();
}

void ScriptClientBase::CheckRunInMainThread() {
	if (mainThreadID_ < 0) return;
	if (mainThreadID_ != GetCurrentThreadId()) {
		std::string error = "This function can only be called in the main thread.\r\n";
		machine_->raise_error(error);
	}
}
std::wstring ScriptClientBase::_ExtendPath(std::wstring path) {
	int line = machine_->get_current_line();
	const std::wstring& pathScript = GetEngineData()->GetScriptFileLineMap()->GetPath(line);

	path = StringUtility::ReplaceAll(path, L'\\', L'/');
	path = StringUtility::ReplaceAll(path, L"./", pathScript);

	return path;
}

//共通関数：スクリプト引数結果
value ScriptClientBase::Func_GetScriptArgument(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = (ScriptClientBase*)machine->data;
	int index = argv->as_int();
	if (index < 0 || index >= script->listValueArg_.size()) {
		std::string error = "Invalid script argument index.\r\n";
		throw gstd::wexception(error);
	}
	return script->listValueArg_[index];
}
value ScriptClientBase::Func_GetScriptArgumentCount(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = (ScriptClientBase*)machine->data;
	size_t res = script->listValueArg_.size();
	return script->CreateIntValue(res);
}
value ScriptClientBase::Func_SetScriptResult(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = (ScriptClientBase*)machine->data;
	script->valueRes_ = argv[0];
	return value();
}

//Floating point functions
value ScriptClientBase::Float_Classify(script_machine* machine, int argc, const value* argv) {
	double f = argv[0].as_float();
	return CreateIntValue(std::fpclassify(f));
}
value ScriptClientBase::Float_IsNan(script_machine* machine, int argc, const value* argv) {
	double f = argv[0].as_float();
	return CreateBooleanValue(std::isnan(f));
}
value ScriptClientBase::Float_IsInf(script_machine* machine, int argc, const value* argv) {
	double f = argv[0].as_float();
	return CreateBooleanValue(std::isinf(f));
}
value ScriptClientBase::Float_GetSign(script_machine* machine, int argc, const value* argv) {
	double f = argv[0].as_float();
	return CreateFloatValue(std::signbit(f) ? -1.0 : 1.0);
}
value ScriptClientBase::Float_CopySign(script_machine* machine, int argc, const value* argv) {
	double src = argv[0].as_float();
	double dst = argv[1].as_float();
	return CreateFloatValue(std::copysign(src, dst));
}

//Maths functions
value ScriptClientBase::Func_Min(script_machine* machine, int argc, const value* argv) {
	double v1 = argv[0].as_float();
	double v2 = argv[1].as_float();
	return CreateFloatValue(std::min(v1, v2));
}
value ScriptClientBase::Func_Max(script_machine* machine, int argc, const value* argv) {
	double v1 = argv[0].as_float();
	double v2 = argv[1].as_float();
	return CreateFloatValue(std::max(v1, v2));
}
value ScriptClientBase::Func_Clamp(script_machine* machine, int argc, const value* argv) {
	double v = argv[0].as_float();
	double bound_lower = argv[1].as_float();
	double bound_upper = argv[2].as_float();
	//if (bound_lower > bound_upper) std::swap(bound_lower, bound_upper);
	double res = std::clamp(v, bound_lower, bound_upper);
	return CreateFloatValue(res);
}
value ScriptClientBase::Func_Log(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(log(argv->as_float()));
}
value ScriptClientBase::Func_Log2(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(log2(argv->as_float()));
}
value ScriptClientBase::Func_Log10(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(log10(argv->as_float()));
}
value ScriptClientBase::Func_LogN(script_machine* machine, int argc, const value* argv) {
	double x = argv[0].as_float();
	double base = argv[1].as_float();
	return CreateFloatValue(log(x) / log(base));
}
value ScriptClientBase::Func_ErF(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(erf(argv->as_float()));
}
value ScriptClientBase::Func_Gamma(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(tgamma(argv->as_float()));
}

value ScriptClientBase::Func_Cos(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(cos(Math::DegreeToRadian(argv->as_float())));
}
value ScriptClientBase::Func_Sin(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(sin(Math::DegreeToRadian(argv->as_float())));
}
value ScriptClientBase::Func_Tan(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(tan(Math::DegreeToRadian(argv->as_float())));
}
value ScriptClientBase::Func_SinCos(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);

	double scArray[2];
	Math::DoSinCos(Math::DegreeToRadian(argv->as_float()), scArray);

	return script->CreateFloatArrayValue(scArray, 2U);
}

value ScriptClientBase::Func_RCos(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(cos(argv->as_float()));
}
value ScriptClientBase::Func_RSin(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(sin(argv->as_float()));
}
value ScriptClientBase::Func_RTan(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(tan(argv->as_float()));
}
value ScriptClientBase::Func_RSinCos(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);

	double scArray[2];
	Math::DoSinCos(argv->as_float(), scArray);

	return script->CreateFloatArrayValue(scArray, 2U);
}

value ScriptClientBase::Func_Acos(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(Math::RadianToDegree(acos(argv->as_float())));
}
value ScriptClientBase::Func_Asin(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(Math::RadianToDegree(asin(argv->as_float())));
}
value ScriptClientBase::Func_Atan(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(Math::RadianToDegree(atan(argv->as_float())));
}
value ScriptClientBase::Func_Atan2(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(Math::RadianToDegree(atan2(argv[0].as_float(), argv[1].as_float())));
}
value ScriptClientBase::Func_RAcos(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(acos(argv->as_float()));
}
value ScriptClientBase::Func_RAsin(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(asin(argv->as_float()));
}
value ScriptClientBase::Func_RAtan(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(atan(argv->as_float()));
}
value ScriptClientBase::Func_RAtan2(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(atan2(argv[0].as_float(), argv[1].as_float()));
}

value ScriptClientBase::Func_Exp(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(exp(argv->as_float()));
}
value ScriptClientBase::Func_Sqrt(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(sqrt(argv->as_float()));
}
value ScriptClientBase::Func_Cbrt(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(cbrt(argv->as_float()));
}
value ScriptClientBase::Func_NRoot(script_machine* machine, int argc, const value* argv) {
	double val = 1.0;
	double _p = argv[1].as_float();
	if (_p != 0.0) val = pow(argv[0].as_float(), 1.0 / _p);
	return CreateFloatValue(val);
}
value ScriptClientBase::Func_Hypot(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(hypot(argv[0].as_float(), argv[1].as_float()));
}
value ScriptClientBase::Func_Distance(script_machine* machine, int argc, const value* argv) {
	double dx = argv[2].as_float() - argv[0].as_float();
	double dy = argv[3].as_float() - argv[1].as_float();
	return CreateFloatValue(hypot(dx, dy));
}
value ScriptClientBase::Func_DistanceSq(script_machine* machine, int argc, const value* argv) {
	double dx = argv[2].as_float() - argv[0].as_float();
	double dy = argv[3].as_float() - argv[1].as_float();
	return CreateFloatValue(Math::HypotSq<double>(dx, dy));
}
template<bool USE_RAD>
value ScriptClientBase::Func_GapAngle(script_machine* machine, int argc, const value* argv) {
	double dx = argv[2].as_float() - argv[0].as_float();
	double dy = argv[3].as_float() - argv[1].as_float();
	double res = atan2(dy, dx);
	return CreateFloatValue(USE_RAD ? res : Math::RadianToDegree(res));
}

value ScriptClientBase::Func_Rand(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	script->CheckRunInMainThread();
	++randCalls_;
	double min = argv[0].as_float();
	double max = argv[1].as_float();
	return script->CreateFloatValue(script->mt_->GetReal(min, max));
}
value ScriptClientBase::Func_RandI(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	script->CheckRunInMainThread();
	++randCalls_;
	double min = argv[0].as_int();
	double max = argv[1].as_int() + 0.9999999;
	return script->CreateIntValue(script->mt_->GetReal(min, max));
}
value ScriptClientBase::Func_RandEff(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	++prandCalls_;
	double min = argv[0].as_float();
	double max = argv[1].as_float();
	double res = script->mtEffect_->GetReal(min, max);
	return script->CreateFloatValue(res);
}
value ScriptClientBase::Func_RandEffI(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	++prandCalls_;
	double min = argv[0].as_int();
	double max = argv[1].as_int() + 0.9999999;
	return script->CreateIntValue(script->mtEffect_->GetReal(min, max));
}
value ScriptClientBase::Func_RandEffSet(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	int64_t seed = argv[0].as_int();
	uint32_t prseed = (uint32_t)(seed >> 32) ^ (uint32_t)(seed & 0xffffffff);
	script->mtEffect_->Initialize(prseed);
	return value();
}
value ScriptClientBase::Func_GetRandCount(script_machine* machine, int argc, const value* argv) {
	return CreateIntValue(randCalls_);
}
value ScriptClientBase::Func_GetRandEffCount(script_machine* machine, int argc, const value* argv) {
	return CreateIntValue(prandCalls_);
}
value ScriptClientBase::Func_ResetRandCount(script_machine* machine, int argc, const value* argv) {
	randCalls_ = 0;
	return value();
}
value ScriptClientBase::Func_ResetRandEffCount(script_machine* machine, int argc, const value* argv) {
	prandCalls_ = 0;
	return value();
}

static value _ScriptValueLerp(script_machine* machine, const value* v1, const value* v2, double vx, 
	double (*lerpFunc)(double, double, double)) 
{
	if (v1->get_type()->get_kind() == type_data::type_kind::tk_array 
		&& v2->get_type()->get_kind() == type_data::type_kind::tk_array)
	{
		value res;

		if (v1->length_as_array() != v2->length_as_array()) {
			std::string err = StringUtility::Format("Sizes must be the same when interpolating arrays. (%u and %u)",
				v1->length_as_array(), v2->length_as_array());
			machine->raise_error(err);
		}
		else {
			std::vector<value> resArr;
			resArr.resize(v1->length_as_array());
			for (size_t i = 0; i < v1->length_as_array(); ++i) {
				const value* a1 = &(*v1)[i];
				const value* a2 = &(*v2)[i];
				resArr[i] = _ScriptValueLerp(machine, a1, a2, vx, lerpFunc);
			}

			res.reset(v1->get_type(), resArr);
		}
		
		return res;
	}
	else {
		return ScriptClientBase::CreateFloatValue(lerpFunc(v1->as_float(), v2->as_float(), vx));
	}
}
template<double (*func)(double, double, double)>
value ScriptClientBase::Func_Interpolate(script_machine* machine, int argc, const value* argv) {
	double x = argv[2].as_float();
	return _ScriptValueLerp(machine, &argv[0], &argv[1], x, func);
}
value ScriptClientBase::Func_Interpolate_Modulate(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_float();
	double b = argv[1].as_float();
	double c = argv[2].as_float();
	double x = argv[3].as_float();

	double y = sin(GM_PI_X2 * x) * GM_1_PI * 0.5;
	double res = a + (x + y * c) * (b - a);

	return CreateFloatValue(res);
}
value ScriptClientBase::Func_Interpolate_Overshoot(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_float();
	double b = argv[1].as_float();
	double c = argv[2].as_float();
	double x = argv[3].as_float();

	double y = sin(GM_PI * x) * GM_1_PI;
	double res = a + (x + y * c) * (b - a);

	return CreateFloatValue(res);
}
value ScriptClientBase::Func_Interpolate_QuadraticBezier(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_float();
	double b = argv[1].as_float();
	double c = argv[2].as_float();
	double x = argv[3].as_float();

	double y = 1.0 - x;
	double res = (a * y * y) + x * (b * x + c * 2 * y);

	return CreateFloatValue(res);
}
value ScriptClientBase::Func_Interpolate_CubicBezier(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_float();
	double b = argv[1].as_float();
	double c1 = argv[2].as_float();
	double c2 = argv[3].as_float();
	double x = argv[4].as_float();

	double y = 1.0 - x;
	double z = y * y;
	double res = (a * y * z) + x * ((b * x * x) + (c1 * c1 * c2 * 3 * z));

	return CreateFloatValue(res);
}
value ScriptClientBase::Func_Interpolate_Hermite(script_machine* machine, int argc, const value* argv) {
	//Start and end points
	double sx = argv[0].as_float();
	double sy = argv[1].as_float();
	double ex = argv[2].as_float();
	double ey = argv[3].as_float();

	//Tangent vectors
	double vsm = argv[4].as_float();							//start magnitude
	double vsa = Math::DegreeToRadian(argv[5].as_float());	//start angle
	double vem = argv[6].as_float();							//end magnitude
	double vea = Math::DegreeToRadian(argv[7].as_float());	//end angle

	double x = argv[8].as_float();

	__m128d vec_s;		//[sin, cos]
	__m128d vec_e;		//[sin, cos]
	Math::DoSinCos(vsa, vec_s.m128d_f64);
	Math::DoSinCos(vea, vec_e.m128d_f64);
	vec_s = Vectorize::Mul(vec_s, Vectorize::Replicate(vsm));
	vec_e = Vectorize::Mul(vec_e, Vectorize::Replicate(vem));

	double x_2 = 2 * x;
	double x2 = x * x;
	double x_s1 = x - 1;
	double x_s1_2 = x_s1 * x_s1;

	double rps = (1 + x_2) * x_s1_2;	//(1 + 2t) * (1 - t)^2
	double rpe = x2 * (3 - x_2);		//t^2 * (3 - 2t)
	double rvs = x * x_s1_2;			//t * (1 - t)^2
	double rve = x2 * x_s1;				//t^2 * (t - 1)
	double res_pos[2] = {
		sx * rps + ex * rpe + vec_s.m128d_f64[1] * rvs + vec_e.m128d_f64[1] * rve,
		sy * rps + ey * rpe + vec_s.m128d_f64[0] * rvs + vec_e.m128d_f64[0] * rve
	};

	return CreateFloatArrayValue(res_pos, 2U);
}

value ScriptClientBase::Func_Interpolate_X(script_machine* machine, int argc, const value* argv) {
	double x = argv[2].as_float();

	Math::Lerp::Type type = (Math::Lerp::Type)argv[3].as_int();
	auto func = Math::Lerp::GetFunc<double, double>(type);

	return _ScriptValueLerp(machine, &argv[0], &argv[1], x, func);
}
value ScriptClientBase::Func_Interpolate_X_Packed(script_machine* machine, int argc, const value* argv) {
	int64_t a = argv[0].as_int();
	int64_t b = argv[1].as_int();
	double x = argv[2].as_float();

	Math::Lerp::Type type = (Math::Lerp::Type)argv[3].as_int();
	auto lerpFunc = Math::Lerp::GetFunc<int64_t, double>(type);

	/*
	size_t packetSize = argv[4].as_int();
	if (packetSize >= sizeof(int64_t))
		return CreateIntValue(lerpFunc(a, b, x));
	packetSize *= 8U;
	*/
	const size_t packetSize = 8U;
	const uint64_t mask = (1ui64 << packetSize) - 1;

	int64_t res = 0;
	for (size_t i = 0; i < sizeof(int64_t) * 8; i += packetSize) {
		int64_t _a = (a >> i) & mask;
		int64_t _b = (b >> i) & mask;
		if (_a == 0 && _b == 0) continue;
		int64_t tmp = lerpFunc(_a, _b, x) & mask;
		res |= tmp << i;
	}
	return CreateIntValue(res);
}
template<bool USE_RAD>
value ScriptClientBase::Func_Interpolate_X_Angle(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_float();
	double b = argv[1].as_float();
	double x = argv[2].as_float();

	Math::Lerp::Type type = (Math::Lerp::Type)argv[3].as_int();
	auto funcLerp = Math::Lerp::GetFunc<double, double>(type);
	auto funcDiff = USE_RAD ? Math::AngleDifferenceRad : Math::AngleDifferenceDeg;
	auto funcNorm = USE_RAD ? Math::NormalizeAngleRad : Math::NormalizeAngleDeg;

	b = a + funcDiff(a, b);

	return CreateFloatValue(funcNorm(funcLerp(a, b, x)));
}
// :souperdying:
value ScriptClientBase::Func_Interpolate_X_Array(script_machine* machine, int argc, const value* argv) {
	BaseFunction::_null_check(machine, argv, argc);

	const value* val = &argv[0];
	type_data* valType = val->get_type();

	if (valType->get_kind() != type_data::tk_array || val->length_as_array() == 0) {
		BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "Interpolate_X_Array");
		return value();
	}

	std::vector<value> arr = *(val->as_array_ptr());
	double x = argv[1].as_float();

	size_t len = arr.size();

	x = fmod(fmod(x, len) + len, len);
	int from = floor(x);
	int to = (from + 1) % len;
	x -= from;

    Math::Lerp::Type type = (Math::Lerp::Type)argv[2].as_int();
	auto lerpFunc =  Math::Lerp::GetFunc<double, double>(type);

	return _ScriptValueLerp(machine, &arr[from], &arr[to], x, lerpFunc);
}

value ScriptClientBase::Func_Rotate2D(script_machine* machine, int argc, const value* argv) {
	double pos[2] = { argv[0].as_float(), argv[1].as_float() };
	double ang = argv[2].as_float();

	double ox = 0, oy = 0;
	if (argc > 3) {
		ox = argv[3].as_float();
		oy = argv[4].as_float();
	}

	Math::Rotate2D(pos, Math::DegreeToRadian(ang), ox, oy);

	return CreateFloatArrayValue(pos, 2U);
}
value ScriptClientBase::Func_Rotate3D(script_machine* machine, int argc, const value* argv) {
	double x = argv[0].as_float();
	double y = argv[1].as_float();
	double z = argv[2].as_float();

	double sc_x[2];
	double sc_y[2];
	double sc_z[2];
	Math::DoSinCos(-Math::DegreeToRadian(argv[3].as_float()), sc_x);
	Math::DoSinCos(-Math::DegreeToRadian(argv[4].as_float()), sc_y);
	Math::DoSinCos(-Math::DegreeToRadian(argv[5].as_float()), sc_z);

	double ox = 0, oy = 0, oz = 0;
	if (argc > 6) {
		ox = argv[6].as_float();
		oy = argv[7].as_float();
		oz = argv[8].as_float();
	}

	double cx = sc_x[1];
	double sx = sc_x[0];
	double cy = sc_y[1];
	double sy = sc_y[0];
	double cz = sc_z[1];
	double sz = sc_z[0];
	double sx_sy = sx * sy;
	double sx_cy = sx * cy;

	double m11 = cy * cz - sx_sy * sz;
	double m12 = -cx * sz;
	double m13 = sy * cz + sx_cy * sz;
	double m21 = cy * sz + sx_sy * cz;
	double m22 = cx * cz;
	double m23 = sy * sz - sx_cy * cz;
	double m31 = -cx * sy;
	double m32 = sx;
	double m33 = cx * cy;

	x -= ox;
	y -= oy;
	z -= oz;
	double res[3] = {
		ox + (x * m11 + y * m21 + z * m31),
		oy + (x * m12 + y * m22 + z * m32),
		oz + (x * m13 + y * m23 + z * m33)
	};
	return CreateFloatArrayValue(res, 3U);
}

//組み込み関数：文字列操作
value ScriptClientBase::Func_ToString(script_machine* machine, int argc, const value* argv) {
	return CreateStringValue(argv->as_string());
}
value ScriptClientBase::Func_ItoA(script_machine* machine, int argc, const value* argv) {
	std::wstring res = std::to_wstring(argv->as_int());
	return CreateStringValue(res);
}
value ScriptClientBase::Func_RtoA(script_machine* machine, int argc, const value* argv) {
	std::wstring res = std::to_wstring(argv->as_float());
	return CreateStringValue(res);
}
value ScriptClientBase::Func_RtoS(script_machine* machine, int argc, const value* argv) {
	std::string res = "";
	std::string fmtV = StringUtility::ConvertWideToMulti(argv[0].as_string());
	double num = argv[1].as_float();

	try {
		bool bF = false;
		int countIS = 0;
		int countI0 = 0;
		int countF = 0;

		for (char ch : fmtV) {
			if (ch == '#') countIS++;
			else if (ch == '.' && bF) throw false;
			else if (ch == '.') bF = true;
			else if (ch == '0') {
				if (bF) countF++;
				else countI0++;
			}
		}

		std::string fmt = "";
		if (countI0 > 0 && countF >= 0) {
			fmt += "%0";
			fmt += StringUtility::Format("%d", countI0);
			fmt += ".";
			fmt += StringUtility::Format("%d", countF);
			fmt += "f";
		}
		else if (countIS > 0 && countF >= 0) {
			fmt += "%";
			fmt += StringUtility::Format("%d", countIS);
			fmt += ".";
			fmt += StringUtility::Format("%d", countF);
			fmt += "f";
		}

		if (fmt.size() > 0) {
			res = StringUtility::Format((char*)fmt.c_str(), num);
		}
	}
	catch (...) {
		res = "[invalid format]";
	}

	return CreateStringValue(StringUtility::ConvertMultiToWide(res));
}
value ScriptClientBase::Func_VtoS(script_machine* machine, int argc, const value* argv) {
	std::string res = "";
	std::string fmtV = StringUtility::ConvertWideToMulti(argv[0].as_string());

	try {
		int countIS = 0;
		int countI0 = 0;
		int countF = 0;

		auto _IsPattern = [](char ch) -> bool {
			return ch == 'd' || ch == 's' || ch == 'f';
		};

		int advance = 0;//0:-, 1:0, 2:num, 3:[d,s,f], 4:., 5:num
		for (char ch : fmtV) {
			if (advance == 0 && ch == '-')
				advance = 1;
			else if ((advance == 0 || advance == 1 || advance == 2) && std::isdigit(ch))
				advance = 2;
			else if (advance == 2 && _IsPattern(ch))
				advance = 4;
			else if (advance == 2 && (ch == '.'))
				advance = 5;
			else if (advance == 4 && (ch == '.'))
				advance = 5;
			else if (advance == 5 && std::isdigit(ch))
				advance = 5;
			else if (advance == 5 && _IsPattern(ch))
				advance = 6;
			else throw false;
		}

		fmtV = std::string("%") + fmtV;
		char* fmt = (char*)fmtV.c_str();
		if (strstr(fmt, "d")) {
			fmtV = StringUtility::ReplaceAll(fmtV, "d", "lld");
			res = StringUtility::Format(fmtV.c_str(), argv[1].as_int());
		}
		else if (strstr(fmt, "f"))
			res = StringUtility::Format(fmt, argv[1].as_float());
		else if (strstr(fmt, "s"))
			res = StringUtility::Format(fmt, StringUtility::ConvertWideToMulti(argv[1].as_string()).c_str());
	}
	catch (...) {
		res = "[invalid format]";
	}

	return CreateStringValue(StringUtility::ConvertMultiToWide(res));
}
value ScriptClientBase::Func_StringFormat(script_machine* machine, int argc, const value* argv) {
	std::wstring res = L"";
	
	std::wstring srcStr = argv[0].as_string();
	std::wstring fmtTypes = argv[1].as_string();

	try {
		if (fmtTypes.size() != argc - 2)
			throw L"[invalid argc]";

		std::list<std::wstring> stringCache;
		std::vector<byte> fakeVaList;
		char tmp[8]{};

		const value* pValue = &argv[2];
		size_t iMem = 0;
		for (char ch : fmtTypes) {
			size_t cpySize = 0;

			switch (ch) {
			case 'd':	//int type
				cpySize = sizeof(int);
				*reinterpret_cast<int*>(tmp) = (int)(pValue->as_int());
				break;
			case 'l':	//long int type
				cpySize = sizeof(int64_t);
				*reinterpret_cast<int64_t*>(tmp) = (int64_t)(pValue->as_int());
				break;
			case 'f':	//float type - !! VA_LIST PROMOTES FLOATS TO DOUBLES !!
				cpySize = sizeof(double);
				*reinterpret_cast<double*>(tmp) = (double)(pValue->as_float());
				break;
			case 's':	//wstring type
			{
				stringCache.push_back(pValue->as_string());
				cpySize = sizeof(wchar_t*);
				*reinterpret_cast<wchar_t**>(tmp) = (wchar_t*)(stringCache.back().data());
				break;
			}
			default:
				throw L"[invalid format]";
			}

			fakeVaList.resize(fakeVaList.size() + cpySize);
			memcpy(fakeVaList.data() + iMem, tmp, cpySize);
			iMem += cpySize;

			++pValue;
		}

		res = StringUtility::Format(srcStr.c_str(), reinterpret_cast<va_list>(fakeVaList.data()));
	}
	catch (const wchar_t* err) {
		res = err;
	}

	return CreateStringValue(res);
}
value ScriptClientBase::Func_AtoI(script_machine* machine, int argc, const value* argv) {
	std::wstring str = argv[0].as_string();
	int radix = argc > 1 ? argv[1].as_int() : 10;
	int64_t num = wcstoll(str.c_str(), nullptr, radix);
	return CreateIntValue(num);
}
value ScriptClientBase::Func_AtoR(script_machine* machine, int argc, const value* argv) {
	std::wstring str = argv->as_string();
	double num = StringUtility::ToDouble(str);
	return CreateFloatValue(num);
}
value ScriptClientBase::Func_TrimString(script_machine* machine, int argc, const value* argv) {
	std::wstring res = StringUtility::Trim(argv->as_string());
	return CreateStringValue(res);
}
value ScriptClientBase::Func_SplitString(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::vector<std::wstring> list = StringUtility::Split(argv[0].as_string(), argv[1].as_string());
	return script->CreateStringArrayValue(list);
}
value ScriptClientBase::Func_SplitString2(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::vector<std::wstring> list = StringUtility::SplitPattern(argv[0].as_string(), argv[1].as_string());
	return script->CreateStringArrayValue(list);
}

value ScriptClientBase::Func_RegexMatch(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::wstring str = argv[0].as_string();
	std::wstring pattern = argv[1].as_string();

	std::vector<std::wstring> res;

	std::wsmatch base_match;
	if (std::regex_search(str, base_match, std::wregex(pattern))) {
		for (const std::wssub_match& itr : base_match) {
			res.push_back(itr.str());
		}
	}

	return script->CreateStringArrayValue(res);
}
value ScriptClientBase::Func_RegexMatchRepeated(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::wstring str = argv[0].as_string();
	std::wstring pattern = argv[1].as_string();

	std::vector<gstd::value> valueArrayRes;
	std::vector<std::wstring> singleArray;

	std::wregex reg(pattern);

	auto itrBegin = std::wsregex_iterator(str.begin(), str.end(), reg);
	auto itrEnd = std::wsregex_iterator();

	for (size_t i = 0; itrBegin != itrEnd; ++itrBegin, ++i) {
		const std::wsmatch& match = *itrBegin;

		singleArray.clear();
		for (const std::wssub_match& itrMatch : match) {
			singleArray.push_back(itrMatch.str());
		}

		valueArrayRes.push_back(script->CreateStringArrayValue(singleArray));
	}

	return script->CreateValueArrayValue(valueArrayRes);
}
value ScriptClientBase::Func_RegexReplace(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::wstring str = argv[0].as_string();
	std::wstring pattern = argv[1].as_string();
	std::wstring replacing = argv[2].as_string();
	return script->CreateStringValue(std::regex_replace(str, std::wregex(pattern), replacing));
}

value ScriptClientBase::Func_ToDegrees(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(Math::RadianToDegree(argv->as_float()));
}
value ScriptClientBase::Func_ToRadians(script_machine* machine, int argc, const value* argv) {
	return CreateFloatValue(Math::DegreeToRadian(argv->as_float()));
}
template<bool USE_RAD>
value ScriptClientBase::Func_NormalizeAngle(script_machine* machine, int argc, const value* argv) {
	double ang = argv->as_float();
	auto func = USE_RAD ? Math::NormalizeAngleRad : Math::NormalizeAngleDeg;
	return CreateFloatValue(func(ang));
}
template<bool USE_RAD>
value ScriptClientBase::Func_AngularDistance(script_machine* machine, int argc, const value* argv) {
	double angFrom = argv[0].as_float();
	double angTo = argv[1].as_float();
	auto func = USE_RAD ? Math::AngleDifferenceRad : Math::AngleDifferenceDeg;
	return CreateFloatValue(func(angFrom, angTo));
}
template<bool USE_RAD>
value ScriptClientBase::Func_ReflectAngle(script_machine* machine, int argc, const value* argv) {
	double angRay = argv[0].as_float();
	double angSurf = argv[1].as_float();
	auto func = USE_RAD ? Math::NormalizeAngleRad : Math::NormalizeAngleDeg;
	return CreateFloatValue(func(2 * angSurf - angRay));
}

//共通関数：パス関連
value ScriptClientBase::Func_GetParentScriptDirectory(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	const std::wstring& path = script->GetEngineData()->GetPath();
	std::wstring res = PathProperty::GetFileDirectory(path);
	return script->CreateStringValue(res);
}
value ScriptClientBase::Func_GetCurrentScriptDirectory(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	int line = machine->get_current_line();
	const std::wstring& path = script->GetEngineData()->GetScriptFileLineMap()->GetPath(line);
	std::wstring res = PathProperty::GetFileDirectory(path);
	return script->CreateStringValue(res);
}
value ScriptClientBase::Func_GetFilePathList(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::wstring dir = PathProperty::GetFileDirectory(argv->as_string());
	std::vector<std::wstring> listDir = File::GetFilePathList(dir, true);
	return script->CreateStringArrayValue(listDir);
}
value ScriptClientBase::Func_GetDirectoryList(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::wstring dir = PathProperty::GetFileDirectory(argv->as_string());
	std::vector<std::wstring> listDir = File::GetDirectoryPathList(dir, true);
	return script->CreateStringArrayValue(listDir);
}

//Path utility
value ScriptClientBase::Func_GetWorkingDirectory(script_machine* machine, int argc, const value* argv) {
	wchar_t dir[_MAX_PATH];
	ZeroMemory(dir, sizeof(dir));
	::GetCurrentDirectoryW(_MAX_PATH, dir);
	return ScriptClientBase::CreateStringValue(std::wstring(dir));
}
value ScriptClientBase::Func_GetModuleName(script_machine* machine, int argc, const value* argv) {
	const std::wstring& res = PathProperty::GetModuleName();
	return ScriptClientBase::CreateStringValue(res);
}
value ScriptClientBase::Func_GetModuleDirectory(script_machine* machine, int argc, const value* argv) {
	const std::wstring& res = PathProperty::GetModuleDirectory();
	return ScriptClientBase::CreateStringValue(res);
}
value ScriptClientBase::Func_GetFileDirectory(script_machine* machine, int argc, const value* argv) {
	std::wstring res = PathProperty::GetFileDirectory(argv->as_string());
	return ScriptClientBase::CreateStringValue(res);
}
value ScriptClientBase::Func_GetFileDirectoryFromModule(script_machine* machine, int argc, const value* argv) {
	std::wstring res = PathProperty::GetDirectoryWithoutModuleDirectory(argv->as_string());
	return ScriptClientBase::CreateStringValue(res);
}
value ScriptClientBase::Func_GetFileTopDirectory(script_machine* machine, int argc, const value* argv) {
	std::wstring res = PathProperty::GetDirectoryName(argv->as_string());
	return ScriptClientBase::CreateStringValue(res);
}
value ScriptClientBase::Func_GetFileName(script_machine* machine, int argc, const value* argv) {
	std::wstring res = PathProperty::GetFileName(argv->as_string());
	return ScriptClientBase::CreateStringValue(res);
}
value ScriptClientBase::Func_GetFileNameWithoutExtension(script_machine* machine, int argc, const value* argv) {
	std::wstring res = PathProperty::GetFileNameWithoutExtension(argv->as_string());
	return ScriptClientBase::CreateStringValue(res);
}
value ScriptClientBase::Func_GetFileExtension(script_machine* machine, int argc, const value* argv) {
	std::wstring res = PathProperty::GetFileExtension(argv->as_string());
	return ScriptClientBase::CreateStringValue(res);
}
value ScriptClientBase::Func_IsFileExists(script_machine* machine, int argc, const value* argv) {
	std::wstring path = argv->as_string();
	bool res = false;
	
	if (File::IsExists(path)) {
		res = true;
	}
	else {
		path = PathProperty::GetUnique(path);

		res = FileManager::GetBase()->IsArchiveFileExists(path);
	}

	return ScriptClientBase::CreateBooleanValue(res);
}
value ScriptClientBase::Func_IsDirectoryExists(script_machine* machine, int argc, const value* argv) {
	std::wstring dir = argv->as_string();
	bool res = false;

	if (File::IsDirectory(dir)) {
		res = true;
	}
	else {
		dir = PathProperty::GetUnique(dir);

		std::wstring moduleDir = PathProperty::GetModuleDirectory();
		if (dir.find(moduleDir) != std::wstring::npos) {
			res = FileManager::GetBase()->IsArchiveDirectoryExists(dir);
		}
	}

	return ScriptClientBase::CreateBooleanValue(res);
}

//共通関数：時刻関連
value ScriptClientBase::Func_GetSystemTimeMilliS(script_machine* machine, int argc, const value* argv) {
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	return ScriptClientBase::CreateIntValue(time);
}
value ScriptClientBase::Func_GetSystemTimeNanoS(script_machine* machine, int argc, const value* argv) {
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	int64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
	return ScriptClientBase::CreateIntValue(time);
}
value ScriptClientBase::Func_GetCurrentDateTimeS(script_machine* machine, int argc, const value* argv) {
	SYSTEMTIME date;
	GetLocalTime(&date);

	std::wstring strDateTime = StringUtility::Format(
		L"%04d%02d%02d%02d%02d%02d",
		date.wYear, date.wMonth, date.wDay,
		date.wHour, date.wMinute, date.wSecond
		);

	return ScriptClientBase::CreateStringValue(strDateTime);
}

//共通関数：デバッグ関連
value ScriptClientBase::Func_WriteLog(script_machine* machine, int argc, const value* argv) {
	std::wstring msg = L"";
	for (int i = 0; i < argc; ) {
		msg += argv[i].as_string();
		if ((++i) >= argc) break;
		msg += L",";
	}
	Logger::WriteTop(ILogger::LogType::User1, msg);
	return value();
}
value ScriptClientBase::Func_RaiseError(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::wstring msg = argv->as_string();
	script->RaiseError(msg);

	return value();
}
value ScriptClientBase::Func_RaiseMessageWindow(script_machine* machine, int argc, const value* argv) {
	std::wstring title = argv[0].as_string();
	std::wstring message = argv[1].as_string();

	DWORD flags = MB_APPLMODAL | MB_OK;
	if (argc == 3) {
		int64_t userFlags = argv[2].as_int();
		//The Forbidden Flags
		userFlags = userFlags & (~MB_APPLMODAL);
		userFlags = userFlags & (~MB_SYSTEMMODAL);
		userFlags = userFlags & (~MB_TASKMODAL);
		userFlags = userFlags & (~MB_DEFAULT_DESKTOP_ONLY);
		userFlags = userFlags & (~MB_SETFOREGROUND);
		userFlags = userFlags & (~MB_SERVICE_NOTIFICATION);
		flags = MB_APPLMODAL | userFlags;
	}
	
	int res = ::MessageBoxW(nullptr, message.c_str(),
		title.c_str(), flags);
	return ScriptClientBase::CreateIntValue(res);
}

//****************************************************************************
//ScriptLoader
//****************************************************************************
ScriptLoader::ScriptLoader(ScriptClientBase* script, const std::wstring& path, std::vector<char>& source, ScriptFileLineMap* mapLine) {
	script_ = script;

	pathSource_ = path;

	src_ = source;
	scanner_.reset(new Scanner(src_));
	encoding_ = scanner_->GetEncoding();
	charSize_ = Encoding::GetCharSize(encoding_);

	mapLine_ = mapLine;
}

void ScriptLoader::_RaiseError(int line, const std::wstring& err) {
	script_->engineData_->SetSource(src_);
	script_->_RaiseError(line, err);
}
void ScriptLoader::_DumpRes() {
	return;

	static int countTest = 0;
	static std::wstring tPath = L"";
	if (tPath != pathSource_) {
		countTest = 0;
		tPath = pathSource_;
	}
	std::wstring pathTest = PathProperty::GetModuleDirectory()
		+ StringUtility::Format(L"temp\\script_%s%03d.dnh",
			PathProperty::GetFileName(pathSource_).c_str(), countTest);
	File file(pathTest);
	File::CreateFileDirectory(pathTest);
	file.Open(File::WRITEONLY);
	file.Write(&src_[0], src_.size());

	if (false) {
		std::string strNewLine = "\r\n";
		std::wstring strNewLineW = L"\r\n";
		if (encoding_ == Encoding::UTF16LE) {
			file.Write(&strNewLineW[0], strNewLine.size() * sizeof(wchar_t));
			file.Write(&strNewLineW[0], strNewLine.size() * sizeof(wchar_t));
		}
		else {
			file.Write(&strNewLine[0], strNewLine.size());
			file.Write(&strNewLine[0], strNewLine.size());
		}

		std::list<ScriptFileLineMap::Entry>& listEntry = mapLine_->GetEntryList();
		std::list<ScriptFileLineMap::Entry>::iterator itr = listEntry.begin();

		for (; itr != listEntry.end(); itr++) {
			if (encoding_ == Encoding::UTF16LE) {
				ScriptFileLineMap::Entry entry = (*itr);
				std::wstring strPath = entry.path_ + L"\r\n";
				std::wstring strLineStart = StringUtility::Format(L"  lineStart   :%4d\r\n", entry.lineStart_);
				std::wstring strLineEnd = StringUtility::Format(L"  lineEnd     :%4d\r\n", entry.lineEnd_);
				std::wstring strLineStartOrg = StringUtility::Format(L"  lineStartOrg:%4d\r\n", entry.lineStartOriginal_);
				std::wstring strLineEndOrg = StringUtility::Format(L"  lineEndOrg  :%4d\r\n", entry.lineEndOriginal_);

				file.Write(&strPath[0], strPath.size() * sizeof(wchar_t));
				file.Write(&strLineStart[0], strLineStart.size() * sizeof(wchar_t));
				file.Write(&strLineEnd[0], strLineEnd.size() * sizeof(wchar_t));
				file.Write(&strLineStartOrg[0], strLineStartOrg.size() * sizeof(wchar_t));
				file.Write(&strLineEndOrg[0], strLineEndOrg.size() * sizeof(wchar_t));
				file.Write(&strNewLineW[0], strNewLineW.size() * sizeof(wchar_t));
			}
			else {
				ScriptFileLineMap::Entry entry = (*itr);
				std::string strPath = StringUtility::ConvertWideToMulti(entry.path_) + "\r\n";
				std::string strLineStart = StringUtility::Format("  lineStart   :%4d\r\n", entry.lineStart_);
				std::string strLineEnd = StringUtility::Format("  lineEnd     :%4d\r\n", entry.lineEnd_);
				std::string strLineStartOrg = StringUtility::Format("  lineStartOrg:%4d\r\n", entry.lineStartOriginal_);
				std::string strLineEndOrg = StringUtility::Format("  lineEndOrg  :%4d\r\n", entry.lineEndOriginal_);

				file.Write(&strPath[0], strPath.size());
				file.Write(&strLineStart[0], strLineStart.size());
				file.Write(&strLineEnd[0], strLineEnd.size());
				file.Write(&strLineStartOrg[0], strLineStartOrg.size());
				file.Write(&strLineEndOrg[0], strLineEndOrg.size());
				file.Write(&strNewLine[0], strNewLine.size());
			}
		}
	}

	++countTest;
}

void ScriptLoader::_ResetScanner(size_t iniReadPos) {
	scanner_.reset(new Scanner(src_));
	scanner_->SetCurrentPointer(iniReadPos);
}
void ScriptLoader::_AssertNewline() {
	if (scanner_->HasNext() && scanner_->Next().GetType() != Token::Type::TK_NEWLINE) {
		int line = scanner_->GetCurrentLine();
		_RaiseError(line, L"A newline is required.\r\n");
	}
}
bool ScriptLoader::_SkipToNextValidLine() {
	Token* tok = &scanner_->GetToken();
	while (true) {
		if (tok->GetType() == Token::Type::TK_EOF || !scanner_->HasNext())
			return false;

		if (tok->GetType() == Token::Type::TK_NEWLINE) {
			tok = &scanner_->Next();
			break;
		}
		tok = &scanner_->Next();
	}
	return tok->GetType() != Token::Type::TK_EOF;
}

void ScriptLoader::Parse() {
	try {
		scanner_->Next();
		_ParseIfElse();
		mapLine_->AddEntry(pathSource_, 1, StringUtility::CountCharacter(src_, '\n') + 1);

		_ResetScanner(0);
		scanner_->Next();
		_ParseInclude();

		_ConvertToEncoding(Encoding::UTF16LE);

		if (false) {
			std::wstring pathTest = PathProperty::GetModuleDirectory() +
				StringUtility::Format(L"temp/script_result_%s", PathProperty::GetFileName(pathSource_).c_str());
			pathTest = PathProperty::Canonicalize(pathTest);
			pathTest = PathProperty::ReplaceYenToSlash(pathTest);

			File file(pathTest);
			File::CreateFileDirectory(pathTest);
			file.Open(File::WRITEONLY);
			file.Write(src_.data(), src_.size());
			file.Close();
		}
	}
	catch (const wexception& e) {
		int line = scanner_->GetCurrentLine();
		_RaiseError(line, e.GetErrorMessage());
	}

	for (size_t i = 0; i < charSize_; ++i)
		src_.push_back(0);
}

static inline void _CheckEnd(const unique_ptr<Scanner>& scanner) {
	if (!scanner->HasNext())
		throw wexception("Unexpected EOF while parsing script.");
}
void ScriptLoader::_ParseInclude() {
	while (true) {
		bool bReread = false;
		Token* tok = &scanner_->GetToken();
		if (tok->GetType() == Token::Type::TK_SHARP) {
			size_t posBeforeDirective = scanner_->GetCurrentPointer() - charSize_;

			_CheckEnd(scanner_);
			tok = &scanner_->Next();
			if (tok->GetType() == Token::Type::TK_ID) {
				int directiveLine = scanner_->GetCurrentLine();
				std::wstring directiveType = tok->GetElement();

				if (directiveType == L"include") {
					_CheckEnd(scanner_);
					tok = &scanner_->Next();
					std::wstring wPath = tok->GetString();

					size_t posAfterInclude = scanner_->GetCurrentPointer();
					if (scanner_->HasNext()) {
						_AssertNewline();

						_CheckEnd(scanner_);
						scanner_->Next();
					}

					//Transform a "../" or a "..\" at the start into a "./"
					if (wPath._Starts_with(L"../") || wPath._Starts_with(L"..\\"))
						wPath = L"./" + wPath;

					//Expand the relative "./" into the full path
					if (wPath.find(L".\\") != std::wstring::npos || wPath.find(L"./") != std::wstring::npos) {
						const std::wstring& linePath = mapLine_->GetPath(directiveLine);
						std::wstring tDir = PathProperty::GetFileDirectory(linePath);
						//std::string tDir = PathProperty::GetFileDirectory(pathSource);
						wPath = tDir.substr(PathProperty::GetModuleDirectory().size()) + wPath.substr(2);
					}
					wPath = PathProperty::GetModuleDirectory() + wPath;
					wPath = PathProperty::GetUnique(wPath);

					if (setIncludedPath_.find(wPath) != setIncludedPath_.end()) {
						//Logger::WriteTop(StringUtility::Format(
						//	L"Scanner: File already included, skipping. (%s)", wPath.c_str()));
						src_.erase(src_.begin() + posBeforeDirective, src_.begin() + posAfterInclude);
						_ResetScanner(posBeforeDirective);
					}
					else {
						setIncludedPath_.insert(wPath);

						std::vector<char> bufIncluding;
						{
							shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(wPath);
							if (reader == nullptr || !reader->Open()) {
								std::wstring error = StringUtility::Format(
									L"Include file is not found. [%s]\r\n", wPath.c_str());
								_RaiseError(directiveLine, error);
							}

							//Detect target encoding
							size_t targetBomSize = 0;
							Encoding::Type includeEncoding = Encoding::UTF8;
							if (reader->GetFileSize() >= 2) {
								byte data[3]{};
								reader->Read(data, 3);

								includeEncoding = Encoding::Detect((char*)data, reader->GetFileSize());
								targetBomSize = Encoding::GetBomSize(includeEncoding);

								reader->SetFilePointerBegin();
							}

							if (reader->GetFileSize() >= targetBomSize) {
								reader->Seek(targetBomSize);
								bufIncluding.resize(reader->GetFileSize() - targetBomSize); //- BOM size
								reader->Read(&bufIncluding[0], bufIncluding.size());
							}

							if (bufIncluding.size() > 0U) {
								if (includeEncoding == Encoding::UTF16LE || includeEncoding == Encoding::UTF16BE) {
									//Including UTF-16

									//Convert the including file to UTF-8
									if (encoding_ == Encoding::UTF8 || encoding_ == Encoding::UTF8BOM) {
										if (includeEncoding == Encoding::UTF16BE) {
											for (auto wItr = bufIncluding.begin(); wItr != bufIncluding.end(); wItr += 2) {
												std::swap(*wItr, *(wItr + 1));
											}
										}

										std::vector<char> mbres;
										size_t countMbRes = StringUtility::ConvertWideToMulti(
											(wchar_t*)bufIncluding.data(), bufIncluding.size() / 2U, mbres, CP_UTF8);
										if (countMbRes == 0) {
											std::wstring error = StringUtility::Format(L"Error reading include file. "
												"(%s -> UTF-8) [%s]\r\n",
												Encoding::WStringRepresentation(includeEncoding), wPath.c_str());
											_RaiseError(scanner_->GetCurrentLine(), error);
										}

										includeEncoding = encoding_;
										bufIncluding = mbres;
									}
								}
								else {
									//Including UTF-8

									//Convert the include file to UTF-16 if it's in UTF-8
									if (encoding_ == Encoding::UTF16LE || encoding_ == Encoding::UTF16BE) {
										size_t includeSize = bufIncluding.size();

										std::vector<char> wplacement;
										size_t countWRes = StringUtility::ConvertMultiToWide(bufIncluding.data(),
											includeSize, wplacement, CP_UTF8);
										if (countWRes == 0) {
											std::wstring error = StringUtility::Format(L"Error reading include file. "
												"(UTF-8 -> %s) [%s]\r\n",
												Encoding::WStringRepresentation(encoding_), wPath.c_str());
											_RaiseError(scanner_->GetCurrentLine(), error);
										}

										bufIncluding = wplacement;

										//Swap bytes for UTF-16 BE
										if (encoding_ == Encoding::UTF16BE) {
											for (auto wItr = bufIncluding.begin(); wItr != bufIncluding.end(); wItr += 2) {
												std::swap(*wItr, *(wItr + 1));
											}
										}
									}
								}
							}
						}

						{
							ScriptLoader includeLoader(script_, pathSource_, bufIncluding, mapLine_);
							includeLoader._ParseIfElse();

							std::vector<char>& bufIncludingNew = includeLoader.GetResult();

							mapLine_->AddEntry(wPath, directiveLine,
								StringUtility::CountCharacter(bufIncludingNew, '\n') + 1);
							{
								src_.erase(src_.begin() + posBeforeDirective, src_.begin() + posAfterInclude);
								src_.insert(src_.begin() + posBeforeDirective, bufIncludingNew.begin(), bufIncludingNew.end());

								_ResetScanner(posBeforeDirective);
							}
						}
					}

					_DumpRes();
					bReread = true;
				}
			}
		}
		if (bReread) {
			if (!scanner_->HasNext()) break;
			tok = &scanner_->Next();
			if (tok->GetType() == Token::Type::TK_EOF) break;
			continue;
		}
		if (!_SkipToNextValidLine()) break;
	}
}
void ScriptLoader::_ParseIfElse() {
	struct _DirectivePos {
		size_t posBefore;
		size_t posAfter;
	};

	while (true) {
		bool bReread = false;
		Token* tok = &scanner_->GetToken();
		if (tok->GetType() == Token::Type::TK_SHARP) {
			size_t posBeforeDirective = scanner_->GetCurrentPointer() - charSize_;

			_CheckEnd(scanner_);
			tok = &scanner_->Next();
			if (tok->GetType() == Token::Type::TK_ID) {
				int directiveLine = scanner_->GetCurrentLine();
				std::wstring directiveType = tok->GetElement();

				if (directiveType == L"ifdef" || directiveType == L"ifndef") {
					//TODO: Unspaghettify this code

					bool bIfdef = directiveType.size() == 5;

					_CheckEnd(scanner_);
					std::wstring macroName = scanner_->Next().GetElement();

					if (scanner_->HasNext()) {
						_AssertNewline();
						//scanner.Next();
					}

					_DirectivePos posMain = { posBeforeDirective, (size_t)scanner_->GetCurrentPointer() };
					//scanner_->SetCurrentPointer(posCurrent);

					bool bValidSkipFirst = false;
					{
						auto pMacroMap = &(script_->definedMacro_);

						auto itrFind = pMacroMap->find(macroName);
						bool bMacroDefined = pMacroMap->find(macroName) != pMacroMap->end();
						bValidSkipFirst = bMacroDefined ^ bIfdef;

						//true + true	-> false
						//true + false	-> true
						//false + true	-> true
						//false + false	-> false
					}

					bool bHasElse = false;
					_DirectivePos posElse;
					_DirectivePos posEndif;

					auto _ThrowErrorNoEndif = [&]() {
						std::wstring error = L"The #endif for this directive is missing.";
						_RaiseError(scanner_->GetCurrentLine(), error);
					};

					if (!scanner_->HasNext())
						_ThrowErrorNoEndif();
					_CheckEnd(scanner_);
					scanner_->Next();
					while (true) {
						size_t _posBefore = scanner_->GetCurrentPointer() - charSize_;
						Token& ntok = scanner_->GetToken();
						if (ntok.GetType() == Token::Type::TK_SHARP) {
							_CheckEnd(scanner_);

							size_t posCurrent = scanner_->GetCurrentPointer();
							std::wstring strNext = scanner_->Next().GetElement();

							_AssertNewline();

							if (strNext == L"else") {
								if (bHasElse) {
									std::wstring error = L"Duplicate #else directive.";
									_RaiseError(scanner_->GetCurrentLine(), error);
								}
								bHasElse = true;
								posElse = { _posBefore, (size_t)scanner_->GetCurrentPointer() };
							}
							else if (strNext == L"endif") {
								posEndif = { _posBefore, (size_t)scanner_->GetCurrentPointer() };
								break;
							}
						}
						if (!_SkipToNextValidLine())
							_ThrowErrorNoEndif();
					}

					{
						std::vector<char> survivedCode;
						if (!bValidSkipFirst) {
							size_t posEnd = bHasElse ? posElse.posBefore : posEndif.posBefore;
							survivedCode.insert(survivedCode.end(),
								src_.begin() + posMain.posAfter, src_.begin() + posEnd);
						}
						else if (bHasElse) {
							survivedCode.insert(survivedCode.end(),
								src_.begin() + posElse.posAfter, src_.begin() + posEndif.posBefore);
						}
						src_.erase(src_.begin() + posBeforeDirective, src_.begin() + posEndif.posAfter);
						src_.insert(src_.begin() + posBeforeDirective, survivedCode.begin(), survivedCode.end());

						_ResetScanner(posBeforeDirective);
					}

					_DumpRes();
					bReread = true;
				}
			}
		}
		if (bReread) {
			if (!scanner_->HasNext()) break;
			tok = &scanner_->Next();
			if (tok->GetType() == Token::Type::TK_EOF) break;
			continue;
		}
		if (!_SkipToNextValidLine()) break;
	}
}

void ScriptLoader::_ConvertToEncoding(Encoding::Type targetEncoding) {
	if (encoding_ == targetEncoding) return;

	size_t orgCharSize = Encoding::GetCharSize(encoding_);
	size_t newCharSize = Encoding::GetCharSize(targetEncoding);
	size_t orgBomSize = Encoding::GetBomSize(encoding_);
	size_t newBomSize = Encoding::GetBomSize(targetEncoding);

	std::vector<char> newSource;
	if (newBomSize > 0) {
		newSource.resize(newBomSize);
		memcpy(newSource.data(), Encoding::GetBom(targetEncoding), newBomSize);
	}
	
	if (orgCharSize == 2 && newCharSize == 1) {
		//From UTF16(LE/BE) to UTF8(BOM)

		//Skip src BOM
		std::vector<char> tmpWch(src_.begin() + orgBomSize, src_.end());
		if (encoding_ == Encoding::UTF16BE) {
			//Src is UTF16BE, swap bytes
			for (auto wItr = tmpWch.begin(); wItr != tmpWch.end(); wItr += 2) {
				std::swap(*wItr, *(wItr + 1));
			}
		}

		std::vector<char> placement;
		size_t countRes = StringUtility::ConvertWideToMulti((wchar_t*)tmpWch.data(),
			tmpWch.size(), placement, CP_UTF8);
		if (countRes == 0) {
			std::wstring error = StringUtility::Format(L"Error converting script encoding. "
				"(%s -> %s) [%s]\r\n",
				Encoding::WStringRepresentation(encoding_), 
				Encoding::WStringRepresentation(targetEncoding), pathSource_.c_str());
			_RaiseError(0, error);
		}

		newSource.insert(newSource.end(), placement.begin(), placement.end());
	}
	else if (orgCharSize == 1 && newCharSize == 2) {
		//From UTF8(BOM) to UTF16(LE/BE)

		//Skip src BOM
		std::vector<char> tmpCh(src_.begin() + orgBomSize, src_.end());

		std::vector<char> placement;
		size_t countRes = StringUtility::ConvertMultiToWide(tmpCh.data(),
			tmpCh.size(), placement, CP_UTF8);
		if (countRes == 0) {
			std::wstring error = StringUtility::Format(L"Error converting script encoding. "
				"(%s -> %s) [%s]\r\n",
				Encoding::WStringRepresentation(encoding_),
				Encoding::WStringRepresentation(targetEncoding), pathSource_.c_str());
			_RaiseError(0, error);
		}

		if (targetEncoding == Encoding::UTF16BE) {
			//Dest is UTF16BE, swap bytes
			for (auto wItr = placement.begin(); wItr != placement.end(); wItr += 2) {
				std::swap(*wItr, *(wItr + 1));
			}
		}
		newSource.insert(newSource.end(), placement.begin(), placement.end());
	}
	else {
		//From UTF8(BOM) to UTF8(BOM) or UTF16(LE/BE) to UTF16(LE/BE)

		std::vector<char> placement(src_.begin() + orgBomSize, src_.end());
		if ((encoding_ == Encoding::UTF16LE && targetEncoding == Encoding::UTF16BE)
			|| (encoding_ == Encoding::UTF16BE && targetEncoding == Encoding::UTF16LE)) 
		{
			//Swap byte order
			for (auto wItr = placement.begin(); wItr != placement.end(); wItr += 2) {
				std::swap(*wItr, *(wItr + 1));
			}
		}

		newSource.insert(newSource.end(), placement.begin(), placement.end());
	}

	src_ = newSource;
	encoding_ = targetEncoding;
	charSize_ = newCharSize;
}

//****************************************************************************
//ScriptFileLineMap
//****************************************************************************
ScriptFileLineMap::ScriptFileLineMap() {

}
ScriptFileLineMap::~ScriptFileLineMap() {

}
void ScriptFileLineMap::AddEntry(const std::wstring& path, int lineAdd, int lineCount) {
	Entry entryNew;
	entryNew.path_ = path;
	entryNew.lineStartOriginal_ = 1;
	entryNew.lineEndOriginal_ = lineCount;
	entryNew.lineStart_ = lineAdd;
	entryNew.lineEnd_ = entryNew.lineStart_ + lineCount - 1;
	if (listEntry_.size() == 0) {
		listEntry_.push_back(entryNew);
		return;
	}

	Entry* pEntryDivide = nullptr;
	std::list<Entry>::iterator itrInsert;
	for (itrInsert = listEntry_.begin(); itrInsert != listEntry_.end(); itrInsert++) {
		pEntryDivide = (Entry*)&*itrInsert;
		if (lineAdd >= pEntryDivide->lineStart_ && lineAdd <= pEntryDivide->lineEnd_) break;
	}

	Entry& entryDivide = *pEntryDivide;
	if (entryDivide.lineStart_ == lineAdd) {
		entryDivide.lineStartOriginal_++;
		listEntry_.insert(itrInsert, entryNew);
	}
	else if (entryDivide.lineEnd_ == lineAdd) {
		entryDivide.lineEnd_--;
		entryDivide.lineEndOriginal_--;

		listEntry_.insert(itrInsert, entryNew);
		itrInsert++;
	}
	else {
		Entry entryNew2 = entryDivide;
		entryDivide.lineEnd_ = lineAdd - 1;
		entryDivide.lineEndOriginal_ = lineAdd - entryDivide.lineStart_;

		entryNew2.lineStartOriginal_ = entryDivide.lineEndOriginal_ + 2;
		entryNew2.lineStart_ = entryNew.lineEnd_ + 1;
		entryNew2.lineEnd_ += lineCount - 1;

		if (itrInsert != listEntry_.end())
			itrInsert++;
		listEntry_.insert(itrInsert, entryNew);
		listEntry_.insert(itrInsert, entryNew2);
	}

	for (; itrInsert != listEntry_.end(); itrInsert++) {
		Entry& entry = *itrInsert;
		entry.lineStart_ += lineCount - 1;
		entry.lineEnd_ += lineCount - 1;
	}
}
ScriptFileLineMap::Entry* ScriptFileLineMap::GetEntry(int line) {
	Entry* res = nullptr;
	for (auto itrInsert = listEntry_.begin(); itrInsert != listEntry_.end(); itrInsert++) {
		res = &*itrInsert;
		if (line >= res->lineStart_ && line <= res->lineEnd_) break;
	}
	return res;
}
std::wstring& ScriptFileLineMap::GetPath(int line) {
	Entry* entry = GetEntry(line);
	return entry->path_;
}


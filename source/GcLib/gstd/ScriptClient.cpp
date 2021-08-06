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
	mapLine_ = new ScriptFileLineMap();
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
void ScriptEngineCache::AddCache(const std::wstring& name, ref_count_ptr<ScriptEngineData>& data) {
	cache_[name] = data;
}
void ScriptEngineCache::RemoveCache(const std::wstring& name) {
	auto itrFind = cache_.find(name);
	if (cache_.find(name) != cache_.end())
		cache_.erase(itrFind);
}
ref_count_ptr<ScriptEngineData> ScriptEngineCache::GetCache(const std::wstring& name) {
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
	{ "cossin", ScriptClientBase::Func_CosSin, 1 },
	{ "rcos", ScriptClientBase::Func_RCos, 1 },
	{ "rsin", ScriptClientBase::Func_RSin, 1 },
	{ "rtan", ScriptClientBase::Func_RTan, 1 },
	{ "rsincos", ScriptClientBase::Func_RSinCos, 1 },
	{ "rcossin", ScriptClientBase::Func_RCosSin, 1 },

	{ "sec", ScriptClientBase::Func_Sec, 1 },
	{ "csc", ScriptClientBase::Func_Csc, 1 },
	{ "cot", ScriptClientBase::Func_Cot, 1 },
	{ "rsec", ScriptClientBase::Func_RSec, 1 },
	{ "rcsc", ScriptClientBase::Func_RCsc, 1 },
	{ "rcot", ScriptClientBase::Func_RCot, 1 },

	{ "acos", ScriptClientBase::Func_Acos, 1 },
	{ "asin", ScriptClientBase::Func_Asin, 1 },
	{ "atan", ScriptClientBase::Func_Atan, 1 },
	{ "atan2", ScriptClientBase::Func_Atan2, 2 },
	{ "racos", ScriptClientBase::Func_RAcos, 1 },
	{ "rasin", ScriptClientBase::Func_RAsin, 1 },
	{ "ratan", ScriptClientBase::Func_RAtan, 1 },
	{ "ratan2", ScriptClientBase::Func_RAtan2, 2 },
	{ "asec", ScriptClientBase::Func_Asec, 1 },
	{ "acsc", ScriptClientBase::Func_Acsc, 1 },
	{ "acot", ScriptClientBase::Func_Acot, 1 },
	{ "rasec", ScriptClientBase::Func_RAsec, 1 },
	{ "racsc", ScriptClientBase::Func_RAcsc, 1 },
	{ "racot", ScriptClientBase::Func_RAcot, 1 },

	{ "cas", ScriptClientBase::Func_Cas, 1 },
	{ "rcas", ScriptClientBase::Func_RCas, 1 },

	{ "cosh", ScriptClientBase::Func_CosH, 1 },
	{ "sinh", ScriptClientBase::Func_SinH, 1 },
	{ "tanh", ScriptClientBase::Func_TanH, 1 },
	{ "acosh", ScriptClientBase::Func_AcosH, 1 },
	{ "asinh", ScriptClientBase::Func_AsinH, 1 },
	{ "atanh", ScriptClientBase::Func_AtanH, 1 },
	{ "sech", ScriptClientBase::Func_SecH, 1 },
	{ "csch", ScriptClientBase::Func_CscH, 1 },
	{ "coth", ScriptClientBase::Func_CotH, 1 },
	{ "asech", ScriptClientBase::Func_AsecH, 1 },
	{ "acsch", ScriptClientBase::Func_AcscH, 1 },
	{ "acoth", ScriptClientBase::Func_AcotH, 1 },

	//Math functions: Angles
	{ "ToDegrees", ScriptClientBase::Func_ToDegrees, 1 },
	{ "ToRadians", ScriptClientBase::Func_ToRadians, 1 },
	{ "NormalizeAngle", ScriptClientBase::Func_NormalizeAngle, 1 },
	{ "NormalizeAngleR", ScriptClientBase::Func_NormalizeAngleR, 1 },
	{ "AngularDistance", ScriptClientBase::Func_AngularDistance, 2 },
	{ "AngularDistanceR", ScriptClientBase::Func_AngularDistanceR, 2 },
	{ "ReflectAngle", ScriptClientBase::Func_ReflectAngle, 2 },
	{ "ReflectAngleR", ScriptClientBase::Func_ReflectAngleR, 2 },

	//Math functions: Extra
	{ "exp", ScriptClientBase::Func_Exp, 1 },
	{ "sqrt", ScriptClientBase::Func_Sqrt, 1 },
	{ "cbrt", ScriptClientBase::Func_Cbrt, 1 },
	{ "nroot", ScriptClientBase::Func_NRoot, 2 },
	{ "hypot", ScriptClientBase::Func_Hypot, 2 },
	{ "distance", ScriptClientBase::Func_Distance, 4 },
	{ "distancesq", ScriptClientBase::Func_DistanceSq, 4 },
	{ "dottheta", ScriptClientBase::Func_GapAngle, 4 },
	{ "rdottheta", ScriptClientBase::Func_RGapAngle, 4 },
	{ "PolarToCartesian", ScriptClientBase::Func_PolarToCartesian, 2 },
	{ "PolarToCartesianR", ScriptClientBase::Func_RPolarToCartesian, 2 },
	{ "CartesianToPolar", ScriptClientBase::Func_CartesianToPolar, 2 },
	{ "CartesianToPolarR", ScriptClientBase::Func_RCartesianToPolar, 2 },

	//Random
	{ "rand", ScriptClientBase::Func_Rand, 2 },
	{ "rand_int", ScriptClientBase::Func_RandI, 2 },
	{ "prand", ScriptClientBase::Func_RandEff, 2 },
	{ "prand_int", ScriptClientBase::Func_RandEffI, 2 },
	{ "rand_array", ScriptClientBase::Func_RandArray, 3 },
	{ "rand_int_array", ScriptClientBase::Func_RandArrayI, 3 },
	{ "prand_array", ScriptClientBase::Func_RandEffArray, 3 },
	{ "prand_int_array", ScriptClientBase::Func_RandEffArrayI, 3 },
	{ "choose", ScriptClientBase::Func_Choose, 1 },
	{ "pchoose", ScriptClientBase::Func_ChooseEff, 1 },
	{ "shuffle", ScriptClientBase::Func_Shuffle, 1 },
	{ "pshuffle", ScriptClientBase::Func_ShuffleEff, 1 },
	{ "count_rand", ScriptClientBase::Func_GetRandCount, 0 },
	{ "count_prand", ScriptClientBase::Func_GetRandEffCount, 0 },
	{ "reset_count_rand", ScriptClientBase::Func_ResetRandCount, 0 },
	{ "reset_count_prand", ScriptClientBase::Func_ResetRandEffCount, 0 },

	//Interpolation
	{ "Interpolate_Linear", ScriptClientBase::Func_Interpolate_Linear, 3 },
	{ "Interpolate_Smooth", ScriptClientBase::Func_Interpolate_Smooth, 3 },
	{ "Interpolate_Smoother", ScriptClientBase::Func_Interpolate_Smoother, 3 },
	{ "Interpolate_Accelerate", ScriptClientBase::Func_Interpolate_Accelerate, 3 },
	{ "Interpolate_Decelerate", ScriptClientBase::Func_Interpolate_Decelerate, 3 },
	{ "Interpolate_Modulate", ScriptClientBase::Func_Interpolate_Modulate, 4 },
	{ "Interpolate_Overshoot", ScriptClientBase::Func_Interpolate_Overshoot, 4 },
	{ "Interpolate_QuadraticBezier", ScriptClientBase::Func_Interpolate_QuadraticBezier, 4 },
	{ "Interpolate_CubicBezier", ScriptClientBase::Func_Interpolate_CubicBezier, 5 },
	{ "Interpolate_Hermite", ScriptClientBase::Func_Interpolate_Hermite, 9 },
	{ "Interpolate_X", ScriptClientBase::Func_Interpolate_X, 4 },
	{ "Interpolate_X_PackedInt", ScriptClientBase::Func_Interpolate_X_Packed, 4 },
    { "Interpolate_Array", ScriptClientBase::Func_Interpolate_Array, 3 },

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
	{ "ator", ScriptClientBase::Func_AtoR, 1 },
	{ "TrimString", ScriptClientBase::Func_TrimString, 1 },
	{ "SplitString", ScriptClientBase::Func_SplitString, 2 },

	{ "RegexMatch", ScriptClientBase::Func_RegexMatch, 2 },
	{ "RegexMatchRepeated", ScriptClientBase::Func_RegexMatchRepeated, 2 },
	{ "RegexReplace", ScriptClientBase::Func_RegexReplace, 3 },

	//Digit functions
	{ "DigitToArray", ScriptClientBase::Func_DigitToArray, 1 },
	{ "DigitToArray", ScriptClientBase::Func_DigitToArray, 2 }, //Overloaded
	{ "CountDigits", ScriptClientBase::Func_GetDigitCount, 1 },

	//Point lists
	{ "GetPoints_Line", ScriptClientBase::Func_GetPoints_Line, 5 },
	{ "GetPoints_Circle", ScriptClientBase::Func_GetPoints_Circle, 5 },
	{ "GetPoints_Ellipse", ScriptClientBase::Func_GetPoints_Ellipse, 7 },
	{ "GetPoints_EquidistantEllipse", ScriptClientBase::Func_GetPoints_EquidistantEllipse, 7 },
	{ "GetPoints_RegularPolygon", ScriptClientBase::Func_GetPoints_RegularPolygon, 7 },


	//Path utilities
	{ "GetParentScriptDirectory", ScriptClientBase::Func_GetParentScriptDirectory, 0 },
	{ "GetCurrentScriptDirectory", ScriptClientBase::Func_GetCurrentScriptDirectory, 0 },
	{ "GetFilePathList", ScriptClientBase::Func_GetFilePathList, 1 },
	{ "GetDirectoryList", ScriptClientBase::Func_GetDirectoryList, 1 },

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
	{ "GetSystemTimeMicroS", ScriptClientBase::Func_GetSystemTimeMicroS, 0 },
	{ "GetSystemTimeNanoS", ScriptClientBase::Func_GetSystemTimeNanoS, 0 },
	{ "GetCurrentDateTimeS", ScriptClientBase::Func_GetCurrentDateTimeS, 0 },

	//Debug stuff
	{ "WriteLog", ScriptClientBase::Func_WriteLog, -2 },		//0 fixed + ... -> 1 minimum
	{ "RaiseError", ScriptClientBase::Func_RaiseError, 1 },
	{ "RaiseMessageWindow", ScriptClientBase::Func_RaiseMessageWindow, 2 },
	{ "RaiseMessageWindow", ScriptClientBase::Func_RaiseMessageWindow, 3 },	//Overloaded

	//Common data
	{ "SetCommonData", ScriptClientBase::Func_SetCommonData, 2 },
	{ "GetCommonData", ScriptClientBase::Func_GetCommonData, 2 },
	{ "GetCommonData", ScriptClientBase::Func_GetCommonData, 1 },	//Overloaded
	{ "ClearCommonData", ScriptClientBase::Func_ClearCommonData, 0 },
	{ "DeleteCommonData", ScriptClientBase::Func_DeleteCommonData, 1 },
	{ "SetAreaCommonData", ScriptClientBase::Func_SetAreaCommonData, 3 },
	{ "GetAreaCommonData", ScriptClientBase::Func_GetAreaCommonData, 3 },
	{ "GetAreaCommonData", ScriptClientBase::Func_GetAreaCommonData, 2 },	//Overloaded
	{ "ClearAreaCommonData", ScriptClientBase::Func_ClearAreaCommonData, 1 },
	{ "DeleteAreaCommonData", ScriptClientBase::Func_DeleteAreaCommonData, 2 },
	{ "DeleteWholeAreaCommonData", ScriptClientBase::Func_DeleteWholeAreaCommonData, 1 },
	{ "CreateCommonDataArea", ScriptClientBase::Func_CreateCommonDataArea, 1 },
	{ "CopyCommonDataArea", ScriptClientBase::Func_CopyCommonDataArea, 2 },
	{ "IsCommonDataAreaExists", ScriptClientBase::Func_IsCommonDataAreaExists, 1 },
	{ "GetCommonDataAreaKeyList", ScriptClientBase::Func_GetCommonDataAreaKeyList, 0 },
	{ "GetCommonDataValueKeyList", ScriptClientBase::Func_GetCommonDataValueKeyList, 1 },

	{ "LoadCommonDataValuePointer", ScriptClientBase::Func_LoadCommonDataValuePointer, 1 },
	{ "LoadCommonDataValuePointer", ScriptClientBase::Func_LoadCommonDataValuePointer, 2 },			//Overloaded
	{ "LoadAreaCommonDataValuePointer", ScriptClientBase::Func_LoadAreaCommonDataValuePointer, 2 },
	{ "LoadAreaCommonDataValuePointer", ScriptClientBase::Func_LoadAreaCommonDataValuePointer, 3 },	//Overloaded
	{ "IsValidCommonDataValuePointer", ScriptClientBase::Func_IsValidCommonDataValuePointer, 1 },
	{ "SetCommonDataPtr", ScriptClientBase::Func_SetCommonDataPtr, 2 },
	{ "GetCommonDataPtr", ScriptClientBase::Func_GetCommonDataPtr, 1 },
	{ "GetCommonDataPtr", ScriptClientBase::Func_GetCommonDataPtr, 2 },		//Overloaded
};
static const std::vector<constant> commonConstant = {
	constant("NULL", 0i64),

	//Types for typeof and ftypeof
	constant("VAR_INT", type_data::tk_int),
	constant("VAR_REAL", type_data::tk_real),
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

script_type_manager* ScriptClientBase::pTypeManager_ = new script_type_manager();
uint64_t ScriptClientBase::randCalls_ = 0;
uint64_t ScriptClientBase::prandCalls_ = 0;
ScriptClientBase::ScriptClientBase() {
	bError_ = false;

	engine_ = new ScriptEngineData();
	machine_ = nullptr;

	mainThreadID_ = -1;
	idScript_ = ID_SCRIPT_FREE;

	//commonDataManager_.reset(new ScriptCommonDataManager());

	{
		DWORD seed = timeGetTime();

		mt_ = std::make_shared<RandProvider>();
		mt_->Initialize(seed ^ 0xc3c3c3c3);

		mtEffect_ = std::make_shared<RandProvider>();
		mtEffect_->Initialize(((seed ^ 0xf27ea021) << 11) ^ ((seed ^ 0x8b56c1b5) >> 11));
	}

	_AddFunction(&commonFunction);
	_AddConstant(&commonConstant);
	{
		definedMacro_[L"_DNH_PH3SX_"] = L"";
		definedMacro_[L"_DNH_PH3SX_ZLABEL_"] = L"";
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

	gstd::ref_count_ptr<ScriptFileLineMap> mapLine = engine_->GetScriptFileLineMap();
	ScriptFileLineMap::Entry* entry = mapLine->GetEntry(line);
	int lineOriginal = entry->lineEndOriginal_ - (entry->lineEnd_ - line);

	std::wstring fileName = PathProperty::GetFileName(entry->path_);

	std::wstring str = StringUtility::Format(L"%s\r\n%s" "\r\n[%s (main=%s)] " "line-> %d\r\n\r\n↓\r\n%s\r\n～～～",
		message.c_str(),
		entry->path_.c_str(),
		fileName.c_str(),
		PathProperty::GetFileName(engine_->GetPath()).c_str(),
		lineOriginal,
		errorPos.c_str());
	throw wexception(str);
}
void ScriptClientBase::_RaiseErrorFromEngine() {
	int line = engine_->GetEngine()->get_error_line();
	_RaiseError(line, engine_->GetEngine()->get_error_message());
}
void ScriptClientBase::_RaiseErrorFromMachine() {
	int line = machine_->get_error_line();
	_RaiseError(line, machine_->get_error_message());
}
std::wstring ScriptClientBase::_GetErrorLineSource(int line) {
	if (line == 0) return L"";

	Encoding::Type encoding = engine_->GetEncoding();
	std::vector<char>& source = engine_->GetSource();

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
std::vector<char> ScriptClientBase::_ParsePreprocessors(std::vector<char>& source) {
	std::wstring pathSource = engine_->GetPath();
	std::vector<char> res = source;
	FileManager* fileManager = FileManager::GetBase();
	std::set<std::wstring> setReadPath;

	gstd::ref_count_ptr<ScriptFileLineMap> mapLine = new ScriptFileLineMap();
	engine_->SetScriptFileLineMap(mapLine);

	mapLine->AddEntry(pathSource, 1, StringUtility::CountCharacter(source, '\n') + 1);

	//std::vector<Token> mapTokens;
	auto _AssertNewline = [&](Scanner* sc) {
		if (sc->HasNext() && sc->Next().GetType() != Token::Type::TK_NEWLINE) {
			int line = sc->GetCurrentLine();
			source = res;
			engine_->SetSource(source);

			_RaiseError(line, L"A newline is required.\r\n");
		}
	};
	struct _DirectivePos {
		size_t posBefore;
		size_t posAfter;
	};

	Encoding::Type mainEncoding = Encoding::UTF8;
	bool bEnd = false;
	while (true) {
		if (bEnd) break;
		Scanner scanner(res);
		mainEncoding = scanner.GetEncoding();
		size_t resSize = res.size();

		bEnd = true;
		while (scanner.HasNext()) {
			Token& tok = scanner.Next();
			//mapTokens.push_back(tok);
			if (tok.GetType() == Token::Type::TK_EOF) {
				break;
			}
			else if (tok.GetType() == Token::Type::TK_SHARP) {
				size_t posBeforeDirective = scanner.GetCurrentPointer() - 1;
				if (mainEncoding == Encoding::UTF16LE || mainEncoding == Encoding::UTF16BE)
					posBeforeDirective--;

				tok = scanner.Next();
				std::wstring directiveType = tok.GetElement();

				if (directiveType == L"include") {
					bEnd = false;

					size_t posCurrent = scanner.GetCurrentPointer();
					std::wstring wPath = scanner.Next().GetString();
					
					if (scanner.HasNext()) {
						int posBeforeNewLine = scanner.GetCurrentPointer();
						_AssertNewline(&scanner);
						scanner.SetCurrentPointer(posBeforeNewLine);
					}

					size_t posAfterInclude = scanner.GetCurrentPointer();
					scanner.SetCurrentPointer(posCurrent);

					//Transform a "../" or a "..\" at the start into a "./"
					if (wPath._Starts_with(L"../") || wPath._Starts_with(L"..\\"))
						wPath = L"./" + wPath;

					//Expand the relative "./" into the full path
					if (wPath.find(L".\\") != std::wstring::npos || wPath.find(L"./") != std::wstring::npos) {
						int line = scanner.GetCurrentLine();
						const std::wstring& linePath = mapLine->GetPath(line);
						std::wstring tDir = PathProperty::GetFileDirectory(linePath);
						//std::string tDir = PathProperty::GetFileDirectory(pathSource);
						wPath = tDir.substr(PathProperty::GetModuleDirectory().size()) + wPath.substr(2);
					}
					wPath = PathProperty::GetModuleDirectory() + wPath;
					wPath = PathProperty::GetUnique(wPath);

					bool bAlreadyIncluded = setReadPath.find(wPath) != setReadPath.end();
					if (bAlreadyIncluded) {
						//Logger::WriteTop(StringUtility::Format(L"Scanner: File already included, skipping. (%s)", wPath.c_str()));
						res.erase(res.begin() + posBeforeDirective, res.begin() + posAfterInclude);
						break;
					}

					std::vector<char> placement;
					{
						shared_ptr<FileReader> reader = fileManager->GetFileReader(wPath);
						if (reader == nullptr || !reader->Open()) {
							int line = scanner.GetCurrentLine();
							source = res;
							engine_->SetSource(source);

							std::wstring error = StringUtility::Format(L"Include file is not found. [%s]\r\n", wPath.c_str());
							_RaiseError(line, error);
						}

						//Detect target encoding
						size_t targetBomSize = 0;
						Encoding::Type includeEncoding = Encoding::UTF8;
						if (reader->GetFileSize() >= 2) {
							byte data[3];
							reader->Read(data, 3);

							includeEncoding = Encoding::Detect((char*)data, reader->GetFileSize());
							targetBomSize = Encoding::GetBomSize(includeEncoding);

							reader->SetFilePointerBegin();
						}

						if (includeEncoding == Encoding::UTF16LE || includeEncoding == Encoding::UTF16BE) {
							//Including UTF-16

							reader->Seek(targetBomSize);
							placement.resize(reader->GetFileSize() - targetBomSize); //- BOM size
							reader->Read(&placement[0], placement.size());

							if (placement.size() > 0U) {
								//Convert the including file to UTF-8
								if (mainEncoding == Encoding::UTF8 || mainEncoding == Encoding::UTF8BOM) {
									if (includeEncoding == Encoding::UTF16BE) {
										for (auto wItr = placement.begin(); wItr != placement.end(); wItr += 2) {
											std::swap(*wItr, *(wItr + 1));
										}
									}

									std::vector<char> mbres;
									size_t countMbRes = StringUtility::ConvertWideToMulti((wchar_t*)placement.data(),
										placement.size() / 2U, mbres, CP_UTF8);
									if (countMbRes == 0) {
										std::wstring error = StringUtility::Format(L"Error reading include file. "
											"(%s -> UTF-8) [%s]\r\n",
											includeEncoding == Encoding::UTF16LE ? L"UTF-16 LE" : L"UTF-16 BE", wPath.c_str());
										_RaiseError(scanner.GetCurrentLine(), error);
									}

									includeEncoding = mainEncoding;
									placement = mbres;
								}
							}
						}
						else {
							//Including UTF-8

							reader->Seek(targetBomSize);
							placement.resize(reader->GetFileSize() - targetBomSize);
							reader->Read(&placement[0], placement.size());

							if (placement.size() > 0U) {
								//Convert the include file to UTF-16 if it's in UTF-8
								if (mainEncoding == Encoding::UTF16LE || mainEncoding == Encoding::UTF16BE) {
									size_t placementSize = placement.size();

									std::vector<char> wplacement;
									size_t countWRes = StringUtility::ConvertMultiToWide(placement.data(),
										placementSize, wplacement, CP_UTF8);
									if (countWRes == 0) {
										std::wstring error = StringUtility::Format(L"Error reading include file. "
											"(UTF-8 -> %s) [%s]\r\n",
											mainEncoding == Encoding::UTF16LE ? L"UTF-16 LE" : L"UTF-16 BE", wPath.c_str());
										_RaiseError(scanner.GetCurrentLine(), error);
									}

									placement = wplacement;

									//Swap bytes for UTF-16 BE
									if (mainEncoding == Encoding::UTF16BE) {
										for (auto wItr = placement.begin(); wItr != placement.end(); wItr += 2) {
											std::swap(*wItr, *(wItr + 1));
										}
									}
								}
							}
						}
					}
					mapLine->AddEntry(wPath,
						scanner.GetCurrentLine(),
						StringUtility::CountCharacter(placement, '\n') + 1);

					{
						std::vector<char> newSource;
						newSource.insert(newSource.begin(), res.begin(), res.begin() + posBeforeDirective);
						newSource.insert(newSource.end(), placement.begin(), placement.end());
						newSource.insert(newSource.end(), res.begin() + posAfterInclude, res.end());

						res = newSource;
					}
					setReadPath.insert(wPath);

					if (false) {
						static int countTest = 0;
						static std::wstring tPath = L"";
						if (tPath != pathSource) {
							countTest = 0;
							tPath = pathSource;
						}
						std::wstring pathTest = PathProperty::GetModuleDirectory() + StringUtility::Format(L"temp\\script_%s%03d.txt", PathProperty::GetFileName(pathSource).c_str(), countTest);
						File file(pathTest);
						File::CreateFileDirectory(pathTest);
						file.Open(File::WRITEONLY);
						file.Write(&res[0], res.size());

						if (false) {
							std::string strNewLine = "\r\n";
							std::wstring strNewLineW = L"\r\n";
							if (mainEncoding == Encoding::UTF16LE) {
								file.Write(&strNewLineW[0], strNewLine.size() * sizeof(wchar_t));
								file.Write(&strNewLineW[0], strNewLine.size() * sizeof(wchar_t));
							}
							else {
								file.Write(&strNewLine[0], strNewLine.size());
								file.Write(&strNewLine[0], strNewLine.size());
							}

							std::list<ScriptFileLineMap::Entry>& listEntry = mapLine->GetEntryList();
							std::list<ScriptFileLineMap::Entry>::iterator itr = listEntry.begin();

							for (; itr != listEntry.end(); itr++) {
								if (mainEncoding == Encoding::UTF16LE) {
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

						countTest++;
					}
				}
				else if (directiveType == L"ifdef" || directiveType == L"ifndef") {
					//TODO: Unspaghettify this code
					
					bEnd = false;

					bool bIfdef = directiveType.size() == 5;

					size_t posCurrent = scanner.GetCurrentPointer();
					std::wstring macroName = scanner.Next().GetElement();

					if (scanner.HasNext()) {
						int posBeforeNewLine = scanner.GetCurrentPointer();
						_AssertNewline(&scanner);
						//scanner.SetCurrentPointer(posBeforeNewLine);
					}

					_DirectivePos posMain = { posBeforeDirective, (size_t)scanner.GetCurrentPointer() };
					//scanner.SetCurrentPointer(posCurrent);

					bool bValidSkipFirst = false;
					{
						auto itrFind = definedMacro_.find(macroName);
						bool bMacroDefined = definedMacro_.find(macroName) != definedMacro_.end();
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
						_RaiseError(scanner.GetCurrentLine(), error);
					};

					if (!scanner.HasNext())
						_ThrowErrorNoEndif();
					{
						bool bLoop = true;
						while (bLoop && scanner.HasNext()) {
							size_t _posBefore = scanner.GetCurrentPointer();
							Token& ntok = scanner.Next();
							switch (ntok.GetType()) {
							case Token::Type::TK_EOF:
								_ThrowErrorNoEndif();
							case Token::Type::TK_SHARP:
							{
								size_t posCurrent = scanner.GetCurrentPointer();
								std::wstring strNext = scanner.Next().GetElement();

								_AssertNewline(&scanner);

								if (strNext == L"else") {
									if (bHasElse) {
										std::wstring error = L"Duplicate #else directive.";
										_RaiseError(scanner.GetCurrentLine(), error);
									}
									bHasElse = true;
									posElse = { _posBefore, (size_t)scanner.GetCurrentPointer() };
								}
								else if (strNext == L"endif") {
									posEndif = { _posBefore, (size_t)scanner.GetCurrentPointer() };
									bLoop = false;
								}

								break;
							}
							}
						}
					}

					{
						std::vector<char> survivedCode;
						if (!bValidSkipFirst) {
							size_t posEnd = bHasElse ? posElse.posBefore : posEndif.posBefore;
							survivedCode.insert(survivedCode.end(), 
								res.begin() + posMain.posAfter, res.begin() + posEnd);
						}
						else if (bHasElse) {
							survivedCode.insert(survivedCode.end(), 
								res.begin() + posElse.posAfter, res.begin() + posEndif.posBefore);
						}
						res.erase(res.begin() + posBeforeDirective, res.begin() + posEndif.posAfter);
						res.insert(res.begin() + posBeforeDirective, survivedCode.begin(), survivedCode.end());

						scanner = Scanner(res);
						scanner.SetCurrentPointer(posBeforeDirective + survivedCode.size());
					}
				}
				/*
				else {
					std::wstring error = L"Unrecognized preprocessor directive: " + directiveType;
					_RaiseError(scanner.GetCurrentLine(), error);
				}
				*/
			}
		}

		if (false) {
			std::wstring pathTest = PathProperty::GetModuleDirectory() +
				StringUtility::Format(L"temp/script_incl_%s", PathProperty::GetFileName(pathSource).c_str());
			pathTest = PathProperty::Canonicalize(pathTest);
			pathTest = PathProperty::ReplaceYenToSlash(pathTest);

			File file(pathTest);
			File::CreateFileDirectory(pathTest);
			file.Open(File::WRITEONLY);
			file.Write(res.data(), res.size());
			file.Close();
		}
	}

	/*
	{
		std::wstring pathTest = PathProperty::GetModuleDirectory() + StringUtility::Format(L"temp\\script_token_%s.txt", PathProperty::GetFileName(pathSource).c_str());
		File file(pathTest);
		file.CreateDirectory();
		file.Create();

		for (auto& itr = mapTokens.begin(); itr != mapTokens.end(); ++itr) {
			Token& tk = *itr;
			const char*& tkStr = Token::Type_Str[(int)tk.GetType()];
			file.Write(const_cast<char*>(tkStr), strlen(tkStr));
			file.Write("\n", 1);
		}

		file.Close();
	}
	{
		std::wstring pathTest = PathProperty::GetModuleDirectory() + StringUtility::Format(L"temp\\script_map_%s.txt", PathProperty::GetFileName(pathSource).c_str());
		File file(pathTest);
		file.CreateDirectory();
		file.Create();

		std::list<ScriptFileLineMap::Entry> listEntry = mapLine->GetEntryList();
		std::list<ScriptFileLineMap::Entry>::iterator itr = listEntry.begin();

		std::string strNewLine = "\r\n";
		std::wstring strNewLineW = L"\r\n";
		if (encoding == Encoding::UTF16LE) {
			file.Write(&strNewLineW[0], strNewLine.size() * sizeof(wchar_t));
			file.Write(&strNewLineW[0], strNewLine.size() * sizeof(wchar_t));
		}
		else {
			file.Write(&strNewLine[0], strNewLine.size());
			file.Write(&strNewLine[0], strNewLine.size());
		}

		for (; itr != listEntry.end(); itr++) {
			if (encoding == Encoding::UTF16LE) {
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

		file.Close();
	}
	*/

	res.push_back(0);
	if (mainEncoding == Encoding::UTF16LE || mainEncoding == Encoding::UTF16BE) {
		res.push_back(0);
	}

	return res;
}
bool ScriptClientBase::_CreateEngine() {
	ref_count_ptr<script_engine> engine = new script_engine(engine_->GetSource(), &func_, &const_);
	engine_->SetEngine(engine);
	return !engine->get_error();
}
bool ScriptClientBase::SetSourceFromFile(std::wstring path) {
	path = PathProperty::GetUnique(path);

	if (cache_) {
		auto pCachedEngine = cache_->GetCache(path);
		if (pCachedEngine) {
			engine_ = pCachedEngine;
			return true;
		}
	}

	engine_->SetPath(path);
	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr || !reader->Open())
		throw gstd::wexception(L"SetScriptFileSource: " + ErrorUtility::GetFileNotFoundErrorMessage(path, true));

	size_t size = reader->GetFileSize();
	std::vector<char> source;
	source.resize(size);
	reader->Read(&source[0], size);
	this->SetSource(source);

	return true;
}
void ScriptClientBase::SetSource(const std::string& source) {
	std::vector<char> vect;
	vect.resize(source.size());
	memcpy(&vect[0], &source[0], source.size());
	this->SetSource(vect);
}
void ScriptClientBase::SetSource(std::vector<char>& source) {
	engine_->SetSource(source);
	gstd::ref_count_ptr<ScriptFileLineMap> mapLine = engine_->GetScriptFileLineMap();
	mapLine->AddEntry(engine_->GetPath(), 1, StringUtility::CountCharacter(source, '\n') + 1);
}
void ScriptClientBase::Compile() {
	if (engine_->GetEngine() == nullptr) {
		std::vector<char> source = _ParsePreprocessors(engine_->GetSource());
		engine_->SetSource(source);

		bool bCreateSuccess = _CreateEngine();
		if (!bCreateSuccess) {
			bError_ = true;
			_RaiseErrorFromEngine();
		}
		if (cache_ != nullptr && engine_->GetPath().size() > 0) {
			cache_->AddCache(engine_->GetPath(), engine_);
		}
	}

	machine_.reset(new script_machine(engine_->GetEngine().get()));
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
		else if (engine_->GetEngine()->get_error())
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

value ScriptClientBase::CreateStringArrayValue(std::vector<std::string>& list) {
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
value ScriptClientBase::CreateStringArrayValue(std::vector<std::wstring>& list) {
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
value ScriptClientBase::CreateValueArrayValue(std::vector<value>& list) {
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
bool ScriptClientBase::IsRealValue(value& v) {
	if (!v.has_data()) return false;
	return v.get_type() == script_type_manager::get_real_type();
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
bool ScriptClientBase::IsRealArrayValue(value& v) {
	if (!v.has_data()) return false;
	return v.get_type() == script_type_manager::get_real_array_type();
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
	const std::wstring& pathScript = GetEngine()->GetScriptFileLineMap()->GetPath(line);

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

//組み込み関数：数学系
value ScriptClientBase::Func_Min(script_machine* machine, int argc, const value* argv) {
	double v1 = argv[0].as_real();
	double v2 = argv[1].as_real();
	return CreateRealValue(std::min(v1, v2));
}
value ScriptClientBase::Func_Max(script_machine* machine, int argc, const value* argv) {
	double v1 = argv[0].as_real();
	double v2 = argv[1].as_real();
	return CreateRealValue(std::max(v1, v2));
}
value ScriptClientBase::Func_Clamp(script_machine* machine, int argc, const value* argv) {
	double v = argv[0].as_real();
	double bound_lower = argv[1].as_real();
	double bound_upper = argv[2].as_real();
	//if (bound_lower > bound_upper) std::swap(bound_lower, bound_upper);
	double res = std::clamp(v, bound_lower, bound_upper);
	return CreateRealValue(res);
}
value ScriptClientBase::Func_Log(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(log(argv->as_real()));
}
value ScriptClientBase::Func_Log2(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(log2(argv->as_real()));
}
value ScriptClientBase::Func_Log10(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(log10(argv->as_real()));
}
value ScriptClientBase::Func_LogN(script_machine* machine, int argc, const value* argv) {
	double x = argv[0].as_real();
	double base = argv[1].as_real();
	return CreateRealValue(log(x) / log(base));
}
value ScriptClientBase::Func_ErF(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(erf(argv->as_real()));
}
value ScriptClientBase::Func_Gamma(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(tgamma(argv->as_real()));
}

value ScriptClientBase::Func_Cos(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(cos(Math::DegreeToRadian(argv->as_real())));
}
value ScriptClientBase::Func_Sin(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(sin(Math::DegreeToRadian(argv->as_real())));
}
value ScriptClientBase::Func_Tan(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(tan(Math::DegreeToRadian(argv->as_real())));
}
value ScriptClientBase::Func_SinCos(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);

	double scArray[2];
	Math::DoSinCos(Math::DegreeToRadian(argv->as_real()), scArray);

	return script->CreateRealArrayValue(scArray, 2U);
}
value ScriptClientBase::Func_CosSin(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);

	double scArray[2];
	Math::DoSinCos(-Math::DegreeToRadian(argv->as_real()) + GM_PI_2, scArray);

	return script->CreateRealArrayValue(scArray, 2U);
}
value ScriptClientBase::Func_RCos(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(cos(argv->as_real()));
}
value ScriptClientBase::Func_RSin(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(sin(argv->as_real()));
}
value ScriptClientBase::Func_RTan(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(tan(argv->as_real()));
}
value ScriptClientBase::Func_RSinCos(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);

	double scArray[2];
	Math::DoSinCos(argv->as_real(), scArray);

	return script->CreateRealArrayValue(scArray, 2U);
}
value ScriptClientBase::Func_RCosSin(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);

	double scArray[2];
	Math::DoSinCos(-(argv->as_real()) + GM_PI_2, scArray);

	return script->CreateRealArrayValue(scArray, 2U);
}

value ScriptClientBase::Func_Sec(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(1 / cos(Math::DegreeToRadian(argv->as_real())));
}
value ScriptClientBase::Func_Csc(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(1 / sin(Math::DegreeToRadian(argv->as_real())));
}
value ScriptClientBase::Func_Cot(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(1 / tan(Math::DegreeToRadian(argv->as_real())));
}
value ScriptClientBase::Func_RSec(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(1 / cos(argv->as_real()));
}
value ScriptClientBase::Func_RCsc(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(1 / sin(argv->as_real()));
}
value ScriptClientBase::Func_RCot(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(1 / tan(argv->as_real()));
}

value ScriptClientBase::Func_Acos(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(acos(argv->as_real())));
}
value ScriptClientBase::Func_Asin(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(asin(argv->as_real())));
}
value ScriptClientBase::Func_Atan(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(atan(argv->as_real())));
}
value ScriptClientBase::Func_Atan2(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(atan2(argv[0].as_real(), argv[1].as_real())));
}
value ScriptClientBase::Func_RAcos(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(acos(argv->as_real()));
}
value ScriptClientBase::Func_RAsin(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(asin(argv->as_real()));
}
value ScriptClientBase::Func_RAtan(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(atan(argv->as_real()));
}
value ScriptClientBase::Func_RAtan2(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(atan2(argv[0].as_real(), argv[1].as_real()));
}

value ScriptClientBase::Func_Asec(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(acos(1 / argv->as_real())));
}
value ScriptClientBase::Func_Acsc(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(asin(1 / argv->as_real())));
}
value ScriptClientBase::Func_Acot(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(atan(1 / argv->as_real())));
}
value ScriptClientBase::Func_RAsec(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(acos(1 / argv->as_real()));
}
value ScriptClientBase::Func_RAcsc(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(asin(1 / argv->as_real()));
}
value ScriptClientBase::Func_RAcot(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(atan(1 / argv->as_real()));
}

value ScriptClientBase::Func_Cas(script_machine* machine, int argc, const value* argv) {
	double scArray[2];
	Math::DoSinCos(Math::RadianToDegree(argv->as_real()), scArray);
	return CreateRealValue(scArray[0] + scArray[1]);
}
value ScriptClientBase::Func_RCas(script_machine* machine, int argc, const value* argv) {
	double scArray[2];
	Math::DoSinCos(argv->as_real(), scArray);
	return CreateRealValue(scArray[0] + scArray[1]);
}

value ScriptClientBase::Func_CosH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(cosh(argv->as_real()));
}
value ScriptClientBase::Func_SinH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(sinh(argv->as_real()));
}
value ScriptClientBase::Func_TanH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(tanh(argv->as_real()));
}
value ScriptClientBase::Func_AcosH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(acosh(argv->as_real()));
}
value ScriptClientBase::Func_AsinH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(asinh(argv->as_real()));
}
value ScriptClientBase::Func_AtanH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(atanh(argv->as_real()));
}
value ScriptClientBase::Func_SecH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(1 / cosh(argv->as_real()));
}
value ScriptClientBase::Func_CscH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(1 / sinh(argv->as_real()));
}
value ScriptClientBase::Func_CotH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(1 / tanh(argv->as_real()));
}
value ScriptClientBase::Func_AsecH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(acosh(1 / argv->as_real()));
}
value ScriptClientBase::Func_AcscH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(asinh(1 / argv->as_real()));
}
value ScriptClientBase::Func_AcotH(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(atanh(1 / argv->as_real()));
}

value ScriptClientBase::Func_Exp(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(exp(argv->as_real()));
}
value ScriptClientBase::Func_Sqrt(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(sqrt(argv->as_real()));
}
value ScriptClientBase::Func_Cbrt(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(cbrt(argv->as_real()));
}
value ScriptClientBase::Func_NRoot(script_machine* machine, int argc, const value* argv) {
	double val = 1.0;
	double _p = argv[1].as_real();
	if (_p != 0.0) val = pow(argv[0].as_real(), 1.0 / _p);
	return CreateRealValue(val);
}
value ScriptClientBase::Func_Hypot(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(hypot(argv[0].as_real(), argv[1].as_real()));
}
value ScriptClientBase::Func_Distance(script_machine* machine, int argc, const value* argv) {
	double dx = argv[2].as_real() - argv[0].as_real();
	double dy = argv[3].as_real() - argv[1].as_real();
	return CreateRealValue(hypot(dx, dy));
}
value ScriptClientBase::Func_DistanceSq(script_machine* machine, int argc, const value* argv) {
	double dx = argv[2].as_real() - argv[0].as_real();
	double dy = argv[3].as_real() - argv[1].as_real();
	return CreateRealValue(Math::HypotSq<double>(dx, dy));
}
value ScriptClientBase::Func_GapAngle(script_machine* machine, int argc, const value* argv) {
	double dx = argv[2].as_real() - argv[0].as_real();
	double dy = argv[3].as_real() - argv[1].as_real();
	double res = atan2(dy, dx);
	return CreateRealValue(Math::RadianToDegree(res));
}
value ScriptClientBase::Func_RGapAngle(script_machine* machine, int argc, const value* argv) {
	double dx = argv[2].as_real() - argv[0].as_real();
	double dy = argv[3].as_real() - argv[1].as_real();
	double res = atan2(dy, dx);
	return CreateRealValue(res);
}

value ScriptClientBase::Func_PolarToCartesian(script_machine* machine, int argc, const value* argv) {
	double scArray[2];
	double rad = argv[0].as_real();
	double dir = Math::DegreeToRadian(argv[1].as_real());
	Math::DoSinCos(dir, scArray);
	double res[]{ rad * scArray[1], rad * scArray[0] };
	return CreateRealArrayValue(res, 2U);
}
value ScriptClientBase::Func_RPolarToCartesian(script_machine* machine, int argc, const value* argv) {
	double scArray[2];
	double rad = argv[0].as_real();
	double dir = argv[1].as_real();
	Math::DoSinCos(dir, scArray);
	double res[]{ rad * scArray[1], rad * scArray[0] };
	return CreateRealArrayValue(res, 2U);
}
value ScriptClientBase::Func_CartesianToPolar(script_machine* machine, int argc, const value* argv) {
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	double res[]{ hypot(x, y), Math::RadianToDegree(atan2(y, x)) };
	return CreateRealArrayValue(res, 2U);
}
value ScriptClientBase::Func_RCartesianToPolar(script_machine* machine, int argc, const value* argv) {
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	double res[]{ hypot(x, y), atan2(y, x) };
	return CreateRealArrayValue(res, 2U);
}

value ScriptClientBase::Func_Rand(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	script->CheckRunInMainThread();
	++randCalls_;
	double min = argv[0].as_real();
	double max = argv[1].as_real();
	return script->CreateRealValue(script->mt_->GetReal(min, max));
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
	double min = argv[0].as_real();
	double max = argv[1].as_real();
	double res = script->mtEffect_->GetReal(min, max);
	return script->CreateRealValue(res);
}
value ScriptClientBase::Func_RandEffI(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	++prandCalls_;
	double min = argv[0].as_int();
	double max = argv[1].as_int() + 0.9999999;
	return script->CreateIntValue(script->mtEffect_->GetReal(min, max));
}

value ScriptClientBase::Func_RandArray(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	script->CheckRunInMainThread();

	size_t size = argv[0].as_int();
	double min = argv[1].as_real();
	double max = argv[2].as_real();
	value res;
	std::vector<value> resArr;

	type_data* type = script_type_manager::get_real_type();

	for (int i = 0; i < size; ++i) {
		++randCalls_;
		resArr.push_back(value(type, script->mt_->GetReal(min, max)));
	}

	res.reset(script_type_manager::get_real_array_type(), resArr);
	return res;
}
value ScriptClientBase::Func_RandEffArray(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	script->CheckRunInMainThread();

	size_t size = argv[0].as_int();
	double min = argv[1].as_real();
	double max = argv[2].as_real();
	value res;
	std::vector<value> resArr;

	type_data* type = script_type_manager::get_real_type();

	for (int i = 0; i < size; ++i) {
		++prandCalls_;
		resArr.push_back(value(type, script->mtEffect_->GetReal(min, max)));
	}

	res.reset(script_type_manager::get_real_array_type(), resArr);
	return res;
}

value ScriptClientBase::Func_RandArrayI(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	script->CheckRunInMainThread();

	size_t size = argv[0].as_int();
	double min = argv[1].as_int();
	double max = argv[2].as_int() + 0.9999999;
	value res;
	std::vector<value> resArr;

	type_data* type = script_type_manager::get_int_type();

	for (int i = 0; i < size; ++i) {
		++randCalls_;
		resArr.push_back(value(type, (int64_t)script->mt_->GetReal(min, max))); // Why is this necessary :dying:
	}

	res.reset(script_type_manager::get_int_array_type(), resArr);
	return res;
}
value ScriptClientBase::Func_RandEffArrayI(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	script->CheckRunInMainThread();

	size_t size = argv[0].as_int();
	double min = argv[1].as_int();
	double max = argv[2].as_int() + 0.9999999;
	value res;
	std::vector<value> resArr;

	type_data* type = script_type_manager::get_int_type();

	for (int i = 0; i < size; ++i) {
		++prandCalls_;
		resArr.push_back(value(type, (int64_t)script->mtEffect_->GetReal(min, max)));
	}

	res.reset(script_type_manager::get_int_array_type(), resArr);
	return res;
}

value ScriptClientBase::Func_Choose(script_machine* machine, int argc, const value* argv) {
	BaseFunction::_null_check(machine, argv, argc);

	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	script->CheckRunInMainThread();

	const value* val = &argv[0];
	type_data* valType = val->get_type();

	if (valType->get_kind() != type_data::tk_array) {
		BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "choose");
		return value();
	}

	size_t size = val->length_as_array() - 1;
	int64_t index = script->mt_->GetReal(0, size + 0.9999999);
	++randCalls_;

	return val->index_as_array(index);
}
value ScriptClientBase::Func_ChooseEff(script_machine* machine, int argc, const value* argv) {
	BaseFunction::_null_check(machine, argv, argc);

	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);

	const value* val = &argv[0];
	type_data* valType = val->get_type();

	if (valType->get_kind() != type_data::tk_array) {
		BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "pchoose");
		return value();
	}

	size_t size = val->length_as_array() - 1;
	int64_t index = script->mtEffect_->GetReal(0, size + 0.9999999);
	++prandCalls_;

	return val->index_as_array(index);
}
value ScriptClientBase::Func_Shuffle(script_machine* machine, int argc, const value* argv) {
	BaseFunction::_null_check(machine, argv, argc);

	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	script->CheckRunInMainThread();

	const value* val = &argv[0];
	type_data* valType = val->get_type();

	if (valType->get_kind() != type_data::tk_array) {
		BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "shuffle");
		return value();
	}

	size_t size = val->length_as_array();

	value res = *val;
	res.make_unique();

	std::vector<value> arrVal;
	std::vector<value> arrOld(size);

	for (size_t i = 0; i < size; ++i)
		arrOld[i] = val->index_as_array(i);

	for (size_t i = 0; i < size; ++i) {
		int64_t index = 0;
		if (i < size - 1) {
			index = script->mt_->GetReal(0, size - 0.0000001 - i);
			++randCalls_;
		}
		arrVal.push_back(arrOld[index]);
		arrOld.erase(arrOld.begin() + index);
	}

	

	res.reset(valType, arrVal);
	return res;
}
value ScriptClientBase::Func_ShuffleEff(script_machine* machine, int argc, const value* argv) {
	BaseFunction::_null_check(machine, argv, argc);

	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);

	const value* val = &argv[0];
	type_data* valType = val->get_type();

	if (valType->get_kind() != type_data::tk_array) {
		BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "pshuffle");
		return value();
	}

	size_t size = val->length_as_array();

	value res = *val;
	res.make_unique();

	std::vector<value> arrVal;
	std::vector<value> arrOld(size);

	for (size_t i = 0; i < size; ++i)
		arrOld[i] = val->index_as_array(i);

	for (size_t i = 0; i < size; ++i) {
		int64_t index = 0;
		if (i < size - 1) {
			index = script->mtEffect_->GetReal(0, size - 0.0000001 - i);
			++prandCalls_;
		}
		arrVal.push_back(arrOld[index]);
		arrOld.erase(arrOld.begin() + index);
	}

	res.reset(valType, arrVal);
	return res;
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
		if (v1->length_as_array() != v2->length_as_array()) {
			std::string err = StringUtility::Format("Sizes must be the same when interpolating arrays. (%u and %u)",
				v1->length_as_array(), v2->length_as_array());
			machine->raise_error(err);
		}

		std::vector<value> resArr;
		resArr.resize(v1->length_as_array());
		for (size_t i = 0; i < v1->length_as_array(); ++i) {
			const value* a1 = &v1->index_as_array(i);
			resArr[i] = _ScriptValueLerp(machine, a1, &v2->index_as_array(i), vx, lerpFunc);
		}

		value res;
		res.reset(v1->get_type(), resArr);
		return res;
	}
	else {
		return ScriptClientBase::CreateRealValue(lerpFunc(v1->as_real(), v2->as_real(), vx));
	}
}
value ScriptClientBase::Func_Interpolate_Linear(script_machine* machine, int argc, const value* argv) {
	double x = argv[2].as_real();
	return _ScriptValueLerp(machine, &argv[0], &argv[1], x, Math::Lerp::Linear);
}
value ScriptClientBase::Func_Interpolate_Smooth(script_machine* machine, int argc, const value* argv) {
	double x = argv[2].as_real();
	return _ScriptValueLerp(machine, &argv[0], &argv[1], x, Math::Lerp::Smooth);
}
value ScriptClientBase::Func_Interpolate_Smoother(script_machine* machine, int argc, const value* argv) {
	double x = argv[2].as_real();
	return _ScriptValueLerp(machine, &argv[0], &argv[1], x, Math::Lerp::Smoother);
}
value ScriptClientBase::Func_Interpolate_Accelerate(script_machine* machine, int argc, const value* argv) {
	double x = argv[2].as_real();
	return _ScriptValueLerp(machine, &argv[0], &argv[1], x, Math::Lerp::Accelerate);
}
value ScriptClientBase::Func_Interpolate_Decelerate(script_machine* machine, int argc, const value* argv) {
	double x = argv[2].as_real();
	return _ScriptValueLerp(machine, &argv[0], &argv[1], x, Math::Lerp::Decelerate);
}
value ScriptClientBase::Func_Interpolate_Modulate(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_real();
	double b = argv[1].as_real();
	double c = argv[2].as_real();
	double x = argv[3].as_real();

	double y = sin(GM_PI_X2 * x) * GM_1_PI * 0.5;
	double res = a + (x + y * c) * (b - a);

	return CreateRealValue(res);
}
value ScriptClientBase::Func_Interpolate_Overshoot(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_real();
	double b = argv[1].as_real();
	double c = argv[2].as_real();
	double x = argv[3].as_real();

	double y = sin(GM_PI * x) * GM_1_PI;
	double res = a + (x + y * c) * (b - a);

	return CreateRealValue(res);
}
value ScriptClientBase::Func_Interpolate_QuadraticBezier(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_real();
	double b = argv[1].as_real();
	double c = argv[2].as_real();
	double x = argv[3].as_real();

	double y = 1.0 - x;
	double res = (a * y * y) + x * (b * x + c * 2 * y);

	return CreateRealValue(res);
}
value ScriptClientBase::Func_Interpolate_CubicBezier(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_real();
	double b = argv[1].as_real();
	double c1 = argv[2].as_real();
	double c2 = argv[3].as_real();
	double x = argv[4].as_real();

	double y = 1.0 - x;
	double z = y * y;
	double res = (a * y * z) + x * ((b * x * x) + (c1 * c1 * c2 * 3 * z));

	return CreateRealValue(res);
}
value ScriptClientBase::Func_Interpolate_Hermite(script_machine* machine, int argc, const value* argv) {
	//Start and end points
	double sx = argv[0].as_real();
	double sy = argv[1].as_real();
	double ex = argv[2].as_real();
	double ey = argv[3].as_real();

	//Tangent vectors
	double vsm = argv[4].as_real();							//start magnitude
	double vsa = Math::DegreeToRadian(argv[5].as_real());	//start angle
	double vem = argv[6].as_real();							//end magnitude
	double vea = Math::DegreeToRadian(argv[7].as_real());	//end angle

	double x = argv[8].as_real();

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

	return CreateRealArrayValue(res_pos, 2U);
}

value ScriptClientBase::Func_Interpolate_X(script_machine* machine, int argc, const value* argv) {
	double x = argv[2].as_real();

	Math::Lerp::Type type = (Math::Lerp::Type)argv[3].as_int();
	auto func = Math::Lerp::GetFunc<double, double>(type);

	return _ScriptValueLerp(machine, &argv[0], &argv[1], x, func);
}
value ScriptClientBase::Func_Interpolate_X_Packed(script_machine* machine, int argc, const value* argv) {
	int64_t a = argv[0].as_int();
	int64_t b = argv[1].as_int();
	double x = argv[2].as_real();

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
// :souperdying:
value ScriptClientBase::Func_Interpolate_Array(script_machine* machine, int argc, const value* argv) { 
	double x = argv[1].as_real();

	size_t len = argv[0].length_as_array();
	int from = (int)floor(x);

	while (from < 0) {
		x += len;
		from += len;
	}
	while (from >= len) {
		x -= len;
		from -= len;
	}
	

	int to = from + 1;
	if (to >= len) to -= len;
	double x2 = x - from;

    Math::Lerp::Type type = (Math::Lerp::Type)argv[2].as_int();
	auto lerpFunc =  Math::Lerp::GetFunc<double, double>(type);

	return ScriptClientBase::CreateRealValue(lerpFunc(argv[0].index_as_array(from).as_real(), argv[0].index_as_array(to).as_real(), x2));
}

value ScriptClientBase::Func_Rotate2D(script_machine* machine, int argc, const value* argv) {
	double pos[2] = { argv[0].as_real(), argv[1].as_real() };
	double ang = argv[2].as_real();

	double ox = 0, oy = 0;
	if (argc > 3) {
		ox = argv[3].as_real();
		oy = argv[4].as_real();
	}

	Math::Rotate2D(pos, Math::DegreeToRadian(ang), ox, oy);

	return CreateRealArrayValue(pos, 2U);
}
value ScriptClientBase::Func_Rotate3D(script_machine* machine, int argc, const value* argv) {
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	double z = argv[2].as_real();

	double sc_x[2];
	double sc_y[2];
	double sc_z[2];
	Math::DoSinCos(-Math::DegreeToRadian(argv[3].as_real()), sc_x);
	Math::DoSinCos(-Math::DegreeToRadian(argv[4].as_real()), sc_y);
	Math::DoSinCos(-Math::DegreeToRadian(argv[5].as_real()), sc_z);

	double ox = 0, oy = 0, oz = 0;
	if (argc > 6) {
		ox = argv[6].as_real();
		oy = argv[7].as_real();
		oz = argv[8].as_real();
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
	return CreateRealArrayValue(res, 3U);
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
	std::wstring res = std::to_wstring(argv->as_real());
	return CreateStringValue(res);
}
value ScriptClientBase::Func_RtoS(script_machine* machine, int argc, const value* argv) {
	std::string res = "";
	std::string fmtV = StringUtility::ConvertWideToMulti(argv[0].as_string());
	double num = argv[1].as_real();

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
			res = StringUtility::Format(fmt, argv[1].as_real());
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
		char tmp[8];

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
				*reinterpret_cast<double*>(tmp) = (double)(pValue->as_real());
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
	std::wstring str = argv->as_string();
	int64_t num = wcstoll(str.c_str(), nullptr, 10);
	return CreateIntValue(num);
}
value ScriptClientBase::Func_AtoR(script_machine* machine, int argc, const value* argv) {
	std::wstring str = argv->as_string();
	double num = StringUtility::ToDouble(str);
	return CreateRealValue(num);
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

	valueArrayRes.resize(std::distance(itrBegin, itrEnd));
	for (size_t i = 0; itrBegin != itrEnd; ++itrBegin, ++i) {
		const std::wsmatch& match = *itrBegin;
		singleArray.clear();
		for (const std::wssub_match& itrMatch : match) {
			singleArray.push_back(itrMatch.str());
		}
		valueArrayRes[i] = script->CreateStringArrayValue(singleArray);
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
value ScriptClientBase::Func_DigitToArray(script_machine* machine, int argc, const value* argv) {
	// mkm why didn't you just hardcode this
	int64_t input = abs(argv[0].as_int()); // Just forget about negatives tbh
	std::vector<size_t> res;

	int64_t length = 0;
	if (argc == 2) {
		length = argv[1].as_int();
		if (length < 0) {
			std::string error = "Length cannot be negative.";
			machine->raise_error(error);
			return value();
		}
	}

	// length applies to leading digits, not trailing ones. What should I do about overflow though :thinkies:
	for (size_t i = 0; (argc == 1 && input != 0) || (argc == 2 && i < length); ++i) {
		res.insert(res.begin(), input % 10);
		input /= 10;
	}

	return CreateIntArrayValue(res);
}
value ScriptClientBase::Func_GetDigitCount(script_machine* machine, int argc, const value* argv) {
	int64_t input = argv[0].as_int();
	size_t res = 0;

	for (size_t i = 0; input != 0; ++i) {
		res++;
		input /= 10;
	}

	return CreateIntValue(res);
}

//Point lists
// Args: x1, y1, x2, y2, midpoints
value ScriptClientBase::Func_GetPoints_Line(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	double x1 = argv[0].as_real();
	double y1 = argv[1].as_real();
	double x2 = argv[2].as_real();
	double y2 = argv[3].as_real();
	size_t mid = std::max(argv[4].as_int(), 0i64);

	double gap = 1.0 / (mid + 1.0);

	value res;
	std::vector<value> arr;

	for (size_t i = 0; i < 2 + mid; ++i) {
		double xy[2] = { Math::Lerp::Linear(x1, x2, i * gap), Math::Lerp::Linear(y1, y2, i * gap) };
		value v = script->CreateRealArrayValue(xy, 2);
		arr.push_back(v);
	}

	return script->CreateValueArrayValue(arr);
}

// Args: x, y, count, radius, angle
value ScriptClientBase::Func_GetPoints_Circle(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	size_t cnt = std::max(argv[2].as_int(), 0i64);
	double rad = argv[3].as_real();
	double dir = Math::DegreeToRadian(argv[4].as_real());
	

	double gap = GM_PI_X2 / cnt;

	double sc[2];

	std::vector<value> arr;

	for (size_t i = 0; i < cnt; ++i) {
		Math::DoSinCos(dir + i * gap, sc);
		double xy[2] = { x + sc[1] * rad, y + sc[0] * rad };
		value v = script->CreateRealArrayValue(xy, 2);
		arr.push_back(v);
	}

	return script->CreateValueArrayValue(arr);
}

// Args: x, y, count, x rad, y rad, offset angle, rotation angle
value ScriptClientBase::Func_GetPoints_Ellipse(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	size_t cnt = std::max(argv[2].as_int(), 0i64);
	double rx = argv[3].as_real();
	double ry = argv[4].as_real();
	double a1 = Math::DegreeToRadian(argv[5].as_real());
	double a2 = Math::DegreeToRadian(argv[6].as_real());

	double gap = GM_PI_X2 / cnt;

	double sc1[2];
	double sc2[2];

	Math::DoSinCos(a2, sc2);

	std::vector<value> arr;

	for (size_t i = 0; i < cnt; ++i) {
		Math::DoSinCos(a1 + i * gap, sc1);
		double _xy[2] = { sc1[1] * rx, sc1[0] * ry };

		double xy[2] = {
			x + _xy[0] * sc2[1] - _xy[1] * sc2[0],
			y + _xy[0] * sc2[0] + _xy[1] * sc2[1]
		};

		value v = script->CreateRealArrayValue(xy, 2);
		arr.push_back(v);
	}

	return script->CreateValueArrayValue(arr);
}

// Args: x, y, count, x rad, y rad, offset angle, rotation angle
value ScriptClientBase::Func_GetPoints_EquidistantEllipse(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	size_t cnt = std::max(argv[2].as_int(), 0i64);
	double rx = argv[3].as_real();
	double ry = argv[4].as_real();
	double a1 = Math::DegreeToRadian(argv[5].as_real());
	double a2 = Math::DegreeToRadian(argv[6].as_real());

	std::vector<std::vector<double>> points;

	double sc1[2];
	double sc2[2];

	double theta = a1;
	double delta = 1 / 360.0;
	int microIntegrals = floor(GM_PI_X2 / delta + 0.5);
	double circ = 0;

	for (int i = 0; i < microIntegrals; ++i) {
		theta += delta;
		Math::DoSinCos(theta, sc1);
		circ += sqrt((rx * sc1[0]) * (rx * sc1[0]) + (ry * sc1[1]) * (ry * sc1[1]));
	}

	double next = 0;
	double run = 0;

	for (int i = 0; i < microIntegrals; ++i) {
		theta += delta;
		Math::DoSinCos(theta, sc1);
		if (floor(cnt * run / circ) >= next) {
			std::vector<double>vec{ rx * sc1[1], ry * sc1[0] };
			points.push_back(vec);
			next++;
			// theta += skip;
		}

		run += sqrt((rx * sc1[0]) * (rx * sc1[0]) + (ry * sc1[1]) * (ry * sc1[1]));
	}
	

	Math::DoSinCos(a2, sc2);

	std::vector<value> arr;

	for (size_t i = 0; i < cnt; ++i) {
		std::vector<double>& point = points[i];

		double xy[2] = {
			x + point[0] * sc2[1] - point[1] * sc2[0],
			y + point[0] * sc2[0] + point[1] * sc2[1]
		};

		value v = script->CreateRealArrayValue(xy, 2);
		arr.push_back(v);
	}

	return script->CreateValueArrayValue(arr);
}

// Args: x, y, sides, side skip, midpoints, radius, angle
value ScriptClientBase::Func_GetPoints_RegularPolygon(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	size_t side = std::max(argv[2].as_int(), 0i64);
	size_t skip = std::max(argv[3].as_int(), 0i64);
	size_t mid = std::max(argv[4].as_int(), 0i64);
	double rad = argv[5].as_real();
	double dir = Math::DegreeToRadian(argv[6].as_real());

	double gapS = GM_PI_X2 / side * skip;
	double gapM = 1.0 / (mid + 1);

	double sc[2];

	std::vector<value> arr;
	std::vector<std::vector<double>> pts;

	for (size_t i = 0; i < side; ++i) {
		Math::DoSinCos(dir + i * gapS, sc);
		std::vector vec{ x + sc[1] * rad, y + sc[0] * rad };
		pts.push_back(vec);
	}

	for (size_t i = 0; i < side; ++i) {
		std::vector<double>& pt1 = pts[i];
		std::vector<double>& pt2 = pts[(i + skip) % side];
		for (size_t j = 0; j < mid + 1; ++j) {
			double xy[2] = { Math::Lerp::Linear(pt1[0], pt2[0], j * gapM), Math::Lerp::Linear(pt1[1], pt2[1], j * gapM) };
			value v = script->CreateRealArrayValue(xy, 2);
			arr.push_back(v);
		}
	}

	return script->CreateValueArrayValue(arr);
}


value ScriptClientBase::Func_ToDegrees(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(argv->as_real()));
}
value ScriptClientBase::Func_ToRadians(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::DegreeToRadian(argv->as_real()));
}
value ScriptClientBase::Func_NormalizeAngle(script_machine* machine, int argc, const value* argv) {
	double ang = argv->as_real();
	return CreateRealValue(Math::NormalizeAngleDeg(ang));
}
value ScriptClientBase::Func_NormalizeAngleR(script_machine* machine, int argc, const value* argv) {
	double ang = argv->as_real();
	return CreateRealValue(Math::NormalizeAngleRad(ang));
}
value ScriptClientBase::Func_AngularDistance(script_machine* machine, int argc, const value* argv) {
	double angFrom = argv[0].as_real();
	double angTo = argv[1].as_real();
	double dist = Math::NormalizeAngleDeg(angTo - angFrom);
	if (dist > 180.0) dist -= 360.0;
	return CreateRealValue(dist);
}
value ScriptClientBase::Func_AngularDistanceR(script_machine* machine, int argc, const value* argv) {
	double angFrom = argv[0].as_real();
	double angTo = argv[1].as_real();
	double dist = Math::NormalizeAngleRad(angTo - angFrom);
	if (dist > GM_PI) dist -= GM_PI_X2;
	return CreateRealValue(dist);
}
value ScriptClientBase::Func_ReflectAngle(script_machine* machine, int argc, const value* argv) {
	double angRay = argv[0].as_real();
	double angSurf = argv[1].as_real();
	double angRef = Math::NormalizeAngleDeg(2 * angSurf - angRay);
	return CreateRealValue(angRef);
}
value ScriptClientBase::Func_ReflectAngleR(script_machine* machine, int argc, const value* argv) {
	double angRay = argv[0].as_real();
	double angSurf = argv[1].as_real();
	double angRef = Math::NormalizeAngleRad(2 * angSurf - angRay);
	return CreateRealValue(angRef);
}

//共通関数：パス関連
value ScriptClientBase::Func_GetParentScriptDirectory(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	const std::wstring& path = script->GetEngine()->GetPath();
	std::wstring res = PathProperty::GetFileDirectory(path);
	return script->CreateStringValue(res);
}
value ScriptClientBase::Func_GetCurrentScriptDirectory(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	int line = machine->get_current_line();
	const std::wstring& path = script->GetEngine()->GetScriptFileLineMap()->GetPath(line);
	std::wstring res = PathProperty::GetFileDirectory(path);
	return script->CreateStringValue(res);
}
value ScriptClientBase::Func_GetFilePathList(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::wstring dir = PathProperty::GetFileDirectory(argv->as_string());
	std::vector<std::wstring> listDir = File::GetFilePathList(dir);
	return script->CreateStringArrayValue(listDir);
}
value ScriptClientBase::Func_GetDirectoryList(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::wstring dir = PathProperty::GetFileDirectory(argv->as_string());
	std::vector<std::wstring> listDir = File::GetDirectoryPathList(dir);
	return script->CreateStringArrayValue(listDir);
}

//Path utility
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
	return ScriptClientBase::CreateBooleanValue(File::IsExists(path));
}
value ScriptClientBase::Func_IsDirectoryExists(script_machine* machine, int argc, const value* argv) {
	std::wstring path = argv->as_string();
	return ScriptClientBase::CreateBooleanValue(File::IsDirectory(path));
}

//共通関数：時刻関連
value ScriptClientBase::Func_GetSystemTimeMilliS(script_machine* machine, int argc, const value* argv) {
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	return ScriptClientBase::CreateIntValue(time);
}
value ScriptClientBase::Func_GetSystemTimeMicroS(script_machine* machine, int argc, const value* argv) {
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	int64_t time = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
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
	Logger::WriteTop(msg);
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

//共通関数：共通データ
value ScriptClientBase::Func_SetCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	auto itrArea = commonDataManager->GetDefaultAreaIterator();
	std::string key = StringUtility::ConvertWideToMulti(argv[0].as_string());
	shared_ptr<ScriptCommonData> dataArea = commonDataManager->GetData(itrArea);
	dataArea->SetValue(key, argv[1]);

	return value();
}
value ScriptClientBase::Func_GetCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	value res;
	if (argc == 2)
		res = argv[1];

	auto itrArea = commonDataManager->GetDefaultAreaIterator();
	std::string key = StringUtility::ConvertWideToMulti(argv[0].as_string());
	shared_ptr<ScriptCommonData> dataArea = commonDataManager->GetData(itrArea);

	auto resFind = dataArea->IsExists(key);
	if (resFind.first)
		res = resFind.second->second;

	return res;
}
value ScriptClientBase::Func_ClearCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	auto itrArea = commonDataManager->GetDefaultAreaIterator();
	shared_ptr<ScriptCommonData> dataArea = commonDataManager->GetData(itrArea);
	dataArea->Clear();

	return value();
}
value ScriptClientBase::Func_DeleteCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();
	auto area = commonDataManager->GetDefaultAreaIterator();

	std::string key = StringUtility::ConvertWideToMulti(argv[0].as_string());
	shared_ptr<ScriptCommonData> dataArea = commonDataManager->GetData(area);
	dataArea->DeleteValue(key);

	return value();
}
value ScriptClientBase::Func_SetAreaCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	std::string area = StringUtility::ConvertWideToMulti(argv[0].as_string());
	std::string key = StringUtility::ConvertWideToMulti(argv[1].as_string());

	auto resFind = commonDataManager->IsExists(area);
	if (resFind.first) {
		shared_ptr<ScriptCommonData> dataArea = commonDataManager->GetData(resFind.second);
		dataArea->SetValue(key, argv[2]);
	}

	return value();
}
value ScriptClientBase::Func_GetAreaCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	value res;
	if (argc == 3)
		res = argv[2];

	std::string area = StringUtility::ConvertWideToMulti(argv[0].as_string());
	std::string key = StringUtility::ConvertWideToMulti(argv[1].as_string());

	auto resFindArea = commonDataManager->IsExists(area);
	if (resFindArea.first) {
		shared_ptr<ScriptCommonData> dataArea = commonDataManager->GetData(resFindArea.second);
		auto resFindData = dataArea->IsExists(key);
		if (resFindData.first)
			res = dataArea->GetValue(resFindData.second);
	}
	return res;
}
value ScriptClientBase::Func_ClearAreaCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	std::string area = StringUtility::ConvertWideToMulti(argv[0].as_string());

	auto resFind = commonDataManager->IsExists(area);
	if (resFind.first) {
		shared_ptr<ScriptCommonData> dataArea = commonDataManager->GetData(resFind.second);
		dataArea->Clear();
	}

	return value();
}
value ScriptClientBase::Func_DeleteAreaCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	std::string area = StringUtility::ConvertWideToMulti(argv[0].as_string());
	std::string key = StringUtility::ConvertWideToMulti(argv[1].as_string());

	auto resFind = commonDataManager->IsExists(area);
	if (resFind.first) {
		shared_ptr<ScriptCommonData> dataArea = commonDataManager->GetData(resFind.second);
		dataArea->DeleteValue(key);
	}

	return value();
}
value ScriptClientBase::Func_DeleteWholeAreaCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	std::string area = StringUtility::ConvertWideToMulti(argv->as_string());
	commonDataManager->Erase(area);

	return value();
}
value ScriptClientBase::Func_CreateCommonDataArea(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	std::string area = StringUtility::ConvertWideToMulti(argv->as_string());
	commonDataManager->CreateArea(area);

	return value();
}
value ScriptClientBase::Func_CopyCommonDataArea(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	std::string areaDest = StringUtility::ConvertWideToMulti(argv[0].as_string());
	std::string areaSrc = StringUtility::ConvertWideToMulti(argv[1].as_string());
	if (commonDataManager->IsExists(areaSrc).first)
		commonDataManager->CopyArea(areaDest, areaSrc);

	return value();
}
value ScriptClientBase::Func_IsCommonDataAreaExists(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	std::string area = StringUtility::ConvertWideToMulti(argv->as_string());
	bool res = commonDataManager->IsExists(area).first;

	return script->CreateBooleanValue(res);
}
value ScriptClientBase::Func_GetCommonDataAreaKeyList(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	std::vector<std::string> listKey;
	for (auto itr = commonDataManager->MapBegin(); itr != commonDataManager->MapEnd(); ++itr) {
		listKey.push_back(itr->first);
	}

	return script->CreateStringArrayValue(listKey);
}
value ScriptClientBase::Func_GetCommonDataValueKeyList(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	std::string area = StringUtility::ConvertWideToMulti(argv->as_string());

	std::vector<std::string> listKey;

	auto resFind = commonDataManager->IsExists(area);
	if (resFind.first) {
		shared_ptr<ScriptCommonData> data = commonDataManager->GetData(resFind.second);
		for (auto itr = data->MapBegin(); itr != data->MapEnd(); ++itr) {
			listKey.push_back(itr->first);
		}
	}

	return script->CreateStringArrayValue(listKey);
}

value ScriptClientBase::Func_LoadCommonDataValuePointer(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	auto itrArea = commonDataManager->GetDefaultAreaIterator();
	auto dataArea = commonDataManager->GetData(itrArea);
	std::string key = StringUtility::ConvertWideToMulti(argv[0].as_string());

	uint64_t res = (uint64_t)nullptr;

	{
		value* pData = nullptr;

		auto resFind = dataArea->IsExists(key);
		if (resFind.first) {
			pData = &(resFind.second->second);
		}
		else if (argc == 2) {
			dataArea->SetValue(key, argv[1]);
			pData = &(dataArea->IsExists(key).second->second);
		}
		else dataArea = nullptr;

		//Galaxy brain hax method
		res = ((uint64_t)(dataArea.get()) << 32) | (uint64_t)(pData);
	}

	return script->CreateIntValue((int64_t&)res);
}
value ScriptClientBase::Func_LoadAreaCommonDataValuePointer(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	std::string area = StringUtility::ConvertWideToMulti(argv[0].as_string());
	std::string key = StringUtility::ConvertWideToMulti(argv[1].as_string());

	uint64_t res = (uint64_t)nullptr;

	{
		ScriptCommonData* pArea = commonDataManager->GetData(area).get();
		value* pData = nullptr;

		if (pArea) {
			auto resFind = pArea->IsExists(key);
			if (resFind.first) {
				pData = &(resFind.second->second);
			}
			else if (argc == 3) {
				pArea->SetValue(key, argv[2]);
				pData = &(pArea->IsExists(key).second->second);
			}
			else pArea = nullptr;

			res = ((uint64_t)(pArea) << 32) | (uint64_t)(pData);
		}
	}

	return script->CreateIntValue((int64_t&)res);
}
value ScriptClientBase::Func_SetCommonDataPtr(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	int64_t val = argv[0].as_int();

	ScriptCommonData::_Script_PointerData ptrData;
	if (ScriptCommonData::Script_DecomposePtr((uint64_t&)val, &ptrData)) {
		*(ptrData.pData) = argv[1];
	}

	return value();
}
value ScriptClientBase::Func_GetCommonDataPtr(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	value res;
	if (argc == 2) res = argv[1];

	int64_t val = argv[0].as_int();

	ScriptCommonData::_Script_PointerData ptrData;
	if (ScriptCommonData::Script_DecomposePtr((uint64_t&)val, &ptrData)) {
		res = *(ptrData.pData);
	}

	return res;
}
value ScriptClientBase::Func_IsValidCommonDataValuePointer(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = ScriptCommonDataManager::GetInstance();

	int64_t val = argv[0].as_int();
	return script->CreateBooleanValue(ScriptCommonData::Script_DecomposePtr((uint64_t&)val, nullptr));
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

//****************************************************************************
//ScriptCommonDataManager
//****************************************************************************
const std::string ScriptCommonDataManager::nameAreaDefault_ = "";
ScriptCommonDataManager* ScriptCommonDataManager::inst_ = nullptr;
ScriptCommonDataManager::ScriptCommonDataManager() {
	inst_ = this;

	defaultAreaIterator_ = CreateArea(nameAreaDefault_);
}
ScriptCommonDataManager::~ScriptCommonDataManager() {
	for (auto itr = mapData_.begin(); itr != mapData_.end(); ++itr) {
		itr->second->Clear();
		//delete itr->second;
	}
	mapData_.clear();

	inst_ = nullptr;
}
void ScriptCommonDataManager::Clear() {
	for (auto itr = mapData_.begin(); itr != mapData_.end(); ++itr) {
		itr->second->Clear();
		//delete itr->second;
	}
	mapData_.clear();
}
void ScriptCommonDataManager::Erase(const std::string& name) {
	auto itr = mapData_.find(name);
	if (itr != mapData_.end()) {
		itr->second->Clear();
		//delete itr->second;
		mapData_.erase(itr);
	}
}
std::pair<bool, ScriptCommonDataManager::CommonDataMap::iterator> ScriptCommonDataManager::IsExists(const std::string& name) {
	auto itr = mapData_.find(name);
	return std::make_pair(itr != mapData_.end(), itr);
}
ScriptCommonDataManager::CommonDataMap::iterator ScriptCommonDataManager::CreateArea(const std::string& name) {
	auto itrCheck = mapData_.find(name);
	if (itrCheck != mapData_.end()) {
		Logger::WriteTop(StringUtility::Format("ScriptCommonDataManager: Area \"%s\" already exists.", name.c_str()));
		return itrCheck;
	}
	auto pairRes = mapData_.insert(std::make_pair(name, new ScriptCommonData()));
	return pairRes.first;
}
void ScriptCommonDataManager::CopyArea(const std::string& nameDest, const std::string& nameSrc) {
	shared_ptr<ScriptCommonData> dataSrc = mapData_[nameSrc];
	shared_ptr<ScriptCommonData> dataDest(new ScriptCommonData());
	dataDest->Copy(dataSrc);
	mapData_[nameDest] = dataDest;
}
shared_ptr<ScriptCommonData> ScriptCommonDataManager::GetData(const std::string& name) {
	auto itr = mapData_.find(name);
	return GetData(itr);
}
shared_ptr<ScriptCommonData> ScriptCommonDataManager::GetData(CommonDataMap::iterator itr) {
	if (itr == mapData_.end()) return nullptr;
	return itr->second;
}
void ScriptCommonDataManager::SetData(const std::string& name, shared_ptr<ScriptCommonData> commonData) {
	mapData_[name] = commonData;
}
void ScriptCommonDataManager::SetData(CommonDataMap::iterator itr, shared_ptr<ScriptCommonData> commonData) {
	if (itr == mapData_.end()) return;
	itr->second = commonData;
}
//****************************************************************************
//ScriptCommonData
//****************************************************************************
ScriptCommonData::ScriptCommonData() {
	verifHash_ = DATA_HASH;
}
ScriptCommonData::~ScriptCommonData() {}
void ScriptCommonData::Clear() {
	mapValue_.clear();
}
std::pair<bool, std::map<std::string, gstd::value>::iterator> ScriptCommonData::IsExists(const std::string& name) {
	auto itr = mapValue_.find(name);
	return std::make_pair(itr != mapValue_.end(), itr);
}
gstd::value* ScriptCommonData::GetValueRef(const std::string& name) {
	auto itr = mapValue_.find(name);
	return GetValueRef(itr);
}
gstd::value* ScriptCommonData::GetValueRef(std::map<std::string, gstd::value>::iterator itr) {
	if (itr == mapValue_.end()) return nullptr;
	return &itr->second;
}
gstd::value ScriptCommonData::GetValue(const std::string& name) {
	auto itr = mapValue_.find(name);
	return GetValue(itr);
}
gstd::value ScriptCommonData::GetValue(std::map<std::string, gstd::value>::iterator itr) {
	if (itr == mapValue_.end()) return value();
	return itr->second;
}
void ScriptCommonData::SetValue(const std::string& name, gstd::value v) {
	mapValue_[name] = v;
}
void ScriptCommonData::SetValue(std::map<std::string, gstd::value>::iterator itr, gstd::value v) {
	if (itr == mapValue_.end()) return;
	itr->second = v;
}
void ScriptCommonData::DeleteValue(const std::string& name) {
	mapValue_.erase(name);
}
void ScriptCommonData::Copy(shared_ptr<ScriptCommonData>& dataSrc) {
	mapValue_.clear();
	for (auto itrKey = dataSrc->MapBegin(); itrKey != dataSrc->MapEnd(); ++itrKey) {
		mapValue_.insert(std::make_pair(itrKey->first, itrKey->second));
	}
}
void ScriptCommonData::ReadRecord(gstd::RecordBuffer& record) {
	mapValue_.clear();

	std::vector<std::string> listKey = record.GetKeyList();
	for (const std::string& key : listKey) {
		shared_ptr<RecordEntry> entry = record.GetEntry(key);
		gstd::ByteBuffer& buffer = entry->GetBufferRef();

		gstd::value storedVal;
		uint32_t storedSize = 0U;
		buffer.Seek(0);
		buffer.Read(&storedSize, sizeof(uint32_t));

		if (storedSize > 0U) {
			gstd::ByteBuffer bufferRes;
			bufferRes.SetSize(storedSize);
			buffer.Read(bufferRes.GetPointer(), bufferRes.GetSize());
			storedVal = _ReadRecord(bufferRes);
		}

		mapValue_[key] = storedVal;
	}
}
gstd::value ScriptCommonData::_ReadRecord(gstd::ByteBuffer& buffer) {
	script_type_manager* scriptTypeManager = script_type_manager::get_instance();

	uint8_t kind;
	buffer.Read(&kind, sizeof(uint8_t));

	switch ((type_data::type_kind)kind) {
	case type_data::type_kind::tk_int:
	{
		int64_t data = buffer.ReadInteger64();
		return value(scriptTypeManager->get_int_type(), data);
	}
	case type_data::type_kind::tk_real:
	{
		double data = buffer.ReadDouble();
		return value(scriptTypeManager->get_real_type(), data);
	}
	case type_data::type_kind::tk_char:
	{
		wchar_t data = buffer.ReadValue<wchar_t>();
		return value(scriptTypeManager->get_char_type(), data);
	}
	case type_data::type_kind::tk_boolean:
	{
		bool data = buffer.ReadBoolean();
		return value(scriptTypeManager->get_boolean_type(), data);
	}
	case type_data::type_kind::tk_array:
	{
		uint32_t arrayLength = buffer.ReadValue<uint32_t>();
		if (arrayLength > 0U) {
			std::vector<value> v;
			v.resize(arrayLength);
			for (uint32_t iArray = 0; iArray < arrayLength; iArray++) {
				v[iArray] = _ReadRecord(buffer);
			}
			value res;
			res.reset(scriptTypeManager->get_array_type(v[0].get_type()), v);
			return res;
		}
		return value(scriptTypeManager->get_null_array_type(), std::wstring());
	}
	}

	return value();
}
void ScriptCommonData::WriteRecord(gstd::RecordBuffer& record) {
	for (auto itrValue = mapValue_.begin(); itrValue != mapValue_.end(); ++itrValue) {
		const std::string& key = itrValue->first;
		const gstd::value& comVal = itrValue->second;
		
		gstd::ByteBuffer buffer;
		buffer.WriteValue<uint32_t>(0U);

		if (comVal.has_data()) {
			_WriteRecord(buffer, comVal);
			buffer.Seek(0U);
			buffer.WriteValue<uint32_t>(buffer.GetSize() - sizeof(uint32_t));
		}

		record.SetRecord(key, buffer.GetPointer(), buffer.GetSize());
	}
}
void ScriptCommonData::_WriteRecord(gstd::ByteBuffer& buffer, const gstd::value& comValue) {
	type_data::type_kind kind = comValue.get_type()->get_kind();

	buffer.WriteValue<uint8_t>((uint8_t)kind);
	switch (kind) {
	case type_data::type_kind::tk_int:
		buffer.WriteInteger64(comValue.as_int());
		break;
	case type_data::type_kind::tk_real:
		buffer.WriteDouble(comValue.as_real());
		break;
	case type_data::type_kind::tk_char:
		buffer.WriteValue(comValue.as_char());
		break;
	case type_data::type_kind::tk_boolean:
		buffer.WriteBoolean(comValue.as_boolean());
		break;
	case type_data::type_kind::tk_array:
	{
		uint32_t arrayLength = comValue.length_as_array();
		buffer.WriteValue(arrayLength);
		for (size_t iArray = 0; iArray < arrayLength; iArray++) {
			const value& arrayValue = comValue.index_as_array(iArray);
			_WriteRecord(buffer, arrayValue);
		}
		break;
	}
	}
}

bool ScriptCommonData::Script_DecomposePtr(uint64_t val, _Script_PointerData* dst) {
	//Pointer format:
	//	63    32				31     0
	//	xxxxxxxx				xxxxxxxx
	//	(ScriptCommonData*)		(value*)

	ScriptCommonData* pArea = (ScriptCommonData*)(val >> 32);
	value* pData = (value*)(val & 0xffffffff);

	if (dst) {
		dst->pArea = pArea;
		dst->pData = pData;
	}
	return pArea != nullptr && pData != nullptr && pArea->CheckHash();
}

//****************************************************************************
//ScriptCommonDataPanel
//****************************************************************************
ScriptCommonDataInfoPanel::ScriptCommonDataInfoPanel() {
	timeLastUpdate_ = 0;
	timeUpdateInterval_ = 1000;
	commonDataManager_ = nullptr;
}
bool ScriptCommonDataInfoPanel::_AddedLogger(HWND hTab) {
	Create(hTab);

	gstd::WListView::Style styleListViewArea;
	styleListViewArea.SetStyle(WS_CHILD | WS_VISIBLE |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOCOLUMNHEADER);
	styleListViewArea.SetStyleEx(WS_EX_CLIENTEDGE);
	styleListViewArea.SetListViewStyleEx(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	wndListViewArea_.Create(hWnd_, styleListViewArea);
	wndListViewArea_.AddColumn(256, 0, L"Area");

	gstd::WListView::Style styleListViewValue;
	styleListViewValue.SetStyle(WS_CHILD | WS_VISIBLE |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER);
	styleListViewValue.SetStyleEx(WS_EX_CLIENTEDGE);
	styleListViewValue.SetListViewStyleEx(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	wndListViewValue_.Create(hWnd_, styleListViewValue);
	wndListViewValue_.AddColumn(96, COL_KEY, L"Key");
	wndListViewValue_.AddColumn(512, COL_VALUE, L"Value");

	wndSplitter_.Create(hWnd_, WSplitter::TYPE_HORIZONTAL);
	wndSplitter_.SetRatioY(0.25f);

	return true;
}
void ScriptCommonDataInfoPanel::LocateParts() {
	int wx = GetClientX();
	int wy = GetClientY();
	int wWidth = GetClientWidth();
	int wHeight = GetClientHeight();

	int ySplitter = (int)((float)wHeight * wndSplitter_.GetRatioY());
	int heightSplitter = 6;

	wndSplitter_.SetBounds(wx, ySplitter, wWidth, heightSplitter);
	wndListViewArea_.SetBounds(wx, wy, wWidth, ySplitter);
	wndListViewValue_.SetBounds(wx, ySplitter + heightSplitter, wWidth, wHeight - ySplitter - heightSplitter);
}
void ScriptCommonDataInfoPanel::Update() {
	if (!IsWindowVisible()) return;
	{
		Lock lock(lock_);
		//if (commonDataManager_) commonDataManager_->Clear();

		if (commonDataManager_ = ScriptCommonDataManager::GetInstance()) {
			_UpdateAreaView();
			_UpdateValueView();
		}
		else {
			wndListViewArea_.Clear();
			wndListViewValue_.Clear();
		}
	}
}
void ScriptCommonDataInfoPanel::_UpdateAreaView() {
	vecMapItr_.clear();

	int iRow = 0;
	for (auto itr = commonDataManager_->MapBegin(); itr != commonDataManager_->MapEnd(); ++itr, ++iRow) {
		std::wstring key = StringUtility::ConvertMultiToWide(itr->first);
		wndListViewArea_.SetText(iRow, COL_KEY, std::wstring(L"> ") + key);
		vecMapItr_.push_back(itr);
	}

	int countRow = wndListViewArea_.GetRowCount();
	for (; iRow < countRow; ++iRow)
		wndListViewArea_.DeleteRow(iRow);
}
void ScriptCommonDataInfoPanel::_UpdateValueView() {
	int indexArea = wndListViewArea_.GetSelectedRow();
	if (indexArea < 0) {
		wndListViewValue_.Clear();
		return;
	}

	shared_ptr<ScriptCommonData>& selectedArea = commonDataManager_->GetData(vecMapItr_[indexArea]);
	int iRow = 0;
	for (auto itr = selectedArea->MapBegin(); itr != selectedArea->MapEnd(); ++itr, ++iRow) {
		gstd::value* val = selectedArea->GetValueRef(itr);
		wndListViewValue_.SetText(iRow, COL_KEY, StringUtility::ConvertMultiToWide(itr->first));
		wndListViewValue_.SetText(iRow, COL_VALUE, val->as_string());
	}

	int countRow = wndListViewValue_.GetRowCount();
	for (; iRow < countRow; ++iRow)
		wndListViewValue_.DeleteRow(iRow);
}

#include "source/GcLib/pch.h"

#include "ScriptClient.hpp"
#include "File.hpp"
#include "Logger.hpp"

using namespace gstd;

/**********************************************************
//ScriptEngineData
**********************************************************/
ScriptEngineData::ScriptEngineData() {
	encoding_ = Encoding::UNKNOWN;
	mapLine_ = new ScriptFileLineMap();
}
ScriptEngineData::~ScriptEngineData() {}
void ScriptEngineData::SetSource(std::vector<char>& source) {
	encoding_ = Encoding::UTF8;
	if (Encoding::IsUtf16Le(&source[0], source.size())) {
		encoding_ = Encoding::UTF16LE;
	}
	source_ = source;
}

/**********************************************************
//ScriptEngineCache
**********************************************************/
ScriptEngineCache::ScriptEngineCache() {

}
ScriptEngineCache::~ScriptEngineCache() {}
void ScriptEngineCache::Clear() {
	cache_.clear();
}
void ScriptEngineCache::AddCache(const std::wstring& name, ref_count_ptr<ScriptEngineData> data) {
	cache_[name] = data;
}
ref_count_ptr<ScriptEngineData> ScriptEngineCache::GetCache(const std::wstring& name) {
	auto itrFind = cache_.find(name);
	if (cache_.find(name) == cache_.end()) return nullptr;
	return itrFind->second;
}
bool ScriptEngineCache::IsExists(const std::wstring& name) {
	return cache_.find(name) != cache_.end();
}

/**********************************************************
//ScriptClientBase
**********************************************************/
function const commonFunction[] =
{
	//共通関数：スクリプト引数結果
	{ "GetScriptArgument", ScriptClientBase::Func_GetScriptArgument, 1 },
	{ "GetScriptArgumentCount", ScriptClientBase::Func_GetScriptArgumentCount, 0 },
	{ "SetScriptResult", ScriptClientBase::Func_SetScriptResult, 1 },

	//共通関数：数学系
	{ "min", ScriptClientBase::Func_Min, 2 },
	{ "max", ScriptClientBase::Func_Max, 2 },
	{ "clamp", ScriptClientBase::Func_Clamp, 3 },
	{ "log", ScriptClientBase::Func_Log, 1 },
	{ "log10", ScriptClientBase::Func_Log10, 1 },
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

	{ "exp", ScriptClientBase::Func_Exp, 1 },
	{ "sqrt", ScriptClientBase::Func_Sqrt, 1 },
	{ "nroot", ScriptClientBase::Func_NRoot, 2 },
	{ "hypot", ScriptClientBase::Func_Hypot, 2 },

	{ "rand", ScriptClientBase::Func_Rand, 2 },
	{ "prand", ScriptClientBase::Func_RandEff, 2 },

	{ "ToDegrees", ScriptClientBase::Func_ToDegrees, 1 },
	{ "ToRadians", ScriptClientBase::Func_ToRadians, 1 },
	{ "NormalizeAngle", ScriptClientBase::Func_NormalizeAngle, 1 },
	{ "NormalizeAngleR", ScriptClientBase::Func_RNormalizeAngle, 1 },

	{ "Interpolate_Linear", ScriptClientBase::Func_Interpolate_Linear, 3 },
	{ "Interpolate_Smooth", ScriptClientBase::Func_Interpolate_Smooth, 3 },
	{ "Interpolate_Smoother", ScriptClientBase::Func_Interpolate_Smoother, 3 },
	{ "Interpolate_Accelerate", ScriptClientBase::Func_Interpolate_Accelerate, 3 },
	{ "Interpolate_Decelerate", ScriptClientBase::Func_Interpolate_Decelerate, 3 },
	{ "Interpolate_Modulate", ScriptClientBase::Func_Interpolate_Modulate, 4 },
	{ "Interpolate_Overshoot", ScriptClientBase::Func_Interpolate_Overshoot, 4 },
	{ "Interpolate_QuadraticBezier", ScriptClientBase::Func_Interpolate_QuadraticBezier, 4 },
	{ "Interpolate_CubicBezier", ScriptClientBase::Func_Interpolate_CubicBezier, 5 },

	{ "typeof", ScriptClientBase::Func_TypeOf, 1 },
	{ "ftypeof", ScriptClientBase::Func_FTypeOf, 1 },

	//共通関数：文字列操作
	{ "ToString", ScriptClientBase::Func_ToString, 1 },
	{ "IntToString", ScriptClientBase::Func_ItoA, 1 },
	{ "itoa", ScriptClientBase::Func_ItoA, 1 },
	{ "rtoa", ScriptClientBase::Func_RtoA, 1 },
	//{ "rtoa_ex", ScriptClientBase::Func_RtoA_Ex, -2 },
	{ "rtoa_ex", ScriptClientBase::Func_RtoA_Ex, 2 },
	{ "rtos", ScriptClientBase::Func_RtoS, 2 },
	{ "vtos", ScriptClientBase::Func_VtoS, 2 },
	{ "atoi", ScriptClientBase::Func_AtoI, 1 },
	{ "ator", ScriptClientBase::Func_AtoR, 1 },
	{ "TrimString", ScriptClientBase::Func_TrimString, 1 },
	{ "SplitString", ScriptClientBase::Func_SplitString, 2 },

	{ "RegexMatch", ScriptClientBase::Func_RegexMatch, 2 },
	{ "RegexMatchRepeated", ScriptClientBase::Func_RegexMatchRepeated, 2 },
	{ "RegexReplace", ScriptClientBase::Func_RegexReplace, 3 },

	//共通関数：パス関連
	{ "GetParentScriptDirectory", ScriptClientBase::Func_GetParentScriptDirectory, 0 },
	{ "GetCurrentScriptDirectory", ScriptClientBase::Func_GetCurrentScriptDirectory, 0 },
	{ "GetFilePathList", ScriptClientBase::Func_GetFilePathList, 1 },
	{ "GetDirectoryList", ScriptClientBase::Func_GetDirectoryList, 1 },

	//Path utility
	{ "GetModuleDirectory", ScriptClientBase::Func_GetModuleDirectory, 0 },
	{ "GetFileDirectory", ScriptClientBase::Func_GetFileDirectory, 1 },
	{ "GetFileDirectoryFromModule", ScriptClientBase::Func_GetFileDirectoryFromModule, 1 },
	{ "GetFileTopDirectory", ScriptClientBase::Func_GetFileTopDirectory, 1 },
	{ "GetFileName", ScriptClientBase::Func_GetFileName, 1 },
	{ "GetFileNameWithoutExtension", ScriptClientBase::Func_GetFileNameWithoutExtension, 1 },
	{ "GetFileExtension", ScriptClientBase::Func_GetFileExtension, 1 },

	//共通関数：時刻関連
	{ "GetCurrentDateTimeS", ScriptClientBase::Func_GetCurrentDateTimeS, 0 },

	//共通関数：デバッグ関連
	{ "WriteLog", ScriptClientBase::Func_WriteLog, 1 },
	{ "RaiseError", ScriptClientBase::Func_RaiseError, 1 },

	//共通関数：共通データ
	{ "SetCommonData", ScriptClientBase::Func_SetCommonData, 2 },
	{ "GetCommonData", ScriptClientBase::Func_GetCommonData, 2 },
	{ "ClearCommonData", ScriptClientBase::Func_ClearCommonData, 0 },
	{ "DeleteCommonData", ScriptClientBase::Func_DeleteCommonData, 1 },
	{ "SetAreaCommonData", ScriptClientBase::Func_SetAreaCommonData, 3 },
	{ "GetAreaCommonData", ScriptClientBase::Func_GetAreaCommonData, 3 },
	{ "ClearAreaCommonData", ScriptClientBase::Func_ClearAreaCommonData, 1 },
	{ "DeleteAreaCommonData", ScriptClientBase::Func_DeleteAreaCommonData, 2 },
	{ "DeleteWholeAreaCommonData", ScriptClientBase::Func_DeleteWholeAreaCommonData, 1 },
	{ "CreateCommonDataArea", ScriptClientBase::Func_CreateCommonDataArea, 1 },
	{ "CopyCommonDataArea", ScriptClientBase::Func_CopyCommonDataArea, 2 },
	{ "IsCommonDataAreaExists", ScriptClientBase::Func_IsCommonDataAreaExists, 1 },
	{ "GetCommonDataAreaKeyList", ScriptClientBase::Func_GetCommonDataAreaKeyList, 0 },
	{ "GetCommonDataValueKeyList", ScriptClientBase::Func_GetCommonDataValueKeyList, 1 },

	//定数
	{ "NULL", constant<0>::func, 0 },

	{ "VAR_REAL", constant<(int)type_data::type_kind::tk_real>::func, 0 },
	{ "VAR_CHAR", constant<(int)type_data::type_kind::tk_char>::func, 0 },
	{ "VAR_BOOL", constant<(int)type_data::type_kind::tk_boolean>::func, 0 },
	{ "VAR_ARRAY", constant<(int)type_data::type_kind::tk_array>::func, 0 },
	{ "VAR_STRING", constant<(int)type_data::type_kind::tk_array + 1>::func, 0 },

	{ "LERP_LINEAR", constant<Math::Lerp::LINEAR>::func, 0 },
	{ "LERP_SMOOTH", constant<Math::Lerp::SMOOTH>::func, 0 },
	{ "LERP_SMOOTHER", constant<Math::Lerp::SMOOTHER>::func, 0 },
	{ "LERP_ACCELERATE", constant<Math::Lerp::ACCELERATE>::func, 0 },
	{ "LERP_DECELERATE", constant<Math::Lerp::DECELERATE>::func, 0 },

	{ "pi", pconstant<&GM_PI>::func, 0 },		//For compatibility reasons.
	{ "M_PI", pconstant<&GM_PI>::func, 0 },
	{ "M_PI_2", pconstant<&GM_PI_2>::func, 0 },
	{ "M_PI_4", pconstant<&GM_PI_4>::func, 0 },
	{ "M_PI_X2", pconstant<&GM_PI_X2>::func, 0 },
	{ "M_PI_X4", pconstant<&GM_PI_X4>::func, 0 },
	{ "M_1_PI", pconstant<&GM_1_PI>::func, 0 },
	{ "M_2_PI", pconstant<&GM_2_PI>::func, 0 },
	{ "M_SQRTPI", pconstant<&GM_SQRTP>::func, 0 },
	{ "M_1_SQRTPI", pconstant<&GM_1_SQRTP>::func, 0 },
	{ "M_2_SQRTPI", pconstant<&GM_2_SQRTP>::func, 0 },
	{ "M_SQRT2", pconstant<&GM_SQRT2>::func, 0 },
	{ "M_SQRT2_2", pconstant<&GM_SQRT2_2>::func, 0 },
	{ "M_SQRT2_X2", pconstant<&GM_SQRT2_X2>::func, 0 },
	{ "M_E", pconstant<&GM_E>::func, 0 },
	{ "M_LOG2E", pconstant<&GM_LOG2E>::func, 0 },
	{ "M_LOG10E", pconstant<&GM_LOG10E>::func, 0 },
	{ "M_LN2", pconstant<&GM_LN2>::func, 0 },
	{ "M_LN10", pconstant<&GM_LN10>::func, 0 },
	{ "M_PHI", pconstant<&GM_PHI>::func, 0 },
	{ "M_1_PHI", pconstant<&GM_1_PHI>::func, 0 },
};

ScriptClientBase::ScriptClientBase() {
	bError_ = false;
	engine_ = new ScriptEngineData();
	machine_ = nullptr;
	mainThreadID_ = -1;
	idScript_ = ID_SCRIPT_FREE;
	valueRes_ = value();

	pTypeManager_ = script_type_manager::get_instance();
	//Leak
	if (pTypeManager_ == nullptr)
		pTypeManager_ = new script_type_manager;

	commonDataManager_ = new ScriptCommonDataManager();

	{
		DWORD seed = timeGetTime();

		mt_ = std::make_shared<RandProvider>();
		mt_->Initialize(seed ^ 0xc3c3c3c3);

		mtEffect_ = std::make_shared<RandProvider>();
		mtEffect_->Initialize(((seed ^ 0xf27ea021) << 11) ^ ((seed ^ 0x8b56c1b5) >> 11));
	}

	_AddFunction(commonFunction, sizeof(commonFunction) / sizeof(function));
}
ScriptClientBase::~ScriptClientBase() {
	ptr_delete(machine_);
	engine_ = nullptr;
}

void ScriptClientBase::_AddFunction(const char* name, callback f, size_t arguments) {
	function tFunc;
	tFunc.name = name;
	tFunc.func = f;
	tFunc.arguments = arguments;
	func_.push_back(tFunc);
}
void ScriptClientBase::_AddFunction(const function* f, size_t count) {
	size_t funcPos = func_.size();
	func_.resize(funcPos + count);
	memcpy(&func_[funcPos], f, sizeof(function) * count);
}

void ScriptClientBase::_RaiseError(int line, const std::wstring& message) {
	bError_ = true;
	std::wstring errorPos = _GetErrorLineSource(line);

	gstd::ref_count_ptr<ScriptFileLineMap> mapLine = engine_->GetScriptFileLineMap();
	ScriptFileLineMap::Entry* entry = mapLine->GetEntry(line);
	int lineOriginal = entry->lineEndOriginal_ - (entry->lineEnd_ - line);

	std::wstring fileName = PathProperty::GetFileName(entry->path_);

	std::wstring str = StringUtility::Format(L"%s\r\n%s" "\r\n[%s] " "line-> %d\r\n\r\n↓\r\n%s\r\n～～～",
		message.c_str(),
		entry->path_.c_str(),
		fileName.c_str(),
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
	int encoding = engine_->GetEncoding();
	std::vector<char>& source = engine_->GetSource();
	char* pbuf = (char*)&source[0];
	char* sbuf = pbuf;
	char* ebuf = sbuf + source.size();

	int tLine = 1;
	int rLine = line;
	while (pbuf < ebuf) {
		if (tLine == rLine)
			break;

		if (encoding == Encoding::UTF16LE || encoding == Encoding::UTF16BE) {
			wchar_t ch = (wchar_t&)*pbuf;
			if (encoding == Encoding::UTF16BE) ch = (ch >> 8) | (ch << 8);
			if (ch == L'\n')
				tLine++;
			pbuf += 2;
		}
		else {
			if (*pbuf == '\n')
				tLine++;
			pbuf++;
		}
	}

	const int countMax = 256;
	int count = 0;
	sbuf = pbuf;
	while (pbuf < ebuf && count < countMax) {
		pbuf++;
		count++;
	}

	int size = std::max(count - 1, 0);
	std::wstring res;
	if (encoding == Encoding::UTF16LE || encoding == Encoding::UTF16BE) {
		wchar_t* wbufS = (wchar_t*)sbuf;
		wchar_t* wbufE = wbufS + size;
		res = std::wstring(wbufS, wbufE);
		if (encoding == Encoding::UTF16BE) {
			for (auto itr = res.begin(); itr != res.end(); ++itr) {
				wchar_t wch = *itr;
				*itr = (wch >> 8) | (wch << 8);
			}
		}
	}
	else {
		std::string sStr = std::string(sbuf, sbuf + size);
		res = StringUtility::ConvertMultiToWide(sStr);
	}

	return res;
}
std::vector<char> ScriptClientBase::_Include(std::vector<char>& source) {
	//TODO とりあえず実装
	std::wstring pathSource = engine_->GetPath();
	std::vector<char> res = source;
	FileManager* fileManager = FileManager::GetBase();
	std::set<std::wstring> setReadPath;

	gstd::ref_count_ptr<ScriptFileLineMap> mapLine = new ScriptFileLineMap();
	engine_->SetScriptFileLineMap(mapLine);

	mapLine->AddEntry(pathSource, 1, StringUtility::CountCharacter(source, '\n') + 1);

	//std::vector<Token> mapTokens;

	int mainEncoding = Encoding::UTF8;
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
			if (tok.GetType() == Token::Type::TK_EOF)//Eofの識別子が来たらファイルの調査終了
			{
				break;
			}
			else if (tok.GetType() == Token::Type::TK_SHARP) {
				size_t ptrPosBeforeInclude = scanner.GetCurrentPointer() - 1;
				if (mainEncoding == Encoding::UTF16LE || mainEncoding == Encoding::UTF16BE)
					ptrPosBeforeInclude--;
				std::vector<char>::iterator posBeforeInclude = res.begin() + ptrPosBeforeInclude;

				tok = scanner.Next();
				if (tok.GetElement() != L"include") continue;

				bEnd = false;
				int posCurrent = scanner.GetCurrentPointer();
				std::wstring wPath = scanner.Next().GetString();
				bool bNeedNewLine = false;
				if (scanner.HasNext()) {
					int posBeforeNewLine = scanner.GetCurrentPointer();
					if (scanner.Next().GetType() != Token::Type::TK_NEWLINE) {
						int line = scanner.GetCurrentLine();
						source = res;
						engine_->SetSource(source);

						std::wstring error = L"Newline is not found after #include. (Did you accidentally put a semicolon?)\r\n";
						_RaiseError(line, error);
					}
					scanner.SetCurrentPointer(posBeforeNewLine);
				}
				else {
					//bNeedNewLine = true;
				}

				size_t ptrPosAfterInclude = scanner.GetCurrentPointer();
				std::vector<char>::iterator posAfterInclude = res.begin() + scanner.GetCurrentPointer();
				scanner.SetCurrentPointer(posCurrent);

				// "../"から始まっていたら、前に"./"をつける。
				if (wPath.find(L"../") == 0 || wPath.find(L"..\\") == 0) {
					wPath = L"./" + wPath;
				}

				if (wPath.find(L".\\") != std::wstring::npos || wPath.find(L"./") != std::wstring::npos) {	//".\"展開
					int line = scanner.GetCurrentLine();
					const std::wstring& linePath = mapLine->GetPath(line);
					std::wstring tDir = PathProperty::GetFileDirectory(linePath);
					//std::string tDir = PathProperty::GetFileDirectory(pathSource);
					wPath = tDir.substr(PathProperty::GetModuleDirectory().size()) + wPath.substr(2);
				}
				wPath = PathProperty::GetModuleDirectory() + wPath;
				wPath = PathProperty::GetUnique(wPath);

				bool bAlreadyIncluded = setReadPath.find(wPath) != setReadPath.end();
				if (bAlreadyIncluded) {//すでに読み込まれていた
					Logger::WriteTop(StringUtility::Format(L"Scanner: File already included, skipping. (%s)", wPath.c_str()));
					res.erase(posBeforeInclude, posAfterInclude);
					break;
				}

				std::vector<char> placement;
				ref_count_ptr<FileReader> reader;
				reader = fileManager->GetFileReader(wPath);
				if (reader == nullptr || !reader->Open()) {
					int line = scanner.GetCurrentLine();
					source = res;
					engine_->SetSource(source);

					std::wstring error;
					error += StringUtility::Format(L"Include file is not found. [%s]\r\n", wPath.c_str());
					_RaiseError(line, error);
				}

				//ファイルを読み込み最後に改行を付加
				size_t targetBomSize = 0;
				int includeEncoding = Encoding::UTF8;
				if (reader->GetFileSize() >= 2) {
					char data[2];
					reader->Read(&data[0], 2);
					if (Encoding::IsUtf16Le(&data[0], 2)) {
						includeEncoding = Encoding::UTF16LE;
						targetBomSize = 2;
					}
					else if (Encoding::IsUtf16Be(&data[0], 2)) {
						includeEncoding = Encoding::UTF16BE;
						targetBomSize = 2;
					}
					//ファイルポインタを最初に戻す
					reader->SetFilePointerBegin();
				}

				if (includeEncoding == Encoding::UTF16LE || includeEncoding == Encoding::UTF16BE) {
					//Including UTF-16

					reader->Seek(targetBomSize);
					placement.resize(reader->GetFileSize() - targetBomSize); //- BOM size
					reader->Read(&placement[0], placement.size());

					//Convert the including file to UTF-8
					if (mainEncoding == Encoding::UTF8) {
						if (includeEncoding == Encoding::UTF16BE) {
							for (auto itr = placement.begin(); itr != placement.end(); itr += 2) {
								wchar_t* wch = (wchar_t*)&*itr;
								*wch = (*wch >> 8) | (*wch << 8);
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
				else {
					//Including UTF-8

					placement.resize(reader->GetFileSize());
					reader->Read(&placement[0], reader->GetFileSize());

					//Convert the include file to the main script's encoding
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
								*wItr = *(wItr + 1);
								*(wItr + 1) = *wItr;
							}
						}
					}
				}
				mapLine->AddEntry(wPath,
					scanner.GetCurrentLine(),
					StringUtility::CountCharacter(placement, '\n') + 1);

				{
					std::vector<char> newSource;
					newSource.insert(newSource.begin(), res.begin(), posBeforeInclude);
					newSource.insert(newSource.end(), placement.begin(), placement.end());
					newSource.insert(newSource.end(), posAfterInclude, res.end());

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

				break;
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
	if (true) {
		std::wstring pathTest = PathProperty::GetModuleDirectory() +
			StringUtility::Format(L"temp/script_incl_%s", PathProperty::GetFileName(pathSource).c_str());
		pathTest = PathProperty::Canonicalize(pathTest);
		pathTest = PathProperty::ReplaceYenToSlash(pathTest);

		File file(pathTest);
		File::CreateFileDirectory(pathTest);
		file.Open(File::WRITEONLY);
		file.Write(tmpCh, sizeBuf);
		file.Close();
	}
	*/
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
	ptr_delete(machine_);

	std::vector<char>& source = engine_->GetSource();

	ref_count_ptr<script_engine> engine = new script_engine(source, func_.size(), &func_[0]);
	engine_->SetEngine(engine);
	return true;
}
bool ScriptClientBase::SetSourceFromFile(std::wstring path) {
	path = PathProperty::GetUnique(path);
	if (cache_ != nullptr && cache_->IsExists(path)) {
		engine_ = cache_->GetCache(path);
		return true;
	}

	engine_->SetPath(path);
	ref_count_ptr<FileReader> reader;
	reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr) throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path));
	if (!reader->Open()) throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path));

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
		std::vector<char> source = _Include(engine_->GetSource());
		engine_->SetSource(source);

		_CreateEngine();
		if (engine_->GetEngine()->get_error()) {
			bError_ = true;
			_RaiseErrorFromEngine();
		}
		if (cache_ != nullptr && engine_->GetPath().size() != 0) {
			cache_->AddCache(engine_->GetPath(), engine_);
		}
	}

	ptr_delete(machine_);
	machine_ = new script_machine(engine_->GetEngine().GetPointer());
	if (machine_->get_error()) {
		bError_ = true;
		_RaiseErrorFromMachine();
	}
	machine_->data = this;
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

	std::map<std::string, script_engine::block*>::iterator itrEvent;
	if (!machine_->has_event(target, itrEvent)) {
		_RaiseError(0, StringUtility::FormatToWide("The requested event does not exist. [%s]", target.c_str()));
	}

	Run();
	machine_->call(itrEvent);

	if (machine_->get_error()) {
		bError_ = true;
		_RaiseErrorFromMachine();
	}
	return true;
}
bool ScriptClientBase::Run(std::map<std::string, script_engine::block*>::iterator target) {
	if (bError_) return false;

	Run();
	machine_->call(target);

	if (machine_->get_error()) {
		bError_ = true;
		_RaiseErrorFromMachine();
	}
	return true;
}
bool ScriptClientBase::IsEventExists(const std::string& name, std::map<std::string, script_engine::block*>::iterator& res) {
	if (bError_) {
		if (machine_ && machine_->get_error()) _RaiseErrorFromMachine();
		else if (engine_->GetEngine()->get_error()) _RaiseErrorFromEngine();
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
value ScriptClientBase::CreateStringValue(const std::string& s) {
	std::wstring wstr = StringUtility::ConvertMultiToWide(s);
	return CreateStringValue(wstr);
}
value ScriptClientBase::CreateStringArrayValue(std::vector<std::string>& list) {
	script_type_manager* typeManager = script_type_manager::get_instance();

	if (list.size() > 0) {
		type_data* type_arr = typeManager->get_array_type(typeManager->get_string_type());

		std::vector<value> res_arr;
		res_arr.resize(list.size());
		for (size_t iVal = 0U; iVal < list.size(); ++iVal) {
			value data = CreateStringValue(list[iVal]);
			res_arr[iVal] = data;
		}

		value res;
		res.set(type_arr, res_arr);
		return res;
	}

	return value(typeManager->get_string_type(), 0.0);
}
value ScriptClientBase::CreateStringArrayValue(std::vector<std::wstring>& list) {
	script_type_manager* typeManager = script_type_manager::get_instance();

	if (list.size() > 0) {
		type_data* type_arr = typeManager->get_array_type(typeManager->get_string_type());

		std::vector<value> res_arr;
		res_arr.resize(list.size());
		for (size_t iVal = 0U; iVal < list.size(); ++iVal) {
			value data = CreateStringValue(list[iVal]);
			res_arr[iVal] = data;
		}

		value res;
		res.set(type_arr, res_arr);
		return res;
	}

	return value(typeManager->get_string_type(), 0.0);
}
value ScriptClientBase::CreateValueArrayValue(std::vector<value>& list) {
	script_type_manager* typeManager = script_type_manager::get_instance();

	if (list.size() > 0) {
		type_data* type_array = typeManager->get_array_type(list[0].get_type());

		std::vector<value> res_arr;
		res_arr.resize(list.size());
		for (size_t iVal = 0U; iVal < list.size(); ++iVal) {
			machine_->append_check(iVal, type_array, list[iVal].get_type());
			res_arr[iVal] = list[iVal];
		}

		value res;
		res.set(type_array, res_arr);
		return res;
	}

	return value(typeManager->get_string_type(), 0.0);
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
bool ScriptClientBase::IsArrayValue(value& v, type_data* element) {
	if (!v.has_data()) return false;
	return v.get_type() == script_type_manager::get_instance()->get_array_type(element);
}
bool ScriptClientBase::IsRealArrayValue(value& v) {
	if (!v.has_data()) return false;
	return v.get_type() == script_type_manager::get_real_array_type();
}
void ScriptClientBase::CheckRunInMainThread() {
	if (mainThreadID_ < 0) return;
	if (mainThreadID_ != GetCurrentThreadId()) {
		std::wstring error;
		error += L"This function can only be called in the main thread.\r\n";
		machine_->raise_error(error);
	}
}
std::wstring ScriptClientBase::_ExtendPath(std::wstring path) {
	int line = machine_->get_current_line();
	const std::wstring& pathScript = GetEngine()->GetScriptFileLineMap()->GetPath(line);

	path = StringUtility::ReplaceAll(path, L"\\", L"/");
	path = StringUtility::ReplaceAll(path, L"./", pathScript);

	return path;
}

//共通関数：スクリプト引数結果
value ScriptClientBase::Func_GetScriptArgument(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = (ScriptClientBase*)machine->data;
	int index = argv[0].as_int();
	if (index < 0 || index >= script->listValueArg_.size()) {
		std::wstring error;
		error += L"Invalid script argument index.\r\n";
		throw gstd::wexception(error);
	}
	return script->listValueArg_[index];
}
value ScriptClientBase::Func_GetScriptArgumentCount(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = (ScriptClientBase*)machine->data;
	size_t res = script->listValueArg_.size();
	return script->CreateRealValue(res);
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
	double res = std::min(std::max(v, bound_lower), bound_upper);
	return CreateRealValue(res);
}
value ScriptClientBase::Func_Log(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(log(argv[0].as_real()));
}
value ScriptClientBase::Func_Log10(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(log10(argv[0].as_real()));
}

value ScriptClientBase::Func_Cos(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(cos(Math::DegreeToRadian(argv[0].as_real())));
}
value ScriptClientBase::Func_Sin(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(sin(Math::DegreeToRadian(argv[0].as_real())));
}
value ScriptClientBase::Func_Tan(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(tan(Math::DegreeToRadian(argv[0].as_real())));
}
value ScriptClientBase::Func_SinCos(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	double ang = Math::DegreeToRadian(argv[0].as_real());
	double csArray[2] = { sin(ang), cos(ang) };
	return script->CreateRealArrayValue(csArray, 2U);
}

value ScriptClientBase::Func_RCos(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(cos(argv[0].as_real()));
}
value ScriptClientBase::Func_RSin(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(sin(argv[0].as_real()));
}
value ScriptClientBase::Func_RTan(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(tan(argv[0].as_real()));
}
value ScriptClientBase::Func_RSinCos(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	double ang = argv[0].as_real();
	double csArray[2] = { sin(ang), cos(ang) };
	return script->CreateRealArrayValue(csArray, 2U);
}

value ScriptClientBase::Func_Acos(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(acos(argv[0].as_real())));
}
value ScriptClientBase::Func_Asin(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(asin(argv[0].as_real())));
}
value ScriptClientBase::Func_Atan(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(atan(argv[0].as_real())));
}
value ScriptClientBase::Func_Atan2(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(Math::RadianToDegree(atan2(argv[0].as_real(), argv[1].as_real())));
}
value ScriptClientBase::Func_RAcos(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(acos(argv[0].as_real()));
}
value ScriptClientBase::Func_RAsin(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(asin(argv[0].as_real()));
}
value ScriptClientBase::Func_RAtan(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(atan(argv[0].as_real()));
}
value ScriptClientBase::Func_RAtan2(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(atan2(argv[0].as_real(), argv[1].as_real()));
}

value ScriptClientBase::Func_Exp(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(exp(argv[0].as_real()));
}
value ScriptClientBase::Func_Sqrt(script_machine* machine, int argc, const value* argv) {
	return CreateRealValue(sqrt(argv[0].as_real()));
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

value ScriptClientBase::Func_Rand(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	script->CheckRunInMainThread();

	double min = argv[0].as_real();
	double max = argv[1].as_real();
	double res = script->mt_->GetReal(min, max);
	return script->CreateRealValue(res);
}
value ScriptClientBase::Func_RandEff(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	double min = argv[0].as_real();
	double max = argv[1].as_real();
	double res = script->mtEffect_->GetReal(min, max);
	return script->CreateRealValue(res);
}

void ScriptClientBase::IsMatrix(script_machine*& machine, const value& v) {
	type_data* typeReal = machine->get_engine()->get_real_type();

	if ((v.get_type() != typeReal) || (v.get_type()->get_element() != typeReal)) {
		std::wstring err = L"Invalid type, only matrices of real numbers may be used.";
		machine->raise_error(err);
	}
	if (v.length_as_array() != 16) {
		std::wstring err = L"This function only supports operations on 4x4 matrices.";
		machine->raise_error(err);
	}
}
void ScriptClientBase::IsVector(script_machine*& machine, const value& v, size_t count) {
	type_data* typeReal = machine->get_engine()->get_real_type();

	if ((v.get_type() != typeReal) || (v.get_type()->get_element() != typeReal)) {
		std::wstring err = L"Invalid type, only vectors of real numbers may be used.";
		machine->raise_error(err);
	}
	if (v.length_as_array() != count) {
		std::wstring err = L"Incorrect number of elements. (Expected " + std::to_wstring(count) + L")";
		machine->raise_error(err);
	}
}

value ScriptClientBase::Func_Interpolate_Linear(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_real();
	double b = argv[1].as_real();
	double x = argv[2].as_real();
	return CreateRealValue(Math::Lerp::Linear(a, b, x));
}
value ScriptClientBase::Func_Interpolate_Smooth(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_real();
	double b = argv[1].as_real();
	double x = argv[2].as_real();
	return CreateRealValue(Math::Lerp::Smooth(a, b, x));
}
value ScriptClientBase::Func_Interpolate_Smoother(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_real();
	double b = argv[1].as_real();
	double x = argv[2].as_real();
	return CreateRealValue(Math::Lerp::Smoother(a, b, x));
}
value ScriptClientBase::Func_Interpolate_Accelerate(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_real();
	double b = argv[1].as_real();
	double x = argv[2].as_real();
	return CreateRealValue(Math::Lerp::Accelerate(a, b, x));
}
value ScriptClientBase::Func_Interpolate_Decelerate(script_machine* machine, int argc, const value* argv) {
	double a = argv[0].as_real();
	double b = argv[1].as_real();
	double x = argv[2].as_real();
	return CreateRealValue(Math::Lerp::Decelerate(a, b, x));
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
	double res = (a * y * y) + x * (b * x + c * 2.0 * y);

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
	double res = (a * y * z) + x * ((b * x * x) + (c1 * c1 * c2 * 3.0 * z));

	return CreateRealValue(res);
}

value ScriptClientBase::Func_TypeOf(script_machine* machine, int argc, const value* argv) {
	type_data* type = argv->get_type();

	if (type->get_kind() == type_data::type_kind::tk_array) {
		if (type->get_element()->get_kind() == type_data::type_kind::tk_char)
			return CreateRealValue((int)type_data::type_kind::tk_array + 1);	//String
	}
	return CreateRealValue((int)type->get_kind());
}
value ScriptClientBase::Func_FTypeOf(script_machine* machine, int argc, const value* argv) {
	type_data* type = argv->get_type();
	while (type->get_kind() == type_data::type_kind::tk_array)
		type = type->get_element();

	return CreateRealValue((int)(type->get_kind()));
}

//組み込み関数：文字列操作
value ScriptClientBase::Func_ToString(script_machine* machine, int argc, const value* argv) {
	return CreateStringValue(argv->as_string());
}
value ScriptClientBase::Func_ItoA(script_machine* machine, int argc, const value* argv) {
	std::wstring res = StringUtility::Format(L"%d", (int)argv->as_real());
	return CreateStringValue(res);
}
value ScriptClientBase::Func_RtoA(script_machine* machine, int argc, const value* argv) {
	std::wstring res = StringUtility::Format(L"%f", argv->as_real());
	return CreateStringValue(res);
}
value ScriptClientBase::Func_RtoA_Ex(script_machine* machine, int argc, const value* argv) {
	/*
	std::wstring res = argv[0].as_string();
	if (argc > 1) {
		std::vector<double> f_va_list;
		for (int i = 1; i < argc; ++i) {
			f_va_list.push_back(argv[i].as_real());
		}
		double* f_data = f_va_list.data();

		res = StringUtility::Format(res.c_str(), reinterpret_cast<va_list>(f_data));
	}
	*/
	std::wstring format = argv[0].as_string();
	std::wstring res = StringUtility::Format(format.c_str(), argv[1].as_real());
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
		res = "error format";
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

		int advance = 0;//0:-, 1:0, 2:num, 3:[d,s,f], 4:., 5:num
		for (char ch : fmtV) {
			if (advance == 0 && ch == '-')
				advance = 1;
			else if ((advance == 0 || advance == 1 || advance == 2) && (ch >= '0' && ch <= '9'))
				advance = 2;
			else if (advance == 2 && (ch == 'd' || ch == 's' || ch == 'f'))
				advance = 4;
			else if (advance == 2 && ('.'))
				advance = 5;
			else if (advance == 4 && ('.'))
				advance = 5;
			else if (advance == 5 && (ch >= '0' && ch <= '9'))
				advance = 5;
			else if (advance == 5 && (ch == 'd' || ch == 's' || ch == 'f'))
				advance = 6;
			else throw false;
		}

		fmtV = std::string("%") + fmtV;
		char* fmt = (char*)fmtV.c_str();
		if (strstr(fmt, "d"))
			res = StringUtility::Format(fmt, argv[1].as_int());
		else if (strstr(fmt, "f"))
			res = StringUtility::Format(fmt, argv[1].as_real());
		else if (strstr(fmt, "s"))
			res = StringUtility::Format(fmt, StringUtility::ConvertWideToMulti(argv[1].as_string()).c_str());
	}
	catch (...) {
		res = "error format";
	}

	return CreateStringValue(StringUtility::ConvertMultiToWide(res));
}
value ScriptClientBase::Func_AtoI(script_machine* machine, int argc, const value* argv) {
	std::wstring str = argv->as_string();
	int num = StringUtility::ToInteger(str);
	return CreateRealValue(num);
}
value ScriptClientBase::Func_AtoR(script_machine* machine, int argc, const value* argv) {
	std::wstring str = argv->as_string();
	double num = StringUtility::ToDouble(str);
	return CreateRealValue(num);
}
value ScriptClientBase::Func_TrimString(script_machine* machine, int argc, const value* argv) {
	std::wstring str = argv->as_string();
	std::wstring res = StringUtility::Trim(str);
	return CreateStringValue(res);
}
value ScriptClientBase::Func_SplitString(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::wstring str = argv[0].as_string();
	std::wstring delim = argv[1].as_string();
	std::vector<std::wstring> list = StringUtility::Split(str, delim);

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
value ScriptClientBase::Func_RNormalizeAngle(script_machine* machine, int argc, const value* argv) {
	double ang = argv->as_real();
	return CreateRealValue(Math::NormalizeAngleRad(ang));
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

//共通関数：時刻関連
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
	std::wstring msg = argv->as_string();
	Logger::WriteTop(msg);
	return value();
}
value ScriptClientBase::Func_RaiseError(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	std::wstring msg = argv->as_string();
	script->RaiseError(msg);

	return value();
}

//共通関数：共通データ
value ScriptClientBase::Func_SetCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();
	auto area = commonDataManager->GetDefaultAreaIterator();

	std::string key = StringUtility::ConvertWideToMulti(argv[0].as_string());
	shared_ptr<ScriptCommonData> data = commonDataManager->GetData(area);
	data->SetValue(key, argv[1]);

	return value();
}
value ScriptClientBase::Func_GetCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();
	auto area = commonDataManager->GetDefaultAreaIterator();

	std::string key = StringUtility::ConvertWideToMulti(argv[0].as_string());
	shared_ptr<ScriptCommonData> data = commonDataManager->GetData(area);

	auto resFind = data->IsExists(key);
	if (!resFind.first) return argv[1];

	return resFind.second->second;
}
value ScriptClientBase::Func_ClearCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();
	auto area = commonDataManager->GetDefaultAreaIterator();

	shared_ptr<ScriptCommonData> data = commonDataManager->GetData(area);
	data->Clear();

	return value();
}
value ScriptClientBase::Func_DeleteCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();
	auto area = commonDataManager->GetDefaultAreaIterator();

	std::string key = StringUtility::ConvertWideToMulti(argv[0].as_string());
	shared_ptr<ScriptCommonData> data = commonDataManager->GetData(area);
	data->DeleteValue(key);

	return value();
}
value ScriptClientBase::Func_SetAreaCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();

	std::string area = StringUtility::ConvertWideToMulti(argv[0].as_string());
	std::string key = StringUtility::ConvertWideToMulti(argv[1].as_string());

	auto resFind = commonDataManager->IsExists(area);
	if (resFind.first) {
		shared_ptr<ScriptCommonData> data = commonDataManager->GetData(resFind.second);
		data->SetValue(key, argv[2]);
	}

	return value();
}
value ScriptClientBase::Func_GetAreaCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();

	std::string area = StringUtility::ConvertWideToMulti(argv[0].as_string());
	std::string key = StringUtility::ConvertWideToMulti(argv[1].as_string());
	value dv = argv[2];

	auto resFindArea = commonDataManager->IsExists(area);
	if (resFindArea.first) {
		shared_ptr<ScriptCommonData> data = commonDataManager->GetData(resFindArea.second);

		auto resFindData = data->IsExists(key);
		if (!resFindData.first) goto find_fail;

		return data->GetValue(resFindData.second);
	}
find_fail:
	return argv[2];
}
value ScriptClientBase::Func_ClearAreaCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();

	std::string area = StringUtility::ConvertWideToMulti(argv[0].as_string());

	auto resFind = commonDataManager->IsExists(area);
	if (resFind.first) {
		shared_ptr<ScriptCommonData> data = commonDataManager->GetData(resFind.second);
		data->Clear();
	}

	return value();
}
value ScriptClientBase::Func_DeleteAreaCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();

	std::string area = StringUtility::ConvertWideToMulti(argv[0].as_string());
	std::string key = StringUtility::ConvertWideToMulti(argv[1].as_string());

	auto resFind = commonDataManager->IsExists(area);
	if (resFind.first) {
		shared_ptr<ScriptCommonData> data = commonDataManager->GetData(resFind.second);
		data->DeleteValue(key);
	}

	return value();
}
value ScriptClientBase::Func_DeleteWholeAreaCommonData(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();

	std::string area = StringUtility::ConvertWideToMulti(argv->as_string());
	commonDataManager->Erase(area);

	return value();
}
value ScriptClientBase::Func_CreateCommonDataArea(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();

	std::string area = StringUtility::ConvertWideToMulti(argv->as_string());
	commonDataManager->CreateArea(area);

	return value();
}
value ScriptClientBase::Func_CopyCommonDataArea(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();

	std::string areaDest = StringUtility::ConvertWideToMulti(argv[0].as_string());
	std::string areaSrc = StringUtility::ConvertWideToMulti(argv[1].as_string());
	if (commonDataManager->IsExists(areaSrc).first)
		commonDataManager->CopyArea(areaDest, areaSrc);

	return value();
}
value ScriptClientBase::Func_IsCommonDataAreaExists(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();

	std::string area = StringUtility::ConvertWideToMulti(argv->as_string());
	bool res = commonDataManager->IsExists(area).first;

	return script->CreateBooleanValue(res);
}
value ScriptClientBase::Func_GetCommonDataAreaKeyList(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();

	std::vector<std::string> listKey;
	for (auto itr = commonDataManager->MapBegin(); itr != commonDataManager->MapEnd(); ++itr) {
		listKey.push_back(itr->first);
	}

	return script->CreateStringArrayValue(listKey);
}
value ScriptClientBase::Func_GetCommonDataValueKeyList(script_machine* machine, int argc, const value* argv) {
	ScriptClientBase* script = reinterpret_cast<ScriptClientBase*>(machine->data);
	ScriptCommonDataManager* commonDataManager = script->GetCommonDataManager();

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


/**********************************************************
//ScriptFileLineMap
**********************************************************/
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

/**********************************************************
//ScriptCommonDataManager
**********************************************************/
const std::string ScriptCommonDataManager::nameAreaDefault_ = "";
ScriptCommonDataManager::ScriptCommonDataManager() {
	defaultAreaIterator_ = CreateArea(nameAreaDefault_);
}
ScriptCommonDataManager::~ScriptCommonDataManager() {
	for (auto itr = mapData_.begin(); itr != mapData_.end(); ++itr) {
		itr->second->Clear();
		//delete itr->second;
	}
	mapData_.clear();
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
/**********************************************************
//ScriptCommonData
**********************************************************/
ScriptCommonData::ScriptCommonData() {}
ScriptCommonData::~ScriptCommonData() {}
void ScriptCommonData::Clear() {
	mapValue_.clear();
}
std::pair<bool, std::map<std::string, gstd::value>::iterator> ScriptCommonData::IsExists(std::string name) {
	auto itr = mapValue_.find(name);
	return std::make_pair(itr != mapValue_.end(), itr);
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
		gstd::value vDest = itrKey->second;
		vDest.unique();

		mapValue_.insert(std::make_pair(itrKey->first, vDest));
	}
}
void ScriptCommonData::ReadRecord(gstd::RecordBuffer& record) {
	mapValue_.clear();

	std::vector<std::string> listKey = record.GetKeyList();
	for (size_t iKey = 0; iKey < listKey.size(); iKey++) {
		std::string& key = listKey[iKey];
		std::string keyValSize = StringUtility::Format("%s_size", key.c_str());
		if (!record.IsExists(keyValSize)) continue;//サイズ自身がキー登録されている

		size_t valSize = 0U;
		record.GetRecord<size_t>(keyValSize, valSize);

		gstd::ByteBuffer buffer;
		buffer.SetSize(valSize);
		record.GetRecord(key, buffer.GetPointer(), valSize);
		gstd::value comVal = _ReadRecord(buffer);
		mapValue_[key] = comVal;
	}
}
gstd::value ScriptCommonData::_ReadRecord(gstd::ByteBuffer& buffer) {
	script_type_manager* scriptTypeManager = script_type_manager::get_instance();
	gstd::value res;

	type_data::type_kind kind;
	//kind = (type_data::type_kind)buffer.ReadInteger();
	{
		uint8_t _kind;
		buffer.Read(&_kind, sizeof(uint8_t));
		kind = (type_data::type_kind)_kind;
	}

	if (kind == type_data::type_kind::tk_real) {
		double data = buffer.ReadDouble();
		res = gstd::value(scriptTypeManager->get_real_type(), data);
	}
	else if (kind == type_data::type_kind::tk_char) {
		wchar_t data;
		buffer.Read(&data, sizeof(wchar_t));
		res = gstd::value(scriptTypeManager->get_char_type(), data);
	}
	else if (kind == type_data::type_kind::tk_boolean) {
		bool data = buffer.ReadBoolean();
		res = gstd::value(scriptTypeManager->get_boolean_type(), data);
	}
	else if (kind == type_data::type_kind::tk_array) {
		size_t arrayLength = buffer.ReadInteger();
		value v;
		for (size_t iArray = 0; iArray < arrayLength; iArray++) {
			value& arrayValue = _ReadRecord(buffer);
			v.append(scriptTypeManager->get_array_type(arrayValue.get_type()),
				arrayValue);
		}
		res = v;
	}

	return res;
}
void ScriptCommonData::WriteRecord(gstd::RecordBuffer& record) {
	for (auto itrValue = mapValue_.begin(); itrValue != mapValue_.end(); itrValue++) {
		const std::string& key = itrValue->first;
		gstd::value comVal = itrValue->second;

		if (comVal.has_data()) {
			gstd::ByteBuffer buffer;
			_WriteRecord(buffer, comVal);
			std::string keyValSize = StringUtility::Format("%s_size", key.c_str());
			size_t valSize = buffer.GetSize();
			record.SetRecord<size_t>(keyValSize, valSize);
			record.SetRecord(key, buffer.GetPointer(), valSize);
		}
	}
}
void ScriptCommonData::_WriteRecord(gstd::ByteBuffer& buffer, gstd::value& comValue) {
	type_data::type_kind kind = comValue.get_type()->get_kind();

	{
		uint8_t _kind = (uint8_t)kind;
		buffer.Write(&_kind, sizeof(uint8_t));
	}
	//buffer.WriteInteger((uint8_t)kind);

	if (kind == type_data::type_kind::tk_real) {
		buffer.WriteDouble(comValue.as_real());
	}
	else if (kind == type_data::type_kind::tk_char) {
		wchar_t wch = comValue.as_char();
		buffer.Write(&wch, sizeof(wchar_t));
	}
	else if (kind == type_data::type_kind::tk_boolean) {
		buffer.WriteBoolean(comValue.as_boolean());
	}
	else if (kind == type_data::type_kind::tk_array) {
		size_t arrayLength = comValue.length_as_array();
		buffer.WriteInteger(arrayLength);

		for (size_t iArray = 0; iArray < arrayLength; iArray++) {
			const value& arrayValue = comValue.index_as_array(iArray);
			_WriteRecord(buffer, const_cast<value&>(arrayValue));
		}
	}
}


/**********************************************************
//ScriptCommonDataPanel
**********************************************************/
ScriptCommonDataInfoPanel::ScriptCommonDataInfoPanel() {
	timeLastUpdate_ = 0;
	timeUpdateInterval_ = 1000;
	//commonDataManager_ = new ScriptCommonDataManager();
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
	wndListViewValue_.AddColumn(256, COL_VALUE, L"Value");

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
void ScriptCommonDataInfoPanel::Update(gstd::ref_count_ptr<ScriptCommonDataManager> commonDataManager) {
	if (!IsWindowVisible()) return;
	{
		Lock lock(lock_);
		//if (commonDataManager_) commonDataManager_->Clear();

		if (commonDataManager) {
			/*
			std::vector<std::string> listKey = commonDataManager->GetKeyList();
			for (int iKey = 0; iKey < listKey.size(); iKey++) {
				std::string area = listKey[iKey];
				ScriptCommonData::ptr dataSrc = commonDataManager->GetData(area);
				ScriptCommonData::ptr dataDest = new ScriptCommonData();
				dataDest->Copy(dataSrc);
				commonDataManager_->SetData(area, dataDest);
			}
			*/
			commonDataManager_ = commonDataManager;

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
		wndListViewArea_.SetText(iRow, COL_KEY, key);
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

	shared_ptr<ScriptCommonData> selectedArea = commonDataManager_->GetData(vecMapItr_[indexArea]);
	int iRow = 0;
	for (auto itr = selectedArea->MapBegin(); itr != selectedArea->MapEnd(); ++itr, ++iRow) {
		std::wstring key = StringUtility::ConvertMultiToWide(itr->first);
		gstd::value val = selectedArea->GetValue(itr);
		wndListViewValue_.SetText(iRow, COL_KEY, key);
		wndListViewValue_.SetText(iRow, COL_VALUE, val.as_string());
	}

	int countRow = wndListViewValue_.GetRowCount();
	for (; iRow < countRow; ++iRow)
		wndListViewValue_.DeleteRow(iRow);
}

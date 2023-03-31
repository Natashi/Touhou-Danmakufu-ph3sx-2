#include "source/GcLib/pch.h"

#include "GstdUtility.hpp"
#include "Logger.hpp"

using namespace gstd;

//*******************************************************************
//SystemUtility
//*******************************************************************

//Vector compile checks
#ifdef __L_MATH_VECTORIZE
#if !defined(_XM_X64_) && !defined(_XM_X86_)
#if defined(_M_AMD64) || defined(_AMD64_)
#define _XM_X64_
#elif defined(_M_IX86) || defined(_X86_)
#define _XM_X86_
#endif
#endif

#if (!defined(_XM_X86_) && !defined(_XM_X64_))
#error Compilation on vectorized mode requires x86 intrinsics
#endif
#endif

void SystemUtility::TestCpuSupportSIMD() {
#ifdef __L_MATH_VECTORIZE
	bool hasSSE = true;
	bool hasSSE2 = true;
	bool hasSSE3 = true;
	bool hasSSE4_1 = true;

	struct _Four { int m[4]; };

	std::bitset<32> f_1_ECX;
	std::bitset<32> f_1_EDX;
	std::bitset<32> f_7_EBX;
	std::bitset<32> f_7_ECX;
	std::vector<_Four> cpuData;

	memset(&f_1_ECX[0], 0, 32 / 8);
	memset(&f_1_EDX[0], 0, 32 / 8);
	memset(&f_7_EBX[0], 0, 32 / 8);
	memset(&f_7_ECX[0], 0, 32 / 8);

	int cpui[4];
	__cpuid(cpui, 0);
	int nIds = cpui[0];

	for (int i = 0; i <= nIds; ++i) {
		__cpuidex(cpui, i, 0);
		_Four arr;
		memcpy(&arr, cpui, sizeof(cpui));
		cpuData.push_back(arr);
	}

	// load bitset with flags for function 0x00000001
	if (nIds >= 1) {
		f_1_ECX = cpuData[1].m[2];
		f_1_EDX = cpuData[1].m[3];
	}
	// load bitset with flags for function 0x00000007
	if (nIds >= 7) {
		f_7_EBX = cpuData[7].m[1];
		f_7_ECX = cpuData[7].m[2];
	}

	//Test SSE
	hasSSE = f_1_EDX[25];
	hasSSE2 = f_1_EDX[26];
	hasSSE3 = f_1_ECX[0];
	hasSSE4_1 = f_1_ECX[19];

	if (!hasSSE || !hasSSE2 || !hasSSE3 || !hasSSE4_1) {
		std::string err = "The game cannot start because your CPU lacks the required vector instruction sets(s):\r\n  ";
		if (!hasSSE)	err += " SSE";
		if (!hasSSE2)	err += " SSE2";
		if (!hasSSE3)	err += " SSE3";
		if (!hasSSE4_1)	err += " SSE4.1";
		throw wexception(err);
	}
#endif
}

std::wstring SystemUtility::GetSystemFontFilePath(const std::wstring& faceName) {
	static const LPWSTR fontRegistryPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";

	HKEY hKey;

	LONG result = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, fontRegistryPath, 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS)
		return L"";

	DWORD maxValueNameSize, maxValueDataSize;
	result = ::RegQueryInfoKeyW(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
	if (result != ERROR_SUCCESS)
		return L"";

	std::wstring wsFontFile;
	{
		DWORD valueIndex = 0;
		DWORD valueNameSize, valueDataSize, valueType;

		std::wstring valueName;
		valueName.resize(maxValueNameSize);

		std::vector<byte> valueData(maxValueDataSize);

		do {
			wsFontFile.clear();
			valueDataSize = maxValueDataSize;
			valueNameSize = maxValueNameSize;

			result = ::RegEnumValueW(hKey, valueIndex, (LPWSTR)valueName.c_str(), &valueNameSize,
				0, &valueType, (LPBYTE)valueData.data(), &valueDataSize);
			valueIndex++;
			if (result != ERROR_SUCCESS || valueType != REG_SZ)
				continue;

			// Found a match
			if (valueName.find(faceName) != std::wstring::npos) {
				//if (_wcsnicmp(faceName.c_str(), valueName.c_str(), faceName.length()) == 0) {
				wsFontFile.assign((LPWSTR)valueData.data(), valueDataSize);
				break;
			}
		} while (result != ERROR_NO_MORE_ITEMS);
	}

	::RegCloseKey(hKey);

	if (wsFontFile.empty())
		return L"";

	WCHAR winDir[MAX_PATH];
	::GetWindowsDirectoryW(winDir, MAX_PATH);

	std::wstringstream ss;
	ss << winDir << "\\Fonts\\" << wsFontFile;
	wsFontFile = ss.str();

	return std::wstring(wsFontFile.begin(), wsFontFile.end());
}

//*******************************************************************
//AnyMap
//*******************************************************************
AnyMap::AnyMap() {
}
AnyMap::~AnyMap() {
}

//*******************************************************************
//Encoding
//*******************************************************************
const byte Encoding::BOM_UTF16LE[] = { 0xFF, 0xFE };
const byte Encoding::BOM_UTF16BE[] = { 0xFE, 0xFF };
const byte Encoding::BOM_UTF8[] = { 0xEF, 0xBB, 0xBF };
Encoding::Type Encoding::Detect(const char* data, size_t dataSize) {
	if (dataSize < 2U) return Type::UTF8;
	if (memcmp(data, BOM_UTF16LE, 2) == 0) return Type::UTF16LE;
	else if (memcmp(data, BOM_UTF16BE, 2) == 0) return Type::UTF16BE;
	else if (dataSize > 2U) {
		if (memcmp(data, BOM_UTF8, 3) == 0) return Type::UTF8BOM;
	}
	return Type::UTF8;
}
size_t Encoding::GetBomSize(const char* data, size_t dataSize) {
	if (dataSize < 2) return 0U;
	if (memcmp(data, BOM_UTF16LE, 2) == 0 || memcmp(data, BOM_UTF16BE, 2) == 0)
		return 2U;
	else if (dataSize > 2U) {
		if (memcmp(data, BOM_UTF8, 3) == 0) return 3U;
	}
	return 0U;
}
size_t Encoding::GetBomSize(Type encoding) {
	switch (encoding) {
	case Type::UTF16LE:
	case Type::UTF16BE:
		return 2U;
	case Type::UTF8BOM:
		return 3U;
	}
	return 0U;
}
const byte* Encoding::GetBom(Type encoding) {
	switch (encoding) {
	case Type::UTF16LE:
		return BOM_UTF16LE;
	case Type::UTF16BE:
		return BOM_UTF16BE;
	case Type::UTF8BOM:
		return BOM_UTF8;
	}
	return nullptr;
}
size_t Encoding::GetCharSize(Type encoding) {
	switch (encoding) {
	case Type::UTF16LE:
	case Type::UTF16BE:
		return 2U;
	}
	return 1U;
}
const char* Encoding::StringRepresentation(Type encoding) {
	switch (encoding) {
	case Type::UTF16LE:
		return "UTF-16 LE";
	case Type::UTF16BE:
		return "UTF-16 BE";
	case Type::UTF8BOM:
		return "UTF-8-BOM";
	}
	return "UTF-8";
}
const wchar_t* Encoding::WStringRepresentation(Type encoding) {
	switch (encoding) {
	case Type::UTF16LE:
		return L"UTF-16 LE";
	case Type::UTF16BE:
		return L"UTF-16 BE";
	case Type::UTF8BOM:
		return L"UTF-8-BOM";
	}
	return L"UTF-8";
}
size_t Encoding::GetMultibyteSize(const char* data) {
	const char* begin = data;
	if (((byte)data[0] > 0xc0) && ((data[1] & 0xc0) == 0x80)) {
		do {
			data += 1;
		} while ((data[0] & 0xc0) == 0x80);
	}
	return (data - begin);
}
wchar_t Encoding::BytesToWChar(const char* data, Type encoding) {
	if (encoding == Encoding::UTF16LE || encoding == Encoding::UTF16BE) {
		wchar_t res = *(wchar_t*)data;
		if (encoding == Encoding::UTF16BE)
			res = (res >> 8) | (res << 8);
		return res;
	}
	return (wchar_t)(*data);
}
std::string Encoding::BytesToString(const char* pBegin, const char* pEnd, Type encoding) {
	if (GetCharSize(encoding) == 1) {
		return std::string(pBegin, pEnd);
	}
	else {
		std::wstring res;
		while (pBegin < pEnd) {
			res += BytesToWChar(pBegin, encoding);
			pBegin += Encoding::GetCharSize(encoding);
		}
		return StringUtility::ConvertWideToMulti(res);
	}
}
std::string Encoding::BytesToString(const char* pBegin, size_t count, Type encoding) {
	return BytesToString(pBegin, (const char*)(pBegin + count), encoding);
}
std::wstring Encoding::BytesToWString(const char* pBegin, const char* pEnd, Type encoding) {
	if (GetCharSize(encoding) == 1) {
		return StringUtility::ConvertMultiToWide(std::string(pBegin, pEnd));
	}
	else {
		std::wstring res;
		while (pBegin < pEnd) {
			res += BytesToWChar(pBegin, encoding);
			pBegin += Encoding::GetCharSize(encoding);
		}
		return res;
	}
}
std::wstring Encoding::BytesToWString(const char* pBegin, size_t count, Type encoding) {
	return BytesToWString(pBegin, (const char*)(pBegin + count), encoding);
}

//*******************************************************************
//StringUtility
//*******************************************************************
std::wstring StringUtility::ConvertMultiToWide(const std::string& str, int code) {
	if (str.size() == 0) return L"";

	size_t sizeWide = MultiByteToWideChar(code, 0, str.c_str(), -1, nullptr, 0);
	if (sizeWide == 0) return L"";

	std::wstring wstr;
	wstr.resize(sizeWide - 1);
	MultiByteToWideChar(code, 0, str.c_str(), str.size(), &wstr[0], sizeWide - 1);

	return wstr;
}
std::string StringUtility::ConvertWideToMulti(const std::wstring& wstr, int code) {
	if (wstr.size() == 0) return "";

	size_t sizeMulti = WideCharToMultiByte(code, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (sizeMulti == 0) return "";

	std::string str;
	str.resize(sizeMulti - 1);
	WideCharToMultiByte(code, 0, wstr.c_str(), wstr.size(), &str[0], sizeMulti - 1, nullptr, nullptr);

	return str;
}
std::wstring StringUtility::ConvertMultiToWide(std::vector<char>& buf, int code) {
	if (buf.size() == 0) return L"";

	std::string str = std::string(buf.data(), buf.data() + buf.size());
	return ConvertMultiToWide(str, code);
}
std::string StringUtility::ConvertWideToMulti(std::vector<char>& buf, int code) {
	if (buf.size() == 0) return "";

	std::wstring wstr = std::wstring((wchar_t*)buf.data(), buf.size() / 2U);
	return ConvertWideToMulti(wstr, code);
}
size_t StringUtility::ConvertMultiToWide(char* mbstr, size_t mbcount, std::vector<char>& wres, int code) {
	if (mbcount == 0) return 0;

	size_t sizeWide = MultiByteToWideChar(code, 0, mbstr, mbcount, nullptr, 0);
	if (sizeWide == 0) return 0;

	wres.resize(sizeWide * 2U);
	MultiByteToWideChar(code, 0, mbstr, mbcount, (wchar_t*)&wres[0], sizeWide);

	return sizeWide;
}
size_t StringUtility::ConvertWideToMulti(wchar_t* wstr, size_t wcount, std::vector<char>& mbres, int code) {
	if (wcount == 0) return 0;

	size_t sizeMulti = WideCharToMultiByte(code, 0, wstr, wcount, nullptr, 0, nullptr, nullptr);
	if (sizeMulti == 0) return 0;

	mbres.resize(sizeMulti);
	WideCharToMultiByte(code, 0, wstr, wcount, &mbres[0], sizeMulti, nullptr, nullptr);

	return sizeMulti;
}

//----------------------------------------------------------------

#define CRET(x) res = x; break;
char StringUtility::ParseEscapeSequence(const char* str, char** pEnd) {
	char res;
	if ((res = *(str++)) != '\\')
		goto lab_ret;

	char lead = *(str++);
	switch (res = lead) {
	case '\"': CRET('\"');
	case '\'': CRET('\'');
	case '\\': CRET('\\');
	case 'f': CRET('\f');
	case 'n': CRET('\n');
	case 'r': CRET('\r');
	case 't': CRET('\t');
	case 'v': CRET('\v');
	case '0': CRET('\0');
	case 'x':
	{
		if (std::isxdigit(*str)) {
			char* end = nullptr;
			res = (char)strtol(str, &end, 16);
			str = end;
		}
	}
	}

lab_ret:
	if (pEnd) *pEnd = (char*)str;
	return res;
}
wchar_t StringUtility::ParseEscapeSequence(const wchar_t* wstr, wchar_t** pEnd) {
	wchar_t res;
	if ((res = *(wstr++)) != '\\')
		goto lab_ret;

	wchar_t lead = *(wstr++);
	switch (res = lead) {
	case '\"': CRET('\"');
	case '\'': CRET('\'');
	case '\\': CRET('\\');
	case 'f': CRET('\f');
	case 'n': CRET('\n');
	case 'r': CRET('\r');
	case 't': CRET('\t');
	case 'v': CRET('\v');
	case '0': CRET('\0');
	case 'x':
	{
		if (std::isxdigit(*wstr)) {
			wchar_t* end = nullptr;
			res = (wchar_t)wcstol(wstr, &end, 16);
			wstr = end;
		}
	}
	}

lab_ret:
	if (pEnd) *pEnd = (wchar_t*)wstr;
	return res;
}
#undef CRET

std::string StringUtility::ParseStringWithEscape(const std::string& str) {
	std::string res;
	const char* pStr = str.data();
	const char* pEnd = str.data() + str.size();
	while (pStr < pEnd) {
		res += ParseEscapeSequence(pStr, (char**)&pStr);
	}
	return res;
}
std::wstring StringUtility::ParseStringWithEscape(const std::wstring& wstr) {
	std::wstring res;
	const wchar_t* pStr = wstr.data();
	const wchar_t* pEnd = wstr.data() + wstr.size();
	while (pStr < pEnd) {
		res += ParseEscapeSequence(pStr, (wchar_t**)&pStr);
	}
	return res;
}

//----------------------------------------------------------------

template<class T_STR>
void StringUtility::Split(const T_STR& str, const T_STR& delim, std::vector<T_STR>& res) {
	size_t itrBegin = 0;
	size_t itrFind = 0;
	while ((itrFind = str.find_first_of(delim, itrBegin)) != std::string::npos) {
		res.push_back(str.substr(itrBegin, itrFind - itrBegin));
		itrBegin = itrFind + 1;
	}
	res.push_back(str.substr(itrBegin));
}
template<class T_STR>
void StringUtility::SplitPattern(const T_STR& str, const T_STR& pattern, std::vector<T_STR>& res) {
	size_t itrBegin = 0;
	size_t itrFind = 0;
	while ((itrFind = str.find(pattern, itrBegin)) != std::string::npos) {
		res.push_back(str.substr(itrBegin, itrFind - itrBegin));
		itrBegin = itrFind + pattern.size();
	}
	res.push_back(str.substr(itrBegin));
}

std::vector<std::string> StringUtility::Split(const std::string& str, const std::string& delim) {
	std::vector<std::string> res;
	Split<std::string>(str, delim, res);
	return res;
}
std::vector<std::string> StringUtility::SplitPattern(const std::string& str, const std::string& pattern) {
	std::vector<std::string> res;
	SplitPattern<std::string>(str, pattern, res);
	return res;
}
std::string StringUtility::Format(const char* str, ...) {
	va_list	vl;
	va_start(vl, str);
	std::string res = StringUtility::Format(str, vl);
	va_end(vl);
	return res;
}
std::string StringUtility::Format(const char* str, va_list va) {
	//The size returned by _vsnprintf does NOT include null terminator
	int size = _vsnprintf(nullptr, 0U, str, va);
	std::string res;
	if (size > 0) {
		res.resize(size + 1);
		_vsnprintf((char*)res.c_str(), res.size(), str, va);
		res.pop_back();	//Don't include the null terminator
	}
	return res;
}

size_t StringUtility::CountCharacter(const std::string& str, char c) {
	size_t count = 0;
	const char* pbuf = &str[0];
	const char* ebuf = &str.back();
	while (pbuf <= ebuf) {
		if (*pbuf == c)
			count++;
		pbuf++;
	}
	return count;
}
size_t StringUtility::CountCharacter(std::vector<char>& str, char c) {
	if (str.size() == 0) return 0;

	Encoding::Type encoding = Encoding::Detect(str.data(), str.size());

	size_t count = 0;
	char* pBegin = str.data();
	char* pEnd = pBegin + str.size();
	while (pBegin < pEnd) {
		if (Encoding::BytesToWChar(pBegin, encoding) == c)
			++count;
		pBegin += Encoding::GetCharSize(encoding);
	}
	return count;

}
int StringUtility::ToInteger(const std::string& s) {
	return atoi(s.c_str());
}
double StringUtility::ToDouble(const std::string& s) {
	return atof(s.c_str());
}
std::string StringUtility::Replace(const std::string& source, const std::string& pattern, const std::string& placement) {
	return ReplaceAll(source, pattern, placement, 1);
}
std::string StringUtility::ReplaceAll(const std::string& source, const std::string& pattern, const std::string& placement,
	size_t replaceCount, size_t start, size_t end)
{
	std::string result;
	if (end == 0) end = source.size();
	std::string::size_type pos_before = 0;
	std::string::size_type pos = start;
	std::string::size_type len = pattern.size();

	size_t count = 0;
	while ((pos = source.find(pattern, pos)) != std::string::npos) {
		if (pos > 0) {
			char ch = source[pos - 1];
			if (pos >= end) break;
		}

		result.append(source, pos_before, pos - pos_before);
		result.append(placement);
		pos += len;
		pos_before = pos;

		count++;
		if (count >= replaceCount) break;
	}
	result.append(source, pos_before, source.size() - pos_before);
	return result;
}
std::string StringUtility::ReplaceAll(const std::string& source, char pattern, char placement) {
	std::string result;
	if (placement != '\0') {
		result = source;
		for (char& c : result) {
			if (c == pattern)
				c = placement;
		}
	}
	else {		//Deleting all nulls from the string
		for (const char& c : source) {
			if (c != pattern)
				result.push_back(c);
		}
	}
	return result;
}
std::string StringUtility::Slice(const std::string& s, size_t length) {
	if (s.size() > 0)
		length = std::min(s.size() - 1, length);
	else length = 0;
	return s.substr(0, length);
}
std::string StringUtility::Trim(const std::string& str) {
	if (str.size() == 0) return str;

	std::string res = str;

	//Trim left
	res.erase(res.begin(), std::find_if_not(res.begin(), res.end(), [&](int ch) { return std::isspace(ch); }));
	//Trim right
	res.erase(std::find_if_not(res.rbegin(), res.rend(), [](int ch) { return std::isspace(ch); }).base(), res.end());

	return res;
}

template<class _Iter>
std::string StringUtility::Join(_Iter begin, _Iter end, const std::string& join) {
	std::string res;
	size_t i = 0;
	for (auto itr = begin; itr != end; ++i) {
		res += *itr;
		if ((++itr) != end)
			res += join;
	}
	return res;
}
std::string StringUtility::Join(const std::vector<std::string>& strs, const std::string& join) {
	return Join(strs.begin(), strs.end(), join);
}

std::string StringUtility::FromGuid(const GUID* guid) {
	return Format(
		"{%08x-%04x-%04x-%04x-%04x%08x}",
		guid->Data1, guid->Data2, guid->Data3,
		((uint16_t*)guid->Data4)[0], ((uint16_t*)guid->Data4)[1],
		((uint32_t*)guid->Data4)[1]);
}

//----------------------------------------------------------------

std::vector<std::wstring> StringUtility::Split(const std::wstring& str, const std::wstring& delim) {
	std::vector<std::wstring> res;
	Split<std::wstring>(str, delim, res);
	return res;
}
std::vector<std::wstring> StringUtility::SplitPattern(const std::wstring& str, const std::wstring& pattern) {
	std::vector<std::wstring> res;
	SplitPattern<std::wstring>(str, pattern, res);
	return res;
}
std::wstring StringUtility::Format(const wchar_t* str, ...) {
	va_list	vl;
	va_start(vl, str);
	std::wstring res = StringUtility::Format(str, vl);
	va_end(vl);
	return res;
}
std::wstring StringUtility::Format(const wchar_t* str, va_list va) {
	//The size returned by _vsnwprintf does NOT include null terminator
	int size = _vsnwprintf(nullptr, 0U, str, va);
	std::wstring res;
	if (size > 0) {
		res.resize(size + 1);
		_vsnwprintf((wchar_t*)res.c_str(), res.size(), str, va);
		res.pop_back();
	}
	return res;
}
std::wstring StringUtility::FormatToWide(const char* str, ...) {
	va_list	vl;
	va_start(vl, str);
	std::string res = StringUtility::Format(str, vl);
	va_end(vl);
	return StringUtility::ConvertMultiToWide(res);
}

size_t StringUtility::CountCharacter(const std::wstring& str, wchar_t c) {
	size_t count = 0;
	const wchar_t* pbuf = &str[0];
	const wchar_t* ebuf = &str.back();
	while (pbuf <= ebuf) {
		if (*pbuf == c)
			count++;
	}
	return count;
}
int StringUtility::ToInteger(const std::wstring& s) {
	return _wtoi(s.c_str());
}
double StringUtility::ToDouble(const std::wstring& s) {
	wchar_t* stopscan;
	return wcstod(s.c_str(), &stopscan);
	//return _wtof(s.c_str());
}
std::wstring StringUtility::Replace(const std::wstring& source, const std::wstring& pattern, const std::wstring& placement) {
	return ReplaceAll(source, pattern, placement, 1);
}
std::wstring StringUtility::ReplaceAll(const std::wstring& source, const std::wstring& pattern, const std::wstring& placement,
	size_t replaceCount, size_t start, size_t end)
{
	std::wstring result;
	if (end == 0) end = source.size();
	std::wstring::size_type pos_before = 0;
	std::wstring::size_type pos = start;
	std::wstring::size_type len = pattern.size();

	size_t count = 0;
	while ((pos = source.find(pattern, pos)) != std::wstring::npos) {
		result.append(source, pos_before, pos - pos_before);
		result.append(placement);
		pos += len;
		pos_before = pos;

		count++;
		if (count >= replaceCount) break;
	}
	result.append(source, pos_before, source.size() - pos_before);
	return result;
}
std::wstring StringUtility::ReplaceAll(const std::wstring& source, wchar_t pattern, wchar_t placement) {
	std::wstring result;
	if (placement != L'\0') {
		result = source;
		for (wchar_t& c : result) {
			if (c == pattern)
				c = placement;
		}
	}
	else {		//Deleting all nulls from the string
		for (const wchar_t& c : source) {
			if (c != pattern)
				result.push_back(c);
		}
	}
	return result;
}
std::wstring StringUtility::Slice(const std::wstring& s, size_t length) {
	if (s.size() > 0)
		length = std::min(s.size() - 1, length);
	else length = 0;
	return s.substr(0, length);
}
std::wstring StringUtility::Trim(const std::wstring& str) {
	if (str.size() == 0) return str;

	std::wstring res = str;

	//Trim left
	res.erase(res.begin(), std::find_if_not(res.begin(), res.end(), [&](wint_t ch) { return std::iswspace(ch); }));
	//Trim right
	res.erase(std::find_if_not(res.rbegin(), res.rend(), [](wint_t ch) { return std::iswspace(ch); }).base(), res.end());

	return res;
}

template<class _Iter>
std::wstring StringUtility::Join(_Iter begin, _Iter end, const std::wstring& join) {
	std::wstring res;
	size_t i = 0;
	for (auto itr = begin; itr != end; ++i) {
		res += *itr;
		if ((++itr) != end)
			res += join;
	}
	return res;
}
std::wstring StringUtility::Join(const std::vector<std::wstring>& strs, const std::wstring& join) {
	return Join(strs.begin(), strs.end(), join);
}

size_t StringUtility::CountAsciiSizeCharacter(const std::wstring& str) {
	if (str.size() == 0) return 0;

	size_t wcount = str.size();
	WORD* listType = new WORD[wcount];
	GetStringTypeEx(0, CT_CTYPE3, str.c_str(), wcount, listType);

	size_t res = 0;
	for (size_t iType = 0; iType < wcount; iType++) {
		WORD type = listType[iType];
		if ((type & C3_HALFWIDTH) == C3_HALFWIDTH) {
			res++;
		}
		else {
			res += 2;
		}
	}

	delete[] listType;
	return res;
}

//*******************************************************************
//ErrorUtility
//*******************************************************************
std::wstring ErrorUtility::GetLastErrorMessage(DWORD error) {
	LPVOID lpMsgBuf;
	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // 既定の言語
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);
	std::wstring res = (wchar_t*)lpMsgBuf;
	::LocalFree(lpMsgBuf);
	return res;
}
std::wstring ErrorUtility::GetErrorMessage(int type) {
	std::wstring res = L"Unknown error";
	if (type == ERROR_FILE_NOTFOUND)
		res = L"Cannot open file";
	else if (type == ERROR_PARSE)
		res = L"Parse failed";
	else if (type == ERROR_END_OF_FILE)
		res = L"End of file error";
	else if (type == ERROR_OUTOFRANGE_INDEX)
		res = L"Invalid index";
	return res;
}
std::wstring ErrorUtility::GetFileNotFoundErrorMessage(const std::wstring& path, bool bErrorExtended) {
	std::wstring res = GetErrorMessage(ERROR_FILE_NOTFOUND);
	res += StringUtility::Format(L" \"%s\"", path.c_str());
	if (bErrorExtended) {
		//auto sysErr = std::system_error(errno, std::system_category());
		//res += StringUtility::FormatToWide("\r\n  System: %s (code=%d)", sysErr.code().message(), sysErr.code().value());
		res += StringUtility::Format(L"\r\n    System: %s", File::GetLastError().c_str());
	}
	return res;
}
std::wstring ErrorUtility::GetParseErrorMessage(const std::wstring& path, int line, const std::wstring& what) {
	std::wstring res = GetErrorMessage(ERROR_PARSE);
	res += StringUtility::Format(L" path[%s] line[%d] msg[%s]", path.c_str(), line, what.c_str());
	return res;
}

//*******************************************************************
//ByteOrder
//*******************************************************************
void ByteOrder::Reverse(LPVOID buf, DWORD size) {
	if (size < 2) return;

	byte* pStart = reinterpret_cast<byte*>(buf);
	byte* pEnd = pStart + size - 1;

	for (; pStart < pEnd;) {
		byte temp = *pStart;
		*pStart = *pEnd;
		*pEnd = temp;

		pStart++;
		pEnd--;
	}
}

//*******************************************************************
//PathProperty
//*******************************************************************
std::wstring PathProperty::GetFileDirectory(const std::wstring& path) {
#ifdef __L_STD_FILESYSTEM
	path_t p(path);
	p = p.parent_path();
	return p.wstring() + L'/';
#else
	wchar_t pDrive[_MAX_PATH];
	wchar_t pDir[_MAX_PATH];
	_wsplitpath_s(path.c_str(), pDrive, _MAX_PATH, pDir, _MAX_PATH, nullptr, 0, nullptr, 0);
	return std::wstring(pDrive) + std::wstring(pDir);
#endif
}
std::wstring PathProperty::GetDirectoryName(const std::wstring& path) {
	//Returns the name of the topmost directory.
#ifdef __L_STD_FILESYSTEM
	std::wstring dirChain = ReplaceYenToSlash(path_t(path).parent_path());
	std::vector<std::wstring> listDir = StringUtility::Split(dirChain, L"/");
	return listDir.back();
#else
	std::wstring dir = GetFileDirectory(path);
	dir = StringUtility::ReplaceAll(dir, L"\\", L"/");
	std::vector<std::wstring> strs = StringUtility::Split(dir, L"/");
	return strs[strs.size() - 1];
#endif
}
std::wstring PathProperty::GetFileName(const std::wstring& path) {
#ifdef __L_STD_FILESYSTEM
	path_t p(path);
	return p.filename();
#else
	wchar_t pFileName[_MAX_PATH];
	wchar_t pExt[_MAX_PATH];
	_wsplitpath_s(path.c_str(), nullptr, 0, nullptr, 0, pFileName, _MAX_PATH, pExt, _MAX_PATH);
	return std::wstring(pFileName) + std::wstring(pExt);
#endif
}
std::wstring PathProperty::GetDriveName(const std::wstring& path) {
#ifdef __L_STD_FILESYSTEM
	path_t p(path);
	return (p.root_name() / p.root_directory());
#else
	wchar_t pDrive[_MAX_PATH];
	_wsplitpath_s(path.c_str(), pDrive, _MAX_PATH, nullptr, 0, nullptr, 0, nullptr, 0);
	return std::wstring(pDrive);
#endif
}
std::wstring PathProperty::GetFileNameWithoutExtension(const std::wstring& path) {
#ifdef __L_STD_FILESYSTEM
	path_t p(path);
	return p.stem();
#else
	wchar_t pFileName[_MAX_PATH];
	_wsplitpath_s(path.c_str(), nullptr, 0, nullptr, 0, pFileName, _MAX_PATH, nullptr, 0);
	return std::wstring(pFileName);
#endif
}

std::wstring PathProperty::GetFileExtension(const std::wstring& path) {
#ifdef __L_STD_FILESYSTEM
	path_t p(path);
	return p.extension();
#else
	wchar_t pExt[_MAX_PATH];
	_wsplitpath_s(path.c_str(), nullptr, 0, nullptr, 0, nullptr, 0, pExt, _MAX_PATH);
	return std::wstring(pExt);
#endif
}
const std::wstring& PathProperty::GetModuleName() {
	static std::wstring moduleName;
	if (moduleName.size() == 0) {
		wchar_t modulePath[_MAX_PATH];
		ZeroMemory(modulePath, sizeof(modulePath));
		GetModuleFileName(NULL, modulePath, _MAX_PATH - 1);
		moduleName = GetFileNameWithoutExtension(std::wstring(modulePath));
	}
	return moduleName;
}
const std::wstring& PathProperty::GetModuleDirectory() {
#ifdef __L_STD_FILESYSTEM
	static std::wstring moduleDir;
	if (moduleDir.size() == 0) {
		wchar_t modulePath[_MAX_PATH];
		ZeroMemory(modulePath, sizeof(modulePath));
		GetModuleFileName(NULL, modulePath, _MAX_PATH - 1);
		moduleDir = ReplaceYenToSlash(stdfs::path(modulePath).parent_path());
		moduleDir = GetUnique(moduleDir) + L"/";
	}
	return moduleDir;
#else
	wchar_t modulePath[_MAX_PATH];
	ZeroMemory(modulePath, sizeof(modulePath));
	GetModuleFileName(NULL, modulePath, _MAX_PATH - 1);
	return GetFileDirectory(std::wstring(modulePath));
#endif
}
std::wstring PathProperty::GetDirectoryWithoutModuleDirectory(const std::wstring& path) {
	std::wstring res = GetFileDirectory(path);
	const std::wstring& dirModule = GetModuleDirectory();
	if (res.find(dirModule) != std::wstring::npos) {
		res = res.substr(dirModule.size());
	}
	return res;
}
std::wstring PathProperty::GetPathWithoutModuleDirectory(const std::wstring& path) {
	const std::wstring& dirModule = GetModuleDirectory();
	std::wstring res = GetUnique(path);
	if (res.find(dirModule) != std::wstring::npos) {
		res = res.substr(dirModule.size());
	}
	return res;
}
std::wstring PathProperty::ReduceModuleDirectory(const std::wstring& path, std::wstring rep) {
	const std::wstring& dirModule = GetModuleDirectory();
	std::wstring res = GetUnique(path);
	if (res.find(dirModule) != std::wstring::npos) {
		res = res.substr(dirModule.size());
		res = rep + res;
	}
	return res;
}
std::wstring PathProperty::GetRelativeDirectory(const std::wstring& from, const std::wstring& to) {
#ifdef __L_STD_FILESYSTEM
	std::error_code err;
	path_t p = stdfs::relative(from, to, err);

	std::wstring res;
	if (err.value() != 0)
		res = GetFileDirectory(p);
#else
	wchar_t path[_MAX_PATH];
	BOOL b = PathRelativePathTo(path, from.c_str(), FILE_ATTRIBUTE_DIRECTORY, to.c_str(), FILE_ATTRIBUTE_DIRECTORY);

	std::wstring res;
	if (b) {
		res = GetFileDirectory(path);
	}
#endif
	return res;
}
std::wstring PathProperty::ExtendRelativeToFull(const std::wstring& dir, std::wstring path) {
	path = ReplaceYenToSlash(path);
	if (path.size() >= 2) {
		if (memcmp(&path[0], L"./", sizeof(wchar_t) * 2U) == 0) {
			path = path.substr(2);
			path = dir + path;
		}
	}

	std::wstring drive = GetDriveName(path);
	if (drive.size() == 0) {
		path = GetModuleDirectory() + path;
	}

	return path;
}
std::wstring PathProperty::ReplaceYenToSlash(const std::wstring& path) {
	return StringUtility::ReplaceAll(path, L'\\', L'/');
}
std::wstring PathProperty::AppendSlash(const std::wstring& path) {
	if (path.size() == 0 || path.back() == L'/')
		return path;
	return path + L'/';
}
std::wstring PathProperty::Canonicalize(const std::wstring& srcPath) {
#ifdef __L_STD_FILESYSTEM
	path_t p(srcPath);
	std::wstring res = stdfs::weakly_canonical(p);
#else
	wchar_t destPath[_MAX_PATH];
	PathCanonicalize(destPath, srcPath.c_str());
	std::wstring res(destPath);
#endif
	return res;
}
std::wstring PathProperty::MakePreferred(const std::wstring& srcPath) {
#ifdef __L_STD_FILESYSTEM
	path_t p(srcPath);
	std::wstring res = p.make_preferred();
#else
	std::wstring res = StringUtility::ReplaceAll(path, L"/", L"\\");
#endif
	return res;
}
std::wstring PathProperty::GetUnique(const std::wstring& srcPath) {
	std::wstring p = Canonicalize(srcPath);
	return ReplaceYenToSlash(p);
}

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)
//*******************************************************************
//Scanner
//*******************************************************************
Scanner::Scanner(char* str, size_t size) {
	std::vector<char> buf;
	buf.resize(size);
	memcpy(&buf[0], str, size);
	buf.push_back('\0');
	this->Scanner::Scanner(buf);
}
Scanner::Scanner(const std::string& str) {
	std::vector<char> buf;
	buf.resize(str.size() + 1);
	memcpy(&buf[0], str.c_str(), str.size() + 1);
	this->Scanner::Scanner(buf);
}
Scanner::Scanner(const std::wstring& wstr) {
	std::vector<char> buf;
	size_t textSize = wstr.size() * sizeof(wchar_t);
	buf.resize(textSize + 4);
	memcpy(&buf[0], &Encoding::BOM_UTF16LE[0], 2);
	memcpy(&buf[2], wstr.c_str(), textSize + 2);
	this->Scanner::Scanner(buf);
}
Scanner::Scanner(std::vector<char>& buf) {
	bPermitSignNumber_ = true;
	buffer_ = buf;
	pointer_ = 0;
	textStartPointer_ = 0;

	bufStr_ = nullptr;

	typeEncoding_ = Encoding::Detect(buf.data(), buf.size());
	textStartPointer_ = Encoding::GetBomSize(typeEncoding_);

	buffer_.push_back(0);
	if (typeEncoding_ == Encoding::Type::UTF16LE || typeEncoding_ == Encoding::Type::UTF16BE) {
		buffer_.push_back(0);
	}

	bufStr_ = &buffer_[0];

	SetPointerBegin();
}
Scanner::~Scanner() {
}
wchar_t Scanner::_CurrentChar() {
	wchar_t wch = Encoding::BytesToWChar(&buffer_[pointer_], typeEncoding_);
	return wch;
}

wchar_t Scanner::_NextChar() {
	if (!HasNext()) {
		/*
		Logger::WriteTop(L"終端異常発生->");

		int size = buffer_.size() - textStartPointer_;
		std::wstring source = GetString(textStartPointer_, size);
		std::wstring target = StringUtility::Format(L"字句解析対象 -> \r\n%s...", source.c_str());
		Logger::WriteTop(target);

		int index = 1;
		std::list<Token>::iterator itr;
		for (itr = listDebugToken_.begin(); itr != listDebugToken_.end(); itr++) {
			Token token = *itr;
			std::wstring log = StringUtility::Format(L"  %2d token -> type=%2d, element=%s, start=%d, end=%d",
				index, token.GetType(), token.GetElement().c_str(), token.GetStartPointer(), token.GetEndPointer());
			Logger::WriteTop(log);
			index++;
		}
		*/

		_RaiseError(L"Scanner: End-of-file already reached.");
		//bufStr_ = nullptr;
		//return L'\0';
	}

	pointer_ += Encoding::GetCharSize(typeEncoding_);
	bufStr_ = &buffer_[pointer_];

	return _CurrentChar();
}
void Scanner::_SkipComment() {
	while (true) {
		int posStart = pointer_;
		_SkipSpace();

		wchar_t ch = _CurrentChar();

		if (ch == L'/') {			//Comment initial
			int tPos = pointer_;
			ch = _NextChar();
			if (ch == L'/') {		//Line comment
				while (ch != L'\r' && ch != L'\n' && HasNext())
					ch = _NextChar();
			}
			else if (ch == L'*') {	//Block comment
				while (true) {
					ch = _NextChar();
					if (ch == L'*') {
						ch = _NextChar();
						if (ch == L'/')
							break;
					}
				}
				ch = _NextChar();
			}
			else {
				pointer_ = tPos;
				ch = L'/';
			}
		}

		//スキップも空白飛ばしも無い場合、終了
		if (posStart == pointer_) break;
	}
}
void Scanner::_SkipSpace() {
	wchar_t ch = _CurrentChar();
	while (true) {
		if (ch != L' ' && ch != L'\t') break;
		ch = _NextChar();
	}
}

Token& Scanner::Next() {
	if (!HasNext()) {
		//_RaiseError(L"Scanner::Next: End-of-file already reached.");
		token_ = Token(Token::Type::TK_EOF, L"", -1, -1);
		return token_;
	}

	_SkipComment();

	wchar_t ch = _CurrentChar();

	Token::Type type = Token::Type::TK_UNKNOWN;
	int posStart = pointer_;	//Save the pointer at token begin

	switch (ch) {
	case L'\0': type = Token::Type::TK_EOF; break;
	case L',': _NextChar(); type = Token::Type::TK_COMMA;  break;
	case L'.': _NextChar(); type = Token::Type::TK_PERIOD;  break;
	case L'=': _NextChar(); type = Token::Type::TK_EQUAL;  break;
	case L'(': _NextChar(); type = Token::Type::TK_OPENP; break;
	case L')': _NextChar(); type = Token::Type::TK_CLOSEP; break;
	case L'[': _NextChar(); type = Token::Type::TK_OPENB; break;
	case L']': _NextChar(); type = Token::Type::TK_CLOSEB; break;
	case L'{': _NextChar(); type = Token::Type::TK_OPENC; break;
	case L'}': _NextChar(); type = Token::Type::TK_CLOSEC; break;
	case L'*': _NextChar(); type = Token::Type::TK_ASTERISK; break;
	case L'/': _NextChar(); type = Token::Type::TK_SLASH; break;
	case L':': _NextChar(); type = Token::Type::TK_COLON; break;
	case L';': _NextChar(); type = Token::Type::TK_SEMICOLON; break;
	case L'~': _NextChar(); type = Token::Type::TK_TILDE; break;
	case L'!': _NextChar(); type = Token::Type::TK_EXCLAMATION; break;
	case L'#': _NextChar(); type = Token::Type::TK_SHARP; break;
	case L'|': _NextChar(); type = Token::Type::TK_PIPE; break;
	case L'&': _NextChar(); type = Token::Type::TK_AMPERSAND; break;
	case L'<': _NextChar(); type = Token::Type::TK_LESS; break;
	case L'>': _NextChar(); type = Token::Type::TK_GREATER; break;
	case L'\"':
	case L'\'':
	{
		wchar_t enclosing = ch;

		ch = _NextChar();
		while (true) {
			if (ch == L'\\') ch = _NextChar();	//Skip escaped characters
			else if (ch == enclosing) break;
			ch = _NextChar();
		}
		if (ch == enclosing)
			_NextChar();	//Advance once after string end
		else {
			std::wstring error = GetString(posStart, pointer_);
			std::wstring log = StringUtility::Format(L"Scanner::Next: Unterminated string. -> %s", error.c_str());
			_RaiseError(log);
		}
		type = Token::Type::TK_STRING;
		break;
	}

	//Newlines
	case L'\r':
	case L'\n':
		//Skip other successive newlines
		while (ch == L'\r' || ch == L'\n') ch = _NextChar();
		type = Token::Type::TK_NEWLINE;
		break;

	case L'+':
	case L'-':
	{
		if (ch == L'+') {
			ch = _NextChar();
			type = Token::Type::TK_PLUS;
		}
		else if (ch == L'-') {
			ch = _NextChar();
			type = Token::Type::TK_MINUS;
		}
		if (!bPermitSignNumber_ || !iswdigit(ch)) break;
	}
	default:
	{
		if (iswdigit(ch)) {
			while (iswdigit(ch)) ch = _NextChar();
			type = Token::Type::TK_INT;

			if (ch == L'.') {
				//Int -> Real
				ch = _NextChar();
				while (iswdigit(ch)) ch = _NextChar();
				type = Token::Type::TK_REAL;
			}

			if (ch == L'E' || ch == L'e') {
				//Exponent format
				ch = _NextChar();
				while (iswdigit(ch) || ch == L'-') ch = _NextChar();
				type = Token::Type::TK_REAL;
			}
		}
		else if (iswalpha(ch) || ch == L'_') {
			while (iswalpha(ch) || iswdigit(ch) || ch == L'_')
				ch = _NextChar();
			type = Token::Type::TK_ID;
		}
		else {
			//ch = _NextChar();
			type = Token::Type::TK_UNKNOWN;
			if (Encoding::GetCharSize(typeEncoding_) == 1) {
				size_t mb_count = 0;

				//It's a multi-byte sequence
				while ((ch & 0xc0u) == 0xc0u) {
					//It's a continuation byte (fuck, is that even a word?)
					do {
						ch = _NextChar();
					} while ((ch & 0x80u) == 0x80u);
					++mb_count;
				}

				if (mb_count > 0) type = Token::Type::TK_ID;
				else ch = _NextChar();
			}
			else ch = _NextChar();
		}

		break;
	}
	}

	{
		std::wstring wstr = Encoding::BytesToWString(
			&buffer_[posStart], &buffer_[pointer_], typeEncoding_);
		if (type == Token::Type::TK_STRING)
			wstr = StringUtility::ParseStringWithEscape(wstr);
		token_ = Token(type, wstr, posStart, pointer_);
	}

	listDebugToken_.push_back(token_);
	return token_;
}
bool Scanner::HasNext() {
	return pointer_ < buffer_.size() && _CurrentChar() != L'\0' 
		&& token_.GetType() != Token::Type::TK_EOF;
}
void Scanner::CheckType(Token& tok, Token::Type type) {
	if (tok.type_ != type) {
		std::wstring str = StringUtility::Format(L"CheckType error[%s]:", tok.element_.c_str());
		_RaiseError(str);
	}
}
void Scanner::CheckIdentifer(Token& tok, const std::wstring& id) {
	if (tok.type_ != Token::Type::TK_ID || tok.GetIdentifier() != id) {
		std::wstring str = StringUtility::Format(L"CheckID error[%s]:", tok.element_.c_str());
		_RaiseError(str);
	}
}
int Scanner::GetCurrentLine() {
	if (buffer_.size() == 0) return 0;

	int line = 1;

	char* pStr = &buffer_[0];
	char* pEnd = &buffer_[pointer_];
	while (pStr <= pEnd) {
		if (Encoding::BytesToWChar(pStr, typeEncoding_) == L'\n')
			++line;
		pStr += Encoding::GetCharSize(typeEncoding_);
	}

	return line;
}
std::wstring Scanner::GetString(int start, int end) {
	std::wstring res = Encoding::BytesToWString(
		&buffer_[start], &buffer_[end], typeEncoding_);
	res = StringUtility::ParseStringWithEscape(res);
	return res;
}
bool Scanner::CompareMemory(int start, int end, const char* data) {
	if (end >= buffer_.size()) return false;

	int bufSize = end - start;
	bool res = memcmp(&buffer_[start], data, bufSize) == 0;
	return res;
}
std::vector<std::wstring> Scanner::GetArgumentList(bool bRequireEqual) {
	std::vector<std::wstring> res;

	if (bRequireEqual)
		CheckType(Next(), Token::Type::TK_EQUAL);

	Token& tok = Next();
	std::wstring tmp = L"";

	auto _AddStr = [&]() {
		tmp = StringUtility::Trim(tmp);
		if (tmp.size() > 0)
			res.push_back(tmp);
		tmp = L"";
	};

	if (tok.GetType() == Token::Type::TK_OPENP) {
		while (true) {
			tok = Next();

			Token::Type type = tok.GetType();
			if (type == Token::Type::TK_CLOSEP || type == Token::Type::TK_COMMA) {
				_AddStr();
				if (type == Token::Type::TK_CLOSEP)
					break;
			}
			else tmp += tok.GetElement();
		}
	}
	else {
		tmp = tok.GetElement();
		_AddStr();
	}

	return res;
}

//Token
const char* Token::Type_Str[] = {
	"TK_UNKNOWN", "TK_EOF", "TK_NEWLINE",
	"TK_ID",
	"TK_INT", "TK_REAL", "TK_STRING",

	"TK_OPENP", "TK_CLOSEP", "TK_OPENB", "TK_CLOSEB", "TK_OPENC", "TK_CLOSEC",
	"TK_SHARP",
	"TK_PIPE", "TK_AMPERSAND",

	"TK_COMMA", "TK_PERIOD", "TK_EQUAL",
	"TK_ASTERISK", "TK_SLASH", "TK_COLON", "TK_SEMICOLON", "TK_TILDE", "TK_EXCLAMATION",
	"TK_PLUS", "TK_MINUS",
	"TK_LESS", "TK_GREATER",
};
std::wstring& Token::GetIdentifier() {
	if (type_ != Type::TK_ID) {
		throw gstd::wexception(L"Token::GetIdentifier: Incorrect token type");
	}
	return element_;
}
std::wstring Token::GetString() {
	if (type_ != Type::TK_STRING) {
		throw gstd::wexception(L"Token::GetString: Incorrect token type");
	}
	return element_.substr(1, element_.size() - 2);
}
int Token::GetInteger() {
	if (type_ != Type::TK_INT) {
		throw gstd::wexception(L"Token::GetInterger: Incorrect token type");
	}
	return StringUtility::ToInteger(element_);
}
double Token::GetReal() {
	if (type_ != Type::TK_REAL && type_ != Type::TK_INT) {
		throw gstd::wexception(L"Token::GetReal: Incorrect token type");
	}
	return StringUtility::ToDouble(element_);
}
bool Token::GetBoolean() {
	bool res = false;
	if (type_ == Type::TK_REAL || type_ == Type::TK_INT) {
		res = GetReal() != 0;
	}
	else {
		res = element_ == L"true";
	}
	return res;
}

std::string Token::GetElementA() {
	std::wstring wstr = GetElement();
	std::string res = StringUtility::ConvertWideToMulti(wstr);
	return res;
}
std::string Token::GetStringA() {
	std::wstring wstr = GetString();
	std::string res = StringUtility::ConvertWideToMulti(wstr);
	return res;
}
std::string Token::GetIdentifierA() {
	std::wstring wstr = GetIdentifier();
	std::string res = StringUtility::ConvertWideToMulti(wstr);
	return res;
}
#endif

#if defined(DNH_PROJ_EXECUTOR)
//*******************************************************************
//TextParser
//*******************************************************************
TextParser::TextParser() {
}
TextParser::TextParser(const std::string& source) {
	SetSource(source);
}
TextParser::~TextParser() {}
TextParser::Result TextParser::_ParseComparison(int pos) {
	Result res = _ParseSum(pos);
	while (scan_->HasNext()) {
		scan_->SetCurrentPointer(res.pos_);

		Token& tok = scan_->Next();
		Token::Type type = tok.GetType();
		if (type == Token::Type::TK_EXCLAMATION || type == Token::Type::TK_EQUAL) {
			//「==」「!=」
			bool bNot = type == Token::Type::TK_EXCLAMATION;
			tok = scan_->Next();
			type = tok.GetType();
			if (type != Token::Type::TK_EQUAL) break;

			Result tRes = _ParseSum(scan_->GetCurrentPointer());
			res.pos_ = tRes.pos_;
			if (res.type_ == Type::TYPE_BOOLEAN && tRes.type_ == Type::TYPE_BOOLEAN) {
				res.valueBoolean_ = bNot ?
					res.valueBoolean_ != tRes.valueBoolean_ : res.valueBoolean_ == tRes.valueBoolean_;
			}
			else if (res.type_ == Type::TYPE_REAL && tRes.type_ == Type::TYPE_REAL) {
				res.valueBoolean_ = bNot ?
					res.valueReal_ != tRes.valueReal_ : res.valueReal_ == tRes.valueReal_;
			}
			else {
				_RaiseError(L"比較できない型");
			}
			res.type_ = Type::TYPE_BOOLEAN;
		}
		else if (type == Token::Type::TK_PIPE) {
			tok = scan_->Next();
			type = tok.GetType();
			if (type != Token::Type::TK_PIPE) break;
			Result tRes = _ParseSum(scan_->GetCurrentPointer());
			res.pos_ = tRes.pos_;
			if (res.type_ == Type::TYPE_BOOLEAN && tRes.type_ == Type::TYPE_BOOLEAN) {
				res.valueBoolean_ = res.valueBoolean_ || tRes.valueBoolean_;
			}
			else {
				_RaiseError(L"真偽値以外での||");
			}
		}
		else if (type == Token::Type::TK_AMPERSAND) {
			tok = scan_->Next();
			type = tok.GetType();
			if (type != Token::Type::TK_AMPERSAND) break;
			Result tRes = _ParseSum(scan_->GetCurrentPointer());
			res.pos_ = tRes.pos_;
			if (res.type_ == Type::TYPE_BOOLEAN && tRes.type_ == Type::TYPE_BOOLEAN) {
				res.valueBoolean_ = res.valueBoolean_ && tRes.valueBoolean_;
			}
			else {
				_RaiseError(L"真偽値以外での&&");
			}
		}
		else break;
	}
	return res;
}

TextParser::Result TextParser::_ParseSum(int pos) {
	Result res = _ParseProduct(pos);
	while (scan_->HasNext()) {
		scan_->SetCurrentPointer(res.pos_);

		Token& tok = scan_->Next();
		Token::Type type = tok.GetType();
		if (type != Token::Type::TK_PLUS && type != Token::Type::TK_MINUS)
			break;

		Result tRes = _ParseProduct(scan_->GetCurrentPointer());
		if (res.IsString() || tRes.IsString()) {
			res.type_ = Type::TYPE_STRING;
			res.valueString_ = res.GetString() + tRes.GetString();
		}
		else {
			if (tRes.type_ == Type::TYPE_BOOLEAN) _RaiseError(L"真偽値の加算減算");
			res.pos_ = tRes.pos_;
			if (type == Token::Type::TK_PLUS)
				res.valueReal_ += tRes.valueReal_;
			else if (type == Token::Type::TK_MINUS)
				res.valueReal_ -= tRes.valueReal_;
		}

	}

	return res;
}
TextParser::Result TextParser::_ParseProduct(int pos) {
	Result res = _ParseTerm(pos);
	while (scan_->HasNext()) {
		scan_->SetCurrentPointer(res.pos_);
		Token& tok = scan_->Next();
		Token::Type type = tok.GetType();
		if (type != Token::Type::TK_ASTERISK && type != Token::Type::TK_SLASH) break;

		Result tRes = _ParseTerm(scan_->GetCurrentPointer());
		if (tRes.type_ == Type::TYPE_BOOLEAN) _RaiseError(L"真偽値の乗算除算");

		res.type_ = tRes.type_;
		res.pos_ = tRes.pos_;
		if (type == Token::Type::TK_ASTERISK)
			res.valueReal_ *= tRes.valueReal_;
		else if (type == Token::Type::TK_SLASH)
			res.valueReal_ /= tRes.valueReal_;
	}

	return res;
}

TextParser::Result TextParser::_ParseTerm(int pos) {
	scan_->SetCurrentPointer(pos);
	Result res;
	Token& tok = scan_->Next();

	bool bMinus = false;
	bool bNot = false;
	Token::Type type = tok.GetType();
	if (type == Token::Type::TK_PLUS ||
		type == Token::Type::TK_MINUS ||
		type == Token::Type::TK_EXCLAMATION) {
		if (type == Token::Type::TK_MINUS) bMinus = true;
		if (type == Token::Type::TK_EXCLAMATION) bNot = true;
		tok = scan_->Next();
	}

	if (tok.GetType() == Token::Type::TK_OPENP) {
		res = _ParseComparison(scan_->GetCurrentPointer());
		scan_->SetCurrentPointer(res.pos_);
		tok = scan_->Next();
		if (tok.GetType() != Token::Type::TK_CLOSEP) _RaiseError(L")がありません");
	}
	else {
		Token::Type type = tok.GetType();
		if (type == Token::Type::TK_INT || type == Token::Type::TK_REAL) {
			res.valueReal_ = tok.GetReal();
			res.type_ = Type::TYPE_REAL;
		}
		else if (type == Token::Type::TK_ID) {
			Result tRes = _ParseIdentifer(scan_->GetCurrentPointer());
			res = tRes;
		}
		else if (type == Token::Type::TK_STRING) {
			res.valueString_ = tok.GetString();
			res.type_ = Type::TYPE_STRING;
		}
		else _RaiseError(StringUtility::Format(L"不正なトークン:%s", tok.GetElement().c_str()));
	}

	if (bMinus) {
		if (res.type_ != Type::TYPE_REAL) _RaiseError(L"実数以外での負記号");
		res.valueReal_ = -res.valueReal_;
	}
	if (bNot) {
		if (res.type_ != Type::TYPE_BOOLEAN) _RaiseError(L"真偽値以外での否定");
		res.valueBoolean_ = !res.valueBoolean_;
	}

	res.pos_ = scan_->GetCurrentPointer();

	return res;
}
TextParser::Result TextParser::_ParseIdentifer(int pos) {
	Result res;
	res.pos_ = scan_->GetCurrentPointer();

	Token& tok = scan_->GetToken();
	std::wstring id = tok.GetElement();
	if (id == L"true") {
		res.type_ = Type::TYPE_BOOLEAN;
		res.valueBoolean_ = true;
	}
	else if (id == L"false") {
		res.type_ = Type::TYPE_BOOLEAN;
		res.valueBoolean_ = false;
	}
	else {
		_RaiseError(StringUtility::Format(L"不正な識別子:%s", id.c_str()));
	}

	return res;
}

void TextParser::SetSource(const std::string& source) {
	std::vector<char> buf;
	buf.resize(source.size() + 1);
	memcpy(&buf[0], source.c_str(), source.size() + 1);
	scan_ = new Scanner(buf);
	scan_->SetPermitSignNumber(false);
}
TextParser::Result TextParser::GetResult() {
	if (scan_ == nullptr) _RaiseError(L"テキストが設定されていません");
	scan_->SetPointerBegin();
	Result res = _ParseComparison(scan_->GetCurrentPointer());
	if (scan_->HasNext()) _RaiseError(StringUtility::Format(L"不正なトークン:%s", scan_->GetToken().GetElement().c_str()));
	return res;
}
double TextParser::GetReal() {
	if (scan_ == nullptr) _RaiseError(L"テキストが設定されていません");
	scan_->SetPointerBegin();
	Result res = _ParseSum(scan_->GetCurrentPointer());
	if (scan_->HasNext()) _RaiseError(StringUtility::Format(L"不正なトークン:%s", scan_->GetToken().GetElement().c_str()));
	return res.GetReal();
}

//*******************************************************************
//Font
//*******************************************************************
//const wchar_t* Font::GOTHIC  = L"標準ゴシック";
//const wchar_t* Font::MINCHOH = L"標準明朝";
const wchar_t* Font::GOTHIC = L"MS Gothic";
const wchar_t* Font::MINCHOH = L"MS Mincho";

Font::Font() {
	hFont_ = nullptr;
	ZeroMemory(&info_, sizeof(info_));
	ZeroMemory(&metrics_, sizeof(metrics_));
}
Font::~Font() {
	this->Clear();
}
void Font::Clear() {
	if (hFont_) {
		::DeleteObject(hFont_);
		hFont_ = nullptr;
		ZeroMemory(&info_, sizeof(LOGFONT));
	}
}
void Font::CreateFont(const wchar_t* type, int size, bool bBold, bool bItalic, bool bLine) {
	ZeroMemory(&info_, sizeof(info_));

	info_.lfHeight = size;
	info_.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
	info_.lfItalic = bItalic;
	info_.lfUnderline = bLine;
	//info_.lfStrikeOut = 1;
	info_.lfCharSet = DetectCharset(type);
	lstrcpy(info_.lfFaceName, type);

	this->CreateFontIndirect(info_);
}
void Font::CreateFontIndirect(LOGFONT& fontInfo) {
	if (hFont_) this->Clear();
	hFont_ = ::CreateFontIndirect(&fontInfo);
	info_ = fontInfo;

	HDC hDC = ::GetDC(nullptr);
	HFONT oldFont = (HFONT)SelectObject(hDC, hFont_);

	::GetTextMetrics(hDC, &metrics_);

	::SelectObject(hDC, oldFont);
	::ReleaseDC(nullptr, hDC);
}
#pragma warning (disable : 4129)	//Unrecognized escape character
BYTE Font::DetectCharset(const wchar_t* type) {
	std::wcmatch match;
	bool reg = std::regex_search(type, match, std::wregex(L"[^a-zA-Z0-9_ \-]"));
	if (reg) {
		/*
		wchar_t ch = match[0].str()[0];
		auto InRange = [&](wchar_t low, wchar_t up) -> bool {
			return (ch >= low && ch <= up);
		};

		//Check Japanese first
		if (InRange(0x3040, 0x309F)			//Hiragana
			|| InRange(0x30A0, 0x30FF)		//Katakana
			|| InRange(0x3190, 0x319F))		//Kanbun
			return SHIFTJIS_CHARSET;
		else if (InRange(0x0400, 0x04FF)	//Cyrillic
			|| InRange(0x0500, 0x052F)		//Cyrillic supplement
			|| InRange(0x2DE0, 0x2DFF) || InRange(0xA640, 0xA69F) || InRange(0x1C80, 0x1C88))	//Cyrillic extended
			return RUSSIAN_CHARSET;
		else if (InRange(0x0370, 0x03FF)	//Greek
			|| InRange(0x1F00, 0x1FFE))		//Greek extended
			return GREEK_CHARSET;
		else if (InRange(0x0600, 0x06FF)	//Arabic
			|| InRange(0x0750, 0x077F)		//Arabic supplement
			|| InRange(0x08A0, 0x08FF)		//Arabic extended
			|| InRange(0xFB50, 0xFDFD) || InRange(0xFE70, 0xFEFC))		//Arabic presentation forms
			return ARABIC_CHARSET;
		else if (InRange(0x0591, 0x05F4)	//Hebrew
			|| InRange(0xFB1D, 0xFB4F))		//Hebrew presentation forms
			return HEBREW_CHARSET;
		else if (InRange(0x0E01, 0x0E5B))	//Thai
			return THAI_CHARSET;
		else if (InRange(0x1100, 0x11FF)	//Hangul Jamo
			|| InRange(0xA960, 0xA97F) || InRange(0xD7B0, 0xD7FF)		//Hangul Jamo extended
			|| InRange(0x3130, 0x318F)		//Hangul compatibility Jamo
			|| InRange(0xAC00, 0xD7AF))		//Hangul syllables
			return HANGUL_CHARSET;
		*/

		//Probably one of the eight million kanjis or chinese characters
		return SHIFTJIS_CHARSET;
	}
	else return DEFAULT_CHARSET;
}
#endif
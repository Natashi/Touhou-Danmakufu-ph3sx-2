#pragma once

#include "../pch.h"

#include "SmartPointer.hpp"
#include "VectorExtension.hpp"

namespace gstd {
	//================================================================
	//DebugUtility
	class DebugUtility {
	public:
		static void DumpMemoryLeaksOnExit() {
#ifdef _DEBUG
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
			_CrtDumpMemoryLeaks();
#endif
		}
	};

	//================================================================
	//SystemUtility
	class SystemUtility {
	public:
		static void TestCpuSupportSIMD();
	};

	//================================================================
	//ThreadUtility
	template<class F>
	static void ParallelFor(size_t countLoop, F&& func) {
		size_t countCore = std::max(std::thread::hardware_concurrency(), 1U);

		std::vector<std::future<void>> workers;
		workers.reserve(countCore);

		auto coreTask = [&](size_t id) {
			const size_t begin = countLoop / countCore * id + std::min(countLoop % countCore, id);
			const size_t end = countLoop / countCore * (id + 1U) + std::min(countLoop % countCore, id + 1U);

			for (size_t i = begin; i < end; ++i)
				func(i);
		};

		for (size_t iCore = 0; iCore < countCore; ++iCore)
			workers.emplace_back(std::async(std::launch::async | std::launch::deferred, coreTask, iCore));
		for (const auto& worker : workers)
			worker.wait();
	}

	//================================================================
	//VersionUtility
	class VersionUtility {
	public:
		static inline uint64_t ExtractMajor(uint64_t target) {
			return (target >> 48) & 0xffff;
		}
		static inline uint64_t ExtractMinor(uint64_t target) {
			return (target >> 32) & 0xffff;
		}
		static inline uint64_t ExtractMicro(uint64_t target) {
			return (target >> 16) & 0xffff;
		}
		static inline uint64_t ExtractRevision(uint64_t target) {
			return target & 0xffff;
		}
		//Should ensure compatibility if version's [reserved], [major], and [minor] match target's
		static inline const bool IsDataBackwardsCompatible(uint64_t target, uint64_t version) {
			return ((version ^ target) >> 16) == 0;
		}
	};

	//================================================================
	//Encoding
	class Encoding {
		//http://msdn.microsoft.com/ja-jp/library/system.text.encoding(v=vs.110).aspx
		//babel
		//http://d.hatena.ne.jp/A7M/20100801/1280629387
	public:
		typedef enum : int8_t {
			UNKNOWN = -1,
			UTF8 = 1,
			UTF8BOM,
			UTF16LE,
			UTF16BE,
		} Type;
	public:
		static const byte BOM_UTF16LE[];
		static const byte BOM_UTF16BE[];
		static const byte BOM_UTF8[];
	public:
		static Type Detect(const char* data, size_t dataSize);
		static size_t GetBomSize(const char* data, size_t dataSize);
		static size_t GetBomSize(Type encoding);
		static size_t GetCharSize(Type encoding);

		static wchar_t BytesToWChar(const char* data, Type encoding);
		static std::string BytesToString(const char* pBegin, const char* pEnd, Type encoding);
		static std::string BytesToString(const char* pBegin, size_t count, Type encoding);
		static std::wstring BytesToWString(const char* pBegin, const char* pEnd, Type encoding);
		static std::wstring BytesToWString(const char* pBegin, size_t count, Type encoding);
	};


	//================================================================
	//StringUtility
	class StringUtility {
	public:
		static std::wstring ConvertMultiToWide(const std::string& str, int code = CP_UTF8);
		static std::string ConvertWideToMulti(const std::wstring& wstr, int code = CP_UTF8);
		static std::wstring ConvertMultiToWide(std::vector<char>& buf, int code = CP_UTF8);
		static std::string ConvertWideToMulti(std::vector<char>& buf, int code = CP_UTF8);
		static size_t ConvertMultiToWide(char* mbstr, size_t mbcount, std::vector<char>& wres, int code);
		static size_t ConvertWideToMulti(wchar_t* mbstr, size_t wcount, std::vector<char>& mbres, int code);

		//----------------------------------------------------------------
		static std::vector<std::string> Split(const std::string& str, const std::string& delim);
		static void Split(const std::string& str, const std::string& delim, std::vector<std::string>& res);
		static std::string Format(const char* str, ...);
		static std::string Format(const char* str, va_list va);

		static size_t CountCharacter(const std::string& str, char c);
		static size_t CountCharacter(std::vector<char>& str, char c);
		static int ToInteger(const std::string& s);
		static double ToDouble(const std::string& s);
		static std::string Replace(const std::string& source, const std::string& pattern, const std::string& placement);
		static std::string ReplaceAll(const std::string& source, const std::string& pattern, const std::string& placement,
			size_t replaceCount = UINT_MAX, size_t start = 0, size_t end = 0);
		static std::string ReplaceAll(const std::string& source, char pattern, char placement);
		static std::string Slice(const std::string& s, size_t length);
		static std::string Trim(const std::string& str);

		//----------------------------------------------------------------
		static std::vector<std::wstring> Split(const std::wstring& str, const std::wstring& delim);
		static void Split(const std::wstring& str, const std::wstring& delim, std::vector<std::wstring>& res);
		static std::wstring Format(const wchar_t* str, ...);
		static std::wstring Format(const wchar_t* str, va_list va);
		static std::wstring FormatToWide(const char* str, ...);

		static size_t CountCharacter(const std::wstring& str, wchar_t c);
		static int ToInteger(const std::wstring& s);
		static double ToDouble(const std::wstring& s);
		static std::wstring Replace(const std::wstring& source, const std::wstring& pattern, const std::wstring& placement);
		static std::wstring ReplaceAll(const std::wstring& source, const std::wstring& pattern, const std::wstring& placement,
			size_t replaceCount = UINT_MAX, size_t start = 0, size_t end = 0);
		static std::wstring ReplaceAll(const std::wstring& source, wchar_t pattern, wchar_t placement);
		static std::wstring Slice(const std::wstring& s, size_t length);
		static std::wstring Trim(const std::wstring& str);

		static size_t CountAsciiSizeCharacter(const std::wstring& str);
		static size_t GetByteSize(const std::wstring& str) {
			return str.size() * sizeof(wchar_t);
		}
	};

	//================================================================
	//ErrorUtility
	class ErrorUtility {
	public:
		enum {
			ERROR_FILE_NOTFOUND,
			ERROR_PARSE,
			ERROR_END_OF_FILE,
			ERROR_OUTOFRANGE_INDEX,
		};
	public:
		static std::wstring GetLastErrorMessage(DWORD error);
		static std::wstring GetLastErrorMessage() {
			return GetLastErrorMessage(GetLastError());
		}
		static std::wstring GetErrorMessage(int type);
		static std::wstring GetFileNotFoundErrorMessage(const std::wstring& path, bool bErrorExtended = false);
		static std::wstring GetParseErrorMessage(int line, const std::wstring& what) {
			return GetParseErrorMessage(L"", line, what);
		}
		static std::wstring GetParseErrorMessage(const std::wstring& path, int line, const std::wstring& what);
	};

	//================================================================
	//wexception
	class wexception {
	protected:
		std::wstring message_;
	public:
		wexception() {}
		wexception(const std::wstring& msg) { message_ = msg; }
		wexception(const std::string& msg) {
			message_ = StringUtility::ConvertMultiToWide(msg);
		}
		std::wstring& GetMessage() { return message_; }
		const wchar_t* what() { return message_.c_str(); }
	};

#if defined(DNH_PROJ_EXECUTOR)
	//================================================================
	//Math
	constexpr double GM_PI = 3.14159265358979323846;
	constexpr double GM_PI_X2 = GM_PI * 2.0;
	constexpr double GM_PI_X4 = GM_PI * 4.0;
	constexpr double GM_PI_2 = GM_PI / 2.0;
	constexpr double GM_PI_4 = GM_PI / 4.0;
	constexpr double GM_1_PI = 1.0 / GM_PI;
	constexpr double GM_2_PI = 2.0 / GM_PI;
	constexpr double GM_SQRTP = 1.772453850905516027298;
	constexpr double GM_1_SQRTP = 1.0 / GM_SQRTP;
	constexpr double GM_2_SQRTP = 2.0 / GM_SQRTP;
	constexpr double GM_SQRT2 = 1.41421356237309504880;
	constexpr double GM_SQRT2_2 = GM_SQRT2 / 2.0;
	constexpr double GM_SQRT2_X2 = GM_SQRT2 * 2.0;
	constexpr double GM_E = 2.71828182845904523536;
	constexpr double GM_LOG2E = 1.44269504088896340736;		//log2(e)
	constexpr double GM_LOG10E = 0.434294481903251827651;	//log10(e)
	constexpr double GM_LN2 = 0.693147180559945309417;		//ln(2)
	constexpr double GM_LN10 = 2.30258509299404568402;		//ln(10)
	constexpr double GM_PHI = 1.618033988749894848205;		//Golden ratio
	constexpr double GM_1_PHI = 1.0 / GM_PHI;				//The other golden ratio
	class Math {
	public:
		static inline constexpr double DegreeToRadian(double angle) { return angle * GM_PI / 180.0; }
		static inline constexpr double RadianToDegree(double angle) { return angle * 180.0 / GM_PI; }

		static inline double NormalizeAngleDeg(double angle) { 
			angle = fmod(angle, 360.0);
			return angle < 0.0 ? angle + 360.0 : angle;
		}
		static inline double NormalizeAngleRad(double angle) {
			angle = fmod(angle, GM_PI_X2);
			return angle < 0.0 ? angle + GM_PI_X2 : angle;
		}
		static inline double AngleDifferenceRad(double angle1, double angle2) {
			double dist = NormalizeAngleRad(angle2 - angle1);
			return dist > GM_PI ? dist - GM_PI_X2 : dist;
		}

		template<typename T>
		static inline T HypotSq(const T& x, const T& y) {
			return (x * x + y * y);
		}

		static void InitializeFPU() {
			__asm {
				finit
			};
		}

		static inline const double Round(double val) { return std::round(val); }
		static inline const int Trunc(double val) { return (int)(val + 0.01); }

		class Lerp {
		public:
			typedef enum : uint8_t {
				LINEAR,
				SMOOTH,
				SMOOTHER,
				ACCELERATE,
				DECELERATE,
			} Type;
		public:
			template<typename T, typename L>
			static inline T Linear(T a, T b, L x) {
				return a + (b - a) * x;
			}
			template<typename T, typename L>
			static inline T Smooth(T a, T b, L x) {
				return a + x * x * ((L)3 - (L)2 * x) * (b - a);
			}
			template<typename T, typename L>
			static inline T Smoother(T a, T b, L x) {
				return a + x * x * x * (x * (x * (L)6 - (L)15) + (L)10) * (b - a);
			}
			template<typename T, typename L>
			static inline T Accelerate(T a, T b, L x) {
				return a + x * x * (b - a);
			}
			template<typename T, typename L>
			static inline T Decelerate(T a, T b, L x) {
				return a + ((L)1 - ((L)1 - x) * ((L)1 - x)) * (b - a);
			}

			//For finding lerp speeds
			template<typename T>
			static inline T DifferentialLinear(T x) {
				return (T)1;
			}
			template<typename T>
			static inline T DifferentialSmooth(T x) {
				return (T)6 * x * ((T)1 - x);
			}
			template<typename T>
			static inline T DifferentialSmoother(T x) {
				return (T)30 * x * x * (x * x - (T)2 * x + (T)1);
			}
			template<typename T>
			static inline T DifferentialAccelerate(T x) {
				return (T)2 * x;
			}
			template<typename T>
			static inline T DifferentialDecelerate(T x) {
				return (T)2 * ((T)1 - x);
			}
		};
	};
#endif

	//================================================================
	//ByteOrder
	class ByteOrder {
	public:
		enum {
			ENDIAN_LITTLE,
			ENDIAN_BIG,
		};
	public:
		static void Reverse(LPVOID buf, DWORD size);
	};

	//================================================================
	//PathProperty
	class PathProperty {
	public:
		static std::wstring GetFileDirectory(const std::wstring& path) {
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

		static std::wstring GetDirectoryName(const std::wstring& path) {
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

		static std::wstring GetFileName(const std::wstring& path) {
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

		static std::wstring GetDriveName(const std::wstring& path) {
#ifdef __L_STD_FILESYSTEM
			path_t p(path);
			return (p.root_name() / p.root_directory());
#else
			wchar_t pDrive[_MAX_PATH];
			_wsplitpath_s(path.c_str(), pDrive, _MAX_PATH, nullptr, 0, nullptr, 0, nullptr, 0);
			return std::wstring(pDrive);
#endif
		}

		static std::wstring GetFileNameWithoutExtension(const std::wstring& path) {
#ifdef __L_STD_FILESYSTEM
			path_t p(path);
			return p.stem();
#else
			wchar_t pFileName[_MAX_PATH];
			_wsplitpath_s(path.c_str(), nullptr, 0, nullptr, 0, pFileName, _MAX_PATH, nullptr, 0);
			return std::wstring(pFileName);
#endif
		}

		static std::wstring GetFileExtension(const std::wstring& path) {
#ifdef __L_STD_FILESYSTEM
			path_t p(path);
			return p.extension();
#else
			wchar_t pExt[_MAX_PATH];
			_wsplitpath_s(path.c_str(), nullptr, 0, nullptr, 0, nullptr, 0, pExt, _MAX_PATH);
			return std::wstring(pExt);
#endif
		}
		//Returns the path of the executable.
		static const std::wstring& GetModuleName() {
			static std::wstring moduleName;
			if (moduleName.size() == 0) {
				wchar_t modulePath[_MAX_PATH];
				ZeroMemory(modulePath, sizeof(modulePath));
				GetModuleFileName(NULL, modulePath, _MAX_PATH - 1);
				moduleName = GetFileNameWithoutExtension(std::wstring(modulePath));
			}
			return moduleName;
		}
		//Returns the directory of the executable.
		static const std::wstring& GetModuleDirectory() {
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
		//Returns the directory of the path relative to the executable's directory.
		static std::wstring GetDirectoryWithoutModuleDirectory(const std::wstring& path) {
			std::wstring res = GetFileDirectory(path);
			const std::wstring& dirModule = GetModuleDirectory();
			if (res.find(dirModule) != std::wstring::npos) {
				res = res.substr(dirModule.size());
			}
			return res;
		}
		//Returns the the path relative to the executable's directory.
		static std::wstring GetPathWithoutModuleDirectory(const std::wstring& path) {
			const std::wstring& dirModule = GetModuleDirectory();
			std::wstring res = Canonicalize(path);
			if (res.find(dirModule) != std::wstring::npos) {
				res = res.substr(dirModule.size());
			}
			return res;
		}
		static std::wstring GetRelativeDirectory(const std::wstring& from, const std::wstring& to) {
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
		static std::wstring ExtendRelativeToFull(const std::wstring& dir, std::wstring path) {
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
		//Replaces all the '\\' characters with '/'.
		static std::wstring ReplaceYenToSlash(const std::wstring& path) {
			return StringUtility::ReplaceAll(path, L'\\', L'/');
		}
		static std::wstring Canonicalize(const std::wstring& srcPath) {
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
		//Replaces all the '/' characters with '\\' (at the very least).
		static std::wstring MakePreferred(const std::wstring& srcPath) {
#ifdef __L_STD_FILESYSTEM
			path_t p(srcPath);
			std::wstring res = p.make_preferred();
#else
			std::wstring res = StringUtility::ReplaceAll(path, L"/", L"\\");
#endif
			return res;
		}
		static std::wstring GetUnique(const std::wstring& srcPath) {
			std::wstring p = Canonicalize(srcPath);
			return ReplaceYenToSlash(p);
		}
	};

#if defined(DNH_PROJ_EXECUTOR)
	//================================================================
	//BitAccess
	class BitAccess {
	public:
		template <typename T> static bool GetBit(T value, size_t bit) {
			T mask = (T)1 << bit;
			return (value & mask) != 0;
		}
		template <typename T> static T& SetBit(T& value, size_t bit, bool b) {
			T mask = (T)1 << bit;
			T write = (T)b << bit;
			value &= ~mask;
			value |= write;
			return value;
		}
		template <typename T> static byte GetByte(T value, size_t bit) {
			return (byte)(value >> bit);
		}
		template <typename T> static T& SetByte(T& value, size_t bit, byte c) {
			T mask = (T)0xff << bit;
			value = (value & (~mask)) | ((T)c << bit);
			return value;
		}
	};

	//================================================================
	//IStringInfo
	class IStringInfo {
	public:
		virtual ~IStringInfo() {}
		virtual std::wstring GetInfoAsString() {
			int address = (int)this;
			char* name = (char*)typeid(*this).name();
			std::string str = StringUtility::Format("%s[%08x]", name, address);
			std::wstring res = StringUtility::ConvertMultiToWide(str);
			return res;
		}
	};

	//================================================================
	//InnerClass
	//C++には内部クラスがないので、外部クラスアクセス用
	template <class T>
	class InnerClass {
		T* outer_;
	protected:
		T* _GetOuter() { return outer_; }
		void _SetOuter(T* outer) { outer_ = outer; }
	public:
		InnerClass(T* outer = nullptr) { outer_ = outer; }
	};
#endif

	//================================================================
	//Singleton
	template<class T> class Singleton {
	protected:
		Singleton() {};
		inline static T*& _This() {
			static T* s = nullptr;
			return s;
		}
	public:
		virtual ~Singleton() {};
		static T* CreateInstance() {
			T*& p = _This();
			if (p == nullptr) p = new T();
			return p;
		}
		static T* GetInstance() {
			T*& p = _This();
			if (p == nullptr) throw gstd::wexception("Instance uninitialized.");
			return p;
		}
		static void DeleteInstance() {
			T*& p = _This();
			ptr_delete(p);
		}
	};

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)
	//================================================================
	//Scanner
	class Scanner;
	class Token {
		friend Scanner;
	public:
		enum class Type : uint8_t {
			TK_UNKNOWN, TK_EOF, TK_NEWLINE,
			TK_ID,
			TK_INT, TK_REAL, TK_STRING,

			TK_OPENP, TK_CLOSEP, TK_OPENB, TK_CLOSEB, TK_OPENC, TK_CLOSEC,
			TK_SHARP,
			TK_PIPE, TK_AMPERSAND,

			TK_COMMA, TK_PERIOD, TK_EQUAL,
			TK_ASTERISK, TK_SLASH, TK_COLON, TK_SEMICOLON, TK_TILDE, TK_EXCLAMATION,
			TK_PLUS, TK_MINUS,
			TK_LESS, TK_GREATER,
		};
		static const char* Type_Str[];
	protected:
		Type type_;
		std::wstring element_;
		int posStart_;
		int posEnd_;
	public:
		Token() { type_ = Type::TK_UNKNOWN; posStart_ = 0; posEnd_ = 0; }
		Token(Type type, std::wstring& element, int start, int end) { type_ = type; element_ = element; posStart_ = start; posEnd_ = end; }
		virtual ~Token() {};

		Type GetType() { return type_; }
		std::wstring& GetElement() { return element_; }
		std::string GetElementA();

		int GetStartPointer() { return posStart_; }
		int GetEndPointer() { return posEnd_; }

		int GetInteger();
		double GetReal();
		bool GetBoolean();
		std::wstring GetString();
		std::wstring& GetIdentifier();

		std::string GetStringA();
		std::string GetIdentifierA();
	};

	class Scanner {
	protected:
		Encoding::Type typeEncoding_;
		int textStartPointer_;
		std::vector<char> buffer_;
		int pointer_;//今の位置
		Token token_;//現在のトークン
		bool bPermitSignNumber_;
		std::list<Token> listDebugToken_;

		const char* bufStr_;

		wchar_t _CurrentChar();
		wchar_t _NextChar();//ポインタを進めて次の文字を調べる

		virtual void _SkipComment();//コメントをとばす
		virtual void _SkipSpace();//空白をとばす
		virtual void _RaiseError(const std::wstring& str) {	//例外を投げます
			throw gstd::wexception(str);
		}
	public:
		Scanner(char* str, size_t size);
		Scanner(const std::string& str);
		Scanner(const std::wstring& wstr);
		Scanner(std::vector<char>& buf);
		virtual ~Scanner();

		void SetPermitSignNumber(bool bEnable) { bPermitSignNumber_ = bEnable; }
		Encoding::Type GetEncoding() { return typeEncoding_; }

		Token& GetToken() {	//現在のトークンを取得
			return token_;
		}
		Token& Next();
		bool HasNext();
		void CheckType(Token& tok, Token::Type type);
		void CheckIdentifer(Token& tok, const std::wstring& id);
		int GetCurrentLine();

		int GetCurrentPointer() { return pointer_; }
		void SetCurrentPointer(int pos) { pointer_ = pos; }
		void SetPointerBegin() { pointer_ = textStartPointer_; }
		std::wstring GetString(int start, int end);

		bool CompareMemory(int start, int end, const char* data);
	};
#endif

#if defined(DNH_PROJ_EXECUTOR)
	//================================================================
	//TextParser
	class TextParser {
	public:
		enum class Type : uint8_t {
			TYPE_REAL,
			TYPE_BOOLEAN,
			TYPE_STRING,
		};

		class Result {
			friend TextParser;
		protected:
			Type type_;
			int pos_;
			std::wstring valueString_;
			union {
				double valueReal_;
				bool valueBoolean_;
			};
		public:
			virtual ~Result() {};
			Type GetType() { return type_; }
			double GetReal() {
				double res = valueReal_;
				if (IsBoolean()) res = valueBoolean_ ? 1.0 : 0.0;
				if (IsString()) res = StringUtility::ToDouble(valueString_);
				return res;
			}
			void SetReal(double value) {
				type_ = Type::TYPE_REAL;
				valueReal_ = value;
			}
			bool GetBoolean() {
				bool res = valueBoolean_;
				if (IsReal()) res = (valueReal_ != 0.0 ? true : false);
				if (IsString()) res = (valueString_ == L"true" ? true : false);
				return res;
			}
			void SetBoolean(bool value) {
				type_ = Type::TYPE_BOOLEAN;
				valueBoolean_ = value;
			}
			std::wstring GetString() {
				std::wstring res = valueString_;
				if (IsReal()) res = gstd::StringUtility::Format(L"%f", valueReal_);
				if (IsBoolean()) res = (valueBoolean_ ? L"true" : L"false");
				return res;
			}
			void SetString(const std::wstring& value) {
				type_ = Type::TYPE_STRING;
				valueString_ = value;
			}
			bool IsReal() { return type_ == Type::TYPE_REAL; }
			bool IsBoolean() { return type_ == Type::TYPE_BOOLEAN; }
			bool IsString() { return type_ == Type::TYPE_STRING; }
		};

	protected:
		gstd::ref_count_ptr<Scanner> scan_;

		void _RaiseError(const std::wstring& message) {
			throw gstd::wexception(message);
		}
		Result _ParseComparison(int pos);
		Result _ParseSum(int pos);
		Result _ParseProduct(int pos);
		Result _ParseTerm(int pos);
		virtual Result _ParseIdentifer(int pos);
	public:
		TextParser();
		TextParser(const std::string& source);
		virtual ~TextParser();

		void SetSource(const std::string& source);
		Result GetResult();
		double GetReal();
	};

	//================================================================
	//Font
	class Font {
	public:
		const static wchar_t* GOTHIC;
		const static wchar_t* MINCHOH;
	protected:
		HFONT hFont_;
		LOGFONT info_;
		TEXTMETRICW metrics_;
	public:
		Font();
		virtual ~Font();

		void CreateFont(const wchar_t* type, int size, bool bBold = false, bool bItalic = false, bool bLine = false);
		void CreateFontIndirect(LOGFONT& fontInfo);
		void Clear();

		HFONT GetHandle() { return hFont_; }
		const LOGFONT& GetInfo() { return info_; }
		const TEXTMETRICW& GetMetrics() { return metrics_; }

		static BYTE DetectCharset(const wchar_t* type);
	};
#endif
}
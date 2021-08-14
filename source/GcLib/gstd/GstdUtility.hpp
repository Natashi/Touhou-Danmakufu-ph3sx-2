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

		static size_t GetMultibyteSize(const char* data);
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

		static char ParseEscapeSequence(const char* str, char** pEnd);
		static wchar_t ParseEscapeSequence(const wchar_t* wstr, wchar_t** pEnd);
		static std::string ParseStringWithEscape(const std::string& str);
		static std::wstring ParseStringWithEscape(const std::wstring& wstr);

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
		template<class E, class T = std::char_traits<E>, class A = std::allocator<E>>
		static size_t GetByteSize(const std::basic_string<E, T, A>& str) {
			return str.size() * sizeof(E);
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
		static void InitializeFPU() {
			_asm { 
				finit 
			};
		}

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
		static inline double AngleDifferenceDeg(double angle1, double angle2) {
			double dist = NormalizeAngleDeg(angle2 - angle1);
			return dist > 180.0 ? dist - 360.0 : dist;
		}
		static inline double AngleDifferenceRad(double angle1, double angle2) {
			double dist = NormalizeAngleRad(angle2 - angle1);
			return dist > GM_PI ? dist - GM_PI_X2 : dist;
		}

		template<typename T>
		static inline T HypotSq(const T& x, const T& y) {
			return (x * x + y * y);
		}

		static inline void DoSinCos(double angle, double* pRes) {
			_asm {
				mov esi, DWORD PTR[pRes]	//Load pRes into esi
				fld QWORD PTR[angle]		//Load angle into the FPU stack
				fsincos
				fstp QWORD PTR[esi + 8]		//Pop cos
				fstp QWORD PTR[esi]			//Pop sin
			};
		}
		static inline void Rotate2D(double* pos, double ang, double ox, double oy) {
			double sc[2];
			DoSinCos(ang, sc);
			pos[0] -= ox;
			pos[1] -= oy;
			double x = pos[0] * sc[1] - pos[1] * sc[0];
			double y = pos[0] * sc[0] + pos[1] * sc[1];
			pos[0] = ox + x;
			pos[1] = oy + y;
		}

		static inline size_t FloorBase(size_t val, size_t base) {
			return (val % base != 0) ? ((val / base) * base) : val;
		}
		static inline size_t CeilBase(size_t val, size_t base) {
			return (val % base != 0) ? ((val / base) * base + base) : val;
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

			template<typename T, typename L>
			using funcLerp = T(*)(T, T, L);

			template<typename T>
			using funcLerpDiff = T(*)(T);
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

			template<typename T, typename L>
			static funcLerp<T, L> GetFunc(Type type) {
				switch (type) {
				case Math::Lerp::SMOOTH:
					return Smooth<T, L>;
				case Math::Lerp::SMOOTHER:
					return Smoother<T, L>;
				case Math::Lerp::ACCELERATE:
					return Math::Lerp::Accelerate<T, L>;
				case Math::Lerp::DECELERATE:
					return Math::Lerp::Decelerate<T, L>;
				}
				return Math::Lerp::Linear<T, L>;
			}
			template<typename T>
			static funcLerpDiff<T> GetFuncDifferential(Type type) {
				switch (type) {
				case Math::Lerp::SMOOTH:
					return DifferentialSmooth<T>;
				case Math::Lerp::SMOOTHER:
					return DifferentialSmoother<T>;
				case Math::Lerp::ACCELERATE:
					return DifferentialAccelerate<T>;
				case Math::Lerp::DECELERATE:
					return DifferentialDecelerate<T>;
				}
				return DifferentialLinear<T>;
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
		static std::wstring GetFileDirectory(const std::wstring& path);
		static std::wstring GetDirectoryName(const std::wstring& path);
		static std::wstring GetFileName(const std::wstring& path);
		static std::wstring GetDriveName(const std::wstring& path);
		static std::wstring GetFileNameWithoutExtension(const std::wstring& path);
		static std::wstring GetFileExtension(const std::wstring& path);

		//Returns the path of the executable.
		static const std::wstring& GetModuleName();

		//Returns the directory of the executable.
		static const std::wstring& GetModuleDirectory();

		//Returns the directory of the path relative to the executable's directory.
		static std::wstring GetDirectoryWithoutModuleDirectory(const std::wstring& path);

		//Returns the the path relative to the executable's directory.
		static std::wstring GetPathWithoutModuleDirectory(const std::wstring& path);

		//Replaces the executable's directory with the specified string
		static std::wstring ReduceModuleDirectory(const std::wstring& path, std::wstring rep = L"../");

		static std::wstring GetRelativeDirectory(const std::wstring& from, const std::wstring& to);
		static std::wstring ExtendRelativeToFull(const std::wstring& dir, std::wstring path);

		//Replaces all the '\\' characters with '/'.
		static std::wstring ReplaceYenToSlash(const std::wstring& path);

		static std::wstring Canonicalize(const std::wstring& srcPath);

		//Replaces all the '/' characters with '\\' (at the very least).
		static std::wstring MakePreferred(const std::wstring& srcPath);

		static std::wstring GetUnique(const std::wstring& srcPath);
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

		std::vector<std::wstring> GetArgumentList(bool bRequireEqual = true);
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

		HFONT GetHandle() const { return hFont_; }
		const LOGFONT& GetInfo() const { return info_; }
		const TEXTMETRICW& GetMetrics() const { return metrics_; }

		static BYTE DetectCharset(const wchar_t* type);
	};
#endif
}
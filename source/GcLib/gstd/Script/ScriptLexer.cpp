#include "source/GcLib/pch.h"

#include "ScriptLexer.hpp"

using namespace gstd;

script_scanner::script_scanner(const char* source, const char* end) : current(source), line(1) {
	encoding = Encoding::UTF8;
	current = source;
	endPoint = end;
	line = 1;
	
	encoding = Encoding::Detect(source, (size_t)(end - source));
	bytePerChar = Encoding::GetCharSize(encoding);

	current += Encoding::GetBomSize(encoding);

	advance();
}
script_scanner::script_scanner(const script_scanner& source) {
	encoding = source.encoding;
	bytePerChar = source.bytePerChar;
	current = source.current;
	endPoint = source.endPoint;
	next = source.next;
	token_list = source.token_list;
	word = source.word;
	line = source.line;
}

void script_scanner::copy_state(script_scanner* src) {
	encoding = src->encoding;
	bytePerChar = src->bytePerChar;
	current = src->current;
	endPoint = src->endPoint;
	next = src->next;
	token_list = src->token_list;
	word = src->word;
	real_value = src->real_value;
	char_value = src->char_value;
	string_value = src->string_value;
	line = src->line;
}

wchar_t script_scanner::current_char() {
	return Encoding::BytesToWChar(current, encoding);
}
wchar_t script_scanner::peek_next_char(int index) {
	const char* pos = current + index * bytePerChar;
	if (pos >= endPoint) return L'\0';
	return Encoding::BytesToWChar(pos, encoding);
}
wchar_t script_scanner::next_char() {
	current += bytePerChar;
	return current_char();
}

void script_scanner::skip() {
	wchar_t ch1 = current_char();
	wchar_t ch2 = peek_next_char(1);

	bool bReskip = true;
	while (bReskip) {
		bReskip = false;

		//Skip whitespaces
		if (std::iswspace(ch1)) {
			bReskip = true;
			while (std::iswspace(ch1)) {
				if (ch1 == L'\n') ++line;
				ch1 = next_char();
			}
			ch2 = peek_next_char(1);
		}

		//Skip block comment
		if (ch1 == L'/' && ch2 == L'*') {
			bReskip = true;

			next_char();
			while (true) {
				ch1 = next_char();
				ch2 = peek_next_char(1);

				scanner_assert(current < endPoint, "Block comment unenclosed at end of file.");
				if (ch1 == L'\n') ++line;

				if (ch1 == L'*' && ch2 == L'/') break;
			}
			next_char();

			ch1 = next_char();
			ch2 = peek_next_char(1);
		}

		//Skip line comments and unrecognized #'s
		if (ch1 == L'#' || (ch1 == L'/' && ch2 == L'/')) {
			bReskip = true;
			while (true) {
				ch1 = next_char();
				if (ch1 == L'\n') break;
			}
			//++line;
			ch2 = peek_next_char(1);
		}
	}
}

void script_scanner::AddLog(wchar_t* data) {
	{
		wchar_t* pStart = (wchar_t*)current;
		wchar_t* pEnd = (wchar_t*)(current + std::min(16, endPoint - current));
		std::wstring wstr = std::wstring(pStart, pEnd);
		//Logger::WriteTop(StringUtility::Format(L"%s current=%d, endPoint=%d, val=%d, ch=%s", data, pStart, endPoint, (wchar_t)*current, wstr.c_str()));
	}
}

void script_scanner::advance() {
	skip();

	wchar_t ch = current_char();
	if (ch == L'\0' || current >= endPoint) {
		next = token_kind::tk_end;
		return;
	}

	switch (ch) {
	case L'[':
		next = token_kind::tk_open_bra;
		ch = next_char();
		break;
	case L']':
		next = token_kind::tk_close_bra;
		ch = next_char();
		break;
	case L'(':
		next = token_kind::tk_open_par;
		ch = next_char();
		if (ch == L'|') {
			next = token_kind::tk_open_abs;
			ch = next_char();
		}
		break;
	case L')':
		next = token_kind::tk_close_par;
		ch = next_char();
		break;
	case L'{':
		next = token_kind::tk_open_cur;
		ch = next_char();
		break;
	case L'}':
		next = token_kind::tk_close_cur;
		ch = next_char();
		break;
	case L'@':
		next = token_kind::tk_at;
		ch = next_char();
		break;
	case L',':
		next = token_kind::tk_comma;
		ch = next_char();
		break;
	case L':':
		next = token_kind::tk_colon;
		ch = next_char();
		break;
	case L';':
		next = token_kind::tk_semicolon;
		ch = next_char();
		break;
	case L'?':
		next = token_kind::tk_query;
		ch = next_char();
		break;
	case L'~':
		next = token_kind::tk_tilde;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_concat_assign;
			ch = next_char();
		}
		else if (ch == L'/') {
			next = token_kind::tk_f_slash;
			ch = next_char();
			if (ch == L'=') {
				next = token_kind::tk_fdivide_assign;
				ch = next_char();
			}
		}
		break;
	case L'*':
		next = token_kind::tk_asterisk;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_multiply_assign;
			ch = next_char();
		}
		break;
	case L'/':
		next = token_kind::tk_slash;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_divide_assign;
			ch = next_char();
		}
		break;
	case L'%':
		next = token_kind::tk_percent;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_remainder_assign;
			ch = next_char();
		}
		break;
	case L'^':
		next = token_kind::tk_caret;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_power_assign;
			ch = next_char();
		}
		else if (ch == L'^') {
			next = token_kind::tk_bit_xor;
			ch = next_char();
		}
		break;
	case L'=':
		next = token_kind::tk_assign;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_e;
			ch = next_char();
		}
		break;
	case L'>':
		next = token_kind::tk_g;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_ge;
			ch = next_char();
		}
		else if (ch == L'>') {
			next = token_kind::tk_bit_shf_right;
			ch = next_char();
		}
		break;
	case L'<':
		next = token_kind::tk_l;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_le;
			ch = next_char();
		}
		else if (ch == L'<') {
			next = token_kind::tk_bit_shf_left;
			ch = next_char();
		}
		break;
	case L'!':
		next = token_kind::tk_exclamation;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_ne;
			ch = next_char();
		}
		break;
	case L'+':
		next = token_kind::tk_plus;
		ch = next_char();
		if (ch == L'+') {
			next = token_kind::tk_inc;
			ch = next_char();
		}
		else if (ch == L'=') {
			next = token_kind::tk_add_assign;
			ch = next_char();
		}
		break;
	case L'-':
		next = token_kind::tk_minus;
		ch = next_char();
		if (ch == L'-') {
			next = token_kind::tk_dec;
			ch = next_char();
		}
		else if (ch == L'=') {
			next = token_kind::tk_subtract_assign;
			ch = next_char();
		}
		break;
	case L'&':
		next = token_kind::tk_bit_and;
		ch = next_char();
		if (ch == L'&') {
			next = token_kind::tk_logic_and;
			ch = next_char();
		}
		break;
	case L'|':
		next = token_kind::tk_bit_or;
		ch = next_char();
		if (ch == L'|') {
			next = token_kind::tk_logic_or;
			ch = next_char();
		}
		else if (ch == L')') {
			next = token_kind::tk_close_abs;
			ch = next_char();
		}
		break;
	case L'.':
		ch = next_char();
		if (ch == L'.') {
			next = token_kind::tk_range;
			ch = next_char();
			/*
			if (ch == L'.') {
				next = token_kind::tk_args_variadic;
				ch = next_char();
			}
			*/
		}
		else scanner_assert(false, "Invalid period(.) placement.");
		break;

	case L'\"':
	case L'\'':
	{
		wchar_t enclosing = ch;
		next = ch == L'\"' ? token_kind::tk_string : token_kind::tk_char;

		{
			ch = next_char();
			const char* pBeg = current;
			while (true) {
				if (ch == L'\n') ++line;			//For multiple-lined strings
				else if (ch == L'\\') next_char();	//Skip escaped characters
				else if (ch == enclosing) break;
				ch = next_char();
			}
			const char* pEnd = current;

			next_char();
			string_value = Encoding::BytesToWString(pBeg, pEnd, encoding);
			string_value = StringUtility::ParseStringWithEscape(string_value);
		}

		if (next == token_kind::tk_char) {
			scanner_assert(string_value.size() == 1, "A value of type char may only be one character long.");
			char_value = string_value[0];
		}
		break;
	}
	default:
		if (std::iswdigit(ch)) {
			next = token_kind::tk_real;
			int_value = 0;
			real_value = 0;

			bool bBaseLiteral = false;
			std::string strNum = "";

			if (std::iswdigit(ch)) {
				strNum += ch;
				ch = next_char();
			}
			if (strNum[0] == '0' && (ch == 'b' || ch == 'o' || ch == 'x')) {
				bBaseLiteral = true;

				wchar_t lead = ch;

				ch = next_char();
				scanner_assert(std::iswdigit(ch) || std::iswxdigit(ch), "Invalid base literal.");

				strNum = "";
				do {
					if (ch != '_')
						strNum += ch;
					ch = next_char();
				} while (std::iswdigit(ch) || std::iswxdigit(ch) || ch == '_');

				auto _ValidateAndParseNum = [](std::string& str, int base, int (*funcValidate)(wint_t)) -> uint64_t {
					size_t i = 0;
					for (wchar_t ch : str) {
						if ((i++) >= str.size() - 1) break;
						scanner_assert(funcValidate(ch), "Invalid base literal.");
					}
					return std::strtoull(str.c_str(), nullptr, base);
				};

				uint64_t result = 0;
				if (lead == 'x') {
					result = _ValidateAndParseNum(strNum, 16, std::iswxdigit);
				}
				else if (lead == 'o') {
					result = _ValidateAndParseNum(strNum, 8, 
						[](wint_t ch) -> int { return (ch >= '0') && (ch < '8'); });
					result = std::strtoull(strNum.c_str(), nullptr, 8);
				}
				else if (lead == 'b') {
					result = _ValidateAndParseNum(strNum, 2, 
						[](wint_t ch) -> int { return ch == '0' || ch == '1'; });
				}

				int_value = (int64_t&)result;
				real_value = result;
				next = token_kind::tk_int;
			}
			else {
				while (std::iswdigit(ch) || ch == '_') {
					if (ch != '_')
						strNum += ch;
					ch = next_char();
				};

				wchar_t ch2 = peek_next_char(1);
				if (ch == L'.' && std::iswdigit(ch2)) {
					strNum += ch;
					ch = next_char();
					do {
						if (ch != '_')
							strNum += ch;
						ch = next_char();
					} while (std::iswdigit(ch) || ch == '_');
				}

				//real_value = std::strtod(strNum.c_str(), nullptr);
				//int_value = real_value;
				next = token_kind::tk_real;
			}

			{
				wchar_t suffix = std::towlower(ch);
				if (suffix == L'i') {
					next = token_kind::tk_int;
					ch = next_char();
				}
				else if (suffix == L'r') {
					next = token_kind::tk_real;
					ch = next_char();
				}
			}

			if (!bBaseLiteral) {
				if (next == token_kind::tk_int) {
					int_value = std::strtoll(strNum.c_str(), nullptr, 10);
				}
				else {
					real_value = std::strtod(strNum.c_str(), nullptr);
				}
			}

			break;
		}
		else if (std::iswalpha(ch) || ch == L'_') {
			word = "";
			do {
				word += (char)ch;
				ch = next_char();
			} while (std::iswalpha(ch) || ch == '_' || std::iswdigit(ch));

			auto itr = token_map.find(word);
			if (itr != token_map.end())
				next = itr->second;
			else
				next = token_kind::tk_word;
		}
		else {
			next = token_kind::tk_invalid;
		}
	}

	{
		prev_ptr_list.push_front(current);
		if (prev_ptr_list.size() > MAX_TOKEN_LIST) prev_ptr_list.pop_back();

		token_list.push_front(next);
		if (token_list.size() > MAX_TOKEN_LIST) token_list.pop_back();
	}
}

std::unordered_map<std::string, token_kind> script_scanner::token_map = {
	{ "let", token_kind::tk_decl_auto },
	{ "var", token_kind::tk_decl_auto },
	{ "void", token_kind::tk_decl_void },
	{ "real", token_kind::tk_decl_real },
	{ "int", token_kind::tk_decl_int },
	{ "char", token_kind::tk_decl_char },
	{ "string", token_kind::tk_decl_string },
	{ "bool", token_kind::tk_decl_bool },

	{ "const", token_kind::tk_decl_mod_const },
	{ "ref", token_kind::tk_decl_mod_ref },

	{ "as_int", token_kind::tk_cast_int },
	{ "as_real", token_kind::tk_cast_real },
	{ "as_char", token_kind::tk_cast_char },
	{ "as_bool", token_kind::tk_cast_bool },
	{ "as_string", token_kind::tk_cast_string },

	{ "length", token_kind::tk_LENGTH },

	{ "alternative", token_kind::tk_ALTERNATIVE },
	//{ "switch", token_kind::tk_ALTERNATIVE },
	{ "case", token_kind::tk_CASE },
	{ "others", token_kind::tk_OTHERS },
	//{ "default", token_kind::tk_OTHERS },
	{ "if", token_kind::tk_IF },
	{ "else", token_kind::tk_ELSE },
	{ "loop", token_kind::tk_LOOP },
	{ "times", token_kind::tk_TIMES },
	{ "while", token_kind::tk_WHILE },
	{ "for", token_kind::tk_FOR },
	{ "each", token_kind::tk_EACH },
	{ "ascent", token_kind::tk_ASCENT },
	{ "descent", token_kind::tk_DESCENT },
	{ "in", token_kind::tk_IN },

	{ "local", token_kind::tk_LOCAL },
	{ "function", token_kind::tk_FUNCTION },
	{ "sub", token_kind::tk_SUB },
	{ "task", token_kind::tk_TASK },
	{ "async", token_kind::tk_ASYNC },

	{ "continue", token_kind::tk_CONTINUE },
	{ "break", token_kind::tk_BREAK },
	{ "return", token_kind::tk_RETURN },

	{ "yield", token_kind::tk_YIELD },
	{ "wait", token_kind::tk_WAIT },

	{ "true", token_kind::tk_TRUE },
	{ "false", token_kind::tk_FALSE },
};
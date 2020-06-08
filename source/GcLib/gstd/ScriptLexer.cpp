#include "source/GcLib/pch.h"

#include "ScriptLexer.hpp"

#ifdef _MSC_VER
namespace std {
	using::wcstombs;
	using::mbstowcs;
	using::isalpha;
	using::fmodl;
	using::powl;
	using::swprintf;
	using::atof;
	using::isdigit;
	using::isxdigit;
	using::floorl;
	using::ceill;
	using::fabsl;
	using::iswdigit;
	using::iswalpha;
}

#endif

using namespace gstd;

wchar_t script_scanner::current_char() {
	wchar_t res = L'\0';
	if (encoding == Encoding::UTF16LE || encoding == Encoding::UTF16BE) {
		res = (wchar_t&)current[0];
		if (encoding == Encoding::UTF16BE)
			res = (res >> 8) | (res << 8);
	}
	else {
		res = *current;
	}
	return res;
}
wchar_t script_scanner::index_from_current_char(int index) {
	wchar_t res = L'\0';
	if (encoding == Encoding::UTF16LE || encoding == Encoding::UTF16BE) {
		const char* pos = current + index * 2;
		if (pos >= endPoint)return L'\0';
		res = (wchar_t&)current[index * 2];
		if (encoding == Encoding::UTF16BE)
			res = (res >> 8) | (res << 8);
	}
	else {
		const char* pos = current + index;
		if (pos >= endPoint)return L'\0';
		res = current[index];
	}

	return res;
}
wchar_t script_scanner::next_char() {
	if (encoding == Encoding::UTF16LE || encoding == Encoding::UTF16BE) {
		current += 2;
	}
	else {
		++current;
	}

	wchar_t res = current_char();
	return res;
}

void script_scanner::skip() {
	//‹ó”’‚ð”ò‚Î‚·
	wchar_t ch1 = current_char();
	wchar_t ch2 = index_from_current_char(1);
	while (ch1 == '\r' || ch1 == '\n' || ch1 == L'\t' || ch1 == L' '
		|| ch1 == L'#' || (ch1 == L'/' && (ch2 == L'/' || ch2 == L'*'))) {
		//ƒRƒƒ“ƒg‚ð”ò‚Î‚·
		if (ch1 == L'#' ||
			(ch1 == L'/' && (ch2 == L'/' || ch2 == L'*'))) {
			if (ch1 == L'#' || ch2 == L'/') {
				do {
					ch1 = next_char();
				} while (ch1 != L'\r' && ch1 != L'\n');
			}
			else {
				next_char();
				ch1 = next_char();
				ch2 = index_from_current_char(1);
				while (ch1 != L'*' || ch2 != L'/') {
					if (ch1 == L'\n') ++line;
					ch1 = next_char();
					ch2 = index_from_current_char(1);
				}
				ch1 = next_char();
				ch1 = next_char();
			}
		}
		else if (ch1 == L'\n') {
			++line;
			ch1 = next_char();
		}
		else
			ch1 = next_char();
		ch2 = index_from_current_char(1);
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
	case L'~':
		next = token_kind::tk_tilde;
		ch = next_char();
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
		break;
	case L'<':
		next = token_kind::tk_l;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_le;
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
		next = token_kind::tk_ampersand;
		ch = next_char();
		if (ch == L'&') {
			next = token_kind::tk_logic_and;
			ch = next_char();
		}
		break;
	case L'|':
		next = token_kind::tk_vertical;
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
		}
		else {
			throw parser_error("Invalid period(.) placement.\r\n");
		}
		break;

	case L'\'':
	case L'\"':
	{
		wchar_t q = current_char();
		next = (q == L'\"') ? token_kind::tk_string : token_kind::tk_char;
		ch = next_char();
		wchar_t pre = (wchar_t)next;
		if (encoding == Encoding::UTF16LE || encoding == Encoding::UTF16BE) {
			std::wstring s;
			while (true) {
				if (ch == q && pre != L'\\')break;

				if (ch == L'\\') {
					if (pre == L'\\')s += ch;
				}
				else {
					s += ch;
				}

				pre = ch;
				ch = next_char();
				if (ch == L'\n') ++line;	//For multiple-lined strings
			}
			ch = next_char();
			string_value = s;
		}
		else {
			std::string s;
			while (true) {
				if (ch == q && pre != L'\\')break;

				if (ch == L'\\') {
					if (pre == L'\\')s += *current;
				}
				else {
					s += *current;
				}

				pre = ch;
				ch = next_char();
				if (ch == L'\n') ++line;	//For multiple-lined strings
			}
			ch = next_char();
			string_value = StringUtility::ConvertMultiToWide(s);
		}

		if (q == L'\'') {
			if (string_value.size() == 1)
				char_value = string_value[0];
			else
				throw parser_error("A value of type char may only be one character long.");
		}
	}
	break;
	case L'\\':
	{
		ch = next_char();
		next = token_kind::tk_char;
		wchar_t c = ch;
		ch = next_char();
		switch (c) {
		case L'0':
			char_value = L'\0';
			break;
		case L'n':
			char_value = L'\n';
			break;
		case L'r':
			char_value = L'\r';
			break;
		case L't':
			char_value = L'\t';
			break;
		case L'x':
			char_value = 0;
			while (std::isxdigit(ch)) {
				char_value = char_value * 16 + (ch >= L'a') ? ch - L'a' + 10 : (ch >= L'A') ?
					ch - L'A' + 10 : ch - L'0';
				ch = next_char();
			}
			break;
		default:
		{
			throw parser_error("Invalid character.\r\n");
		}
		}
	}
	break;
	default:
		if (std::iswdigit(ch)) {
			next = token_kind::tk_real;
			real_value = 0.0;

			bool has_decimal_part = false;
			std::string str_num = "";
			do {
				str_num += ch;
				ch = next_char();
			} while (std::iswdigit(ch) || std::iswalpha(ch));
			{
				wchar_t ch2 = index_from_current_char(1);
				if (ch == L'.' && std::iswdigit(ch2)) {
					has_decimal_part = true;
					str_num += ch;
					ch = next_char();
					while (std::iswdigit(ch) || std::iswalpha(ch)) {
						str_num += ch;
						ch = next_char();
					}
				}
			}

			std::smatch base_match;
			if (std::regex_match(str_num, base_match, std::regex("0x([0-9a-fA-F]+)"))) {
				real_value = (double)std::strtol(base_match[1].str().c_str(), nullptr, 16);
				if (has_decimal_part) goto throw_err_no_decimal;
			}
			else if (std::regex_match(str_num, base_match, std::regex("0o([0-7]+)"))) {
				real_value = (double)std::strtol(base_match[1].str().c_str(), nullptr, 8);
				if (has_decimal_part) goto throw_err_no_decimal;
			}
			else if (std::regex_match(str_num, base_match, std::regex("0b([0-1]+)"))) {
				real_value = (double)std::strtol(base_match[1].str().c_str(), nullptr, 2);
				if (has_decimal_part) goto throw_err_no_decimal;
			}
			else if (std::regex_match(str_num, base_match, std::regex("[0-9]+(\.[0-9]+)?"))) {
				real_value = std::strtod(base_match[0].str().c_str(), nullptr);
			}
			else {
				throw parser_error("Invalid number.\r\n");
			}

			break;
throw_err_no_decimal:
			throw parser_error("Cannot create a decimal number in base literals.\r\n");
		}
		else if (std::iswalpha(ch) || ch == L'_') {
			if (encoding == Encoding::UTF16LE || encoding == Encoding::UTF16BE) {
				word = "";
				do {
					word += (char)ch;
					ch = next_char();
				} while (std::iswalpha(ch) || ch == '_' || std::iswdigit(ch));
			}
			else {
				char* pStart = (char*)current;
				char* pEnd = pStart;
				do {
					ch = next_char();
					pEnd = (char*)current;
				} while (std::iswalpha(ch) || ch == '_' || std::iswdigit(ch));
				word = std::string(pStart, pEnd);
			}

			std::unordered_map<std::string, token_kind>::iterator itr = token_map.find(word);
			if (itr != token_map.end())
				next = itr->second;
			else
				next = token_kind::tk_word;
		}
		else {
			next = token_kind::tk_invalid;
		}
	}
}

std::unordered_map<std::string, token_kind> script_scanner::token_map = {
	{ "let", token_kind::tk_let },
	{ "var", token_kind::tk_let },
	{ "const", token_kind::tk_const },
	{ "real", token_kind::tk_def_real },
	//{ "char", token_kind::tk_def_char },
	//{ "string", token_kind::tk_def_string },
	//{ "bool", token_kind::tk_def_bool },
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
	{ "continue", token_kind::tk_CONTINUE },
	{ "break", token_kind::tk_BREAK },
	{ "return", token_kind::tk_RETURN },
	{ "yield", token_kind::tk_YIELD },
	{ "wait", token_kind::tk_WAIT },
	{ "true", token_kind::tk_TRUE },
	{ "false", token_kind::tk_FALSE },
};
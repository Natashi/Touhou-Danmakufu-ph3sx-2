#pragma once

#include "../../pch.h"
#include "../GstdUtility.hpp"

namespace gstd {
	/* parser_error */
	class parser_error : public gstd::wexception {
	public:
		parser_error(const std::wstring& msg) : wexception(msg) {};
		parser_error(const std::string& msg) : wexception(msg) {};
	};
	class parser_error_mapped : public gstd::wexception {
	public:
		parser_error_mapped(int line, const std::wstring& msg) : wexception(msg), line_(line) {};
		parser_error_mapped(int line, const std::string& msg) : wexception(msg), line_(line) {};

		int GetLine() { return line_; }
	private:
		int line_;
	};

	/* lexical analyzer */
	enum class token_kind : uint8_t {
		tk_end, tk_invalid,
		tk_word, tk_real, tk_char, tk_string,
		tk_open_par, tk_close_par, tk_open_bra, tk_close_bra, tk_open_cur, tk_close_cur,
		tk_open_abs, tk_close_abs, tk_comma, tk_semicolon, tk_colon, tk_query, tk_tilde, tk_assign,
		tk_plus, tk_minus, tk_asterisk, tk_slash, tk_percent, tk_caret, tk_exclamation, tk_ampersand,
		tk_e, tk_g, tk_ge, tk_l, tk_le, tk_ne, tk_logic_and, tk_logic_or, 
		tk_vertical, tk_at,
		tk_inc, tk_dec, tk_range, tk_args_variadic,
		tk_add_assign, tk_subtract_assign, tk_multiply_assign, tk_divide_assign, tk_remainder_assign, tk_power_assign,

		tk_decl_auto, tk_const,
		tk_decl_real, tk_decl_char, tk_decl_string, tk_decl_bool,
		tk_cast_int, tk_cast_real, tk_cast_char, tk_cast_bool,
		tk_ALTERNATIVE, tk_CASE, tk_OTHERS,
		tk_IF, tk_ELSE, tk_LOOP, tk_TIMES, tk_WHILE, tk_FOR, tk_EACH, tk_ASCENT, tk_DESCENT, tk_IN,
		tk_LOCAL, tk_FUNCTION, tk_SUB, tk_TASK,
		tk_CONTINUE, tk_BREAK, tk_RETURN,
		tk_YIELD, tk_WAIT,
		tk_TRUE, tk_FALSE,
	};

	class script_scanner {
		int encoding;
		const char* current;
		const char* endPoint;

		static std::unordered_map<std::string, token_kind> token_map;

		wchar_t current_char();
		wchar_t index_from_current_char(int index);
		inline wchar_t next_char();

		wchar_t parse_escape_char();
		wchar_t parse_utf8_char();
	public:
		token_kind next;
		std::string word;
		double real_value;
		wchar_t char_value;
		std::wstring string_value;
		int line;

		script_scanner(const char* source, const char* end) : current(source), line(1) {
			endPoint = end;
			encoding = Encoding::UTF8;
			if (Encoding::IsUtf16Le(source, 2)) {
				encoding = Encoding::UTF16LE;
				current += 2;
			}
			else if (Encoding::IsUtf16Be(source, 2)) {
				encoding = Encoding::UTF16BE;
				current += 2;
			}

			advance();
		}
		script_scanner(const script_scanner& source) :
			encoding(source.encoding),
			current(source.current), endPoint(source.endPoint),
			next(source.next),
			word(source.word),
			line(source.line) {
		}

		void copy_state(script_scanner* src) {
			encoding = src->encoding;
			current = src->current;
			endPoint = src->endPoint;
			next = src->next;
			word = src->word;
			real_value = src->real_value;
			char_value = src->char_value;
			string_value = src->string_value;
			line = src->line;
		}

		void skip();
		void advance();

		void AddLog(wchar_t* data);
	};
}
#pragma once
#include "../pch.h"

#include "GstdUtility.hpp"

namespace gstd {
	/* parser_error */
	class parser_error : public gstd::wexception {
	public:
		parser_error(std::wstring const& the_message) : gstd::wexception(the_message) {}
		parser_error(std::string const& the_message) : gstd::wexception(the_message) {}
	private:

	};

	/* lexical analyzer */
	enum class token_kind : uint8_t {
		tk_end, tk_invalid,
		tk_word, tk_real, tk_char, tk_string,
		tk_open_par, tk_close_par, tk_open_bra, tk_close_bra, tk_open_cur, tk_close_cur,
		tk_open_abs, tk_close_abs, tk_comma, tk_semicolon, tk_colon, tk_tilde, tk_assign, tk_plus, tk_minus,
		tk_asterisk, tk_slash, tk_percent, tk_caret, tk_e, tk_g, tk_ge, tk_l, tk_le, tk_ne, tk_exclamation, tk_ampersand,
		tk_logic_and, tk_logic_or, tk_vertical, tk_at,
		tk_inc, tk_dec, tk_range,
		tk_add_assign, tk_subtract_assign, tk_multiply_assign, tk_divide_assign, tk_remainder_assign, tk_power_assign,

		tk_let, tk_const,
		tk_def_real, tk_def_char, tk_def_string, tk_def_bool,
		tk_ALTERNATIVE, tk_CASE, tk_OTHERS,
		tk_IF, tk_ELSE, tk_LOOP, tk_TIMES, tk_WHILE, tk_FOR, tk_EACH, tk_ASCENT, tk_DESCENT, tk_IN,
		tk_LOCAL, tk_FUNCTION, tk_SUB, tk_TASK,
		tk_CONTINUE, tk_BREAK, tk_RETURN,
		tk_YIELD, tk_WAIT,
		tk_TRUE, tk_FALSE,
	};

	class script_scanner {
		int encoding;
		char const* current;
		char const* endPoint;

		static std::unordered_map<std::string, token_kind> token_map;

		inline wchar_t current_char();
		inline wchar_t index_from_current_char(int index);
		inline wchar_t next_char();
	public:
		token_kind next;
		std::string word;
		double real_value;
		wchar_t char_value;
		std::wstring string_value;
		int line;

		script_scanner(char const* source, char const* end) : current(source), line(1) {
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
		script_scanner(script_scanner const& source) :
			encoding(source.encoding),
			current(source.current), endPoint(source.endPoint),
			next(source.next),
			word(source.word),
			line(source.line) {
		}

		void copy_state(const script_scanner& src) {
			encoding = src.encoding;
			current = src.current;
			endPoint = src.endPoint;
			next = src.next;
			word = src.word;
			real_value = src.real_value;
			char_value = src.char_value;
			string_value = src.string_value;
			line = src.line;
		}

		void skip();
		void advance();

		void AddLog(wchar_t* data);
	};
}
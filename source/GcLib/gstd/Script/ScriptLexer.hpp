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

		tk_word, tk_int, tk_float, tk_char, tk_string,
		tk_open_par, tk_close_par, tk_open_bra, tk_close_bra, tk_open_cur, tk_close_cur,

		tk_open_abs, tk_close_abs, tk_comma, tk_semicolon, tk_colon, tk_query, tk_tilde, tk_assign,
		tk_plus, tk_minus, tk_asterisk, tk_slash, tk_f_slash, tk_percent, tk_caret, tk_exclamation,

		tk_add_assign, tk_subtract_assign, tk_multiply_assign,
		tk_divide_assign, tk_fdivide_assign, tk_remainder_assign,
		tk_power_assign, tk_concat_assign,

		tk_e, tk_g, tk_ge, tk_l, tk_le, tk_ne, 
		tk_logic_and, tk_logic_or, 
		tk_bit_and, tk_bit_or, tk_bit_xor, tk_bit_shf_left, tk_bit_shf_right, 
		tk_at, 
		tk_inc, tk_dec, tk_range, tk_args_variadic,

		tk_decl_auto, tk_decl_void,
		tk_decl_int, tk_decl_float, tk_decl_char, tk_decl_bool, tk_decl_string,

		tk_decl_mod_const, tk_decl_mod_ref,

		tk_cast_int, tk_cast_float, tk_cast_char, tk_cast_bool, tk_cast_string,

		tk_LENGTH, 

		tk_ALTERNATIVE, tk_CASE, tk_OTHERS,
		tk_IF, tk_ELSE, tk_LOOP, tk_TIMES, tk_WHILE, tk_FOR, tk_EACH, tk_ASCENT, tk_DESCENT, tk_IN,
		tk_LOCAL, tk_FUNCTION, tk_SUB, tk_TASK, tk_ASYNC,
		tk_CONTINUE, tk_BREAK, tk_RETURN,
		tk_YIELD, tk_WAIT,

		tk_TRUE, tk_FALSE,
	};

	class script_scanner {
		static std::unordered_map<std::string, token_kind> token_map;

		const wchar_t* current;
		const wchar_t* endPoint;

		wchar_t current_char();
		wchar_t peek_next_char(int index);
		inline wchar_t next_char();

		static void scanner_assert(bool expr, const std::string& error) {
			if (!expr)
				throw parser_error(error + "\r\n");
		}
	public:
		enum {
			MAX_TOKEN_LIST = 64,
		};
	public:
		token_kind next;

		std::list<const wchar_t*> prev_ptr_list;
		std::list<token_kind> token_list;

		std::string word;
		int64_t int_value;
		double float_value;
		wchar_t char_value;
		std::wstring string_value;
		int line;

		script_scanner(const wchar_t* source, const wchar_t* end);
		script_scanner(const script_scanner& source);

		void copy_state(const script_scanner* src);

		const wchar_t* get() { return current; }
		inline std::wstring ToStr(const wchar_t* b, const wchar_t* e) {
			return std::wstring(b, e);
		}

		void skip();
		void advance();
	};
}
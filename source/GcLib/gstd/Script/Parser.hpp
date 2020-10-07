#pragma once

#include "../../pch.h"

#include "../GstdUtility.hpp"
#include "ScriptLexer.hpp"
#include "ScriptFunction.hpp"

#pragma warning (disable : 4786)	//STL Warningó}é~
#pragma warning (disable : 4018)	//signed Ç∆ unsigned ÇÃêîílÇî‰är
#pragma warning (disable : 4244)	//double' Ç©ÇÁ 'float' Ç…ïœä∑

namespace gstd {
	enum class command_kind : uint8_t {
		pc_var_alloc, pc_var_format,

		pc_copy_assign, pc_ref_overwrite, 
		pc_break_loop, pc_break_routine,
		pc_call, pc_call_and_push_result,

		pc_jump_target,
		pc_jump, pc_jump_if, pc_jump_if_not,
		pc_jump_if_nopop, pc_jump_if_not_nopop,
		_pc_jump, _pc_jump_if, _pc_jump_if_not,
		_pc_jump_if_nopop, _pc_jump_if_not_nopop,

		pc_compare_e, pc_compare_g, pc_compare_ge, pc_compare_l,
		pc_compare_le, pc_compare_ne,

		pc_dup_n, pc_dup_n_unique,

		pc_loop_ascent, pc_loop_descent, pc_loop_count, pc_loop_foreach,
		pc_loop_continue,

		pc_construct_array,
		pc_pop, pc_push_value, pc_push_variable, pc_swap, pc_yield, pc_wait,

		//Inline operations
		pc_inline_top_inc, pc_inline_top_dec,
		pc_inline_inc, pc_inline_dec,

		pc_inline_add_asi, pc_inline_sub_asi, pc_inline_mul_asi, pc_inline_div_asi, pc_inline_mod_asi, 
		pc_inline_pow_asi, pc_inline_cat_asi,

		pc_inline_neg, pc_inline_not, pc_inline_abs,
		pc_inline_add, pc_inline_sub, pc_inline_mul, pc_inline_div, pc_inline_mod, pc_inline_pow,
		pc_inline_app, pc_inline_cat,

		pc_inline_cmp_e, pc_inline_cmp_g, pc_inline_cmp_ge, pc_inline_cmp_l, pc_inline_cmp_le, pc_inline_cmp_ne,
		pc_inline_logic_and, pc_inline_logic_or,

		pc_inline_cast_var,
		pc_inline_index_array,
		pc_inline_length_array,

		pc_null = 0xff,
	};
	enum class block_kind : uint8_t {
		bk_normal, bk_sub, bk_function, bk_microthread
	};

	struct code;
	struct script_block {
		int level;
		int arguments;
		std::string name;
		callback func;
		std::vector<code> codes;
		block_kind kind;

		script_block(int the_level, block_kind the_kind);
	};
	struct code {
		command_kind command = command_kind::pc_null;
		int line = -1;
#ifdef _DEBUG
		std::string var_name;	//For assign/push_variable
#endif

		union {
			struct {	//assign/push_variable
				int level;
				size_t variable;
			};
			struct {	//call/call_and_push_result
				script_block* sub;
				size_t arguments;
			};
			struct {	//loop_back
				size_t ip;
			};
			struct {	//var_format
				size_t off;
				size_t len;
			};
			struct {	//push_value
				value data;
			};
		};

		code();
		code(int the_line, command_kind the_command);
		code(int the_line, command_kind the_command, int the_level, size_t the_variable, const std::string& the_name);
		code(int the_line, command_kind the_command, script_block* the_sub, int the_arguments);
		code(int the_line, command_kind the_command, size_t the_ip);
		code(int the_line, command_kind the_command, size_t the_off, size_t the_len);
		code(int the_line, command_kind the_command, const value& the_data);

		code(command_kind the_command) : code(0, the_command) {}
		code(command_kind the_command, int the_level, size_t the_variable, const std::string& the_name)
			: code(0, the_command, the_level, the_variable, the_name) {}
		code(command_kind the_command, script_block* the_sub, int the_arguments)
			: code(0, the_command, the_sub, the_arguments) {}
		code(command_kind the_command, size_t the_ip) : code(0, the_command, the_ip) {}
		code(command_kind the_command, size_t the_off, size_t the_len) : code(0, the_command, the_off, the_len) {}
		code(command_kind the_command, const value& the_data) : code(0, the_command, the_data) {}

		code(const code& src);

		~code();

		code& operator=(const code& src);
	};

	class parser {
	private:
		//Have a blatant name plagiarisation from thecl. Good morning.
		class parser_state_t {
		public:
			parser_state_t* state_pred;
			script_scanner* lex;
			size_t ip;
			size_t var_count_main;
			size_t var_count_sub;

			parser_state_t() : state_pred(nullptr), lex(nullptr), ip(0) {
				var_count_main = 0;
				var_count_sub = 0;
			}
			parser_state_t(script_scanner* _lex) : parser_state_t() {
				lex = _lex;
			}
			parser_state_t(script_scanner* _lex, size_t _ip) : parser_state_t(_lex) {
				ip = _ip;
			}

			void AddCode(script_block* bk, code&& cd) {
				cd.line = lex->line;
				bk->codes.push_back(cd);
				++ip;
			}
			void PopCode(script_block* bk) {
				bk->codes.pop_back();
				--ip;
			}
			void GrowVarCount(size_t count) {
				var_count_sub = std::max(var_count_sub, count);
			}

			token_kind next() const { return lex->next; }
			void advance() const { return lex->advance(); }
		};
	public:
		struct symbol {
			int level;
			script_block* sub;
			int variable;
			bool can_overload;
			bool can_modify;		//Applies to the scripter, not the engine

			symbol() : level(0), sub(nullptr), variable(-1), can_overload(true), can_modify(true) {}
			symbol(int lv, script_block* pSub, int var, bool bOver, bool bMod) : level(lv), sub(pSub), 
				variable(var), can_overload(bOver), can_modify(bMod) {}
		};

		struct scope_t : public std::multimap<std::string, symbol> {
			block_kind kind;

			scope_t(block_kind the_kind) : kind(the_kind) {}

			void singular_insert(const std::string& name, const symbol& s, int argc = 0);
		};

		std::list<scope_t> frame;
		script_scanner* lexer_main;
		script_engine* engine;
		bool error;
		std::wstring error_message;
		int error_line;
		std::map<std::string, script_block*> events;

		script_block* block_const_reg;
		size_t count_base_constants;

		parser(script_engine* e, script_scanner* s);
		virtual ~parser() {}

		void load_functions(std::vector<function>* list_func);
		void load_constants(std::vector<constant>* list_const);
		void begin_parse();

		void parse_parentheses(script_block* block, parser_state_t* state);
		void parse_clause(script_block* block, parser_state_t* state);
		void parse_prefix(script_block* block, parser_state_t* state);
		void parse_suffix(script_block* block, parser_state_t* state);
		void parse_product(script_block* block, parser_state_t* state);
		void parse_sum(script_block* block, parser_state_t* state);
		void parse_bitwise_shift(script_block* block, parser_state_t* state);
		void parse_comparison(script_block* block, parser_state_t* state);
		void parse_bitwise(script_block* block, parser_state_t* state);
		void parse_logic(script_block* block, parser_state_t* state);
		void parse_ternary(script_block* block, parser_state_t* state);
		void parse_expression(script_block* block, parser_state_t* state);

		int parse_arguments(script_block* block, parser_state_t* state);
		void parse_single_statement(script_block* block, parser_state_t* state, 
			bool check_terminator, token_kind statement_terminator);
		void parse_statements(script_block* block, parser_state_t* state,
			token_kind block_terminator, token_kind statement_terminator);
		std::pair<size_t, std::array<size_t, 2>> parse_block(script_block* block, parser_state_t* state,
			const std::vector<std::string>* args, bool allow_single = false);
		size_t parse_block_inlined(script_block* block, parser_state_t* state, bool allow_single = true);
	private:
		void register_function(const function& func);
		symbol* search(const std::string& name, scope_t** ptrScope = nullptr);
		symbol* search(const std::string& name, int argc, scope_t** ptrScope = nullptr);
		symbol* search_in(scope_t* scope, const std::string& name);
		symbol* search_in(scope_t* scope, const std::string& name, int argc);
		symbol* search_result();

		int scan_current_scope(parser_state_t* state, int level, const std::vector<std::string>* args,
			bool adding_result, int initVar = 0);

		void write_operation(script_block* block, parser_state_t* state, const char* name, int clauses);
		void write_operation(script_block* block, parser_state_t* state, const symbol* s, int clauses);

		void optimize_expression(script_block* block, parser_state_t* state);
		void link_jump(script_block* block, parser_state_t* state, size_t ip_off);
		void link_break_continue(script_block* block, parser_state_t* state, 
			size_t ip_begin, size_t ip_end, size_t ip_break, size_t ip_continue);
		void scan_final(script_block* block, parser_state_t* state);

		inline static void parser_assert(bool expr, const std::wstring& error);
		inline static void parser_assert(bool expr, const std::string& error);
		inline static void parser_assert(parser_state_t* state, bool expr, const std::wstring& error);
		inline static void parser_assert(parser_state_t* state, bool expr, const std::string& error);
		inline static void parser_assert(int line, bool expr, const std::wstring& error);
		inline static void parser_assert(int line, bool expr, const std::string& error);

		inline static bool test_variadic(int require, int argc);
		inline static bool IsDeclToken(token_kind tk);

		inline static command_kind get_replacing_jump(command_kind c);
	};

	void parser::parser_assert(bool expr, const std::wstring& error) {
		if (!expr)
			throw parser_error(error);
	}
	void parser::parser_assert(bool expr, const std::string& error) {
		if (!expr)
			throw parser_error(error);
	}
	void parser::parser_assert(parser_state_t* state, bool expr, const std::wstring& error) {
		if (!expr)
			throw parser_error_mapped(state->lex->line, error);
	}
	void parser::parser_assert(parser_state_t* state, bool expr, const std::string& error) {
		if (!expr)
			throw parser_error_mapped(state->lex->line, error);
	}
	void parser::parser_assert(int line, bool expr, const std::wstring& error) {
		if (!expr)
			throw parser_error_mapped(line, error);
	}
	void parser::parser_assert(int line, bool expr, const std::string& error) {
		if (!expr)
			throw parser_error_mapped(line, error);
	}

	bool parser::test_variadic(int require, int argc) {
		return (require <= -1) && (argc >= (-require - 1));
	}
	bool parser::IsDeclToken(token_kind tk) {
		return tk == token_kind::tk_decl_auto || tk == token_kind::tk_decl_real;
		// || tk == token_kind::tk_decl_int || tk == token_kind::tk_decl_char 
		// || tk == token_kind::tk_decl_bool || tk == token_kind::tk_decl_string;
	}

	command_kind parser::get_replacing_jump(command_kind c) {
		switch (c) {
		case command_kind::_pc_jump:
			return command_kind::pc_jump;
		case command_kind::_pc_jump_if:
			return command_kind::pc_jump_if;
		case command_kind::_pc_jump_if_not:
			return command_kind::pc_jump_if_not;
		case command_kind::_pc_jump_if_nopop:
			return command_kind::pc_jump_if_nopop;
		case command_kind::_pc_jump_if_not_nopop:
			return command_kind::pc_jump_if_not_nopop;
		}
		return command_kind::pc_jump_target;
	}
}
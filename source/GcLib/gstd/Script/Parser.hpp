#pragma once

#include "../../pch.h"

#include "../GstdUtility.hpp"
#include "ScriptLexer.hpp"
#include "ScriptFunction.hpp"

namespace gstd {
	enum class command_kind : uint8_t {
		pc_yield,				//Transfer control to next thread
		pc_wait,				//Set nWait to ({esp-0} - 1), and cause thread to do pc_yield until (nWait-- == 0)

		pc_var_alloc,			//Resize variable array to [arg0]
		pc_var_format,			//Set variable array (from=[arg0], length=[arg1]) to empty

		pc_pop,					//Pop [arg0] values from stack
		pc_push_value,			//Push value=[data] to stack
		pc_push_variable,		//Push value of variable=[arg0, arg1] to stack
		pc_push_variable2,		//Push pointer of variable=[arg0, arg1] to stack
		pc_dup_n,				//Push {esp-[arg0]} to stack
		pc_swap,				//Swap {esp-0} and {esp-1}
		pc_load_ptr,			//Push &{esp-[arg0]} to the stack
		pc_unload_ptr,			//Dereferences {esp-[arg0]}
		pc_make_unique,			//Turns {esp-[arg0]} into a unique array

		pc_copy_assign,			//Copy variable=[arg0, arg1] to {esp-0}
		pc_ref_assign,			//Set *{esp-1} to {esp-0}

		pc_sub_return,			//Return from a function/task/sub

		pc_call,				//Call (script_block*)[arg0] with argc=[arg1]
		pc_call_and_push_result,//pc_call, and push result to stack

		pc_jump,				//Jump to [arg0]
		pc_jump_if,				//Jump to [arg0] if ({esp-0} == true), pop stack
		pc_jump_if_not,			//Jump to [arg0] if ({esp-0} == false), pop stack
		pc_jump_if_nopop,		//Jump to [arg0] if ({esp-0} == true)
		pc_jump_if_not_nopop,	//Jump to [arg0] if ({esp-0} == false)
		pc_jump_target,				//Parser dummy
		_pc_jump,					//Parser dummy
		_pc_jump_if,				//Parser dummy
		_pc_jump_if_not,			//Parser dummy
		_pc_jump_if_nopop, 			//Parser dummy
		_pc_jump_if_not_nopop,		//Parser dummy

		pc_compare_e,			//Push ({esp-0} == 0) to stack
		pc_compare_g, 			//Push ({esp-0} > 0) to stack
		pc_compare_ge, 			//Push ({esp-0} >= 0) to stack
		pc_compare_l,			//Push ({esp-0} < 0) to stack
		pc_compare_le, 			//Push ({esp-0} <= 0) to stack
		pc_compare_ne,			//Push ({esp-0} != 0) to stack

		pc_loop_ascent,			//Compare({esp-1}, {esp-0}), do pc_compare_le on result and push to stack
		pc_loop_descent,		//Compare({esp-1}, {esp-0}), do pc_compare_ge on result and push to stack
		pc_loop_count, 			//Do pc_compare_g {esp-0}, push result to stack, do (--{esp-0}) if false
		pc_loop_foreach,		//Push true if {esp-0} is larger than array {esp-1}, false otherwise and do (++{esp-0})
		pc_loop_continue,			//Parser dummy
		pc_loop_break,				//Parser dummy

		pc_construct_array,		//Create array of length [arg0] from values {esp-0} to {esp-[arg0]}, and push to stack

		//------------------------------------------------------------------------
		//Inline operations
		//------------------------------------------------------------------------
		pc_inline_inc,			//If [arg0]: (++(variable=[arg1, arg2])), else: (++{esp-0}) and pop stack if [arg1]
		pc_inline_dec,			//If [arg0]: (--(variable=[arg1, arg2])), else: (--{esp-0}) and pop stack if [arg1]

		pc_inline_add_asi,		//If [arg0]: ((variable=[arg1, arg2]) += {esp-0}), else: (*{esp-1} += {esp-0})
		pc_inline_sub_asi,		//If [arg0]: ((variable=[arg1, arg2]) -= {esp-0}), else: (*{esp-1} -= {esp-0})
		pc_inline_mul_asi,		//If [arg0]: ((variable=[arg1, arg2]) *= {esp-0}), else: (*{esp-1} *= {esp-0})
		pc_inline_div_asi,		//If [arg0]: ((variable=[arg1, arg2]) /= {esp-0}), else: (*{esp-1} /= {esp-0})
		pc_inline_fdiv_asi,		//If [arg0]: ((variable=[arg1, arg2]) ~/= {esp-0}), else: (*{esp-1} ~/= {esp-0})
		pc_inline_mod_asi,		//If [arg0]: ((variable=[arg1, arg2]) %= {esp-0}), else: (*{esp-1} %= {esp-0})
		pc_inline_pow_asi,		//If [arg0]: ((variable=[arg1, arg2]) ^= {esp-0}), else: (*{esp-1} ^= {esp-0})
		pc_inline_cat_asi,		//If [arg0]: ((variable=[arg1, arg2]) ~= {esp-0}), else: (*{esp-1} ~= {esp-0})

		pc_inline_neg,			//Push (-{esp-0}) to stack
		pc_inline_not,			//Push (!{esp-0}) to stack
		pc_inline_abs,			//Push abs({esp-0}) to stack

		pc_inline_add,			//Push ({esp-1} + {esp-0}) to stack
		pc_inline_sub,			//Push ({esp-1} - {esp-0}) to stack
		pc_inline_mul,			//Push ({esp-1} * {esp-0}) to stack
		pc_inline_div,			//Push ({esp-1} / {esp-0}) to stack
		pc_inline_fdiv,			//Push ({esp-1} ~/ {esp-0}) to stack
		pc_inline_mod,			//Push ({esp-1} % {esp-0}) to stack
		pc_inline_pow,			//Push ({esp-1} ^ {esp-0}) to stack
		pc_inline_app,			//Push ({esp-1} ~ to_array({esp-0})) to stack
		pc_inline_cat,			//Push ({esp-1} ~ {esp-0}) to stack

		pc_inline_cmp_e,		//Push ({esp-1} == {esp-0}) to stack
		pc_inline_cmp_g,		//Push ({esp-1} > {esp-0}) to stack
		pc_inline_cmp_ge,		//Push ({esp-1} >= {esp-0}) to stack
		pc_inline_cmp_l,		//Push ({esp-1} < {esp-0}) to stack
		pc_inline_cmp_le,		//Push ({esp-1} <= {esp-0}) to stack
		pc_inline_cmp_ne,		//Push ({esp-1} != {esp-0}) to stack

		pc_inline_logic_and,	//Push ({esp-1} && {esp-0}) to stack 
		pc_inline_logic_or,		//Push ({esp-1} || {esp-0}) to stack

		pc_inline_cast_var,			//Cast {esp-0} to (type_data*)[arg0], check type conversion if [arg1]
		pc_inline_index_array,		//Push &((*{esp-1})[{esp-0}]) to stack
		pc_inline_index_array2,		//Push ({esp-1}[{esp-0}]) to stack
		pc_inline_length_array,		//Push length({esp-0}) to stack

		pc_nop = 0xff,			//No operation
	};
	enum class block_kind : uint8_t {
		bk_normal, bk_sub, bk_function, bk_microthread
	};

	struct code;
	struct script_block {
		uint32_t level;
		uint32_t arguments;
		std::string name;
		dnh_func_callback_t func;
		std::vector<code> codes;
		block_kind kind;

		script_block(uint32_t the_level, block_kind the_kind);
	};
	struct code {
		command_kind command = command_kind::pc_nop;
		int line = -1;
#ifdef _DEBUG
		std::string var_name;	//For assign/push_variable
#endif

		union {
			struct {
				union {
#ifdef WIN32	//Just in case
					uint32_t arg0;
#else
					uint64_t arg0;
#endif
					script_block* block;
				};
				uint32_t arg1;
				uint32_t arg2;
			};
			struct {	//push_value
				value data;
			};
		};

		code();

		code(command_kind _command);
		code(command_kind _command, uint32_t _a0);
		code(command_kind _command, uint32_t _a0, uint32_t _a1);
		code(command_kind _command, uint32_t _a0, uint32_t _a1, uint32_t _a2);
		code(command_kind _command, uint32_t _a0, const std::string& _name);
		code(command_kind _command, uint32_t _a0, uint32_t _a1, const std::string& _name);
		code(command_kind _command, uint32_t _a0, uint32_t _a1, uint32_t _a2, const std::string& _name);
		code(command_kind _command, const value& _data);

		code(int _line, command_kind _command, uint32_t _a0) : code(_command, _a0) { line = _line; }
		code(int _line, command_kind _command, const value& _data) : code(_command, _data) { line = _line; }

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

			void AddCode(script_block* bk, const code& cd) {
				bk->codes.push_back(cd);
				bk->codes.back().line = lex->line;
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
		struct block_return_t {
			size_t size;
			size_t alloc_base;
			size_t alloc_size;
		};
	public:
		struct arg_data {
			std::string name;
			type_data* type;
			bool bConst;

			arg_data() : arg_data("", nullptr, false) {}
			arg_data(const std::string& name_) : arg_data(name_, nullptr, false) {}
			arg_data(const std::string& name_, bool bConst_) : arg_data(name_, nullptr, bConst_) {}
			arg_data(const std::string& name_, type_data* type_, bool bConst_)
				: name(name_), type(type_), bConst(bConst_) {}
		};
		struct symbol {
			uint32_t level;
			type_data* type = nullptr;
			bool bVariable = true;

			//Func/task/sub
			struct {
				bool bAllowOverload;
				script_block* sub;
				std::vector<arg_data> argData;
			};
			//Variable
			struct {
				uint32_t var;
				bool bConst;		//Applies to the scripter, not the engine
				bool bAssigned;
			};

			symbol();
			symbol(uint32_t lv, type_data* type_);
			symbol(uint32_t lv, type_data* type_, bool overl, script_block* pSub);
			symbol(uint32_t lv, type_data* type_, bool overl, script_block* pSub, const std::vector<arg_data>& argData_);
			symbol(uint32_t lv, type_data* type_, uint32_t var_, bool bConst_);
		};

		struct scope_t : public std::multimap<std::string, symbol> {
			block_kind kind;

			scope_t(block_kind the_kind) : kind(the_kind) {}

			symbol* singular_insert(const std::string& name, const symbol& s, int argc = 0);
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
		size_t _parse_array_suffix_lvalue(script_block* block, parser_state_t* state);
		void _parse_array_suffix_rvalue(script_block* block, parser_state_t* state);
		void parse_suffix(script_block* block, parser_state_t* state);
		void parse_product(script_block* block, parser_state_t* state);
		void parse_sum(script_block* block, parser_state_t* state);
		void parse_bitwise_shift(script_block* block, parser_state_t* state);
		void parse_comparison(script_block* block, parser_state_t* state);
		void parse_bitwise(script_block* block, parser_state_t* state);
		void parse_logic(script_block* block, parser_state_t* state);
		void parse_ternary(script_block* block, parser_state_t* state);
		void parse_expression(script_block* block, parser_state_t* state);

		int parse_arguments(script_block* block, parser_state_t* state, const std::vector<arg_data>* argsData);
		void parse_single_statement(script_block* block, parser_state_t* state, 
			bool check_terminator, token_kind statement_terminator);
		void parse_statements(script_block* block, parser_state_t* state,
			token_kind block_terminator, token_kind statement_terminator);

		block_return_t parse_block(script_block* block, parser_state_t* state,
			const std::vector<arg_data>* args, bool allow_single = false);
		size_t parse_block_inlined(script_block* block, parser_state_t* state, bool allow_single = true);
	private:
		void register_function(const function& func);

		symbol* search(const std::string& name, scope_t** ptrScope = nullptr);
		symbol* search(const std::string& name, int argc, scope_t** ptrScope = nullptr);
		symbol* search_in(scope_t* scope, const std::string& name);
		symbol* search_in(scope_t* scope, const std::string& name, int argc);
		symbol* search_result();

		arg_data parse_variable_decl(parser_state_t* state, bool bParameter);
		void parse_type_decl(parser_state_t* state, arg_data* pArg);
		int scan_current_scope(parser_state_t* state, int level, int initVar, const std::vector<arg_data>* args);

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

		static bool _parser_skip_post_decl(parser_state_t* state, int* pBrk, int* pPar);
		static void _parser_assert_end(parser_state_t* state);
		static void _parser_assert_nend(parser_state_t* state);

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
		switch (tk) {
		case token_kind::tk_decl_auto:
		case token_kind::tk_decl_void:
		case token_kind::tk_decl_float:
		case token_kind::tk_decl_int:
		case token_kind::tk_decl_char:
		case token_kind::tk_decl_bool:
		case token_kind::tk_decl_string:
		case token_kind::tk_decl_mod_const:
			return true;
		}
		return false;
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
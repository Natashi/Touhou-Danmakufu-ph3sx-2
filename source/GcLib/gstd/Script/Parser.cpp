#include "source/GcLib/pch.h"

#include "Parser.hpp"
#include "ScriptFunction.hpp"
#include "Script.hpp"

//Natashi's TODO: Implement a parse tree

namespace gstd {
	const function base_operations[] = {
		//{ "true", true_, 0 },
		//{ "false", false_, 0 },
		{ "length", BaseFunction::length, 1 },
		{ "not", BaseFunction::not_, 1 },
		{ "negative", BaseFunction::negative, 1 },
		{ "predecessor", BaseFunction::predecessor, 1 },
		{ "successor", BaseFunction::successor, 1 },
		{ "round", BaseFunction::round, 1 },
		{ "trunc", BaseFunction::truncate, 1 },
		{ "truncate", BaseFunction::truncate, 1 },
		{ "ceil", BaseFunction::ceil, 1 },
		{ "floor", BaseFunction::floor, 1 },
		//{ "abs", BaseFunction::absolute, 1 },
		{ "absolute", BaseFunction::absolute, 1 },
		{ "add", BaseFunction::add, 2 },
		{ "subtract", BaseFunction::subtract, 2 },
		{ "multiply", BaseFunction::multiply, 2 },
		{ "divide", BaseFunction::divide, 2 },
		{ "remainder", BaseFunction::remainder_, 2 },
		{ "modc", BaseFunction::modc, 2 },
		{ "power", BaseFunction::power, 2 },
		//{ "index_", BaseFunction::index<false>, 2 },
		//{ "index_w", BaseFunction::index<true>, 2 },
		{ "slice", BaseFunction::slice, 3 },
		{ "erase", BaseFunction::erase, 2 },
		{ "append", BaseFunction::append, 2 },
		{ "concatenate", BaseFunction::concatenate, 2 },
		{ "compare", BaseFunction::compare, 2 },

		{ "bit_not", BaseFunction::bitwiseNot, 1 },
		{ "bit_and", BaseFunction::bitwiseAnd, 2 },
		{ "bit_or", BaseFunction::bitwiseOr, 2 },
		{ "bit_xor", BaseFunction::bitwiseXor, 2 },
		{ "bit_left", BaseFunction::bitwiseLeft, 2 },
		{ "bit_right", BaseFunction::bitwiseRight, 2 },

		{ "assert", BaseFunction::assert_, 2 },
		{ "__DEBUG_BREAK", BaseFunction::script_debugBreak, 0 },
	};

	void parser::scope_t::singular_insert(const std::string& name, const symbol& s, int argc) {
		bool exists = this->find(name) != this->end();
		auto itr = this->equal_range(name);

		if (exists) {
			symbol* sPrev = &(itr.first->second);

			if (!sPrev->can_overload && sPrev->level > 0) {
				std::string error = StringUtility::Format("Default functions cannot be overloaded. (%s)",
					sPrev->sub->name.c_str());
				parser_assert(false, error);
			}
			else {
				for (auto itrPair = itr.first; itrPair != itr.second; ++itrPair) {
					if (argc == itrPair->second.sub->arguments) {
						itrPair->second = s;
						return;
					}
				}
			}
		}

		this->insert(std::make_pair(name, s));
	}

	parser::parser(script_engine* e, script_scanner* s, int funcc, const function* funcv) {
		engine = e;
		lexer_main = s;
		error = false;

		frame.push_back(scope_t(block_kind::bk_normal));		//Scope for default symbols

		for (size_t i = 0; i < sizeof(base_operations) / sizeof(function); ++i)
			register_function(base_operations[i]);

		for (size_t i = 0; i < funcc; ++i)
			register_function(funcv[i]);

		try {
			frame.push_back(scope_t(block_kind::bk_normal));	//Scope for user symbols

			parser_state_t stateParser(lexer_main);

			int countVar = scan_current_scope(&stateParser, 1, nullptr, false);
			if (countVar > 0)
				engine->main_block->codes.push_back(code(lexer_main->line, command_kind::pc_var_alloc, countVar));
			parse_statements(engine->main_block, &stateParser);

			parser_assert(lexer_main->next == token_kind::tk_end,
				"Unexpected end-of-file while parsing. (Did you forget a semicolon after a string?)\r\n");
		}
		catch (parser_error& e) {
			error = true;
			error_message = e.what();
			error_line = lexer_main->line;
		}
		catch (parser_error_mapped& e) {
			error = true;
			error_message = e.what();
			error_line = e.GetLine();
		}
	}

	void parser::register_function(const function& func) {
		symbol s;
		s.level = 0;
		s.sub = engine->new_block(0, block_kind::bk_function);
		s.sub->arguments = func.arguments;
		s.sub->name = func.name;
		s.sub->func = func.func;
		s.variable = -1;
		s.can_overload = false;
		s.can_modify = false;
		frame[0].singular_insert(func.name, s, func.arguments);
	}

	parser::symbol* parser::search(const std::string& name, scope_t** ptrScope) {
		for (auto itr = frame.rbegin(); itr != frame.rend(); ++itr) {
			scope_t* scope = &*itr;
			if (ptrScope) *ptrScope = scope;

			auto itrSymbol = scope->find(name);
			if (itrSymbol != scope->end()) {
				return &(itrSymbol->second);
			}
		}
		return nullptr;
	}
	parser::symbol* parser::search(const std::string& name, int argc, scope_t** ptrScope) {
		for (auto itr = frame.rbegin(); itr != frame.rend(); ++itr) {
			scope_t* scope = &*itr;
			if (ptrScope) *ptrScope = scope;

			if (scope->find(name) == scope->end()) continue;

			auto itrSymbol = scope->equal_range(name);
			if (itrSymbol.first != scope->end()) {
				for (auto itrPair = itrSymbol.first; itrPair != itrSymbol.second; ++itrPair) {
					if (itrPair->second.sub) {
						if (argc == itrPair->second.sub->arguments)
							return &(itrPair->second);
					}
					else {
						return &(itrPair->second);
					}
				}
				return nullptr;
			}
		}
		return nullptr;
	}
	parser::symbol* parser::search_in(scope_t* scope, const std::string& name) {
		auto itrSymbol = scope->find(name);
		if (itrSymbol != scope->end())
			return &(itrSymbol->second);
		return nullptr;
	}
	parser::symbol* parser::search_in(scope_t* scope, const std::string& name, int argc) {
		if (scope->find(name) == scope->end()) return nullptr;

		auto itrSymbol = scope->equal_range(name);
		if (itrSymbol.first != scope->end()) {
			for (auto itrPair = itrSymbol.first; itrPair != itrSymbol.second; ++itrPair) {
				if (itrPair->second.sub) {
					if (argc == itrPair->second.sub->arguments)
						return &(itrPair->second);
				}
				else {
					return &(itrPair->second);
				}
			}
		}

		return nullptr;
	}
	parser::symbol* parser::search_result() {
		for (auto itr = frame.rbegin(); itr != frame.rend(); ++itr) {
			if (itr->kind == block_kind::bk_sub || itr->kind == block_kind::bk_microthread)
				return nullptr;

			auto itrSymbol = itr->find("\x01");
			if (itrSymbol != itr->end())
				return &(itrSymbol->second);
		}
		return nullptr;
	}

	int parser::scan_current_scope(parser_state_t* state, int level, const std::vector<std::string>* args,
		bool adding_result, int initVar) 
	{
		//Scans and defines scope variables and functions/tasks/subs

		script_scanner lex2(*state->lex);

		int var = initVar;

		try {
			scope_t* current_frame = &frame.back();
			int cur = 0;

			if (adding_result) {
				symbol s;
				s.level = level;
				s.sub = nullptr;
				s.variable = var;
				s.can_overload = false;
				s.can_modify = false;
				++var;
				current_frame->singular_insert("\x01", s);
			}

			if (args) {
				for (size_t i = 0; i < args->size(); ++i) {
					symbol s;
					s.level = level;
					s.sub = nullptr;
					s.variable = var;
					s.can_overload = false;
					s.can_modify = true;
					++var;
					current_frame->singular_insert(args->at(i), s);
				}
			}

			while (cur >= 0 && lex2.next != token_kind::tk_end && lex2.next != token_kind::tk_invalid) {
				bool is_const = false;

				switch (lex2.next) {
				case token_kind::tk_open_cur:
					++cur;
					lex2.advance();
					break;
				case token_kind::tk_close_cur:
					--cur;
					lex2.advance();
					break;
				case token_kind::tk_at:
				case token_kind::tk_SUB:
				case token_kind::tk_FUNCTION:
				case token_kind::tk_TASK:
				{
					token_kind type = lex2.next;
					lex2.advance();
					if (cur == 0) {
						block_kind kind = (type == token_kind::tk_SUB || type == token_kind::tk_at) ?
							block_kind::bk_sub : (type == token_kind::tk_FUNCTION) ?
							block_kind::bk_function : block_kind::bk_microthread;

						std::string name = lex2.word;
						int countArgs = 0;

						lex2.advance();
						if (lex2.next == token_kind::tk_open_par) {
							lex2.advance();

							//I honestly should just throw out support for subs
							if (lex2.next != token_kind::tk_close_par && kind == block_kind::bk_sub) {
								parser_assert(state, false, "A parameter list in not allowed here.\r\n");
							}
							else {
								while (lex2.next == token_kind::tk_word || IsDeclToken(lex2.next)) {
									++countArgs;

									if (IsDeclToken(lex2.next)) lex2.advance();

									if (lex2.next == token_kind::tk_word) lex2.advance();
									//TODO: Make it available to the scripters
									/*
									else if (lex2.next == token_kind::tk_args_variadic) {
										countArgs = -countArgs;
										lex2.advance();
										break;
									}
									*/
									if (lex2.next != token_kind::tk_comma)
										break;

									lex2.advance();
								}
							}
						}

						{
							symbol* dup = search_in(current_frame, name, countArgs);

							//If the level is 1(first user level), also search for duplications in level 0(default level)
							if (dup == nullptr && level == 1)
								dup = search_in(&frame.front(), name, countArgs);

							if (dup) {
								//Woohoo for detailed error messages.
								std::string typeSub;
								switch (dup->sub->kind) {
								case block_kind::bk_function:
									typeSub = "function";
									break;
								case block_kind::bk_microthread:
									typeSub = "task";
									break;
								case block_kind::bk_sub:
									typeSub = "sub or an \'@\' block";
									break;
								default:
									typeSub = "block";
									break;
								}

								std::string error = "A ";
								error += typeSub;
								error += StringUtility::Format(" of the same name was already defined "
									"in the current scope: \'%s\'\r\n", name.c_str());
								if (dup->can_overload && countArgs >= 0)
									error += "**You may overload functions and tasks by giving them "
									"different argument counts.\r\n";
								else
									error += StringUtility::Format("**\'%s\' cannot be overloaded.\r\n", name.c_str());
								throw parser_error(error);
							}
						}

						symbol s;
						s.level = level;
						s.sub = engine->new_block(level + 1, kind);
						s.sub->name = name;
						s.sub->func = nullptr;
						s.sub->arguments = countArgs;
						s.variable = -1;
						s.can_overload = kind != block_kind::bk_sub;
						s.can_modify = false;
						current_frame->singular_insert(name, s, countArgs);
					}
				}
				break;
				case token_kind::tk_const:
					is_const = true;
				case token_kind::tk_decl_real:
				//case token_kind::tk_decl_char:
				//case token_kind::tk_decl_bool:
				//case token_kind::tk_decl_string:
				case token_kind::tk_decl_auto:
				{
					lex2.advance();
					if (cur == 0) {
						{
							symbol* dup = search_in(current_frame, lex2.word);

							//If the level is 1(first user level), also search for duplications in level 0(default level)
							if (dup == nullptr && level == 1)
								dup = search_in(&frame.front(), lex2.word);

							if (dup) {
								std::string error = StringUtility::Format("A variable of the same name "
									"was already declared in the current scope.\r\n", lex2.word.c_str());
								parser_assert(state, true, error);
							}
						}

						symbol s;
						s.level = level;
						s.sub = nullptr;
						s.variable = var;
						s.can_overload = false;
						s.can_modify = !is_const;
						++var;
						current_frame->singular_insert(lex2.word, s);

						lex2.advance();
					}
					break;
				}
				default:
					lex2.advance();
					break;
				}
			}
		}
		catch (...) {
			state->lex->line = lex2.line;
			throw;
		}

		return (var - initVar);
	}

	void parser::write_operation(script_block* block, parser_state_t* state, const char* name, int clauses) {
		symbol* s = search(name);
		assert(s != nullptr);
		{
			std::string error = "Invalid argument count for the default function. (expected " +
				std::to_string(clauses) + ")\r\n";
			parser_assert(state, s->sub->arguments == clauses, error);
		}
		state->AddCode(block, code(command_kind::pc_call_and_push_result, s->sub, clauses));
	}
	void parser::write_operation(script_block* block, parser_state_t* state, const symbol* s, int clauses) {
		assert(s != nullptr);
		{
			std::string error = "Invalid argument count for the default function. (expected " +
				std::to_string(clauses) + ")\r\n";
			parser_assert(state, s->sub->arguments == clauses, error);
		}
		state->AddCode(block, code(command_kind::pc_call_and_push_result, s->sub, clauses));
	}

	void parser::parse_parentheses(script_block* block, parser_state_t* state) {
		parser_assert(state, state->next() == token_kind::tk_open_par, "\"(\" is required.\r\n");
		state->advance();

		parse_expression(block, state);

		parser_assert(state, state->next() == token_kind::tk_close_par, "\")\" is required.\r\n");
		state->advance();
	}

	void parser::parse_clause(script_block* block, parser_state_t* state) {
		switch (state->next()) {
		case token_kind::tk_real:
			state->AddCode(block, code(command_kind::pc_push_value, 
				value(script_type_manager::get_real_type(), state->lex->real_value)));
			state->advance();
			return;
		case token_kind::tk_char:
			state->AddCode(block, code(command_kind::pc_push_value, 
				value(script_type_manager::get_char_type(), state->lex->char_value)));
			state->advance();
			return;
		case token_kind::tk_TRUE:
			state->AddCode(block, code(command_kind::pc_push_value, 
				value(script_type_manager::get_boolean_type(), true)));
			state->advance();
			return;
		case token_kind::tk_FALSE:
			state->AddCode(block, code(command_kind::pc_push_value, 
				value(script_type_manager::get_boolean_type(), false)));
			state->advance();
			return;
		case token_kind::tk_string:
		{
			std::wstring str = state->lex->string_value;
			state->advance();
			while (state->next() == token_kind::tk_string || state->next() == token_kind::tk_char) {
				str += state->next() == token_kind::tk_string ? state->lex->string_value : 
					std::wstring(1, state->lex->char_value);
				state->advance();
			}

			state->AddCode(block, code(command_kind::pc_push_value, value(script_type_manager::get_string_type(), str)));
			return;
		}
		case token_kind::tk_word:
		{
			std::string name = state->lex->word;
			state->advance();
			int argc = parse_arguments(block, state);

			symbol* s = search(name, argc);

			if (s == nullptr) {
				if (s = search(name)) {
					if (test_variadic(s->sub->arguments, argc))
						goto continue_as_variadic;
				}

				std::string error;
				if (search(name))
					error = StringUtility::Format("No matching overload for %s with %d arguments was found.\r\n",
						name.c_str(), argc);
				else
					error = StringUtility::Format("%s is not defined.\r\n", name.c_str());
				throw parser_error(error);
			}

continue_as_variadic:
			if (s->sub) {
				parser_assert(state, s->sub->kind == block_kind::bk_function, "Tasks and subs cannot return values.\r\n");
				state->AddCode(block, code(command_kind::pc_call_and_push_result, s->sub, argc));
			}
			else {
				//Variable
				state->AddCode(block, code(command_kind::pc_push_variable, s->level, s->variable, name));
			}

			return;
		}
		case token_kind::tk_cast_int:
		case token_kind::tk_cast_real:
		case token_kind::tk_cast_char:
		case token_kind::tk_cast_bool:
		{
			type_data::type_kind target = type_data::type_kind::tk_null;
			switch (state->next()) {
			case token_kind::tk_cast_int:
				target = type_data::type_kind::tk_int;
				break;
			case token_kind::tk_cast_real:
				target = type_data::type_kind::tk_real;
				break;
			case token_kind::tk_cast_char:
				target = type_data::type_kind::tk_char;
				break;
			case token_kind::tk_cast_bool:
				target = type_data::type_kind::tk_boolean;
				break;
			}
			state->advance();
			parse_parentheses(block, state);
			state->AddCode(block, code(command_kind::pc_inline_cast_var, (size_t)target));
			return;
		}
		case token_kind::tk_open_bra:
		{
			state->advance();

			size_t count_member = 0U;
			while (state->next() != token_kind::tk_close_bra) {
				parse_expression(block, state);
				++count_member;

				if (state->next() != token_kind::tk_comma) break;
				state->advance();
			}

			if (count_member > 0U)
				state->AddCode(block, code(command_kind::pc_construct_array, count_member));
			else
				state->AddCode(block, code(command_kind::pc_push_value,
					value(script_type_manager::get_string_type(), 0i64)));

			parser_assert(state, state->next() == token_kind::tk_close_bra, "\"]\" is required.\r\n");
			state->advance();

			return;
		}
		case token_kind::tk_open_abs:
			state->advance();
			parse_expression(block, state);
			state->AddCode(block, code(command_kind::pc_inline_abs));
			parser_assert(state, state->next() == token_kind::tk_close_abs, "\"|)\" is required.\r\n");
			state->advance();
			return;
		case token_kind::tk_open_par:
			parse_parentheses(block, state);
			return;
		default:
			parser_assert(state, false, "Invalid expression.\r\n");
		}
	}

	void parser::parse_suffix(script_block* block, parser_state_t* state) {
		parse_clause(block, state);
		if (state->next() == token_kind::tk_caret) {
			state->advance();
			parse_suffix(block, state);
			state->AddCode(block, code(command_kind::pc_inline_pow));
		}
		else {
			while (state->next() == token_kind::tk_open_bra) {
				state->advance();
				parse_expression(block, state);

				if (state->next() == token_kind::tk_range) {
					state->advance();
					parse_expression(block, state);
					write_operation(block, state, "slice", 3);
				}
				else {
					state->AddCode(block, code(command_kind::pc_inline_index_array, (size_t)false));
				}

				parser_assert(state, state->next() == token_kind::tk_close_bra, "\"]\" is required.\r\n");
				state->advance();
			}
		}
	}

	void parser::parse_prefix(script_block* block, parser_state_t* state) {
		switch (state->next()) {
		case token_kind::tk_plus:
			state->advance();
			parse_prefix(block, state);
			return;
		case token_kind::tk_minus:
			state->advance();
			parse_prefix(block, state);
			state->AddCode(block, code(command_kind::pc_inline_neg));
			return;
		case token_kind::tk_exclamation:	//Logical NOT
			state->advance();
			parse_prefix(block, state);
			state->AddCode(block, code(command_kind::pc_inline_not));
			return;
		case token_kind::tk_tilde:			//Bitwise NOT
			state->advance();
			parse_prefix(block, state);
			write_operation(block, state, "bit_not", 1);
			return;
		default:
			parse_suffix(block, state);
		}
	}

	void parser::parse_product(script_block* block, parser_state_t* state) {
		parse_prefix(block, state);
		while (state->next() == token_kind::tk_asterisk || state->next() == token_kind::tk_slash
			|| state->next() == token_kind::tk_percent) {
			command_kind f = (state->next() == token_kind::tk_asterisk) ? command_kind::pc_inline_mul :
				(state->next() == token_kind::tk_slash) ? command_kind::pc_inline_div : command_kind::pc_inline_mod;
			state->advance();
			parse_prefix(block, state);
			state->AddCode(block, code(f));
		}
	}

	void parser::parse_sum(script_block* block, parser_state_t* state) {
		parse_product(block, state);
		while (state->next() == token_kind::tk_tilde || state->next() == token_kind::tk_plus
			|| state->next() == token_kind::tk_minus) {
			command_kind f = (state->next() == token_kind::tk_tilde) ? command_kind::pc_inline_cat :
				(state->next() == token_kind::tk_plus) ? command_kind::pc_inline_add : command_kind::pc_inline_sub;
			state->advance();
			parse_product(block, state);
			state->AddCode(block, code(f));
		}
	}

	void parser::parse_bitwise_shift(script_block* block, parser_state_t* state) {
		parse_sum(block, state);
		while (state->next() == token_kind::tk_bit_shf_left || state->next() == token_kind::tk_bit_shf_right) {
			const char* f = state->next() == token_kind::tk_bit_shf_left ? "bit_left" : "bit_right";
			state->advance();
			parse_sum(block, state);
			write_operation(block, state, f, 2);
		}
	}

	void parser::parse_comparison(script_block* block, parser_state_t* state) {
		parse_bitwise_shift(block, state);
		switch (state->next()) {
		case token_kind::tk_assign:
			parser_assert(state, false, "Did you intend to write \"==\"?\r\n");
			break;
		case token_kind::tk_e:
		case token_kind::tk_g:
		case token_kind::tk_ge:
		case token_kind::tk_l:
		case token_kind::tk_le:
		case token_kind::tk_ne:
			token_kind op = state->next();
			state->advance();
			parse_bitwise_shift(block, state);

			switch (op) {
			case token_kind::tk_e:
				state->AddCode(block, code(command_kind::pc_inline_cmp_e));
				break;
			case token_kind::tk_g:
				state->AddCode(block, code(command_kind::pc_inline_cmp_g));
				break;
			case token_kind::tk_ge:
				state->AddCode(block, code(command_kind::pc_inline_cmp_ge));
				break;
			case token_kind::tk_l:
				state->AddCode(block, code(command_kind::pc_inline_cmp_l));
				break;
			case token_kind::tk_le:
				state->AddCode(block, code(command_kind::pc_inline_cmp_le));
				break;
			case token_kind::tk_ne:
				state->AddCode(block, code(command_kind::pc_inline_cmp_ne));
				break;
			}

			break;
		}
	}

	void parser::parse_bitwise(script_block* block, parser_state_t* state) {
		auto ParseAND = [&]() {
			parse_comparison(block, state);
			while (state->next() == token_kind::tk_bit_and) {
				state->advance();
				parse_comparison(block, state);
				write_operation(block, state, "bit_and", 2);
			}
		};
		auto ParseOR = [&]() {
			ParseAND();
			while (state->next() == token_kind::tk_bit_or) {
				state->advance();
				ParseAND();
				write_operation(block, state, "bit_or", 2);
			}
		};

		ParseOR();
		while (state->next() == token_kind::tk_bit_xor) {
			state->advance();
			ParseOR();
			write_operation(block, state, "bit_xor", 2);
		}
	}

	//TODO: Change logic operator precedence to match C's
	void parser::parse_logic(script_block* block, parser_state_t* state) {
		parse_bitwise(block, state);

		size_t iter = 0x80000000;
		while (state->next() == token_kind::tk_logic_and || state->next() == token_kind::tk_logic_or) {
			command_kind cmdLogic = (state->next() == token_kind::tk_logic_and) ?
				command_kind::pc_inline_logic_and : command_kind::pc_inline_logic_or;
			command_kind cmdJump = (state->next() == token_kind::tk_logic_and) ?
				command_kind::_pc_jump_if_not : command_kind::_pc_jump_if;
			state->advance();

			state->AddCode(block, code(command_kind::pc_dup_n, 1));
			state->AddCode(block, code(cmdJump, iter));

			parse_bitwise(block, state);

			//state->AddCode(block, code(cmdLogic));
			state->AddCode(block, code(command_kind::pc_jump_target, iter));

			++iter;
		}
	}

	void parser::parse_ternary(script_block* block, parser_state_t* state) {
		parse_logic(block, state);
		if (state->next() == token_kind::tk_query) {
			state->advance();

			state->AddCode(block, code(command_kind::_pc_jump_if_not, 0xb4ab5cc9));	//Jump to expression 2

			//Parse expression 1
			parse_expression(block, state);
			state->AddCode(block, code(command_kind::_pc_jump, 0xf88c0c27));		//Jump to exit

			parser_assert(state, state->next() == token_kind::tk_colon, "Incomplete ternary statement; a colon(:) is required.");
			state->advance();

			//Parse expression 2
			state->AddCode(block, code(command_kind::pc_jump_target, 0xb4ab5cc9));
			parse_expression(block, state);

			//Exit point
			state->AddCode(block, code(command_kind::pc_jump_target, 0xf88c0c27));
		}
	}

	void parser::parse_expression(script_block* block, parser_state_t* state) {
		script_block tmp(0, block_kind::bk_normal);
		size_t ip = state->ip;

		parse_ternary(&tmp, state);

		try {
			optimize_expression(&tmp, state);
			link_jump(&tmp, state, ip);

			block->codes.insert(block->codes.end(), tmp.codes.begin(), tmp.codes.end());
		}
		catch (std::wstring& err) {
			parser_assert(state, false, err);
		}
	}

	//Format for variadic arguments (To-be-completed until after 1.10a):
	// argc = -(n + 1)
	// where n = fixed(required) arguments
	int parser::parse_arguments(script_block* block, parser_state_t* state) {
		int result = 0;
		if (state->next() == token_kind::tk_open_par) {
			state->advance();

			while (state->next() != token_kind::tk_close_par) {
				++result;
				parse_expression(block, state);
				if (state->next() != token_kind::tk_comma) break;
				state->advance();
			}

			parser_assert(state, state->next() == token_kind::tk_close_par, "\")\" is required.\r\n");
			state->advance();
		}
		return result;
	}

	bool parser::parse_single_statement(script_block* block, parser_state_t* state) {
		auto assert_const = [](symbol* s, const std::string& name) {
			if (!s->can_modify) {
				std::string error = StringUtility::Format("\"%s\": ", name.c_str());
				if (s->sub)
					throw parser_error(error + "Functions, tasks, and subs are implicitly const\r\n"
						"and thus cannot be modified in this manner.\r\n");
				else
					throw parser_error(error + "const variables cannot be modified.\r\n");
			}
		};
		auto get_op_assign_command = [](token_kind tk) {
			switch (tk) {
			case token_kind::tk_add_assign:
				return command_kind::pc_inline_add_asi;
			case token_kind::tk_subtract_assign:
				return command_kind::pc_inline_sub_asi;
			case token_kind::tk_multiply_assign:
				return command_kind::pc_inline_mul_asi;
			case token_kind::tk_divide_assign:
				return command_kind::pc_inline_div_asi;
			case token_kind::tk_remainder_assign:
				return command_kind::pc_inline_mod_asi;
			case token_kind::tk_power_assign:
				return command_kind::pc_inline_pow_asi;
			}
			return command_kind::pc_inline_add_asi;
		};

		bool need_terminator = true;

		switch (state->next()) {
		case token_kind::tk_word:
		{
			std::string name = state->lex->word;

			scope_t* resScope = nullptr;
			symbol* s = search(name, &resScope);
			parser_assert(state, s, StringUtility::Format("%s is not defined.\r\n", name.c_str()));

			state->advance();
			switch (state->next()) {
			case token_kind::tk_assign:
				assert_const(s, name);

				state->advance();
				parse_expression(block, state);
				state->AddCode(block, code(command_kind::pc_assign, s->level, s->variable, name));
				break;
			case token_kind::tk_open_bra:
				assert_const(s, name);

				state->AddCode(block, code(command_kind::pc_push_variable, s->level, s->variable, name));
				while (state->next() == token_kind::tk_open_bra) {
					state->advance();
					parse_expression(block, state);

					parser_assert(state, state->next() == token_kind::tk_close_bra, "\"]\" is required.\r\n");
					state->advance();

					state->AddCode(block, code(command_kind::pc_inline_index_array, (size_t)true));
				}

				switch (state->next()) {
				case token_kind::tk_assign:
					state->advance();
					parse_expression(block, state);
					state->AddCode(block, code(command_kind::pc_assign_writable, 0, 0, name));
					break;
				case token_kind::tk_inc:
				case token_kind::tk_dec:
				{
					command_kind f = (state->next() == token_kind::tk_inc) ? command_kind::pc_inline_inc :
						command_kind::pc_inline_dec;
					state->advance();
					state->AddCode(block, code(f, -1, 0, name));
					break;
				}
				case token_kind::tk_add_assign:
				case token_kind::tk_subtract_assign:
				case token_kind::tk_multiply_assign:
				case token_kind::tk_divide_assign:
				case token_kind::tk_remainder_assign:
				case token_kind::tk_power_assign:
				{
					command_kind f = get_op_assign_command(state->next());
					state->advance();
					parse_expression(block, state);
					state->AddCode(block, code(f, -1, 0, name));
					break;
				}
				default:
					parser_assert(state, false, "\"=\" is required.\r\n");
					break;
				}

				break;
			case token_kind::tk_add_assign:
			case token_kind::tk_subtract_assign:
			case token_kind::tk_multiply_assign:
			case token_kind::tk_divide_assign:
			case token_kind::tk_remainder_assign:
			case token_kind::tk_power_assign:
			{
				assert_const(s, name);

				command_kind f = get_op_assign_command(state->next());
				state->advance();
				parse_expression(block, state);
				state->AddCode(block, code(f, s->level, s->variable, name));

				break;
			}
			case token_kind::tk_inc:
			case token_kind::tk_dec:
			{
				assert_const(s, name);

				command_kind f = (state->next() == token_kind::tk_inc) ? command_kind::pc_inline_inc : command_kind::pc_inline_dec;
				state->advance();
				state->AddCode(block, code(f, s->level, s->variable, name));

				break;
			}
			default:
			{
				parser_assert(state, s->sub, "A variable cannot be called as if it was a function, a task, or a sub.\r\n");

				int argc = parse_arguments(block, state);

				if (!test_variadic(s->sub->arguments, argc)) {
					s = search_in(resScope, name, argc);
					parser_assert(state, s, StringUtility::Format("No matching overload for %s with %d arguments was found.\r\n",
						name.c_str(), argc));
				}

				state->AddCode(block, code(command_kind::pc_call, s->sub, argc));

				break;
			}
			}

			break;
		}
		case token_kind::tk_const:
		case token_kind::tk_decl_real:
		//case token_kind::tk_decl_char:
		//case token_kind::tk_decl_bool:
		//case token_kind::tk_decl_string:
		case token_kind::tk_decl_auto:
		{
			state->advance();
			parser_assert(state, state->next() == token_kind::tk_word, "Variable name is required.\r\n");

			std::string name = state->lex->word;

			state->advance();
			if (state->next() == token_kind::tk_assign) {
				symbol* s = search(name);

				state->advance();
				parse_expression(block, state);
				state->AddCode(block, code(command_kind::pc_assign, s->level, s->variable, name));
			}

			break;
		}
		//"local" can be omitted to make C-style blocks
		case token_kind::tk_LOCAL:
		case token_kind::tk_open_cur:
			if (state->next() == token_kind::tk_LOCAL) state->advance();
			parse_inline_block(nullptr, block, state, block_kind::bk_normal, false);
			need_terminator = false;
			break;
		case token_kind::tk_LOOP:
		{
			state->advance();
			size_t ip_begin = state->ip;

			if (state->next() == token_kind::tk_open_par) {
				parse_parentheses(block, state);
				size_t ip = state->ip;
				{
					script_block tmp(block->level, block_kind::bk_normal);
					parser_state_t tmpState(state->lex, state->ip);

					tmpState.AddCode(&tmp, code(command_kind::pc_inline_cast_var, (size_t)type_data::type_kind::tk_int));

					size_t ip_loop_begin = tmpState.ip;
					tmpState.AddCode(&tmp, code(command_kind::pc_loop_count));

					script_block* childBlock = nullptr;
					size_t childSize = parse_inline_block(&childBlock, &tmp, &tmpState, block_kind::bk_loop, true);

					tmpState.AddCode(&tmp, code(command_kind::pc_continue_marker));
					tmpState.AddCode(&tmp, code(command_kind::pc_jump, ip_loop_begin));
					tmpState.AddCode(&tmp, code(command_kind::pc_loop_back));
					tmpState.AddCode(&tmp, code(command_kind::pc_pop, 1));

					//Try to optimize looped yield to a wait
					if (childSize == 1U && childBlock->codes[0].command == command_kind::pc_yield) {
						engine->blocks.pop_back();
						state->AddCode(block, code(command_kind::pc_wait));
					}
					//Discard everything if the loop is empty
					else if (childSize > 0U) {
						block->codes.insert(block->codes.end(), tmp.codes.begin(), tmp.codes.end());
						state->ip = tmpState.ip;
					}
				}
			}
			else {
				size_t blockSize = parse_inline_block(nullptr, block, state, block_kind::bk_loop, true);
				state->AddCode(block, code(command_kind::pc_continue_marker));
				state->AddCode(block, code(command_kind::pc_jump, ip_begin));
				state->AddCode(block, code(command_kind::pc_loop_back));

				if (blockSize == 0U) {
					while (state->ip > ip_begin)
						state->PopCode(block);
				}
			}

			need_terminator = false;
			break;
		}
		case token_kind::tk_TIMES:
		{
			state->advance();
			size_t ip_begin = state->ip;

			parse_parentheses(block, state);
			size_t ip = state->ip;
			if (state->next() == token_kind::tk_LOOP)
				state->advance();
			{
				script_block tmp(block->level, block_kind::bk_normal);
				parser_state_t tmpState(state->lex, state->ip);

				tmpState.AddCode(&tmp, code(command_kind::pc_inline_cast_var, (size_t)type_data::type_kind::tk_int));

				size_t ip_loop_begin = tmpState.ip;
				tmpState.AddCode(&tmp, code(command_kind::pc_loop_count));

				script_block* childBlock = nullptr;
				size_t childSize = parse_inline_block(&childBlock, &tmp, &tmpState, block_kind::bk_loop, true);

				tmpState.AddCode(&tmp, code(command_kind::pc_continue_marker));
				tmpState.AddCode(&tmp, code(command_kind::pc_jump, ip_loop_begin));
				tmpState.AddCode(&tmp, code(command_kind::pc_loop_back));
				tmpState.AddCode(&tmp, code(command_kind::pc_pop, 1));

				//Try to optimize looped yield to a wait
				if (childSize == 1U && childBlock->codes[0].command == command_kind::pc_yield) {
					engine->blocks.pop_back();
					state->AddCode(block, code(command_kind::pc_wait));
				}
				//Discard everything if the loop is empty
				else if (childSize > 0U) {
					block->codes.insert(block->codes.end(), tmp.codes.begin(), tmp.codes.end());
					state->ip = tmpState.ip;
				}
			}

			need_terminator = false;
			break;
		}
		case token_kind::tk_WHILE:
		{
			state->advance();
			size_t ip = state->ip;

			parse_parentheses(block, state);
			if (state->next() == token_kind::tk_LOOP)
				state->advance();

			state->AddCode(block, code(command_kind::pc_loop_if));

			size_t newBlockSize = parse_inline_block(nullptr, block, state, block_kind::bk_loop, true);

			state->AddCode(block, code(command_kind::pc_continue_marker));
			state->AddCode(block, code(command_kind::pc_jump, ip));
			state->AddCode(block, code(command_kind::pc_loop_back));

			if (newBlockSize == 0U) {
				while (state->ip > ip)
					state->PopCode(block);
			}

			need_terminator = false;
			break;
		}
		case token_kind::tk_FOR:
		{
			state->advance();

			size_t ip_for_begin = state->ip;

			if (state->next() == token_kind::tk_EACH) {				//Foreach loop
				state->advance();
				parser_assert(state, state->next() == token_kind::tk_open_par, "\"(\" is required.\r\n");

				state->advance();
				if (state->next() == token_kind::tk_decl_auto) state->advance();
				else if (state->next() == token_kind::tk_const)
					parser_assert(state, false, "The counter variable cannot be const.\r\n");

				parser_assert(state, state->next() == token_kind::tk_word, "Variable name is required.\r\n");

				std::string feIdentifier = state->lex->word;

				state->advance();
				parser_assert(state, state->next() == token_kind::tk_IN || state->next() == token_kind::tk_colon,
					"\"in\" or a colon is required.\r\n");
				state->advance();

				//The array
				parse_expression(block, state);

				parser_assert(state, state->next() == token_kind::tk_close_par, "\")\" is required.\r\n");
				state->advance();

				if (state->next() == token_kind::tk_LOOP) state->advance();

				//The counter
				state->AddCode(block, code(command_kind::pc_push_value,
					value(script_type_manager::get_int_type(), 0i64)));

				size_t ip = state->ip;

				state->AddCode(block, code(command_kind::pc_for_each_and_push_first));

				script_block* b = engine->new_block(block->level + 1, block_kind::bk_loop);
				{
					parser_state_t newState(state->lex);

					std::vector<std::string> counter;
					counter.push_back(feIdentifier);

					parse_block(b, &newState, &counter, false, true);
				}

				//state->AddCode(block, code(command_kind::pc_dup_n, 1));
				state->AddCode(block, code(command_kind::pc_call, b, 1));

				state->AddCode(block, code(command_kind::pc_continue_marker));
				state->AddCode(block, code(command_kind::pc_jump, ip));
				state->AddCode(block, code(command_kind::pc_loop_back));

				//Pop twice for the array and the counter
				state->AddCode(block, code(command_kind::pc_pop, 2));

				if (b->codes.size() == 1U) {	//1 for pc_assign
					engine->blocks.pop_back();
					while (state->ip > ip_for_begin)
						state->PopCode(block);
				}
			}
			else if (state->next() == token_kind::tk_open_par) {	//Regular for loop
				state->advance();

				bool isNewVar = false;
				bool isNewVarConst = false;
				std::string newVarName = "";

				script_scanner lex_s1(*state->lex);		//First statement (initialization)

				if (IsDeclToken(state->next()) || state->next() == token_kind::tk_const) {
					isNewVar = true;
					isNewVarConst = state->next() == token_kind::tk_const;
					state->advance();
					parser_assert(state, state->next() == token_kind::tk_word, "Variable name is required.\r\n");
					newVarName = state->lex->word;
				}

				while (state->next() != token_kind::tk_semicolon) state->advance();
				state->advance();

				bool hasExpr = true;
				if (state->next() == token_kind::tk_semicolon) hasExpr = false;

				script_scanner lex_s2(*state->lex);		//Second statement (evaluation)

				while (state->next() != token_kind::tk_semicolon) state->advance();
				state->advance();

				script_scanner lex_s3(*state->lex);		//Third statement (update)

				while (state->next() != token_kind::tk_semicolon && state->next() != token_kind::tk_close_par) state->advance();
				state->advance();
				if (state->next() == token_kind::tk_close_par) state->advance();

				script_scanner lex_s4(*state->lex);		//Loop body

				script_block* forBlock = engine->new_block(block->level + 1, block_kind::bk_normal);
				state->AddCode(block, code(command_kind::pc_call, forBlock, 0));

				parser_state_t forBlockState(state->lex);

				size_t codeBlockSize = 0;

				//For block
				frame.push_back(scope_t(forBlock->kind));
				{
					if (isNewVar) {
						symbol s;
						s.level = forBlock->level;
						s.sub = nullptr;
						s.variable = 0;
						s.can_overload = false;
						s.can_modify = !isNewVarConst;
						frame.back().singular_insert(newVarName, s);
					}

					//Initialization statement
					forBlockState.lex = &lex_s1;
					forBlockState.AddCode(forBlock, code(command_kind::pc_var_alloc, 1));
					parse_single_statement(forBlock, &forBlockState);

					size_t ip_begin = forBlock->codes.size();

					//Evaluation statement
					if (hasExpr) {
						forBlockState.lex = &lex_s2;
						parse_expression(forBlock, &forBlockState);
						forBlockState.AddCode(forBlock, code(command_kind::pc_loop_if));
					}

					//Parse loop body
					forBlockState.lex = &lex_s4;
					codeBlockSize = parse_inline_block(nullptr, forBlock, &forBlockState, block_kind::bk_loop, true);
					state->lex->copy_state(forBlockState.lex);

					forBlockState.AddCode(forBlock, code(command_kind::pc_continue_marker));
					{
						//Update statement
						forBlockState.lex = &lex_s3;
						parse_single_statement(forBlock, &forBlockState);
						while (lex_s3.next == token_kind::tk_comma) {
							lex_s3.advance();
							parse_single_statement(forBlock, &forBlockState);
						}
					}
					forBlockState.AddCode(forBlock, code(command_kind::pc_jump, ip_begin));
					forBlockState.AddCode(forBlock, code(command_kind::pc_loop_back));
				}
				frame.pop_back();

				if (codeBlockSize == 0U) {
					engine->blocks.pop_back();	//forBlock
					while (state->ip > ip_for_begin)
						state->PopCode(block);
				}
			}
			else {
				parser_assert(state, false, "\"(\" is required.\r\n");
			}

			need_terminator = false;
			break;
		}
		case token_kind::tk_ASCENT:
		case token_kind::tk_DESCENT:
		{
			bool isAscent = state->next() == token_kind::tk_ASCENT;
			state->advance();

			parser_assert(state, state->next() == token_kind::tk_open_par, "\"(\" is required.\r\n");
			state->advance();

			if (IsDeclToken(state->next())) state->advance();
			else if (state->next() == token_kind::tk_const)
				parser_assert(false, "The counter variable cannot be const.\r\n");

			parser_assert(state, state->next() == token_kind::tk_word, "Variable name is required.\r\n");

			std::string counterName = state->lex->word;

			state->advance();

			parser_assert(state, state->next() == token_kind::tk_IN, "\"in\" is required.\r\n");
			state->advance();

			size_t ip_ascdsc_begin = state->ip;

			script_block* containerBlock = engine->new_block(block->level + 1, block_kind::bk_normal);
			state->AddCode(block, code(command_kind::pc_call, containerBlock, 0));

			parser_state_t containerState(state->lex);

			frame.push_back(scope_t(containerBlock->kind));
			{
				auto InsertSymbol = [&](size_t var, const std::string& name, bool isInternal) {
					symbol s;
					s.level = containerBlock->level;
					s.sub = nullptr;
					s.variable = var;
					s.can_overload = false;
					s.can_modify = !isInternal;
					frame.back().singular_insert(name, s);
				};

				InsertSymbol(0, "!0", true);	//An internal value, inaccessible to scripters (value copied to s_2 in each loop)
				InsertSymbol(1, "!1", true);	//An internal value, inaccessible to scripters
				InsertSymbol(2, counterName, false);	//The actual counter

				containerState.AddCode(containerBlock, code(command_kind::pc_var_alloc, 3));

				parse_expression(containerBlock, &containerState);	//First value, to s_0 if ascent
				containerState.AddCode(containerBlock, code(command_kind::pc_assign,
					containerBlock->level, (int)(!isAscent), "!0"));

				parser_assert(&containerState, containerState.next() == token_kind::tk_range, "\"..\" is required.\r\n");
				containerState.advance();

				parse_expression(containerBlock, &containerState);	//Second value, to s_0 if descent
				containerState.AddCode(containerBlock, code(command_kind::pc_assign,
					containerBlock->level, (int)(isAscent), "!1"));

				parser_assert(&containerState, containerState.next() == token_kind::tk_close_par, "\")\" is required.\r\n");
				containerState.advance();

				if (containerState.next() == token_kind::tk_LOOP) {
					containerState.advance();
				}

				size_t ip = containerBlock->codes.size();

				containerState.AddCode(containerBlock, code(command_kind::pc_push_variable,
					containerBlock->level, 0, "!0"));
				containerState.AddCode(containerBlock, code(command_kind::pc_push_variable,
					containerBlock->level, 1, "!1"));
				containerState.AddCode(containerBlock, code(isAscent ? command_kind::pc_compare_and_loop_ascent :
					command_kind::pc_compare_and_loop_descent));

				if (!isAscent) {
					containerState.AddCode(containerBlock, code(command_kind::pc_inline_dec,
						containerBlock->level, 0, "!0"));
				}

				//Copy s_0 to s_2
				containerState.AddCode(containerBlock, code(command_kind::pc_push_variable,
					containerBlock->level, 0, "!0"));
				containerState.AddCode(containerBlock, code(command_kind::pc_assign,
					containerBlock->level, 2, counterName));

				//Parse the code contained inside the loop
				size_t codeBlockSize = parse_inline_block(nullptr, containerBlock, &containerState, block_kind::bk_loop, true);

				containerState.AddCode(containerBlock, code(command_kind::pc_continue_marker));

				if (isAscent) {
					containerState.AddCode(containerBlock, code(command_kind::pc_inline_inc,
						containerBlock->level, 0, "!0"));
				}

				containerState.AddCode(containerBlock, code(command_kind::pc_jump, ip));
				containerState.AddCode(containerBlock, code(command_kind::pc_loop_back));

				if (codeBlockSize == 0U) {
					engine->blocks.pop_back();	//containerBlock
					while (state->ip > ip_ascdsc_begin)
						state->PopCode(block);
				}
			}
			frame.pop_back();

			need_terminator = false;
			break;
		}
		case token_kind::tk_IF:
		{
			std::map<size_t, size_t> mapLabelCode;

			size_t ip_begin = state->ip;

			size_t indexLabel = 0;
			bool hasNextCase = false;
			do {
				hasNextCase = false;
				parser_assert(state, indexLabel < 0xffffff, "Too many conditional cases.");
				state->advance();

				//Jump to next case if false
				parse_parentheses(block, state);
				state->AddCode(block, code(command_kind::_pc_jump_if_not, indexLabel));
				parse_inline_block(nullptr, block, state, block_kind::bk_normal, true);

				if (state->next() == token_kind::tk_ELSE) {
					state->advance();

					//Jump to exit
					state->AddCode(block, code(command_kind::_pc_jump, UINT_MAX));

					mapLabelCode.insert(std::make_pair(indexLabel, state->ip));
					if (state->next() != token_kind::tk_IF) {
						parse_inline_block(nullptr, block, state, block_kind::bk_normal, true);
						hasNextCase = false;
					}
					else if (state->next() == token_kind::tk_IF) hasNextCase = true;
				}
				++indexLabel;
			} while (hasNextCase);

			size_t ip_end = state->ip;
			mapLabelCode.insert(std::make_pair(UINT_MAX, ip_end));

			//Replace labels with actual code offset
			for (; ip_begin < ip_end; ++ip_begin) {
				code* c = &block->codes[ip_begin];
				if (c->command == command_kind::_pc_jump_if || c->command == command_kind::_pc_jump_if_not
					|| c->command == command_kind::_pc_jump) {
					auto itrMap = mapLabelCode.find(c->ip);
					c->command = get_replacing_jump(c->command);
					c->ip = (itrMap != mapLabelCode.end()) ? itrMap->second : ip_end;
				}
			}

			need_terminator = false;
			break;
		}
		case token_kind::tk_ALTERNATIVE:
		{
			std::map<size_t, size_t> mapLabelCode;

			state->advance();
			parse_parentheses(block, state);

			size_t ip_begin = state->ip;

			size_t indexLabel = 0;
			while (state->next() == token_kind::tk_CASE) {
				//Too many, man. Too many.
				parser_assert(state, indexLabel < 0x1000000, "Too many conditional cases.");

				state->advance();
				parser_assert(state, state->next() == token_kind::tk_open_par, "\"(\" is required.\r\n");

				do {
					state->advance();
					state->AddCode(block, code(command_kind::pc_dup_n, 1));
					parse_expression(block, state);
					state->AddCode(block, code(command_kind::pc_inline_cmp_e));
					state->AddCode(block, code(command_kind::_pc_jump_if, indexLabel));
				} while (state->next() == token_kind::tk_comma);

				parser_assert(state, state->next() == token_kind::tk_close_par, "\")\" is required.\r\n");
				state->advance();

				//Skip to next case
				state->AddCode(block, code(command_kind::_pc_jump, indexLabel + 1));

				mapLabelCode.insert(std::make_pair(indexLabel, state->ip));
				parse_inline_block(nullptr, block, state, block_kind::bk_normal, true);

				//Jump to exit
				state->AddCode(block, code(command_kind::_pc_jump, UINT_MAX));

				mapLabelCode.insert(std::make_pair(indexLabel + 1, state->ip));
				indexLabel += 2;
			}
			if (state->next() == token_kind::tk_OTHERS) {
				state->advance();
				parse_inline_block(nullptr, block, state, block_kind::bk_normal, true);
			}
			state->AddCode(block, code(command_kind::pc_pop, 1));

			size_t ip_end = state->ip;
			mapLabelCode.insert(std::make_pair(UINT_MAX, ip_end));

			//Replace labels with actual code offset
			for (; ip_begin < ip_end; ++ip_begin) {
				code* c = &block->codes[ip_begin];
				if (c->command == command_kind::_pc_jump_if || c->command == command_kind::_pc_jump_if_not
					|| c->command == command_kind::_pc_jump) {
					auto itrMap = mapLabelCode.find(c->ip);
					c->command = get_replacing_jump(c->command);
					c->ip = (itrMap != mapLabelCode.end()) ? itrMap->second : ip_end;
				}
			}

			need_terminator = false;
			break;
		}
		case token_kind::tk_BREAK:
		case token_kind::tk_CONTINUE:
		{
			command_kind c = state->next() == token_kind::tk_BREAK ?
				command_kind::pc_break_loop : command_kind::pc_loop_continue;
			state->advance();
			state->AddCode(block, code(c));
			break;
		}
		case token_kind::tk_RETURN:
		{
			state->advance();

			switch (state->next()) {
			case token_kind::tk_end:
			case token_kind::tk_invalid:
			case token_kind::tk_semicolon:
			case token_kind::tk_close_cur:
				break;
			default:
			{
				parse_expression(block, state);
				symbol* s = search_result();
				parser_assert(state, s, "Only functions may return values.\r\n");

				state->AddCode(block, code(command_kind::pc_assign, s->level, s->variable,
					"[function_result]"));
			}
			}
			state->AddCode(block, code(command_kind::pc_break_routine));

			break;
		}
		case token_kind::tk_YIELD:
		case token_kind::tk_WAIT:
		{
			command_kind c = state->next() == token_kind::tk_YIELD ? command_kind::pc_yield : command_kind::pc_wait;
			state->advance();
			if (c == command_kind::pc_wait) parse_parentheses(block, state);
			state->AddCode(block, code(c));
			break;
		}
		case token_kind::tk_at:
		case token_kind::tk_SUB:
		case token_kind::tk_FUNCTION:
		case token_kind::tk_TASK:
		{
			token_kind token = state->next();

			state->advance();
			if (state->next() != token_kind::tk_word) {
				std::string error = "This token cannot be used to declare a";
				switch (token) {
				case token_kind::tk_at:
					error += "n event(\'@\') block";
					break;
				case token_kind::tk_SUB:
					error += " sub";
					break;
				case token_kind::tk_FUNCTION:
					error += " function";
					break;
				case token_kind::tk_TASK:
					error += " task";
					break;
				}
				error += ".\r\n";
				throw parser_error(error);
			}

			std::string funcName = state->lex->word;
			scope_t* pScope = nullptr;

			symbol* s = search(funcName, &pScope);

			if (token == token_kind::tk_at) {
				parser_assert(state, s->level == 1, "An event(\'@\') block cannot exist here.\r\n");

				events[funcName] = s->sub;
			}

			state->advance();

			std::vector<std::string> args;
			if (s->sub->kind != block_kind::bk_sub) {
				if (state->next() == token_kind::tk_open_par) {
					state->advance();
					while (state->next() == token_kind::tk_word || IsDeclToken(state->next())) {
						if (IsDeclToken(state->next())) {
							state->advance();
							parser_assert(state, state->next() == token_kind::tk_word, "Parameter name is required.\r\n");
						}
						//TODO: Give scripters access to defining functions with variadic argument counts. (After 1.10a)
						args.push_back(state->lex->word);
						state->advance();
						if (state->next() != token_kind::tk_comma)
							break;
						state->advance();
					}
					parser_assert(state, state->next() == token_kind::tk_close_par, "\")\" is required.\r\n");
					state->advance();
				}
			}
			else {
				if (state->next() == token_kind::tk_open_par) {
					state->advance();
					parser_assert(state, state->next() == token_kind::tk_close_par, "Only an empty parameter list is allowed here.\r\n");
					state->advance();
				}
			}

			s = search_in(pScope, funcName, args.size());

			{
				parser_state_t newState(state->lex);
				parse_block(s->sub, &newState, &args, s->sub->kind == block_kind::bk_function, false);
			}

			need_terminator = false;
			break;
		}
		}

		return need_terminator;
	}
	void parser::parse_statements(script_block* block, parser_state_t* state, token_kind statement_terminator) {
		for (; ; ) {
			bool need_terminator = parse_single_statement(block, state);

			if (state->lex->next == statement_terminator) state->advance();
			else if (need_terminator) {
				//parser_assert(state, false, "Expected a semicolon (;).");
				break;
			}
		}
	}

	size_t parser::parse_inline_block(script_block** blockRes, script_block* block, 
		parser_state_t* state, block_kind kind, bool allow_single) 
	{
		script_block* b = engine->new_block(block->level + 1, kind);
		if (blockRes) *blockRes = b;

		parser_state_t newState(state->lex);
		parse_block(b, &newState, nullptr, false, allow_single);

		if (b->codes.size() == 0U) {
			engine->blocks.pop_back();
			return 0;
		}

		state->AddCode(block, code(command_kind::pc_call, b, 0));
		return b->codes.size();
	}

	void parser::parse_block(script_block* block, parser_state_t* state, std::vector<std::string> const* args,
		bool adding_result, bool allow_single) {
		bool single_line = state->next() != token_kind::tk_open_cur && allow_single;
		if (!single_line) {
			parser_assert(state, state->next() == token_kind::tk_open_cur, "\"{\" is required.\r\n");
			state->advance();
		}

		frame.push_back(scope_t(block->kind));

		if (!single_line) {
			int countVar = scan_current_scope(state, block->level, args, adding_result);
			if (countVar > 0)
				state->AddCode(block, code(command_kind::pc_var_alloc, countVar));
		}
		if (args) {
			scope_t* ptrBackFrame = &frame.back();

			for (size_t i = 0; i < args->size(); ++i) {
				const std::string& name = args->at(i);
				if (single_line) {	//As scan_current_scope won't be called.
					symbol s = { block->level, nullptr, (int)i, false, true };
					ptrBackFrame->singular_insert(name, s);
				}

				symbol* s = search(name);
				state->AddCode(block, code(command_kind::pc_assign, s->level, s->variable, name));
			}
		}
		if (single_line) {
			bool need_terminator = parse_single_statement(block, state);

			if (state->next() == token_kind::tk_semicolon) state->advance();
			else if (need_terminator)
				parser_assert(state, false, "Expected a semicolon (;).");
		}
		else {
			parse_statements(block, state, token_kind::tk_semicolon);
		}

		frame.pop_back();

		if (!single_line) {
			parser_assert(state, state->next() == token_kind::tk_close_cur, "\"}\" is required.\r\n");
			state->advance();
		}
	}

	void parser::optimize_expression(script_block* block, parser_state_t* state) {
		std::vector<code> newCodes;

		for (auto iSrcCode = block->codes.begin(); iSrcCode != block->codes.end(); ++iSrcCode) {
			switch (iSrcCode->command) {
			case command_kind::pc_inline_neg:
			case command_kind::pc_inline_not:
			case command_kind::pc_inline_abs:
			{
				code* ptrBack = &newCodes.back();
				if (ptrBack->command == command_kind::pc_push_value) {
					value* arg = &(ptrBack->data);
					value res;
					switch (iSrcCode->command) {
					case command_kind::pc_inline_neg:
						res = BaseFunction::_script_negative(1, arg);
						break;
					case command_kind::pc_inline_not:
						res = BaseFunction::_script_not_(1, arg);
						break;
					case command_kind::pc_inline_abs:
						res = BaseFunction::_script_absolute(1, arg);
						break;
					}
					newCodes.pop_back();
					newCodes.push_back(code(iSrcCode->line, command_kind::pc_push_value, res));
					--(state->ip);
				}
				else {
					newCodes.push_back(*iSrcCode);
				}
				break;
			}
			case command_kind::pc_inline_add:
			case command_kind::pc_inline_sub:
			case command_kind::pc_inline_mul:
			case command_kind::pc_inline_div:
			case command_kind::pc_inline_mod:
			case command_kind::pc_inline_pow:
			{
				code* ptrBack = &newCodes.back();
				if (ptrBack[-1].command == command_kind::pc_push_value && ptrBack->command == command_kind::pc_push_value) {
					value arg[] = { ptrBack[-1].data, ptrBack->data };
					value res;
					switch (iSrcCode->command) {
					case command_kind::pc_inline_add:
						res = BaseFunction::_script_add(2, arg);
						break;
					case command_kind::pc_inline_sub:
						res = BaseFunction::_script_subtract(2, arg);
						break;
					case command_kind::pc_inline_mul:
						res = BaseFunction::_script_multiply(2, arg);
						break;
					case command_kind::pc_inline_div:
						res = BaseFunction::_script_divide(2, arg);
						break;
					case command_kind::pc_inline_mod:
						res = BaseFunction::_script_remainder_(2, arg);
						break;
					case command_kind::pc_inline_pow:
						res = BaseFunction::_script_power(2, arg);
						break;
					}
					newCodes.pop_back();
					newCodes.pop_back();
					newCodes.push_back(code(iSrcCode->line, command_kind::pc_push_value, res));
					state->ip -= 2;
				}
				else {
					newCodes.push_back(*iSrcCode);
				}
				break;
			}
			default:
				newCodes.push_back(*iSrcCode);
				break;
			}
		}

		block->codes = newCodes;
	}
	//Links jump commands with their matching jump targets
	void parser::link_jump(script_block* block, parser_state_t* state, size_t ip_off) {
		std::vector<code> newCodes;
		std::map<size_t, size_t> mapLabelCode;

		{
			size_t ip = 0;
			size_t removing = 0;
			for (auto itr = block->codes.begin(); itr != block->codes.end(); ++itr, ++ip) {
				switch (itr->command) {
				case command_kind::pc_jump_target:
					mapLabelCode.insert(std::make_pair(itr->ip, ip - removing));
					++removing;
					break;
				}
			}
		}

		for (auto itr = block->codes.begin(); itr != block->codes.end(); ++itr) {
			code iCode = *itr;

			switch (iCode.command) {
			case command_kind::pc_jump_target:
				--(state->ip);
				break;
			case command_kind::_pc_jump:
			case command_kind::_pc_jump_if:
			case command_kind::_pc_jump_if_not:
			{
				auto itrFind = mapLabelCode.find(iCode.ip);
				if (itrFind != mapLabelCode.end()) {
					iCode = code(iCode.line, get_replacing_jump(iCode.command), itrFind->second + ip_off);
				}
			}
			//Fallthrough
			default:
				newCodes.push_back(iCode);
				break;
			}
		}

		block->codes = newCodes;
	}
}
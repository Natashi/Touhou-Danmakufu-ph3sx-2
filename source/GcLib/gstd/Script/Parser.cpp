#include "source/GcLib/pch.h"

#include "Parser.hpp"
#include "ScriptFunction.hpp"
#include "Script.hpp"

//Natashi's TODO: Implement a parse tree

namespace gstd {
	script_block::script_block(int the_level, block_kind the_kind) {
		level = the_level;
		arguments = 0;
		func = nullptr;
		kind = the_kind;
	}

#pragma push_macro("new")
#undef new
	code::code() {
	}
	code::code(int the_line, command_kind the_command) {
		line = the_line;
		command = the_command;
	}
	code::code(int the_line, command_kind the_command, int the_level, size_t the_variable, const std::string& the_name)
		: code(the_line, the_command) {
		level = the_level;
		variable = the_variable;
#ifdef _DEBUG
		var_name = the_name;
#endif
	}
	code::code(int the_line, command_kind the_command, script_block* the_sub, int the_arguments) : code(the_line, the_command) {
		sub = the_sub;
		arguments = the_arguments;
	}
	code::code(int the_line, command_kind the_command, size_t the_ip) : code(the_line, the_command) {
		ip = the_ip;
	}
	code::code(int the_line, command_kind the_command, size_t the_off, size_t the_len) : code(the_line, the_command) {
		off = the_off;
		len = the_len;
	}
	code::code(int the_line, command_kind the_command, const value& the_data) : code(the_line, the_command) {
		new (&data) value(the_data);
	}
	code::code(const code& src) {
		*this = src;
	}
	code::~code() {
		switch (command) {
		case command_kind::pc_push_value:
			data.~value();
			break;
		}
	}

	code& code::operator=(const code& src) {
		if (this == std::addressof(src)) return *this;
		this->~code();

		switch (src.command) {
		case command_kind::pc_push_value:
			new (&data) value(src.data);
			break;
		default:
			sub = src.sub;
			arguments = src.arguments;
			break;
		}
		command = src.command;
		line = src.line;
#ifdef _DEBUG
		var_name = src.var_name;
#endif

		return *this;
	}
#pragma pop_macro("new")

	static const std::vector<function> base_operations = {
		//{ "true", true_, 0 },
		//{ "false", false_, 0 },
		//{ "length", BaseFunction::length, 1 },
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

		//Uh oh! Duplication!
		if (exists) {
			symbol* sPrev = &(itr.first->second);

			//Check if the symbol can be overloaded
			if (!sPrev->can_overload && sPrev->level > 0) {
				std::string error = "";
				if (sPrev->sub) {	//Scripter attempted to overload a default function
					error = StringUtility::Format("\"%s\": Function cannot be overloaded.",
						name.c_str());
				}
				else {				//Scripter duplicated parameter/variable name
					error = StringUtility::Format("\"%s\": Duplicated parameter name.",
						name.c_str());
				}
				parser_assert(false, error);
			}
			else {
				for (auto itrPair = itr.first; itrPair != itr.second; ++itrPair) {
					if (argc == itrPair->second.sub->arguments) {
						std::string error = StringUtility::Format("An overload with the same number of "
							"arguments already exists. (%s, argc=%d)",
							sPrev->sub->name.c_str(), argc);
						parser_assert(false, error);
					}
				}
			}
		}

		this->insert(std::make_pair(name, s));
	}

	parser::parser(script_engine* e, script_scanner* s) {
		engine = e;
		lexer_main = s;
		error = false;

		count_base_constants = 0;

		frame.push_back(scope_t(block_kind::bk_normal));		//Scope for default symbols

		//Base script operations
		for (auto& iFunc : base_operations)
			register_function(iFunc);

		{
			block_const_reg = engine->new_block(2, block_kind::bk_normal);
			block_const_reg->name = "$_scpt_const_reg";
			engine->main_block->codes.push_back(code(command_kind::pc_var_alloc, 0));
			engine->main_block->codes.push_back(code(command_kind::pc_call, block_const_reg, 0));
		}
	}
	void parser::load_functions(std::vector<function>* list_func) {
		//Client script function extensions
		for (auto itr = list_func->begin(); itr != list_func->end(); ++itr)
			register_function(*itr);
	}
	void parser::load_constants(std::vector<constant>* list_const) {
		size_t iConst = count_base_constants;
		for (auto itr = list_const->begin(); itr != list_const->end(); ++itr, ++iConst) {
			const constant* pConst = &*itr;

			value const_value;
			switch (pConst->type) {
			case type_data::tk_int:
				const_value.set(script_type_manager::get_int_type(), (int64_t&)pConst->data);
				break;
			case type_data::tk_real:
				const_value.set(script_type_manager::get_real_type(), (double&)pConst->data);
				break;
			case type_data::tk_char:
				const_value.set(script_type_manager::get_char_type(), (wchar_t&)pConst->data);
				break;
			case type_data::tk_boolean:
				const_value.set(script_type_manager::get_boolean_type(), (bool&)pConst->data);
				break;
			default:
				continue;
			}
			block_const_reg->codes.push_back(code(command_kind::pc_push_value, const_value));
			block_const_reg->codes.push_back(code(command_kind::pc_copy_assign, 1, iConst, pConst->name));

			symbol s = symbol(1, nullptr, iConst, false, false);
			frame.begin()->singular_insert(pConst->name, s);
		}
		count_base_constants += list_const->size();
	}
	void parser::begin_parse() {
		try {
			frame.push_back(scope_t(block_kind::bk_normal));	//Scope for user symbols

			parser_state_t stateParser(lexer_main);
			stateParser.ip = engine->main_block->codes.size();

			stateParser.var_count_main = scan_current_scope(&stateParser, 1, nullptr, false, count_base_constants);
			stateParser.var_count_main += count_base_constants;

			parse_statements(engine->main_block, &stateParser, token_kind::tk_end, token_kind::tk_semicolon);
			scan_final(engine->main_block, &stateParser);

			//for pc_var_alloc
			engine->main_block->codes[0].ip = count_base_constants + stateParser.var_count_main + stateParser.var_count_sub;

			parser_assert(stateParser.next() == token_kind::tk_end,
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
		script_block* block = engine->new_block(0, block_kind::bk_function);
		block->arguments = func.arguments;
		block->name = func.name;
		block->func = func.func;
		symbol s = symbol(0, block, -1, false, false);
		frame.begin()->singular_insert(func.name, s, func.arguments);
	}

	parser::symbol* parser::search(const std::string& name, scope_t** ptrScope) {
		for (auto itr = frame.rbegin(); itr != frame.rend(); ++itr) {
			scope_t* scope = &*itr;
			if (ptrScope) *ptrScope = scope;

			auto itrSymbol = scope->find(name);
			if (itrSymbol != scope->end())
				return &itrSymbol->second;
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
					symbol* s = &itrPair->second;
					if (s->sub) {
						//Check overload
						if (argc == s->sub->arguments)
							return s;
					}
					else return s;
				}
				return nullptr;
			}
		}
		return nullptr;
	}
	parser::symbol* parser::search_in(scope_t* scope, const std::string& name) {
		auto itrSymbol = scope->find(name);
		if (itrSymbol != scope->end())
			return &itrSymbol->second;
		return nullptr;
	}
	parser::symbol* parser::search_in(scope_t* scope, const std::string& name, int argc) {
		if (scope->find(name) == scope->end()) return nullptr;

		auto itrSymbol = scope->equal_range(name);
		if (itrSymbol.first != scope->end()) {
			for (auto itrPair = itrSymbol.first; itrPair != itrSymbol.second; ++itrPair) {
				symbol* s = &itrPair->second;
				if (s->sub) {
					//Check overload
					if (argc == s->sub->arguments)
						return s;
				}
				else return s;
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
				return &itrSymbol->second;
		}
		return nullptr;
	}

	int parser::scan_current_scope(parser_state_t* state, int level, const std::vector<std::string>* args,
		bool adding_result, int initVar)
	{
		//Scans and defines scope variables and functions/tasks/subs

		script_scanner lex2(*state->lex);

		auto IsReadableToken = [&]() -> bool {
			return lex2.next != token_kind::tk_end && lex2.next != token_kind::tk_invalid;
		};

		int var = initVar;

		try {
			scope_t* current_frame = &frame.back();
			int cur = 0;
			int par = 0;

			if (adding_result) {
				symbol s = symbol(level, nullptr, var++, false, false);
				current_frame->singular_insert("\x01", s);
			}

			if (args) {
				for (size_t i = 0; i < args->size(); ++i) {
					symbol s = symbol(level, nullptr, var++, false, true);
					current_frame->singular_insert(args->at(i), s);
				}
			}

			while (cur >= 0 && IsReadableToken()) {
				bool is_const = false;

				switch (lex2.next) {
				case token_kind::tk_open_par:
					++par;
					lex2.advance();
					break;
				case token_kind::tk_open_cur:
					++cur;
					lex2.advance();
					break;
				case token_kind::tk_close_par:
					--par;
					lex2.advance();
					break;
				case token_kind::tk_close_cur:
					--cur;
					if (cur == 0) par = 0;
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
								dup = search_in(&*frame.begin(), name, countArgs);

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
								parser_assert(false, error);
							}
						}

						script_block* newBlock = engine->new_block(level + 1, kind);
						newBlock->name = name;
						newBlock->func = nullptr;
						newBlock->arguments = countArgs;
						symbol s = symbol(level, newBlock, -1, kind != block_kind::bk_sub, false);
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
					if (cur == 0 && par == 0) {
						{
							symbol* dup = search_in(current_frame, lex2.word);

							//If the level is 1(first user level), also search for duplications in level 0(default level)
							if (dup == nullptr && level == 1)
								dup = search_in(&*frame.begin(), lex2.word);

							if (dup) {
								std::string error = StringUtility::Format("A variable of the same name "
									"was already declared in the current scope.\r\n", lex2.word.c_str());
								parser_assert(false, error);
							}
						}

						symbol s = symbol(level, nullptr, var++, false, !is_const);
						current_frame->singular_insert(lex2.word, s);

						lex2.advance();
					}
					break;
				}
				/*
				case token_kind::tk_LOOP:
				case token_kind::tk_TIMES:
				case token_kind::tk_WHILE:
				case token_kind::tk_FOR:
				case token_kind::tk_ASCENT:
				case token_kind::tk_DESCENT:
				case token_kind::tk_IF:
				case token_kind::tk_ELSE:
				case token_kind::tk_CASE:
				{
					while (IsReadableToken()) {
						if (lex2.next == token_kind::tk_open_par || lex2.next == token_kind::tk_open_cur)
							break;
						lex2.advance();
					}

					if (lex2.next == token_kind::tk_open_par) {
						int cPar = 0;
						while (IsReadableToken()) {
							if (lex2.next == token_kind::tk_open_par) ++cPar;
							else if (lex2.next == token_kind::tk_close_par) --cPar;
							if (cPar == 0) break;
							lex2.advance();
						}
						lex2.advance();
					}

					//Skip until a { for normal blocks or a ; in case of single-lined blocks
					while (IsReadableToken()) {
						if (lex2.next == token_kind::tk_open_cur || lex2.next == token_kind::tk_semicolon)
							break;
						lex2.advance();
					}

					break;
				}
				*/
				default:
					lex2.advance();
					break;
				}
			}
		}
		catch (parser_error& e) {
			//state->lex->line = lex2.line;
			throw parser_error_mapped(lex2.line, e.GetMessageW());
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
		case token_kind::tk_int:
			state->AddCode(block, code(command_kind::pc_push_value,
				value(script_type_manager::get_int_type(), state->lex->int_value)));
			state->advance();
			return;
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
					error = StringUtility::Format("No overload of %s takes %d arguments.\r\n",
						name.c_str(), argc);
				else
					error = StringUtility::Format("%s is not defined.\r\n", name.c_str());
				parser_assert(state, false, error);
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
			type_data::type_kind target = type_data::tk_null;
			switch (state->next()) {
			case token_kind::tk_cast_int:
				target = type_data::tk_int;
				break;
			case token_kind::tk_cast_real:
				target = type_data::tk_real;
				break;
			case token_kind::tk_cast_char:
				target = type_data::tk_char;
				break;
			case token_kind::tk_cast_bool:
				target = type_data::tk_boolean;
				break;
			}
			state->advance();
			parse_parentheses(block, state);
			state->AddCode(block, code(command_kind::pc_inline_cast_var, (size_t)target));
			return;
		}
		case token_kind::tk_LENGTH:
			state->advance();
			parse_parentheses(block, state);
			state->AddCode(block, code(command_kind::pc_inline_length_array));
			return;
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
					value(script_type_manager::get_null_array_type(), 0i64)));

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
				command_kind::_pc_jump_if_not_nopop : command_kind::_pc_jump_if_nopop;
			state->advance();

			state->AddCode(block, code(cmdJump, iter));

			parse_bitwise(block, state);

			state->AddCode(block, code(cmdLogic));
			state->AddCode(block, code(command_kind::pc_jump_target, iter));
			--(state->ip);

			++iter;
		}
	}

	void parser::parse_ternary(script_block* block, parser_state_t* state) {
		parse_logic(block, state);
		if (state->next() == token_kind::tk_query) {
			state->advance();

			size_t hash = (size_t)block ^ ((size_t)state << 12) ^ 0xb4ab5cc9 + state->ip * 0xf6;
			size_t hashJ1 = 0xd0000000 | (hash & 0x0fffffff);
			size_t hashJ2 = 0xe0000000 | (hash & 0x0fffffff);

			state->AddCode(block, code(command_kind::_pc_jump_if_not, hashJ1));	//Jump to expression 2

			//Parse expression 1
			parse_ternary(block, state);
			state->AddCode(block, code(command_kind::_pc_jump, hashJ2));		//Jump to exit

			parser_assert(state, state->next() == token_kind::tk_colon, 
				"Incomplete ternary statement; a colon(:) is required.");
			state->advance();

			//Parse expression 2
			state->AddCode(block, code(command_kind::pc_jump_target, hashJ1));
			--(state->ip);
			parse_ternary(block, state);

			//Exit point
			state->AddCode(block, code(command_kind::pc_jump_target, hashJ2));
			--(state->ip);
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
		catch (std::string& err) {
			parser_assert(state, false, err);
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

	void parser::parse_single_statement(script_block* block, parser_state_t* state, 
		bool check_terminator, token_kind statement_terminator) 
	{
		auto assert_const = [](parser_state_t* _state, symbol* s, const std::string& name) {
			if (!s->can_modify) {
				std::string error = StringUtility::Format("\"%s\": ", name.c_str());
				if (s->sub)
					parser_assert(_state, false, error + "Functions, tasks, and subs are implicitly const\r\n"
						"and thus cannot be modified in this manner.\r\n");
				else
					parser_assert(_state, false, error + "const variables cannot be modified.\r\n");
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
			case token_kind::tk_concat_assign:
				return command_kind::pc_inline_cat_asi;
			}
			return command_kind::pc_null;
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
				assert_const(state, s, name);

				state->advance();
				parse_expression(block, state);
				state->AddCode(block, code(command_kind::pc_copy_assign, s->level, s->variable, name));
				break;
			case token_kind::tk_open_bra:
				assert_const(state, s, name);

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
					state->AddCode(block, code(command_kind::pc_ref_overwrite, 0, 0, name));
					break;
				case token_kind::tk_add_assign:
				case token_kind::tk_subtract_assign:
				case token_kind::tk_multiply_assign:
				case token_kind::tk_divide_assign:
				case token_kind::tk_remainder_assign:
				case token_kind::tk_power_assign:
				case token_kind::tk_concat_assign:
				{
					command_kind f = get_op_assign_command(state->next());
					state->advance();
					parse_expression(block, state);
					state->AddCode(block, code(f, -1, 0, name));
					break;
				}
				case token_kind::tk_inc:
				case token_kind::tk_dec:
				{
					command_kind f = (state->next() == token_kind::tk_inc) ? command_kind::pc_inline_inc :
						command_kind::pc_inline_dec;
					state->advance();
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
			case token_kind::tk_concat_assign:
			{
				assert_const(state, s, name);

				command_kind f = get_op_assign_command(state->next());
				state->advance();
				parse_expression(block, state);
				state->AddCode(block, code(f, s->level, s->variable, name));

				break;
			}
			case token_kind::tk_inc:
			case token_kind::tk_dec:
			{
				assert_const(state, s, name);

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
					parser_assert(state, s, StringUtility::Format("No overload of %s takes %d arguments.\r\n",
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
				state->AddCode(block, code(command_kind::pc_copy_assign, s->level, s->variable, name));
			}

			break;
		}
		//"local" can be omitted to make C-style blocks
		case token_kind::tk_LOCAL:
		case token_kind::tk_open_cur:
			if (state->next() == token_kind::tk_LOCAL) state->advance();
			parse_block_inlined(block, state, false);
			need_terminator = false;
			break;
		case token_kind::tk_LOOP:
		{
			state->advance();
			size_t ip_begin = state->ip;
			if (state->next() == token_kind::tk_open_par) {
				parse_parentheses(block, state);
				{
					state->AddCode(block, code(command_kind::pc_inline_cast_var, (size_t)type_data::tk_int));

					size_t ip_var_format = state->ip;
					state->AddCode(block, code(command_kind::pc_var_format, 0, 0));

					size_t ip_loop_begin = state->ip;
					state->AddCode(block, code(command_kind::pc_loop_count));
					size_t ip_loopchk = state->ip;
					state->AddCode(block, code(command_kind::pc_jump_if_not, 0U));

					size_t ip_block_begin = state->ip;
					auto blockParam = parse_block(block, state, nullptr, true);

					size_t ip_continue = state->ip;
					state->AddCode(block, code(command_kind::pc_jump, ip_loop_begin));
					size_t ip_back = state->ip;
					state->AddCode(block, code(command_kind::pc_pop, 1));

					//Try to optimize looped yield to a wait
					if (blockParam.first == 1U && block->codes[ip_block_begin].command == command_kind::pc_yield) {
						while (state->ip > ip_loop_begin)
							state->PopCode(block);
						state->AddCode(block, code(command_kind::pc_wait));
					}
					//Discard everything if the loop is empty
					else if (blockParam.first == 0U) {
						while (state->ip > ip_loop_begin)
							state->PopCode(block);
					}
					else {
						block->codes[ip_loopchk].ip = ip_back;
						link_break_continue(block, state, ip_block_begin, ip_continue, ip_back, ip_continue);

						block->codes[ip_var_format].off = blockParam.second[0];
						block->codes[ip_var_format].len = blockParam.second[1];
					}
				}
			}
			else {
				size_t ip_var_format = state->ip;
				state->AddCode(block, code(command_kind::pc_var_format, 0, 0));

				auto blockParam = parse_block(block, state, nullptr, true);

				size_t ip_continue = state->ip;
				state->AddCode(block, code(command_kind::pc_jump, ip_begin));
				size_t ip_back = state->ip;

				if (blockParam.first == 0U) {
					while (state->ip > ip_begin)
						state->PopCode(block);
				}
				else {
					link_break_continue(block, state, ip_begin, ip_continue, ip_back, ip_continue);

					block->codes[ip_var_format].off = blockParam.second[0];
					block->codes[ip_var_format].len = blockParam.second[1];
				}
			}

			need_terminator = false;
			break;
		}
		case token_kind::tk_TIMES:
		{
			state->advance();

			parse_parentheses(block, state);
			if (state->next() == token_kind::tk_LOOP) state->advance();

			{
				state->AddCode(block, code(command_kind::pc_inline_cast_var, (size_t)type_data::tk_int));

				size_t ip_var_format = state->ip;
				state->AddCode(block, code(command_kind::pc_var_format, 0, 0));

				size_t ip_loop_begin = state->ip;
				state->AddCode(block, code(command_kind::pc_loop_count));
				size_t ip_loopchk = state->ip;
				state->AddCode(block, code(command_kind::pc_jump_if_not, 0U));

				size_t ip_block_begin = state->ip;
				auto blockParam = parse_block(block, state, nullptr, true);

				size_t ip_continue = state->ip;
				state->AddCode(block, code(command_kind::pc_jump, ip_loop_begin));
				size_t ip_back = state->ip;
				state->AddCode(block, code(command_kind::pc_pop, 1));

				//Try to optimize looped yield to a wait
				if (blockParam.first == 1U && block->codes[ip_block_begin].command == command_kind::pc_yield) {
					while (state->ip > ip_loop_begin)
						state->PopCode(block);
					state->AddCode(block, code(command_kind::pc_wait));
				}
				//Discard everything if the loop is empty
				else if (blockParam.first == 0U) {
					while (state->ip > ip_loop_begin)
						state->PopCode(block);
				}
				else {
					block->codes[ip_loopchk].ip = ip_back;
					link_break_continue(block, state, ip_block_begin, ip_continue, ip_back, ip_continue);

					block->codes[ip_var_format].off = blockParam.second[0];
					block->codes[ip_var_format].len = blockParam.second[1];
				}
			}

			need_terminator = false;
			break;
		}
		case token_kind::tk_WHILE:
		{
			state->advance();

			size_t ip_var_format = state->ip;
			state->AddCode(block, code(command_kind::pc_var_format, 0, 0));

			size_t ip = state->ip;

			parse_parentheses(block, state);
			if (state->next() == token_kind::tk_LOOP)
				state->advance();

			size_t ip_loopchk = state->ip;
			state->AddCode(block, code(command_kind::pc_jump_if_not, 0U));

			size_t ip_block_begin = state->ip;
			auto blockParam = parse_block(block, state, nullptr, true);

			size_t ip_continue = state->ip;
			state->AddCode(block, code(command_kind::pc_jump, ip));
			size_t ip_back = state->ip;

			if (blockParam.first == 0U) {
				while (state->ip > ip)
					state->PopCode(block);
			}
			else {
				block->codes[ip_loopchk].ip = ip_back;
				link_break_continue(block, state, ip_block_begin, ip_continue, ip_back, ip_continue);

				block->codes[ip_var_format].off = blockParam.second[0];
				block->codes[ip_var_format].len = blockParam.second[1];
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

				std::string counterName = state->lex->word;

				state->advance();
				parser_assert(state, state->next() == token_kind::tk_IN || state->next() == token_kind::tk_colon,
					"\"in\" or a colon is required.\r\n");
				state->advance();

				size_t ip_var_format = state->ip;
				state->AddCode(block, code(command_kind::pc_var_format, 0, 0));

				//The array
				parse_expression(block, state);

				parser_assert(state, state->next() == token_kind::tk_close_par, "\")\" is required.\r\n");
				state->advance();

				if (state->next() == token_kind::tk_LOOP) state->advance();

				//The counter
				state->AddCode(block, code(command_kind::pc_push_value,
					value(script_type_manager::get_int_type(), 0i64)));

				size_t ip = state->ip;

				state->AddCode(block, code(command_kind::pc_loop_foreach));
				size_t ip_loopchk = state->ip;
				state->AddCode(block, code(command_kind::pc_jump_if, 0U));

				std::pair<size_t, std::array<size_t, 2>> blockParam = std::make_pair(0, std::array<size_t, 2>{ 0, 0 });
				{
					std::vector<std::string> counter;
					counter.push_back(counterName);
					blockParam = parse_block(block, state, &counter, true);
				}

				size_t ip_continue = state->ip;
				state->AddCode(block, code(command_kind::pc_jump, ip));
				size_t ip_back = state->ip;

				//Pop twice for the array and the counter
				state->AddCode(block, code(command_kind::pc_pop, 2));

				if (blockParam.first <= 1U) {	//1 for pc_copy_assign
					while (state->ip > ip_for_begin)
						state->PopCode(block);
				}
				else {
					block->codes[ip_loopchk].ip = ip_back;
					link_break_continue(block, state, ip, ip_continue, ip_back, ip_continue);

					block->codes[ip_var_format].off = blockParam.second[0];
					block->codes[ip_var_format].len = blockParam.second[1];
				}
			}
			else if (state->next() == token_kind::tk_open_par) {	//Regular for loop
				state->advance();

				bool isNewVar = false;
				bool isNewVarConst = false;
				std::string newVarName = "";

				script_scanner* const lex_org = state->lex;
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

				size_t varc_prev_total = 0;
				size_t ip_var_format = 0;
				size_t ip_loopchk = 0;
				size_t ip_continue = 0;
				size_t ip_back = 0;
				size_t codeBlockSize = 0;

				parser_state_t newState(state->lex, state->ip);
				newState.state_pred = state;

				//For block
				frame.push_back(scope_t(block->kind));
				{
					if (isNewVar) {
						for (parser_state_t* iState = state; iState != nullptr; iState = iState->state_pred)
							varc_prev_total += iState->var_count_main;

						symbol s = symbol(block->level, nullptr, varc_prev_total, false, !isNewVarConst);
						frame.back().singular_insert(newVarName, s);

						newState.var_count_main = 1;

						ip_var_format = newState.ip;
						newState.AddCode(block, code(command_kind::pc_var_format, 0, 0));
					}

					//Initialization statement
					newState.lex = &lex_s1;
					parse_single_statement(block, &newState, true, token_kind::tk_semicolon);

					size_t ip_begin = newState.ip;

					//Evaluation statement
					if (hasExpr) {
						newState.lex = &lex_s2;
						parse_expression(block, &newState);
						parser_assert(state, lex_s2.next == token_kind::tk_semicolon, "Expected a semicolon (;).");
						ip_loopchk = newState.ip;
						newState.AddCode(block, code(command_kind::pc_jump_if_not, 0U));
					}

					//Parse loop body
					newState.lex = &lex_s4;
					{
						size_t prev = newState.ip;

						bool single_line = newState.next() != token_kind::tk_open_cur;
						if (!single_line)
							newState.advance();

						if (single_line)
							parse_single_statement(block, &newState, true, token_kind::tk_semicolon);
						else {
							newState.var_count_main += scan_current_scope(&newState, block->level, nullptr, 
								false, varc_prev_total);
							parse_statements(block, &newState, token_kind::tk_close_cur, token_kind::tk_semicolon);
						}

						if (!single_line)
							newState.advance();

						codeBlockSize = newState.ip - prev;
					}

					ip_continue = newState.ip;
					{
						//Update statement
						newState.lex = &lex_s3;
						parse_single_statement(block, &newState, false, token_kind::tk_comma);
						while (lex_s3.next == token_kind::tk_comma) {
							lex_s3.advance();
							parse_single_statement(block, &newState, false, token_kind::tk_comma);
						}
					}
					newState.AddCode(block, code(command_kind::pc_jump, ip_begin));
					ip_back = newState.ip;

					newState.lex = lex_org;
					newState.lex->copy_state(&lex_s4);
				}
				frame.pop_back();

				if (codeBlockSize == 0U) {
					while (newState.ip > ip_for_begin)
						newState.PopCode(block);
				}
				else {
					if (hasExpr) block->codes[ip_loopchk].ip = ip_back;
					link_break_continue(block, &newState, ip_for_begin, ip_continue, ip_back, ip_continue);

					if (isNewVar) {
						block->codes[ip_var_format].off = varc_prev_total;
						block->codes[ip_var_format].len = newState.var_count_main;
					}
				}

				state->ip = newState.ip;
				if (isNewVar) state->GrowVarCount(newState.var_count_main + newState.var_count_sub);
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

			//Parse the first expression
			parse_expression(block, state);

			parser_assert(state, state->next() == token_kind::tk_range, "\"..\" is required.\r\n");
			state->advance();

			//Parse the second expression
			parse_expression(block, state);

			parser_assert(state, state->next() == token_kind::tk_close_par, "\")\" is required.\r\n");
			state->advance();

			if (state->next() == token_kind::tk_LOOP)
				state->advance();

			//state->AddCode(block, code(command_kind::pc_dup_n_unique, 2));	//s1 s2		-> s1 s2 s1
			//state->AddCode(block, code(command_kind::pc_dup_n_unique, 2));	//s1 s2 s1	-> s1 s2 s1 s2

			if (isAscent)
				state->AddCode(block, code(command_kind::pc_swap));

			size_t ip_var_format = state->ip;
			state->AddCode(block, code(command_kind::pc_var_format, 0, 0));

			size_t ip = state->ip;
			
			state->AddCode(block, code(isAscent ? command_kind::pc_loop_ascent : command_kind::pc_loop_descent));
			size_t ip_loopchk = state->ip;
			state->AddCode(block, code(command_kind::pc_jump_if, 0U));

			if (!isAscent) {
				state->AddCode(block, code(command_kind::pc_inline_top_dec));
				//state->AddCode(block, code(command_kind::pc_dup_n, 1));
				//state->AddCode(block, code(command_kind::pc_inline_dec, -1, 0, "!0"));
			}

			state->AddCode(block, code(command_kind::pc_dup_n, 1));

			std::pair<size_t, std::array<size_t, 2>> blockParam = std::make_pair(0, std::array<size_t, 2>{ 0, 0 });
			{
				std::vector<std::string> counter;
				counter.push_back(counterName);
				blockParam = parse_block(block, state, &counter, true);
			}

			size_t ip_continue = state->ip;

			if (isAscent) {
				state->AddCode(block, code(command_kind::pc_inline_top_inc));
				//state->AddCode(block, code(command_kind::pc_dup_n, 1));
				//state->AddCode(block, code(command_kind::pc_inline_inc, -1, 0, "!0"));
			}

			state->AddCode(block, code(command_kind::pc_jump, ip));
			size_t ip_back = state->ip;

			//Pop twice for two statements * 2
			state->AddCode(block, code(command_kind::pc_pop, 2));

			if (blockParam.first <= 1U) {	//1 for pc_copy_assign
				while (state->ip > ip_ascdsc_begin)
					state->PopCode(block);
			}
			else {
				block->codes[ip_loopchk].ip = ip_back;
				link_break_continue(block, state, ip_ascdsc_begin, ip_continue, ip_back, ip_continue);

				block->codes[ip_var_format].off = blockParam.second[0];
				block->codes[ip_var_format].len = blockParam.second[1];
			}

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
				parse_block_inlined(block, state);

				if (state->next() == token_kind::tk_ELSE) {
					state->advance();

					//Jump to exit
					state->AddCode(block, code(command_kind::_pc_jump, UINT_MAX));

					mapLabelCode.insert(std::make_pair(indexLabel, state->ip));
					if (state->next() != token_kind::tk_IF) {
						parse_block_inlined(block, state);
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
					|| c->command == command_kind::_pc_jump) 
				{
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
				parse_block_inlined(block, state);

				//Jump to exit
				state->AddCode(block, code(command_kind::_pc_jump, UINT_MAX));

				mapLabelCode.insert(std::make_pair(indexLabel + 1, state->ip));
				indexLabel += 2;
			}
			if (state->next() == token_kind::tk_OTHERS) {
				state->advance();
				parse_block_inlined(block, state);
			}

			size_t ip_end = state->ip;
			mapLabelCode.insert(std::make_pair(UINT_MAX, ip_end));

			state->AddCode(block, code(command_kind::pc_pop, 1));

			//Replace labels with actual code offset
			for (; ip_begin < ip_end; ++ip_begin) {
				code* c = &block->codes[ip_begin];
				if (c->command == command_kind::_pc_jump_if || c->command == command_kind::_pc_jump_if_not
					|| c->command == command_kind::_pc_jump)
				{
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

				state->AddCode(block, code(command_kind::pc_copy_assign, s->level, s->variable,
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
				parser_assert(state, false, error);
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
					parser_assert(state, state->next() == token_kind::tk_close_par, 
						"Only an empty parameter list is allowed here.\r\n");
					state->advance();
				}
			}

			s = search_in(pScope, funcName, args.size());
			parser_assert(state, s->sub->codes.size() == 0,
				"Function body already defined.\r\n");

			{
				frame.push_back(scope_t(s->sub->kind));

				parser_state_t newState(state->lex);

				parser_assert(&newState, newState.next() == token_kind::tk_open_cur, "\"{\" is required.\r\n");
				newState.advance();

				newState.var_count_main = scan_current_scope(&newState, s->sub->level, 
					&args, s->sub->kind == block_kind::bk_function);
				newState.AddCode(s->sub, code(command_kind::pc_var_alloc, 0));

				for (size_t i = 0; i < args.size(); ++i) {
					const std::string& name = args[i];
					symbol* svar = search(name);
					newState.AddCode(s->sub, code(command_kind::pc_copy_assign, svar->level, svar->variable, name));
				}
				parse_statements(s->sub, &newState, token_kind::tk_close_cur, token_kind::tk_semicolon);
				scan_final(s->sub, &newState);

				s->sub->codes[0].ip = newState.var_count_main + newState.var_count_sub;

				parser_assert(&newState, newState.next() == token_kind::tk_close_cur, "\"}\" is required.\r\n");
				newState.advance();

				frame.pop_back();
			}

			need_terminator = false;
			break;
		}
		}

		if (check_terminator) {
			if (need_terminator && state->next() != statement_terminator)
				parser_assert(state, false, "Expected a semicolon (;).");
			while (state->next() == statement_terminator)
				state->advance();
		}
	}
	void parser::parse_statements(script_block* block, parser_state_t* state, 
		token_kind block_terminator, token_kind statement_terminator) 
	{
		for (; state->next() != block_terminator; ) {
			parse_single_statement(block, state, true, statement_terminator);
		}
	}

	std::pair<size_t, std::array<size_t, 2>> parser::parse_block(script_block* block,
		parser_state_t* state, const std::vector<std::string>* args, bool allow_single) 
	{
		size_t prev_size = state->ip;

		parser_state_t newState(state->lex, state->ip);
		newState.state_pred = state;

		size_t varc_prev_total = 0;
		for (parser_state_t* iState = state; iState != nullptr; iState = iState->state_pred)
			varc_prev_total += iState->var_count_main;

		bool single_line = newState.next() != token_kind::tk_open_cur && allow_single;
		if (!single_line) {
			parser_assert(&newState, newState.next() == token_kind::tk_open_cur, "\"{\" is required.\r\n");
			newState.advance();
		}

		frame.push_back(scope_t(block->kind));

		if (!single_line) {
			newState.var_count_main = scan_current_scope(&newState, block->level, 
				args, false, varc_prev_total);
		}
		else if (args) {
			scope_t* ptrBackFrame = &frame.back();
			for (size_t i = 0; i < args->size(); ++i) {
				symbol s = symbol(block->level, nullptr, varc_prev_total + i, false, true);
				ptrBackFrame->singular_insert(args->at(i), s);
			}
			newState.var_count_main = args->size();
		}

		if (args) {
			for (size_t i = 0; i < args->size(); ++i) {
				const std::string& name = args->at(i);
				symbol* s = search(name);
				newState.AddCode(block, code(command_kind::pc_copy_assign, s->level, s->variable, name));
			}
		}
		if (single_line)
			parse_single_statement(block, &newState, true, token_kind::tk_semicolon);
		else
			parse_statements(block, &newState, token_kind::tk_close_cur, token_kind::tk_semicolon);

		frame.pop_back();

		if (!single_line) {
			parser_assert(&newState, newState.next() == token_kind::tk_close_cur, "\"}\" is required.\r\n");
			newState.advance();
		}

		state->ip = newState.ip;
		state->GrowVarCount(newState.var_count_main + newState.var_count_sub);

		return std::make_pair(block->codes.size() - prev_size, 
			std::array<size_t, 2>{ varc_prev_total, newState.var_count_main });
	}
	size_t parser::parse_block_inlined(script_block* block, parser_state_t* state, bool allow_single) {
		size_t ip_var_format = state->ip;
		state->AddCode(block, code(command_kind::pc_var_format, 0, 0));

		auto blockParam = parse_block(block, state, nullptr, allow_single);

		if (blockParam.first == 0U) {
			state->PopCode(block);	//For pc_var_format
		}
		else {
			block->codes[ip_var_format].off = blockParam.second[0];
			block->codes[ip_var_format].len = blockParam.second[1];
		}

		return blockParam.first;
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
			/* Fuses 
			 *   pc_push_value a
			 *   pc_push_value b
			 *   pc_push_value c
			 *   pc_push_value d
			 *   pc_construct_array
			 * into
			 *   pc_push_value [a, b, c, d]
			 */
			case command_kind::pc_construct_array:
			{
				code* ptrBack = &newCodes.back();
				size_t sizeArray = iSrcCode->ip;
				if (newCodes.size() >= sizeArray) {
					type_data* arrayType = nullptr;
					value arrayVal;
					
					if (sizeArray > 0) {
						std::vector<value> listPtrVal;
						listPtrVal.resize(sizeArray);

						code* ptrCode = ptrBack - ((int)sizeArray - 1);
						type_data* type_elem = nullptr;
						{
							if (ptrCode->command != command_kind::pc_push_value)
								goto lab_opt_construct_array_cancel;
							type_elem = ptrCode->data.get_type();
							arrayType = script_type_manager::get_instance()->get_array_type(type_elem);
						}

						for (size_t i = 0; i < sizeArray; ++i) {
							if (ptrCode->command != command_kind::pc_push_value)
								goto lab_opt_construct_array_cancel;
							{
								value appending = ptrCode->data;
								BaseFunction::_append_check(nullptr, arrayType, appending.get_type());
								if (appending.get_type()->get_kind() != type_elem->get_kind()) {
									appending.unique();
									BaseFunction::_value_cast(&appending, type_elem->get_kind());
								}
								listPtrVal[i] = appending;
							}
							++ptrCode;
						}

						arrayVal.set(arrayType, listPtrVal);
					}
					else {
						arrayVal = value(script_type_manager::get_null_array_type(), 0i64);
					}

					for (size_t i = 0; i < sizeArray; ++i) {
						newCodes.pop_back();
						--(state->ip);
					}
					newCodes.push_back(code(iSrcCode->line, command_kind::pc_push_value, arrayVal));
				}
				else {
lab_opt_construct_array_cancel:
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
		if (mapLabelCode.size() == 0U) return;

		for (auto itr = block->codes.begin(); itr != block->codes.end(); ++itr) {
			switch (itr->command) {
			case command_kind::pc_jump_target:
				//--(state->ip);
				break;
			case command_kind::_pc_jump:
			case command_kind::_pc_jump_if:
			case command_kind::_pc_jump_if_not:
			case command_kind::_pc_jump_if_nopop:
			case command_kind::_pc_jump_if_not_nopop:
			{
				auto itrFind = mapLabelCode.find(itr->ip);
				if (itrFind != mapLabelCode.end())
					newCodes.push_back(code(itr->line, get_replacing_jump(itr->command), itrFind->second + ip_off));
				break;
			}
			default:
				newCodes.push_back(*itr);
				break;
			}
		}

		block->codes = newCodes;
	}
	///Replaces breaks and continues with jumps
	void parser::link_break_continue(script_block* block, parser_state_t* state,
		size_t ip_begin, size_t ip_end, size_t ip_break, size_t ip_continue) 
	{
		auto itrBegin = block->codes.begin() + ip_begin;
		auto itrEnd = block->codes.begin() + ip_end + 1;
		for (auto itr = itrBegin; itr != itrEnd; ++itr) {
			if (itr == block->codes.end()) return;
			switch (itr->command) {
			case command_kind::pc_break_loop:
				*itr = code(itr->line, command_kind::pc_jump, ip_break);
				break;
			case command_kind::pc_loop_continue:
				*itr = code(itr->line, command_kind::pc_jump, ip_continue);
				break;
			}
		}
	}
	void parser::scan_final(script_block* block, parser_state_t* state) {
		for (auto itr = block->codes.begin(); itr != block->codes.end(); ++itr) {
			parser_assert(itr->line, itr->command != command_kind::pc_break_loop,
				"\"break\" may only be used inside a loop.");
			parser_assert(itr->line, itr->command != command_kind::pc_loop_continue,
				"\"continue\" may only be used inside a loop.");
		}
	}
}
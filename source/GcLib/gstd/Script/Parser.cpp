#include "source/GcLib/pch.h"

#include "Parser.hpp"
#include "ScriptFunction.hpp"
#include "Script.hpp"

//Natashi's TODO: Implement a parse tree

using namespace gstd;

script_block::script_block(uint32_t the_level, block_kind the_kind) {
	level = the_level;
	arguments = 0;
	func = nullptr;
	kind = the_kind;
}

#pragma push_macro("new")
#undef new
code::code() {
	line = 0;
}
code::code(command_kind _command) : code() {
	command = _command;
}
code::code(command_kind _command, uint32_t _a0) : code(_command) {
	arg0 = _a0;
}
code::code(command_kind _command, uint32_t _a0, uint32_t _a1) : code(_command, _a0) {
	arg1 = _a1;
}
code::code(command_kind _command, uint32_t _a0, uint32_t _a1, uint32_t _a2) : code(_command, _a0, _a1) {
	arg2 = _a2;
}
code::code(command_kind _command, uint32_t _a0, const std::string& _name) : code(_command, _a0) 	{
#ifdef _DEBUG
	var_name = _name;
#endif
}
code::code(command_kind _command, uint32_t _a0, uint32_t _a1, const std::string& _name)
	: code(_command, _a0, _name) 	{
	arg1 = _a1;
}
code::code(command_kind _command, uint32_t _a0, uint32_t _a1, uint32_t _a2, const std::string& _name)
	: code(_command, _a0, _a1, _name) 	{
	arg2 = _a2;
}
code::code(command_kind _command, const value& _data) : code(_command) {
	new (&data) value(_data);	//Reassign data without calling its destructor
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
		arg0 = src.arg0;
		arg1 = src.arg1;
		arg2 = src.arg2;
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

//=========================================================================

parser::symbol::symbol() : symbol(0, nullptr) {
}
parser::symbol::symbol(uint32_t lv, type_data* type_) {
	level = lv;
	type = type_;
}
parser::symbol::symbol(uint32_t lv, type_data* type_, bool overl, script_block* pSub) 
	: symbol(lv, type_) 
{
	bVariable = false;
	bAllowOverload = overl;
	sub = pSub;
}
parser::symbol::symbol(uint32_t lv, type_data* type_, bool overl, script_block* pSub, const std::vector<arg_data>& argData_)
	: symbol(lv, type_, overl, pSub)
{
	argData = argData_;
}
parser::symbol::symbol(uint32_t lv, type_data* type_, uint32_t var_, bool bConst_) 
	: symbol(lv, type_) 
{
	bVariable = true;
	var = var_;
	bConst = bConst_;
	bAssigned = false;
}

//=========================================================================

static const std::vector<function> base_operations = {
	{ "not", BaseFunction::not_, 1 },
	{ "negative", BaseFunction::negative, 1 },
	{ "predecessor", BaseFunction::predecessor, 1 },
	{ "successor", BaseFunction::successor, 1 },

	{ "round", BaseFunction::round, 1 },
	{ "round_base", BaseFunction::round_base, 2 },
	{ "trunc", BaseFunction::truncate, 1 },
	{ "truncate", BaseFunction::truncate, 1 },
	{ "ceil", BaseFunction::ceil, 1 },
	{ "ceil_base", BaseFunction::ceil_base, 2 },
	{ "floor", BaseFunction::floor, 1 },
	{ "floor_base", BaseFunction::floor_base, 2 },
	//{ "abs", BaseFunction::absolute, 1 },
	{ "absolute", BaseFunction::absolute, 1 },

	{ "add", BaseFunction::add, 2 },
	{ "subtract", BaseFunction::subtract, 2 },
	{ "multiply", BaseFunction::multiply, 2 },
	{ "divide", BaseFunction::divide, 2 },
	{ "remainder", BaseFunction::remainder_, 2 },
	{ "modc", BaseFunction::modc, 2 },
	{ "apo", BaseFunction::apo, 1 },
	{ "power", BaseFunction::power, 2 },

	{ "as_int_array", BaseFunction::cast_int_array, 1 },
	{ "as_real_array", BaseFunction::cast_real_array, 1 },
	{ "as_bool_array", BaseFunction::cast_bool_array, 1 },
	{ "as_char_array", BaseFunction::cast_char_array, 1 },
	{ "as_x_array", BaseFunction::cast_x_array, 2 },

	{ "generate", BaseFunction::generate, 2 },

	//{ "length", BaseFunction::length, 1 },
	{ "resize", BaseFunction::resize, 2 },
	{ "resize", BaseFunction::resize, 3 },	//Overloaded

	{ "reverse", BaseFunction::reverse, 1 },
	{ "sort", BaseFunction::sort, 1 },
	{ "sort", BaseFunction::sort, 2 }, //Overloaded

	{ "range", BaseFunction::range, 1 },	
	{ "range", BaseFunction::range, 2 },	//Overloaded
	{ "range", BaseFunction::range, 3 },	//Overloaded x2

	{ "contains", BaseFunction::contains, 2 },
	{ "indexof", BaseFunction::indexof, 2 },
	{ "indexof", BaseFunction::indexof, 3 },	//Overloaded
	{ "matches", BaseFunction::matches, 2 },
	{ "all", BaseFunction::all, 1 },
	{ "any", BaseFunction::any, 1 },

	{ "replace", BaseFunction::replace, 3 },
	{ "remove", BaseFunction::remove, -3 }, //1 fixed + ... -> 2 minimum

	{ "slice", BaseFunction::slice, 3 },
	//{ "slice", BaseFunction::slice, 4 },	//Overloaded
	{ "insert", BaseFunction::insert, 3 },
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

	{ "typeof", BaseFunction::typeOf, 1 },
	{ "ftypeof", BaseFunction::typeOfElem, 1 },

	{ "assert", BaseFunction::assert_, 2 },
	{ "__DEBUG_BREAK", BaseFunction::script_debugBreak, 0 },
};

parser::symbol* parser::scope_t::singular_insert(const std::string& name, const symbol& s, int argc) {
	bool exists = this->find(name) != this->end();
	auto itr = this->equal_range(name);

	//Uh oh! Duplication!
	if (exists) {
		symbol* sPrev = &(itr.first->second);

		//Check if the symbol can be overloaded
		if (!sPrev->bAllowOverload && sPrev->level > 0) {
			std::string error = "";
			if (!sPrev->bVariable) {	//Scripter attempted to overload a default function
				error = StringUtility::Format("\"%s\": Function cannot be overloaded.",
					name.c_str());
			}
			else {						//Scripter duplicated parameter/variable name
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

	return &(this->insert(std::make_pair(name, s))->second);
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
		engine->main_block->codes.push_back(code(command_kind::pc_call, (uint32_t)block_const_reg, 0));
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
			const_value.reset(script_type_manager::get_int_type(), (int64_t&)pConst->data);
			break;
		case type_data::tk_real:
			const_value.reset(script_type_manager::get_real_type(), (double&)pConst->data);
			break;
		case type_data::tk_char:
			const_value.reset(script_type_manager::get_char_type(), (wchar_t&)pConst->data);
			break;
		case type_data::tk_boolean:
			const_value.reset(script_type_manager::get_boolean_type(), (bool&)pConst->data);
			break;
		default:
			continue;
		}
		block_const_reg->codes.push_back(code(command_kind::pc_push_value, const_value));
		block_const_reg->codes.push_back(code(command_kind::pc_copy_assign, 1, iConst, pConst->name));

		symbol s = symbol(1, nullptr, iConst, true);
		s.bAssigned = true;
		frame.begin()->singular_insert(pConst->name, s);
	}
	count_base_constants += list_const->size();
}
void parser::_parser_assert_end(parser_state_t* state) {
	parser_assert(state->next() == token_kind::tk_end,
		"Unexpected end-of-file.\r\n");
}
void parser::_parser_assert_nend(parser_state_t* state) {
	parser_assert(state->next() != token_kind::tk_end,
		"Unexpected end-of-file while parsing.\r\n");
}
void parser::begin_parse() {
	try {
		frame.push_back(scope_t(block_kind::bk_normal));	//Scope for user symbols

		parser_state_t stateParser(lexer_main);
		stateParser.ip = engine->main_block->codes.size();

		stateParser.var_count_main = scan_current_scope(&stateParser, 1, count_base_constants, nullptr);
		stateParser.var_count_main += count_base_constants;

		parse_statements(engine->main_block, &stateParser, token_kind::tk_end, token_kind::tk_semicolon);
		scan_final(engine->main_block, &stateParser);

		//for pc_var_alloc
		engine->main_block->codes[0].arg0 = count_base_constants + stateParser.var_count_main + stateParser.var_count_sub;

		_parser_assert_end(&stateParser);
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
	block->arguments = func.argc;
	block->name = func.name;
	block->func = func.func;
	symbol s = symbol(0, nullptr, false, block);
	frame.begin()->singular_insert(func.name, s, func.argc);
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
				if (!s->bVariable) {
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
			if (!s->bVariable) {
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

bool parser::_parser_skip_post_decl(parser_state_t* state, int* pBrk, int* pPar) {
	while (state->next() != token_kind::tk_semicolon) {
		if (state->next() == token_kind::tk_open_cur) break;

		if (state->next() == token_kind::tk_open_bra) ++(*pBrk);
		else if (state->next() == token_kind::tk_close_bra) --(*pBrk);

		if (state->next() == token_kind::tk_open_par) ++(*pPar);
		else if (state->next() == token_kind::tk_close_par) --(*pPar);

		if (state->next() == token_kind::tk_comma
			&& (*pBrk) == 0 && (*pPar) == 0) break;

		state->advance();
	}
	if (state->next() == token_kind::tk_semicolon) {
		state->advance();
		return false;
	}
	else if (state->next() == token_kind::tk_comma) {
		state->advance();
	}
	else {
		parser_assert(state, false, "Invalid statement.");
	}
	return true;
}

type_data* _token_kind_to_type_data(token_kind token) {
	switch (token) {
	case token_kind::tk_decl_int:
		return script_type_manager::get_int_type();
	case token_kind::tk_decl_real:
		return script_type_manager::get_real_type();
	case token_kind::tk_decl_char:
		return script_type_manager::get_char_type();
	case token_kind::tk_decl_bool:
		return script_type_manager::get_boolean_type();
	case token_kind::tk_decl_string:
		return script_type_manager::get_string_type();
	}
	return script_type_manager::get_null_type();
}

parser::arg_data parser::parse_variable_decl(parser_state_t* state, bool bParameter) {
	arg_data var;

	parse_type_decl(state, &var);
	if (var.type) {
		parser_assert(state, var.type->get_kind() != type_data::tk_null,
			"A void type cannot be used to declare a variable.\r\n");
	}

	//parser_assert(state, state->next() == token_kind::tk_word, "Variable name is required.\r\n");
	if (state->next() != token_kind::tk_word) {
		const wchar_t* prevPtr = *std::next(state->lex->prev_ptr_list.begin());
		std::wstring tokStr = state->lex->ToStr(prevPtr, state->lex->get());
		tokStr = StringUtility::Trim(tokStr);
		std::wstring err = StringUtility::Format(
			L"\"%s\" cannot be used to declare a %s name.\r\n",
			tokStr.c_str(), bParameter ? L"parameter" : L"variable");
		parser_assert(state, false, err);
	}
	var.name = state->lex->word;

	state->advance();

	return var;
}
void parser::parse_type_decl(parser_state_t* state, arg_data* pArg) {
	token_kind typeBaseTok = token_kind::tk_decl_auto;
	size_t arrayCount = 0;
	bool bSpecifiedType = false;

	while (IsDeclToken(state->next())) {
		if (state->next() == token_kind::tk_decl_mod_const) {
			pArg->bConst = true;

			state->advance();
		}
		else if (!bSpecifiedType) {
			bSpecifiedType = true;
			typeBaseTok = state->next();

			state->advance();

			if (state->next() == token_kind::tk_open_bra) {
				parser_assert(state, typeBaseTok != token_kind::tk_decl_auto,
					"An array type declaration cannot be typeless.\r\n");
				while (state->next() == token_kind::tk_open_bra) {
					state->advance();
					parser_assert(state, state->next() == token_kind::tk_close_bra,
						"\"]\" is required.\r\n");
					state->advance();
					++arrayCount;
				}
			}
		}
		else {
			parser_assert(state, false, "Type already specified.\r\n");
		}
	}

	if (typeBaseTok == token_kind::tk_decl_auto)
		pArg->type = nullptr;
	else {
		type_data* typeBase = _token_kind_to_type_data(typeBaseTok);
		if (arrayCount == 0) {
			pArg->type = typeBase;
		}
		else {
			script_type_manager* typeManager = script_type_manager::get_instance();

			pArg->type = typeManager->get_array_type(typeBase);

			if (arrayCount > 1) {
				for (size_t i = 1; i < arrayCount; ++i)
					pArg->type = typeManager->get_array_type(pArg->type);
			}
		}
	}
}

int parser::scan_current_scope(parser_state_t* state, int level, int initVar, const std::vector<arg_data>* args) {
	//Scans and defines scope variables and functions/tasks/subs

	script_scanner lex2(*state->lex);

	auto IsReadableToken = [&]() -> bool {
		return lex2.next != token_kind::tk_end && lex2.next != token_kind::tk_invalid;
	};

	int var = initVar;

	try {
		scope_t* current_frame = &frame.back();
		int brk = 0;
		int cur = 0;
		int par = 0;

		if (args) {
			for (size_t i = 0; i < args->size(); ++i) {
				const arg_data* arg = &args->at(i);
				symbol s = symbol(level, arg->type, var++, arg->bConst);
				current_frame->singular_insert(arg->name, s);
			}
		}

		while (cur >= 0 && IsReadableToken()) {
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
				if (cur == 0) {
					brk = 0;
					par = 0;
				}
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

					type_data* funcReturnType = nullptr;
					if (lex2.next == token_kind::tk_l) {	//<
						parser_assert(kind == block_kind::bk_function,
							"Only functions may have a value type specifier.\r\n");

						lex2.advance();

						arg_data tmp;

						parser_state_t dummyState = parser_state_t(&lex2);
						parse_type_decl(&dummyState, &tmp);

						//parser_assert(&dummyState, !tmp.bConst, "Function return cannot be const.\r\n");

						//>
						parser_assert(&dummyState, lex2.next == token_kind::tk_g, 
							"\">\" is required.\r\n");
						lex2.advance();

						funcReturnType = tmp.type;
					}

					std::string name = lex2.word;
					std::vector<arg_data> argData;

					lex2.advance();
					if (lex2.next == token_kind::tk_open_par) {
						lex2.advance();

						//I honestly should just throw out support for subs
						if (kind == block_kind::bk_sub && lex2.next != token_kind::tk_close_par) {
							parser_assert(state, false, "A parameter list in not allowed here.\r\n");
						}
						else {
							parser_state_t dummyState = parser_state_t(&lex2);
							while (lex2.next == token_kind::tk_word || IsDeclToken(lex2.next)) {
								//TODO: Give scripters the ability to define vararg functions

								arg_data nArg = parse_variable_decl(&dummyState, true);
								argData.push_back(nArg);

								if (lex2.next != token_kind::tk_comma)
									break;
								lex2.advance();
							}
							parser_assert(&dummyState, lex2.next == token_kind::tk_close_par, "\")\" is required.\r\n");
							lex2.advance();
						}
					}

					size_t countArgs = argData.size();
					{
						//First, search for duplications in the default symbol level
						symbol* dup = search_in(&*frame.begin(), name);

						//Default symbol isn't being redefined/overloaded, check user-defined symbols in the current scope
						if (dup == nullptr)
							dup = search_in(current_frame, name, countArgs);

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

							std::string error;
							if (dup->bAllowOverload && countArgs >= 0) {
								error = "A " + typeSub;
								error += StringUtility::Format(" of the same name was already defined "
									"in the current scope: \'%s\'\r\n", name.c_str());
								error += "**You may overload functions and tasks by giving them "
									"different argument counts.\r\n";
							}
							else {
								error += StringUtility::Format("\'%s\' already exists as a default function "
									"and cannot be overloaded.\r\n", name.c_str());
							}
							parser_assert(false, error);
						}
					}

					script_block* newBlock = engine->new_block(level + 1, kind);
					newBlock->name = name;
					newBlock->func = nullptr;
					newBlock->arguments = countArgs;
					symbol s = symbol(level, funcReturnType, kind != block_kind::bk_sub, newBlock, argData);
					current_frame->singular_insert(name, s, countArgs);
				}
			}
			break;

			case token_kind::tk_decl_mod_const:
			case token_kind::tk_decl_real:
			case token_kind::tk_decl_int:
			case token_kind::tk_decl_char:
			case token_kind::tk_decl_bool:
			case token_kind::tk_decl_string:
			case token_kind::tk_decl_auto:
			{
				if (cur == 0 && par == 0) {
					parser_state_t dummyState = parser_state_t(&lex2);
					arg_data firstArg = arg_data();

					while (true) {
						arg_data nArg = parse_variable_decl(&dummyState, false);
						if (firstArg.name.size() == 0)
							firstArg = nArg;

						nArg.type = firstArg.type;
						nArg.bConst = firstArg.bConst;

						{
							{
								//First, search for duplications in the default symbol level
								symbol* dup = search_in(&*frame.begin(), nArg.name);

								//Default symbol isn't being redefined/overloaded, check user-defined symbols in the current scope
								if (dup == nullptr)
									dup = search_in(current_frame, nArg.name);

								//Allows variables to overshadow functions
								if (dup && dup->bVariable) {
									std::string error = StringUtility::Format("\'%s\' was already declared "
										"in the same scope.\r\n", nArg.name.c_str());
									parser_assert(false, error);
								}
							}

							symbol s = symbol(level, nArg.type, var++, nArg.bConst);
							current_frame->singular_insert(nArg.name, s);
						}

						brk = 0;
						if (!_parser_skip_post_decl(&dummyState, &brk, &par))
							break;
					}

					brk = 0;
					cur = 0;
					par = 0;
				}
				else {
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
	catch (parser_error& e) {
		//state->lex->line = lex2.line;
		throw parser_error_mapped(lex2.line, e.GetMessageW());
	}

	return (var - initVar);
}

void parser::write_operation(script_block* block, parser_state_t* state, const char* name, int clauses) {
	symbol* s = search(name, clauses);
	parser_assert(state, !s->bVariable, StringUtility::Format("write_operation: \"%s\" is not callable", name));
	{
		std::string error = StringUtility::Format(
			"Invalid argument count for the default function \"%s\". (argc=%d)",
			name, clauses);
		parser_assert(state, s != nullptr, error);
	}
	state->AddCode(block, code(command_kind::pc_call_and_push_result, (uint32_t)s->sub, clauses));
}
void parser::write_operation(script_block* block, parser_state_t* state, const symbol* s, int clauses) {
	parser_assert(state, s != nullptr, "write_operation: symbol is null");
	parser_assert(state, !s->bVariable, "write_operation: symbol is not callable");
	{
		std::string error = StringUtility::Format(
			"Invalid argument count for the default function \"%s\". (argc=%d, expected=%d)",
			s->sub->name.c_str(), clauses, s->sub->arguments);
		parser_assert(state, s->sub->arguments == clauses, error);
	}
	state->AddCode(block, code(command_kind::pc_call_and_push_result, (uint32_t)s->sub, clauses));
}

void parser::parse_parentheses(script_block* block, parser_state_t* state) {
	parser_assert(state, state->next() == token_kind::tk_open_par, "\"(\" is required.\r\n");
	state->advance();

	parse_expression(block, state);

	parser_assert(state, state->next() == token_kind::tk_close_par, "\")\" is required.\r\n");
	state->advance();
}

void parser::parse_clause(script_block* block, parser_state_t* state) {
	//_parser_assert_nend(state);
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

#ifdef _DEBUG
		if (name == "__PARSER_BREAK_")
			DebugBreak();
#endif

		script_scanner lex_tmp(*state->lex);
		parser_state_t dummyState(&lex_tmp, state->ip);
		script_block dummyBlock(0, block_kind::bk_normal);

		int argc = parse_arguments(&dummyBlock, &dummyState, nullptr);
		symbol* s = search(name, argc);

		if (s == nullptr) {
			if (s = search(name)) {
				if (!s->bVariable && test_variadic(s->sub->arguments, argc))
					goto continue_as_variadic;
			}

			std::string error;
			if (s == nullptr)
				error = StringUtility::Format("%s is not defined.\r\n", name.c_str());
			else if (s->bVariable)
				error = StringUtility::Format("%s is not callable.\r\n", name.c_str());
			//else if (search(name))
			else
				error = StringUtility::Format("No overload of %s takes %d arguments.\r\n",
					name.c_str(), argc);

			parser_assert(state, false, error);
		}

continue_as_variadic:
		if (!s->bVariable) {
			parse_arguments(block, state, &s->argData);
			parser_assert(state, s->sub->kind == block_kind::bk_function,
				"Tasks and subs cannot return values.\r\n");
			state->AddCode(block, code(command_kind::pc_call_and_push_result, (uint32_t)s->sub, argc));
		}
		else {
			//Variable
			state->AddCode(block, code(command_kind::pc_push_variable, s->level, s->var, name));
		}

		return;
	}
	case token_kind::tk_cast_int:
	case token_kind::tk_cast_real:
	case token_kind::tk_cast_char:
	case token_kind::tk_cast_bool:
	case token_kind::tk_cast_string:
	{
		type_data* target = nullptr;
		switch (state->next()) {
		case token_kind::tk_cast_int:
			target = script_type_manager::get_int_type();
			break;
		case token_kind::tk_cast_real:
			target = script_type_manager::get_real_type();
			break;
		case token_kind::tk_cast_char:
			target = script_type_manager::get_char_type();
			break;
		case token_kind::tk_cast_bool:
			target = script_type_manager::get_boolean_type();
			break;
		case token_kind::tk_cast_string:
			target = script_type_manager::get_string_type();
			break;
		}
		state->advance();
		parse_parentheses(block, state);
		state->AddCode(block, code(command_kind::pc_inline_cast_var, (uint32_t)target, false));
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
				BaseFunction::_create_empty(script_type_manager::get_null_array_type())));

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

size_t parser::_parse_array_suffix_lvalue(script_block* block, parser_state_t* state) {
	size_t indexcount = 0;
	while (state->next() == token_kind::tk_open_bra) {
		++indexcount;

		state->advance();
		parse_expression(block, state);
		parser_assert(state, state->next() != token_kind::tk_range,
			"Array slice operation is not allowed here.\r\n");

		state->AddCode(block, code(command_kind::pc_inline_index_array));

		parser_assert(state, state->next() == token_kind::tk_close_bra, "\"]\" is required.\r\n");
		state->advance();
	}
	return indexcount;
}
void parser::_parse_array_suffix_rvalue(script_block* block, parser_state_t* state) {
	while (state->next() == token_kind::tk_open_bra) {
		state->advance();
		parse_ternary(block, state);

		if (state->next() == token_kind::tk_range) {
			state->advance();
			parse_ternary(block, state);
			write_operation(block, state, "slice", 3);
		}
		else {
			state->AddCode(block, code(command_kind::pc_inline_index_array2));
		}

		parser_assert(state, state->next() == token_kind::tk_close_bra, "\"]\" is required.\r\n");
		state->advance();
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
		_parse_array_suffix_rvalue(block, state);
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
		|| state->next() == token_kind::tk_f_slash || state->next() == token_kind::tk_percent)
	{
		command_kind f = command_kind::pc_inline_mod;	//tk_percent
		switch (state->next()) {
		case token_kind::tk_asterisk: f = command_kind::pc_inline_mul; break;
		case token_kind::tk_slash: f = command_kind::pc_inline_div; break;
		case token_kind::tk_f_slash: f = command_kind::pc_inline_fdiv; break;
		}
		state->advance();
		parse_prefix(block, state);
		state->AddCode(block, code(f));
	}
}

void parser::parse_sum(script_block* block, parser_state_t* state) {
	parse_product(block, state);
	while (state->next() == token_kind::tk_tilde || state->next() == token_kind::tk_plus
		|| state->next() == token_kind::tk_minus)
	{
		command_kind f = command_kind::pc_inline_sub;	//tk_minus
		switch (state->next()) {
		case token_kind::tk_tilde: f = command_kind::pc_inline_cat; break;
		case token_kind::tk_plus: f = command_kind::pc_inline_add; break;
		}
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

	bool hasExpr = false;

	size_t base_hash = ((size_t)block ^ 0x45f60e21u) + ((size_t)state ^ 0xce189a9bu) + state->ip * 0x56d3;
	size_t iter = 0x80000000 + (std::_Hash_array_representation((byte*)&base_hash, 4U) & 0x04ffffff);
	while (state->next() == token_kind::tk_logic_and || state->next() == token_kind::tk_logic_or) {
		hasExpr = true;

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

	if (hasExpr) {
		state->AddCode(block, code(command_kind::pc_inline_cast_var,
			(uint32_t)script_type_manager::get_boolean_type(), false));
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

		for (auto& i : tmp.codes)
			block->codes.push_back(i);
	}
	catch (std::string& err) {
		parser_assert(state, false, err);
	}
	catch (std::wstring& err) {
		parser_assert(state, false, err);
	}
}

//Format for variadic arguments:
// argc = -(n + 1)
// where n = fixed(required) arguments
int parser::parse_arguments(script_block* block, parser_state_t* state, const std::vector<arg_data>* argsData) {
	int argc = 0;
	if (state->next() == token_kind::tk_open_par) {
		state->advance();

		while (state->next() != token_kind::tk_close_par) {
			parse_expression(block, state);
			if (argsData && argsData->size() > 0) {
				const arg_data* arg = &argsData->at(argc);
				if (arg->type != nullptr)
					state->AddCode(block, code(command_kind::pc_inline_cast_var, (uint32_t)arg->type, true));
			}
			++argc;
			if (state->next() != token_kind::tk_comma) break;
			state->advance();
		}

		parser_assert(state, state->next() == token_kind::tk_close_par, "\")\" is required.\r\n");
		state->advance();
	}
	return argc;
}

void parser::parse_single_statement(script_block* block, parser_state_t* state,
	bool check_terminator, token_kind statement_terminator)
{
	auto assert_const = [&](symbol* s, const std::string& name) {
		if (s->bConst) {
			std::string error = StringUtility::Format("\"%s\": ", name.c_str());
			if (!s->bVariable)
				parser_assert(state, false, error + "Functions, tasks, and subs are implicitly const\r\n"
					"and thus cannot be modified in this manner.\r\n");
			else
				parser_assert(state, false, error + "const variables cannot be modified.\r\n");
		}
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

		bool isArrayElement = false;
		size_t arrayIndexCount = 0;
		if (s->bVariable) {
			if (state->next() == token_kind::tk_open_bra) {
				isArrayElement = true;
				state->AddCode(block, code(command_kind::pc_push_variable2, s->level, s->var, name));
				arrayIndexCount = _parse_array_suffix_lvalue(block, state);
			}
		}

		switch (state->next()) {
		case token_kind::tk_assign:
			assert_const(s, name);
			state->advance();
			parse_expression(block, state);

			s->bAssigned = true;

			if (type_data* cvtType = s->type) {
				for (size_t i = 0; i < arrayIndexCount; ++i) {
					cvtType = cvtType->get_element();
					if (cvtType == nullptr) break;
				}
				if (cvtType)
					state->AddCode(block, code(command_kind::pc_inline_cast_var, (uint32_t)cvtType, true));
			}

			if (isArrayElement)
				state->AddCode(block, code(command_kind::pc_ref_assign));
			else 
				state->AddCode(block, code(command_kind::pc_copy_assign, s->level, s->var, name));
			break;
		case token_kind::tk_add_assign:
		case token_kind::tk_subtract_assign:
		case token_kind::tk_multiply_assign:
		case token_kind::tk_divide_assign:
		case token_kind::tk_fdivide_assign:
		case token_kind::tk_remainder_assign:
		case token_kind::tk_power_assign:
		case token_kind::tk_concat_assign:
		{
			assert_const(s, name);
			command_kind f = command_kind::pc_nop;
#define DEF_CASE(_tk, _pk) case _tk: f = _pk; break;
			switch (state->next()) {
				DEF_CASE(token_kind::tk_add_assign, command_kind::pc_inline_add_asi);
				DEF_CASE(token_kind::tk_subtract_assign, command_kind::pc_inline_sub_asi);
				DEF_CASE(token_kind::tk_multiply_assign, command_kind::pc_inline_mul_asi);
				DEF_CASE(token_kind::tk_divide_assign, command_kind::pc_inline_div_asi);
				DEF_CASE(token_kind::tk_fdivide_assign, command_kind::pc_inline_fdiv_asi);
				DEF_CASE(token_kind::tk_remainder_assign, command_kind::pc_inline_mod_asi);
				DEF_CASE(token_kind::tk_power_assign, command_kind::pc_inline_pow_asi);
				DEF_CASE(token_kind::tk_concat_assign, command_kind::pc_inline_cat_asi);
			}
#undef DEF_CASE
			state->advance();
			parse_expression(block, state);
			if (isArrayElement)
				state->AddCode(block, code(f, false, name));
			else
				state->AddCode(block, code(f, true, s->level, s->var, name));
			break;
		}
		case token_kind::tk_inc:
		case token_kind::tk_dec:
		{
			assert_const(s, name);
			command_kind f = (state->next() == token_kind::tk_inc) ? command_kind::pc_inline_inc
				: command_kind::pc_inline_dec;
			state->advance();
			if (isArrayElement)
				state->AddCode(block, code(f, false, true, name));
			else
				state->AddCode(block, code(f, true, s->level, s->var, name));
			break;
		}
		default:
		{
			parser_assert(state, !s->bVariable, 
				"A variable cannot be called as if it was a function, a task, or a sub.\r\n");

			script_scanner lex_tmp(*state->lex);
			parser_state_t dummyState(&lex_tmp, state->ip);
			script_block dummyBlock(0, block_kind::bk_normal);

			int argc = parse_arguments(&dummyBlock, &dummyState, nullptr);

			if (!test_variadic(s->sub->arguments, argc)) {
				s = search_in(resScope, name, argc);
				parser_assert(state, s, StringUtility::Format("No overload of %s takes %d arguments.\r\n",
					name.c_str(), argc));
			}

			parse_arguments(block, state, &s->argData);
			state->AddCode(block, code(command_kind::pc_call, (uint32_t)s->sub, argc));

			break;
		}
		}

		break;
	}
	case token_kind::tk_decl_real:
	case token_kind::tk_decl_int:
	case token_kind::tk_decl_char:
	case token_kind::tk_decl_bool:
	case token_kind::tk_decl_string:
	case token_kind::tk_decl_auto:
	case token_kind::tk_decl_mod_const:
	{
		while (state->next() != token_kind::tk_word) state->advance();
		parser_assert(state, state->next() == token_kind::tk_word, "Variable name is required.\r\n");

		while (true) {
			std::string name = state->lex->word;
			state->advance();

			if (state->next() == token_kind::tk_assign) {
				symbol* s = search(name);
				s->bAssigned = true;

				state->advance();

				parse_expression(block, state);
				if (s->type != nullptr) {
					state->AddCode(block, code(command_kind::pc_inline_cast_var, (uint32_t)s->type, true));
				}

				state->AddCode(block, code(command_kind::pc_copy_assign,
					s->level, s->var, name));
			}

			if (state->next() == token_kind::tk_semicolon) break;
			else if (state->next() == token_kind::tk_comma)
				state->advance();
			else
				parser_assert(state, false, "Invalid statement.\r\n");
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
				state->AddCode(block, code(command_kind::pc_inline_cast_var, 
					(uint32_t)script_type_manager::get_int_type(), false));

				size_t ip_var_format = state->ip;
				state->AddCode(block, code(command_kind::pc_var_format, 0U, 0));

				size_t ip_loop_begin = state->ip;
				state->AddCode(block, code(command_kind::pc_loop_count));
				size_t ip_loopchk = state->ip;
				state->AddCode(block, code(command_kind::pc_jump_if_not, 0U));

				size_t ip_block_begin = state->ip;
				block_return_t blockParam = parse_block(block, state, nullptr, true);

				size_t ip_continue = state->ip;
				state->AddCode(block, code(command_kind::pc_jump, ip_loop_begin));
				size_t ip_back = state->ip;
				state->AddCode(block, code(command_kind::pc_pop, 1));

				//Try to optimize looped yield to a wait
				if (blockParam.size == 1U && block->codes[ip_block_begin].command == command_kind::pc_yield) {
					while (state->ip > ip_loop_begin)
						state->PopCode(block);
					state->AddCode(block, code(command_kind::pc_wait));
				}
				//Discard everything if the loop is empty
				else if (blockParam.size == 0U) {
					while (state->ip > ip_loop_begin)
						state->PopCode(block);
				}
				else {
					block->codes[ip_loopchk].arg0 = ip_back;
					link_break_continue(block, state, ip_block_begin, ip_continue, ip_back, ip_continue);

					block->codes[ip_var_format].arg0 = blockParam.alloc_base;
					block->codes[ip_var_format].arg1 = blockParam.alloc_size;
				}
			}
		}
		else {
			size_t ip_var_format = state->ip;
			state->AddCode(block, code(command_kind::pc_var_format, 0U, 0));

			block_return_t blockParam = parse_block(block, state, nullptr, true);

			size_t ip_continue = state->ip;
			state->AddCode(block, code(command_kind::pc_jump, ip_begin));
			size_t ip_back = state->ip;

			if (blockParam.size == 0U) {
				while (state->ip > ip_begin)
					state->PopCode(block);
			}
			else {
				link_break_continue(block, state, ip_begin, ip_continue, ip_back, ip_continue);

				block->codes[ip_var_format].arg0 = blockParam.alloc_base;
				block->codes[ip_var_format].arg1 = blockParam.alloc_size;
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
			state->AddCode(block, code(command_kind::pc_inline_cast_var, 
				(uint32_t)script_type_manager::get_int_type(), false));

			size_t ip_var_format = state->ip;
			state->AddCode(block, code(command_kind::pc_var_format, 0U, 0));

			size_t ip_loop_begin = state->ip;
			state->AddCode(block, code(command_kind::pc_loop_count));
			size_t ip_loopchk = state->ip;
			state->AddCode(block, code(command_kind::pc_jump_if_not, 0U));

			size_t ip_block_begin = state->ip;
			block_return_t blockParam = parse_block(block, state, nullptr, true);

			size_t ip_continue = state->ip;
			state->AddCode(block, code(command_kind::pc_jump, ip_loop_begin));
			size_t ip_back = state->ip;
			state->AddCode(block, code(command_kind::pc_pop, 1));

			//Try to optimize looped yield to a wait
			if (blockParam.size == 1U && block->codes[ip_block_begin].command == command_kind::pc_yield) {
				while (state->ip > ip_loop_begin)
					state->PopCode(block);
				state->AddCode(block, code(command_kind::pc_wait));
			}
			//Discard everything if the loop is empty
			else if (blockParam.size == 0U) {
				while (state->ip > ip_loop_begin)
					state->PopCode(block);
			}
			else {
				block->codes[ip_loopchk].arg0 = ip_back;
				link_break_continue(block, state, ip_block_begin, ip_continue, ip_back, ip_continue);

				block->codes[ip_var_format].arg0 = blockParam.alloc_base;
				block->codes[ip_var_format].arg1 = blockParam.alloc_size;
			}
		}

		need_terminator = false;
		break;
	}
	case token_kind::tk_WHILE:
	{
		state->advance();

		size_t ip_var_format = state->ip;
		state->AddCode(block, code(command_kind::pc_var_format, 0U, 0));

		size_t ip = state->ip;

		parse_parentheses(block, state);
		if (state->next() == token_kind::tk_LOOP)
			state->advance();

		size_t ip_loopchk = state->ip;
		state->AddCode(block, code(command_kind::pc_jump_if_not, 0U));

		size_t ip_block_begin = state->ip;
		block_return_t blockParam = parse_block(block, state, nullptr, true);

		size_t ip_continue = state->ip;
		state->AddCode(block, code(command_kind::pc_jump, ip));
		size_t ip_back = state->ip;

		if (blockParam.size == 0U) {
			while (state->ip > ip)
				state->PopCode(block);
		}
		else {
			block->codes[ip_loopchk].arg0 = ip_back;
			link_break_continue(block, state, ip_block_begin, ip_continue, ip_back, ip_continue);

			block->codes[ip_var_format].arg0 = blockParam.alloc_base;
			block->codes[ip_var_format].arg1 = blockParam.alloc_size;
		}

		need_terminator = false;
		break;
	}
	case token_kind::tk_FOR:
	{
		state->advance();

		size_t ip_for_begin = state->ip;

		if (state->next() == token_kind::tk_EACH) {				//For-each loop
			state->advance();
			parser_assert(state, state->next() == token_kind::tk_open_par, "\"(\" is required.\r\n");

			state->advance();

			bool bHasOpenPar = false;
			if (state->next() == token_kind::tk_open_par) {
				bHasOpenPar = true;
				state->advance();
			}

			std::vector<arg_data> listCounterVars;
			auto _ParseDecl = [&]() {
				arg_data nCounter = parse_variable_decl(state, false);

				parser_assert(state, !nCounter.bConst, "The counter variable cannot be const.\r\n");

				listCounterVars.push_back(nCounter);
			};

			_ParseDecl();

			if (state->next() == token_kind::tk_comma) {
				//Format: 
				//		"for each (i, j in array)"
				//		"for each (let i, let j in array)"
				//		"for each ((i, j) in array)"
				//		"for each ((let i, let j) in array)"
				//	i = loop count
				//	j = array element

				state->advance();

				_ParseDecl();
			}

			if (bHasOpenPar) {
				parser_assert(state, state->next() == token_kind::tk_close_par, "\")\" is required.\r\n");
				state->advance();
			}

			parser_assert(state, state->next() == token_kind::tk_IN || state->next() == token_kind::tk_colon,
				"\"in\" or a colon is required.\r\n");
			state->advance();

			bool bMakeArrayUnique = true;
			if (state->next() == token_kind::tk_decl_mod_ref) {
				bMakeArrayUnique = false;
				state->advance();
			}

			size_t ip_var_format = state->ip;
			state->AddCode(block, code(command_kind::pc_var_format, 0U, 0));

			//The array
			parse_expression(block, state);
			if (bMakeArrayUnique)
				state->AddCode(block, code(command_kind::pc_make_unique, 0));

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

			block_return_t blockParam;
			{
				if (listCounterVars.size() == 2) {
					//Decrease the counter variable by 1
					state->AddCode(block, code(command_kind::pc_dup_n, 1U));
					state->AddCode(block, code(command_kind::pc_load_ptr, 0));
					state->AddCode(block, code(command_kind::pc_inline_dec, false, true));
				}
				blockParam = parse_block(block, state, &listCounterVars, true);
			}

			size_t ip_continue = state->ip;
			state->AddCode(block, code(command_kind::pc_jump, ip));
			size_t ip_back = state->ip;

			//Pop twice for the array and the counter
			state->AddCode(block, code(command_kind::pc_pop, 2));

			if (blockParam.size <= 1U) {	//1 for pc_copy_assign
				while (state->ip > ip_for_begin)
					state->PopCode(block);
			}
			else {
				block->codes[ip_loopchk].arg0 = ip_back;
				link_break_continue(block, state, ip, ip_continue, ip_back, ip_continue);

				block->codes[ip_var_format].arg0 = blockParam.alloc_base;
				block->codes[ip_var_format].arg1 = blockParam.alloc_size;
			}
		}
		else if (state->next() == token_kind::tk_open_par) {	//Regular for loop
			state->advance();

			std::vector<arg_data> listNewVars;

			script_scanner* const lex_org = state->lex;
			script_scanner lex_s1(*state->lex);		//First statement (initialization)

			auto _SkipUntilSemicolon = [&]() {
				while (state->next() != token_kind::tk_semicolon) {
					switch (state->next()) {
					case token_kind::tk_open_cur:
						parser_assert(state, false, "Invalid statement.");
					}
					_parser_assert_nend(state);
					state->advance();
				}
			};

			if (IsDeclToken(state->next())) {
				arg_data firstArg = arg_data();

				while (true) {
					arg_data nArg = parse_variable_decl(state, false);
					if (firstArg.name.size() == 0)
						firstArg = nArg;

					nArg.type = firstArg.type;
					nArg.bConst = firstArg.bConst;
					listNewVars.push_back(nArg);

					int brk = 0;
					int par = 0;
					if (!_parser_skip_post_decl(state, &brk, &par))
						break;
				}
			}
			else {
				_SkipUntilSemicolon();
				state->advance();
			}

			bool hasEvalExpr = true;
			if (state->next() == token_kind::tk_semicolon) hasEvalExpr = false;

			script_scanner lex_s2(*state->lex);		//Second statement (evaluation)

			_SkipUntilSemicolon();
			state->advance();

			script_scanner lex_s3(*state->lex);		//Third statement (update)

			{
				int cur = 0;
				while (true) {
					_parser_assert_nend(state);
					if (state->next() == token_kind::tk_open_par) ++cur;
					else if (state->next() == token_kind::tk_close_par) {
						if (cur == 0) break;
						--cur;
					}
					state->advance();
				}
				state->advance();
			}

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
				for (parser_state_t* iState = state; iState != nullptr; iState = iState->state_pred)
					varc_prev_total += iState->var_count_main;

				//Register new vars to the parser
				for (size_t iNewVar = 0; iNewVar < listNewVars.size(); ++iNewVar) {
					arg_data* pVarData = &listNewVars[iNewVar];
					symbol s = symbol(block->level, pVarData->type, 
						varc_prev_total + iNewVar, pVarData->bConst);
					frame.back().singular_insert(pVarData->name, s);
				}
				newState.var_count_main = listNewVars.size();

				ip_var_format = newState.ip;
				newState.AddCode(block, code(command_kind::pc_var_format, 0U, 0));

				//Initialization statement
				newState.lex = &lex_s1;
				parse_single_statement(block, &newState, true, token_kind::tk_semicolon);

				size_t ip_begin = newState.ip;

				//Evaluation statement
				if (hasEvalExpr) {
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
						newState.var_count_main += scan_current_scope(&newState, block->level, 
							varc_prev_total + listNewVars.size(), nullptr);
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
				if (hasEvalExpr) block->codes[ip_loopchk].arg0 = ip_back;
				link_break_continue(block, &newState, ip_for_begin, ip_continue, ip_back, ip_continue);

				block->codes[ip_var_format].arg0 = varc_prev_total;
				block->codes[ip_var_format].arg1 = newState.var_count_main;
			}

			state->ip = newState.ip;
			state->GrowVarCount(newState.var_count_main + newState.var_count_sub);
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

		arg_data counterData = parse_variable_decl(state, false);
		parser_assert(state, !counterData.bConst, "The counter variable cannot be const.\r\n");

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

		if (isAscent)
			state->AddCode(block, code(command_kind::pc_swap));

		size_t ip_var_format = state->ip;
		state->AddCode(block, code(command_kind::pc_var_format, 0U, 0));

		size_t ip = state->ip;

		state->AddCode(block, code(isAscent ? command_kind::pc_loop_ascent : command_kind::pc_loop_descent));
		size_t ip_loopchk = state->ip;
		state->AddCode(block, code(command_kind::pc_jump_if, 0U));

		if (!isAscent) {
			state->AddCode(block, code(command_kind::pc_load_ptr, 0));
			state->AddCode(block, code(command_kind::pc_inline_dec, false, true));
		}

		state->AddCode(block, code(command_kind::pc_dup_n, 0U));

		block_return_t blockParam;
		{
			std::vector<arg_data> argv = { counterData };
			blockParam = parse_block(block, state, &argv, true);
		}

		size_t ip_continue = state->ip;

		if (isAscent) {
			state->AddCode(block, code(command_kind::pc_load_ptr, 0));
			state->AddCode(block, code(command_kind::pc_inline_inc, false, true));
		}

		state->AddCode(block, code(command_kind::pc_jump, ip));
		size_t ip_back = state->ip;

		//Pop twice for two statements
		state->AddCode(block, code(command_kind::pc_pop, 2));

		if (blockParam.size <= 1U) {	//1 for pc_copy_assign
			while (state->ip > ip_ascdsc_begin)
				state->PopCode(block);
		}
		else {
			block->codes[ip_loopchk].arg0 = ip_back;
			link_break_continue(block, state, ip_ascdsc_begin, ip_continue, ip_back, ip_continue);

			block->codes[ip_var_format].arg0 = blockParam.alloc_base;
			block->codes[ip_var_format].arg1 = blockParam.alloc_size;
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
				|| c->command == command_kind::_pc_jump) 				{
				auto itrMap = mapLabelCode.find(c->arg0);
				c->command = get_replacing_jump(c->command);
				c->arg0 = (itrMap != mapLabelCode.end()) ? itrMap->second : ip_end;
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
				state->AddCode(block, code(command_kind::pc_dup_n, 0U, false));
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
				|| c->command == command_kind::_pc_jump) 				{
				auto itrMap = mapLabelCode.find(c->arg0);
				c->command = get_replacing_jump(c->command);
				c->arg0 = (itrMap != mapLabelCode.end()) ? itrMap->second : ip_end;
			}
		}

		need_terminator = false;
		break;
	}
	case token_kind::tk_BREAK:
	case token_kind::tk_CONTINUE:
	{
		command_kind c = state->next() == token_kind::tk_BREAK ?
			command_kind::pc_loop_break : command_kind::pc_loop_continue;
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
			break;
		default:
		{
			symbol* s = search_result();
			parser_assert(state, s, "Only functions may return values.\r\n");

			parse_expression(block, state);
			if (s->type != nullptr) {
				parser_assert(state, s->type->get_kind() != type_data::tk_null,
					"Functions marked with \"void\" cannot have a return value.\r\n");
				state->AddCode(block, code(command_kind::pc_inline_cast_var, (uint32_t)s->type, true));
			}
			state->AddCode(block, code(command_kind::pc_copy_assign,
				s->level, s->var, "!res"));
		}
		}
		state->AddCode(block, code(command_kind::pc_sub_return));

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

		if (state->next() == token_kind::tk_l) {	//<...>
			while (state->next() != token_kind::tk_g) state->advance();
			state->advance();
		}

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

		/*
		std::vector<arg_data> args;
		if (s->sub->kind != block_kind::bk_sub) {
			if (state->next() == token_kind::tk_open_par) {
				state->advance();
				while (state->next() == token_kind::tk_word || IsDeclToken(state->next())) {
					//TODO: Give scripters the ability to define vararg functions

					arg_data nArg = parse_variable_decl(state, true);
					args.push_back(nArg);

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
		*/
		size_t argc = 0;
		if (state->next() == token_kind::tk_open_par) {
			state->advance();
			if (state->next() != token_kind::tk_close_par) {
				while (true) {
					if (state->next() == token_kind::tk_comma)
						++argc;
					else if (state->next() == token_kind::tk_close_par) {
						++argc;
						break;
					}
					state->advance();
				}
			}
			state->advance();
		}

		s = search_in(pScope, funcName, argc);
		parser_assert(state, s, 
			StringUtility::Format("Invalid callable declaration: \"%s\"\r\n", funcName.c_str()));
		parser_assert(state, s->sub->codes.size() == 0, 
			StringUtility::Format("\"%s\": Body already defined.\r\n", funcName.c_str()));

		{
			frame.push_back(scope_t(s->sub->kind));

			parser_state_t newState(state->lex);

			parser_assert(&newState, newState.next() == token_kind::tk_open_cur, "\"{\" is required.\r\n");
			newState.advance();

			{
				int totalVar = 0;

				if (s->sub->kind == block_kind::bk_function) {
					symbol sRes = symbol(s->sub->level, s->type, 0, false);
					frame.back().singular_insert("\x01", sRes);		//Function return

					totalVar = 1;
				}

				totalVar += scan_current_scope(&newState, s->sub->level, totalVar, &s->argData);
				newState.AddCode(s->sub, code(command_kind::pc_var_alloc, 0));

				newState.var_count_main = totalVar;
			}

			//Function arguments
			for (const auto& iArg : s->argData) {
				symbol* sVar = search(iArg.name);
				//No need to type-convert arguments here
				newState.AddCode(s->sub, code(command_kind::pc_copy_assign,
					sVar->level, sVar->var, iArg.name));
			}
			parse_statements(s->sub, &newState, token_kind::tk_close_cur, token_kind::tk_semicolon);
			scan_final(s->sub, &newState);

			s->sub->codes[0].arg0 = newState.var_count_main + newState.var_count_sub;

			parser_assert(&newState, newState.next() == token_kind::tk_close_cur, "\"}\" is required.\r\n");
			newState.advance();

			frame.pop_back();
		}

		need_terminator = false;
		break;
	}
	case token_kind::tk_ASYNC:
	{
		state->advance();
		parser_assert(state, state->next() == token_kind::tk_open_cur, "\"{\" is required.\r\n");
		state->advance();

		script_block* asyncBlock = engine->new_block(block->level + 1, block_kind::bk_microthread);
#ifdef _DEBUG
		asyncBlock->name = StringUtility::Format("!@_bk_async_%08x", 
			std::hash<uint32_t>::_Do_hash(((uint32_t)block ^ (uint32_t)state) * state->ip + block->level));
#else
		asyncBlock->name = "";
#endif
		asyncBlock->func = nullptr;
		asyncBlock->arguments = 0;

		{
			frame.push_back(scope_t(asyncBlock->kind));

			parser_state_t newState(state->lex);

			{
				int totalVar = scan_current_scope(&newState, asyncBlock->level, 0, nullptr);
				newState.AddCode(asyncBlock, code(command_kind::pc_var_alloc, 0));

				newState.var_count_main = totalVar;
			}

			parse_statements(asyncBlock, &newState, token_kind::tk_close_cur, token_kind::tk_semicolon);
			scan_final(asyncBlock, &newState);

			asyncBlock->codes[0].arg0 = newState.var_count_main + newState.var_count_sub;

			frame.pop_back();
		}

		parser_assert(state, state->next() == token_kind::tk_close_cur, "\"}\" is required.\r\n");
		state->advance();

		if (asyncBlock->codes.size() > 1)
			state->AddCode(block, code(command_kind::pc_call, (uint32_t)asyncBlock, 0));

		need_terminator = false;
		break;
	}
	}

	//_parser_assert_nend(state);
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

parser::block_return_t parser::parse_block(script_block* block, parser_state_t* state,
	const std::vector<arg_data>* args, bool allow_single) 	{
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
		newState.var_count_main = scan_current_scope(&newState, block->level, varc_prev_total, args);
	}
	else if (args) {
		scope_t* ptrBackFrame = &frame.back();
		for (size_t i = 0; i < args->size(); ++i) {
			const arg_data* arg = &args->at(i);
			symbol s = symbol(block->level, arg->type, varc_prev_total + i, arg->bConst);
			ptrBackFrame->singular_insert(arg->name, s);
		}
		newState.var_count_main = args->size();
	}

	if (args) {
		for (size_t i = 0; i < args->size(); ++i) {
			const arg_data* arg = &args->at(i);
			if (arg->type != nullptr) {
				newState.AddCode(block, code(command_kind::pc_inline_cast_var, (uint32_t)arg->type, true));
			}
			newState.AddCode(block, code(command_kind::pc_copy_assign,
				block->level, varc_prev_total + i, arg->name));
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

	block_return_t blockRet = { block->codes.size() - prev_size, varc_prev_total, newState.var_count_main };
	return blockRet;
}
size_t parser::parse_block_inlined(script_block* block, parser_state_t* state, bool allow_single) {
	size_t ip_var_format = state->ip;
	state->AddCode(block, code(command_kind::pc_var_format, 0U, 0));

	block_return_t blockParam = parse_block(block, state, nullptr, allow_single);

	if (blockParam.size == 0U) {
		state->PopCode(block);	//For pc_var_format
	}
	else {
		block->codes[ip_var_format].arg0 = blockParam.alloc_base;
		block->codes[ip_var_format].arg1 = blockParam.alloc_size;
	}

	return blockParam.size;
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
		 *		pc_push_value		a
		 *		pc_push_value		b
		 *		pc_push_value		c
		 *		pc_push_value		d
		 *		pc_construct_array	4
		 * into
		 *		pc_push_value		[a, b, c, d]
		 */
		case command_kind::pc_construct_array:
		{
			code* ptrBack = &newCodes.back();
			size_t sizeArray = iSrcCode->arg0;
			if (newCodes.size() >= sizeArray) {
				type_data* arrayType = nullptr;
				value arrayVal;

				if (sizeArray > 0) {
					std::vector<value> listPtrVal;
					listPtrVal.resize(sizeArray);

					code* ptrPushValueCode = ptrBack + 1;
					for (size_t i = 0; i < sizeArray; ++i) {
						--ptrPushValueCode;
						if (ptrPushValueCode->command != command_kind::pc_push_value)
							goto lab_opt_construct_array_cancel;
					}
					//Make sure the previous value was not part of a ternary statement
					if (newCodes.size() >= sizeArray + 1 && (ptrPushValueCode[-1]).command == command_kind::pc_jump)
						goto lab_opt_construct_array_cancel;

					type_data* type_elem = ptrPushValueCode->data.get_type();
					arrayType = script_type_manager::get_instance()->get_array_type(type_elem);

					for (size_t i = 0; i < sizeArray; ++i) {
						{
							value appending = ptrPushValueCode->data;
							appending.make_unique();

							BaseFunction::_append_check(nullptr, arrayType, appending.get_type());
							BaseFunction::_value_cast(&appending, type_elem);

							listPtrVal[i] = appending;
						}
						++ptrPushValueCode;
					}

					arrayVal.reset(arrayType, listPtrVal);
				}
				else {
					arrayVal = value(script_type_manager::get_null_array_type(), std::wstring());
				}

				for (size_t i = 0; i < sizeArray; ++i)
					newCodes.pop_back();
				arrayVal.make_unique();
				newCodes.push_back(code(iSrcCode->line, command_kind::pc_push_value, arrayVal));
				state->ip -= sizeArray;
			}
			else {
lab_opt_construct_array_cancel:
				newCodes.push_back(*iSrcCode);
			}
			break;
		}
		case command_kind::pc_load_ptr:
		{
			code* ptrBack = &newCodes.back();
			if (ptrBack->command == command_kind::pc_push_variable && (iSrcCode->arg0 == 0)) {
				ptrBack->line = iSrcCode->line;
				ptrBack->command = command_kind::pc_push_variable2;
				--(state->ip);
			}
			else {
				newCodes.push_back(*iSrcCode);
			}
			break;
		}
		case command_kind::pc_nop:
			--(state->ip);
			break;
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
				mapLabelCode.insert(std::make_pair(itr->arg0, ip - removing));
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
			auto itrFind = mapLabelCode.find(itr->arg0);
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
	size_t ip_begin, size_t ip_end, size_t ip_break, size_t ip_continue) 	{
	auto itrBegin = block->codes.begin() + ip_begin;
	auto itrEnd = block->codes.begin() + ip_end + 1;
	for (auto itr = itrBegin; itr != itrEnd; ++itr) {
		if (itr == block->codes.end()) return;
		switch (itr->command) {
		case command_kind::pc_loop_break:
			itr->command = command_kind::pc_jump;
			itr->arg0 = ip_break;
			break;
		case command_kind::pc_loop_continue:
			itr->command = command_kind::pc_jump;
			itr->arg0 = ip_continue;
			break;
		}
	}
}

void parser::scan_final(script_block* block, parser_state_t* state) {
	for (auto itr = block->codes.begin(); itr != block->codes.end(); ++itr) {
		parser_assert(itr->line, itr->command != command_kind::pc_loop_break,
			"\"break\" may only be used inside a loop.");
		parser_assert(itr->line, itr->command != command_kind::pc_loop_continue,
			"\"continue\" may only be used inside a loop.");
	}
}
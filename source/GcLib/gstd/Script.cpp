#include "source/GcLib/pch.h"

#include "GstdUtility.hpp"
#include "Script.hpp"
#include "ScriptLexer.hpp"

#ifdef _MSC_VER
//#define for if(0);else for
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

double fmodl2(double i, double j) {
	if (j < 0) {
		//return (i < 0) ? -(-i % -j) : (i % -j) + j;
		return (i < 0) ? -fmodl(-i, -j) : fmodl(i, -j) + j;
	}
	else {
		//return (i < 0) ? -(-i % j) + j : i % j;
		return (i < 0) ? -fmodl(-i, j) + j : fmodl(i, j);
	}
}

//--------------------------------------

/* operations */

#define SCRIPT_DECLARE_OP(F) \
value F(script_machine* machine, int argc, value const* argv) { \
	try { \
		return _script_##F(argc, argv); \
	} \
	catch (std::wstring& err) { \
		if (machine) machine->raise_error(err); \
		else throw err; \
		return value(); \
	} \
}

value __script_perform_op_array(const value* v_left, const value* v_right, value (*func)(int, value const*)) {
	value result;
	std::vector<value> resArr;

	size_t ct_left = v_left->length_as_array();
	resArr.resize(ct_left);

	if (v_right->get_type()->get_kind() == type_data::type_kind::tk_array) {
		size_t ct_op = std::min(v_right->length_as_array(), ct_left);
		value v[2];
		for (size_t i = 0; i < ct_op; ++i) {
			v[0] = v_left->index_as_array(i);
			v[1] = v_right->index_as_array(i);
			resArr[i] = func(2, v);
		}
		for (size_t i = ct_op; i < ct_left; ++i) resArr[i] = v_left->index_as_array(i);
	}
	else {
		value v[2];
		v[1] = *v_right;
		for (size_t i = 0; i < ct_left; ++i) {
			v[0] = v_left->index_as_array(i);
			resArr[i] = func(2, v);
		}
	}

	result.set(v_left->get_type(), resArr);
	return result;
}

value _script_add(int argc, value const* argv) {
	if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
		return __script_perform_op_array(&argv[0], &argv[1], _script_add);
	else
		return value(script_type_manager::get_real_type(), argv[0].as_real() + argv[1].as_real());
}
SCRIPT_DECLARE_OP(add);

value _script_subtract(int argc, value const* argv) {
	if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
		return __script_perform_op_array(&argv[0], &argv[1], _script_subtract);
	else
		return value(script_type_manager::get_real_type(), argv[0].as_real() - argv[1].as_real());
}
SCRIPT_DECLARE_OP(subtract);

value _script_multiply(int argc, value const* argv) {
	if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
		return __script_perform_op_array(&argv[0], &argv[1], _script_multiply);
	else
		return value(script_type_manager::get_real_type(), argv[0].as_real() * argv[1].as_real());
}
SCRIPT_DECLARE_OP(multiply);

value _script_divide(int argc, value const* argv) {
	if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
		return __script_perform_op_array(&argv[0], &argv[1], _script_divide);
	else
		return value(script_type_manager::get_real_type(), argv[0].as_real() / argv[1].as_real());
}
SCRIPT_DECLARE_OP(divide);

value _script_remainder_(int argc, value const* argv) {
	if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
		return __script_perform_op_array(&argv[0], &argv[1], _script_remainder_);
	else
		return value(script_type_manager::get_real_type(), fmodl2(argv[0].as_real(), argv[1].as_real()));
}
SCRIPT_DECLARE_OP(remainder_);

value modc(script_machine* machine, int argc, value const* argv) {
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	return value(script_type_manager::get_real_type(), fmod(x, y));
}

value _script_negative(int argc, value const* argv) {
	if (argv->get_type()->get_kind() == type_data::type_kind::tk_array) {
		value result;
		std::vector<value> resArr;
		resArr.resize(argv->length_as_array());
		for (size_t i = 0; i < argv->length_as_array(); ++i) {
			resArr[i] = _script_negative(1, &argv->index_as_array(i));
		}
		result.set(argv->get_type(), resArr);
		return result;
	}
	else
		return value(script_type_manager::get_real_type(), -argv->as_real());
}
SCRIPT_DECLARE_OP(negative);

value _script_power(int argc, value const* argv) {
	if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
		return __script_perform_op_array(&argv[0], &argv[1], _script_power);
	else
		return value(script_type_manager::get_real_type(), std::pow(argv[0].as_real(), argv[1].as_real()));
}
SCRIPT_DECLARE_OP(power);

value _script_compare(int argc, value const* argv) {
	if (argv[0].get_type() == argv[1].get_type()) {
		int r = 0;

		switch (argv[0].get_type()->get_kind()) {
		case type_data::type_kind::tk_real:
		{
			double a = argv[0].as_real();
			double b = argv[1].as_real();
			r = (a == b) ? 0 : (a < b) ? -1 : 1;
		}
		break;

		case type_data::type_kind::tk_char:
		{
			wchar_t a = argv[0].as_char();
			wchar_t b = argv[1].as_char();
			r = (a == b) ? 0 : (a < b) ? -1 : 1;
		}
		break;

		case type_data::type_kind::tk_boolean:
		{
			bool a = argv[0].as_boolean();
			bool b = argv[1].as_boolean();
			r = (a == b) ? 0 : (a < b) ? -1 : 1;
		}
		break;

		case type_data::type_kind::tk_array:
		{
			for (size_t i = 0; i < argv[0].length_as_array(); ++i) {
				if (i >= argv[1].length_as_array()) {
					r = +1;	//"123" > "12"
					break;
				}

				value v[2];
				v[0] = argv[0].index_as_array(i);
				v[1] = argv[1].index_as_array(i);
				r = _script_compare(2, v).as_real();
				if (r != 0)
					break;
			}
			if (r == 0 && argv[0].length_as_array() < argv[1].length_as_array()) {
				r = -1;	//"12" < "123"
			}
		}
		break;

		default:
			assert(false);
		}
		return value(script_type_manager::get_int_type(), (int64_t)r);
	}
	else {
		throw std::wstring(L"Values of different types are being compared.\r\n");
	}
}
SCRIPT_DECLARE_OP(compare);

value predecessor(script_machine* machine, int argc, value const* argv) {
	assert(argc == 1);
	assert(argv->has_data());
	switch (argv->get_type()->get_kind()) {
	case type_data::type_kind::tk_real:
		return value(argv->get_type(), argv->as_real() - 1);
	case type_data::type_kind::tk_char:
	{
		wchar_t c = argv->as_char();
		--c;
		return value(argv->get_type(), c);
	}
	case type_data::type_kind::tk_boolean:
		return value(argv->get_type(), false);
	case type_data::type_kind::tk_array:
	{
		value result;
		std::vector<value> resArr;
		resArr.resize(argv->length_as_array());
		for (size_t i = 0; i < argv->length_as_array(); ++i) {
			resArr[i] = predecessor(machine, 1, &(argv->index_as_array(i)));
		}
		result.set(argv->get_type(), resArr);
		return result;
	}
	default:
	{
		std::wstring error = L"This value type does not support the predecessor operation.\r\n";
		machine->raise_error(error);
		return value();
	}
	}
}

value successor(script_machine* machine, int argc, value const* argv) {
	assert(argc == 1);
	assert(argv->has_data());
	switch (argv->get_type()->get_kind()) {
	case type_data::type_kind::tk_real:
		return value(argv->get_type(), argv->as_real() + 1);
	case type_data::type_kind::tk_char:
	{
		wchar_t c = argv->as_char();
		++c;
		return value(argv->get_type(), c);
	}
	case type_data::type_kind::tk_boolean:
		return value(argv->get_type(), true);
	case type_data::type_kind::tk_array:
	{
		value result;
		std::vector<value> resArr;
		resArr.resize(argv->length_as_array());
		for (size_t i = 0; i < argv->length_as_array(); ++i) {
			resArr[i] = successor(machine, 1, &(argv->index_as_array(i)));
		}
		result.set(argv->get_type(), resArr);
		return result;
	}
	default:
	{
		std::wstring error = L"This value type does not support the successor operation.\r\n";
		machine->raise_error(error);
		return value();
	}
	}
}

value _script_not_(int argc, value const* argv) {
	return value(script_type_manager::get_boolean_type(), !argv->as_boolean());
}
SCRIPT_DECLARE_OP(not_);

value length(script_machine* machine, int argc, value const* argv) {
	assert(argc == 1);
	return value(script_type_manager::get_real_type(), (double)argv->length_as_array());
}

bool _index_check(script_machine* machine, type_data* arg0_type, size_t arg0_size, double index) {
	if (arg0_type->get_kind() != type_data::type_kind::tk_array) {
		std::wstring error = L"This value type does not support the array index operation.\r\n";
		machine->raise_error(error);
		return false;
	}
	if (index < 0 || index >= arg0_size) {
		std::wstring error = L"Array index out of bounds.\r\n";
		machine->raise_error(error);
		return false;
	}
	return true;
}

const value& index(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);

	double index = argv[1].as_real();
	if (!_index_check(machine, argv->get_type(), argv->length_as_array(), index))
		return value::val_empty;

	return argv->index_as_array(index);
}

value slice(script_machine* machine, int argc, value const* argv) {
	assert(argc == 3);

	if (argv->get_type()->get_kind() != type_data::type_kind::tk_array) {
		std::wstring error = L"This value type does not support the array slice operation.\r\n";
		machine->raise_error(error);
		return value();
	}

	size_t index_1 = argv[1].as_real();
	size_t index_2 = argv[2].as_real();

	if ((index_2 > index_1 && (index_1 < 0 || index_2 > argv->length_as_array()))
		|| (index_2 < index_1 && (index_2 < 0 || index_1 > argv->length_as_array())))
	{
		std::wstring error = L"Array index out of bounds.\r\n";
		machine->raise_error(error);
		return value();
	}

	value result;
	std::vector<value> resArr;

	if (index_2 > index_1) {
		resArr.resize(index_2 - index_1);
		for (size_t i = index_1, j = 0; i < index_2; ++i, ++j) 
			resArr[j] = argv->index_as_array(i);
	}
	else if (index_1 > index_2) {
		resArr.resize(index_1 - index_2);
		for (size_t i = 0, j = index_1 - 1; i < index_1 - index_2; ++i, --j)
			resArr[i] = argv->index_as_array(j);
	}

	result.set(argv->get_type(), resArr);
	return result;
}

value erase(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);

	if (argv->get_type()->get_kind() != type_data::type_kind::tk_array) {
		std::wstring error = L"This value type does not support the array erase operation.\r\n";
		machine->raise_error(error);
		return value();
	}

	size_t index_1 = argv[1].as_real();
	size_t length = argv->length_as_array();

	if (index_1 >= length) {
		std::wstring error = L"Array index out of bounds.\r\n";
		machine->raise_error(error);
		return value();
	}

	value result;
	std::vector<value> resArr;
	resArr.resize(length - 1U);
	{
		size_t iArr = 0;
		for (size_t i = 0; i < index_1; ++i) {
			resArr[iArr++] = argv->index_as_array(i);
		}
		for (size_t i = index_1 + 1; i < length; ++i) {
			resArr[iArr++] = argv->index_as_array(i);
		}
	}
	result.set(argv->get_type(), resArr);
	return result;
}

bool script_machine::append_check(size_t arg0_size, type_data* arg0_type, type_data* arg1_type) {
	if (arg0_type->get_kind() != type_data::type_kind::tk_array) {
		raise_error(L"This value type does not support the array append operation.\r\n");
		return false;
	}
	if (arg0_size > 0U && arg0_type->get_element() != arg1_type) {
		raise_error(L"Variable type mismatch.\r\n");
		return false;
	}
	return true;
}
value append(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);

	machine->append_check(argv->length_as_array(), argv->get_type(), argv[1].get_type());

	value result = argv[0];
	result.append(script_type_manager::get_instance()->get_array_type(argv[1].get_type()), argv[1]);
	return result;
}

value concatenate(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);

	if (argv[0].get_type()->get_kind() != type_data::type_kind::tk_array 
		|| argv[1].get_type()->get_kind() != type_data::type_kind::tk_array) 
	{
		std::wstring error = L"This value type does not support the array concatenate operation.\r\n";
		machine->raise_error(error);
		return value();
	}

	if (argv[0].length_as_array() > 0 && argv[1].length_as_array() > 0 && argv[0].get_type() != argv[1].get_type()) {
		std::wstring error = L"Variable type mismatch.\r\n";
		machine->raise_error(error);
		return value();
	}

	value result = argv[0];
	result.concatenate(argv[1]);
	return result;
}

value round(script_machine* machine, int argc, value const* argv) {
	double r = std::floor(argv->as_real() + 0.5);
	return value(machine->get_engine()->get_real_type(), r);
}

value truncate(script_machine* machine, int argc, value const* argv) {
	double r = argv->as_real();
	r = (r > 0) ? std::floor(r) : std::ceil(r);
	return value(script_type_manager::get_real_type(), r);
}

value ceil(script_machine* machine, int argc, value const* argv) {
	return value(script_type_manager::get_real_type(), std::ceil(argv->as_real()));
}

value floor(script_machine* machine, int argc, value const* argv) {
	return value(script_type_manager::get_real_type(), std::floor(argv->as_real()));
}

value _script_absolute(int argc, value const* argv) {
	return value(script_type_manager::get_real_type(), std::fabs(argv->as_real()));
}
SCRIPT_DECLARE_OP(absolute);

value assert_(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);
	if (!argv[0].as_boolean()) {
		machine->raise_error(argv[1].as_string());
	}
	return value();
}

int64_t bitDoubleToInt(double val) {
	if (val > INT64_MAX)
		return INT64_MAX;
	if (val < INT64_MIN)
		return INT64_MIN;
	return static_cast<int64_t>(val);
}
value bitwiseNot(script_machine* machine, int argc, value const* argv) {
	int64_t val = bitDoubleToInt(argv[0].as_real());
	return value(script_type_manager::get_real_type(), (double)(~val));
}
value bitwiseAnd(script_machine* machine, int argc, value const* argv) {
	int64_t val1 = bitDoubleToInt(argv[0].as_real());
	int64_t val2 = bitDoubleToInt(argv[1].as_real());
	return value(script_type_manager::get_real_type(), (double)(val1 & val2));
}
value bitwiseOr(script_machine* machine, int argc, value const* argv) {
	int64_t val1 = bitDoubleToInt(argv[0].as_real());
	int64_t val2 = bitDoubleToInt(argv[1].as_real());
	return value(script_type_manager::get_real_type(), (double)(val1 | val2));
}
value bitwiseXor(script_machine* machine, int argc, value const* argv) {
	int64_t val1 = bitDoubleToInt(argv[0].as_real());
	int64_t val2 = bitDoubleToInt(argv[1].as_real());
	return value(script_type_manager::get_real_type(), (double)(val1 ^ val2));
}
value bitwiseLeft(script_machine* machine, int argc, value const* argv) {
	int64_t val1 = bitDoubleToInt(argv[0].as_real());
	size_t val2 = argv[1].as_real();
	return value(script_type_manager::get_real_type(), (double)(val1 << val2));
}
value bitwiseRight(script_machine* machine, int argc, value const* argv) {
	int64_t val1 = bitDoubleToInt(argv[0].as_real());
	size_t val2 = argv[1].as_real();
	return value(script_type_manager::get_real_type(), (double)(val1 >> val2));
}

value script_debugBreak(script_machine* machine, int argc, value const* argv) {
	DebugBreak();
	return value();
}

function const operations[] = {
	//{ "true", true_, 0 },
	//{ "false", false_, 0 },
	{ "length", length, 1 },
	{ "not", not_, 1 },
	{ "negative", negative, 1 },
	{ "predecessor", predecessor, 1 },
	{ "successor", successor, 1 },
	{ "round", round, 1 },
	{ "trunc", truncate, 1 },
	{ "truncate", truncate, 1 },
	{ "ceil", ceil, 1 },
	{ "floor", floor, 1 },
	//{ "abs", absolute, 1 },
	{ "absolute", absolute, 1 },
	{ "add", add, 2 },
	{ "subtract", subtract, 2 },
	{ "multiply", multiply, 2 },
	{ "divide", divide, 2 },
	{ "remainder", remainder_, 2 },
	{ "modc", modc, 2 },
	{ "power", power, 2 },
	//{ "index_", index<false>, 2 },
	//{ "index_w", index<true>, 2 },
	{ "slice", slice, 3 },
	{ "erase", erase, 2 },
	{ "append", append, 2 },
	{ "concatenate", concatenate, 2 },
	{ "compare", compare, 2 },
	{ "assert", assert_, 2 },
	{ "bit_not", bitwiseNot, 1 },
	{ "bit_and", bitwiseAnd, 2 },
	{ "bit_or", bitwiseOr, 2 },
	{ "bit_xor", bitwiseXor, 2 },
	{ "bit_left", bitwiseLeft, 2 },
	{ "bit_right", bitwiseRight, 2 },
	{ "__DEBUG_BREAK", script_debugBreak, 0 },
};

/* parser */

class gstd::parser {
public:
	using command_kind = script_engine::command_kind;
	using block_kind = script_engine::block_kind;

	struct symbol {
		int level;
		script_engine::block* sub;
		int variable;
		bool can_overload = true;
		bool can_modify = true;		//Applies to the scripter, not the engine
	};

	struct scope_t : public std::multimap<std::string, symbol> {
		block_kind kind;

		scope_t(block_kind the_kind) : kind(the_kind) {}

		void singular_insert(std::string name, symbol& s, int argc = 0);
	};

	std::vector<scope_t> frame;
	script_scanner* _lex;
	script_engine* engine;
	bool error;
	std::wstring error_message;
	int error_line;
	std::map<std::string, script_engine::block*> events;

	parser(script_engine* e, script_scanner* s, int funcc, function const* funcv);

	virtual ~parser() {}

	void parse_parentheses(script_engine::block* block, script_scanner* script_scanner);
	void parse_clause(script_engine::block* block, script_scanner* script_scanner);
	void parse_prefix(script_engine::block* block, script_scanner* script_scanner);
	void parse_suffix(script_engine::block* block, script_scanner* script_scanner);
	void parse_product(script_engine::block* block, script_scanner* script_scanner);
	void parse_sum(script_engine::block* block, script_scanner* script_scanner);
	void parse_comparison(script_engine::block* block, script_scanner* script_scanner);
	void parse_logic(script_engine::block* block, script_scanner* script_scanner);
	void parse_ternary(script_engine::block* block, script_scanner* script_scanner);
	void parse_expression(script_engine::block* block, script_scanner* script_scanner);

	int parse_arguments(script_engine::block* block, script_scanner* script_scanner);
	void parse_statements(script_engine::block* block, script_scanner* script_scanner, 
		token_kind statement_terminator = token_kind::tk_semicolon, bool single_parse = false);
	size_t parse_inline_block(script_engine::block** blockRes, script_engine::block* block, script_scanner* script_scanner, 
		block_kind kind, bool allow_single = false);
	void parse_block(script_engine::block* block, script_scanner* script_scanner, std::vector<std::string> const* args, 
		bool adding_result, bool allow_single = false);
private:
	void register_function(function const& func);
	symbol* search(std::string const& name, scope_t** ptrScope = nullptr);
	symbol* search(std::string const& name, int argc, scope_t** ptrScope = nullptr);
	symbol* search_in(scope_t* scope, std::string const& name);
	symbol* search_in(scope_t* scope, std::string const& name, int argc);
	symbol* search_result();
	int scan_current_scope(int level, std::vector<std::string> const* args, bool adding_result, int initVar = 0);
	void write_operation(script_engine::block* block, script_scanner* lex, char const* name, int clauses);
	void write_operation(script_engine::block* block, script_scanner* lex, const symbol* s, int clauses);

	void optimize_expression(script_engine::block* blockDst, script_engine::block* blockSrc);

	inline static void parser_assert(bool expr, std::wstring error) {
		if (!expr) 
			throw parser_error(error);
	}
	inline static void parser_assert(bool expr, std::string error) {
		if (!expr) 
			throw parser_error(error);
	}

	inline static bool IsDeclToken(token_kind& tk) {
		return tk == token_kind::tk_decl_auto || tk == token_kind::tk_decl_real;
		// || tk == token_kind::tk_decl_char || tk == token_kind::tk_decl_bool
		// || tk == token_kind::tk_decl_string;
	}

	typedef script_engine::code code;
};

void parser::scope_t::singular_insert(std::string name, symbol& s, int argc) {
	bool exists = this->find(name) != this->end();
	auto itr = this->equal_range(name);

	if (exists) {
		symbol* sPrev = &(itr.first->second);

		if (!sPrev->can_overload) {
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

parser::parser(script_engine* e, script_scanner* s, int funcc, function const* funcv) : engine(e), _lex(s), frame(), error(false) {
	frame.push_back(scope_t(block_kind::bk_normal));

	for (size_t i = 0; i < sizeof(operations) / sizeof(function); ++i)
		register_function(operations[i]);

	for (size_t i = 0; i < funcc; ++i)
		register_function(funcv[i]);

	try {
		int countVar = scan_current_scope(0, nullptr, false);
		if (countVar > 0)
			engine->main_block->codes.push_back(code(_lex->line, command_kind::pc_var_alloc, countVar));
		parse_statements(engine->main_block, _lex);

		parser_assert(_lex->next == token_kind::tk_end, 
			"Unexpected end-of-file while parsing. (Did you forget a semicolon after a string?)\r\n");
	}
	catch (parser_error& e) {
		error = true;
		error_message = e.what();
		error_line = _lex->line;
	}
}

inline void parser::register_function(function const& func) {
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

parser::symbol* parser::search(std::string const& name, scope_t** ptrScope) {
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
parser::symbol* parser::search(std::string const& name, int argc, scope_t** ptrScope) {
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
parser::symbol* parser::search_in(scope_t* scope, std::string const& name) {
	auto itrSymbol = scope->find(name);
	if (itrSymbol != scope->end())
		return &(itrSymbol->second);
	return nullptr;
}
parser::symbol* parser::search_in(scope_t* scope, std::string const& name, int argc) {
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

int parser::scan_current_scope(int level, std::vector<std::string> const* args, bool adding_result, int initVar) {
	//æ“Ç‚Ý‚µ‚ÄŽ¯•ÊŽq‚ð“o˜^‚·‚é
	script_scanner lex2(*_lex);

	int var = initVar;

	try {
		scope_t* current_frame = &frame[frame.size() - 1];
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
				current_frame->singular_insert((*args)[i], s);
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
							parser_assert(false, "A parameter list in not allowed here.\r\n");
						}
						else {
							while (lex2.next == token_kind::tk_word || IsDeclToken(lex2.next)) {
								++countArgs;

								if (IsDeclToken(lex2.next)) lex2.advance();

								if (lex2.next == token_kind::tk_word) lex2.advance();
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
						if (symbol* dup = search_in(current_frame, name, countArgs)) {
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
							error += " of the same name was already defined in the current scope.\r\n";
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
					parser_assert(current_frame->find(lex2.word) == current_frame->end(),
						"A variable of the same name was already declared in the current scope.\r\n");

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
		_lex->line = lex2.line;
		throw;
	}

	return (var - initVar);
}

void parser::write_operation(script_engine::block* block, script_scanner* lex, char const* name, int clauses) {
	symbol* s = search(name);
	assert(s != nullptr);
	{
		std::string error = "Invalid argument count for the default function. (expected " +
			std::to_string(clauses) + ")\r\n";
		parser_assert(s->sub->arguments == clauses, error);
	}

	block->codes.push_back(script_engine::code(lex->line, command_kind::pc_call_and_push_result, s->sub, clauses));
}
void parser::write_operation(script_engine::block* block, script_scanner* lex, const symbol* s, int clauses) {
	assert(s != nullptr);
	{
		std::string error = "Invalid argument count for the default function. (expected " +
			std::to_string(clauses) + ")\r\n";
		parser_assert(s->sub->arguments == clauses, error);
	}
	block->codes.push_back(script_engine::code(lex->line, command_kind::pc_call_and_push_result, s->sub, clauses));
}

void parser::parse_parentheses(script_engine::block* block, script_scanner* lex) {
	parser_assert(lex->next == token_kind::tk_open_par, "\"(\" is required.\r\n");
	lex->advance();

	parse_expression(block, lex);

	parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
	lex->advance();
}

void parser::parse_clause(script_engine::block* block, script_scanner* lex) {
	switch (lex->next) {
	case token_kind::tk_real:
		block->codes.push_back(code(lex->line, command_kind::pc_push_value,
			value(engine->get_real_type(), lex->real_value)));
		lex->advance();
		return;
	case token_kind::tk_char:
		block->codes.push_back(code(lex->line, command_kind::pc_push_value,
			value(engine->get_char_type(), lex->char_value)));
		lex->advance();
		return;
	case token_kind::tk_TRUE:
		block->codes.push_back(code(lex->line, command_kind::pc_push_value,
			value(engine->get_boolean_type(), true)));
		lex->advance();
		return;
	case token_kind::tk_FALSE:
		block->codes.push_back(code(lex->line, command_kind::pc_push_value,
			value(engine->get_boolean_type(), false)));
		lex->advance();
		return;
	case token_kind::tk_string:
	{
		std::wstring str = lex->string_value;
		lex->advance();
		while (lex->next == token_kind::tk_string || lex->next == token_kind::tk_char) {
			str += (lex->next == token_kind::tk_string) ? lex->string_value : (std::wstring() + lex->char_value);
			lex->advance();
		}

		block->codes.push_back(code(lex->line, command_kind::pc_push_value,
			value(engine->get_string_type(), str)));
		return;
	}
	case token_kind::tk_word:
	{
		std::string name = lex->word;
		lex->advance();
		int argc = parse_arguments(block, lex);

		symbol* s = search(name, argc);

		if (s == nullptr) {
			/*
			if (s = search(name)) {
				int req_argc = s->sub->arguments;
				if (req_argc < 0 && argc >= (-req_argc - 1)) goto continue_as_variadic;
			}
			*/
			std::string error;
			if (search(name))
				error = StringUtility::Format("No matching overload for %s with %d arguments was found.\r\n",
					name.c_str(), argc);
			else
				error = StringUtility::Format("%s is not defined.\r\n", name.c_str());
			throw parser_error(error);
		}

//continue_as_variadic:
		if (s->sub) {
			parser_assert(s->sub->kind == block_kind::bk_function, "Tasks and subs cannot return values.\r\n");
			block->codes.push_back(code(lex->line, command_kind::pc_call_and_push_result, s->sub, argc));
		}
		else {
			//Variable
			block->codes.push_back(code(lex->line, command_kind::pc_push_variable, s->level, s->variable, name));
		}

		return;
	}
	case token_kind::tk_CAST_REAL:
	case token_kind::tk_CAST_CHAR:
	case token_kind::tk_CAST_BOOL:
	{
		command_kind c = lex->next == token_kind::tk_CAST_REAL ? command_kind::pc_inline_cast_real :
			(lex->next == token_kind::tk_CAST_CHAR ? command_kind::pc_inline_cast_char : 
				command_kind::pc_inline_cast_bool);
		lex->advance();
		parse_parentheses(block, lex);
		block->codes.push_back(code(lex->line, c));
		return;
	}
	case token_kind::tk_open_bra:
	{
		lex->advance();

		size_t count_member = 0U;
		while (lex->next != token_kind::tk_close_bra) {
			parse_expression(block, lex);
			++count_member;

			if (lex->next != token_kind::tk_comma) break;
			lex->advance();
		}

		if (count_member > 0U)
			block->codes.push_back(code(lex->line, command_kind::pc_construct_array, count_member));
		else
			block->codes.push_back(code(lex->line, command_kind::pc_push_value,
				value(engine->get_string_type(), std::wstring())));

		parser_assert(lex->next == token_kind::tk_close_bra, "\"]\" is required.\r\n");
		lex->advance();

		return;
	}
	case token_kind::tk_open_abs:
		lex->advance();
		parse_expression(block, lex);
		block->codes.push_back(code(lex->line, command_kind::pc_inline_abs));
		parser_assert(lex->next == token_kind::tk_close_abs, "\"|)\" is required.\r\n");
		lex->advance();
		return;
	case token_kind::tk_open_par:
		parse_parentheses(block, lex);
		return;
	default:
		parser_assert(false, "Invalid expression.\r\n");
	}
}

void parser::parse_suffix(script_engine::block* block, script_scanner* lex) {
	parse_clause(block, lex);
	if (lex->next == token_kind::tk_caret) {
		lex->advance();
		parse_suffix(block, lex);
		block->codes.push_back(code(lex->line, command_kind::pc_inline_pow));
	}
	else {
		while (lex->next == token_kind::tk_open_bra) {
			lex->advance();
			parse_expression(block, lex);

			if (lex->next == token_kind::tk_range) {
				lex->advance();
				parse_expression(block, lex);
				write_operation(block, lex, "slice", 3);
			}
			else {
				block->codes.push_back(code(lex->line, command_kind::pc_inline_index_array, (size_t)false));
			}

			parser_assert(lex->next == token_kind::tk_close_bra, "\"]\" is required.\r\n");
			lex->advance();
		}
	}
}

void parser::parse_prefix(script_engine::block* block, script_scanner* lex) {
	switch (lex->next) {
	case token_kind::tk_plus:
		lex->advance();
		parse_prefix(block, lex);	//Ä‹A
		return;
	case token_kind::tk_minus:
		lex->advance();
		parse_prefix(block, lex);	//Ä‹A
		block->codes.push_back(code(lex->line, command_kind::pc_inline_neg));
		return;
	case token_kind::tk_exclamation:
		lex->advance();
		parse_prefix(block, lex);	//Ä‹A
		block->codes.push_back(code(lex->line, command_kind::pc_inline_not));
		return;
	default:
		parse_suffix(block, lex);
	}
}

void parser::parse_product(script_engine::block* block, script_scanner* lex) {
	parse_prefix(block, lex);
	while (lex->next == token_kind::tk_asterisk || lex->next == token_kind::tk_slash || lex->next == token_kind::tk_percent) {
		command_kind f = (lex->next == token_kind::tk_asterisk) ? command_kind::pc_inline_mul :
			(lex->next == token_kind::tk_slash) ? command_kind::pc_inline_div : command_kind::pc_inline_mod;
		lex->advance();
		parse_prefix(block, lex);
		block->codes.push_back(code(lex->line, f));
	}
}

void parser::parse_sum(script_engine::block* block, script_scanner* lex) {
	parse_product(block, lex);
	while (lex->next == token_kind::tk_tilde || lex->next == token_kind::tk_plus || lex->next == token_kind::tk_minus) {
		command_kind f = (lex->next == token_kind::tk_tilde) ? command_kind::pc_inline_cat :
			(lex->next == token_kind::tk_plus) ? command_kind::pc_inline_add : command_kind::pc_inline_sub;
		lex->advance();
		parse_product(block, lex);
		block->codes.push_back(code(lex->line, f));
	}
}

void parser::parse_comparison(script_engine::block* block, script_scanner* lex) {
	parse_sum(block, lex);
	switch (lex->next) {
	case token_kind::tk_assign:
		parser_assert(false, "Did you intend to write \"==\"?\r\n");
		break;
	case token_kind::tk_e:
	case token_kind::tk_g:
	case token_kind::tk_ge:
	case token_kind::tk_l:
	case token_kind::tk_le:
	case token_kind::tk_ne:
		token_kind op = lex->next;
		lex->advance();
		parse_sum(block, lex);

		switch (op) {
		case token_kind::tk_e:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_e));
			break;
		case token_kind::tk_g:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_g));
			break;
		case token_kind::tk_ge:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_ge));
			break;
		case token_kind::tk_l:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_l));
			break;
		case token_kind::tk_le:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_le));
			break;
		case token_kind::tk_ne:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_ne));
			break;
		}

		break;
	}
}

void parser::parse_logic(script_engine::block* block, script_scanner* lex) {
	parse_comparison(block, lex);
	while (lex->next == token_kind::tk_logic_and || lex->next == token_kind::tk_logic_or) {
		script_engine::command_kind cmd = (lex->next == token_kind::tk_logic_and) ?
			command_kind::pc_inline_logic_and : command_kind::pc_inline_logic_or;
		lex->advance();
		parse_comparison(block, lex);
		block->codes.push_back(code(lex->line, cmd));
	}
}

void parser::parse_ternary(script_engine::block* block, script_scanner* lex) {
	parse_logic(block, lex);
	if (lex->next == token_kind::tk_query) {
		lex->advance();

		size_t ip_jump_1 = block->codes.size();
		block->codes.push_back(code(lex->line, command_kind::pc_jump_if_not_diff, 0));	//Jump to expression 2

		//Parse expression 1
		parse_expression(block, lex);
		size_t ip_jump_2 = block->codes.size();
		block->codes.push_back(code(lex->line, command_kind::pc_jump_diff, 0));		//Jump to exit

		parser_assert(lex->next == token_kind::tk_colon, "Incomplete ternary statement; a colon(:) is required.");
		lex->advance();

		//Parse expression 2
		parse_expression(block, lex);

		size_t ip_exit = block->codes.size();
		block->codes[ip_jump_1].ip = ip_jump_2 - ip_jump_1 + 1;
		block->codes[ip_jump_2].ip = ip_exit - ip_jump_2;
		/*
		//For nested ternaries
		for (auto itr = std::next(block->codes.begin(), code_jump_1); itr != block->codes.end(); ++itr) {
			if (itr->command == command_kind::pc_loop_back)
				itr->ip = ip_exit + jumpOffFromParent;
		}
		*/
	}
}

void parser::parse_expression(script_engine::block* block, script_scanner* lex) {
	script_engine::block tmp(0, block_kind::bk_normal);
	parse_ternary(&tmp, lex);

	try {
		optimize_expression(block, &tmp);
	}
	catch (std::wstring& err) {
		parser_assert(false, err);
	}
}

//Format for variadic arguments (To-be-completed until after 1.10a):
// argc = -(n + 1)
// where n = fixed(required) arguments
int parser::parse_arguments(script_engine::block* block, script_scanner* lex) {
	int result = 0;
	if (lex->next == token_kind::tk_open_par) {
		lex->advance();

		while (lex->next != token_kind::tk_close_par) {
			++result;
			parse_expression(block, lex);
			if (lex->next != token_kind::tk_comma) break;
			lex->advance();
		}

		parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
		lex->advance();
	}
	return result;
}

void parser::parse_statements(script_engine::block* block, script_scanner* lex, 
	token_kind statement_terminator, bool single_parse) 
{
	auto assert_const = [](symbol* s, std::string& name) {
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

	for (; ; ) {
		bool need_semicolon = true;

		switch (lex->next) {
		case token_kind::tk_word:
		{
			std::string name = lex->word;

			scope_t* resScope = nullptr;
			symbol* s = search(name, &resScope);
			parser_assert(s, StringUtility::Format("%s is not defined.\r\n", name.c_str()));

			lex->advance();
			switch (lex->next) {
			case token_kind::tk_assign:
				assert_const(s, name);

				lex->advance();
				parse_expression(block, lex);
				block->codes.push_back(code(lex->line, command_kind::pc_assign, s->level, s->variable, name));
				break;
			case token_kind::tk_open_bra:
				assert_const(s, name);

				block->codes.push_back(code(lex->line, command_kind::pc_push_variable, s->level, s->variable, name));
				while (lex->next == token_kind::tk_open_bra) {
					lex->advance();
					parse_expression(block, lex);

					parser_assert(lex->next == token_kind::tk_close_bra, "\"]\" is required.\r\n");
					lex->advance();

					block->codes.push_back(code(lex->line, command_kind::pc_inline_index_array, (size_t)true));
				}

				switch (lex->next) {
				case token_kind::tk_assign:
					lex->advance();
					parse_expression(block, lex);
					block->codes.push_back(code(lex->line, command_kind::pc_assign_writable, 0, 0, name));
					break;
				case token_kind::tk_inc:
				case token_kind::tk_dec:
				{
					command_kind f = (lex->next == token_kind::tk_inc) ? command_kind::pc_inline_inc :
						command_kind::pc_inline_dec;
					lex->advance();
					block->codes.push_back(code(lex->line, f, -1, 0, name));
					break;
				}
				case token_kind::tk_add_assign:
				case token_kind::tk_subtract_assign:
				case token_kind::tk_multiply_assign:
				case token_kind::tk_divide_assign:
				case token_kind::tk_remainder_assign:
				case token_kind::tk_power_assign:
				{
					command_kind f = get_op_assign_command(lex->next);
					lex->advance();
					parse_expression(block, lex);
					block->codes.push_back(code(lex->line, f, -1, 0, name));
					break;
				}
				default:
					parser_assert(false, "\"=\" is required.\r\n");
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

				command_kind f = get_op_assign_command(lex->next);
				lex->advance();
				parse_expression(block, lex);
				block->codes.push_back(code(lex->line, f, s->level, s->variable, name));

				break;
			}
			case token_kind::tk_inc:
			case token_kind::tk_dec:
			{
				assert_const(s, name);

				command_kind f = (lex->next == token_kind::tk_inc) ? command_kind::pc_inline_inc : command_kind::pc_inline_dec;
				lex->advance();
				block->codes.push_back(code(lex->line, f, s->level, s->variable, name));

				break;
			}
			default:
			{
				parser_assert(s->sub, "A variable cannot be called as if it was a function, a task, or a sub.\r\n");

				int argc = parse_arguments(block, lex);

				s = search_in(resScope, name, argc);
				parser_assert(s, StringUtility::Format("No matching overload for %s with %d arguments was found.\r\n",
					name.c_str(), argc));

				block->codes.push_back(code(lex->line, command_kind::pc_call, s->sub, argc));

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
			lex->advance();
			parser_assert(lex->next == token_kind::tk_word, "Variable name is required.\r\n");

			std::string name = lex->word;

			lex->advance();
			if (lex->next == token_kind::tk_assign) {
				symbol* s = search(name);

				lex->advance();
				parse_expression(block, lex);
				block->codes.push_back(code(lex->line, command_kind::pc_assign, s->level, s->variable, name));
			}

			break;
		}
		//"local" can be omitted to make C-style blocks
		case token_kind::tk_LOCAL:
		case token_kind::tk_open_cur:
			if (lex->next == token_kind::tk_LOCAL) lex->advance();
			parse_inline_block(nullptr, block, lex, block_kind::bk_normal, false);
			need_semicolon = false;
			break;
		case token_kind::tk_LOOP:
		{
			lex->advance();
			size_t ip_begin = block->codes.size();

			if (lex->next == token_kind::tk_open_par) {
				parse_parentheses(block, lex);
				size_t ip = block->codes.size();
				{
					script_engine::block tmp(block->level, block_kind::bk_normal);

					tmp.codes.push_back(code(lex->line, command_kind::pc_loop_count));

					script_engine::block* childBlock = nullptr;
					size_t childSize = parse_inline_block(&childBlock, &tmp, lex, block_kind::bk_loop, true);

					tmp.codes.push_back(code(lex->line, command_kind::pc_continue_marker));
					tmp.codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));
					tmp.codes.push_back(code(lex->line, command_kind::pc_pop, 1));

					//Try to optimize looped yield to a wait
					if (childSize == 1U && childBlock->codes[0].command == command_kind::pc_yield) {
						engine->blocks.pop_back();
						block->codes.push_back(code(lex->line, command_kind::pc_wait));
					}
					else if (childSize == 0U) {
						//Empty loop, discard everything
						while (block->codes.size() > ip_begin)
							block->codes.pop_back();
					}
					else {
						block->codes.insert(block->codes.end(), tmp.codes.begin(), tmp.codes.end());
					}
				}
			}
			else {
				size_t ip = block->codes.size();
				size_t blockSize = parse_inline_block(nullptr, block, lex, block_kind::bk_loop, true);
				block->codes.push_back(code(lex->line, command_kind::pc_continue_marker));
				block->codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));

				if (blockSize == 0U) {
					while (block->codes.size() > ip_begin)
						block->codes.pop_back();
				}
			}

			need_semicolon = false;
			break;
		}
		case token_kind::tk_TIMES:
		{
			lex->advance();
			size_t ip_begin = block->codes.size();

			parse_parentheses(block, lex);
			size_t ip = block->codes.size();
			if (lex->next == token_kind::tk_LOOP)
				lex->advance();
			{
				script_engine::block tmp(block->level, block_kind::bk_normal);

				tmp.codes.push_back(code(lex->line, command_kind::pc_loop_count));

				script_engine::block* childBlock = nullptr;
				size_t childSize = parse_inline_block(&childBlock, &tmp, lex, block_kind::bk_loop, true);

				tmp.codes.push_back(code(lex->line, command_kind::pc_continue_marker));
				tmp.codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));
				tmp.codes.push_back(code(lex->line, command_kind::pc_pop, 1));

				//Try to optimize looped yield to a wait
				if (childSize == 1U && childBlock->codes[0].command == command_kind::pc_yield) {
					engine->blocks.pop_back();
					block->codes.push_back(code(lex->line, command_kind::pc_wait));
				}
				else if (childSize == 0U) {
					//Empty loop, discard everything
					while (block->codes.size() > ip_begin)
						block->codes.pop_back();
				}
				else {
					block->codes.insert(block->codes.end(), tmp.codes.begin(), tmp.codes.end());
				}
			}

			need_semicolon = false;
			break;
		}
		case token_kind::tk_WHILE:
		{
			lex->advance();
			size_t ip = block->codes.size();

			parse_parentheses(block, lex);
			if (lex->next == token_kind::tk_LOOP)
				lex->advance();

			block->codes.push_back(code(lex->line, command_kind::pc_loop_if));

			size_t newBlockSize = parse_inline_block(nullptr, block, lex, block_kind::bk_loop, true);

			block->codes.push_back(code(lex->line, command_kind::pc_continue_marker));
			block->codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));

			if (newBlockSize == 0U) {
				while (block->codes.size() > ip)
					block->codes.pop_back();
			}

			need_semicolon = false;
			break;
		}
		case token_kind::tk_FOR:
		{
			lex->advance();

			size_t ip_for_begin = block->codes.size();

			if (lex->next == token_kind::tk_EACH) {				//Foreach loop
				lex->advance();
				parser_assert(lex->next == token_kind::tk_open_par, "\"(\" is required.\r\n");

				lex->advance();
				if (lex->next == token_kind::tk_decl_auto) lex->advance();
				else if (lex->next == token_kind::tk_const)
					parser_assert(false, "The counter variable cannot be const.\r\n");

				parser_assert(lex->next == token_kind::tk_word, "Variable name is required.\r\n");

				std::string feIdentifier = lex->word;

				lex->advance();
				parser_assert(lex->next == token_kind::tk_IN || lex->next == token_kind::tk_colon,
					"\"in\" or a colon is required.\r\n");
				lex->advance();

				//The array
				parse_expression(block, lex);

				parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
				lex->advance();

				if (lex->next == token_kind::tk_LOOP) lex->advance();

				//The counter
				block->codes.push_back(code(lex->line, command_kind::pc_push_value,
					value(engine->get_real_type(), 0.0)));

				size_t ip = block->codes.size();

				block->codes.push_back(code(lex->line, command_kind::pc_for_each_and_push_first));

				script_engine::block* b = engine->new_block(block->level + 1, block_kind::bk_loop);
				std::vector<std::string> counter;
				counter.push_back(feIdentifier);
				parse_block(b, lex, &counter, false, true);

				//block->codes.push_back(code(lex->line, command_kind::pc_dup_n, 1));
				block->codes.push_back(code(lex->line, command_kind::pc_call, b, 1));
				block->codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));

				//Pop twice for the array and the counter
				block->codes.push_back(code(lex->line, command_kind::pc_pop, 2));

				if (b->codes.size() == 1U) {	//1 for pc_assign
					engine->blocks.pop_back();
					while (block->codes.size() > ip_for_begin)
						block->codes.pop_back();
				}
			}
			else if (lex->next == token_kind::tk_open_par) {	//Regular for loop
				lex->advance();

				bool isNewVar = false;
				bool isNewVarConst = false;
				std::string newVarName = "";
				script_scanner lex_s1(*lex);
				if (IsDeclToken(lex->next) || lex->next == token_kind::tk_const) {
					isNewVar = true;
					isNewVarConst = lex->next == token_kind::tk_const;
					lex->advance();
					parser_assert(lex->next == token_kind::tk_word, "Variable name is required.\r\n");
					newVarName = lex->word;
				}

				while (lex->next != token_kind::tk_semicolon) lex->advance();
				lex->advance();

				bool hasExpr = true;
				if (lex->next == token_kind::tk_semicolon) hasExpr = false;

				script_scanner lex_s2(*lex);

				while (lex->next != token_kind::tk_semicolon) lex->advance();
				lex->advance();

				script_scanner lex_s3(*lex);

				while (lex->next != token_kind::tk_semicolon && lex->next != token_kind::tk_close_par) lex->advance();
				lex->advance();
				if (lex->next == token_kind::tk_close_par) lex->advance();

				script_engine::block* forBlock = engine->new_block(block->level + 1, block_kind::bk_normal);
				block->codes.push_back(code(lex->line, command_kind::pc_call, forBlock, 0));

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

					forBlock->codes.push_back(code(lex->line, command_kind::pc_var_alloc, 1));
					parse_statements(forBlock, &lex_s1, token_kind::tk_semicolon, true);

					//hahaha what
					script_engine::block tmpEvalBlock(block->level + 1, block_kind::bk_normal);
					parse_statements(&tmpEvalBlock, &lex_s3, token_kind::tk_comma, true);
					while (lex_s3.next == token_kind::tk_comma) {
						lex_s3.advance();
						parse_statements(&tmpEvalBlock, &lex_s3, token_kind::tk_comma, true);
					}
					lex->copy_state(lex_s3);
					lex->advance();

					size_t ip_begin = forBlock->codes.size();

					//Code block
					if (hasExpr) {
						parse_expression(forBlock, &lex_s2);
						forBlock->codes.push_back(code(lex_s2.line, command_kind::pc_loop_if));
					}

					//Parse the code contained inside the for loop
					codeBlockSize = parse_inline_block(nullptr, forBlock, lex, block_kind::bk_loop, true);

					forBlock->codes.push_back(code(lex_s2.line, command_kind::pc_continue_marker));
					{
						for (auto itr = tmpEvalBlock.codes.begin(); itr != tmpEvalBlock.codes.end(); ++itr)
							forBlock->codes.push_back(*itr);
					}
					forBlock->codes.push_back(code(lex_s2.line, command_kind::pc_loop_back, ip_begin));
				}
				frame.pop_back();

				if (codeBlockSize == 0U) {
					engine->blocks.pop_back();	//forBlock
					while (block->codes.size() > ip_for_begin)
						block->codes.pop_back();
				}
			}
			else {
				parser_assert(false, "\"(\" is required.\r\n");
			}

			need_semicolon = false;
			break;
		}
		case token_kind::tk_ASCENT:
		case token_kind::tk_DESCENT:
		{
			bool isAscent = lex->next == token_kind::tk_ASCENT;
			lex->advance();

			parser_assert(lex->next == token_kind::tk_open_par, "\"(\" is required.\r\n");
			lex->advance();

			if (IsDeclToken(lex->next)) lex->advance();
			else if (lex->next == token_kind::tk_const)
				parser_assert(false, "The counter variable cannot be const.\r\n");

			parser_assert(lex->next == token_kind::tk_word, "Variable name is required.\r\n");

			std::string counterName = lex->word;

			lex->advance();

			parser_assert(lex->next == token_kind::tk_IN, "\"in\" is required.\r\n");
			lex->advance();

			size_t ip_ascdsc_begin = block->codes.size();

			script_engine::block* containerBlock = engine->new_block(block->level + 1, block_kind::bk_normal);
			block->codes.push_back(code(lex->line, command_kind::pc_call, containerBlock, 0));

			frame.push_back(scope_t(containerBlock->kind));
			{
				auto InsertSymbol = [&](size_t var, std::string name, bool isInternal) {
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

				containerBlock->codes.push_back(code(lex->line, command_kind::pc_var_alloc, 3));

				parse_expression(containerBlock, lex);	//First value, to s_0 if ascent
				containerBlock->codes.push_back(code(lex->line, command_kind::pc_assign,
					containerBlock->level, (int)(!isAscent), "!0"));

				parser_assert(lex->next == token_kind::tk_range, "\"..\" is required.\r\n");
				lex->advance();

				parse_expression(containerBlock, lex);	//Second value, to s_0 if descent
				containerBlock->codes.push_back(code(lex->line, command_kind::pc_assign,
					containerBlock->level, (int)(isAscent), "!1"));

				parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
				lex->advance();

				if (lex->next == token_kind::tk_LOOP) {
					lex->advance();
				}

				size_t ip = containerBlock->codes.size();

				containerBlock->codes.push_back(code(lex->line, command_kind::pc_push_variable,
					containerBlock->level, 0, "!0"));
				containerBlock->codes.push_back(code(lex->line, command_kind::pc_push_variable,
					containerBlock->level, 1, "!1"));
				containerBlock->codes.push_back(code(lex->line, isAscent ? command_kind::pc_compare_and_loop_ascent :
					command_kind::pc_compare_and_loop_descent));

				if (!isAscent) {
					containerBlock->codes.push_back(code(lex->line, command_kind::pc_inline_dec,
						containerBlock->level, 0, "!0"));
				}

				//Copy s_0 to s_2
				containerBlock->codes.push_back(code(lex->line, command_kind::pc_push_variable,
					containerBlock->level, 0, "!0"));
				containerBlock->codes.push_back(code(lex->line, command_kind::pc_assign,
					containerBlock->level, 2, counterName));

				//Parse the code contained inside the loop
				size_t codeBlockSize = parse_inline_block(nullptr, containerBlock, lex, block_kind::bk_loop, true);

				containerBlock->codes.push_back(code(lex->line, command_kind::pc_continue_marker));

				if (isAscent) {
					containerBlock->codes.push_back(code(lex->line, command_kind::pc_inline_inc,
						containerBlock->level, 0, "!0"));
				}

				containerBlock->codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));

				if (codeBlockSize == 0U) {
					engine->blocks.pop_back();	//containerBlock
					while (block->codes.size() > ip_ascdsc_begin)
						block->codes.pop_back();
				}
			}
			frame.pop_back();

			need_semicolon = false;
			break;
		}
		case token_kind::tk_IF:
		{
			std::map<size_t, size_t> mapLabelCode;

			size_t ip_begin = block->codes.size();

			size_t indexLabel = 0;
			bool hasNextCase = false;
			do {
				hasNextCase = false;
				parser_assert(indexLabel < 0xffffff, "Too many conditional cases.");
				lex->advance();

				//Jump to next case if false
				parse_parentheses(block, lex);
				block->codes.push_back(code(lex->line, command_kind::pc_jump_if_not, indexLabel));
				parse_inline_block(nullptr, block, lex, block_kind::bk_normal, true);

				if (lex->next == token_kind::tk_ELSE) {
					lex->advance();

					//Jump to exit
					block->codes.push_back(code(lex->line, command_kind::pc_loop_back, UINT_MAX));

					mapLabelCode.insert(std::make_pair(indexLabel, block->codes.size()));
					if (lex->next != token_kind::tk_IF) {
						parse_inline_block(nullptr, block, lex, block_kind::bk_normal, true);
						hasNextCase = false;
					}
					else if (lex->next == token_kind::tk_IF) hasNextCase = true;
				}
				++indexLabel;
			} while (hasNextCase);

			size_t ip_end = block->codes.size();
			mapLabelCode.insert(std::make_pair(UINT_MAX, ip_end));

			//Replace labels with actual code offset
			for (; ip_begin < ip_end; ++ip_begin) {
				code* c = &block->codes[ip_begin];
				if (c->command == command_kind::pc_jump_if || c->command == command_kind::pc_jump_if_not
					|| c->command == command_kind::pc_loop_back) {
					auto itrMap = mapLabelCode.find(c->ip);
					c->ip = (itrMap != mapLabelCode.end()) ? itrMap->second : ip_end;
				}
			}

			need_semicolon = false;
			break;
		}
		case token_kind::tk_ALTERNATIVE:
		{
			std::map<size_t, size_t> mapLabelCode;

			lex->advance();
			parse_parentheses(block, lex);

			size_t ip_begin = block->codes.size();

			size_t indexLabel = 0;
			while (lex->next == token_kind::tk_CASE) {
				//Too many, man. Too many.
				parser_assert(indexLabel < 0x1000000, "Too many conditional cases.");

				lex->advance();
				parser_assert(lex->next == token_kind::tk_open_par, "\"(\" is required.\r\n");

				do {
					lex->advance();
					block->codes.push_back(code(lex->line, command_kind::pc_dup_n, 1));
					parse_expression(block, lex);
					block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_e));
					block->codes.push_back(code(lex->line, command_kind::pc_jump_if, indexLabel));
				} while (lex->next == token_kind::tk_comma);

				parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
				lex->advance();

				//Skip to next case
				block->codes.push_back(code(lex->line, command_kind::pc_loop_back, indexLabel + 1));

				mapLabelCode.insert(std::make_pair(indexLabel, block->codes.size()));
				parse_inline_block(nullptr, block, lex, block_kind::bk_normal, true);

				//Jump to exit
				block->codes.push_back(code(lex->line, command_kind::pc_loop_back, UINT_MAX));

				mapLabelCode.insert(std::make_pair(indexLabel + 1, block->codes.size()));
				indexLabel += 2;
			}
			if (lex->next == token_kind::tk_OTHERS) {
				lex->advance();
				parse_inline_block(nullptr, block, lex, block_kind::bk_normal, true);
			}
			block->codes.push_back(code(lex->line, command_kind::pc_pop, 1));

			size_t ip_end = block->codes.size();
			mapLabelCode.insert(std::make_pair(UINT_MAX, ip_end));

			//Replace labels with actual code offset
			for (; ip_begin < ip_end; ++ip_begin) {
				code* c = &block->codes[ip_begin];
				if (c->command == command_kind::pc_jump_if || c->command == command_kind::pc_jump_if_not
					|| c->command == command_kind::pc_loop_back) {
					auto itrMap = mapLabelCode.find(c->ip);
					c->ip = (itrMap != mapLabelCode.end()) ? itrMap->second : ip_end;
				}
			}

			need_semicolon = false;
			break;
		}
		case token_kind::tk_BREAK:
		case token_kind::tk_CONTINUE:
		{
			command_kind c = lex->next == token_kind::tk_BREAK ?
				command_kind::pc_break_loop : command_kind::pc_loop_continue;
			lex->advance();
			block->codes.push_back(code(lex->line, c));
			break;
		}
		case token_kind::tk_RETURN:
			lex->advance();

			switch (lex->next) {
			case token_kind::tk_end:
			case token_kind::tk_invalid:
			case token_kind::tk_semicolon:
			case token_kind::tk_close_cur:
				break;
			default:
			{
				parse_expression(block, lex);
				symbol* s = search_result();
				parser_assert(s, "Only functions may return values.\r\n");

				block->codes.push_back(code(lex->line, command_kind::pc_assign, s->level, s->variable,
					"[function_result]"));
			}
			}
			block->codes.push_back(code(lex->line, command_kind::pc_break_routine));

			break;
		case token_kind::tk_YIELD:
		case token_kind::tk_WAIT:
		{
			command_kind c = lex->next == token_kind::tk_YIELD ? command_kind::pc_yield : command_kind::pc_wait;
			lex->advance();
			if (c == command_kind::pc_wait) parse_parentheses(block, lex);
			block->codes.push_back(code(lex->line, c));
			break;
		}
		case token_kind::tk_at:
		case token_kind::tk_SUB:
		case token_kind::tk_FUNCTION:
		case token_kind::tk_TASK:
		{
			token_kind token = lex->next;

			lex->advance();
			if (lex->next != token_kind::tk_word) {
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

			std::string funcName = lex->word;
			scope_t* pScope = nullptr;

			symbol* s = search(funcName, &pScope);

			if (token == token_kind::tk_at) {
				parser_assert(s->sub->level <= 1, "An event(\'@\') block cannot exist here.\r\n");

				events[funcName] = s->sub;
			}

			lex->advance();

			std::vector<std::string> args;
			if (s->sub->kind != block_kind::bk_sub) {
				if (lex->next == token_kind::tk_open_par) {
					lex->advance();
					while (lex->next == token_kind::tk_word || IsDeclToken(lex->next)) {
						if (IsDeclToken(lex->next)) {
							lex->advance();
							parser_assert(lex->next == token_kind::tk_word, "Parameter name is required.\r\n");
						}
						//TODO: Give scripters access to defining functions with variadic argument counts. (After 1.10a)
						args.push_back(lex->word);
						lex->advance();
						if (lex->next != token_kind::tk_comma)
							break;
						lex->advance();
					}
					parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
					lex->advance();
				}
			}
			else {
				if (lex->next == token_kind::tk_open_par) {
					lex->advance();
					parser_assert(lex->next == token_kind::tk_close_par, "Only an empty parameter list is allowed here.\r\n");
					lex->advance();
				}
			}

			s = search_in(pScope, funcName, args.size());

			parse_block(s->sub, lex, &args, s->sub->kind == block_kind::bk_function, false);

			need_semicolon = false;
			break;
		}
		}

		//ƒZƒ~ƒRƒƒ“‚ª–³‚¢‚ÆŒp‘±‚µ‚È‚¢
		if (need_semicolon && lex->next != statement_terminator)
			break;

		if (lex->next == statement_terminator)
			lex->advance();

		if (single_parse) break;
	}
}

size_t parser::parse_inline_block(script_engine::block** blockRes, script_engine::block* block, script_scanner* lex,
	script_engine::block_kind kind, bool allow_single) 
{
	script_engine::block* b = engine->new_block(block->level + 1, kind);
	if (blockRes) *blockRes = b;

	parse_block(b, lex, nullptr, false, allow_single);

	if (b->codes.size() == 0U) {
		engine->blocks.pop_back();
		return 0;
	}

	block->codes.push_back(code(lex->line, command_kind::pc_call, b, 0));
	return b->codes.size();
}

void parser::parse_block(script_engine::block* block, script_scanner* lex, std::vector<std::string> const* args, 
	bool adding_result, bool allow_single) 
{
	bool single_line = lex->next != token_kind::tk_open_cur && allow_single;
	if (!single_line) {
		parser_assert(lex->next == token_kind::tk_open_cur, "\"{\" is required.\r\n");
		lex->advance();
	}

	frame.push_back(scope_t(block->kind));

	if (!single_line) {
		int countVar = scan_current_scope(block->level, args, adding_result);
		if (countVar > 0)
			block->codes.push_back(code(lex->line, command_kind::pc_var_alloc, countVar));
	}
	if (args) {
		scope_t* ptrBackFrame = &frame.back();

		for (size_t i = 0; i < args->size(); ++i) {
			const std::string& name = (*args)[i];
			if (single_line) {	//As scan_current_scope won't be called.
				symbol s = { block->level, nullptr, (int)i, false, true };
				ptrBackFrame->singular_insert(name, s);
			}

			symbol* s = search(name);
			block->codes.push_back(code(lex->line, command_kind::pc_assign, s->level, s->variable, name));
		}
	}
	parse_statements(block, lex, token_kind::tk_semicolon, single_line);

	frame.pop_back();

	if (!single_line) {
		parser_assert(lex->next == token_kind::tk_close_cur, "\"}\" is required.\r\n");
		lex->advance();
	}
}

void parser::optimize_expression(script_engine::block* blockDst, script_engine::block* blockSrc) {
	using command_kind = script_engine::command_kind;

	for (auto iSrcCode = blockSrc->codes.begin(); iSrcCode != blockSrc->codes.end(); ++iSrcCode) {
		switch (iSrcCode->command) {
		case command_kind::pc_inline_neg:
		case command_kind::pc_inline_not:
		case command_kind::pc_inline_abs:
		{
			code* ptrBack = &blockDst->codes.back();
			if (ptrBack->command == command_kind::pc_push_value) {
				value* arg = &(ptrBack->data);
				value res;
				switch (iSrcCode->command) {
				case command_kind::pc_inline_neg:
					res = _script_negative(1, arg);
					break;
				case command_kind::pc_inline_not:
					res = _script_not_(1, arg);
					break;
				case command_kind::pc_inline_abs:
					res = _script_absolute(1, arg);
					break;
				}
				blockDst->codes.pop_back();
				blockDst->codes.push_back(code(iSrcCode->line, command_kind::pc_push_value, res));
			}
			else {
				blockDst->codes.push_back(*iSrcCode);
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
			code* ptrBack = &blockDst->codes.back();
			if (ptrBack[-1].command == command_kind::pc_push_value && ptrBack->command == command_kind::pc_push_value) {
				value arg[] = { ptrBack[-1].data, ptrBack->data };
				value res;
				switch (iSrcCode->command) {
				case command_kind::pc_inline_add:
					res = _script_add(2, arg);
					break;
				case command_kind::pc_inline_sub:
					res = _script_subtract(2, arg);
					break;
				case command_kind::pc_inline_mul:
					res = _script_multiply(2, arg);
					break;
				case command_kind::pc_inline_div:
					res = _script_divide(2, arg);
					break;
				case command_kind::pc_inline_mod:
					res = _script_remainder_(2, arg);
					break;
				case command_kind::pc_inline_pow:
					res = _script_power(2, arg);
					break;
				}
				blockDst->codes.pop_back();
				blockDst->codes.pop_back();
				blockDst->codes.push_back(code(iSrcCode->line, command_kind::pc_push_value, res));
			}
			else {
				blockDst->codes.push_back(*iSrcCode);
			}
			break;
		}
		default:
			blockDst->codes.push_back(*iSrcCode);
			break;
		}
	}
}

/* script_type_manager */

script_type_manager* script_type_manager::base_ = nullptr;
script_type_manager::script_type_manager() {
	if (base_) return;
	base_ = this;

	int_type = deref_itr(types.insert(type_data(type_data::type_kind::tk_int)).first);
	real_type = deref_itr(types.insert(type_data(type_data::type_kind::tk_real)).first);
	char_type = deref_itr(types.insert(type_data(type_data::type_kind::tk_char)).first);
	boolean_type = deref_itr(types.insert(type_data(type_data::type_kind::tk_boolean)).first);
	string_type = deref_itr(types.insert(type_data(type_data::type_kind::tk_array,
		char_type)).first);		//Char array (string)
	real_array_type = deref_itr(types.insert(type_data(type_data::type_kind::tk_array,
		real_type)).first);		//Real array	

	//Bool array
	types.insert(type_data(type_data::type_kind::tk_array, boolean_type));
	//String array (Might not really be all that necessary to initialize it here)
	types.insert(type_data(type_data::type_kind::tk_array, string_type));
}

type_data* script_type_manager::get_array_type(type_data* element) {
	type_data target = type_data(type_data::type_kind::tk_array, element);
	auto itr = types.find(target);
	if (itr == types.end()) {
		//No types found, insert and return the new type
		itr = types.insert(target).first;
	}
	return deref_itr(itr);
}

/* script_engine */

script_engine::script_engine(std::string const& source, int funcc, function const* funcv) {
	main_block = new_block(0, block_kind::bk_normal);

	const char* end = &source[0] + source.size();
	script_scanner s(source.c_str(), end);
	parser p(this, &s, funcc, funcv);

	events = p.events;

	error = p.error;
	error_message = p.error_message;
	error_line = p.error_line;
}

script_engine::script_engine(std::vector<char> const& source, int funcc, function const* funcv) {
	main_block = new_block(0, block_kind::bk_normal);

	if (false) {
		wchar_t* pStart = (wchar_t*)&source[0];
		wchar_t* pEnd = (wchar_t*)(&source[0] + std::min(source.size(), 64U));
		std::wstring str = std::wstring(pStart, pEnd);
		//Logger::WriteTop(str);
	}
	const char* end = &source[0] + source.size();
	script_scanner s(&source[0], end);
	parser p(this, &s, funcc, funcv);

	events = p.events;

	error = p.error;
	error_message = p.error_message;
	error_line = p.error_line;
}

script_engine::~script_engine() {
	blocks.clear();
}

/* script_machine */

script_machine::script_machine(script_engine* the_engine) {
	assert(!the_engine->get_error());
	engine = the_engine;

	error = false;
	bTerminate = false;
}

script_machine::~script_machine() {
}

script_machine::environment::environment(std::shared_ptr<environment> parent, script_engine::block* b) {
	this->parent = parent;
	this->sub = b;
	this->ip = 0;
	this->variables.clear();
	this->stack.clear();
	this->has_result = false;
	this->waitCount = 0;
}
script_machine::environment::~environment() {
	this->variables.clear();
	this->stack.clear();
}

void script_machine::run() {
	if (bTerminate) return;

	assert(!error);
	if (threads.size() == 0) {
		error_line = -1;

		threads.push_back(std::make_shared<environment>(nullptr, engine->main_block));
		current_thread_index = threads.begin();

		finished = false;
		stopped = false;
		resuming = false;

		run_code();
	}
}

void script_machine::resume() {
	if (bTerminate) return;

	assert(!error);
	assert(stopped);

	stopped = false;
	finished = false;
	resuming = true;

	run_code();
}

void script_machine::call(std::string event_name) {
	call(engine->events.find(event_name));
}
void script_machine::call(std::map<std::string, script_engine::block*>::iterator event_itr) {
	if (bTerminate) return;

	assert(!error);
	assert(!stopped);
	if (event_itr != engine->events.end()) {
		run();	//”O‚Ì‚½‚ß

		auto prev_thread = current_thread_index;
		current_thread_index = threads.begin();

		environment_ptr& env_first = *current_thread_index;
		env_first = std::make_shared<environment>(env_first, event_itr->second);

		finished = false;

		environment_ptr epp = env_first->parent->parent;
		call_start_parent_environment_list.push_back(epp);
		run_code();
		call_start_parent_environment_list.pop_back();
		finished = false;

		current_thread_index = prev_thread;
	}
}

void script_machine::run_code() {
	typedef script_engine::command_kind command_kind;
	typedef script_engine::block_kind block_kind;
	typedef type_data::type_kind type_kind;

	environment_ptr current = *current_thread_index;

	while (!finished && !bTerminate) {
		if (current->waitCount > 0) {
			yield();
			--current->waitCount;
			current = *current_thread_index;
			continue;
		}

		if (current->ip >= current->sub->codes.size()) {
			environment_ptr parent = current->parent;

			bool bFinish = false;
			if (parent == nullptr)
				bFinish = true;
			else if (call_start_parent_environment_list.size() > 1 && parent->parent != nullptr) {
				environment_ptr env = *call_start_parent_environment_list.rbegin();
				if (parent->parent == env)
					bFinish = true;
			}

			if (bFinish) {
				finished = true;
			}
			else {
				if (current->sub->kind == block_kind::bk_microthread) {
					current_thread_index = threads.erase(current_thread_index);
					yield();
					current = *current_thread_index;
				}
				else {
					if (current->has_result && parent != nullptr)
						parent->stack.push_back(current->variables[0]);
					*current_thread_index = parent;
					current = parent;
				}
			}
		}
		else {
			script_engine::code* c = &(current->sub->codes[current->ip]);
			error_line = c->line;
			++(current->ip);

			switch (c->command) {
			case command_kind::pc_wait:
			{
				stack_t& stack = current->stack;
				value* t = &(stack.back());
				current->waitCount = (int)(t->as_real() + 0.01) - 1;
				stack.pop_back();
				if (current->waitCount < 0) break;
			}
			//Fallthrough
			case command_kind::pc_yield:
				yield();
				current = *current_thread_index;
				break;

			case command_kind::pc_var_alloc:
				current->variables.resize(c->ip);
				break;

			case command_kind::pc_loop_back:
				current->ip = c->ip;
				break;
			case command_kind::pc_jump_if:
			case command_kind::pc_jump_if_not:
			{
				stack_t& stack = current->stack;
				value* top = &stack.back();
				if ((c->command == command_kind::pc_jump_if && top->as_boolean())
					|| (c->command == command_kind::pc_jump_if_not && !top->as_boolean()))
					current->ip = c->ip;
				stack.pop_back();
				break;
			}
			case command_kind::pc_jump_diff:
				current->ip += c->level - 1;
				break;
			case command_kind::pc_jump_if_diff:
			case command_kind::pc_jump_if_not_diff:
			{
				stack_t& stack = current->stack;
				value* top = &stack.back();
				if ((c->command == command_kind::pc_jump_if_diff && top->as_boolean())
					|| (c->command == command_kind::pc_jump_if_not_diff && !top->as_boolean()))
					current->ip += c->level - 1;
				stack.pop_back();
				break;
			}

			case command_kind::pc_assign:
			{
				stack_t& stack = current->stack;
				assert(stack.size() > 0);

				for (environment* i = current.get(); i != nullptr; i = (i->parent).get()) {
					if (i->sub->level == c->level) {
						variables_t& vars = i->variables;

						//Should never happen with pc_var_alloc, but it's here as a failsafe anyway
						if (vars.capacity <= c->variable) {
							vars.resize(c->variable + 4);
						}

						value* dest = &(vars[c->variable]);
						value* src = &stack.back();

						if (dest->has_data() && dest->get_type() != src->get_type()
							&& !(dest->get_type()->get_kind() == type_kind::tk_array
								&& src->get_type()->get_kind() == type_kind::tk_array
								&& (dest->length_as_array() > 0 || src->length_as_array() > 0))) {
							raise_error(L"A variable cannot change its value type.\r\n");
							break;
						}

						*dest = *src;
						dest->unique();
						stack.pop_back();

						break;
					}
				}

				break;
			}
			case command_kind::pc_assign_writable:
			{
				stack_t& stack = current->stack;
				assert(stack.size() >= 2);

				value* dest = &stack[stack.size() - 2];
				value* src = &stack[stack.size() - 1];

				if (dest->has_data() && dest->get_type() != src->get_type()
					&& !(dest->get_type()->get_kind() == type_kind::tk_array
						&& src->get_type()->get_kind() == type_kind::tk_array
						&& (dest->length_as_array() > 0 || src->length_as_array() > 0))) {
					raise_error(L"A variable cannot change its value type.\r\n");
				}
				else {
					dest->overwrite(*src);
					stack.pop_back(2U);
				}

				break;
			}
			case command_kind::pc_continue_marker:	//Dummy token for pc_loop_continue
				break;
			case command_kind::pc_loop_continue:
			case command_kind::pc_break_loop:
			case command_kind::pc_break_routine:
				for (environment* i = current.get(); i != nullptr; i = (i->parent).get()) {
					i->ip = i->sub->codes.size();

					if (c->command == command_kind::pc_break_loop || c->command == command_kind::pc_loop_continue) {
						bool exit = false;
						switch (i->sub->kind) {
						case block_kind::bk_loop:
						{
							command_kind targetCommand = command_kind::pc_loop_back;
							if (c->command == command_kind::pc_loop_continue)
								targetCommand = command_kind::pc_continue_marker;

							environment* e = (i->parent).get();
							if (e) {
								do
									++(e->ip);
								while (e->sub->codes[e->ip - 1].command != targetCommand);
							}
							else exit = true;
						}
						case block_kind::bk_microthread:	//Prevents catastrophes.
						case block_kind::bk_function:
						//case block_kind::bk_normal:
							exit = true;
						default:
							break;
						}
						if (exit) break;
					}
					else {	//pc_break_routine
						if (i->sub->kind == block_kind::bk_sub || i->sub->kind == block_kind::bk_function
							|| i->sub->kind == block_kind::bk_microthread)
							break;
						else if (i->sub->kind == block_kind::bk_loop)
							i->parent->stack.clear();
					}
				}
				break;
			case command_kind::pc_call:
			case command_kind::pc_call_and_push_result:
			{
				stack_t& current_stack = current->stack;
				//assert(current_stack.size() >= c->arguments);

				if (current_stack.size() < c->arguments) {
					std::wstring error = StringUtility::FormatToWide(
						"Unexpected script error: Stack size[%d] is less than the number of arguments[%d].\r\n",
						current_stack.size(), c->arguments);
					raise_error(error);
					break;
				}

				if (c->sub->func) {
					//Default functions

					size_t sizePrev = current_stack.size();

					value* argv = nullptr;
					if (sizePrev > 0 && c->arguments > 0)
						argv = current_stack.at + (sizePrev - c->arguments);
					value ret = c->sub->func(this, c->arguments, argv);

					if (stopped) {
						--(current->ip);
					}
					else {
						resuming = false;

						//Erase the passed arguments from the stack
						current_stack.pop_back(c->arguments);

						//Return value
						if (c->command == command_kind::pc_call_and_push_result)
							current_stack.push_back(ret);
					}
				}
				else if (c->sub->kind == block_kind::bk_microthread) {
					//Tasks
					environment_ptr e = std::make_shared<environment>(current, c->sub);
					threads.insert(++current_thread_index, e);
					--current_thread_index;

					//Pass the arguments
					for (size_t i = 0; i < c->arguments; ++i) {
						e->stack.push_back(current_stack.back());
						current_stack.pop_back();
					}

					current = *current_thread_index;
				}
				else {
					//User-defined functions or internal blocks
					environment_ptr e = std::make_shared<environment>(current, c->sub);
					e->has_result = c->command == command_kind::pc_call_and_push_result;
					*current_thread_index = e;

					//Pass the arguments
					for (size_t i = 0; i < c->arguments; ++i) {
						e->stack.push_back(current_stack.back());
						current_stack.pop_back();
					}

					current = e;
				}

				break;
			}

			case command_kind::pc_compare_e:
			case command_kind::pc_compare_g:
			case command_kind::pc_compare_ge:
			case command_kind::pc_compare_l:
			case command_kind::pc_compare_le:
			case command_kind::pc_compare_ne:
			{
				stack_t& stack = current->stack;
				value& t = stack.back();
				int r = t.as_int();
				bool b = false;
				switch (c->command) {
				case command_kind::pc_compare_e:
					b = r == 0;
					break;
				case command_kind::pc_compare_g:
					b = r > 0;
					break;
				case command_kind::pc_compare_ge:
					b = r >= 0;
					break;
				case command_kind::pc_compare_l:
					b = r < 0;
					break;
				case command_kind::pc_compare_le:
					b = r <= 0;
					break;
				case command_kind::pc_compare_ne:
					b = r != 0;
					break;
				}
				t.set(engine->get_boolean_type(), b);

				break;
			}
			case command_kind::pc_dup_n:
			{
				stack_t& stack = current->stack;
				size_t len = stack.size();
				assert(c->ip <= len);
				stack.push_back(stack[len - c->ip]);
				break;
			}
			case command_kind::pc_swap:
			{
				size_t len = current->stack.size();
				assert(len >= 2);
				value tmp = current->stack[len - 1];
				current->stack[len - 1] = current->stack[len - 2];
				current->stack[len - 2] = tmp;

				break;
			}
			case command_kind::pc_compare_and_loop_ascent:
			case command_kind::pc_compare_and_loop_descent:
			{
				stack_t& stack = current->stack;

				value* cmp_arg = (value*)(&stack.back() - 1);
				value cmp_res = compare(this, 2, cmp_arg);

				bool is_skip = c->command == command_kind::pc_compare_and_loop_ascent ?
					(cmp_res.as_int() >= 0) : (cmp_res.as_int() <= 0);
				if (is_skip) {
					do
						++(current->ip);
					while (current->sub->codes[current->ip - 1].command != command_kind::pc_loop_back);
				}

				current->stack.pop_back(2U);
				break;
			}
			case command_kind::pc_loop_count:
			{
				stack_t& stack = current->stack;
				value* i = &stack.back();
				assert(i->get_type()->get_kind() == type_kind::tk_real);
				float r = i->as_real();
				if (r > 0)
					i->set(engine->get_real_type(), r - 1);
				else {
					do
						++(current->ip);
					while (current->sub->codes[current->ip - 1].command != command_kind::pc_loop_back);
				}

				break;
			}
			case command_kind::pc_loop_if:
			{
				stack_t& stack = current->stack;
				bool c = stack.back().as_boolean();
				current->stack.pop_back();
				if (!c) {
					do
						++(current->ip);
					while (current->sub->codes[current->ip - 1].command != command_kind::pc_loop_back);
				}

				break;
			}
			case command_kind::pc_for_each_and_push_first:
			{
				//Stack: .... [array] [counter]

				stack_t& stack = current->stack;

				value* src_array = &stack.back() - 1;
				value* i = &stack.back();

				std::vector<value>::iterator itrCur = src_array->array_get_begin() + (ptrdiff_t)(i->as_real());
				std::vector<value>::iterator itrEnd = src_array->array_get_end();

				if (src_array->get_type()->get_kind() != type_kind::tk_array || itrCur >= itrEnd) {
					do
						++(current->ip);
					while (current->sub->codes[current->ip - 1].command != command_kind::pc_loop_back);
				}
				else {
					current->stack.push_back(value::new_from(*itrCur));
					i->set(i->get_type(), i->as_real() + 1);
				}

				break;
			}

			case command_kind::pc_construct_array:
			{
				if (c->ip == 0U) {
					current->stack.push_back(value(engine->get_string_type(), 0.0));
					break;
				}

				std::vector<value> res_arr;
				res_arr.resize(c->ip);

				value* val_ptr = &current->stack.back() - c->ip + 1;

				type_data* type_arr = engine->get_array_type(val_ptr->get_type());

				value res;
				for (size_t iVal = 0U; iVal < c->ip; ++iVal, ++val_ptr) {
					this->append_check(iVal, type_arr, val_ptr->get_type());
					res_arr[iVal] = *val_ptr;
				}
				res.set(type_arr, res_arr);

				current->stack.pop_back(c->ip);
				current->stack.push_back(res);
				break;
			}

			case command_kind::pc_pop:
				current->stack.pop_back(c->ip);
				break;
			case command_kind::pc_push_value:
				current->stack.push_back(c->data);
				break;
			case command_kind::pc_push_variable:
			//case command_kind::pc_push_variable_unique:
			{
				value* var = find_variable_symbol(current.get(), c);
				if (var == nullptr) break;
				//if (c->command == command_kind::pc_push_variable_unique)
				//	var->unique();
				current->stack.push_back(*var);
				break;
			}

			//----------------------------------Inline operations----------------------------------
			case command_kind::pc_inline_inc:
			case command_kind::pc_inline_dec:
			{
				//A hack for assign_writable's, since symbol::level can't normally go below 0 anyway
				if (c->level >= 0) {
					value* var = find_variable_symbol(current.get(), c);
					if (var == nullptr) break;

					value res = (c->command == command_kind::pc_inline_inc) ? 
						successor(this, 1, var) : predecessor(this, 1, var);
					*var = res;
				}
				else {
					value* var = &(current->stack.back());

					value res = (c->command == command_kind::pc_inline_inc) ? 
						successor(this, 1, var) : predecessor(this, 1, var);
					if (!res.has_data()) break;
					var->overwrite(res);
					current->stack.pop_back();
				}
				break;
			}
			case command_kind::pc_inline_add_asi:
			case command_kind::pc_inline_sub_asi:
			case command_kind::pc_inline_mul_asi:
			case command_kind::pc_inline_div_asi:
			case command_kind::pc_inline_mod_asi:
			case command_kind::pc_inline_pow_asi:
			{
				if (c->level >= 0) {
					value* var = find_variable_symbol(current.get(), c);
					if (var == nullptr) break;

					value tmp[2] = { *var, current->stack.back() };
					value res;

#define DEF_CASE(cmd, fn) case cmd: res = fn(this, 2, tmp); break;
					switch (c->command) {
						DEF_CASE(command_kind::pc_inline_add_asi, add);
						DEF_CASE(command_kind::pc_inline_sub_asi, subtract);
						DEF_CASE(command_kind::pc_inline_mul_asi, multiply);
						DEF_CASE(command_kind::pc_inline_div_asi, divide);
						DEF_CASE(command_kind::pc_inline_mod_asi, remainder_);
						DEF_CASE(command_kind::pc_inline_pow_asi, power);
					}
#undef DEF_CASE

					current->stack.pop_back();
					*var = res;
				}
				else {
					value* dest = &(current->stack.back()) - 1;

					value arg[2] = { *dest, current->stack.back() };
					value res;

#define DEF_CASE(cmd, fn) case cmd: res = fn(this, 2, arg); break;
					switch (c->command) {
						DEF_CASE(command_kind::pc_inline_add_asi, add);
						DEF_CASE(command_kind::pc_inline_sub_asi, subtract);
						DEF_CASE(command_kind::pc_inline_mul_asi, multiply);
						DEF_CASE(command_kind::pc_inline_div_asi, divide);
						DEF_CASE(command_kind::pc_inline_mod_asi, remainder_);
						DEF_CASE(command_kind::pc_inline_pow_asi, power);
					}
#undef DEF_CASE

					dest->overwrite(res);
					current->stack.pop_back(2U);
				}
				break;
			}
			case command_kind::pc_inline_neg:
			case command_kind::pc_inline_not:
			case command_kind::pc_inline_abs:
			{
				value res;

#define DEF_CASE(cmd, fn) case cmd: res = fn(this, 1, &current->stack.back()); break;
				switch (c->command) {
					DEF_CASE(command_kind::pc_inline_neg, negative);
					DEF_CASE(command_kind::pc_inline_not, not_);
					DEF_CASE(command_kind::pc_inline_abs, absolute);
				}
#undef DEF_CASE

				current->stack.pop_back();
				current->stack.push_back(res);
				break;
			}
			case command_kind::pc_inline_add:
			case command_kind::pc_inline_sub:
			case command_kind::pc_inline_mul:
			case command_kind::pc_inline_div:
			case command_kind::pc_inline_mod:
			case command_kind::pc_inline_pow:
			case command_kind::pc_inline_app:
			case command_kind::pc_inline_cat:
			{
				value res;
				value* args = (value*)(&current->stack.back() - 1);

#define DEF_CASE(cmd, fn) case cmd: res = fn(this, 2, args); break;
				switch (c->command) {
					DEF_CASE(command_kind::pc_inline_add, add);
					DEF_CASE(command_kind::pc_inline_sub, subtract);
					DEF_CASE(command_kind::pc_inline_mul, multiply);
					DEF_CASE(command_kind::pc_inline_div, divide);
					DEF_CASE(command_kind::pc_inline_mod, remainder_);
					DEF_CASE(command_kind::pc_inline_pow, power);
					DEF_CASE(command_kind::pc_inline_app, append);
					DEF_CASE(command_kind::pc_inline_cat, concatenate);
				}
#undef DEF_CASE

				current->stack.pop_back(2U);
				current->stack.push_back(res);
				break;
			}
			case command_kind::pc_inline_cmp_e:
			case command_kind::pc_inline_cmp_g:
			case command_kind::pc_inline_cmp_ge:
			case command_kind::pc_inline_cmp_l:
			case command_kind::pc_inline_cmp_le:
			case command_kind::pc_inline_cmp_ne:
			{
				value* args = (value*)(&current->stack.back() - 1);
				value cmp_res = compare(this, 2, args);
				int cmp_r = cmp_res.as_int();

#define DEF_CASE(cmd, expr) case cmd: cmp_res.set(engine->get_boolean_type(), expr); break;
				switch (c->command) {
					DEF_CASE(command_kind::pc_inline_cmp_e, cmp_r == 0);
					DEF_CASE(command_kind::pc_inline_cmp_g, cmp_r > 0);
					DEF_CASE(command_kind::pc_inline_cmp_ge, cmp_r >= 0);
					DEF_CASE(command_kind::pc_inline_cmp_l, cmp_r < 0);
					DEF_CASE(command_kind::pc_inline_cmp_le, cmp_r <= 0);
					DEF_CASE(command_kind::pc_inline_cmp_ne, cmp_r != 0);
				}
#undef DEF_CASE

				current->stack.pop_back(2U);
				current->stack.push_back(cmp_res);
				break;
			}
			case command_kind::pc_inline_logic_and:
			case command_kind::pc_inline_logic_or:
			{
				value* var2 = &(current->stack.back());
				value* var1 = var2 - 1;

				value res;
				if (c->command == command_kind::pc_inline_logic_and) {
					res.set(engine->get_boolean_type(), var1->as_boolean() && var2->as_boolean());
				}
				else {
					res.set(engine->get_boolean_type(), var1->as_boolean() || var2->as_boolean());
				}

				current->stack.pop_back(2U);
				current->stack.push_back(res);
				break;
			}
			case command_kind::pc_inline_cast_real:
			case command_kind::pc_inline_cast_char:
			case command_kind::pc_inline_cast_bool:
			{
				value* var = &(current->stack.back());
				switch (c->command) {
				case command_kind::pc_inline_cast_real:
					var->set(engine->get_real_type(), var->as_real());
					break;
				case command_kind::pc_inline_cast_char:
				{
					var->set(engine->get_char_type(), var->as_char());
					break;
				}
				case command_kind::pc_inline_cast_bool:
					var->set(engine->get_boolean_type(), var->as_boolean());
					break;
				}
				break;
			}
			case command_kind::pc_inline_index_array:
			{
				value* var = &(current->stack.back()) - 1;
				const value& res = index(this, 2, var);
				if (c->ip) res.unique();
				current->stack.pop_back(2U);
				current->stack.push_back(res);
				break;
			}
			default:
				assert(false);
			}
		}
	}
}
value* script_machine::find_variable_symbol(environment* current_env, script_engine::code* var_data) {
	/*
	auto err_uninitialized = [&]() {
#ifdef _DEBUG
		std::wstring error = L"Variable hasn't been initialized";
		error += StringUtility::FormatToWide(": %s\r\n", var_data->var_name.c_str());
		raise_error(error);
#else
		raise_error(L"Variable hasn't been initialized.\r\n");
#endif	
	};
	*/

	for (environment* i = current_env; i != nullptr; i = (i->parent).get()) {
		if (i->sub->level == var_data->level) {
			variables_t& vars = i->variables;

			if (vars[var_data->variable].has_data())
				return &vars[var_data->variable];
			else break;
		}
	}

	raise_error(L"Variable hasn't been initialized.\r\n");
	return nullptr;
}
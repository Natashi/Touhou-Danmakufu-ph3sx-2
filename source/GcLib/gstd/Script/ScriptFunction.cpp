#include "source/GcLib/pch.h"

#include "ScriptFunction.hpp"
#include "Script.hpp"

#ifdef _MSC_VER
//#define for if(0);else for
namespace std {
	using ::wcstombs;
	using ::mbstowcs;
	using ::isalpha;
	using ::fmodl;
	using ::powl;
	using ::swprintf;
	using ::atof;
	using ::isdigit;
	using ::isxdigit;
	using ::floorl;
	using ::ceill;
	using ::fabsl;
	using ::iswdigit;
	using ::iswalpha;
}
#endif

#define SCRIPT_DECLARE_OP(F) \
value BaseFunction::F(script_machine* machine, int argc, const value* argv) { \
	try { \
		return _script_##F(argc, argv); \
	} \
	catch (std::string& err) { \
		if (machine) machine->raise_error(err); \
		else throw err; \
		return value(); \
	} \
	catch (std::wstring& err) { \
		if (machine) machine->raise_error(err); \
		else throw err; \
		return value(); \
	} \
}

namespace gstd {
	double BaseFunction::fmod2(double i, double j) {
		if (j < 0) {
			//return (i < 0) ? -(-i % -j) : (i % -j) + j;
			return (i < 0) ? -fmod(-i, -j) : fmod(i, -j) + j;
		}
		else {
			//return (i < 0) ? -(-i % j) + j : i % j;
			return (i < 0) ? -fmod(-i, j) + j : fmod(i, j);
		}
	}

	value BaseFunction::__script_perform_op_array(const value* v_left, const value* v_right, value(*func)(int, const value*)) {
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

	value BaseFunction::_script_add(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_add);
		else
			return value(script_type_manager::get_real_type(), argv[0].as_real() + argv[1].as_real());
	}
	SCRIPT_DECLARE_OP(add);

	value BaseFunction::_script_subtract(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_subtract);
		else
			return value(script_type_manager::get_real_type(), argv[0].as_real() - argv[1].as_real());
	}
	SCRIPT_DECLARE_OP(subtract);

	value BaseFunction::_script_multiply(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_multiply);
		else
			return value(script_type_manager::get_real_type(), argv[0].as_real() * argv[1].as_real());
	}
	SCRIPT_DECLARE_OP(multiply);

	value BaseFunction::_script_divide(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_divide);
		else
			return value(script_type_manager::get_real_type(), argv[0].as_real() / argv[1].as_real());
	}
	SCRIPT_DECLARE_OP(divide);

	value BaseFunction::_script_remainder_(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_remainder_);
		else
			return value(script_type_manager::get_real_type(), fmod2(argv[0].as_real(), argv[1].as_real()));
	}
	SCRIPT_DECLARE_OP(remainder_);

	value BaseFunction::_script_negative(int argc, const value* argv) {
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

	value BaseFunction::_script_power(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::type_kind::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_power);
		else
			return value(script_type_manager::get_real_type(), std::pow(argv[0].as_real(), argv[1].as_real()));
	}
	SCRIPT_DECLARE_OP(power);

	value BaseFunction::_script_compare(int argc, const value* argv) {
		if (argv[0].get_type() == argv[1].get_type()) {
			int r = 0;

			switch (argv[0].get_type()->get_kind()) {
			case type_data::type_kind::tk_real:
			{
				double a = argv[0].as_real();
				double b = argv[1].as_real();
				r = (a == b) ? 0 : (a < b) ? -1 : 1;
				break;
			}
			case type_data::type_kind::tk_char:
			{
				wchar_t a = argv[0].as_char();
				wchar_t b = argv[1].as_char();
				r = (a == b) ? 0 : (a < b) ? -1 : 1;
				break;
			}
			case type_data::type_kind::tk_boolean:
			{
				bool a = argv[0].as_boolean();
				bool b = argv[1].as_boolean();
				r = (a == b) ? 0 : (a < b) ? -1 : 1;
				break;
			}
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
				break;
			}
			default:
				assert(false);
			}
			return value(script_type_manager::get_int_type(), (int64_t)r);
		}
		else {
			throw std::string("Values of different types are being compared.\r\n");
		}
	}
	SCRIPT_DECLARE_OP(compare);

	value BaseFunction::_script_not_(int argc, const value* argv) {
		return value(script_type_manager::get_boolean_type(), !argv->as_boolean());
	}
	SCRIPT_DECLARE_OP(not_);

	value BaseFunction::modc(script_machine* machine, int argc, const value* argv) {
		double x = argv[0].as_real();
		double y = argv[1].as_real();
		return value(script_type_manager::get_real_type(), fmod(x, y));
	}

	value BaseFunction::predecessor(script_machine* machine, int argc, const value* argv) {
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
			std::string error = "This value type does not support the predecessor operation.\r\n";
			machine->raise_error(error);
			return value();
		}
		}
	}
	value BaseFunction::successor(script_machine* machine, int argc, const value* argv) {
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
			std::string error = "This value type does not support the successor operation.\r\n";
			machine->raise_error(error);
			return value();
		}
		}
	}

	value BaseFunction::length(script_machine* machine, int argc, const value* argv) {
		assert(argc == 1);
		return value(script_type_manager::get_real_type(), (double)argv->length_as_array());
	}

	bool BaseFunction::_index_check(script_machine* machine, type_data* arg0_type, size_t arg0_size, double index) {
		if (arg0_type->get_kind() != type_data::type_kind::tk_array) {
			std::string error = "This value type does not support the array index operation.\r\n";
			machine->raise_error(error);
			return false;
		}
		if (index < 0 || index >= arg0_size) {
			std::string error = "Array index out of bounds.\r\n";
			machine->raise_error(error);
			return false;
		}
		return true;
	}
	const value& BaseFunction::index(script_machine* machine, int argc, const value* argv) {
		assert(argc == 2);

		double index = argv[1].as_real();
		if (!_index_check(machine, argv->get_type(), argv->length_as_array(), index))
			return value::val_empty;

		return argv->index_as_array(index);
	}

	value BaseFunction::slice(script_machine* machine, int argc, const value* argv) {
		assert(argc == 3);

		if (argv->get_type()->get_kind() != type_data::type_kind::tk_array) {
			std::string error = "This value type does not support the array slice operation.\r\n";
			machine->raise_error(error);
			return value();
		}

		size_t index_1 = argv[1].as_real();
		size_t index_2 = argv[2].as_real();

		if ((index_2 > index_1 && (index_1 < 0 || index_2 > argv->length_as_array()))
			|| (index_2 < index_1 && (index_2 < 0 || index_1 > argv->length_as_array()))) {
			std::string error = "Array index out of bounds.\r\n";
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
	value BaseFunction::erase(script_machine* machine, int argc, const value* argv) {
		assert(argc == 2);

		if (argv->get_type()->get_kind() != type_data::type_kind::tk_array) {
			std::string error = "This value type does not support the array erase operation.\r\n";
			machine->raise_error(error);
			return value();
		}

		size_t index_1 = argv[1].as_real();
		size_t length = argv->length_as_array();

		if (index_1 >= length) {
			std::string error = "Array index out of bounds.\r\n";
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

	bool BaseFunction::_append_check(script_machine* machine, size_t arg0_size, type_data* arg0_type, type_data* arg1_type) {
		if (arg0_type->get_kind() != type_data::type_kind::tk_array) {
			machine->raise_error("This value type does not support the array append operation.\r\n");
			return false;
		}
		if (arg0_size > 0U && arg0_type->get_element() != arg1_type) {
			machine->raise_error("Variable type mismatch.\r\n");
			return false;
		}
		return true;
	}
	value BaseFunction::append(script_machine* machine, int argc, const value* argv) {
		assert(argc == 2);

		_append_check(machine, argv->length_as_array(), argv->get_type(), argv[1].get_type());

		value result = argv[0];
		result.append(script_type_manager::get_instance()->get_array_type(argv[1].get_type()), argv[1]);
		return result;
	}
	value BaseFunction::concatenate(script_machine* machine, int argc, const value* argv) {
		assert(argc == 2);

		if (argv[0].get_type()->get_kind() != type_data::type_kind::tk_array
			|| argv[1].get_type()->get_kind() != type_data::type_kind::tk_array) {
			std::string error = "This value type does not support the array concatenate operation.\r\n";
			machine->raise_error(error);
			return value();
		}

		if (argv[0].length_as_array() > 0 && argv[1].length_as_array() > 0 && argv[0].get_type() != argv[1].get_type()) {
			std::string error = "Variable type mismatch.\r\n";
			machine->raise_error(error);
			return value();
		}

		value result = argv[0];
		result.concatenate(argv[1]);
		return result;
	}

	value BaseFunction::round(script_machine* machine, int argc, const value* argv) {
		double r = std::floor(argv->as_real() + 0.5);
		return value(script_type_manager::get_real_type(), r);
	}
	value BaseFunction::truncate(script_machine* machine, int argc, const value* argv) {
		double r = argv->as_real();
		return value(script_type_manager::get_real_type(), (r > 0) ? std::floor(r) : std::ceil(r));
	}
	value BaseFunction::ceil(script_machine* machine, int argc, const value* argv) {
		return value(script_type_manager::get_real_type(), std::ceil(argv->as_real()));
	}
	value BaseFunction::floor(script_machine* machine, int argc, const value* argv) {
		return value(script_type_manager::get_real_type(), std::floor(argv->as_real()));
	}

	value BaseFunction::_script_absolute(int argc, const value* argv) {
		return value(script_type_manager::get_real_type(), std::fabs(argv->as_real()));
	}
	SCRIPT_DECLARE_OP(absolute);

	value BaseFunction::assert_(script_machine* machine, int argc, const value* argv) {
		if (!argv[0].as_boolean())
			machine->raise_error(argv[1].as_string());
		return value();
	}

	int64_t BaseFunction::bitDoubleToInt(double val) {
		if (val > INT64_MAX)
			return INT64_MAX;
		if (val < INT64_MIN)
			return INT64_MIN;
		return static_cast<int64_t>(val);
	}
	value BaseFunction::bitwiseNot(script_machine* machine, int argc, const value* argv) {
		int64_t val = bitDoubleToInt(argv[0].as_real());
		return value(script_type_manager::get_real_type(), (double)(~val));
	}
	value BaseFunction::bitwiseAnd(script_machine* machine, int argc, const value* argv) {
		int64_t val1 = bitDoubleToInt(argv[0].as_real());
		int64_t val2 = bitDoubleToInt(argv[1].as_real());
		return value(script_type_manager::get_real_type(), (double)(val1 & val2));
	}
	value BaseFunction::bitwiseOr(script_machine* machine, int argc, const value* argv) {
		int64_t val1 = bitDoubleToInt(argv[0].as_real());
		int64_t val2 = bitDoubleToInt(argv[1].as_real());
		return value(script_type_manager::get_real_type(), (double)(val1 | val2));
	}
	value BaseFunction::bitwiseXor(script_machine* machine, int argc, const value* argv) {
		int64_t val1 = bitDoubleToInt(argv[0].as_real());
		int64_t val2 = bitDoubleToInt(argv[1].as_real());
		return value(script_type_manager::get_real_type(), (double)(val1 ^ val2));
	}
	value BaseFunction::bitwiseLeft(script_machine* machine, int argc, const value* argv) {
		int64_t val1 = bitDoubleToInt(argv[0].as_real());
		size_t val2 = argv[1].as_real();
		return value(script_type_manager::get_real_type(), (double)(val1 << val2));
	}
	value BaseFunction::bitwiseRight(script_machine* machine, int argc, const value* argv) {
		int64_t val1 = bitDoubleToInt(argv[0].as_real());
		size_t val2 = argv[1].as_real();
		return value(script_type_manager::get_real_type(), (double)(val1 >> val2));
	}

	value BaseFunction::script_debugBreak(script_machine* machine, int argc, const value* argv) {
		DebugBreak();
		return value();
	}
}
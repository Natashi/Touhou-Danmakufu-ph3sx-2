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
	static inline bool _type_check_two_any(type_data* type_l, type_data* type_r, uint8_t type) {
		if (type_l != nullptr && type_r != nullptr) {
			uint8_t type_combine = (uint8_t)type_l->get_kind() | (uint8_t)type_r->get_kind();
			return (type_combine & type) != 0;
		}
		return false;
	}
	static inline const bool _is_force_convert_real(type_data* type) {
		return type->get_kind() & (type_data::tk_real | type_data::tk_array);
	}
	static const void _raise_error_unsupported(script_machine* machine, type_data* type, const std::string& op_name) {
		std::string error = StringUtility::Format("This value type does not support the %s operation: %s\r\n",
			op_name.c_str(), type_data::string_representation(type).c_str());
		machine->raise_error(error);
	}

	//-------------------------------------------------------------------------------------------

	type_data::type_kind BaseFunction::_type_test_promotion(type_data* type_l, type_data* type_r) {
		if (type_l == nullptr || type_r == nullptr) return type_data::tk_null;
		uint8_t kind_l = type_l->get_kind();
		uint8_t kind_r = type_r->get_kind();
		uint8_t type_combine = kind_l | kind_r;
		//In order of highest->lowest priority
		if (type_combine & (uint8_t)type_data::tk_array)
			return kind_l == kind_r ? type_data::tk_array : type_data::tk_null;
		else if (type_combine & (uint8_t)type_data::tk_real)
			return type_data::tk_real;
		else if (type_combine & (uint8_t)type_data::tk_int)
			return type_data::tk_int;
		else if (type_combine & (uint8_t)type_data::tk_char)
			return type_data::tk_char;
		else if (type_combine & (uint8_t)type_data::tk_boolean)
			return type_data::tk_boolean;
		return type_data::tk_real;
	}
	bool BaseFunction::_type_assign_check(script_machine* machine, const value* v_src, const value* v_dst) {
		if (!v_dst->has_data()) return true;		//dest is null, assign ahead
		type_data* type_src = v_src->get_type();
		type_data* type_dst = v_dst->get_type();
		if (type_src != type_dst					//If the types are different
			&& !((type_src->get_kind() & type_dst->get_kind()) == type_data::tk_array	//unless they're both arrays
				&& (v_dst->length_as_array() == 0 || v_src->length_as_array() == 0))) {		//and either is empty
			std::string error = "Variable assignment cannot convert its type: ";
			error += type_data::string_representation(type_dst);
			error += " to ";
			error += type_data::string_representation(type_src);
			error += "\r\n";
			machine->raise_error(error);
			return false;
		}
		return true;
	}

	value BaseFunction::__script_perform_op_array(const value* v_left, const value* v_right, value(*func)(int, const value*)) {
		value result;
		std::vector<value> resArr;

		size_t ct_left = v_left->length_as_array();
		resArr.resize(ct_left);

		if (v_right->get_type()->get_kind() == type_data::tk_array) {
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

	inline double BaseFunction::_fmod2(double i, double j) {
		if (j < 0)
			return (i < 0) ? -fmod(-i, -j) : fmod(i, -j) + j;
		else
			return (i < 0) ? -fmod(-i, j) + j : fmod(i, j);
	}
	inline int64_t BaseFunction::_mod2(int64_t i, int64_t j) {
		if (j < 0)
			return (i < 0) ? -(-i % -j) : (i % -j) + j;
		else
			return (i < 0) ? -(-i % j) + j : (i % j);
	}

	value* BaseFunction::_value_cast(value* val, type_data::type_kind kind) {
		if (val == nullptr || val->get_type() == nullptr || val->get_type()->get_kind() == kind)
			return val;
		switch (kind) {
		case type_data::tk_int:
			val->set(script_type_manager::get_int_type(), val->as_int());
			break;
		case type_data::tk_real:
			val->set(script_type_manager::get_real_type(), val->as_real());
			break;
		case type_data::tk_char:
			val->set(script_type_manager::get_char_type(), val->as_char());
			break;
		case type_data::tk_boolean:
			val->set(script_type_manager::get_boolean_type(), val->as_boolean());
			break;
		}
		return val;
	}

	//-------------------------------------------------------------------------------------------

	value BaseFunction::_script_add(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_add);
		else {
			if (_type_check_two_any(argv[0].get_type(), argv[1].get_type(), type_data::tk_real))
				return value(script_type_manager::get_real_type(), argv[0].as_real() + argv[1].as_real());
			else
				return value(script_type_manager::get_int_type(), argv[0].as_int() + argv[1].as_int());
		}
	}
	SCRIPT_DECLARE_OP(add);

	value BaseFunction::_script_subtract(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_subtract);
		else {
			if (_type_check_two_any(argv[0].get_type(), argv[1].get_type(), type_data::tk_real))
				return value(script_type_manager::get_real_type(), argv[0].as_real() - argv[1].as_real());
			else
				return value(script_type_manager::get_int_type(), argv[0].as_int() - argv[1].as_int());
		}
	}
	SCRIPT_DECLARE_OP(subtract);

	value BaseFunction::_script_multiply(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_multiply);
		else {
			if (_type_check_two_any(argv[0].get_type(), argv[1].get_type(), type_data::tk_real))
				return value(script_type_manager::get_real_type(), argv[0].as_real() * argv[1].as_real());
			else
				return value(script_type_manager::get_int_type(), argv[0].as_int() * argv[1].as_int());
		}
	}
	SCRIPT_DECLARE_OP(multiply);

	value BaseFunction::_script_divide(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_divide);
		else {
			if (_type_check_two_any(argv[0].get_type(), argv[1].get_type(), type_data::tk_real))
				return value(script_type_manager::get_real_type(), argv[0].as_real() / argv[1].as_real());
			else
				return value(script_type_manager::get_int_type(), argv[0].as_int() / argv[1].as_int());
		}
	}
	SCRIPT_DECLARE_OP(divide);

	value BaseFunction::_script_remainder_(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_remainder_);
		else {
			if (_type_check_two_any(argv[0].get_type(), argv[1].get_type(), type_data::tk_real))
				return value(script_type_manager::get_real_type(), _fmod2(argv[0].as_real(), argv[1].as_real()));
			else
				return value(script_type_manager::get_int_type(), _mod2(argv[0].as_int(), argv[1].as_int()));
		}
	}
	SCRIPT_DECLARE_OP(remainder_);

	value BaseFunction::_script_negative(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::tk_array) {
			value result;
			std::vector<value> resArr;
			resArr.resize(argv->length_as_array());
			for (size_t i = 0; i < argv->length_as_array(); ++i) {
				resArr[i] = _script_negative(1, &argv->index_as_array(i));
			}
			result.set(argv->get_type(), resArr);
			return result;
		}
		else {
			if (argv->get_type()->get_kind() == type_data::tk_real)
				return value(script_type_manager::get_real_type(), -argv->as_real());
			else
				return value(script_type_manager::get_int_type(), -argv->as_int());
		}
	}
	SCRIPT_DECLARE_OP(negative);

	value BaseFunction::_script_power(int argc, const value* argv) {
		if (argv->get_type()->get_kind() == type_data::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_power);
		else {
			if (_type_check_two_any(argv[0].get_type(), argv[1].get_type(), type_data::tk_real))
				return value(script_type_manager::get_real_type(), std::pow(argv[0].as_real(), argv[1].as_real()));
			else {
				return value(script_type_manager::get_int_type(),
					(int64_t)(std::pow(argv[0].as_int(), argv[1].as_int()) + 0.01));
			}
		}
	}
	SCRIPT_DECLARE_OP(power);

	value BaseFunction::_script_compare(int argc, const value* argv) {
		type_data::type_kind type_check = _type_test_promotion(argv[0].get_type(), argv[1].get_type());
		if (type_check != type_data::tk_null) {
			int r = 0;
			switch (type_check) {
			case type_data::tk_int:
			{
				int64_t a = argv[0].as_int();
				int64_t b = argv[1].as_int();
				r = (a == b) ? 0 : (a < b) ? -1 : 1;
				break;
			}
			case type_data::tk_real:
			{
				double a = argv[0].as_real();
				double b = argv[1].as_real();
				r = (a == b) ? 0 : (a < b) ? -1 : 1;
				break;
			}
			case type_data::tk_char:
			{
				wchar_t a = argv[0].as_char();
				wchar_t b = argv[1].as_char();
				r = (a == b) ? 0 : (a < b) ? -1 : 1;
				break;
			}
			case type_data::tk_boolean:
			{
				bool a = argv[0].as_boolean();
				bool b = argv[1].as_boolean();
				r = (a == b) ? 0 : (a < b) ? -1 : 1;
				break;
			}
			case type_data::tk_array:
			{
				int64_t sl = argv[0].length_as_array();
				int64_t sr = argv[1].length_as_array();
				if (sl != sr) {
					r = sl < sr ? -1 : 1;
					break;
				}
				else {
					value v[2];
					for (size_t i = 0; i < sr; ++i) {
						v[0] = argv[0].index_as_array(i);
						v[1] = argv[1].index_as_array(i);
						r = _script_compare(2, v).as_real();
						if (r != 0)
							break;
					}
				}
				break;
			}
			}
			return value(script_type_manager::get_int_type(), (int64_t)r);
		}
		else {
			std::string error = "Unsupported compare operation: ";
			error += type_data::string_representation(argv[0].get_type());
			error += " and ";
			error += type_data::string_representation(argv[1].get_type());
			error += "\r\n";
			throw error;
		}
	}
	SCRIPT_DECLARE_OP(compare);

	value BaseFunction::_script_not_(int argc, const value* argv) {
		return value(script_type_manager::get_boolean_type(), !argv->as_boolean());
	}
	SCRIPT_DECLARE_OP(not_);

	value BaseFunction::modc(script_machine* machine, int argc, const value* argv) {
		if (_type_check_two_any(argv[0].get_type(), argv[1].get_type(), type_data::tk_real))
			return value(script_type_manager::get_real_type(), fmod(argv[0].as_real(), argv[1].as_real()));
		else
			return value(script_type_manager::get_int_type(), argv[0].as_int() % argv[1].as_int());
	}

	value BaseFunction::predecessor(script_machine* machine, int argc, const value* argv) {
		switch (argv->get_type()->get_kind()) {
		case type_data::tk_int:
			return value(argv->get_type(), argv->as_int() - 1i64);
		case type_data::tk_real:
			return value(argv->get_type(), argv->as_real() - 1);
		case type_data::tk_char:
			return value(argv->get_type(), (wchar_t)(argv->as_char() - 1));
		case type_data::tk_boolean:
			return value(argv->get_type(), false);
		case type_data::tk_array:
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
			_raise_error_unsupported(machine, argv->get_type(), "predecessor");
			return value();
		}
		}
	}
	value BaseFunction::successor(script_machine* machine, int argc, const value* argv) {
		switch (argv->get_type()->get_kind()) {
		case type_data::tk_int:
			return value(argv->get_type(), argv->as_int() + 1i64);
		case type_data::tk_real:
			return value(argv->get_type(), argv->as_real() + 1);
		case type_data::tk_char:
			return value(argv->get_type(), (wchar_t)(argv->as_char() + 1));
		case type_data::tk_boolean:
			return value(argv->get_type(), true);
		case type_data::tk_array:
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
			_raise_error_unsupported(machine, argv->get_type(), "successor");
			return value();
		}
		}
	}

	value BaseFunction::length(script_machine* machine, int argc, const value* argv) {
		assert(argc == 1);
		return value(script_type_manager::get_int_type(), (int64_t)argv->length_as_array());
	}

	bool BaseFunction::_index_check(script_machine* machine, type_data* arg0_type, size_t arg0_size, int index) {
		if (arg0_type->get_kind() != type_data::tk_array) {
			_raise_error_unsupported(machine, arg0_type, "array index");
			return false;
		}
		if (index < 0 || index >= arg0_size) {
			std::string error = StringUtility::Format("Array index out of bounds. (indexing=%d, size=%u)\r\n",
				index, arg0_size);
			machine->raise_error(error);
			return false;
		}
		return true;
	}
	const value& BaseFunction::index(script_machine* machine, int argc, const value* argv) {
		assert(argc == 2);

		int index = argv[1].as_int();
		if (!_index_check(machine, argv->get_type(), argv->length_as_array(), index))
			return value::val_empty;

		return argv->index_as_array(index);
	}

	value BaseFunction::slice(script_machine* machine, int argc, const value* argv) {
		assert(argc == 3);

		if (argv->get_type()->get_kind() != type_data::tk_array) {
			_raise_error_unsupported(machine, argv->get_type(), "array slice");
			return value();
		}

		size_t index_1 = argv[1].as_real();
		size_t index_2 = argv[2].as_real();

		if ((index_2 > index_1 && (index_1 < 0 || index_2 > argv->length_as_array()))
			|| (index_2 < index_1 && (index_2 < 0 || index_1 > argv->length_as_array()))) 
		{
			std::string error = StringUtility::Format("Array index out of bounds. (slicing=(%d, %d), size=%u)\r\n",
				index_1, index_2, argv->length_as_array());
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

		if (argv->get_type()->get_kind() != type_data::tk_array) {
			_raise_error_unsupported(machine, argv->get_type(), "array erase");
			return value();
		}

		int index_1 = argv[1].as_int();
		size_t length = argv->length_as_array();

		if (index_1 < 0 || index_1 >= length) {
			std::string error = StringUtility::Format("Array index out of bounds. (erasing=%d, size=%u)\r\n",
				index_1, length);
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
		if (arg0_type->get_kind() != type_data::tk_array) {
			_raise_error_unsupported(machine, arg0_type, "array append");
			return false;
		}
		if (arg0_size > 0U && arg0_type->get_element() != arg1_type) {
			std::string error = StringUtility::Format("Value type does not match. (%s ~ [%s])\r\n",
				type_data::string_representation(arg0_type).c_str(),
				type_data::string_representation(arg1_type).c_str());
			machine->raise_error(error);
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

		if (!_type_check_two_any(argv[0].get_type(), argv[1].get_type(), type_data::tk_array)) {
			_raise_error_unsupported(machine, argv->get_type(), "array concatenate");
			return value();
		}

		if (argv[0].length_as_array() > 0 && argv[1].length_as_array() > 0 && argv[0].get_type() != argv[1].get_type()) {
			std::string error = StringUtility::Format("Value type does not match. (%s ~ %s)\r\n",
				type_data::string_representation(argv[0].get_type()).c_str(),
				type_data::string_representation(argv[1].get_type()).c_str());
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

#define BITWISE_RET if (_is_force_convert_real(argv[0].get_type()) || _is_force_convert_real(argv[1].get_type())) \
						return value(script_type_manager::get_real_type(), (double)res); \
					else \
						return value(argv->get_type(), res);
	value BaseFunction::bitwiseNot(script_machine* machine, int argc, const value* argv) {
		int64_t val = argv[0].as_int();
		int64_t res = ~val;
		BITWISE_RET;
	}
	value BaseFunction::bitwiseAnd(script_machine* machine, int argc, const value* argv) {
		int64_t val1 = argv[0].as_int();
		int64_t val2 = argv[1].as_int();
		int64_t res = val1 & val2;
		BITWISE_RET;
	}
	value BaseFunction::bitwiseOr(script_machine* machine, int argc, const value* argv) {
		int64_t val1 = argv[0].as_int();
		int64_t val2 = argv[1].as_int();
		int64_t res = val1 | val2;
		BITWISE_RET;
	}
	value BaseFunction::bitwiseXor(script_machine* machine, int argc, const value* argv) {
		int64_t val1 = argv[0].as_int();
		int64_t val2 = argv[1].as_int();
		int64_t res = val1 ^ val2;
		BITWISE_RET;
	}
	value BaseFunction::bitwiseLeft(script_machine* machine, int argc, const value* argv) {
		int64_t val1 = argv[0].as_int();
		int64_t val2 = argv[1].as_int();
		int64_t res = val1 << val2;
		BITWISE_RET;
	}
	value BaseFunction::bitwiseRight(script_machine* machine, int argc, const value* argv) {
		int64_t val1 = argv[0].as_int();
		int64_t val2 = argv[1].as_int();
		int64_t res = val1 >> val2;
		BITWISE_RET;
	}
#undef BITWISE_RET

	value BaseFunction::script_debugBreak(script_machine* machine, int argc, const value* argv) {
#ifdef _DEBUG
		//Prevents crashes if called without a debugger attached, not to prevent external debugging
		if (IsDebuggerPresent())
			DebugBreak();
#endif
		return value();
	}
}
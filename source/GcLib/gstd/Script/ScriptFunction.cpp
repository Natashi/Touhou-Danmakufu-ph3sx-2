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
	constant::constant(const char* name_, int64_t d_int) : name(name_), type(type_data::tk_int) {
		memcpy(&data, &d_int, sizeof(int64_t));
	}
	constant::constant(const char* name_, double d_real) : name(name_), type(type_data::tk_real) {
		memcpy(&data, &d_real, sizeof(double));
	}
	constant::constant(const char* name_, wchar_t d_char) : name(name_), type(type_data::tk_char) {
		memcpy(&data, &d_char, sizeof(wchar_t));
	}
	constant::constant(const char* name_, bool d_bool) : name(name_), type(type_data::tk_boolean) {
		memcpy(&data, &d_bool, sizeof(bool));
	}

	//-------------------------------------------------------------------------------------------

	static inline const bool _is_force_convert_real(type_data* type) {
		return type->get_kind() & (type_data::tk_real | type_data::tk_array);
	}

	//-------------------------------------------------------------------------------------------

	const void BaseFunction::_raise_error_unsupported(script_machine* machine, type_data* type, const std::string& op_name) {
		std::string error = StringUtility::Format("This value type does not support the %s operation: %s\r\n",
			op_name.c_str(), type_data::string_representation(type).c_str());
		if (machine) machine->raise_error(error);
		else throw error;
	}

	
	type_data::type_kind BaseFunction::_type_test_promotion(type_data* type_l, type_data* type_r) {
		uint8_t kind_l = type_l->get_kind();
		uint8_t kind_r = type_r->get_kind();
		if (kind_l == kind_r) return (type_data::type_kind)kind_l;

		//In order of highest->lowest priority
		uint8_t type_combine = kind_l | kind_r;
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

	bool BaseFunction::__type_assign_check(type_data* type_src, type_data* type_dst) {
		if (type_dst == nullptr) return true;	//dest is null, assign ahead
		if (type_src) {
			if (type_src == type_dst)
				return true;		//Same type
			if (!_type_check_two_any(type_dst, type_src, type_data::tk_array))
				return true;		//Different type, but neither is an array
			if (type_src->get_kind() == type_dst->get_kind()) {
				type_data* elem_src = type_src->get_element();
				type_data* elem_dst = type_dst->get_element();

				if (elem_src == nullptr || elem_dst == nullptr)
					return true;	//One is a null array (empty array)

				if ((elem_src->get_kind() | elem_dst->get_kind()) == (type_data::tk_int | type_data::tk_real))
					return true;	//Allow implicit (int[]) <-> (real[]) conversion

				if (_type_check_two_all(elem_src, elem_dst, type_data::tk_array))
					return __type_assign_check(elem_src, elem_dst);
			}
		}
		return false;
	}
	bool BaseFunction::__type_assign_check_no_convert(type_data* type_src, type_data* type_dst) {
		if (type_dst == nullptr) return true;	//dest is null, assign ahead
		if (type_src) {
			if (type_src == type_dst)
				return true;		//Same type
			else if (_type_check_two_all(type_src, type_dst, type_data::tk_array)
				&& (type_src->get_element() == nullptr || type_dst->get_element() == nullptr))
				return true;		//Different type, but both are arrays, and one is empty
		}
		return false;
	}

	bool BaseFunction::_type_assign_check(script_machine* machine, const value* v_src, const value* v_dst) {
		type_data* type_src = v_src->get_type();
		type_data* type_dst = v_dst->get_type();
		return _type_assign_check(machine, type_src, type_dst);
	}
	bool BaseFunction::_type_assign_check(script_machine* machine, type_data* type_src, type_data* type_dst) {
		bool res = __type_assign_check(type_src, type_dst);
		if (!res) {
			std::string error = StringUtility::Format(
				"Cannot implicitly convert from \"%s\" to \"%s\".\r\n",
				type_data::string_representation(type_src).c_str(),
				type_data::string_representation(type_dst).c_str());
			if (machine) machine->raise_error(error);
			else throw error;
		}
		return res;
	}
	bool BaseFunction::_type_assign_check_no_convert(script_machine* machine, const value* v_src, const value* v_dst) {
		bool res = __type_assign_check_no_convert(v_src->get_type(), v_dst->get_type());
		if (!res) {
			std::string error = StringUtility::Format(
				"Cannot assign type \"%s\" to type \"%s\".\r\n",
				type_data::string_representation(v_dst->get_type()).c_str(),
				type_data::string_representation(v_src->get_type()).c_str());
			if (machine) machine->raise_error(error);
			else throw error;
		}
		return res;
	}

	bool BaseFunction::_type_check_two_any(type_data* type_l, type_data* type_r, uint8_t type) {
		if (type_l != nullptr && type_r != nullptr) {
			uint8_t type_combine = (uint8_t)type_l->get_kind() | (uint8_t)type_r->get_kind();
			return (type_combine & type) != 0;
		}
		return false;
	}
	bool BaseFunction::_type_check_two_all(type_data* type_l, type_data* type_r, uint8_t type) {
		if (type_l != nullptr && type_r != nullptr) {
			uint8_t type_combine = (uint8_t)type_l->get_kind() & (uint8_t)type_r->get_kind();
			return (type_combine & type) != 0;
		}
		return false;
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

		result.reset(v_left->get_type(), resArr);
		//_value_cast(&result, result.get_type());

		return result;
	}

	inline double BaseFunction::_fmod2(double i, double j) {
		if (j < 0)
			return (i < 0) ? -fmod(-i, -j) : fmod(fmod(i, -j) + j, j);
		else
			return (i < 0) ? fmod(j - fmod(-i, j), j) : fmod(i, j);
	}
	inline int64_t BaseFunction::_mod2(int64_t i, int64_t j) {
		if (j < 0)
			return (i < 0) ? -((-i) % (-j)) : (((i % (-j)) + j) % j);
		else
			return (i < 0) ? ((j - ((-i) % j)) % j) : (i % j);
	}

	bool BaseFunction::_is_empty_type(value* val) {
		if (val == nullptr)
			return true;
		return _is_empty_type(val->get_type());
	}
	bool BaseFunction::_is_empty_type(type_data* type) {
		if (type == nullptr)
			return true;
		return _rtypeof(type) == type_data::tk_null;
	}
	value* BaseFunction::_value_cast(value* val, type_data* cast) {
		if (val == nullptr)
			return val;
		if (val->get_type() == nullptr) {
			val->set(cast);
			return val;
		}
		if (_rtypeof(cast) == type_data::tk_null)
			return val;

		switch (cast->get_kind()) {
		case type_data::tk_int:
			return val->reset(cast, val->as_int());
		case type_data::tk_real:
			return val->reset(cast, val->as_real());
		case type_data::tk_char:
			return val->reset(cast, val->as_char());
		case type_data::tk_boolean:
			return val->reset(cast, val->as_boolean());
		case type_data::tk_array:
			if (type_data* castElem = cast->get_element()) {
				if (val->length_as_array() > 0) {
					std::vector<value> arrVal = *(val->as_array_ptr());
					for (value& iVal : arrVal)
						_value_cast(&iVal, castElem);
					return val->reset(cast, arrVal);
				}
			}
		}
		val->set(cast);
		return val;
	}

	bool BaseFunction::_index_check(script_machine* machine, type_data* arg0_type, size_t arg0_size, int index) {
		if (arg0_type == nullptr || arg0_type->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, arg0_type, "array index");
			return false;
		}
		if (index < 0 || index >= arg0_size) {
			std::string error = StringUtility::Format("Array index out of bounds. (indexing=%d, size=%u)\r\n",
				index, arg0_size);
			if (machine) machine->raise_error(error);
			else throw error;
			return false;
		}
		return true;
	}

	bool BaseFunction::_append_check(script_machine* machine, type_data* arg0_type, type_data* arg1_type) {
		if (arg0_type == nullptr || arg0_type->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, arg0_type, "array append");
			return false;
		}
		type_data* elem = arg0_type->get_element();
		if (elem != nullptr && !__type_assign_check(elem, arg1_type)) {
			std::string error = StringUtility::Format(
				"Array append cannot implicitly convert from \"%s\" to \"%s\".\r\n",
				type_data::string_representation(elem).c_str(),
				type_data::string_representation(arg1_type).c_str());
			if (machine) machine->raise_error(error);
			else throw error;
			return false;
		}
		return true;
	}
	bool BaseFunction::_append_check_no_convert(script_machine* machine, type_data* arg0_type, type_data* arg1_type) {
		if (arg0_type == nullptr || arg0_type->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, arg0_type, "array append");
			return false;
		}
		type_data* elem = arg0_type->get_element();
		if (elem != nullptr && elem != arg1_type) {
			std::string error = StringUtility::Format("Invalid value type for append: %s ~ [%s]\r\n",
				type_data::string_representation(arg0_type).c_str(),
				type_data::string_representation(arg1_type).c_str());
			if (machine) machine->raise_error(error);
			else throw error;
			return false;
		}
		return true;
	}
	bool BaseFunction::_null_check(script_machine* machine, const value* argv, size_t count) {
		for (size_t i = 0; i < count; ++i) {
			const value* val = &argv[i];
			if (val == nullptr || !val->has_data()) {
				std::string error = StringUtility::Format("Unsupported operation: value is null\r\n");
				if (machine) machine->raise_error(error);
				return false;
			}
		}
		return true;
	}

	value BaseFunction::_create_empty(type_data* type) {
		value res;
		if (type) {
			switch (type->get_kind()) {
			case type_data::tk_int:
				res.reset(type, 0i64); break;
			case type_data::tk_real:
				res.reset(type, 0.0); break;
			case type_data::tk_char:
				res.reset(type, L'\0'); break;
			case type_data::tk_boolean:
				res.reset(type, false); break;
			case type_data::tk_array:
			{
				std::vector<value> vec;
				res.reset(type, vec); break;
			}
			}
		}
		return res;
	}

	type_data::type_kind BaseFunction::_typeof(type_data* type) {
		if (type == nullptr)
			return type_data::tk_null;
		if (type->get_kind() == type_data::tk_array) {
			type_data* elem = type->get_element();
			if (elem && elem->get_kind() == type_data::tk_char) {	//String
				return type_data::tk_string;
			}
		}
		return type->get_kind();
	}
	type_data::type_kind BaseFunction::_rtypeof(type_data* type) {
		if (type == nullptr)
			return type_data::tk_null;
		type_data* typec = type;
		while (typec->get_kind() == type_data::tk_array) {
			typec = typec->get_element();
			if (typec == nullptr)
				return type_data::tk_null;
		}
		return typec->get_kind();
	}
	size_t _array_dimension(type_data* type) {
		size_t dim = 0;
		for (; type != nullptr; type = type->get_element()) {
			++dim;
		}
		return dim;
	}

	//-------------------------------------------------------------------------------------------

	type_data* __cast_array(script_machine* machine, type_data* rootType, value* val, type_data* setType) {
		std::vector<value> arrVal(val->length_as_array());

		script_type_manager* typeManager = script_type_manager::get_instance();
		type_data* elemType = nullptr;

		for (size_t i = 0; i < arrVal.size(); ++i) {
			value nv = val->index_as_array(i);
			if (nv.get_type()->get_kind() == type_data::tk_array) {
				type_data* nextElemType = __cast_array(machine, rootType, &nv, setType);
				if (elemType == nullptr)
					elemType = nextElemType;
			}
			else
				BaseFunction::_value_cast(&nv, rootType);
			arrVal[i] = nv;
		}

		if (elemType == nullptr) elemType = setType;
		type_data* arrayType = typeManager->get_array_type(elemType);

		val->set(arrayType, arrVal);
		return arrayType;
	}
	value BaseFunction::_cast_array(script_machine* machine, const value* argv, type_data::type_kind target) {
		type_data* arg0_type = argv->get_type();
		if (arg0_type == nullptr || arg0_type->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, arg0_type, "array cast");
			return value();
		}
		else {
			script_type_manager* typeManager = script_type_manager::get_instance();
			type_data* rootType = typeManager->get_type(target);

			value res = *argv;
			res.make_unique();

			__cast_array(machine, rootType, &res, rootType);

			return res;
		}
	}
	value BaseFunction::cast_int_array(script_machine* machine, int argc, const value* argv) {
		return _cast_array(machine, argv, type_data::tk_int);
	}
	value BaseFunction::cast_real_array(script_machine* machine, int argc, const value* argv) {
		return _cast_array(machine, argv, type_data::tk_real);
	}
	value BaseFunction::cast_bool_array(script_machine* machine, int argc, const value* argv) {
		return _cast_array(machine, argv, type_data::tk_boolean);
	}
	value BaseFunction::cast_char_array(script_machine* machine, int argc, const value* argv) {
		return _cast_array(machine, argv, type_data::tk_char);
		//return value(script_type_manager::get_string_type(), argv->as_string());
	}
	value BaseFunction::cast_x_array(script_machine* machine, int argc, const value* argv) {
		_null_check(nullptr, argv, argc);

		type_data::type_kind targetKind = (type_data::type_kind)argv[1].as_int();
		switch (targetKind) {
		case type_data::tk_int:
		case type_data::tk_real:
		case type_data::tk_char:
		case type_data::tk_boolean:
			break;
		default:
		{
			type_data tmp(targetKind);
			std::string error = StringUtility::Format("Invalid target type for array cast: %s\r\n",
				type_data::string_representation(&tmp).c_str());
			machine->raise_error(error);
			return value();
		}
		}
		return _cast_array(machine, argv, targetKind);
	}

	value BaseFunction::_script_add(int argc, const value* argv) {
		_null_check(nullptr, argv, argc);

		if (argv[0].get_type()->get_kind() == type_data::tk_array)
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
		_null_check(nullptr, argv, argc);

		if (argv[0].get_type()->get_kind() == type_data::tk_array)
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
		_null_check(nullptr, argv, argc);

		if (argv[0].get_type()->get_kind() == type_data::tk_array)
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
		_null_check(nullptr, argv, argc);

		if (argv[0].get_type()->get_kind() == type_data::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_divide);
		else {
			if (_type_check_two_any(argv[0].get_type(), argv[1].get_type(), type_data::tk_real))
				return value(script_type_manager::get_real_type(), argv[0].as_real() / argv[1].as_real());
			else {
				int64_t deno = argv[1].as_int();
				if (deno == 0)
					throw std::string("Invalid operation: integer division by zero.\r\n");
				return value(script_type_manager::get_int_type(), (int64_t)(argv[0].as_int() / deno));
			}
		}
	}
	SCRIPT_DECLARE_OP(divide);

	value BaseFunction::_script_fdivide(int argc, const value* argv) {
		_null_check(nullptr, argv, argc);

		if (argv[0].get_type()->get_kind() == type_data::tk_array)
			return __script_perform_op_array(&argv[0], &argv[1], _script_fdivide);
		else {
			int64_t res;
			if (_type_check_two_any(argv[0].get_type(), argv[1].get_type(), type_data::tk_real))
				res = argv[0].as_real() / argv[1].as_real();
			else {
				int64_t deno = argv[1].as_int();
				if (deno == 0)
					throw std::string("Invalid operation: integer division by zero.\r\n");
				res = argv[0].as_int() / deno;
			}
			return value(script_type_manager::get_int_type(), res);
		}
	}
	SCRIPT_DECLARE_OP(fdivide);

	value BaseFunction::_script_remainder_(int argc, const value* argv) {
		_null_check(nullptr, argv, argc);

		if (argv[0].get_type()->get_kind() == type_data::tk_array)
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
		_null_check(nullptr, argv, argc);

		if (argv->get_type()->get_kind() == type_data::tk_array) {
			value result;
			std::vector<value> resArr;
			resArr.resize(argv->length_as_array());
			for (size_t i = 0; i < argv->length_as_array(); ++i) {
				resArr[i] = _script_negative(1, &argv->index_as_array(i));
			}
			result.reset(argv->get_type(), resArr);
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
		_null_check(nullptr, argv, argc);

		if (argv[0].get_type()->get_kind() == type_data::tk_array)
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
		//_null_check(nullptr, argv, argc);

		if (argv[0].get_type() != nullptr && argv[1].get_type() != nullptr) {
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
		}

		{
			std::string error = "Cannot directly compare between ";
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
		_null_check(machine, argv, argc);

		if (_type_check_two_any(argv[0].get_type(), argv[1].get_type(), type_data::tk_real))
			return value(script_type_manager::get_real_type(), fmod(argv[0].as_real(), argv[1].as_real()));
		else
			return value(script_type_manager::get_int_type(), argv[0].as_int() % argv[1].as_int());
	}

	//Allows for non-integer side counts because A. no division by 0 errors and B. why not 
	DNH_FUNCAPI_DEF_(BaseFunction::apo) {
		double n = argv->as_real();
		double r = n > 2 ? cos(GM_PI / n) : 0;
		return value(script_type_manager::get_real_type(), r);
	}

	value BaseFunction::predecessor(script_machine* machine, int argc, const value* argv) {
		_null_check(machine, argv, argc);

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
			result.reset(argv->get_type(), resArr);
			return result;
		}
		default:
		{
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "predecessor");
			return value();
		}
		}
	}
	value BaseFunction::successor(script_machine* machine, int argc, const value* argv) {
		_null_check(machine, argv, argc);

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
			result.reset(argv->get_type(), resArr);
			return result;
		}
		default:
		{
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "successor");
			return value();
		}
		}
	}

	value BaseFunction::length(script_machine* machine, int argc, const value* argv) {
		return value(script_type_manager::get_int_type(), (int64_t)argv->length_as_array());
	}
	value BaseFunction::resize(script_machine* machine, int argc, const value* argv) {
		_null_check(machine, &argv[0], 1);
		if (argc == 3)
			_null_check(machine, &argv[2], 1);

		const value* val = &argv[0];
		type_data* valType = val->get_type();

		if (valType->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "resize");
			return value();
		}

		size_t oldSize = val->length_as_array();
		size_t newSize = argv[1].as_int();
		type_data* newType = val->get_type();

		value res = *val;
		res.make_unique();
		if (newSize != oldSize) {
			std::vector<value> arrVal(newSize);

			for (size_t i = 0; i < oldSize && i < newSize; ++i)
				arrVal[i] = val->index_as_array(i);
			if (newSize > oldSize) {
				value fill;
				if (argc == 3) {	//Has fill value
					fill = argv[2];
					if (oldSize > 0) {
						if (!BaseFunction::_append_check(machine, valType, fill.get_type())) return value();
						BaseFunction::_value_cast(&fill, valType->get_element());
					}
					else newType = script_type_manager::get_instance()->get_array_type(fill.get_type());
				}
				else {
					if (valType->get_element() == nullptr) {
						machine->raise_error("Resizing from a null array requires a fill value.\r\n");
						return value();
					}
					fill = BaseFunction::_create_empty(valType->get_element());
				}

				for (size_t i = oldSize; i < newSize; ++i)
					arrVal[i] = fill;
			}

			res.reset(newType, arrVal);
		}
		return res;
	}

	DNH_FUNCAPI_DEF_(BaseFunction::reverse) {
		_null_check(machine, argv, argc);

		const value* val = &argv[0];
		type_data* valType = val->get_type();

		if (valType->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "reverse");
			return value();
		}

		size_t size = val->length_as_array();

		value res = *val;
		res.make_unique();

		std::vector<value> arrVal(size);

		for (size_t i = 0; i < size; ++i) arrVal[i] = val->index_as_array(size - i - 1);

		res.reset(valType, arrVal);
		return res;
	}

	DNH_FUNCAPI_DEF_(BaseFunction::sort) {
		_null_check(machine, argv, argc);

		const value* val = &argv[0];
		type_data* valType = val->get_type();

		bool bRev = false;
		if (argc == 2) bRev = argv[1].as_boolean();

		if (valType->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "sort");
			return value();
		}
	

		size_t size = val->length_as_array();

		value res = *val;
		res.make_unique();

		std::vector<value> arrVal(size);

		// Populate source array
		for (size_t i = 0; i < size; ++i)
			arrVal[i] = val->index_as_array(i);

		for (size_t i = 0; i < size - 1; ++i) {
			size_t idx = i;
			for (size_t j = i + 1; j < size; ++j) {
				value args[2] = { arrVal[idx], arrVal[j] };
				if (compare(machine, 2, args).as_int() == (bRev ? -1 : 1)) {
					idx = j;
				}
			}
			std::swap(arrVal[i], arrVal[idx]);
		}

		res.reset(valType, arrVal);
		return res;
	}

	DNH_FUNCAPI_DEF_(BaseFunction::range) {
		int64_t start = 0;
		int64_t stop = 0;
		int64_t step = 1;

		if (argc == 1) stop = argv[0].as_int();
		else {
			start = argv[0].as_int();
			stop = argv[1].as_int();
			if (argc == 3) step = argv[2].as_int();
		}

		if (argc < 3 && stop < start) step = -1;
		else if (argc == 3 && (step == 0
			|| (start > stop && step > 0)
			|| (start < stop && step < 0)
			)) {
			std::string error = "Invalid range.";
			machine->raise_error(error);
			return value();
		}

		value result;
		std::vector<value> resArr;

		for (int64_t i = start; (step > 0 && i < stop) || (step < 0 && i > stop); i += step) {
			resArr.push_back(value(script_type_manager::get_int_type(), i));
		}

		result.reset(script_type_manager::get_int_array_type(), resArr);
		return result;
	}

	DNH_FUNCAPI_DEF_(BaseFunction::contains) {
		_null_check(machine, argv, argc);

		const value* arr = &argv[0];
		type_data* arrType = arr->get_type();

		if (arrType->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "contains");
			return value();
		}

		const value& val = argv[1];
		size_t length = arr->length_as_array();

		bool res = false;
		for (size_t i = 0; i < length; ++i) {
			value args[2] = { arr->index_as_array(i), val };
			if (compare(machine, 2, args).as_int() == 0) {
				res = true;
				break;
			}
		}
		return value(script_type_manager::get_boolean_type(), res);
	}

	DNH_FUNCAPI_DEF_(BaseFunction::indexof) {
		_null_check(machine, argv, argc);

		const value* arr = &argv[0];
		type_data* arrType = arr->get_type();

		if (arrType->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "indexof");
			return value();
		}

		const value& val = argv[1];
		size_t length = arr->length_as_array();

		int64_t res = -1;
		for (size_t i = 0; i < length; ++i) {
			value args[2] = { arr->index_as_array(i), val };
			if (compare(machine, 2, args).as_int() == 0) {
				res = i;
				break;
			}
		}
		return value(script_type_manager::get_int_type(), res);
	}

	DNH_FUNCAPI_DEF_(BaseFunction::matches) {
		_null_check(machine, argv, argc);

		const value* arr = &argv[0];
		type_data* arrType = arr->get_type();

		if (arrType->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "matches");
			return value();
		}

		const value& val = argv[1];
		size_t length = arr->length_as_array();

		int64_t res = 0;
		for (size_t i = 0; i < length; ++i) {
			value args[2] = { arr->index_as_array(i), val };
			if (compare(machine, 2, args).as_int() == 0) {
				++res;
			}
		}
		return value(script_type_manager::get_int_type(), res);
	}

	DNH_FUNCAPI_DEF_(BaseFunction::all) {
		_null_check(machine, argv, argc);

		const value* arr = &argv[0];
		type_data* arrType = arr->get_type();

		if (arrType->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "all");
			return value();
		}

		const value& val = argv[1];
		size_t length = arr->length_as_array();

		bool res = true;
		for (size_t i = 0; i < length && res; ++i)
			res = arr->index_as_array(i).as_boolean();

		return value(script_type_manager::get_boolean_type(), res);
	}

	DNH_FUNCAPI_DEF_(BaseFunction::any) {
		_null_check(machine, argv, argc);

		const value* arr = &argv[0];
		type_data* arrType = arr->get_type();

		if (arrType->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "any");
			return value();
		}

		const value& val = argv[1];
		size_t length = arr->length_as_array();

		bool res = false;
		for (size_t i = 0; i < length && !res; ++i)
			res = arr->index_as_array(i).as_boolean();

		return value(script_type_manager::get_boolean_type(), res);
	}

	DNH_FUNCAPI_DEF_(BaseFunction::replace) {
		_null_check(machine, argv, argc);

		const value* val = &argv[0];
		type_data* valType = val->get_type();

		if (valType->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "replace");
			return value();
		}


		size_t size = val->length_as_array();

		value from = argv[1];
		value to = argv[2];

		type_data* addType = to.get_type();
		type_data* elemType = valType->get_element();

		_append_check(machine, valType, addType);

		if (addType != elemType)
			BaseFunction::_value_cast(&to, elemType);

		value res = *val;
		res.make_unique();

		std::vector<value> arrVal(size);

		// Populate source array
		for (size_t i = 0; i < size; ++i)
			arrVal[i] = val->index_as_array(i);

		for (size_t i = 0; i < size; ++i) {
			value args[2] = { arrVal[i], from };
			if (compare(machine, 2, args).as_int() == 0) {
				arrVal[i] = to;
			}
		}

		res.reset(valType, arrVal);
		return res;
	}

	DNH_FUNCAPI_DEF_(BaseFunction::remove) {
		_null_check(machine, argv, argc);

		const value* val = &argv[0];
		type_data* valType = val->get_type();

		if (valType->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv->get_type(), "remove");
			return value();
		}

		size_t size = val->length_as_array();

		size_t count = argc - 1;

		value res = *val;
		res.make_unique();

		std::vector<value> arrVal(size);

		// Populate source array
		for (size_t i = 0; i < size; ++i)
			arrVal[i] = val->index_as_array(i);

		for (size_t i = 0; i < size; ++i) {
			for (size_t j = 1; j <= count; ++j) {
				value args[2] = { arrVal[i], argv[j] };
				if (compare(machine, 2, args).as_int() == 0) {
					arrVal.erase(arrVal.begin() + i);
					size--;
					i--;
					break;
				}
			}
		}

		res.reset(valType, arrVal);
		return res;
	}

	const value* BaseFunction::index(script_machine* machine, int argc, value* arr, value* indexer) {
		_null_check(machine, arr, 1);
		int index = indexer->as_int();
		size_t length = arr->length_as_array();
		if (index < 0) index += length;
		if (!_index_check(machine, arr->get_type(), length, index))
			return nullptr;
		return &arr->index_as_array(index);
	}

	value BaseFunction::slice(script_machine* machine, int argc, const value* argv) {
		_null_check(machine, &argv[0], 1);

		if (argv[0].get_type()->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv[0].get_type(), "array slice");
			return value();
		}

		size_t length = argv[0].length_as_array();
		int index_1 = argv[1].as_int();
		int index_2 = argv[2].as_int();

		if ((index_2 > index_1 && (index_1 < 0))
			|| (index_2 < index_1 && (index_2 < 0)))
		{
			std::string error = StringUtility::Format("Array index out of bounds. (slicing=(%d, %d), size=%u)\r\n",
				index_1, index_2, length);
			machine->raise_error(error);
			return value();
		}

		value result;
		std::vector<value> resArr;

		if (length > 0) {
			if (index_2 > index_1) {
				index_1 = std::max<int>(index_1, 0);
				index_2 = std::min<int>(index_2, length);

				resArr.resize(index_2 - index_1);
				for (size_t i = 0, j = index_1; i < resArr.size(); ++i, ++j) {
					const value& v = argv[0].index_as_array(j);
					resArr[i] = v;
				}
			}
			else if (index_1 > index_2) {		//Reverse
				index_1 = std::min<int>(index_1, length);
				index_2 = std::max<int>(index_2, 0);

				resArr.resize(index_1 - index_2);
				for (size_t i = 0, j = index_1 - 1; i < resArr.size(); ++i, --j) {
					const value& v = argv[0].index_as_array(j);
					resArr[i] = v;
				}
			}
		}

		result.reset(argv[0].get_type(), resArr);
		result.make_unique();
		return result;
	}
	value BaseFunction::insert(script_machine* machine, int argc, const value* argv) {
		_null_check(machine, &argv[0], 1);
		_null_check(machine, &argv[2], 1);

		type_data* arrType = argv[0].get_type();
		if (arrType->get_kind() != type_data::tk_array) {
			_raise_error_unsupported(machine, argv[0].get_type(), "array insert");
			return value();
		}

		size_t length = argv[0].length_as_array();
		int insertPos = argv[1].as_int();

		if (insertPos < 0 || (length > 0 && insertPos > length) || (length == 0 && insertPos > 0)) {
			std::string error = StringUtility::Format("Array index out of bounds. (inserting=%d, size=%u)\r\n",
				insertPos, length);
			machine->raise_error(error);
			return value();
		}
		if (length == 0) {
			arrType = script_type_manager::get_instance()->get_array_type(arrType);
		}

		value insertVal = argv[2];
		type_data* insertType = insertVal.get_type();
		insertVal.make_unique();

		if (_is_empty_type(arrType)) {
			arrType = script_type_manager::get_instance()->get_array_type(arrType);
		}
		else {
			type_data* elem = arrType->get_element();
			if (elem != nullptr && !__type_assign_check(elem, insertType)) {
				std::string error = StringUtility::Format(
					"Array insert cannot implicitly convert from \"%s\" to \"%s\".\r\n",
					type_data::string_representation(elem).c_str(),
					type_data::string_representation(insertType).c_str());
				machine->raise_error(error);
				return value();
			}
		}

		value result;
		std::vector<value> resArr;
		resArr.resize(length + 1U);
		{
			size_t iArr = 0;
			for (size_t i = 0; i < insertPos; ++i) {
				resArr[iArr++] = argv[0].index_as_array(i);
			}
			resArr[iArr++] = insertVal;
			for (size_t i = insertPos; i < length; ++i) {
				resArr[iArr++] = argv[0].index_as_array(i);
			}
		}
		result.reset(arrType, resArr);
		result.make_unique();
		_value_cast(&result, arrType);
		return result;
	}
	value BaseFunction::erase(script_machine* machine, int argc, const value* argv) {
		_null_check(machine, &argv[0], 1);

		if (argv[0].get_type()->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, argv[0].get_type(), "array erase");
			return value();
		}

		size_t length = argv[0].length_as_array();
		int index_1 = argv[1].as_int();

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
				resArr[iArr++] = argv[0].index_as_array(i);
			}
			for (size_t i = index_1 + 1; i < length; ++i) {
				resArr[iArr++] = argv[0].index_as_array(i);
			}
		}
		result.reset(argv[0].get_type(), resArr);
		result.make_unique();
		return result;
	}

	value BaseFunction::append(script_machine* machine, int argc, const value* argv) {
		_null_check(machine, argv, argc);

		type_data* type_array = argv[0].get_type();
		type_data* type_appending = argv[1].get_type();

		value result = argv[0];
		result.make_unique();

		_append_check(machine, type_array, type_appending);
		type_data* type_target = type_array->get_element() == nullptr ?
			script_type_manager::get_instance()->get_array_type(type_appending) : type_array;
		{
			value appending = argv[1];
			if (appending.get_type() != type_target->get_element()) {
				BaseFunction::_value_cast(&appending, type_target->get_element());
			}
			result.append(type_target, appending);
		}
		return result;
	}

	bool __chk_concat(script_machine* machine, type_data* type_l, type_data* type_r) {
		if (type_l->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, type_l, "array concatenate");
			return false;
		}
		if (type_r->get_kind() != type_data::tk_array) {
			BaseFunction::_raise_error_unsupported(machine, type_r, "array concatenate");
			return false;
		}
		//if (type_l != type_r && !(type_l->get_element() == nullptr || type_r->get_element() == nullptr)) {
		if (!BaseFunction::__type_assign_check(type_l, type_r)) {
			std::string error = StringUtility::Format("Invalid value type for concatenate: %s ~ %s\r\n",
				type_data::string_representation(type_l).c_str(),
				type_data::string_representation(type_r).c_str());
			machine->raise_error(error);
			return false;
		}
		return true;
	}
	value BaseFunction::concatenate(script_machine* machine, int argc, const value* argv) {
		_null_check(machine, argv, argc);
		__chk_concat(machine, argv[0].get_type(), argv[1].get_type());

		value result = argv[0];
		result.make_unique();

		value concat = argv[1];
		if (concat.get_type() != result.get_type()) {
			concat.make_unique();
			BaseFunction::_value_cast(&concat, result.get_type());
		}
		result.concatenate(concat);

		return result;
	}
	value BaseFunction::concatenate_direct(script_machine* machine, int argc, const value* argv) {
		_null_check(machine, argv, argc);
		__chk_concat(machine, argv[0].get_type(), argv[1].get_type());

		value result = argv[0];

		value concat = argv[1];
		if (concat.get_type() != result.get_type()) {
			concat.make_unique();
			BaseFunction::_value_cast(&concat, result.get_type());
		}
		result.concatenate(concat);

		return result;
	}

	value BaseFunction::round(script_machine* machine, int argc, const value* argv) {
		double r = std::floor(argv->as_real() + 0.5);
		return value(script_type_manager::get_real_type(), r);
	}
	DNH_FUNCAPI_DEF_(BaseFunction::round_base) {
		double v = argv[0].as_real();
		double base = argv[1].as_real();
		double r = std::floor(v / base + 0.5) * base;
		return value(script_type_manager::get_real_type(), r);
	}
	DNH_FUNCAPI_DEF_(BaseFunction::truncate) {
		double r = argv->as_real();
		return value(script_type_manager::get_real_type(), (r > 0) ? std::floor(r) : std::ceil(r));
	}
	value BaseFunction::ceil(script_machine* machine, int argc, const value* argv) {
		return value(script_type_manager::get_real_type(), std::ceil(argv->as_real()));
	}
	DNH_FUNCAPI_DEF_(BaseFunction::ceil_base) {
		double v = argv[0].as_real();
		double base = argv[1].as_real();
		return value(script_type_manager::get_real_type(), std::ceil(v / base) * base);
	}
	DNH_FUNCAPI_DEF_(BaseFunction::floor) {
		return value(script_type_manager::get_real_type(), std::floor(argv->as_real()));
	}
	DNH_FUNCAPI_DEF_(BaseFunction::floor_base) {
		double v = argv[0].as_real();
		double base = argv[1].as_real();
		return value(script_type_manager::get_real_type(), std::floor(v / base) * base);
	}
	value BaseFunction::_script_absolute(int argc, const value* argv) {
		return value(script_type_manager::get_real_type(), std::fabs(argv->as_real()));
	}
	SCRIPT_DECLARE_OP(absolute);

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
		if (_is_force_convert_real(argv[0].get_type()))
			return value(script_type_manager::get_real_type(), (double)res);
		else
			return value(argv->get_type(), res);
	}
	value BaseFunction::bitwiseRight(script_machine* machine, int argc, const value* argv) {
		int64_t val1 = argv[0].as_int();
		int64_t val2 = argv[1].as_int();
		int64_t res = val1 >> val2;
		if (_is_force_convert_real(argv[0].get_type()))
			return value(script_type_manager::get_real_type(), (double)res);
		else
			return value(argv->get_type(), res);
	}
#undef BITWISE_RET

	value BaseFunction::typeOf(script_machine* machine, int argc, const value* argv) {
		int64_t kind = _typeof(argv->get_type());
		return value(script_type_manager::get_int_type(), kind);
	}
	value BaseFunction::typeOfElem(script_machine* machine, int argc, const value* argv) {
		int64_t kind = _rtypeof(argv->get_type());
		return value(script_type_manager::get_int_type(), kind);
	}

	value BaseFunction::assert_(script_machine* machine, int argc, const value* argv) {
		if (!argv[0].as_boolean())
			machine->raise_error(argv[1].as_string());
		return value();
	}
	value BaseFunction::script_debugBreak(script_machine* machine, int argc, const value* argv) {
		//Prevents crashes if called without a debugger attached, not to prevent external debugging
		if (IsDebuggerPresent())
			DebugBreak();
		return value();
	}
}
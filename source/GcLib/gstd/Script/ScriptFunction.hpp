#pragma once

#include "../../pch.h"

#include "../Logger.hpp"
#include "Value.hpp"

namespace gstd {
	class script_engine;
	class script_machine;

	using dnh_func_callback_t = value (*)(script_machine*, int, const value*);

	//Breaks IntelliSense for some reason
#define DNH_FUNCAPI_DECL_(_fn) static gstd::value _fn (gstd::script_machine*, int, const gstd::value*)
#define DNH_FUNCAPI_DEF_(_fn) gstd::value _fn (gstd::script_machine* machine, int argc, const gstd::value* argv)

	struct function {
		const char* name;
		dnh_func_callback_t func;
		int argc;
		const char* signature;

		function(const char* name_, dnh_func_callback_t func_) : function(name_, func_, 0, "") {};
		function(const char* name_, dnh_func_callback_t func_, int argc_) : function(name_, func_, argc_, "") {};
		function(const char* name_, dnh_func_callback_t func_, int argc_, const char* signature_) : name(name_),
			func(func_), argc(argc_), signature(signature_) {};
	};
	struct constant {
		const char* name;
		type_data::type_kind type;
		uint64_t data;

		constant(const char* name_, int d_int) : constant(name_, (int64_t)d_int) {};
		constant(const char* name_, int64_t d_int);
		constant(const char* name_, double d_real);
		constant(const char* name_, wchar_t d_char);
		constant(const char* name_, bool d_bool);
	};

	class BaseFunction {
	public:
		static const void BaseFunction::_raise_error_unsupported(script_machine* machine, type_data* type, const std::string& op_name);

		static type_data::type_kind _type_test_promotion(type_data* type_l, type_data* type_r);

		static bool __type_assign_check(type_data* type_src, type_data* type_dst);
		static bool __type_assign_check_no_convert(type_data* type_src, type_data* type_dst);
		static bool _type_assign_check(script_machine* machine, const value* v_src, const value* v_dst);
		static bool _type_assign_check(script_machine* machine, type_data* type_src, type_data* type_dst);
		static bool _type_assign_check_no_convert(script_machine* machine, const value* v_src, const value* v_dst);

		static bool _type_check_two_any(type_data* type_l, type_data* type_r, uint8_t type);
		static bool _type_check_two_all(type_data* type_l, type_data* type_r, uint8_t type);

		static value __script_perform_op_array(const value* v_left, const value* v_right, value(*func)(int, const value*));

		inline static double _fmod2(double i, double j);
		inline static int64_t _mod2(int64_t i, int64_t j);

		static bool _is_empty_type(value* val);
		static bool _is_empty_type(type_data* type);
		static value* _value_cast(value* val, type_data* cast);

		static bool _index_check(script_machine* machine, type_data* arg0_type, size_t arg0_size, int index);
		static bool _append_check(script_machine* machine, type_data* arg0_type, type_data* arg1_type);
		static bool _append_check_no_convert(script_machine* machine, type_data* arg0_type, type_data* arg1_type);

		static bool _null_check(script_machine* machine, const value* argv, size_t count);

		static value _create_empty(type_data* type);

		static type_data::type_kind _typeof(type_data* type);
		static type_data::type_kind _rtypeof(type_data* type);
		static size_t _array_dimension(type_data* type);

		//---------------------------------------------------------------------

		static value _cast_array(script_machine* machine, const value* argv, type_data::type_kind target);
		DNH_FUNCAPI_DECL_(cast_int_array);
		DNH_FUNCAPI_DECL_(cast_real_array);
		DNH_FUNCAPI_DECL_(cast_bool_array);
		DNH_FUNCAPI_DECL_(cast_char_array);
		DNH_FUNCAPI_DECL_(cast_x_array);

		static value _script_add(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(add);
		static value _script_subtract(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(subtract);
		static value _script_multiply(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(multiply);
		static value _script_divide(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(divide);
		static value _script_fdivide(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(fdivide);
		static value _script_remainder_(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(remainder_);
		static value _script_negative(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(negative);
		static value _script_power(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(power);
		static value _script_compare(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(compare);
		static value _script_not_(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(not_);

		DNH_FUNCAPI_DECL_(modc);
		DNH_FUNCAPI_DECL_(apo);
		DNH_FUNCAPI_DECL_(predecessor);
		DNH_FUNCAPI_DECL_(successor);

		static const value* index(script_machine* machine, int argc, value* arr, value* indexer);

		DNH_FUNCAPI_DECL_(length);
		DNH_FUNCAPI_DECL_(generate);
		DNH_FUNCAPI_DECL_(resize);
		DNH_FUNCAPI_DECL_(reverse);
		DNH_FUNCAPI_DECL_(sort);
		DNH_FUNCAPI_DECL_(range);
		DNH_FUNCAPI_DECL_(contains);
		DNH_FUNCAPI_DECL_(indexof);
		DNH_FUNCAPI_DECL_(matches);
		DNH_FUNCAPI_DECL_(all);
		DNH_FUNCAPI_DECL_(any);
		DNH_FUNCAPI_DECL_(replace);
		DNH_FUNCAPI_DECL_(remove);

		DNH_FUNCAPI_DECL_(slice);
		DNH_FUNCAPI_DECL_(insert);
		DNH_FUNCAPI_DECL_(erase);
		DNH_FUNCAPI_DECL_(append);
		DNH_FUNCAPI_DECL_(concatenate);
		DNH_FUNCAPI_DECL_(concatenate_direct);

		DNH_FUNCAPI_DECL_(round);
		DNH_FUNCAPI_DECL_(round_base);
		DNH_FUNCAPI_DECL_(truncate);
		DNH_FUNCAPI_DECL_(ceil);
		DNH_FUNCAPI_DECL_(ceil_base);
		DNH_FUNCAPI_DECL_(floor);
		DNH_FUNCAPI_DECL_(floor_base);

		static value _script_absolute(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(absolute);

		DNH_FUNCAPI_DECL_(bitwiseNot);
		DNH_FUNCAPI_DECL_(bitwiseAnd);
		DNH_FUNCAPI_DECL_(bitwiseOr);
		DNH_FUNCAPI_DECL_(bitwiseXor);
		DNH_FUNCAPI_DECL_(bitwiseLeft);
		DNH_FUNCAPI_DECL_(bitwiseRight);

		DNH_FUNCAPI_DECL_(typeOf);
		DNH_FUNCAPI_DECL_(typeOfElem);

		DNH_FUNCAPI_DECL_(assert_);
		DNH_FUNCAPI_DECL_(script_debugBreak);
	};
}
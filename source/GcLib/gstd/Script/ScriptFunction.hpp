#pragma once

#include "../../pch.h"

#include "../Logger.hpp"

#include "LightweightVector.hpp"
#include "Value.hpp"

namespace gstd {
	class script_engine;
	class script_machine;

	typedef value (*callback)(script_machine* machine, int argc, const value* argv);

	//Breaks IntelliSense for some reason
#define DNH_FUNCAPI_DECL_(fn) static gstd::value fn (gstd::script_machine* machine, int argc, const gstd::value* argv)
#define DNH_FUNCAPI_(fn) gstd::value fn (gstd::script_machine* machine, int argc, const gstd::value* argv)

	struct function {
		const char* name;
		callback func;
		int arguments;
	};

	class BaseFunction {
	public:
		static double fmod2(double i, double j);

		static value __script_perform_op_array(const value* v_left, const value* v_right, value(*func)(int, const value*));

		static value _script_add(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(add);
		static value _script_subtract(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(subtract);
		static value _script_multiply(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(multiply);
		static value _script_divide(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(divide);
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
		DNH_FUNCAPI_DECL_(predecessor);
		DNH_FUNCAPI_DECL_(successor);
		DNH_FUNCAPI_DECL_(length);

		static bool _index_check(script_machine* machine, type_data* arg0_type, size_t arg0_size, double index);
		static const value& index(script_machine* machine, int argc, const value* argv);

		DNH_FUNCAPI_DECL_(slice);
		DNH_FUNCAPI_DECL_(erase);

		static bool _append_check(script_machine* machine, size_t arg0_size, type_data* arg0_type, type_data* arg1_type);
		DNH_FUNCAPI_DECL_(append);
		DNH_FUNCAPI_DECL_(concatenate);

		DNH_FUNCAPI_DECL_(round);
		DNH_FUNCAPI_DECL_(truncate);
		DNH_FUNCAPI_DECL_(ceil);
		DNH_FUNCAPI_DECL_(floor);

		static value _script_absolute(int argc, const value* argv);
		DNH_FUNCAPI_DECL_(absolute);

		DNH_FUNCAPI_DECL_(bitwiseNot);
		DNH_FUNCAPI_DECL_(bitwiseAnd);
		DNH_FUNCAPI_DECL_(bitwiseOr);
		DNH_FUNCAPI_DECL_(bitwiseXor);
		DNH_FUNCAPI_DECL_(bitwiseLeft);
		DNH_FUNCAPI_DECL_(bitwiseRight);

		DNH_FUNCAPI_DECL_(assert_);
		DNH_FUNCAPI_DECL_(script_debugBreak);
	};
}
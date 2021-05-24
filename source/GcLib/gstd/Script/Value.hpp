#pragma once

#include "../../pch.h"

namespace gstd {
	class type_data {
	public:
		typedef enum : uint8_t {
			tk_null = 0x00,
			tk_int = 0x01,
			tk_real = 0x02,
			tk_char = 0x04,
			tk_boolean = 0x08,
			tk_array = 0x10,
			tk_pointer = 0x20,
		} type_kind;

		type_data(type_kind k, type_data* t = nullptr) : kind(k), element(t) {}
		type_data(const type_data& source) : kind(source.kind), element(source.element) {}

		type_kind get_kind() { return kind; }
		type_data* get_element() { return element; }

		static std::string string_representation(type_data* data);

		bool operator==(const type_data& other);
		bool operator!=(const type_data& other) { return !this->operator==(other); }
		bool operator<(const type_data& other) const;
	private:
		type_kind kind = type_kind::tk_null;
		type_data* element = nullptr;
	};

	class value {
	private:
		type_data::type_kind kind = type_data::tk_null;
		type_data* type = nullptr;

		union {
			struct {
				double real_value;
				wchar_t char_value;
				bool boolean_value;
				int64_t int_value;
				value* ptr_value;
			};
			std::vector<value> array_value;
		};
	public:
		value() {}
		value(type_data* t, int64_t v);
		value(type_data* t, double v);
		value(type_data* t, wchar_t v);
		value(type_data* t, bool v);
		value(type_data* t, value* v);
		value(type_data* t, const std::wstring& v);
		value(const value& source) {
			*this = source;
		}

		~value();
		void release();

		value& operator=(const value& source);

		//--------------------------------------------------------------------------

		bool has_data() const { return type != nullptr; }
		type_data* get_type() const { return type; }

		size_t length_as_array() const;
		const value& index_as_array(size_t i) const;
		value& index_as_array(size_t i);

		std::vector<value>::iterator array_get_begin() const;
		std::vector<value>::iterator array_get_end() const;

		//--------------------------------------------------------------------------

		value* reset(type_data* t, int64_t v);
		value* reset(type_data* t, double v);
		value* reset(type_data* t, wchar_t v);
		value* reset(type_data* t, bool v);
		value* reset(type_data* t, value* v);
		value* reset(type_data* t, std::vector<value>& v);
		value* set(type_data* t, int64_t v);
		value* set(type_data* t, double v);
		value* set(type_data* t, wchar_t v);
		value* set(type_data* t, bool v);
		value* set(type_data* t, value* v);
		value* set(type_data* t, std::vector<value>& v);

		void append(type_data* t, const value& x);
		void concatenate(const value& x);

		//--------------------------------------------------------------------------

		int64_t as_int() const;
		double as_real() const;
		wchar_t as_char() const;
		bool as_boolean() const;
		value* as_ptr() const { return ptr_value; }
		std::wstring as_string() const;
	};
}
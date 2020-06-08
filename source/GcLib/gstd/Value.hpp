#pragma once
#include "../pch.h"

#include "LightweightVector.hpp"

namespace gstd {
	class type_data {
	public:
		enum class type_kind : uint8_t {
			tk_real, tk_char, tk_boolean, tk_array
		};

		type_data(type_kind k, type_data* t = nullptr) : kind(k), element(t) {}
		type_data(type_data const& source) : kind(source.kind), element(source.element) {}

		//デストラクタはデフォルトに任せる

		type_kind get_kind() { return kind; }
		type_data* get_element() { return element; }

		bool operator==(const type_data& other);
		bool operator<(const type_data& other) const;
	private:
		type_kind kind;
		type_data* element;
	};

	class value {
	public:
		static value val_empty;
	public:
		value() : data(nullptr) {}
		value(type_data* t, double v);
		value(type_data* t, wchar_t v);
		value(type_data* t, bool v);
		value(type_data* t, std::wstring v);
		value(value const& source) {
			data = source.data;
		}

		~value() {
			release();
		}

		value& operator=(value const& source) {
			data = source.data;
			return *this;
		}

		bool has_data() const { return data != nullptr; }

		void set(type_data* t, double v);
		void set(type_data* t, bool v);
		void set(type_data* t, std::vector<value>& v);

		void append(type_data* t, value const& x);
		void concatenate(value const& x);

		double as_real() const;
		wchar_t as_char() const;
		bool as_boolean() const;
		std::wstring as_string() const;

		size_t length_as_array() const { return data->array_value.size(); }
		value const& index_as_array(size_t i) const { return data->array_value[i]; }
		value& index_as_array(size_t i) { return data->array_value[i]; }
		type_data* get_type() const { return data->type; }

		std::vector<value>::iterator array_get_begin() { return data->array_value.begin(); }
		std::vector<value>::iterator array_get_end() { return data->array_value.end(); }

		void overwrite(const value& source);	//危険！外から呼ぶな
		static value new_from(const value& source);

		void unique() const;
	private:
		inline void release() {
			if (data) data.reset();
		}
		struct body {
			type_data* type = nullptr;
			std::vector<value> array_value;

			union {
				double real_value = 0.0;
				wchar_t char_value;
				bool boolean_value;
			};
		};

		mutable std::shared_ptr<body> data;
	};
}
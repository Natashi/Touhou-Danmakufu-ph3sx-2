#include "source/GcLib/pch.h"

#include "../GstdUtility.hpp"
#include "Value.hpp"

using namespace gstd;

std::string type_data::string_representation(type_data* data) {
	if (data == nullptr) return "[null]";
	switch (data->get_kind()) {
	case type_kind::tk_null:
		return "null";
	case type_kind::tk_int:
		return "int";
	case type_kind::tk_real:
		return "real";
	case type_kind::tk_char:
		return "char";
	case type_kind::tk_boolean:
		return "bool";
	case type_kind::tk_array:
	{
		type_data* elem = data->get_element();
		if (elem != nullptr && elem->get_kind() == type_kind::tk_char)
			return "string";
		return string_representation(elem) + "-array";
	}
	}
	return "invalid";
}
bool type_data::operator==(const type_data& other) {
	if (kind != other.kind) return false;
	if (element != nullptr && other.element != nullptr)
		return (*element) == (*other.element);
	return element == other.element;
}
bool type_data::operator<(const type_data& other) const {
	if (kind != other.kind) return ((uint8_t)kind < (uint8_t)other.kind);
	if (element != nullptr && other.element != nullptr)
		return (*element) < (*other.element);
	return element == nullptr && other.element != nullptr;
}

value value::val_empty = value();
value::value(type_data* t, int64_t v) {
	data = std::make_shared<body>();
	data->type = t;
	data->int_value = v;
}
value::value(type_data* t, double v) {
	data = std::make_shared<body>();
	data->type = t;
	data->real_value = v;
}
value::value(type_data* t, wchar_t v) {
	data = std::make_shared<body>();
	data->type = t;
	data->char_value = v;
}
value::value(type_data* t, bool v) {
	data = std::make_shared<body>();
	data->type = t;
	data->boolean_value = v;
}
value::value(type_data* t, const std::wstring& v) {
	data = std::make_shared<body>();
	data->type = t;
	for (wchar_t ch : v)
		data->array_value.push_back(value(t->get_element(), ch));
}

value* value::set(type_data* t, int64_t v) {
	unique();
	data->type = t;
	data->int_value = v;
	return this;
}
value* value::set(type_data* t, double v) {
	unique();
	data->type = t;
	data->real_value = v;
	return this;
}
value* value::set(type_data* t, wchar_t v) {
	unique();
	data->type = t;
	data->char_value = v;
	return this;
}
value* value::set(type_data* t, bool v) {
	unique();
	data->type = t;
	data->boolean_value = v;
	return this;
}
value* value::set(type_data* t, std::vector<value>& v) {
	unique();
	data->type = t;
	data->array_value = v;
	return this;
}

void value::append(type_data* t, const value& x) {
	unique();
	data->type = t;
	data->array_value.push_back(x);
}
void value::concatenate(const value& x) {
	unique();
	if (data->type->get_element() == nullptr) data->type = x.data->type;
	data->array_value.insert(array_get_end(), x.array_get_begin(), x.array_get_end());
}

void value::overwrite(const value& source) {
	if (data == nullptr) return;
	if (std::addressof(data) == std::addressof(source.data)) return;

	*data = *source.data;
}
value value::new_from(const value& source) {
	value res = source;
	res.unique();
	return res;
}

void value::unique() const {
	if (data == nullptr) {
		data = std::make_shared<body>();
		data->type = nullptr;
	}
	else if (!data.unique()) {
		data = std::make_shared<body>(*data);
	}
}

int64_t value::as_int() const {
	if (data == nullptr) return 0i64;
	uint8_t kind = data->type->get_kind();
	if (kind & type_data::tk_int)
		return data->int_value;
	if (kind & type_data::tk_real)
		return (int64_t)(data->real_value + (data->real_value > 0 ? 0.01 : -0.01));
	if (kind & type_data::tk_char)
		return (int64_t)data->char_value;
	if (kind & type_data::tk_boolean)
		return (int64_t)data->boolean_value;
	if (kind & type_data::tk_array) {
		if (data->type->get_element()->get_kind() == type_data::type_kind::tk_char) {
			try {
				return std::stoll(as_string());
			}
			catch (...) {
				return 0i64;
			}
		}
		else return length_as_array();
	}
	return 0i64;
}
double value::as_real() const {
	if (data == nullptr) return 0.0;
	uint8_t kind = data->type->get_kind();
	if (kind & type_data::tk_real)
		return data->real_value;
	if (kind & type_data::tk_int)
		return (double)data->int_value;
	if (kind & type_data::tk_char)
		return (double)data->char_value;
	if (kind & type_data::tk_boolean)
		return (double)data->boolean_value;
	if (kind & type_data::tk_array) {
		if (data->type->get_element()->get_kind() == type_data::type_kind::tk_char) {
			try {
				return std::stod(as_string());
			}
			catch (...) {
				return 0.0;
			}
		}
		else return length_as_array();
	}
	return 0.0;
}
wchar_t value::as_char() const {
	if (data == nullptr) return L'\0';
	uint8_t kind = data->type->get_kind();
	if (kind & type_data::tk_char)
		return data->char_value;
	if (kind & type_data::tk_int)
		return (wchar_t)data->int_value;
	if (kind & type_data::tk_real)
		return (wchar_t)data->real_value;
	if (kind & type_data::tk_boolean)
		return data->boolean_value ? L'1' : L'0';
	if (kind & type_data::tk_array)
		return L'\0';
	return L'\0';
}
bool value::as_boolean() const {
	if (data == nullptr) return false;
	uint8_t kind = data->type->get_kind();
	if (kind & type_data::tk_boolean)
		return data->boolean_value;
	if (kind & (type_data::tk_real | type_data::tk_int))
		return data->int_value;
	if (kind & type_data::tk_char)
		return data->char_value;
	if (kind & type_data::tk_array)
		return data->array_value.size();
	return false;
}
std::wstring value::as_string() const {
	if (data == nullptr) return L"(NULL)";
	uint8_t kind = data->type->get_kind();
	if (kind & type_data::tk_real)
		return std::to_wstring(data->real_value);
	if (kind & type_data::tk_int)
		return std::to_wstring(data->int_value);
	if (kind & type_data::tk_boolean)
		return data->boolean_value ? L"true" : L"false";
	if (kind & type_data::tk_char)
		return std::wstring(&data->char_value, 1);
	if (kind & type_data::tk_array) {
		std::wstring result = L"";
		if (type_data* elem = data->type->get_element()) {
			if (elem->get_kind() == type_data::type_kind::tk_char) {
				for (auto& iChar : data->array_value)
					result += iChar.as_char();
			}
			else {
				result = L"[";
				auto itr = data->array_value.begin();
				while (itr != data->array_value.end()) {
					result += itr->as_string();
					if ((++itr) == data->array_value.end()) break;
					result += L",";
				}
				result += L"]";
			}
		}
		else result = L"[]";
		return result;
	}
	return L"(INVALID-TYPE)";
}
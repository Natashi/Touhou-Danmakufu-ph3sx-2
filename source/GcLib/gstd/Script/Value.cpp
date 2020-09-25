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
		if (elem->get_kind() == type_kind::tk_char)
			return "string";
		return string_representation(elem) + "-array";
	}
	}
	return "invalid";
}
bool type_data::operator==(const type_data& other) {
	if (this->kind != other.kind) return false;

	//Same element or both null
	if (this->element == other.element) return true;
	//Either null
	else if (this->element == nullptr || other.element == nullptr) return false;

	return (*this->element == *other.element);
}
bool type_data::operator<(const type_data& other) const {
	if (kind != other.kind) return ((uint8_t)kind < (uint8_t)other.kind);
	if (element == nullptr || other.element == nullptr) return false;
	return (*element) < (*other.element);
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

void value::set(type_data* t, int64_t v) {
	unique();
	data->type = t;
	data->int_value = v;
}
void value::set(type_data* t, double v) {
	unique();
	data->type = t;
	data->real_value = v;
}
void value::set(type_data* t, wchar_t v) {
	unique();
	data->type = t;
	data->char_value = v;
}
void value::set(type_data* t, bool v) {
	unique();
	data->type = t;
	data->boolean_value = v;
}
void value::set(type_data* t, std::vector<value>& v) {
	unique();
	data->type = t;
	data->array_value = v;
}

void value::append(type_data* t, const value& x) {
	unique();
	data->type = t;
	data->array_value.push_back(x);
}
void value::concatenate(const value& x) {
	unique();

	if (length_as_array() == 0) data->type = x.data->type;
	for (auto itr = x.data->array_value.begin(); itr != x.data->array_value.end(); ++itr) {
		data->array_value.push_back(*itr);
	}
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
	if (kind & (type_data::tk_char | type_data::tk_boolean))
		return (int64_t)data->char_value;
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
	if (kind & (type_data::tk_char | type_data::tk_boolean))
		return (double)data->char_value;
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
		if (type_data* elem = data->type->get_element()) {
			if (elem->get_kind() == type_data::type_kind::tk_char) {
				std::wstring result = L"";
				for (auto& iChar : data->array_value)
					result += iChar.as_char();
				return result;
			}
			else {
				std::wstring result = L"[";
				auto itr = data->array_value.begin();
				while (itr != data->array_value.end()) {
					result += itr->as_string();
					if ((++itr) == data->array_value.end()) break;
					result += L",";
				}
				result += L"]";
				return result;
			}
		}
	}
	return L"(INVALID-TYPE)";
}
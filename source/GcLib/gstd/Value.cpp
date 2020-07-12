#include "source/GcLib/pch.h"

#include "Value.hpp"
#include "GstdUtility.hpp"

using namespace gstd;

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
		value v = *itr;
		data->array_value.push_back(v);
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
	if (data == nullptr)
		return 0i64;
	switch (data->type->get_kind()) {
	case type_data::type_kind::tk_int:
		return data->int_value;
	case type_data::type_kind::tk_real:
		return (int64_t)(data->real_value + 0.01);
	case type_data::type_kind::tk_char:
		return (int64_t)data->char_value;
	case type_data::type_kind::tk_boolean:
		return data->boolean_value ? 1i64 : 0i64;
	case type_data::type_kind::tk_array:
		if (data->type->get_element()->get_kind() == type_data::type_kind::tk_char) {
			try {
				return std::stoll(as_string());
			}
			catch (...) {
				return 0i64;
			}
		}
		else
			return length_as_array();
	}
	assert(false);
	return 0i64;
}
double value::as_real() const {
	if (data == nullptr)
		return 0.0;
	switch (data->type->get_kind()) {
	case type_data::type_kind::tk_int:
		return (double)data->int_value;
	case type_data::type_kind::tk_real:
		return data->real_value;
	case type_data::type_kind::tk_char:
		return (double)data->char_value;
	case type_data::type_kind::tk_boolean:
		return (data->boolean_value) ? 1.0 : 0.0;
	case type_data::type_kind::tk_array:
		if (data->type->get_element()->get_kind() == type_data::type_kind::tk_char) {
			try {
				return std::stol(as_string());
			}
			catch (...) {
				return 0.0;
			}
		}
		else
			return length_as_array();
	}
	assert(false);
	return 0.0;
}
wchar_t value::as_char() const {
	if (data == nullptr)
		return L'\0';
	switch (data->type->get_kind()) {
	case type_data::type_kind::tk_int:
		return (wchar_t)data->int_value;
	case type_data::type_kind::tk_real:
		return (wchar_t)data->real_value;
	case type_data::type_kind::tk_char:
		return (wchar_t)data->char_value;
	case type_data::type_kind::tk_boolean:
		return (data->boolean_value) ? L'1' : L'0';
	case type_data::type_kind::tk_array:
		return L'\0';
	}
	assert(false);
	return L'\0';
}
bool value::as_boolean() const {
	if (data == nullptr)
		return false;
	switch (data->type->get_kind()) {
	case type_data::type_kind::tk_int:
		return data->int_value != 0;
	case type_data::type_kind::tk_real:
		return data->real_value != 0.0;
	case type_data::type_kind::tk_char:
		return data->char_value != L'\0';
	case type_data::type_kind::tk_boolean:
		return data->boolean_value;
	case type_data::type_kind::tk_array:
		return data->array_value.size() != 0U;
	}
	assert(false);
	return false;
}
std::wstring value::as_string() const {
	if (data == nullptr)
		return L"(NULL)";
	switch (data->type->get_kind()) {
	case type_data::type_kind::tk_int:
		return std::to_wstring(data->int_value);
	case type_data::type_kind::tk_real:
		return std::to_wstring(data->real_value);
	case type_data::type_kind::tk_char:
		return std::wstring(&data->char_value, 1);
	case type_data::type_kind::tk_boolean:
		return (data->boolean_value) ? L"true" : L"false";
	case type_data::type_kind::tk_array:
	{
		type_data* elem = data->type->get_element();
		if (elem != nullptr && elem->get_kind() == type_data::type_kind::tk_char) {
			std::wstring result = L"";
			for (auto itr = data->array_value.begin(); itr != data->array_value.end(); ++itr)
				result += itr->as_char();
			return result;
		}
		else {
			std::wstring result = L"[";
			auto itrLast = data->array_value.rbegin().base();
			for (auto itr = data->array_value.begin(); itr != data->array_value.end(); ++itr) {
				result += itr->as_string();
				if (itr != itrLast) result += L",";
			}
			result += L"]";
			return result;
		}
	}
	}
	assert(false);
	return L"(INVALID-TYPE)";
}

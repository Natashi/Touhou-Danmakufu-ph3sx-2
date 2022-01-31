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
	case type_kind::tk_float:
		return "float";
	case type_kind::tk_char:
		return "char";
	case type_kind::tk_boolean:
		return "bool";
	case type_kind::tk_pointer:
		return "pointer";
	case type_kind::tk_array:
	{
		type_data* elem = data->get_element();
		if (elem != nullptr && elem->get_kind() == type_kind::tk_char)
			return "string";
		return string_representation(elem) + "[]";
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
	if (kind != other.kind)
		return ((uint8_t)kind < (uint8_t)other.kind);
	if (element != nullptr && other.element != nullptr)
		return (*element) < (*other.element);
	return element == nullptr && other.element != nullptr;
}

value::value(type_data* t, int64_t v) {
	this->set(t, v);
}
value::value(type_data* t, double v) {
	this->set(t, v);
}
value::value(type_data* t, wchar_t v) {
	this->set(t, v);
}
value::value(type_data* t, bool v) {
	this->set(t, v);
}
value::value(type_data* t, value* v) {
	this->set(t, v);
}
value::value(type_data* t, const std::wstring& v) {
	std::vector<value> vec(v.size());
	for (size_t i = 0; i < v.size(); ++i)
		vec[i] = value(t->get_element(), v[i]);
	this->set(t, vec);
}
value::~value() {
	this->release();
}

void value::release() {
	if (!has_data()) return;
	if (kind == type_data::tk_array)
		p_array_value.~ref_count_ptr();
}

value* value::reset(type_data* t, int64_t v) {
	release();
	return this->set(t, v);
}
value* value::reset(type_data* t, double v) {
	release();
	return this->set(t, v);
}
value* value::reset(type_data* t, wchar_t v) {
	release();
	return this->set(t, v);
}
value* value::reset(type_data* t, bool v) {
	release();
	return this->set(t, v);
}
value* value::reset(type_data* t, value* v) {
	release();
	return this->set(t, v);
}
value* value::reset(type_data* t, std::vector<value>& v) {
	release();
	return this->set(t, v);
}

#pragma push_macro("new")
#undef new
value* value::set(type_data* t, int64_t v) {
	kind = type_data::tk_int;
	type = t;
	new (&int_value) auto(v);
	return this;
}
value* value::set(type_data* t, double v) {
	kind = type_data::tk_float;
	type = t;
	new (&float_value) auto(v);
	return this;
}
value* value::set(type_data* t, wchar_t v) {
	kind = type_data::tk_char;
	type = t;
	new (&char_value) auto(v);
	return this;
}
value* value::set(type_data* t, bool v) {
	kind = type_data::tk_boolean;
	type = t;
	new (&boolean_value) auto(v);
	return this;
}
value* value::set(type_data* t, value* v) {
	kind = type_data::tk_pointer;
	type = t;
	new (&ptr_value) auto(v);
	return this;
}
value* value::set(type_data* t, std::vector<value>& v) {
	kind = type_data::tk_array;
	type = t;
	ref_unsync_ptr<std::vector<value>> nv = new std::vector<value>(v);
	new (&p_array_value) auto(nv);
	return this;
}
value* value::set(type_data* t, ref_unsync_ptr<std::vector<value>> v) {
	kind = type_data::tk_array;
	type = t;
	new (&p_array_value) auto(v);
	return this;
}
value* value::set(type_data* t) {
	kind = t ? t->get_kind() : type_data::tk_null;
	type = t;
	return this;
}
#pragma pop_macro("new")

value& value::operator=(const value& source) {
	if (this == std::addressof(source)) return *this;
	this->~value();

	kind = source.kind;
	type = source.type;
	if (type) {
		if (kind == type_data::tk_int)
			this->set(source.type, source.int_value);
		else if (kind == type_data::tk_float)
			this->set(source.type, source.float_value);
		else if (kind == type_data::tk_char)
			this->set(source.type, source.char_value);
		else if (kind == type_data::tk_boolean)
			this->set(source.type, source.boolean_value);
		else if (kind == type_data::tk_pointer)
			this->set(source.type, source.ptr_value);
		else if (kind == type_data::tk_array)
			this->set(source.type, source.p_array_value);
	}

	return *this;
}

void value::make_unique() {
	if (has_data() && kind == type_data::tk_array) {
		if (p_array_value.use_count() == 1) return;
		std::vector<value> vec = *p_array_value.get();
		for (value& v : vec)
			v.make_unique();
		this->reset(type, vec);
	}
}

void value::append(type_data* t, const value& x) {
	if (!has_data() || kind != type_data::tk_array)
		this->reset(t, std::vector<value>());
	//make_unique();
	type = t;
	p_array_value->push_back(x);
}
void value::concatenate(const value& x) {
	if (!has_data() || kind != type_data::tk_array)
		this->reset(x.type, std::vector<value>());
	//make_unique();
	if (type->get_element() == nullptr)
		type = x.type;
	p_array_value->insert(array_get_end(),
		x.array_get_begin(), x.array_get_end());
}

size_t value::length_as_array() const {
	if (has_data() && kind == type_data::tk_array)
		return p_array_value->size();
	return 0U;
}
const value& value::index_as_array(size_t i) const {
	if (has_data() && kind == type_data::tk_array)
		return p_array_value->at(i);
	throw wexception("index_as_array: not an array");
}
value& value::index_as_array(size_t i) {
	if (has_data() && kind == type_data::tk_array)
		return p_array_value->at(i);
	throw wexception("index_as_array: not an array");
}
std::vector<value>::iterator value::array_get_begin() const {
	if (has_data() && kind == type_data::tk_array)
		return p_array_value->begin();
	return std::vector<value>::iterator();
}
std::vector<value>::iterator value::array_get_end() const {
	if (has_data() && kind == type_data::tk_array)
		return p_array_value->end();
	return std::vector<value>::iterator();
}

int64_t value::as_int() const {
	if (!has_data()) return 0i64;
	if (kind == type_data::tk_int)
		return int_value;
	if (kind == type_data::tk_float) {
		constexpr double EPSILON = 0.000001;
		return (int64_t)(float_value + (float_value > 0 ? EPSILON : -EPSILON));
	}
	if (kind == type_data::tk_char)
		return (int64_t)char_value;
	if (kind == type_data::tk_boolean)
		return (int64_t)boolean_value;
	if (kind == type_data::tk_pointer)
		return (uint32_t)ptr_value;
	if (kind == type_data::tk_array) {
		if (type->get_element()->get_kind() == type_data::tk_char) {
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
double value::as_float() const {
	if (!has_data()) return 0.0;
	if (kind == type_data::tk_float)
		return float_value;
	if (kind == type_data::tk_int)
		return (double)int_value;
	if (kind == type_data::tk_char)
		return (double)char_value;
	if (kind == type_data::tk_boolean)
		return (double)boolean_value;
	if (kind == type_data::tk_pointer)
		return (uint32_t)ptr_value;
	if (kind == type_data::tk_array) {
		if (type->get_element()->get_kind() == type_data::tk_char) {
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
	if (!has_data()) return L'\0';
	if (kind == type_data::tk_char)
		return char_value;
	if (kind == type_data::tk_int)
		return (wchar_t)int_value;
	if (kind == type_data::tk_float)
		return (wchar_t)float_value;
	if (kind == type_data::tk_boolean)
		return boolean_value ? L'1' : L'0';
	if (kind == type_data::tk_pointer)
		return (wchar_t)(ptr_value != nullptr);
	if (kind == type_data::tk_array)
		return L'\0';
	return L'\0';
}
bool value::as_boolean() const {
	if (!has_data()) return false;
	if (kind == type_data::tk_boolean)
		return boolean_value;
	if (kind == type_data::tk_int)
		return (int_value != 0);
	if (kind == type_data::tk_float)
		return (float_value != 0.0);
	if (kind == type_data::tk_char)
		return (char_value != L'\0');
	if (kind == type_data::tk_pointer)
		return (ptr_value != nullptr);
	if (kind == type_data::tk_array)
		return (p_array_value->size() != 0U);
	return false;
}
std::wstring value::as_string() const {
	if (!has_data()) return L"(NULL)";
	if (kind == type_data::tk_float)
		return std::to_wstring(float_value);
	if (kind == type_data::tk_int)
		return std::to_wstring(int_value);
	if (kind == type_data::tk_boolean)
		return boolean_value ? L"true" : L"false";
	if (kind == type_data::tk_char)
		return std::wstring(&char_value, 1);
	if (kind == type_data::tk_pointer)
		return StringUtility::Format(L"%08x", (uint32_t)ptr_value);
	if (kind == type_data::tk_array) {
		std::wstring result = L"";
		if (type_data* elem = type->get_element()) {
			if (elem->get_kind() == type_data::tk_char) {
				for (auto itr = p_array_value->begin(); itr != p_array_value->end(); ++itr)
					result += itr->as_char();
			}
			else {
				result = L"[";
				auto itr = p_array_value->begin();
				while (itr != p_array_value->end()) {
					result += itr->as_string();
					if ((++itr) == p_array_value->end()) break;
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
ref_unsync_ptr<std::vector<value>> value::as_array_ptr() const {
	if (!has_data()) return nullptr;
	if (kind == type_data::tk_array)
		return p_array_value;
	return nullptr;
}
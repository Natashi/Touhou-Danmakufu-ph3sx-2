#include "source/GcLib/pch.h"

#include "ValueVector.hpp"

using namespace gstd;

script_value_vector::script_value_vector() : length(0), capacity(0), at(nullptr) {
	expand();
}
script_value_vector::~script_value_vector() {
	release();
}

script_value_vector& script_value_vector::operator=(const script_value_vector& source) {
	if (this == std::addressof(source)) return *this;

	ptr_delete_scalar(at);

	length = source.length;
	capacity = source.capacity;

	if (source.capacity > 0) {
		at = new value[source.capacity];
		for (size_t i = 0; i < length; ++i)
			at[i] = source.at[i];
		_fill_with_empty(at + length, source.capacity - length);
	}

	return *this;
}

void script_value_vector::_fill_with_empty(value* pos, size_t count) {
	for (size_t i = 0; i < count; ++i, ++pos)
		*pos = value();
}
void script_value_vector::_relink_pointers(value* oldAt, size_t oldSize, value* newAt) {
	//If a value of pointer type was referencing some value in the stack,
	//	that pointer will become invalid after a resize, so it has to be corrected.

	for (size_t i = 0; i < oldSize; ++i) {
		if (!oldAt->has_data()) continue;
		if (oldAt->get_type()->get_kind() == type_data::tk_pointer) {
			value* linking = oldAt->as_ptr();
			ptrdiff_t pDist = linking - oldAt;
			if (pDist >= 0 && pDist < oldSize)	//Check if ptr points to a value in the stack
				newAt[i].set(oldAt->get_type(), (value*)(newAt + pDist));
		}
	}
}

void script_value_vector::clear() {
	_fill_with_empty(at, length);
	length = 0;
}
void script_value_vector::release() {
	length = 0;
	if (at) capacity = 0;
	ptr_delete_scalar(at);
}

void script_value_vector::resize(size_t newSize) {
	while (capacity < newSize)
		expand();
}
void script_value_vector::expand() {
	if (capacity == 0) {
		constexpr const size_t INI_SIZE = 16U;
		capacity = INI_SIZE;
		at = new value[INI_SIZE];
		_fill_with_empty(at, INI_SIZE);
	}
	else {
		if (capacity < 0x400000) capacity = capacity << 1;
		else if (capacity == 0x400000) capacity = 0x4fffff;
		else throw wexception("Cannot expand script vector any further. (max = 5242879)");

		value* n = new value[capacity];
		for (size_t i = 0; i < length; ++i)
			n[i] = at[i];
		_relink_pointers(at, length, n);
		_fill_with_empty(n + length, capacity - length);

		delete[] at;
		at = n;
	}
}

void script_value_vector::push_back(const value& value) {
	at[length++] = value;
	if (length + 1 >= capacity) expand();
}
void script_value_vector::pop_back(size_t count) {
	if (length < count) count = length;
	length -= count;
	//_fill_with_empty(at + length, count);
}

void script_value_vector::erase(value* pos) {
	if (length == 0) return;
	--length;
	for (value* i = pos; i < at + length; ++i)
		*i = *(i + 1);
	_fill_with_empty(at + length, capacity - length);
}
void script_value_vector::insert(value* pos, const value& val) {
	if (length == capacity) {
		size_t pos_index = pos - at;
		expand();
		pos = at + pos_index;
	}
	for (value* i = at + length; i > pos; --i)
		*i = *(i - 1);
	*pos = val;
	++length;
}
#pragma once

#include "../../pch.h"
#include "../GstdUtility.hpp"

#include "Value.hpp"

namespace gstd {
	class script_value_vector {
	public:
		size_t length;
		size_t capacity;
		value* at = nullptr;
	private:
		void _fill_with_empty(value* pos, size_t count);
		void _relink_pointers(value* oldAt, size_t oldSize, value* newAt);
	public:
		script_value_vector();
		script_value_vector(const script_value_vector& source) {
			*this = source;
		}
		~script_value_vector();

		script_value_vector& operator=(const script_value_vector& source);

		void resize(size_t newSize);
		void expand();

		void push_back(const value& value);
		void pop_back(size_t count = 1U);

		void clear();
		void release();

		size_t size() const {
			return length;
		}

		value& operator[] (size_t i) {
			return at[i];
		}
		const value& operator[] (size_t i) const {
			return at[i];
		}

		value& back() {
			return at[length - 1];
		}
		value* begin() {
			return &at[0];
		}

		void erase(value* pos);
		void insert(value* pos, const value& value);
	};
}
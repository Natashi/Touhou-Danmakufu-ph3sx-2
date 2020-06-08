#pragma once

#include "../pch.h"
#include "GstdUtility.hpp"

namespace gstd {
	template<typename T>
	class script_vector {
	public:
		size_t length;
		size_t capacity;
		T* at;

		script_vector() : length(0), capacity(0), at(nullptr) {
		}
		script_vector(const script_vector& source);

		~script_vector() {
			ptr_delete_scalar(at);
		}

		script_vector& operator=(const script_vector& source);

		void fill_with_empty(T* pos, size_t count);
		void resize(size_t newSize);
		void expand();

		inline void push_back(const T& value) {
			if (length == capacity) expand();
			at[length++] = value;
		}
		inline void pop_back(size_t count = 1U) {
			if (length < count) count = length;
			length -= count;
			//fill_with_empty(at + length, count);
		}

		void clear() {
			fill_with_empty(at, length);
			length = 0;
		}

		inline void release() {
			length = 0;
			if (at) capacity = 0;
			ptr_delete_scalar(at);
		}

		size_t size() const {
			return length;
		}

		T& operator[] (size_t i) {
			return at[i];
		}
		const T& operator[] (size_t i) const {
			return at[i];
		}

		T& back() {
			return at[length - 1];
		}
		T* begin() {
			return &at[0];
		}

		void erase(T* pos);
		void insert(T* pos, T const& value);
	};

	template<typename T>
	void script_vector<T>::resize(size_t newSize) {
		while (capacity < newSize) expand();
	}
	template<typename T>
	void script_vector<T>::fill_with_empty(T* pos, size_t count) {
		//T empty;
		for (size_t i = 0; i < count; ++i, ++pos) *pos = T::val_empty;
	}

	template<typename T>
	script_vector<T>::script_vector(const script_vector& source) {
		length = source.length;
		capacity = source.capacity;

		if (source.capacity > 0) {
			at = new T[source.capacity];
			for (size_t i = 0; i < length; ++i) {
				at[i] = source.at[i];
			}
			fill_with_empty(at + length, source.capacity - length);
		}
		else {
			at = nullptr;
		}
	}

	template<typename T>
	script_vector<T>& script_vector<T>::operator=(const script_vector<T>& source) {
		if (this == std::addressof(source)) return *this;

		ptr_delete_scalar(at);

		length = source.length;
		capacity = source.capacity;

		if (source.capacity > 0) {
			at = new T[source.capacity];
			for (size_t i = 0; i < length; ++i) {
				at[i] = source.at[i];
			}
			fill_with_empty(at + length, source.capacity - length);
		}

		return *this;
	}

	template<typename T>
	void script_vector<T>::expand() {
		if (capacity == 0) {
			capacity = 0x4;
			at = new T[0x4];
			fill_with_empty(at, 0x4);
		}
		else {
			if (capacity < 0x800000) capacity = capacity << 1;
			else if (capacity == 0x800000) capacity = 0x8fffff;
			else throw gstd::wexception("Cannot expand script vector any further. (max = 9437183)");

			T* n = new T[capacity];
			for (size_t i = 0; i < length; ++i) {
				n[i] = at[i];
			}
			fill_with_empty(n + length, capacity - length);

			delete[] at;
			at = n;
		}
	}

	template<typename T>
	void script_vector<T>::erase(T* pos) {
		if (length == 0) return;
		--length;
		for (T* i = pos; i < at + length; ++i) {
			*i = *(i + 1);
		}
		fill_with_empty(at + length, source.capacity - length);
	}
	template<typename T>
	void script_vector<T>::insert(T* pos, const T& value) {
		if (length == capacity) {
			size_t pos_index = pos - at;
			expand();
			pos = at + pos_index;
		}
		for (T* i = at + length; i > pos; --i) {
			*i = *(i - 1);
		}
		*pos = value;
		++length;
	}
}
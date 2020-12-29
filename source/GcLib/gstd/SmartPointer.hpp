#pragma once

//Update: I found a use for these.
//	I'm gonna be using these for non-atomic, performance-sensitive parts of the engine from now on.

#include "../pch.h"

namespace gstd {
	//Base for reference counting
	template<class T>
	class _ptr_ref_counter {
	public:
		uint32_t countRef_ = 0;		//Strong refs
		uint32_t countWeak_ = 0;	//Weak refs
		T* pPtr_ = nullptr;			//Pointer
	public:
		_ptr_ref_counter(T* src) noexcept {
			pPtr_ = src;
		}
		template<class U> _ptr_ref_counter(const _ptr_ref_counter<U>& other) noexcept {
			countRef_ = other.countRef_;
			countWeak_ = other.countWeak_;
			pPtr_ = (T*)other.pPtr_;
		}

		_ptr_ref_counter& operator=(_ptr_ref_counter& other) = default;
		template<class U> _ptr_ref_counter<T>& operator=(_ptr_ref_counter<U>& other) {
			countRef_ = other.countRef_;
			countWeak_ = other.countWeak_;
			pPtr_ = (T*)other.pPtr_;
			return *this;
		}

		//----------------------------------------------------------------------

		virtual void DeleteResource() noexcept {	//Deletes managed pointer
			if constexpr (std::is_array_v<T>)
				ptr_delete_scalar(pPtr_);
			else
				ptr_delete(pPtr_);
		}
		virtual void DeleteSelf() noexcept {		//Deletes [this]
			delete this;
		}

		void AddRef() noexcept { ++countRef_; }
		void AddRefWeak() noexcept { ++countWeak_; }
		bool RemoveRef() noexcept {
			if ((--countRef_) == 0) {
				DeleteResource();
				return false;
			}
			return true;
		}
		bool RemoveRefWeak() noexcept {
			if ((--countWeak_) == 0) {
				DeleteSelf();
				return false;
			}
			return true;
		}
	};

	template<class T> class ref_count_weak_ptr;

	//A non-atomic smart pointer
	template<class T>
	class ref_count_ptr {
		template<class U> friend class ref_count_ptr;
		friend ref_count_weak_ptr<T>;
	private:
		_ptr_ref_counter<T>* pInfo_ = nullptr;
		T* pPtr_ = nullptr;
	private:
		void _AddRef() noexcept {
			if (pInfo_ == nullptr) return;
			pInfo_->AddRef();
			pInfo_->AddRefWeak();
		}
		void _RemoveRef() noexcept {
			if (pInfo_ == nullptr) return;
			pInfo_->RemoveRef();
			pInfo_->RemoveRefWeak();
			pInfo_ = nullptr;
			pPtr_ = nullptr;
		}

		void _SetPointer(T* src) noexcept {
			_RemoveRef();
			if (src) {
				pInfo_ = new _ptr_ref_counter<T>(src);
				pPtr_ = src;
			}
			_AddRef();
		}
		template<class U> void _SetPointerFromInfo(_ptr_ref_counter<U>* info, T* src) noexcept {
			_RemoveRef();
			pInfo_ = (_ptr_ref_counter<T>*)info;
			pPtr_ = src;
			_AddRef();
		}
	public:
		ref_count_ptr() {
			_SetPointer(nullptr);
		}
		ref_count_ptr(T* src) {
			_SetPointer(src);
		}
		ref_count_ptr(const ref_count_ptr<T>& src) {
			_SetPointerFromInfo<T>(src.pInfo_, src.pPtr_);
		}
		template<class U> ref_count_ptr(ref_count_ptr<U>& src) {
			_SetPointerFromInfo<U>(src.pInfo_, (T*)src.pPtr_);
		}

		~ref_count_ptr() {
			_RemoveRef();
		}

		//----------------------------------------------------------------------

		ref_count_ptr<T>& operator=(T* src) {
			if (get() != src) _SetPointer(src);
			return *this;
		}
		ref_count_ptr<T>& operator=(const ref_count_ptr<T>& src) {
			if (get() != src.get()) {
				_SetPointerFromInfo<T>(src.pInfo_, src.pPtr_);
			}
			return *this;
		}
		template<class U> ref_count_ptr<T>& operator=(ref_count_ptr<U>& src) {
			if (get() != src.get()) {
				_SetPointerFromInfo<U>(src.pInfo_, (T*)src.pPtr_);
			}
			return (*this);
		}

		//----------------------------------------------------------------------

		inline uint32_t use_count() const { return pInfo_ ? (pInfo_->countRef_) : 0; }
		inline uint32_t weak_count() const { return pInfo_ ? (pInfo_->countWeak_) : 0; }
		inline T* get() const { return pInfo_ ? pInfo_->pPtr_ : nullptr; }

		T& operator*() const { return *get(); }
		T* operator->() const { return get(); }

		T& operator[](size_t n) const { return get()[n]; }

		inline bool unique() const { return use_count() == 1U; }

		//----------------------------------------------------------------------

		const bool operator==(nullptr_t) const { return get() == nullptr; }
		const bool operator==(const T* p) const { return get() == p; }
		const bool operator==(const ref_count_ptr<T>& p) const { return get() == p.get(); }
		template<class U> bool operator==(ref_count_ptr<U>& p) const { return get() == (T*)p.get(); }

		const bool operator!=(nullptr_t) const { return get() != nullptr; }
		const bool operator!=(const T* p) const { return get() != p; }
		const bool operator!=(const ref_count_ptr<T>& p) const { return get() != p.get(); }
		template<class U> bool operator!=(ref_count_ptr<U>& p) const { return get() != (T*)p.get(); }

		explicit operator bool() const {
			return pInfo_ ? (pInfo_->countRef_ > 0) : false;
		}

		//----------------------------------------------------------------------

		template<class U> static ref_count_ptr<T> Cast(ref_count_ptr<U>& src) {
			ref_count_ptr<T> res;
			if (T* castPtr = dynamic_cast<T*>(src.get())) {
				res._SetPointerFromInfo<U>(src.pInfo_, castPtr);
			}
			return res;
		}
	};

	//A non-atomic smart pointer (weak ref)
	template<class T>
	class ref_count_weak_ptr {
		template<class U> friend class ref_count_weak_ptr;
	private:
		_ptr_ref_counter<T>* pInfo_ = nullptr;
		T* pPtr_ = nullptr;
	private:
		void _AddRef() noexcept {
			if (pInfo_ == nullptr) return;
			pInfo_->AddRefWeak();
		}
		void _RemoveRef() noexcept {
			if (pInfo_ == nullptr) return;
			pInfo_->RemoveRefWeak();
			pInfo_ = nullptr;
			pPtr_ = nullptr;
		}

		template<class U> void _SetPointerFromInfo(_ptr_ref_counter<U>* info, T* src) noexcept {
			_RemoveRef();
			pInfo_ = (_ptr_ref_counter<T>*)info;
			pPtr_ = src;
			_AddRef();
		}
	public:
		ref_count_weak_ptr() {
			pInfo_ = nullptr;
		}
		ref_count_weak_ptr(const ref_count_weak_ptr<T>& src) {
			_SetPointerFromInfo<T>(src.pInfo_, src.pPtr_);
		}
		template<class U> ref_count_weak_ptr(ref_count_weak_ptr<U>& src) {
			_SetPointerFromInfo<U>(src.pInfo_, (T*)src.pPtr_);
		}
		template<class U> ref_count_weak_ptr(ref_count_ptr<U>& src) {
			_SetPointerFromInfo<U>(src.pInfo_, (T*)src.pPtr_);
		}

		~ref_count_weak_ptr() {
			_RemoveRef();
		}

		//----------------------------------------------------------------------

		ref_count_weak_ptr<T>& operator=(const ref_count_weak_ptr<T>& src) {
			if (get() != src.get()) {
				_SetPointerFromInfo<T>(src.pInfo_, src.pPtr_);
			}
			return *this;
		}
		ref_count_weak_ptr<T>& operator=(const ref_count_ptr<T>& src) {	//Create from ref_count_ptr
			if (get() != src.get()) {
				_SetPointerFromInfo<T>(src.pInfo_, src.pPtr_);
			}
			return *this;
		}
		template<class U> ref_count_weak_ptr& operator=(ref_count_weak_ptr<U>& src) {
			if (get() != src.get()) {
				_SetPointerFromInfo<U>(src.pInfo_, (T*)src.pPtr_);
			}
			return *this;
		}

		//----------------------------------------------------------------------

		inline bool IsExists() const {
			return pInfo_ ? (pInfo_->countRef_ > 0) : false;
		}
		inline bool expired() const { return !IsExists(); }

		inline uint32_t use_count() const { return IsExists() ? (pInfo_->countRef_) : 0; }
		inline T* get() const { return IsExists() ? pInfo_->pPtr_ : nullptr; }

		T& operator*() const { return *get(); }
		T* operator->() const { return get(); }
		T& operator[](size_t n) const { return get()[n]; }

		//----------------------------------------------------------------------

		const bool operator==(nullptr_t) const {
			return !IsExists();
		}
		const bool operator==(const T* p) const {
			return IsExists() ? get() == p : (p == nullptr);
		}
		const bool operator==(const ref_count_ptr<T>& p) const {
			return IsExists() ? get() == p.get() : (p.get() == nullptr);
		}
		template<class U> bool operator==(ref_count_ptr<U>& p) const {
			return IsExists() ? (get() == (T*)p.get()) : (p.get() == nullptr);
		}

		const bool operator!=(nullptr_t) const {
			return IsExists();
		}
		const bool operator!=(const T* p) const {
			return IsExists() ? get() != p : (p != nullptr);
		}
		const bool operator!=(const ref_count_ptr<T>& p) const {
			return IsExists() ? get() != p.get() : (p.get() != nullptr);
		}
		template<class U> bool operator!=(ref_count_ptr<U>& p) const {
			return IsExists() ? (get() != (T*)p.get()) : (p.get() != nullptr);
		}

		explicit operator bool() const {
			return IsExists() ? (pInfo_.pPtr_ != nullptr) : false;
		}

		//----------------------------------------------------------------------

		template<class U> static ref_count_weak_ptr<T> Cast(ref_count_weak_ptr<U>& src) {
			ref_count_weak_ptr<T> res;
			if (T* castPtr = dynamic_cast<T*>(src.get())) {
				res._SetPointerFromInfo<U>(src.pInfo_, castPtr);
			}
			return res;
		}
	};
}
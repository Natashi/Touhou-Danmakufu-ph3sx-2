#pragma once

//Update: I found a use for these.
//	I can customize them to use in place of shared_ptr's because of their non-atomicity option

#include "../pch.h"

namespace gstd {
	//Base for reference counting
	template<class T, bool ATOMIC>
	class _ptr_ref_counter {
	public:
		uint32_t countRef_ = 0;		//Strong refs
		uint32_t countWeak_ = 0;	//Weak refs
		T* pPtr_ = nullptr;			//Pointer
	public:
		_ptr_ref_counter(T* src) noexcept {
			pPtr_ = src;
		}
		template<class U, bool ATOMIC> _ptr_ref_counter(const _ptr_ref_counter<U, ATOMIC>& other) noexcept {
			countRef_ = other.countRef_;
			countWeak_ = other.countWeak_;
			pPtr_ = (T*)other.pPtr_;
		}

		_ptr_ref_counter& operator=(_ptr_ref_counter& other) = default;
		template<class U, bool ATOMIC> _ptr_ref_counter<T, ATOMIC>& operator=(_ptr_ref_counter<U, ATOMIC>& other) {
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

		void AddRef() noexcept {
			if constexpr (ATOMIC) _MT_INCR(countRef_);
			else ++countRef_;
		}
		void AddRefWeak() noexcept {
			if constexpr (ATOMIC) _MT_INCR(countWeak_);
			else ++countWeak_;
		}
		bool RemoveRef() noexcept {
			if constexpr (ATOMIC) _MT_DECR(countRef_);
			else --countRef_;

			if (countRef_ == 0) {
				DeleteResource();
				return false;
			}
			return true;
		}
		bool RemoveRefWeak() noexcept {
			if constexpr (ATOMIC) _MT_DECR(countWeak_);
			else --countWeak_;

			if (countWeak_ == 0) {
				DeleteSelf();
				return false;
			}
			return true;
		}
	};

	template<class T, bool ATOMIC> class ref_count_weak_ptr;

	//A non-atomic smart pointer
	template<class T, bool ATOMIC = true>
	class ref_count_ptr {
		template<class U, bool ATOMIC> friend class ref_count_ptr;
		friend ref_count_weak_ptr<T, ATOMIC>;
	public:
		using _MyCounter = _ptr_ref_counter<T, ATOMIC>;
		using _MyType = ref_count_ptr<T, ATOMIC>;
	private:
		_MyCounter* pInfo_ = nullptr;
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
				pInfo_ = new _MyCounter(src);
				pPtr_ = src;
			}
			_AddRef();
		}
		template<class U> void _SetPointerFromInfo(_ptr_ref_counter<U, ATOMIC>* info, T* src) noexcept {
			_RemoveRef();
			pInfo_ = (_MyCounter*)info;
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
		ref_count_ptr(const _MyType& src) {
			_SetPointerFromInfo<T>(src.pInfo_, src.pPtr_);
		}
		template<class U> ref_count_ptr(ref_count_ptr<U, ATOMIC>& src) {
			_SetPointerFromInfo<U>(src.pInfo_, (T*)src.pPtr_);
		}

		~ref_count_ptr() {
			_RemoveRef();
		}

		//----------------------------------------------------------------------

		_MyType& operator=(T* src) {
			if (get() != src) _SetPointer(src);
			return *this;
		}
		_MyType& operator=(const _MyType& src) {
			if (get() != src.get()) {
				_SetPointerFromInfo<T>(src.pInfo_, src.pPtr_);
			}
			return *this;
		}
		template<class U> _MyType& operator=(ref_count_ptr<U>& src) {
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
		inline bool atomic() const { return ATOMIC; }

		//----------------------------------------------------------------------

		const bool operator==(nullptr_t) const { return get() == nullptr; }
		const bool operator==(const T* p) const { return get() == p; }
		const bool operator==(const _MyType& p) const { return get() == p.get(); }
		template<class U, bool _ATO> bool operator==(ref_count_ptr<U, _ATO>& p) const { return get() == (T*)p.get(); }

		const bool operator!=(nullptr_t) const { return get() != nullptr; }
		const bool operator!=(const T* p) const { return get() != p; }
		const bool operator!=(const _MyType& p) const { return get() != p.get(); }
		template<class U, bool _ATO> bool operator!=(ref_count_ptr<U, _ATO>& p) const { return get() != (T*)p.get(); }

		explicit operator bool() const {
			return pInfo_ ? (pInfo_->countRef_ > 0) : false;
		}

		//----------------------------------------------------------------------

		template<class U> static _MyType Cast(ref_count_ptr<U, ATOMIC>& src) {
			_MyType res;
			if (T* castPtr = dynamic_cast<T*>(src.get())) {
				res._SetPointerFromInfo<U>(src.pInfo_, castPtr);
			}
			return res;
		}
	};

	//A non-atomic smart pointer (weak ref)
	template<class T, bool ATOMIC = true>
	class ref_count_weak_ptr {
		template<class U, bool ATOMIC> friend class ref_count_weak_ptr;
	public:
		using _MyCounter = _ptr_ref_counter<T, ATOMIC>;
		using _MyType = ref_count_weak_ptr<T, ATOMIC>;
	private:
		_MyCounter* pInfo_ = nullptr;
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

		template<class U> void _SetPointerFromInfo(_ptr_ref_counter<U, ATOMIC>* info, T* src) noexcept {
			_RemoveRef();
			pInfo_ = (_MyCounter*)info;
			pPtr_ = src;
			_AddRef();
		}
	public:
		ref_count_weak_ptr() {
			pInfo_ = nullptr;
		}
		ref_count_weak_ptr(const _MyType& src) {
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

		_MyType& operator=(const _MyType& src) {
			if (get() != src.get()) {
				_SetPointerFromInfo<T>(src.pInfo_, src.pPtr_);
			}
			return *this;
		}
		_MyType& operator=(const ref_count_ptr<T, ATOMIC>& src) {	//Create from ref_count_ptr
			if (get() != src.get()) {
				_SetPointerFromInfo<T>(src.pInfo_, src.pPtr_);
			}
			return *this;
		}
		template<class U> ref_count_weak_ptr& operator=(ref_count_weak_ptr<U, ATOMIC>& src) {
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

		inline bool atomic() const { return ATOMIC; }

		//----------------------------------------------------------------------

		const bool operator==(nullptr_t) const {
			return !IsExists();
		}
		const bool operator==(const T* p) const {
			return IsExists() ? get() == p : (p == nullptr);
		}
		const bool operator==(const _MyType& p) const {
			return IsExists() ? get() == p.get() : (p.get() == nullptr);
		}
		template<class U, bool _ATO> bool operator==(ref_count_ptr<U, _ATO>& p) const {
			return IsExists() ? (get() == (T*)p.get()) : (p.get() == nullptr);
		}

		const bool operator!=(nullptr_t) const {
			return IsExists();
		}
		const bool operator!=(const T* p) const {
			return IsExists() ? get() != p : (p != nullptr);
		}
		const bool operator!=(const _MyType& p) const {
			return IsExists() ? get() != p.get() : (p.get() != nullptr);
		}
		template<class U, bool _ATO> bool operator!=(ref_count_ptr<U, _ATO>& p) const {
			return IsExists() ? (get() != (T*)p.get()) : (p.get() != nullptr);
		}

		explicit operator bool() const {
			return IsExists() ? (pInfo_.pPtr_ != nullptr) : false;
		}

		//----------------------------------------------------------------------

		template<class U> static _MyType Cast(ref_count_weak_ptr<U, ATOMIC>& src) {
			_MyType res;
			if (T* castPtr = dynamic_cast<T*>(src.get())) {
				res._SetPointerFromInfo<U>(src.pInfo_, castPtr);
			}
			return res;
		}
	};
}
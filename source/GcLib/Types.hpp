#pragma once

// Pointer utilities
template<typename T> static constexpr inline void ptr_delete(T*& ptr) {
	delete ptr;
	ptr = nullptr;
}
template<typename T> static constexpr inline void ptr_delete_scalar(T*& ptr) {
	delete[] ptr;
	ptr = nullptr;
}
template<typename T> static constexpr inline void ptr_release(T*& ptr) {
	if (ptr)
		ptr->Release();
	ptr = nullptr;
}

//------------------------------------------------------------------------------

using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
#define uptr std::unique_ptr
#define sptr std::shared_ptr
#define wptr std::weak_ptr

#define scast(_ty, _p) static_cast<_ty>(_p)
#define rcast(_ty, _p) reinterpret_cast<_ty>(_p)
#define dcast(_ty, _p) dynamic_cast<_ty>(_p)
#define dptr_cast(_ty, _p) std::dynamic_pointer_cast<_ty>(_p)

using std::make_unique;
using std::make_shared;

//------------------------------------------------------------------------------

using std::optional;

//------------------------------------------------------------------------------

template<class T>
using optional_ref = std::optional<std::reference_wrapper<T>>;

template <typename T>
using unique_ptr_fd = std::unique_ptr<T, std::function<void(T*)>>;

//------------------------------------------------------------------------------

#define API_DEFINE_GET(_type, _name, _var) _type Get##_name() { return _var; }
#define API_DEFINE_SET(_type, _name, _var) void Set##_name(_type v) { _var = v; }
#define API_DEFINE_GETSET(_type, _name, _target) \
			API_DEFINE_GET(_type, _name, _target) \
			API_DEFINE_SET(_type, _name, _target)

#define LOCK_WEAK(_v, _p) if (auto _v = (_p).lock())

#define MOVE(x) std::move(x)

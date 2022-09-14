#pragma once

#include <type_traits>

#include "errors.hpp"
#include "any.hpp"

namespace archimedes {
struct reflected_record_type;
struct any;

// a "baked" cast function which casts from one type to another
// see archimedes::make_case_fn
using cast_fn = std::function<void*(void*)>;

namespace detail {
// get byte difference between a cast from a ptr_t to type "to"
result<std::ptrdiff_t, cast_error> cast_diff_impl(
    const any &ptr_t,
    const reflected_record_type &from,
    const reflected_record_type &to,
    bool only_up = false);

// make a function which, when called with a pointer of type ptr_t "from" will
// cast it to "to"
result<cast_fn, cast_error> make_cast_fn_impl(
    const any &ptr_t,
    const reflected_record_type &from,
    const reflected_record_type &to);

// see cast()
result<void*, cast_error> cast_impl(
    const any &ptr_t,
    const reflected_record_type &from,
    const reflected_record_type &to);

// TODO: RTTI optional
// TODO: doc
result<any, cast_error> dyncast_impl(
    const any &ptr_t,
    const std::type_info &ti,
    const reflected_record_type &to);

} // namespace detail

// get byte difference between a cast from a ptr_t to type "to"
inline result<std::ptrdiff_t, cast_error> cast_diff(
    const any &ptr_t,
    const reflected_record_type &from,
    const reflected_record_type &to) {
    return detail::cast_diff_impl(ptr_t, from, to);
}

// cast some pointer T of record type "from" to "to", dynamically, statically,
// or however the inheritance tree needs to be traversed
// NOTE: will break if multiple instances of the same (non-virtual) base are
// found in the inheritance tree
template <
    typename T,
    typename V =
        std::conditional_t<
            std::is_const_v<std::remove_pointer_t<T>>,
            const void *,
            void*>>
    requires std::is_pointer_v<T>
result<V, cast_error> cast(
    T ptr_t,
    const reflected_record_type &from,
    const reflected_record_type &to) {
    const auto res = detail::cast_impl(any::make(ptr_t), from, to);
    if (res) {
        return const_cast<V>(*res);
    }
    return res.unwrap_error();
}

inline result<any, cast_error> cast(
    const any &ptr,
    const reflected_record_type &from,
    const reflected_record_type &to) {
    const auto res = detail::cast_impl(ptr, from, to);
    return
        result<any, cast_error>::make(
            res,
            [&]() {
                return any::make(*res);
            },
            [&]() {
                return res.unwrap_error();
            });
}

// make a function which will cast Ts (of type "from") to some type "to"
template <typename T>
    requires std::is_pointer_v<T>
result<cast_fn, cast_error> make_cast_fn(
    T ptr_t,
    const reflected_record_type &from,
    const reflected_record_type &to) {
    return detail::make_cast_fn_impl(any::make(ptr_t), from, to);
}

// make a function which will cast ptrs (of type "from") to some type "to"
inline result<cast_fn, cast_error> make_cast_fn(
    const any &ptr,
    const reflected_record_type &from,
    const reflected_record_type &to) {
    return detail::make_cast_fn_impl(ptr, from, to);
}
} // end namespace archimedes

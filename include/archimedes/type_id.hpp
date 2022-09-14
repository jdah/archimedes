#pragma once

#include <string_view>
#include <cstdint>
#include <type_traits>

#include "errors.hpp"

#if defined(__clang__) || defined(__GNUC__)
    #define ARCHIMEDES_PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
    #define ARCHIMEDES_PRETTY_FUNCTION __FUNCSIG__
#else
    #error "unsupported compiler (pretty function)"
#endif

namespace archimedes {
namespace detail {
struct type_info;

// type_name adapted from:
// https://bitwizeshift.github.io/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/
template <std::size_t ...Is>
constexpr auto substring_as_array(
    std::string_view s,
    std::index_sequence<Is...>) {
    return std::array { s[Is]... };
}

template <typename T>
constexpr auto type_name_array() {
#if defined(__clang__)
    constexpr auto prefix   = std::string_view{"[T = "};
    constexpr auto suffix   = std::string_view{"]"};
#elif defined(__GNUC__)
    constexpr auto prefix   = std::string_view{"with T = "};
    constexpr auto suffix   = std::string_view{"]"};
#elif defined(_MSC_VER)
    constexpr auto prefix   = std::string_view{"type_name_array<"};
    constexpr auto suffix   = std::string_view{">(void)"};
#else
    #error "unsupported compiler (type_name_array)"
#endif

    constexpr auto function = std::string_view { ARCHIMEDES_PRETTY_FUNCTION };
    constexpr auto start = function.find(prefix) + prefix.size();
    constexpr auto end = function.rfind(suffix);

    static_assert(start < end);

    constexpr auto name = function.substr(start, (end - start));
    return substring_as_array(name, std::make_index_sequence<name.size()>{});
}

template <typename T>
struct type_name_holder {
    static inline constexpr auto value = type_name_array<T>();
};

constexpr uint64_t fnv1a_partial(uint64_t partial, std::string_view s) {
    if (s.length() == 0) {
        return 0;
    }

    partial = (partial ^ s[0]) * 1099511628211u;
    return s.length() == 1 ? partial : fnv1a_partial(partial, s.substr(1));
}

// constexpr FNV1a hash
constexpr uint64_t fnv1a(std::string_view s) {
    return fnv1a_partial(14695981039346656037u, s);
}

constexpr uint64_t fnv1a_append(uint64_t partial, std::string_view s) {
    return fnv1a_partial(partial, s);
}
} // namespace detail

// get non-mangled type name for T
template <typename T>
constexpr auto type_name() {
    constexpr auto &value = detail::type_name_holder<T>::value;
    return std::string_view { value.data(), value.size() };
}

// represents a unique type id
struct type_id {
    using internal = uint64_t;

    static constexpr auto NO_ID =
        std::numeric_limits<internal>::max();

    // type id from type
    template <typename T>
    static constexpr inline type_id from() {
        return type_id(detail::fnv1a(type_name<T>()));
    }

    // type id from name
    static constexpr inline type_id from(std::string_view name) {
        return type_id(detail::fnv1a(name));
    }

    template <internal ID>
    static constexpr inline type_id from() {
        return type_id(ID);
    }

    static constexpr inline type_id from(internal id) {
        return type_id(id);
    }

    static constexpr inline type_id none() {
        return type_id(NO_ID);
    }

    // must be public (sadly) for type to be "structural" for use in templates
    internal _id_internal = NO_ID;

    explicit constexpr type_id(internal i)
        : _id_internal(i) {}

    constexpr type_id()
        : _id_internal(NO_ID) {}

    constexpr internal value() const {
        return _id_internal;
    }

    // differing implementations according to runtime! plugin uses a lookup
    // though global context, runtime uses lookup in global type table
    const detail::type_info &operator*() const;

    const detail::type_info *operator->() const {
        return &**this;
    }

    constexpr bool operator==(const type_id &rhs) const {
        return this->_id_internal == rhs._id_internal;
    }

    constexpr auto operator<=>(const type_id &rhs) const {
        return this->_id_internal <=> rhs._id_internal;
    }

    operator bool() const;

    // abuse our hash function to append a pointer to the type id
    constexpr type_id add_pointer() const {
        return type_id(detail::fnv1a_append(this->_id_internal, " *"));
    }
};
} // namespace archimedes

// hash implementation
namespace std {
template<>
struct hash<archimedes::type_id> {
    size_t operator() (const archimedes::type_id &t) const {
        return static_cast<size_t>(t.value());
    }
};
} // namespace std

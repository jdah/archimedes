#pragma once

// exceptions can be enabled with (this replaces assert()-s)
// #define ARCHIMEDES_USE_EXCEPTIONS

#include <cassert>
#include <variant>
#include <type_traits>
#include <optional>

#ifdef ARCHIMEDES_USE_EXCEPTIONS
#include <string>
#include <stdexcept>

namespace archimedes {
struct exception : public std::runtime_error {
    explicit exception(std::string_view message)
        : std::runtime_error(std::string(message)) {}
};
}
#endif

// end user can define assert function is they so choose
#ifndef ARCHIMEDES_ASSERT
#define ARCHIMEDES_ASSERT assert
#endif

// ARCHIMEDES_FAIL to fail directly
#ifdef ARCHIMEDES_USE_EXCEPTIONS
#define ARCHIMEDES_FAIL(_m) throw new archimedes::exception((_m))
#else
#define ARCHIMEDES_FAIL(_m) ARCHIMEDES_ASSERT(false && (_m))
#endif

namespace archimedes {
// static field constexpr error codes
enum class constexpr_error {
    IS_NOT_CONSTEXPR
};

// template parameter error codes
enum class template_parameter_error {
     NO_VALUE
};

// field get error codes
enum class field_error {
    IS_NOT_BIT_FIELD,
    IS_BIT_FIELD,
    COULD_NOT_CAST,
    WRONG_TYPE
};

// cast error codes
enum class cast_error {
    NOT_FOUND,
    COULD_NOT_REFLECT
};

// "any" error codes
enum class any_error {
    COULD_NOT_REFLECT,
    NO_DEFAULT_CTOR,
    INVALID_TYPE
};

// basic result type
template <typename T, typename E>
struct result : public std::variant<T, E> {
    using base = std::variant<T, E>;
    using base::base;

    static constexpr auto
        INDEX_ERROR = 1,
        INDEX_VALUE = 0;

    // returns true if not error type
    operator bool() const { return this->index() == INDEX_VALUE; }

    // implicit conversion to optional
    operator std::optional<T>() const {
        return this->is_value() ? std::make_optional(**this) : std::nullopt;
    }

    const T &operator*() const {
        if (this->is_error()) {
            ARCHIMEDES_FAIL("attempt to unwrap an error result");
        }
        return std::get<INDEX_VALUE>(*this);
    }

    T &operator*() {
        if (this->is_error()) {
            ARCHIMEDES_FAIL("attempt to unwrap an error result");
        }
        return std::get<INDEX_VALUE>(*this);
    }

    const T *operator->() const
        requires (!std::is_pointer_v<T>) {
        return &**this;
    }

    const std::remove_pointer_t<T> *operator->() const
        requires (std::is_pointer_v<T>) {
        return **this;
    }

    T *operator->()
        requires (!std::is_pointer_v<T>) {
        return &**this;
    }

    std::remove_pointer_t<T> *operator->()
        requires (std::is_pointer_v<T>) {
        return **this;
    }

    bool is_error() const { return this->index() == INDEX_ERROR; }
    bool is_value() const { return this->index() == INDEX_VALUE; }

    const T &unwrap() const { return **this; }
    T &unwrap() { return **this; }

    const E &unwrap_error() const {
        if (this->is_value()) {
#ifdef ARCHIMEDES_USE_EXCEPTIONS
            throw new exception("attempt to unwrap a result as an error");
#else
            ARCHIMEDES_ASSERT(false&&"attempt to unwrap a result as an error");
#endif
        }

        return std::get<INDEX_ERROR>(*this);
    }

    // make from function if true/error code if false
    template <typename F>
        requires std::is_convertible_v<std::invoke_result_t<F>, T>
    static result<T, E> make(bool e, F if_true, E &&if_false) {
        return e ?
            result<T, E>(T(if_true()))
            : result<T, E>(E(std::forward<E>(if_false)));
    }

    // make from function if true/function if false
    template <typename F, typename G>
        requires
            (std::is_convertible_v<std::invoke_result_t<F>, T>
                && std::is_convertible_v<std::invoke_result_t<G>, E>)
    static result<T, E> make(bool e, F if_true, G if_false) {
        return e ? result<T, E>(T(if_true())) : result<T, E>(E(if_false()));
    }

    // make from value if true/value if false
    template <typename U, typename V>
        requires
            (std::is_convertible_v<U, T>
                && std::is_convertible_v<V,E>)
    static result<T, E> make(bool e, U &&if_true, V &&if_false) {
        return e ?
            result<T, E>(T(U(std::forward<U>(if_true))))
            : result<T, E>(E(V(std::forward<V>(if_false))));
    }
};
}

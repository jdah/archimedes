#pragma once

#include <type_traits>

template <typename T>
struct Foo {
    int bar() requires std::is_floating_point_v<T> {
        return 12;
    }

    int baz() requires std::is_integral_v<T> {
        return 14;
    }

    template <typename U = T>
    auto boo() -> typename std::enable_if<std::is_same_v<U, int>, int>::type {
        return 1334;
    }

    int qux() { return 123; }
};

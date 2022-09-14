#pragma once

#include <type_traits>

struct Bar {
    float x;

    Bar() = default;
    Bar(float x, float y, float z)
        : x(x + y + z) {}
};

struct Qux {
    virtual ~Qux() = default;

    template <typename T>
    Qux(T &&t) : x(t) {}

    int x;
};

template <typename T>
struct Foo;

template <typename T>
    requires std::is_class_v<T>
struct Foo<T> {
    int x;

    Foo() = default;

    Foo &operator=(const Foo &foo) {
        this->x = 13;
    }

    Foo(const Foo &foo) {
        this->x = 12;
    }
};

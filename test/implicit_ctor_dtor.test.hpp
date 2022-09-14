#pragma once

#include <utility>

struct Foo {
    union {
        int z;
    } u;

    int x;
};

struct Bar {
    Bar() = default;

    // according to the standard, this *does not count* as a move constructor
    // unless it is instantiatd for Bar, therefore templated constructors MUST
    // be excluded from implicit function generation
    template <typename T>
    explicit Bar(T &&t) {
        ints[0] = *t.begin();
    }

    int ints[8];
};

struct WithAReference {
    Bar &b;
};

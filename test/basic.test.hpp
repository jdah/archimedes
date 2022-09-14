#pragma once

#include <iostream>
#include <string>

namespace baz {
struct Foo {
    static constexpr int baza = 12;
    int bazb;
    mutable int foob;

    std::string bar(const std::string &s) const noexcept(false) {
        std::string t = s + "asfsf";
        return t;
    }
};
}

template <typename T>
struct Goob {
    int y;
};

struct Glob : public baz::Foo, protected Goob<Glob> {
    int Goob<Glob>::*ptr;
    int z;
    unsigned int u : 5;
    unsigned int w : 4;

    Glob()
        : ptr(&Goob<Glob>::y) {}
};

using MyInt = uint8_t;

enum Eeeee : MyInt {
    A = 0,
    B = 12,
    C = B,
    Z = (A + 1)
};

int free_func(Eeeee e);

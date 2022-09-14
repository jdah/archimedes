#pragma once

#include <string>
#include "test.hpp"

inline int i_have_zero_with_defaults(int x = 2, int y = 4) {
    return x + y;
}

inline int i_have_defaults(int x, int y = 2) {
    return x + y;
}

inline std::string i_have_strings(int x, std::string s = "x is: ") {
    return s + std::to_string(x);
}

struct Foo {
    int x = 2;

    explicit Foo(int y = 2) {
        this->x += y;
    }

    int bar(int x = 12, int y = 33) {
        return y - x;
    }
};

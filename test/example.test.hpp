#pragma once

#include <string>
#include <vector>

struct Foo {
    enum Qux : uint_fast32_t { QUX_A, QUX_B };

    using string_type = std::string;

    int f;

    Foo() {
        this->f = 4545;
    }

    Foo(float flt) {
        this->f = static_cast<int>(flt * 1284.124f);
    }

    virtual ~Foo() = default;

    string_type speak() { return "foo!"; }

    virtual string_type virtual_speak() { return "virtual foo!"; }
};

template <typename T>
struct Baz : Foo {
    int x;
    std::string s;
    std::vector<T> ts;

    string_type virtual_speak() override { return "virtual baz!"; }
};

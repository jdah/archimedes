#pragma once

#include <optional>

namespace ns {
    enum Enum {
        A,
        B
    };
}

template <typename T>
struct Foo {
    T foo(const std::optional<T> &o) { return *o; }
};

template <typename T>
struct Bar {
    T t;
};

namespace q {
    using namespace std;
    struct Baz : Foo<Bar<tuple<int, ns::Enum, tuple<basic_string<char>>>>> {
        void fn(int my_param) { }
    };
}

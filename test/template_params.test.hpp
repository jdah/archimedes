#pragma once

template <typename T>
struct Foo {
    T t;
};

template <>
struct Foo<int> { int something_special; };

template <typename T, int X>
struct Bar {
    T t;
};

struct Baz {
    int x, y;
};

struct Qux : Bar<int, 55> {};

template <int X>
struct Bar<double, X> {
    int special() { return 12; }
};

struct Foob : Bar<double, 4> {};

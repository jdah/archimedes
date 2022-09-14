#pragma once

struct Foo {
    explicit Foo(int x) : x(x) {}

    Foo() = default;
    Foo(const Foo&) = delete;
    Foo(Foo&&) = default;
    Foo &operator=(const Foo&) = delete;
    Foo &operator=(Foo&&) = default;

    int x;
};

struct Bar {
    Bar(int x = 12) : x(x) {}
    Bar(const Bar&) = default;
    Bar(Bar&&) = delete;
    Bar &operator=(const Bar&) = default;
    Bar &operator=(Bar&&) = delete;

    int x;
};

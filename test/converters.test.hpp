#pragma once

struct Foo {
    int x;

    explicit operator const Foo*() const { return this; }
};

struct Bar {
    operator int() const { return 12; }
    operator int() { return 13; }
    operator double() & { return 1.0; }
    operator double() && { return 2.0; }
    operator double() const& { return 3.0; }
    operator double() const&& { return 4.0; }
    explicit operator Foo() { return Foo { .x = 101 }; }
};

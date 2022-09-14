#pragma once

struct Bar;

struct Foo {
    int x, y;
    int Foo::*foo_member;
    float Bar::*bar_member;
    Foo *Bar::*foo_ptr;
};

struct Bar {
    float f;
    Foo *ptr0, *ptr1;
};

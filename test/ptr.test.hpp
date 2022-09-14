#pragma once

struct Foo;

struct Bar {
    const Foo *foo;
    Bar *bar;
};

struct Foo {
    const Bar *bar;
    Foo *foo;
};

#pragma once

struct Bar {
    int x;
    int y;
};

struct Baz {
    int z;
    float w;
};

union Foo {
    Bar bar;
    Baz baz;
};

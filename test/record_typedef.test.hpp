#pragma once

struct Foo;

struct Bar {
    using FooPtr1 = const Foo*;
    typedef const Foo* FooPtr1A;
    using FooPtr2 = Foo*;
    typedef Foo* FooPtr2A;
    using BarRef = Bar&;
};

struct Foo {
    struct InFoo {
        using Z = Foo;
    };

    using X = int;
    using Y = InFoo;
};

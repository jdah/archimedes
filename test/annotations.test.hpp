#pragma once

struct __attribute__((annotate("abc"))) __attribute__((annotate("xyz"))) Foo {
    static int __attribute__((annotate("bcd"))) y;
    int __attribute__((annotate("aaaa"))) x;

    [[clang::annotate("bbb")]] int z;

    __attribute__((annotate("efg"))) int bar(int x) { return x; }
};

template <typename T>
struct __attribute__((annotate("bar_annotation"))) Bar {
    int x;
};

using Baz = Bar<float>;

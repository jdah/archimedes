#pragma once

struct Foo {
    int x;
};

struct Bar {
    int y;
};

template <typename T>
struct Baz {
    T t;
};

struct Qux {
    int y;
};

struct Quxical {
    int z;
};

namespace ns {
    struct Buxtastic { int z; };
    struct Quxtastic { int z; };
};

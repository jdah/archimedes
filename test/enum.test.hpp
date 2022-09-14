#pragma once

enum Foo {
    A = 1,
    B = 2,
    C = A + 1,
    D = C + 12,
    E = D,
    Z = -1
};

enum class Bar : unsigned short {
    FIRST = 0,
    SECOND = 1,
    THIRD,
    FOURTH
};

namespace names {
    enum class Baz {
        AAAA = 3
    };
}

struct S {
    enum SFoo {
        A = 333,
        B = 345
    };
};

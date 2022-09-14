#pragma once

namespace a {
    struct IntWrapper { int x; };
    struct IntPairWrapper { int x, y; };
    static constexpr int aa = 2;
    namespace b {
        static constexpr int bb = 2;

        namespace c {
            enum E {
                VALUE_0 = 0,
                VALUE_1 = 12
            };

            static constexpr IntWrapper w = IntWrapper { .x = 22 };
        }

        struct IUseMyNamespaces {
            static constexpr IntWrapper w =
                IntWrapper { .x = aa + bb + c::w.x };
            static constexpr IntPairWrapper p =
                IntPairWrapper { .x = aa + bb, .y = aa - bb };
        };
    }
}

struct Foo {
    static constexpr int x = 12;
    static constexpr const char *s = "some string";

    struct Nested {
        static constexpr int y = 39;
    };
};

template <typename T>
struct Baz {
    static constexpr T field = 333;
    int fff = 3;
};

template <a::b::c::E OPTION>
struct Boo {
    static constexpr a::b::c::E E_VALUE = OPTION;
};

using Boo0 = Boo<a::b::c::VALUE_0>;


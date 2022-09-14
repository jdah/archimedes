#pragma once

namespace ns {
    enum MyEnum {
        MyEnum0 = 0,
        MyEnum1 = 1
    };

    template <typename T, int N, MyEnum E>
    struct MyType {
        union {
            T ts[N];

            struct {
                T x, y;
            };
        };

        constexpr MyType(T x, T y) : x(x), y(y) {}
    };
}

namespace ns {
    using MyTypeInt2 = MyType<int, 2, MyEnum1>;
    using MyTypeInt2X = MyType<int, 2, (MyEnum) 0U>;
}

struct AnotherType {
    static constexpr auto FIELD = ns::MyTypeInt2 { 13, 12 };
    static constexpr ns::MyTypeInt2X FIELD2 =
        { 12, 12 };
};

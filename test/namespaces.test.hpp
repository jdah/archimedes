#pragma once

template <typename T>
struct Bar { T field; };

template <typename T>
int func(T x) {
    return x + 12;
}

// nothing in anonymous namespaces can be reflected
namespace {
    using Foo = Bar<unsigned int>;

    struct Baz : Foo {
        int y;
    };
}

using Foob = Bar<unsigned int>;

namespace a {
    struct InA { };

    namespace b {
        struct InB { };

        namespace c {
            struct InC : public InB { };

            namespace d {
                struct InD : public InC { };
            }

            using InDInC = d::InD;

            enum EnumInC {
                ENUM_A,
                ENUM_B
            };
        }

        using InAInB = InA;

        namespace d = c;
        using namespace d::d;
    }

    template <b::c::EnumInC E>
    struct IHaveEnumParam {
        static constexpr b::c::EnumInC E_VALUE = E;
    };

    using IHaveEnumParamA = IHaveEnumParam<b::c::ENUM_A>;

    namespace {
        struct AnonymousInA {};

        inline int func_in_anon_namespace(int bar) {
            return bar + 2;
        }
    }

    inline int i_use_func_in_anon_namespace() {
        return func_in_anon_namespace(2);
    }

    namespace bc = b::c;
    namespace cb = b::c;
}

namespace abc = a::b::c;
namespace ab = a::b;

namespace ns {
    struct Foo {
        int x;
        Foo() : x(12) {}
    };
}

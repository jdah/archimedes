#pragma once

#include <archimedes.hpp>

template <typename T>
struct Foo {
    T t;
};

#define DECL_ME(_T)                                                \
    template <>                                                    \
    struct [[ARCHIMEDES_ANNOTATE("_decl_me_" ARCHIMEDES_STRINGIFY(_T))]] Foo<_T> {  \
        int baz() { return archimedes::type_name<_T>().length(); } \
    };

DECL_ME(int)
DECL_ME(float)
DECL_ME(double)
DECL_ME(bool)

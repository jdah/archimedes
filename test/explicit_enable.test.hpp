#pragma once

#include <archimedes.hpp>

struct [[ARCHIMEDES_REFLECT]] Foo {
    int some_function() { return 44; }
};

struct Bar {
    int x;
};

inline int free_function(int x, int y) { return x * y; }

[[ARCHIMEDES_REFLECT]] inline int free_function2(int x, int y) { return x * y; };

namespace [[ARCHIMEDES_REFLECT]] ns {
    struct YouCanSeeMe { int i_am_a_function() { return 12; } };
}

namespace ns2 {
    struct YouCantSeeMe {};

    struct [[ARCHIMEDES_REFLECT]] YouShouldBeAbleToSeeMe {
        int i_am_a_function2() { return 12; }
    };
}

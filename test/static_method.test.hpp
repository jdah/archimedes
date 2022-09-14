#pragma once

struct Foo {
    struct Qux {
        int z;

        static int qux_me(int z) {
            return z + 3;
        }
    };

    int bar(int x) {
        return x + 1;
    }

    int baz(int z) {
        return z + 2;
    }

    static int baz(int z, int w) {
        return z + w;
    }
};

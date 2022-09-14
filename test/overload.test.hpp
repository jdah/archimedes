#pragma once

struct Foo {
    int bar(int x) { return x; }
    int bar(int x, int y) { return x + y; }
    int bar(int x, int y, int z) { return x + y + z; }
};

inline int baz(int x) { return x; }
inline int baz(int x, int y) { return x + y; }
inline int baz(int x, int y, int z) { return x + y + z; }

namespace ns {
    inline int baz(int x) { return -x; }
}

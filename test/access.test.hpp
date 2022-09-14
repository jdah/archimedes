#pragma once

struct Foo {
private:
    struct Bar {
        int am_i_visible(int x, int y) { return x + y; }
    };
    int y;
public:
    int x;
protected:
    int z;
};

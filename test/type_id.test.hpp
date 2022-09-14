#pragma once

#include <cstdint>

struct Y {
    uint64_t y_x = 7;
    virtual ~Y() {}
};

struct W : virtual public Y {
    uint64_t w_x = 6;
};

struct Z : public W {
    uint64_t z_x = 5;
    Z(): W() {}
};

template <typename T>
struct A {
    T t;
};

struct D0 {
    int x;
    virtual ~D0() = default;
};

struct D1 : D0 {
    int y;
};

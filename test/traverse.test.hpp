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

struct A {
    uint64_t a_x = 1;
    virtual ~A() {}
};

struct B : virtual public A, virtual public Z {
    uint64_t b_y = 2;
};

struct C : virtual public A, virtual public Z {
    uint64_t c_y = 3;
};

struct D : public B, public C {
    uint64_t d_z = 4;

    D(): A(), Z(), B(), C() {}
};

struct X0 {
    int x;
};

struct X1 : public X0 {
    int y;
};

struct X2 : public X1 {
    int z;
};

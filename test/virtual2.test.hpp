#pragma once

#include <cstdint>

struct A {
    uint64_t x = 1;
    virtual ~A() = default;
};

struct B : virtual public A {
    uint64_t y = 2;
};

struct C : virtual public A {
    uint64_t y = 3;
};

struct D : public B, public C {
    uint64_t z = 4;

    D(): A(), B(), C() {}
};

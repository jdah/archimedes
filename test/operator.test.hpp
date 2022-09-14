#pragma once

struct IHaveOperators {
    int x;

    IHaveOperators operator+(const IHaveOperators &rhs) const {
        auto t = *this;
        t.x += rhs.x;
        return t;
    }

    IHaveOperators operator<<(bool x) const {
        auto t = *this;
        t.x *= x ? 1 : -1;
        return t;
    }
};

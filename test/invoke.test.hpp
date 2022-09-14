#pragma once

#include <utility>
#include <string>
#include "test.hpp"

struct ICountMyMoves {
    int count = 0;

    ICountMyMoves() = default;
    ICountMyMoves(const ICountMyMoves&) = default;
    ICountMyMoves(ICountMyMoves &&other) { *this = std::move(other); }
    ICountMyMoves &operator=(const ICountMyMoves&) = delete;
    ICountMyMoves &operator=(ICountMyMoves &&other) {
        this->count = other.count + 1;
        return *this;
    }
};

struct Foo {
    int w = 20;

    int inc_w(int x) { this->w++; return this->w; }
    int member_c(int x) const { return this->w + 2; }
    int member_cr(int x) const& { return this->w + 4; }
    int member_rr(int x) && { return this->w + 6; }

    static int some_static_method(const int &x) {
        return x + 2;
    }

    static int another_static_method(ICountMyMoves &&x) {
        ICountMyMoves i = std::move(x);
        return i.count;
    }
};

std::string &i_return_my_argument(std::string &s);

std::string &i_return_something_new(std::string &s);

int lots_of_args(int x, int y, int z, const std::string &p, std::string &&q);

inline int i_am_inline(int x) { return x * x; }

inline std::string my_arg_is_void(void) { return "xyz"; }

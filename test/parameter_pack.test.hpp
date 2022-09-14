#pragma once

#include <utility>

template <typename ...Ts>
int i_add_my_args(Ts&& ...ts) {
    int x = 0;
    ([&x](auto &&t){
        x += t;
    }(std::forward<Ts>(ts)), ...);
    return x;
}

template <typename ...Ts>
int i_dont_collide(int ts_, int ts_pp_0, int ts_pp______2, Ts&& ...ts) {
    int x;
    ([&x](auto &&t){
        x += t;
    }(std::forward<Ts>(ts)), ...);
    return x;
}

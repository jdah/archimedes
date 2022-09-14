#pragma once

struct Foo {
    static constexpr auto LENGTH = 222;

    int ws[LENGTH + 2];
    int xs[2];
    int ys[LENGTH];
    int zs[];
};

inline int i_sum_my_array(int arr[8]) {
    int x = 0;
    for (int i = 0; i < 8; i++) {
        x += arr[i];
    }
    return x;
}

inline int i_sum_my_const_array(int const arr[8]) {
    int x = 0;
    for (int i = 0; i < 8; i++) {
        x += arr[i];
    }
    return x;
}

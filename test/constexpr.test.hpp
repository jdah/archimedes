#pragma once

#include "test.hpp"


struct Foo {
    static constexpr int INTS[] = { 1, 2, 3 };
    static constexpr int INT = 3;

    static constexpr auto lambda_result =
        []() {
            return INT + INTS[2];
        }();
};

// TODO: NTTP support
/* template <typename T, arc::type_id ID = arc::type_id::none()> */
/* struct Bar { */
/*     using MyType = T; */

/*     static constexpr auto L = */
/*         []() { */
/*             return ID == arc::type_id::none() ? arc::type_id::from<T>() : ID; */
/*         }(); */
/* }; */


constexpr inline int i_add_my_args(int x, int y) {
    return x + y;
}

#pragma once

#include <cstdint>

struct Foo {
    struct {
        int x, y;
    } some_fields;

    struct {
        union {
            struct {
                uint8_t l, h;
            } h_l;
            uint16_t hl;
        } u;
        int z;
        const Foo *f = nullptr;
    } x;
};

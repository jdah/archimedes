#pragma once

#include <memory>

struct Foo {
    std::shared_ptr<bool[]> bs;

    Foo(std::shared_ptr<bool[]> bs)
        : bs(bs) {}
};

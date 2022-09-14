#pragma once

struct Bar;
int fwd(int,int);

struct Foo {
    const Bar *bar = nullptr;
};

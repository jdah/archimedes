#pragma once

struct Foo {
    int bar() const& { return 2; }
    int bar() const&& { return 3; }
    int bar() & { return 4; }
    int bar() && { return 5; }
    int baz() const { return 6; }
    int baz() { return 7; }
};

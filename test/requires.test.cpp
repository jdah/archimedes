#include "test.hpp"
#include "requires.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();

    // TODO

    const auto f_f = archimedes::reflect<Foo<float>>();
    ASSERT(f_f->function("bar"));
    ASSERT(!f_f->function("baz"));
    // TODO: ASSERT(!f_f->function("boo"));
    ASSERT(f_f->function("qux"));

    const auto f_i = archimedes::reflect<Foo<int>>();
    ASSERT(!f_i->function("bar"));
    ASSERT(f_i->function("baz"));
    // TODO: ASSERT(f_i->function("boo"));
    ASSERT(f_i->function("qux"));

    Foo<float>().bar();
    Foo<int>().baz();
    Foo<int>().boo();
    Foo<float>().qux();
    return 0;
}

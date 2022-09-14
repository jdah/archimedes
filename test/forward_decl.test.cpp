#include "test.hpp"
#include "forward_decl.test.hpp"
#include "forward_decl2.hpp"

FORCE_TYPE_INSTANTIATION(Foo)

int main(int argc, char *argv[]) {
    archimedes::load();

    Foo f;
    Bar b;
    f.bar = &b;
    const auto foo = archimedes::reflect<Foo>();
    ASSERT(foo);
    const auto bar = archimedes::reflect<Bar>();
    ASSERT(bar);
    ASSERT(bar->kind() == archimedes::UNKNOWN);
    const auto fun = archimedes::reflect_function("fwd");
    ASSERT(!fun);
    return 0;
}

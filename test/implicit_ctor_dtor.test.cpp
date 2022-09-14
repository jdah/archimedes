#include "test.hpp"
#include "implicit_ctor_dtor.test.hpp"

FORCE_TYPE_INSTANTIATION(Foo)
FORCE_TYPE_INSTANTIATION(Bar)

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto f = *archimedes::reflect<Foo>();
    ASSERT(f.constructors());
    ASSERT(f.default_constructor());
    ASSERT(f.copy_constructor());
    ASSERT(f.move_constructor());
    ASSERT(f.copy_assignment());
    ASSERT(f.move_assignment());
    ASSERT(f.destructor());

    Bar b;
    WithAReference x = { b };
    x.b.ints[0] = 3;

    // TODO: test bar
    return 0;
}

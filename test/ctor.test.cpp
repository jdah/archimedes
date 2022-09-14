#include "test.hpp"
#include "ctor.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto foo = archimedes::reflect<Foo>()->as_record();
    const auto foo_default = foo.default_constructor();
    ASSERT(foo_default);
    ASSERT(foo_default->is_defaulted());

    // should be able to find if defined and deleted but should not be able to
    // invoke
    const auto foo_copy =
        foo.constructors()->get<void(const Foo &)>();
    ASSERT(foo_copy);
    ASSERT(foo_copy->is_deleted());
    ASSERT(!foo_copy->can_invoke());

    const auto foo_move =
        foo.constructors()->get<void(Foo &&)>();
    ASSERT(foo_move);

    const auto foo_move_assign =
        foo.function<Foo&(Foo&&)>();
    ASSERT(foo_move_assign);

    const auto bar = archimedes::reflect<Bar>()->as_record();
    const auto bar_default = bar.default_constructor();
    ASSERT(bar_default);
    ASSERT(!bar_default->is_defaulted());

    Bar b;
    bar_default->invoke(&b);
    ASSERT(b.x == 12);
    bar_default->invoke(&b, 33);
    ASSERT(b.x == 33);
    return 0;
}

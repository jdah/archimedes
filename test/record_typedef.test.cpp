#include "test.hpp"
#include "record_typedef.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto
        foo = archimedes::reflect<Foo>(),
        in_foo = archimedes::reflect<Foo::InFoo>(),
        bar = archimedes::reflect<Bar>();

    ASSERT(foo->type_aliases().size() == 2);
    ASSERT(in_foo->type_aliases().size() == 1);
    ASSERT(bar->type_aliases().size() == 5);

    ASSERT(
        bar->type_alias("FooPtr1")->type().type()
            == archimedes::reflect<const Foo*>());
    ASSERT(
        bar->type_alias("FooPtr1A")->type().type()
            == archimedes::reflect<const Foo*>());
    ASSERT(
        bar->type_alias("FooPtr2")->type().type()
            == archimedes::reflect<Foo*>());
    ASSERT(
        bar->type_alias("FooPtr2A")->type().type()
            == archimedes::reflect<Foo*>());
    ASSERT(
        bar->type_alias("BarRef")->type().type()
            == archimedes::reflect<Bar&>());
    ASSERT(
        in_foo->type_alias("Z")->type().type()
            == archimedes::reflect<Foo>());
    ASSERT(
        foo->type_alias("X")->type().type()
            == archimedes::reflect<int>());
    ASSERT(
        foo->type_alias("Y")->type().type()
            == archimedes::reflect<Foo::InFoo>());

    return 0;
}

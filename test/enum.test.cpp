#include "test.hpp"
#include "enum.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto f = archimedes::reflect<Foo>();
    ASSERT(f);
    ASSERT(f->is_enum());
    ASSERT(f->as_enum().value_of<int>("A"));
    ASSERT(f->as_enum().names_of(Foo::D).size() == 2);

    const auto bar = archimedes::reflect("Bar");
    ASSERT(bar);
    ASSERT(bar->is_enum());
    ASSERT(bar->as_enum().values().size() == 4);
    ASSERT(bar->as_enum().value_of<int>("FIRST") == 0);
    ASSERT(bar->as_enum().value_of<int>("THIRD") == 2);
    ASSERT(
        bar->as_enum().base_type().type() ==
           archimedes::reflect("unsigned short"));

    const auto baz = archimedes::reflect("names::Baz");
    ASSERT(baz);
    ASSERT(baz->is_enum());
    ASSERT(baz->as_enum().values().size() == 1);
    ASSERT(baz->as_enum().value_of<int>("AAAA") == 3);

    const auto s_foo = archimedes::reflect("S::SFoo");
    ASSERT(s_foo);
    ASSERT(s_foo->is_enum());
    ASSERT(s_foo->as_enum().values().size() == 2);
    ASSERT(s_foo->as_enum().value_of<int>("A") == 333);
    ASSERT(s_foo->as_enum().value_of<int>("B") == 345);
    return 0;
}

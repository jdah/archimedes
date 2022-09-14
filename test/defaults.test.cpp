#include "test.hpp"
#include "defaults.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto ihzwd =
        *archimedes::reflect_functions("i_have_zero_with_defaults").begin()->first();
    ASSERT(ihzwd.invoke()->as<int>() == 6);
    ASSERT(ihzwd.invoke(4)->as<int>() == 8);
    ASSERT(ihzwd.invoke(12, 12)->as<int>() == 24);

    const auto ihd =
        *archimedes::reflect_functions("i_have_defaults").begin()->first();
    ASSERT(!ihd.invoke().is_success());
    ASSERT(ihd.invoke(4)->as<int>() == 6);
    ASSERT(ihd.invoke(12, 12)->as<int>() == 24);

    const auto ihs =
        *archimedes::reflect_functions("i_have_strings").begin()->first();
    ASSERT(!ihs.invoke().is_success());
    ASSERT(ihs.invoke(44)->as<std::string>() == "x is: 44");
    ASSERT(ihs.invoke(45, std::string("bbbbb"))->as<std::string>() == "bbbbb45");

    ASSERT(archimedes::reflect("Foo"));
    const auto f = archimedes::reflect("Foo")->as_record();
    ASSERT(f.function("Foo"));
    const auto ctor = *f.function<void(int)>();

    Foo foo;
    ASSERT(!ctor.invoke().is_success());

    ctor.invoke(&foo);
    ASSERT(foo.x == 4);

    ctor.invoke(&foo, 12);
    ASSERT(foo.x == 14);

    const auto bar = *f.function("bar");
    ASSERT(bar.invoke(&foo)->as<int>() == 21);
    ASSERT(bar.invoke(&foo, 1)->as<int>() == 32);
    ASSERT(bar.invoke(&foo, 1, 2)->as<int>() == 1);

    return 0;
}

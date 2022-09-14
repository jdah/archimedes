#include "test.hpp"
#include "overload.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto bars =
        archimedes::reflect<Foo>()->as_record().function_set("bar");
    ASSERT(bars);
    ASSERT(bars->size() == 3);

    Foo f;
    ASSERT(
        bars->get<int(int)>()->invoke(&f, 1)->as<int>() == 1);
    ASSERT(
        bars->get<int(int, int)>()->invoke(&f, 1, 2)->as<int>() == 3);
    ASSERT(
        bars->get<int(int, int, int)>()->invoke(&f, 1, 2, 3)->as<int>() == 6);

    const auto bazs = archimedes::reflect_functions("baz");
    ASSERT(bazs.begin()->size() == 3);
    ASSERT(
        bazs.begin()->get<int(int)>()->invoke(1)->as<int>() == 1);
    ASSERT(
        bazs.begin()->get<int(int, int)>()->invoke(1, 2)->as<int>() == 3);
    ASSERT(
        bazs.begin()->get<int(int, int, int)>()->invoke(1, 2, 3)->as<int>() == 6);

    const auto ns_baz = archimedes::reflect_functions("ns::baz");
    ASSERT(ns_baz.size() == 1);
    ASSERT(ns_baz.begin()->get<int(int)>()->invoke(100)->as<int>() == -100);

    return 0;
}

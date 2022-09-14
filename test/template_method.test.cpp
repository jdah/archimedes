#include "test.hpp"
#include "template_method.test.hpp"

struct Bar {

};

using BooDouble = Boo<double>;
FORCE_TYPE_INSTANTIATION(BooDouble);

int main(int argc, char *argv[]) {
    archimedes::load();
    Foo f;
    ASSERT(f.bar(static_cast<const int &>(12)) == 13);
    ASSERT(f.bar(std::move(12)) == 2);
    f.baz<Bar, Foo>();
    const auto fs =
        *archimedes::reflect<Foo>()->function_set("bar");
    ASSERT(fs.size() == 2);

    const auto bar_templated_opt = fs.get<int(const int &) const>();
    ASSERT(bar_templated_opt);
    const auto bar_templated = *bar_templated_opt;

    const auto bar_normal_opt = fs.get<int(int &&)const>();
    ASSERT(bar_normal_opt);
    const auto bar_normal = *bar_normal_opt;

    ASSERT(bar_templated != bar_normal);

    ASSERT(bar_templated.is_member());
    ASSERT(bar_templated.parameter("t"));
    ASSERT(
        bar_templated.parameter("t")->type().type()
            == *archimedes::reflect<const int &>());

    ASSERT(bar_normal.is_member());
    ASSERT(bar_normal.parameter("i"));
    ASSERT(
        bar_normal.parameter("i")->type().type()
            == *archimedes::reflect<int &&>());

    const auto bd = archimedes::reflect<Boo<double>>();
    ASSERT(bd);

    Quxical().qux<IntHolder>(4, 55);
    Quxical().qux<Quxical>(4);

    return 0;
}

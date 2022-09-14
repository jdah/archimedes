#include "union.test.hpp"
#include "test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto bar = *archimedes::reflect<Bar>();
    const auto baz = *archimedes::reflect<Baz>();
    const auto foo = *archimedes::reflect<Foo>();
    ASSERT(foo.is_record());
    const auto r = foo.as_record();
    const auto fields = r.fields();
    ASSERT(fields.size() == 2);
    const auto
        f_bar = *r.field("bar"),
        f_baz = *r.field("baz");
    ASSERT(f_bar.offset() == f_baz.offset());

    const auto
        f_bar_0 = *f_bar.type().type().as_record().field("x"),
        f_baz_0 = *f_baz.type().type().as_record().field("z");

    ASSERT(f_bar_0 == *bar.as_record().field("x"));
    ASSERT(f_baz_0 == *baz.as_record().field("z"));

    Foo f;
    f_bar_0.set(f.bar, 12);
    ASSERT(*f_bar_0.get<int>(f.bar) == 12);
    ASSERT(*f_baz_0.get<int>(f.baz) == 12);
    f_baz_0.set(f.baz, 66666);
    ASSERT(*f_bar_0.get<int>(f.bar) == 66666);

    const auto
        f_bar_1 = *f_bar.type().type().as_record().field("y"),
        f_baz_1 = *f_baz.type().type().as_record().field("w");
    f_baz_1.set(f.baz, 123.0f);

    union U {
        float f;
        int i;
    };
    ASSERT(*f_bar_1.get<int>(f.bar) == (U { .f = 123.0f }).i);

    return 0;
}

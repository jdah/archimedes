#include "test.hpp"
#include "static_method.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();

    const auto f = archimedes::reflect<Foo>();
    ASSERT(f);

    const auto bars = f->as_record().function_set("bar");
    ASSERT(bars);
    ASSERT(bars->size() == 1);

    const auto bazs = f->as_record().function_set("baz");
    ASSERT(bazs);
    ASSERT(bazs->size() == 2);

    const auto f_m = bazs->get<int(int)>();
    const auto f_s = bazs->get<int(int,int)>();

    ASSERT(f_m);
    ASSERT(f_s);
    ASSERT(*f_m != *f_s);

    ASSERT(!f_m->is_static() && f_m->is_member());
    ASSERT(f_s->is_static() && f_s->is_member());

    Foo foo;
    ASSERT(f_m->invoke(&foo, 2)->as<int>() == 4);
    ASSERT(f_s->invoke(2, 3)->as<int>() == 5);

    const auto q = archimedes::reflect<Foo::Qux>();
    const auto f_q = q->as_record().function("qux_me");
    ASSERT(f_q->invoke(44)->as<int>() == 47);

    return 0;
}

#include "test.hpp"
#include "anonymous.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto f = archimedes::reflect<Foo>()->as_record();
    const auto some_fields = f.field("some_fields")->type()->as_record();
    ASSERT(some_fields.field("x"));
    ASSERT(some_fields.field("y"));

    Foo foo;
    foo.some_fields.x = 12;
    foo.some_fields.y = 345;
    ASSERT(*some_fields.field("x")->get<int>(foo.some_fields) == 12);
    ASSERT(*some_fields.field("y")->get<int>(foo.some_fields) == 345);
    some_fields.field("x")->set(foo.some_fields, 444);
    ASSERT(foo.some_fields.x == 444);

    const auto r_x = f.field("x")->type()->as_record();
    const auto r_u = r_x.field("u")->type()->as_record();
    const auto r_u_h_l = r_u.field("h_l")->type()->as_record();
    ASSERT(r_u.is_union());

    foo.x.u.hl = 0xAA55;
    ASSERT(*r_u_h_l.field("l")->get<uint8_t>(foo.x.u) == 0x55);
    ASSERT(*r_u_h_l.field("h")->get<uint8_t>(foo.x.u) == 0xAA);

    r_x.field("f")->set(foo.x, &foo);
    ASSERT(foo.x.f == &foo);
    return 0;
}

#include "test.hpp"
#include "member_ptr.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto f = archimedes::reflect<Foo>()->as_record();
    Foo foo;
    Bar bar;

    ASSERT(
        *f.field("foo_member")->type() ==
            archimedes::reflect<int Foo::*>());
    ASSERT(
        *f.field("bar_member")->type() ==
            archimedes::reflect<float Bar::*>());
    ASSERT(
        *f.field("foo_ptr")->type() ==
            archimedes::reflect<Foo *Bar::*>());


    f.field("foo_member")->set(foo, &Foo::x);
    foo.*f.field("foo_member")->get<int Foo::*>(foo)->get() = 12;
    ASSERT(foo.x == 12);

    f.field("foo_member")->set(foo, &Foo::y);
    foo.*f.field("foo_member")->get<int Foo::*>(foo)->get() = 20;
    ASSERT(foo.y == 20);

    f.field("bar_member")->set(foo, &Bar::f);
    bar.*f.field("bar_member")->get<float Bar::*>(foo)->get() = 0.125f;
    ASSERT(bar.f == 0.125f);

    bar.ptr0 = nullptr;
    bar.ptr1 = &foo;

    f.field("foo_ptr")->set(foo, &Bar::ptr0);
    ASSERT(
        bar.*f.field("foo_ptr")->get<Foo *Bar::*>(foo)->get()
            == nullptr);
    f.field("foo_ptr")->set(foo, &Bar::ptr1);
    ASSERT(
        bar.*f.field("foo_ptr")->get<Foo *Bar::*>(foo)->get()
            == &foo);

    return 0;
}

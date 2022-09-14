#include "test.hpp"
#include "converters.test.hpp"

FORCE_TYPE_INSTANTIATION(Foo)
FORCE_TYPE_INSTANTIATION(Bar)

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto foo = archimedes::reflect<Foo>()->as_record();
    const auto bar = archimedes::reflect<Bar>()->as_record();
    Foo f;
    Bar b;

    const auto foo_to_const_foo_ptr =
        foo.function<const Foo*()const>();
    ASSERT(foo_to_const_foo_ptr);
    ASSERT(foo_to_const_foo_ptr->is_explicit());
    ASSERT(foo_to_const_foo_ptr->is_converter());
    ASSERT(foo_to_const_foo_ptr->is_member());
    ASSERT(foo_to_const_foo_ptr->type().is_const());
    ASSERT(foo_to_const_foo_ptr->invoke(&f)->as<const Foo*>() == &f);

    const auto bar_to_int_const = bar.function<int() const>();
    ASSERT(bar_to_int_const);
    ASSERT(bar_to_int_const->type().is_const());
    ASSERT(bar_to_int_const->invoke(&b)->as<int>() == 12);

    const auto bar_to_int = bar.function<int()>();
    ASSERT(bar_to_int);
    ASSERT(!bar_to_int->type().is_const());
    ASSERT(bar_to_int->invoke(&b)->as<int>() == 13);

    const auto bar_to_double_l = bar.function<double()&>();
    ASSERT(bar_to_double_l);
    ASSERT(bar_to_double_l->type().is_ref());
    ASSERT(bar_to_double_l->invoke(archimedes::ref(b))->as<double>() == 1.0);

    const auto bar_to_double_cl = bar.function<double()const&>();
    ASSERT(bar_to_double_cl);
    ASSERT(bar_to_double_cl->type().is_ref());
    ASSERT(bar_to_double_cl->type().is_const());
    ASSERT(bar_to_double_cl->invoke(archimedes::cref(b))->as<double>() == 3.0);

    const auto bar_to_double_r = bar.function<double()&&>();
    ASSERT(bar_to_double_r);
    ASSERT(bar_to_double_r->type().is_rref());
    ASSERT(bar_to_double_r->invoke(Bar())->as<double>() == 2.0);

    const auto bar_to_double_cr = bar.function<double()const&&>();
    ASSERT(bar_to_double_cr);
    ASSERT(bar_to_double_cr->type().is_rref());
    ASSERT(bar_to_double_cr->type().is_const());
    ASSERT(bar_to_double_cr->invoke(Bar())->as<double>() == 4.0);

    const auto bar_to_foo = bar.function<Foo()>();
    ASSERT(bar_to_foo);
    ASSERT(!bar_to_foo->type().is_const());
    ASSERT(!bar_to_foo->type().is_ref());
    ASSERT(!bar_to_foo->type().is_rref());
    ASSERT(bar_to_foo->invoke(&b)->as<Foo>().x == 101);

    return 0;
}

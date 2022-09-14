#include "test.hpp"
#include "namespaces.test.hpp"

FORCE_TYPE_INSTANTIATION(Bar<unsigned short>)
FORCE_TYPE_INSTANTIATION(Foo)
FORCE_TYPE_INSTANTIATION(a::InA)
FORCE_TYPE_INSTANTIATION(a::b::InB)
FORCE_TYPE_INSTANTIATION(a::b::c::InC)
FORCE_FUNCTION_INSTANTIATION(a::i_use_func_in_anon_namespace)
FORCE_TYPE_INSTANTIATION(a::IHaveEnumParamA)
FORCE_TYPE_INSTANTIATION(ns::Foo)

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto r_foo = archimedes::reflect<Foo>();
    // should be able to reflect because even though it's in an anonymous
    // namespace, the using decl gets resolved at compile time
    ASSERT(r_foo);

    // should *not* be able to reflect baz because it's not an alias and has its
    // definition in an anonymous namespace
    const auto r_baz = archimedes::reflect<Baz>();
    ASSERT(!r_baz);
    ASSERT(r_foo == archimedes::reflect("Bar<unsigned int>"));
    const auto r_foob_str = archimedes::reflect("Foob");
    ASSERT(r_foob_str);

    const auto abc_inc = archimedes::reflect("abc::InC");
    ASSERT(archimedes::reflect<abc::InC>());
    ASSERT(abc_inc);
    ASSERT(abc_inc == archimedes::reflect<abc::InC>());

    const auto a_bc_inc = archimedes::reflect("a::bc::InC");
    ASSERT(a_bc_inc);
    ASSERT(a_bc_inc == abc_inc);

    const auto ab_d_inc = archimedes::reflect("ab::d::InC");
    ASSERT(ab_d_inc);
    ASSERT(ab_d_inc == abc_inc);

    ASSERT(
        abc_inc->as_record().bases()[0].type() ==
            archimedes::reflect("ab::InB"));

    const auto ab_ind = archimedes::reflect("a::b::InD");
    ASSERT(ab_ind);

    const auto abc_indinc = archimedes::reflect("abc::InDInC");
    ASSERT(abc_indinc);
    ASSERT(abc_indinc == archimedes::reflect<a::b::c::d::InD>());

    const auto ns_foo = archimedes::reflect<ns::Foo>();
    ASSERT(ns_foo);
    ASSERT(ns_foo->function_set("Foo"));
    ASSERT(ns_foo->constructors());

    return 0;
}

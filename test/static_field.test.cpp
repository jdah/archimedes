#include "test.hpp"
#include "static_field.test.hpp"

FORCE_TYPE_INSTANTIATION(Baz<unsigned short>)
FORCE_TYPE_INSTANTIATION(Boo0)

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto f = archimedes::reflect<Foo>();
    ASSERT(f);
    const auto x = f->as_record().static_field("x");
    ASSERT(x);
    ASSERT(x->is_constexpr());
    ASSERT(x->constexpr_value()->as<int>() == 12);
    const auto s = f->as_record().static_field("s");
    ASSERT(s->constexpr_value()->as<const char*>() == Foo::s);

    const auto n = archimedes::reflect<Foo::Nested>();
    const auto y = n->as_record().static_field("y");
    ASSERT(y);
    ASSERT(y->constexpr_value()->as<int>() == 39);

    const auto b = archimedes::reflect<Baz<unsigned short>>();
    ASSERT(b);

    // TODO: for some reason template constexpr VarDecls do not have init
    // expressions :(
    /* const auto i = b->as_record().static_field("field"); */
    /* ASSERT(i); */
    /* ASSERT(i->type().type() == archimedes::reflect<unsigned short>()); */
    /* ASSERT(i->constexpr_value()->as<unsigned short>() == 333); */

    const auto iumn = archimedes::reflect("a::b::IUseMyNamespaces");
    ASSERT(iumn);
    ASSERT(
        iumn->as_record().static_field("w")
        ->constexpr_value()
        ->as<int>() == 26);
    ASSERT(
        iumn->as_record().static_field("p")
            ->constexpr_value()
            ->as<a::IntPairWrapper>().x == 4);
    ASSERT(
        archimedes::reflect("Boo0")->as_record()
            .static_field("E_VALUE")
            ->constexpr_value()
            ->as<a::b::c::E>()
            == a::b::c::VALUE_0);

    return 0;
}

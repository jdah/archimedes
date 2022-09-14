#include "test.hpp"
#include "explicit_enable.test.hpp"
#include "explicit_enable_other.hpp"

ARCHIMEDES_ARG("explicit-enable")
ARCHIMEDES_ARG("enable")

ARCHIMEDES_REFLECT_TYPE_REGEX("InAnotherHeader.*")

static __attribute__((unused)) const auto my_var = InAnotherHeader<int>();

int main(int argc, char *argv[]) {
    archimedes::load();

    /* ASSERT(archimedes::reflect<InAnotherHeader<int>>()); */
    /* TODO: ASSERT( */
    /*     archimedes::reflect<InAnotherHeader<int>>() */
    /*        ->function("some_function")); */

    const auto foo = archimedes::reflect<Foo>();
    ASSERT(foo);
    const auto sf = foo->function("some_function");
    ASSERT(sf);
    ASSERT(sf->can_invoke());

    Foo f;
    ASSERT(sf->invoke(&f)->as<int>() == 44);

    ASSERT(!archimedes::reflect_function("free_function"));
    ASSERT(archimedes::reflect_function("free_function2"));
    ASSERT(archimedes::reflect("ns::YouCanSeeMe"));
    ASSERT(!archimedes::reflect("ns2::YouCantSeeMe"));
    ASSERT(archimedes::reflect<ns::YouCanSeeMe>()->function("i_am_a_function"));

    ASSERT(archimedes::reflect("ns2::YouShouldBeAbleToSeeMe"));
    ASSERT(
        archimedes::reflect<ns2::YouShouldBeAbleToSeeMe>()
            ->function("i_am_a_function2"));
    return 0;
}

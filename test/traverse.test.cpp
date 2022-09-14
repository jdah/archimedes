#include "test.hpp"
#include "traverse.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    // check that traversal order is correct
    bool got_z_w = false, got_z_y;
    archimedes::reflect<Z>()->traverse_bases(
        [&](const archimedes::reflected_base &base,
           const archimedes::any&) {
            const auto name = base.type().qualified_name();
            if (name == "W") {
                ASSERT(!got_z_y && !got_z_w);
                got_z_w = true;
            } else if (name == "Y") {
                ASSERT(got_z_w);
                got_z_y = true;
            } else {
                ASSERT(false, "unknown base {}", name);
            }
        });

    D d;
    archimedes::reflect<D>()->traverse_bases(
       [&](const archimedes::reflected_base &base,
           const archimedes::any &any_ptr) {
            if (base.type().qualified_name() == "B") {
                ASSERT(any_ptr.as<void*>() == dynamic_cast<B*>(&d));
            } else if (base.type().qualified_name() == "C") {
                ASSERT(any_ptr.as<void*>() == dynamic_cast<C*>(&d));
            } else if (base.type().qualified_name() == "A") {
                ASSERT(any_ptr.as<void*>() == dynamic_cast<A*>(&d));
            }else if (base.type().qualified_name() == "Z") {
                ASSERT(any_ptr.as<void*>() == dynamic_cast<Z*>(&d));
            }else if (base.type().qualified_name() == "W") {
                ASSERT(any_ptr.as<void*>() == dynamic_cast<W*>(&d));
            }else if (base.type().qualified_name() == "Y") {
                ASSERT(any_ptr.as<void*>() == dynamic_cast<Y*>(&d));
            }
       }, &d);

    // check that upcasting works
    ASSERT(
        *archimedes::cast(
            &d,
            *archimedes::reflect<D>(),
            *archimedes::reflect<A>())
            == dynamic_cast<A*>(&d));
    ASSERT(
        *archimedes::cast(
            &d,
            *archimedes::reflect<D>(),
            *archimedes::reflect<Z>())
            == dynamic_cast<Z*>(&d));
    ASSERT(
        *archimedes::cast(
            dynamic_cast<C*>(&d),
            *archimedes::reflect<C>(),
            *archimedes::reflect<Z>())
            == dynamic_cast<Z*>(&d));

    // check that downcasting works
    ASSERT(
        *archimedes::cast(
            dynamic_cast<Z*>(&d),
            *archimedes::reflect<Z>(),
            *archimedes::reflect<C>())
            == dynamic_cast<C*>(&d));

    ASSERT(
        *archimedes::cast(
            dynamic_cast<Z*>(&d),
            *archimedes::reflect<Z>(),
            *archimedes::reflect<D>())
            == &d);

    // check static casts
    X2 x2;
    ASSERT(
        *archimedes::cast(
            static_cast<X0*>(&x2),
            *archimedes::reflect<X0>(),
            *archimedes::reflect<X2>())
            == static_cast<X0*>(&x2));

    ASSERT(
        *archimedes::cast(
            &x2,
            *archimedes::reflect<X2>(),
            *archimedes::reflect<X1>())
            == static_cast<X1*>(&x2));

    ASSERT(
        *archimedes::cast(
            static_cast<X1*>(&x2),
            *archimedes::reflect<X1>(),
            *archimedes::reflect<X0>())
            == static_cast<X0*>(&x2));

    // check that bogus casts are rejected
    ASSERT(
        !archimedes::cast(
            dynamic_cast<A*>(&d),
            *archimedes::reflect<A>(),
            *archimedes::reflect<Z>()));

    // check that we can make cast functions and use them multiple times
    const auto fn =
        archimedes::make_cast_fn(
            &d,
            *archimedes::reflect<D>(),
            *archimedes::reflect<Z>());
    ASSERT(fn);

    D d2;
    ASSERT((*fn)(&d) == dynamic_cast<Z*>(&d));
    ASSERT((*fn)(&d2) == dynamic_cast<Z*>(&d2));
    ASSERT((*fn)(&d) != dynamic_cast<Z*>(&d2));

    ASSERT(
        !archimedes::make_cast_fn(
            dynamic_cast<A*>(&d),
            *archimedes::reflect<A>(),
            *archimedes::reflect<Z>()));

    return 0;
}

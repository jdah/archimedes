#include "test.hpp"
#include "example.test.hpp"

namespace arc = archimedes;

Baz<int> g_baz;

int main(int argc, char *argv[]) {
    arc::load(); // must be run on program startup to load type information

    // we could also use arc::reflect("Foo") and arc::reflect("Baz<int>")
    const auto
        foo = arc::reflect<Foo>(),
        baz_int = arc::reflect<Baz<int>>();

    // like this! but then we need to manually "cast" it to a reflected enum
    const auto qux = arc::reflect("Qux")->as_enum();

    // TODO

    return 0;
}

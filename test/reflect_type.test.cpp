#include <archimedes.hpp>

#include "test.hpp"
#include "reflect_type.test.hpp"

ARCHIMEDES_ARG("explicit-enable")
ARCHIMEDES_ENABLE()

ARCHIMEDES_REFLECT_TYPE(Bar)

ARCHIMEDES_FORCE_TYPE_INSTANTIATION(Baz<int>)
ARCHIMEDES_REFLECT_TYPE(Baz<int>)

ARCHIMEDES_REFLECT_TYPE_REGEX("Qux.*")
ARCHIMEDES_REFLECT_TYPE_REGEX("ns::Qux.*")

int main(int argc, char *argv[]) {
    Baz<int> b;
    b.t = 3485;

    archimedes::load();
    ASSERT(!archimedes::reflect<Foo>());
    ASSERT(archimedes::reflect<Bar>());
    ASSERT(archimedes::reflect<Qux>());
    ASSERT(archimedes::reflect<Quxical>());
    ASSERT(archimedes::reflect<Baz<int>>());
    ASSERT(!archimedes::reflect<ns::Buxtastic>());
    ASSERT(archimedes::reflect<ns::Quxtastic>());
    return 0;
}

#include "test.hpp"
#include "stdlib.test.hpp"

namespace arc = archimedes;

// TODO: test is broken
// type name printers can't print nested stdlib types in parameter packs??

ARCHIMEDES_ARG("include-ns-std")
ARCHIMEDES_ARG("include-path-regex-.*/string")
ARCHIMEDES_REFLECT_TYPE_REGEX("std::string.*");
ARCHIMEDES_REFLECT_TYPE_REGEX("std::basic_string.*");
// TODO: broken ARCHIMEDES_ARG("include-path-regex-.*/iosfwd")

using MyMap = std::unordered_map<int, int>;

int main(int argc, char *argv[]) {
    arc::load();
    ASSERT((arc::reflect<std::unordered_map<int, int>>()));
    ASSERT(
        arc::type_id::from<std::string>()
           == arc::type_id::from<std::basic_string<char>>());
    /* ASSERT(arc::reflect<std::basic_string<char>>()); */
    /* ASSERT(arc::reflect<std::string>()); */
    /* ASSERT( */
    /*     arc::reflect<std::string>()->id() */
    /*         == arc::type_id::from<std::string>().value()); */
    /* ASSERT( */
    /*     arc::reflect<std::basic_string<char>>()->id() */
    /*         == arc::type_id::from<std::basic_string<char>>().value()); */
    // TODO: broken ASSERT(arc::reflect<std::string>());
    return 0;
}

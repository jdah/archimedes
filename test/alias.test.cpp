#include "test.hpp"
#include "alias.test.hpp"

namespace arc = archimedes;

int main(int argc, char *argv[]) {
    arc::load();
    ASSERT(arc::reflect("char") == arc::reflect<char>());
    ASSERT(arc::reflect("MyChar") == arc::reflect<char>());
    ASSERT(arc::reflect("MyChar") == arc::reflect<MyChar2>());
    ASSERT(arc::reflect("MyChar2") == arc::reflect<char>());
    ASSERT(arc::reflect("MyChar2") == arc::reflect<MyChar>());
    ASSERT(arc::reflect("std::string2") == arc::reflect<std::basic_string<char>>());

    return 0;
}

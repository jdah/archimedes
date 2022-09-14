#include "test.hpp"
#include "access.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto f = archimedes::reflect<Foo>();
    ASSERT(f);

    const auto b = archimedes::reflect("Foo::Bar");
    ASSERT(b);

    // should be able to see function but not invoke it
    const auto amv = b->as_record().function("am_i_visible");
    ASSERT(amv);
    ASSERT(amv->parameters().size() == 2);
    ASSERT(!amv->can_invoke());

    // should be able to see x, y, and z
    ASSERT(f->as_record().field("x")->access() == archimedes::PUBLIC);
    ASSERT(f->as_record().field("z")->access() == archimedes::PROTECTED);
    ASSERT(f->as_record().field("y")->access() == archimedes::PRIVATE);
    return 0;
}

#include "test.hpp"
#include "ptr.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();

    const auto foo = archimedes::reflect<Foo>();
    const auto bar = archimedes::reflect<Bar>();
    ASSERT(foo->field("foo")->type() != bar->field("foo")->type());
    ASSERT(foo->field("foo")->type().type() != bar->field("foo")->type().type());

    return 0;
}

#include "test.hpp"
#include "no_reflect.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    Foo f;
    Bar b;
    b.w = f.x;
    const auto foo = archimedes::reflect<Foo>();
    ASSERT(!foo);
    const auto bar = archimedes::reflect<Bar>();
    ASSERT(bar);
    ASSERT(bar->as_record().field("z"));
    ASSERT(bar->as_record().field("w"));
    return 0;
}

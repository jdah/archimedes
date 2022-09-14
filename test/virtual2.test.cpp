#include "test.hpp"
#include "virtual2.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto d = *archimedes::reflect<D>();

    // d is dynamic (no functions but has virtual base)
    ASSERT(d.is_dynamic());

    // 2 bases: B, C
    ASSERT(d.bases().size() == 2);

    // 1 vbase: A
    ASSERT(d.vbases().size() == 1);

    // try to dynamic cast from D to its base C
    D my_d;
    ASSERT(
        d.base("C")->cast_up(&my_d) == dynamic_cast<C*>(&my_d));
    return 0;
}

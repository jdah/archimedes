#include "test.hpp"
#include "operator.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto iho = archimedes::reflect("IHaveOperators");
    ASSERT(iho);

    const auto p = iho->as_record().function("operator\\+");
    ASSERT(p);

    const auto bsl = iho->as_record().function("operator<<");
    ASSERT(bsl);

    IHaveOperators a { .x = 1 }, b { .x = 2 };
    ASSERT(p->invoke(&a, archimedes::cref(b))->as<IHaveOperators>().x == 3);

    ASSERT(bsl->invoke(&a, false)->as<IHaveOperators>().x == -1);
    ASSERT(bsl->invoke(&a, true)->as<IHaveOperators>().x == 1);

    return 0;
}

#include "test.hpp"
#include "types.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    ASSERT(archimedes::reflect<AnotherType>());
    const auto at = archimedes::reflect<AnotherType>()->as_record();
    ASSERT(
        at.static_field("FIELD")
            ->constexpr_value()
            ->as<ns::MyTypeInt2>().x == 13);
    ASSERT(
        at.static_field("FIELD2")
            ->constexpr_value()
            ->as<ns::MyTypeInt2X>().x == 12);
    return 0;
}

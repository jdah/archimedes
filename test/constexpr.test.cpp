#include "constexpr.test.hpp"

namespace arc = archimedes;

FORCE_TYPE_INSTANTIATION(Foo)

// TODO: nttp support
// FORCE_TYPE_INSTANTIATION(Bar<float, arc::type_id::from<int>()>)

int main(int argc, char *argv[]) {
    arc::load();

    const auto iama = arc::reflect_functions("i_add_my_args");
    ASSERT(
        iama.begin()->first()
            == (*arc::reflect_functions<int(int,int)>())[0]);
    ASSERT(iama.begin()->first()->invoke(11, 12)->as<int>() == 23);
    const auto foo = arc::reflect<Foo>();
    ASSERT(
        foo->as_record().static_field("INTS")
        ->constexpr_value()
        ->as<int*>()[2] == 3);
    ASSERT(
        foo->static_field("INT")
        ->constexpr_value()
        ->as<int>() == 3);
    return 0;
}

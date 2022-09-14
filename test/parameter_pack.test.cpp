#include "test.hpp"
#include "parameter_pack.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    i_add_my_args(1, 2, 3, 4, 5);
    i_add_my_args(1, 2, 3);
    const auto iama = archimedes::reflect_functions("i_add_my_args");
    ASSERT(iama.begin()->size() == 2);

    const auto iama3 =
        *std::find_if(
            iama.begin()->begin(),
            iama.begin()->end(),
            [](const auto &f) {
                return f.parameters().size() == 3;
            });

    const auto iama5 =
        *std::find_if(
            iama.begin()->begin(),
            iama.begin()->end(),
            [](const auto &f) {
                return f.parameters().size() == 5;
            });

    ASSERT(iama3.invoke(4, 5, 20)->as<int>() == 29);
    ASSERT(iama5.invoke(10, 10, 33, 1, 2)->as<int>() == 56);

    i_dont_collide(1, 2, 3, 4, 5, 6);
    const auto idc = archimedes::reflect_functions("i_dont_collide");
    ASSERT(idc.begin()->size() == 1);
    ASSERT(idc.begin()->first()->parameters().size() == 6);

    return 0;
}

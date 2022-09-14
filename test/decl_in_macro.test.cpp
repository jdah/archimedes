#include "test.hpp"
#include "decl_in_macro.test.hpp"

FORCE_TYPE_INSTANTIATION(Foo<int>);

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto f = archimedes::reflect<Foo<int>>();
    ASSERT(f);

    const auto as = f->annotations();
    auto it = std::find_if(
        as.begin(),
        as.end(),
        [](const auto &a) {
            return a.starts_with("_decl_me_");
        });
    ASSERT(it != as.end());
    return 0;
}

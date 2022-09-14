#include "test.hpp"
#include "basic.test.hpp"

int free_func(Eeeee e) {
    LOG("here!");
    return 135;
}

int free_func2(void) {
    LOG("here!");
    return 135;
}

static int free_func3(void) {
    LOG("here!");
    return 135;
}

FORCE_FUNCTION_INSTANTIATION(free_func2)
FORCE_FUNCTION_INSTANTIATION(free_func3)

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto opt_foo = archimedes::reflect<baz::Foo>();
    ASSERT(opt_foo);
    const auto opt_glob = archimedes::reflect<Glob>();
    ASSERT(opt_glob);
    const auto opt_goob = archimedes::reflect<Goob<Glob>>();
    ASSERT(opt_goob);

    const auto fs = archimedes::reflect_functions<int()>();
    ASSERT(fs);
    ASSERT(fs->size() == 2);

    decltype(fs)::value_type::const_iterator pos_ff2, pos_ff3;
    ASSERT(
        (pos_ff2 = std::find_if(
            fs->begin(), fs->end(),
            [](const archimedes::reflected_function &f) {
                return f.name() == "free_func2";
            })) != fs->end());
    ASSERT(
        (pos_ff3 = std::find_if(
            fs->begin(), fs->end(),
            [](const archimedes::reflected_function &f) {
                return f.name() == "free_func3";
            })) != fs->end());

    const auto ff2 = *pos_ff2, ff3 = *pos_ff3;
    ASSERT(ff2.can_invoke());
    ASSERT(!ff3.can_invoke());

    return 0;
}

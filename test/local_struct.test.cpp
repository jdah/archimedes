#include "test.hpp"
#include "local_struct.test.hpp"

struct SomeStruct {
    int x;
};

int func(const SomeStruct &s) { return s.x; }

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto r_func = archimedes::reflect_function("func");
    ASSERT(r_func);
    ASSERT(!r_func->can_invoke());
    return 0;
}

#include "test.hpp"
#include "any_add_ptr.test.hpp"

int main(int argc, char *argv[]) {
    int x = 2;
    auto a0 = archimedes::any::make(x);
    auto a1 = archimedes::any::make(&x);
    auto a2 = archimedes::any::make(&x, a0.id().add_pointer());
    ASSERT(a1.id() == a2.id());
    return 0;
};

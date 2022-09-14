#include "test.hpp"
#include "unique_ptr.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();

    Foo<std::unique_ptr<Bar>> f;
    f.t = std::make_unique<Bar>();
    f.t->x = 44;

    // TODO: test more than just compile

    return 0;
}

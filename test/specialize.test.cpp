#include "test.hpp"
#include "specialize.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    Bar b(2, 3, 4);
    Qux q(3);
    Foo<Bar> f;
    Foo<Qux> f_q;
    ASSERT(archimedes::reflect<Foo<Bar>>()->default_constructor());
    ASSERT(archimedes::reflect<Foo<Bar>>()->copy_constructor());
    ASSERT(archimedes::reflect<Foo<Bar>>()->copy_assignment());
    ASSERT(archimedes::reflect<Foo<Qux>>()->default_constructor());
    ASSERT(archimedes::reflect<Foo<Qux>>()->copy_constructor());
    ASSERT(archimedes::reflect<Foo<Qux>>()->copy_assignment());
    return 0;
}

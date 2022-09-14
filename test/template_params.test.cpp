#include "test.hpp"
#include "template_params.test.hpp"

using MyBar = Bar<float, 44>;
FORCE_TYPE_INSTANTIATION(MyBar);

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto ps =
        archimedes::reflect<Bar<float, 44>>()->template_parameters();
    ASSERT(ps.size() == 2);
    ASSERT(ps[0].name() == "T");
    ASSERT(ps[0].is_typename());
    ASSERT(*ps[0].type() == archimedes::reflect<float>());
    ASSERT(ps[1].name() == "X");
    ASSERT(!ps[1].is_typename());
    ASSERT(*ps[1].type() == archimedes::reflect<int>());
    ASSERT(ps[1].value()->as<int>() == 44);

    ASSERT(archimedes::reflect<Baz>()->template_parameters().size() == 0);
    ASSERT(archimedes::reflect<Qux>()->template_parameters().size() == 0);

    Foo<Baz> foo;
    foo.t.x = 12;
    ASSERT(archimedes::reflect<Foo<Baz>>()->template_parameters().size() == 1);

    ASSERT(
        archimedes::reflect<Foob>()->bases()[0]
           .type()
           .template_parameters()
           .size() == 2);
    return 0;
}

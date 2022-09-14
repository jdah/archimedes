#include "test.hpp"

#include "annotations.test.hpp"

FORCE_TYPE_INSTANTIATION(Bar<int>)
FORCE_TYPE_INSTANTIATION(Bar<float>)
FORCE_TYPE_INSTANTIATION(Baz)

int main(int argc, char *argv[]) {
    archimedes::load();

    // check that template instantiations have annotations
    const auto
        baz = archimedes::reflect<Baz>(),
        bar = archimedes::reflect<Bar<int>>();
    ASSERT(baz);
    ASSERT(baz->annotations().size() == 1);
    ASSERT(baz->annotations()[0] == bar->annotations()[0]);

    const auto foo = archimedes::reflect("Foo");
    ASSERT(foo->annotations().size() == 2);

    const auto as = foo->annotations();
    const auto it_abc =
        std::find(
            as.begin(),
            as.end(),
            "abc");
    ASSERT(it_abc != as.end());

    const auto it_xyz =
        std::find(
            as.begin(),
            as.end(),
            "xyz");
    ASSERT(it_xyz != as.end());

    ASSERT(foo->as_record().static_field("y")->annotations().front() == "bcd");
    ASSERT(foo->as_record().field("x")->annotations().front() == "aaaa");
    ASSERT(foo->as_record().field("z")->annotations().front() == "bbb");
    ASSERT(foo->as_record().function("bar")->annotations().front() == "efg");

    // ensure that fundamental types are not polluted with annotations
    ASSERT(archimedes::reflect<int>()->annotations().empty());
    ASSERT(archimedes::reflect<int(int)>()->annotations().empty());

    return 0;
}

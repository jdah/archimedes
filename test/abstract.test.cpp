#include "test.hpp"
#include "abstract.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();

    IAmNotAbstract foo = IAmNotAbstract();

    const auto iaa = archimedes::reflect<IAmAbstract>();
    ASSERT(iaa);
    ASSERT(iaa->as_record().is_abstract());

    const auto iaa_ctor = iaa->as_record().default_constructor();
    ASSERT(iaa_ctor);
    ASSERT(!iaa_ctor->can_invoke());

    ASSERT(iaa->as_record().function("some_method"));
    ASSERT(iaa->as_record().function("some_method")->can_invoke());

    const auto iaavi = archimedes::reflect<IAmAbstractViaInheritance>();
    ASSERT(iaavi);
    ASSERT(iaavi->as_record().is_abstract());

    const auto iaavi_ctor = iaavi->as_record().default_constructor();
    ASSERT(iaavi_ctor);
    ASSERT(!iaavi_ctor->can_invoke());

    const auto ianavi = archimedes::reflect<IAmNotAbstractViaInheritance>();
    ASSERT(ianavi);
    ASSERT(!ianavi->as_record().is_abstract());

    const auto iana = archimedes::reflect<IAmNotAbstract>();
    ASSERT(iana);
    ASSERT(!iana->as_record().is_abstract());

    return 0;
}

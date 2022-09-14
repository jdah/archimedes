#include "test.hpp"
#include "this.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto f = archimedes::reflect<Foo>();
    ASSERT(f);
    const auto bar = f->as_record().function_set("bar");

    const auto bar_cl =
        *std::find_if(
            bar->begin(), bar->end(),
            [](const auto &f){
                return f.type().is_const() && f.type().is_ref();
            });

    const auto bar_cr =
        *std::find_if(
            bar->begin(), bar->end(),
            [](const auto &f){
                return f.type().is_const() && f.type().is_rref();
            });

    const auto bar_l =
        *std::find_if(
            bar->begin(), bar->end(),
            [](const auto &f){
                return !f.type().is_const() && f.type().is_ref();
            });

    const auto bar_r =
        *std::find_if(
            bar->begin(), bar->end(),
            [](const auto &f){
                return !f.type().is_const() && f.type().is_rref();
            });

    Foo foo;
    ASSERT(bar_cl.invoke(archimedes::cref(foo))->as<int>() == 2);
    ASSERT(bar_cr.invoke(Foo())->as<int>() == 3);
    ASSERT(bar_l.invoke(archimedes::ref(foo))->as<int>() == 4);
    ASSERT(bar_r.invoke(Foo())->as<int>() == 5);

    const auto baz = f->as_record().function_set("baz");
    ASSERT(baz);

    const auto baz_const =
        *std::find_if(
            baz->begin(), baz->end(),
            [](const auto &f){
                return f.type().is_const();
            });

    const auto baz_not_const =
        *std::find_if(
            baz->begin(), baz->end(),
            [](const auto &f){
                return !f.type().is_const();
            });

    ASSERT(baz_const.invoke(const_cast<const Foo*>(&foo))->as<int>() == 6);
    ASSERT(baz_not_const.invoke(&foo)->as<int>() == 7);

    return 0;
}

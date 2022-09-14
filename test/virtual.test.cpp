#include "test.hpp"
#include "virtual.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto mf = archimedes::reflect("xy::MaybeFeathered");
    ASSERT(mf);

    const auto animal = archimedes::reflect<Animal>();
    ASSERT(animal);

    const auto biped = archimedes::reflect<Animal>();
    ASSERT(biped);

    const auto fuzzy_animal = archimedes::reflect<Animal>();
    ASSERT(fuzzy_animal);

    const auto quadruped = archimedes::reflect<Animal>();
    ASSERT(quadruped);

    const auto cat = archimedes::reflect<Animal>();
    ASSERT(cat);

    const auto dog = archimedes::reflect<Animal>();
    ASSERT(dog);

    Dog d;
    const auto quadruped_num_legs = quadruped->as_record().function("num_legs");
    ASSERT(quadruped_num_legs->invoke(&d)->as<int>() == 4);

    const auto animal_speak = animal->as_record().function("speak");
    ASSERT(
        animal_speak->invoke(dynamic_cast<Animal*>(&d))->as<std::string>()
            == "woof");
}

#pragma once

#include <string>

namespace x {
    namespace y {
        struct MaybeFeathered {
            virtual ~MaybeFeathered() = default;
            virtual bool has_feathers() const = 0;
        };
    }
}

namespace xy = x::y;

struct Animal {
    virtual ~Animal() = default;

    virtual bool is_fuzzy() const = 0;

    virtual int num_legs() const = 0;

    virtual std::string speak() const = 0;
};

struct Biped : virtual public Animal, public xy::MaybeFeathered {
    int num_legs() const override { return 2; }

    bool has_feathers() const override {
        return false;
    }
};

struct FuzzyAnimal : virtual public Animal {
    bool is_fuzzy() const override {
        return true;
    }
};

struct Quadruped : virtual public Animal {
    int num_legs() const override {
        return 4;
    }
};

struct Cat : public FuzzyAnimal, public Quadruped {
    std::string speak() const {
        return "meow";
    }
};

struct Dog : public FuzzyAnimal, public Quadruped {
    Dog()
        : Animal(),
          FuzzyAnimal(),
          Quadruped() {}

    std::string speak() const {
        return "woof";
    }
};

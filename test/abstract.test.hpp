#pragma once

struct IAmAbstract {
    int x;

    IAmAbstract() { }
    virtual void some_method() = 0;
};

struct IAmAbstractViaInheritance : public IAmAbstract {
    IAmAbstractViaInheritance() : IAmAbstract() {}
};

struct IAmNotAbstractViaInheritance : public IAmAbstractViaInheritance {
    IAmNotAbstractViaInheritance() : IAmAbstractViaInheritance() { }
    void some_method() override { /* ... */ }
};

struct IAmNotAbstract : public IAmAbstract {
    IAmNotAbstract() : IAmAbstract() { }
    void some_method() override { /* ... */ }
};

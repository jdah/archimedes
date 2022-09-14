#pragma once

#include <utility>

template <typename T>
struct Boo {
    T t;

    T operator()(T t) {
        return t + 1;
    }
};

using IntBoo = Boo<int>;

template <typename T, typename U>
struct Qux {

};

template <typename T>
struct Qux<T, int> {
    T operator()(T t) {
        return t + 1;
    }
};

struct Foo {
    operator IntBoo() const {
        return IntBoo { .t = 144 };
    }

    template <typename T>
    int bar(const T &t) const {
        return static_cast<int>(t) + 1;
    }

    int bar(int &&i) const {
        return 2;
    }

    template <typename T, typename U>
    void baz() {
        T();
        U();
    }
};

template <>
struct Boo<double> {
    double special_function() { return 4.0; }
};

struct IntHolder {
    int i;

    IntHolder() = default;
    IntHolder(int x) : i(x) {}
};

struct Quxical {
    template <typename E, typename ...Args>
    E &qux(int i, Args&& ...args) {
        return (*new E(std::forward<Args>(args)...));
    }
};

#include "test.hpp"
#include "builtin.test.hpp"

template <typename T>
void test(AllBuiltins &ab, const archimedes::reflected_field &f, const T &t) {
    f.set(ab, t);
    ASSERT(
        *f.get<T>(ab) == t,
        "failed for field {}",
        archimedes::reflect<T>()->name());
}

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto r = archimedes::reflect<AllBuiltins>()->as_record();

    AllBuiltins ab;
    test(ab, *r.field("b"), true);
    test(ab, *r.field("c"), 'a');
    test(ab, *r.field("uc"), static_cast<unsigned char>('a'));
    test(ab, *r.field("us"), static_cast<unsigned short>(12));
    test(ab, *r.field("ui"), static_cast<unsigned int>(13));
    test(ab, *r.field("ul"), static_cast<unsigned long>(14));
    test(ab, *r.field("ull"), static_cast<unsigned long long>(15));
    test(ab, *r.field("sc"), static_cast<signed char>(16));
    test(ab, *r.field("ss"), static_cast<signed short>(17));
    test(ab, *r.field("si"), static_cast<signed int>(-21414));
    test(ab, *r.field("sl"), static_cast<signed long>(-23535));
    test(ab, *r.field("sll"), static_cast<signed long long>(-24586258));
    test(ab, *r.field("f"), static_cast<float>(1.34f));
    test(ab, *r.field("d"), static_cast<double>(1.555));

    bool b = false;
    test(ab, *r.field("pb"), static_cast<bool*>(&b));

    int i = 12;
    test(ab, *r.field("psi"), static_cast<int*>(&i));

    unsigned int x = 124;
    test(ab, *r.field("pui"), static_cast<unsigned int*>(&x));
    return 0;
}

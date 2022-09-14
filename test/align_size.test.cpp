#include "test.hpp"
#include "align_size.test.hpp"

FORCE_TYPE_INSTANTIATION(AllBuiltins)

int main(int argc, char *argv[]) {
    archimedes::load();

    AllBuiltins ab;
    ab.b = true;

    // test align is correct
    ASSERT(
        archimedes::reflect<AllBuiltins>()->align()
            == std::alignment_of_v<AllBuiltins>);
    ASSERT(
        archimedes::reflect<int*>()->align()
           == std::alignment_of_v<int*>);
    ASSERT(
        archimedes::reflect<double>()->align()
            == std::alignment_of_v<double>);
    ASSERT(
        archimedes::reflect<unsigned long long>()->align()
            == std::alignment_of_v<unsigned long long>);

    // test size is correct
    ASSERT(
        archimedes::reflect<AllBuiltins>()->size()
            == sizeof(AllBuiltins));
    ASSERT(
        archimedes::reflect<int*>()->size()
           == sizeof(int*));
    ASSERT(
        archimedes::reflect<double>()->size()
            == sizeof(double));
    ASSERT(
        archimedes::reflect<unsigned long long>()->size()
            == sizeof(unsigned long long));
    return 0;
}

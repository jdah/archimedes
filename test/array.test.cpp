#include "test.hpp"
#include "array.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    // should be able to find main using all possible array/pointer variants
    ASSERT(archimedes::reflect_functions<int(int, char*[])>());
    ASSERT(archimedes::reflect_functions<int(int, char**)>());

    const auto foo = archimedes::reflect<Foo>()->as_record();
    ASSERT(foo.field("xs")->type()->as_array().length() == 2);
    ASSERT(foo.field("ys")->type()->as_array().length() == 222);
    ASSERT(foo.field("zs")->type()->as_array().length() == 0);
    ASSERT(foo.field("ws")->type()->as_array().length() == 224);

    // TODO: should be able to find length of array params

    const auto isma =
        archimedes::reflect_functions("i_sum_my_array").begin()->first();
    int arr[8] = { 1, 1, 1, 1, 2, 2, 2, 2 };
    ASSERT(isma->invoke(arr)->as<int>() == 12);

    const auto ismca =
        archimedes::reflect_functions("i_sum_my_const_array").begin()->first();
    arr[0] = 100;
    ASSERT(ismca->invoke(arr)->as<int>() == 111);

    // should be able to find both functions via type
    ASSERT(archimedes::reflect_functions<int(int*)>()->size() == 1);
    ASSERT(archimedes::reflect_functions<int(const int*)>()->size() == 1);

    return 0;
}

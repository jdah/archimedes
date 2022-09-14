#include "test.hpp"
#include "nested_template_args.test.hpp"

FORCE_TYPE_INSTANTIATION(Foo<ns::Enum>)
namespace std {
    FORCE_TYPE_INSTANTIATION(Foo<Bar<tuple<int, ns::Enum, tuple<basic_string<char>>>>>)
}
ARCHIMEDES_REFLECT_TYPE_REGEX(".*Foo.*")
FORCE_TYPE_INSTANTIATION(q::Baz)

int main(int argc, char *argv[]) {
    Foo<ns::Enum> f;
    ASSERT(f.foo(ns::Enum::A) == ns::Enum::A);

    Foo<Bar<std::tuple<int, ns::Enum, std::tuple<std::basic_string<char>>>>> f2;
    f2.foo(Bar<std::tuple<int, ns::Enum, std::tuple<std::basic_string<char>>>>());

    return 0;
}

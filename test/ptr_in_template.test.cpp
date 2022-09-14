#include "test.hpp"
#include "ptr_in_template.test.hpp"

int main(int argc, char *argv[]) {
    archimedes::load();
    const auto foo = archimedes::reflect<Foo>();
    ASSERT(foo);
    ASSERT(foo->as_record().function<void(std::shared_ptr<bool[]>)>());
    ASSERT(foo->as_record().function("Foo"));
    return 0;
}

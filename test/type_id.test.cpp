#include "test.hpp"
#include "type_id.test.hpp"

FORCE_TYPE_INSTANTIATION(A<float>)
FORCE_TYPE_INSTANTIATION(A<int>)

int main(int argc, char *argv[]) {
    archimedes::load();
    ASSERT(
        *archimedes::reflect<Z>()->type_id_hash()
            == typeid(Z).hash_code());
    ASSERT(
        *archimedes::reflect<Z&>()->type()->type().type_id_hash()
            == typeid(Z&).hash_code());
    ASSERT(
        *archimedes::reflect<A<float>>()->type_id_hash()
            == typeid(A<float>).hash_code());
    ASSERT(
        *archimedes::reflect<A<int>>()->type_id_hash()
            == typeid(A<int>).hash_code());
    ASSERT(
        *archimedes::reflect<A<int*>>()->type_id_hash()
            == typeid(A<int*>).hash_code());

    // const should not affect things
    ASSERT(
        *archimedes::reflect<A<int*>>()->type_id_hash()
            == typeid(const A<int*>).hash_code());

    // check that everything works with dynamic types
    D1 d1;
    D0 *d0 = dynamic_cast<D0*>(&d1);
    ASSERT(
        *archimedes::reflect<D1>()->type_id_hash()
            == typeid(*d0).hash_code());
    return 0;
}

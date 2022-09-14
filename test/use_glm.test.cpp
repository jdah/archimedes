#include "test.hpp"
#include "use_glm.test.hpp"

ARCHIMEDES_ARG("explicit-enable")
ARCHIMEDES_ARG("enable")

ARCHIMEDES_REFLECT_TYPE(Struct0)
ARCHIMEDES_REFLECT_TYPE(Struct1)

// TODO:
/* ARCHIMEDES_REFLECT_TYPE(glm::vec2) */
/* ARCHIMEDES_REFLECT_TYPE(glm::vec3) */
/* ARCHIMEDES_REFLECT_TYPE(glm::vec4) */

/* ARCHIMEDES_REFLECT_TYPE(glm::uvec2) */
/* ARCHIMEDES_REFLECT_TYPE(glm::uvec3) */
/* ARCHIMEDES_REFLECT_TYPE(glm::uvec4) */

/* ARCHIMEDES_REFLECT_TYPE(glm::ivec2) */
/* ARCHIMEDES_REFLECT_TYPE(glm::ivec3) */
/* ARCHIMEDES_REFLECT_TYPE(glm::ivec4) */

/* ARCHIMEDES_REFLECT_TYPE(glm::bvec2) */
/* ARCHIMEDES_REFLECT_TYPE(glm::bvec3) */
/* ARCHIMEDES_REFLECT_TYPE(glm::bvec4) */

using myvec3 = glm::vec3;

int main(int argc, char *argv[]) {
    archimedes::load();
    ASSERT(
        archimedes::reflect<Struct1>()
            ->as_record()
            .static_field("VEC1")
            ->constexpr_value()
            ->as<uvec2>() == uvec2(23, 23));
    // TODO
    /* ASSERT(archimedes::reflect<glm::vec3>()); */
    /* ASSERT(archimedes::reflect<myvec3>()); */
    return 0;
}

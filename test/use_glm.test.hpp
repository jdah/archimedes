#pragma once

#include "glm/glm/glm.hpp"

namespace ns {
    using uvec2 = glm::uvec2;
};

using namespace ns;

struct Struct0 {
    static constexpr auto VEC = uvec2(12, 12);
};

struct Struct1 {
    static constexpr auto VEC1 = (Struct0::VEC * 2u) - 1u;
};

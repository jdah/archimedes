#pragma once

#include <string_view>

#include "util.hpp"

namespace archimedes {
enum class ImplicitFunction {
    DEFAULT_CTOR = 0,
    COPY_CTOR,
    MOVE_CTOR,
    COPY_ASSIGN,
    MOVE_ASSIGN,
    DTOR,
    COUNT
};

inline std::string get_implicit_name(
    ImplicitFunction if_kind,
    std::string_view record_name) {
    switch (if_kind) {
        case ImplicitFunction::DEFAULT_CTOR:
        case ImplicitFunction::COPY_CTOR:
        case ImplicitFunction::MOVE_CTOR:
            return std::string(record_name);
        case ImplicitFunction::COPY_ASSIGN:
        case ImplicitFunction::MOVE_ASSIGN:
            return "operator=";
        case ImplicitFunction::DTOR:
            return "~" + std::string(record_name);
        default: ASSERT(false);
    }
}
} // namespace archimedes

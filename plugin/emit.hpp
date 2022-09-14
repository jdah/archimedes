#pragma once

#include <archimedes.hpp>

#include "plugin.hpp"
#include "util.hpp"

using namespace archimedes;
using namespace archimedes::detail;

namespace archimedes {
// emit type name (clang type) to C++ type
inline std::string emit_type(const Context &ctx, const clang::Type &type) {
    return get_full_type_name(ctx, type);
}

// emit type name (clang type) to C++ type
inline std::string emit_type(const Context &ctx, const clang::QualType &type) {
    return get_full_type_name(ctx, type);
}

// emit vector as c++ initializer list
template <typename T, typename F>
    requires (requires (F f, T t) { { f(t) } -> std::same_as<std::string>; })
inline std::string emit_vector(
    const vector<T> &ts,
    F &&f,
    std::string_view sep = ",") {
    vector<std::string> ss;
    for (const auto &t : ts) {
        ss.push_back(f(t));
    }

    return fmt::format("{{{}}}", fmt::join(ss, sep));
}

// emit any with constant value
inline std::string emit_any(const std::string &value) {
   return
       fmt::format(
            "{}::make({})",
            NAMEOF_TYPE(archimedes::any),
            value);
}

// emit char array as array initializer
inline std::string emit_data(std::span<const uint8_t> buffer) {
    std::string out;

    // reserve two curly braces + length(0xXX,) * size
    out.resize(2 + buffer.size() * 5);

    out[0] = '{';
    for (size_t i = 0; i < buffer.size(); i++) {
        char cs[6];
        sprintf(cs, "0x%02x,", static_cast<int>(buffer[i]));
        std::memcpy(&out[1 + (i * 5)], &cs, 5);
    }
    out[out.length() - 1] = '}';

    return out;
}

// emit context as c++
std::string emit(Context&);
}

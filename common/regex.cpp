#include <regex>
#include <archimedes/regex.hpp>

#define FMT_HEADER_ONLY
#include "../lib/fmt/include/fmt/core.h"

bool archimedes::regex_matches(
    std::string_view regex,
    std::string_view string) {
    return std::regex_match(
        std::string(string),
        std::regex(
            "^" + std::string(regex) + "$",
            std::regex_constants::ECMAScript));
}

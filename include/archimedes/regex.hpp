#pragma once

#include <string_view>
#include <string>

namespace archimedes {
// escape a string literal for use in a regex
inline std::string escape_for_regex(std::string_view s) {
    constexpr char meta_chars[] = R"(\.^$-+()[]{}|?*)";
    std::string out;
    out.reserve(s.size());
    for (auto ch : s) {
        if (std::strchr(meta_chars, ch)) {
            out.push_back('\\');
        }
        out.push_back(ch);
    }
    return out;
}

// returns true if the regex matches the whole string
bool regex_matches(std::string_view regex, std::string_view string);
}

#pragma once

#include <vector>

template <typename T, typename F>
inline void each(const T &ts, F &&f) {
    for (const auto &t : ts) {
        f(t);
    }
}


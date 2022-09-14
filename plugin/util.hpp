#pragma once

#include <string>
#include <iostream>
#include <filesystem>
#include <sstream>

#define FMT_HEADER_ONLY
#include <fmt/core.h>

#include <nameof.hpp>

#include <llvm/Support/raw_ostream.h>

// undef to turn off logging messages
// #define PLUGIN_VERBOSE

#ifdef PLUGIN_VERBOSE
#define PLUGIN_LOG(...) LOG(__VA_ARGS__)
#else
#define PLUGIN_LOG(...) if (Context::instance().verbose) { LOG(__VA_ARGS__); }
#endif

template <typename ...Args>
static std::string _assert_fmt(
    const std::string &fmt = "",
    Args&&... args) {
    if (fmt.length() > 0) {
        return fmt::vformat(
            std::string_view(fmt),
            fmt::make_format_args(std::forward<Args>(args)...));
    }

    return "";
}

#define LOG_PREFIX()  \
    fmt::format(      \
        "[{}:{}][{}]",\
        __FILE__,     \
        __LINE__,     \
        __FUNCTION__)

#define LOG(_fmt, ...)                        \
    std::cout                                 \
        << LOG_PREFIX()                       \
        << " "                                \
        << (fmt::format(_fmt, ## __VA_ARGS__))\
        << std::endl;

#define ASSERT(_e, ...) do {                                                   \
        if (!(_e)) {                                                           \
            const auto __msg = _assert_fmt(__VA_ARGS__);                       \
            LOG(                                                               \
                "ASSERTION FAILED{}{}", __msg.length() > 0 ? " " : "", __msg); \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)

template <typename... T>
constexpr auto make_array(T&&... values) ->
    std::array<
        typename std::decay<
            typename std::common_type<T...>::type>::type,
            sizeof...(T)> {
    return std::array<
        typename std::decay<
            typename std::common_type<T...>::type>::type,
            sizeof...(T)>{ std::forward<T>(values)... };
}

// hash arbitrary number of values with std::hash impl
template <typename...> struct hasher;

template<typename T>
struct hasher<T> : public std::hash<T> {
    using std::hash<T>::hash;
};

template <typename T, typename ...Ts>
struct hasher<T, Ts...> {
    inline std::size_t operator()(const T& v, const Ts& ...ts) {
        auto seed = hasher<Ts...>{}(ts...);
        seed ^= hasher<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

template <typename ...Ts>
inline std::size_t hash(const Ts& ...ts) {
    return hasher<Ts...>{}(ts...);
}

namespace std {
template <>
struct hash<std::filesystem::path> {
    auto operator()(const std::filesystem::path &p) const {
        return std::filesystem::hash_value(p);
    }
};
}

template <typename F>
std::string f_ostream_to_string(F &&f) {
    std::string s;
    llvm::raw_string_ostream os(s);
    f(os);
    return s;
}

// TODO: doc
inline std::string escape_for_fmt(std::string_view str) {
    std::stringstream res;
    for (const auto &c : str) {
        if (c == '{' || c == '}') {
            res << c;
        }

        res << c;
    }
    return res.str();
}

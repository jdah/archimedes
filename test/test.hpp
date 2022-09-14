#pragma once

#include <string>
#include <iostream>
#include <algorithm>

#define FMT_HEADER_ONLY
#include <fmt/core.h>

#include <archimedes.hpp>

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
            const auto __msg = _assert_fmt(__VA_ARGS__);                      \
            LOG(                                                               \
                "ASSERTION FAILED{}{}", __msg.length() > 0 ? " " : "", __msg); \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)

#define CONCAT_IMPL(x, y) x ## y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define FORCE_FUNCTION_INSTANTIATION_IMPL(_f, _n)                              \
    struct _n { _n(const void *p) {                                            \
        std::string _ = fmt::format("{}", fmt::ptr(p)); } };                   \
    auto CONCAT(_i_, __COUNTER__) = _n(reinterpret_cast<const void*>(&(_f)));

#define FORCE_FUNCTION_INSTANTIATION(_f)                                       \
    FORCE_FUNCTION_INSTANTIATION_IMPL(_f, CONCAT(_fi_t_, __COUNTER__))

#define FORCE_TYPE_INSTANTIATION_IMPL(_n, ...) \
    struct _n {                                \
        __VA_ARGS__ t;                         \
        void *p; _n() : p(foo()) {}            \
            void *foo() { return &t; }};       \
    auto CONCAT(_i_, __COUNTER__) = _n();

#define FORCE_TYPE_INSTANTIATION(...)                                          \
    FORCE_TYPE_INSTANTIATION_IMPL(CONCAT(_i_, __COUNTER__), __VA_ARGS__)


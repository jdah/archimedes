#pragma once

// includes version string, basic control macros so that the whole
// "archimedes.hpp" header does not need to be included for TUs which only need
// access to control macros

#define ARCHIMEDES_VERSION_MAJOR  0
#define ARCHIMEDES_VERSION_MINOR  0
#define ARCHIMEDES_VERSION_BUGFIX 1

#define ARCHIMEDES_STRINGIFY_IMPL(x) #x
#define ARCHIMEDES_STRINGIFY(x) ARCHIMEDES_STRINGIFY_IMPL(x)

#define ARCHIMEDES_CONCAT_IMPL(x, y) x ## y
#define ARCHIMEDES_CONCAT(x, y) ARCHIMEDES_CONCAT_IMPL(x, y)

#define ARCHIMEDES_STRCAT(x, y) x y

#define ARCHIMEDES_VERSION_STRING (                    \
    ARCHIMEDES_STRINGIFY(ARCHIMEDES_VERSION_MAJOR) "." \
    ARCHIMEDES_STRINGIFY(ARCHIMEDES_VERSION_MINOR) "." \
    ARCHIMEDES_STRINGIFY(ARCHIMEDES_VERSION_BUGFIX))

// pass symbol to ARCHIMEDES_PRAGMA to use as pragma
// assumes NOT already prefixed with "archimedes"
#define _ARCHIMEDES_PRAGMA_IMPL(...) _Pragma(#__VA_ARGS__)
#define ARCHIMEDES_PRAGMA(x) _ARCHIMEDES_PRAGMA_IMPL(archimedes x)

#define _ARCHIMEDES_STRING_PRAGMA_IMPL(x) _Pragma(x)
#define ARCHIMEDES_STRING_PRAGMA(x) _ARCHIMEDES_STRING_PRAGMA_IMPL(x)

// annotate types/functions
#define ARCHIMEDES_ANNOTATE(_a) clang::annotate(_a)

// annotate types/functions via attribute
#define ARCHIMEDES_ANNOTATE_ATTR(_a) __attribute__((annotate(_a)))

// types can be annotated with "ARCHIMEDES_NO_REFLECT" to avoid getting
// reflected
#define _ARCHIMEDES_NO_REFLECT_TEXT "_archimedes_no_reflect_"
#define ARCHIMEDES_NO_REFLECT ARCHIMEDES_ANNOTATE(_ARCHIMEDES_NO_REFLECT_TEXT)

// types can be annotated with "ARCHIMEDES_REFLECT" to ensure their being
// reflected in "explicit enable" mode
#define _ARCHIMEDES_REFLECT_TEXT "_archimedes_reflect_"
#define ARCHIMEDES_REFLECT ARCHIMEDES_ANNOTATE(_ARCHIMEDES_REFLECT_TEXT)

// types can be explicitly reflected from anywhere via this macro
#define ARCHIMEDES_REFLECT_TYPE_NAME _archimedes_reflect_type_
#define ARCHIMEDES_REFLECT_TYPE(...)                                   \
    inline __attribute__((unused))                                     \
        ARCHIMEDES_ANNOTATE_ATTR("_archimedes_reflect_type_")          \
        const auto                                                     \
        ARCHIMEDES_CONCAT(ARCHIMEDES_REFLECT_TYPE_NAME, __COUNTER__) = \
            archimedes::detail::template_arg<__VA_ARGS__>();

// types can be explicitly reflected via regex through this macro
#define _ARCHIMEDES_REFLECT_TYPE_REGEX_TEXT                                    \
    ARCHIMEDES_STRINGIFY(_archimedes_reflect_regex_type_1)
#define ARCHIMEDES_REFLECT_TYPE_REGEX(rx)                                      \
    ARCHIMEDES_PRAGMA(_archimedes_reflect_regex_type_1 rx)

// use to force template instantiation
#define ARCHIMEDES_FORCE_TEMPLATE_TYPE_INSTANTIATION(...)                      \
    extern template <> struct __VA_ARGS__;

// used to define arguments inside of individual files
// fx. "-fplugin-arg-archimedes-exclude-ns-std"
//        -> ARCHIMEDES_ARG("exclude-ns-std")
#define ARCHIMEDES_ARG_NAME _archimedes_arg_
#define ARCHIMEDES_ARG(_a)                                                     \
    inline __attribute__((unused)) ARCHIMEDES_ANNOTATE_ATTR(_a) const auto     \
        ARCHIMEDES_CONCAT(ARCHIMEDES_ARG_NAME, __COUNTER__) =                  \
            archimedes::detail::arg();

namespace archimedes {
namespace detail {
// used for ARCHIMEDES_ARG implementation
struct arg {};

// used for some archimedes pragmas requiring a typename
template <typename T>
struct template_arg {};
} // namespace detail
} // namespace archimedes

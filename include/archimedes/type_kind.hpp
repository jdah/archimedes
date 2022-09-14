#pragma once

#include <cstdint>
#include <type_traits>

namespace archimedes {
// TODO: VLAs?
// TODO: long double
// TODO: u128, i128
enum type_kind : uint8_t {
    UNKNOWN = 0,
    VOID,
    BOOL,
    CHAR,
    U_CHAR,
    U_SHORT,
    U_INT,
    U_LONG,
    U_LONG_LONG,
    I_CHAR,
    I_SHORT,
    I_INT,
    I_LONG,
    I_LONG_LONG,
    FLOAT,
    DOUBLE,
    ENUM,
    FUNC,
    ARRAY,
    STRUCT,
    UNION,
    PTR,
    REF,
    RREF,
    MEMBER_PTR
};

template <typename T>
type_kind get_kind() {
    if constexpr (std::is_pointer_v<T>) {
        return PTR;
    } else if constexpr(std::is_rvalue_reference_v<T>) {
        return RREF;
    } else if constexpr (std::is_reference_v<T>) {
        return REF;
    } else if constexpr (std::is_member_pointer_v<T>) {
        return MEMBER_PTR;
    } else if constexpr (std::is_array_v<T>) {
        return ARRAY;
    } else if constexpr (std::is_class_v<T>) {
        return STRUCT;
    } else if constexpr (std::is_function_v<T>) {
        return FUNC;
    } else if constexpr (std::is_enum_v<T>) {
        return ENUM;
    } else if constexpr (std::is_union_v<T>) {
        return UNION;
    } else if constexpr (std::is_same_v<T, void>) {
        return VOID;
    } else if constexpr (std::is_same_v<T, unsigned char>) {
        return U_CHAR;
    } else if constexpr (std::is_same_v<T, unsigned short>) {
        return U_SHORT;
    } else if constexpr (std::is_same_v<T, unsigned int>) {
        return U_INT;
    } else if constexpr (std::is_same_v<T, unsigned long>) {
        return U_LONG;
    } else if constexpr (std::is_same_v<T, unsigned long long>) {
        return U_LONG_LONG;
    } else if constexpr (std::is_same_v<T, signed char>) {
        return I_CHAR;
    } else if constexpr (std::is_same_v<T, short>) {
        return I_SHORT;
    } else if constexpr (std::is_same_v<T, int>) {
        return I_INT;
    } else if constexpr (std::is_same_v<T, long>) {
        return I_LONG;
    } else if constexpr (std::is_same_v<T, long long>) {
        return I_LONG_LONG;
    } else if constexpr (std::is_same_v<T, bool>) {
        return BOOL;
    } else if constexpr (std::is_same_v<T, float>) {
        return FLOAT;
    } else if constexpr (std::is_same_v<T, double>) {
        return DOUBLE;
    } else if constexpr (std::is_same_v<T, char>) {
        return CHAR;
    }
}
}

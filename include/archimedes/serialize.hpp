#pragma once

#include <iostream>

#include "archimedes/type_info.hpp"

// simple and fast (de)serialization based on iostream
namespace archimedes {
namespace detail {
struct bytestream : public std::istream {
    struct bytebuf : public std::basic_streambuf<char> {
        bytebuf(std::span<const uint8_t> bytes) {
            char *p =(char*) &bytes[0];
            setg(p, p, p + bytes.size());
        }
    };

    bytestream(std::span<const uint8_t> bytes)
        : std::istream(&buf),
          buf(bytes) {}

private:
    bytebuf buf;
};

template <typename T>
T deserialize(std::span<const uint8_t> bytes) {
    T t;
    auto is = bytestream(bytes);
    deserialize(is, t);
    return t;
}

template <typename T>
    requires (std::is_integral_v<T> || std::is_enum_v<T>)
void serialize(std::ostream &os, const T &t) {
    os.write(reinterpret_cast<const char*>(&t), sizeof(T));
}

template <typename T>
    requires (std::is_integral_v<T> || std::is_enum_v<T>)
void deserialize(std::istream &is, T &t) {
    char buffer[sizeof(T)];
    is.read(buffer, sizeof(T));
    t = *reinterpret_cast<T*>(&buffer[0]);
}

template <typename T>
void serialize(std::ostream &os, const std::unique_ptr<T> &ptr) {
    if (!ptr) {
        ARCHIMEDES_FAIL("attempt to serialize nullptr");
    }
    serialize(os, *ptr);
}

template <typename T>
void deserialize(std::istream &is, std::unique_ptr<T> &ptr) {
    if (!ptr) {
        ptr = std::make_unique<T>();
    }

    deserialize(is, *ptr);
}

inline void serialize(std::ostream &os, const std::string &str) {
    serialize(os, str.length());
    if (str.length()) {
        os.write(&str[0], str.length());
    }
}

inline void deserialize(std::istream &is, std::string &str) {
    std::string::size_type sz;
    deserialize(is, sz);
    if (sz) {
        str.resize(sz);
        is.read(&str[0], sz);
    }
}

template <typename T>
void serialize(std::ostream &os, const vector<T> &vs) {
    serialize(os, vs.size());
    for (const auto &v : vs) {
        serialize(os, v);
    }
}

template <typename T>
void deserialize(std::istream &is, vector<T> &vs) {
    using size_type = typename vector<T>::size_type;
    size_type sz;
    deserialize(is, sz);
    vs.resize(sz);

    for (size_type i = 0; i < sz; i++) {
        deserialize(is, vs[i]);
    }
}

template <typename K, typename V>
void serialize(std::ostream &os, const map<K, V> &m) {
    serialize(os, m.size());
    for (const auto &[k, v] : m) {
        serialize(os, k);
        serialize(os, v);
    }
}

template <typename K, typename V>
void deserialize(std::istream &is, map<K, V> &m) {
    using size_type = typename map<K, V>::size_type;
    size_type sz;
    deserialize(is, sz);

    for (size_type i = 0; i < sz; i++) {
        K k;
        V v;
        deserialize(is, k);
        deserialize(is, v);
        m.emplace(std::move(k), std::move(v));
    }
}

inline void serialize(std::ostream &os, const type_id &id) {
    // always go through mangled_type_name to support different uses of type
    // index impl
    if (id == type_id::none()) {
        serialize(os, type_id::none().value());
    } else {
        serialize(os, type_id::from(id->type_name).value());
    }
}

inline void deserialize(std::istream &is, type_id &id) {
    type_id::internal i;
    deserialize(is, i);
    id = type_id::from(i);
}

inline void serialize(std::ostream &os, const qualified_type_info &info) {
    serialize(os, info.id);
    serialize(os, info.is_const);
    serialize(os, info.is_volatile);
}

inline void deserialize(std::istream &is, qualified_type_info &info) {
    deserialize(is, info.id);
    deserialize(is, info.is_const);
    deserialize(is, info.is_volatile);
}

inline void serialize(std::ostream &os, const struct_base_type_info &info) {
    serialize(os, info.parent_id);
    serialize(os, info.id);
    serialize(os, info.access);
    serialize(os, info.is_virtual);
    serialize(os, info.is_primary);
    serialize(os, info.is_vbase);
    serialize(os, info.offset);
    serialize(os, info.dyncast_up_index);
    serialize(os, info.dyncast_down_index);
}

inline void deserialize(std::istream &is, struct_base_type_info &info) {
    deserialize(is, info.parent_id);
    deserialize(is, info.id);
    deserialize(is, info.access);
    deserialize(is, info.is_virtual);
    deserialize(is, info.is_primary);
    deserialize(is, info.is_vbase);
    deserialize(is, info.offset);
    deserialize(is, info.dyncast_up_index);
    deserialize(is, info.dyncast_down_index);
}

inline void serialize(std::ostream &os, const field_type_info &info) {
    serialize(os, info.parent_id);
    serialize(os, info.type);
    serialize(os, info.name);
    serialize(os, info.offset);
    serialize(os, info.size);
    serialize(os, info.access);
    serialize(os, info.is_mutable);
    serialize(os, info.is_bit_field);
    serialize(os, info.bit_size);
    serialize(os, info.bit_offset);
    serialize(os, info.annotations);
}

inline void deserialize(std::istream &is, field_type_info &info) {
    deserialize(is, info.parent_id);
    deserialize(is, info.type);
    deserialize(is, info.name);
    deserialize(is, info.offset);
    deserialize(is, info.size);
    deserialize(is, info.access);
    deserialize(is, info.is_mutable);
    deserialize(is, info.is_bit_field);
    deserialize(is, info.bit_size);
    deserialize(is, info.bit_offset);
    deserialize(is, info.annotations);
}

inline void serialize(std::ostream &os, const static_field_type_info &info) {
    serialize(os, info.parent_id);
    serialize(os, info.type);
    serialize(os, info.name);
    serialize(os, info.access);
    serialize(os, info.is_constexpr);
    serialize(os, info.constexpr_value_index);
    serialize(os, info.annotations);
}

inline void deserialize(std::istream &is, static_field_type_info &info) {
    deserialize(is, info.parent_id);
    deserialize(is, info.type);
    deserialize(is, info.name);
    deserialize(is, info.access);
    deserialize(is, info.is_constexpr);
    deserialize(is, info.constexpr_value_index);
    deserialize(is, info.annotations);
}

inline void serialize(std::ostream &os, const function_parameter_info &info) {
    serialize(os, info.name);
    serialize(os, info.index);
    serialize(os, info.is_defaulted);
}

inline void deserialize(std::istream &is, function_parameter_info &info) {
    deserialize(is, info.name);
    deserialize(is, info.index);
    deserialize(is, info.is_defaulted);
}

inline void serialize(std::ostream &os, const function_type_info &info) {
    serialize(os, info.invoker_index);
    serialize(os, info.parent_id);
    serialize(os, info.id);
    serialize(os, info.qualified_name);
    serialize(os, info.name);
    serialize(os, info.is_member);
    serialize(os, info.is_static);
    serialize(os, info.is_ctor);
    serialize(os, info.is_dtor);
    serialize(os, info.is_converter);
    serialize(os, info.is_explicit);
    serialize(os, info.is_virtual);
    serialize(os, info.is_deleted);
    serialize(os, info.is_defaulted);
    serialize(os, info.parameters);
    serialize(os, info.annotations);
    serialize(os, info.definition_path);
}

inline void deserialize(std::istream &is, function_type_info &info) {
    deserialize(is, info.invoker_index);
    deserialize(is, info.parent_id);
    deserialize(is, info.id);
    deserialize(is, info.qualified_name);
    deserialize(is, info.name);
    deserialize(is, info.is_member);
    deserialize(is, info.is_static);
    deserialize(is, info.is_ctor);
    deserialize(is, info.is_dtor);
    deserialize(is, info.is_converter);
    deserialize(is, info.is_explicit);
    deserialize(is, info.is_virtual);
    deserialize(is, info.is_deleted);
    deserialize(is, info.is_defaulted);
    deserialize(is, info.parameters);
    deserialize(is, info.annotations);
    deserialize(is, info.definition_path);
}

inline void serialize(std::ostream &os, const function_overload_set &info) {
    serialize(os, info.qualified_name);
    serialize(os, info.name);
    serialize(os, info.functions);
}

inline void deserialize(std::istream &is, function_overload_set &info) {
    deserialize(is, info.qualified_name);
    deserialize(is, info.name);
    deserialize(is, info.functions);
}

inline void serialize(std::ostream &os, const template_parameter_info &info) {
    serialize(os, info.name);
    serialize(os, info.type);
    serialize(os, info.is_typename);
    serialize(os, info.value_index);
}

inline void deserialize(std::istream &is, template_parameter_info &info) {
    deserialize(is, info.name);
    deserialize(is, info.type);
    deserialize(is, info.is_typename);
    deserialize(is, info.value_index);
}

inline void serialize(std::ostream &os, const type_info &info) {
    serialize(os, info.id);
    serialize(os, info.type_id_hash_index);
    serialize(os, info.kind);
    serialize(os, info.type_name);
    serialize(os, info.mangled_type_name);
    serialize(os, info.type);
    serialize(os, info.size);
    serialize(os, info.align);
    serialize(os, info.annotations);
    serialize(os, info.definition_path);

    if (info.kind == STRUCT || info.kind == UNION) {
        serialize(os, info.record.qualified_name);
        serialize(os, info.record.template_parameters);
        serialize(os, info.record.bases);
        serialize(os, info.record.typedefs);
        serialize(os, info.record.fields);
        serialize(os, info.record.static_fields);
        serialize(os, info.record.functions);
        serialize(os, info.record.is_pod);
        serialize(os, info.record.has_trivial_default_ctor);
        serialize(os, info.record.has_trivial_copy_ctor);
        serialize(os, info.record.has_trivial_copy_assign);
        serialize(os, info.record.has_trivial_move_ctor);
        serialize(os, info.record.has_trivial_move_assign);
        serialize(os, info.record.has_trivial_dtor);
        serialize(os, info.record.is_trivially_copyable);
        serialize(os, info.record.is_abstract);
        serialize(os, info.record.is_polymorphic);
        serialize(os, info.record.is_dynamic);
        serialize(os, info.record.size);
        serialize(os, info.record.alignment);
    } else if (info.kind == FUNC) {
        serialize(os, info.function.return_type);
        serialize(os, info.function.parameters);
        serialize(os, info.function.is_const);
        serialize(os, info.function.is_rref);
        serialize(os, info.function.is_ref);
    } else if (info.kind == ARRAY) {
        serialize(os, info.array.length);
    } else if (info.kind == ENUM) {
        serialize(os, info.enum_.base_type);
        serialize(os, info.enum_.name_to_value);
        serialize(os, info.enum_.value_to_name);
    } else if (info.kind == MEMBER_PTR) {
        serialize(os, info.member_ptr.class_type);
    }
}

inline void deserialize(std::istream &is, type_info &info) {
    deserialize(is, info.id);
    deserialize(is, info.type_id_hash_index);
    deserialize(is, info.kind);
    deserialize(is, info.type_name);
    deserialize(is, info.mangled_type_name);
    deserialize(is, info.type);
    deserialize(is, info.size);
    deserialize(is, info.align);
    deserialize(is, info.annotations);
    deserialize(is, info.definition_path);

    if (info.kind == STRUCT || info.kind == UNION) {
        deserialize(is, info.record.qualified_name);
        deserialize(is, info.record.template_parameters);
        deserialize(is, info.record.bases);
        deserialize(is, info.record.typedefs);
        deserialize(is, info.record.fields);
        deserialize(is, info.record.static_fields);
        deserialize(is, info.record.functions);
        deserialize(is, info.record.is_pod);
        deserialize(is, info.record.has_trivial_default_ctor);
        deserialize(is, info.record.has_trivial_copy_ctor);
        deserialize(is, info.record.has_trivial_copy_assign);
        deserialize(is, info.record.has_trivial_move_ctor);
        deserialize(is, info.record.has_trivial_move_assign);
        deserialize(is, info.record.has_trivial_dtor);
        deserialize(is, info.record.is_trivially_copyable);
        deserialize(is, info.record.is_abstract);
        deserialize(is, info.record.is_polymorphic);
        deserialize(is, info.record.is_dynamic);
        deserialize(is, info.record.size);
        deserialize(is, info.record.alignment);
    } else if (info.kind == FUNC) {
        deserialize(is, info.function.return_type);
        deserialize(is, info.function.parameters);
        deserialize(is, info.function.is_const);
        deserialize(is, info.function.is_rref);
        deserialize(is, info.function.is_ref);
    } else if (info.kind == ARRAY) {
        deserialize(is, info.array.length);
    } else if (info.kind == ENUM) {
        deserialize(is, info.enum_.base_type);
        deserialize(is, info.enum_.name_to_value);
        deserialize(is, info.enum_.value_to_name);
    } else if (info.kind == MEMBER_PTR) {
        deserialize(is, info.member_ptr.class_type);
    }
}

inline void serialize(std::ostream &os, const typedef_info &info) {
    serialize(os, info.name);
    serialize(os, info.aliased_type);
}

inline void deserialize(std::istream &is, typedef_info &info) {
    deserialize(is, info.name);
    deserialize(is, info.aliased_type);
}

inline void serialize(std::ostream &os, const namespace_alias_info &info) {
    serialize(os, info.name);
    serialize(os, info.aliased);
}

inline void deserialize(std::istream &is, namespace_alias_info &info) {
    deserialize(is, info.name);
    deserialize(is, info.aliased);
}

inline void serialize(std::ostream &os, const namespace_using_info &info) {
    serialize(os, info.containing);
    serialize(os, info.used);
}

inline void deserialize(std::istream &is, namespace_using_info &info) {
    deserialize(is, info.containing);
    deserialize(is, info.used);
}
}
} // namespace archimedes

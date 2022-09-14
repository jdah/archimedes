#pragma once

#include "archimedes/basic.hpp"

// archimedes can be enabled/disabled per translation unit with
// ARCHIMEDES_{ENABLE/DISABLE}
#define ARCHIMEDES_ENABLE() ARCHIMEDES_ARG("enable")
#define ARCHIMEDES_DISABLE() ARCHIMEDES_ARG("disable")

// include/exclude namespaces per translation unit
#define ARCHIMEDES_INCLUDE_NS(_ns) \
    (ARCHIMEDES_ARG("include-ns-" ARCHIMEDES_STRINGIFY(_ns)))

#define ARCHIMEDES_EXCLUDE_NS(_ns) \
    (ARCHIMEDES_ARG("exclude-ns-" ARCHIMEDES_STRINGIFY(_ns)))

#define _ARCHIMEDES_FORCE_FUNCTION_INSTANTIATION_IMPL(_f, _n)                  \
    struct _n { _n(const void *p) {                                            \
        std::string _ = fmt::format("{}", fmt::ptr(p)); } };                   \
    auto CONCAT(_fi_, __COUNTER__) = _n(reinterpret_cast<const void*>(&(_f)));

#define ARCHIMEDES_FORCE_FUNCTION_INSTANTIATION(_f)                            \
    _ARCHIMEDES_FORCE_FUNCTION_INSTANTIATION_IMPL(                             \
        _f, CONCAT(_fi_t_, __COUNTER__))

#define _ARCHIMEDES_FORCE_TYPE_INSTANTIATION_IMPL(_n, ...) \
    struct _n {                                            \
        __VA_ARGS__ t;                                     \
        void *p; _n() : p(foo()) {}                        \
        void *foo() { return &t; }};                       \
    auto CONCAT(_ti_, __COUNTER__) = _n();

#define ARCHIMEDES_FORCE_TYPE_INSTANTIATION(...) \
    _ARCHIMEDES_FORCE_TYPE_INSTANTIATION_IMPL(   \
        CONCAT(_ti_t_, __COUNTER__),             \
        __VA_ARGS__)

// extern'd templates for compile performance
#include "archimedes/ds.hpp"
extern template class
    std::vector<std::string>;

#include "archimedes/type_id.hpp"
#include "archimedes/any.hpp"
#include "archimedes/errors.hpp"
#include "archimedes/type_kind.hpp"
#include "archimedes/access_specifier.hpp"
#include "archimedes/types.hpp"
#include "archimedes/registry.hpp"
#include "archimedes/cast.hpp"

namespace archimedes {
// load type data if not already loaded
inline void load() {
    detail::registry::instance().load();
}

// returns true if archimedes is loaded
inline bool loaded() {
    return detail::registry::instance().loaded();
}

// set the type collision callback
// a function which accepts two reflected_types and chooses between them
// which type the type name should resolve to
inline void set_collision_callback(collision_callback &&tcc) {
    detail::registry::instance()
        .set_collision_callback(std::move(tcc));
}

// get reflected type by annotations
inline vector<reflected_type> reflect_by_annotations(
    std::span<const std::string_view> annotations) {
    return detail::transform<vector<reflected_type>>(
        detail::registry::instance().find_by_annotations(annotations),
        [](const auto *info) {
            return reflected_type(info);
        });
}

// returns all registered types
inline vector<reflected_type> types() {
    return detail::transform<vector<reflected_type>>(
        detail::registry::instance().all_types(),
        [](const detail::type_info *info) {
            return reflected_type(info);
        });
}

// get all children of some base class
inline vector<reflected_record_type> reflect_children(
    reflected_record_type rec) {
    return
        detail::transform<vector<reflected_record_type>>(
            detail::filter<vector<reflected_type>>(
                types(),
                [&](const reflected_type &type) {
                    return
                        type.is_record()
                        && type.as_record().in_hierarchy(rec);
                }),
            [](const reflected_type &type) {
                return type.as_record();
            });
}

// get a reflected type by id
inline std::optional<reflected_type> reflect(type_id id) {
    const auto i = detail::registry::instance().type_from_id(id);
    return i ? std::make_optional(reflected_type(*i)) : std::nullopt;
}

// reflect a record (class/struct/union) type
template <typename T>
    requires std::is_class_v<T> || std::is_union_v<T>
inline std::optional<reflected_record_type> reflect() {
    const auto opt = reflect(type_id::from<T>());
    return opt ? std::make_optional(opt->as_record()) : std::nullopt;
}

// reflect an enum type
template <typename T>
    requires std::is_enum_v<T>
inline std::optional<reflected_enum_type> reflect() {
    const auto opt = reflect(type_id::from<T>());
    return opt ? std::make_optional(opt->as_enum()) : std::nullopt;
}

// reflect function type
template <typename T>
    requires std::is_function_v<T>
inline std::optional<reflected_function_type> reflect() {
    const auto opt = reflect(type_id::from<T>());
    return opt ? std::make_optional(opt->as_function()) : std::nullopt;
}

// reflect array type
template <typename T>
    requires std::is_array_v<T>
inline std::optional<reflected_array_type> reflect() {
    const auto opt = reflect(type_id::from<T>());
    return opt ? std::make_optional(opt->as_array()) : std::nullopt;
}

// reflect member ptr type
template <typename T>
    requires std::is_member_pointer_v<T>
inline std::optional<reflected_member_ptr_type> reflect() {
    const auto opt = reflect(type_id::from<T>());
    return opt ? std::make_optional(opt->as_member_ptr()) : std::nullopt;
}

// reflect other types
template <typename T>
    requires (
        !std::is_class_v<T>
        && !std::is_union_v<T>
        && !std::is_enum_v<T>
        && !std::is_function_v<T>
        && !std::is_array_v<T>
        && !std::is_member_pointer_v<T>)
inline std::optional<reflected_type> reflect() {
    return reflect(type_id::from<T>());
}

// reflect a type by name
inline std::optional<reflected_type> reflect(std::string_view name) {
    const auto i = detail::registry::instance().type_from_name(name);
    return i ? std::make_optional(reflected_type(*i)) : std::nullopt;
}

// reflect a function set
inline vector<reflected_function_set> reflect_functions(
    std::string_view name) {
    const auto info_opt = detail::registry::instance().function_by_name(name);
    if (!info_opt) {
        return {};
    }

    vector<reflected_function_set> fs;
    for (const auto &fos : *(*info_opt)) {
        fs.push_back(reflected_function_set(&fos));
    }
    return fs;
}

// reflect a list of functions by type
template <typename T>
std::optional<vector<reflected_function>> reflect_functions()  {
    auto vec_opt =
        detail::registry::instance().function_by_type(type_id::from<T>());

    if (!vec_opt) {
        return std::nullopt;
    }

    return detail::transform<vector<reflected_function>>(
        **vec_opt,
        [](const detail::function_type_info &p) {
            return reflected_function(&p);
        });
}

// reflect a single function, returning the first found if it resolves to
// multiple functions
inline std::optional<reflected_function> reflect_function(
    std::string_view name) {
    const auto fs_opt = reflect_functions(name);
    return fs_opt.empty() ? std::nullopt : fs_opt.begin()->first();
}

// reflect a field by qualified name
// TODO: a little broken, field name lookup is by regex...
inline std::optional<reflected_field> reflect_field(
    std::string_view name) {
    auto pos_qual = name.find("::");
    if (pos_qual == std::string::npos) {
        return std::nullopt;
    }

    // split into type name, field name
    const auto
        type_name = name.substr(0, pos_qual),
        field_name = name.substr(pos_qual + 2);

    if (type_name.empty() || field_name.empty()) {
        return std::nullopt;
    }

    const auto rec_opt = reflect(type_name);
    if (!rec_opt || !rec_opt->is_record()) {
        return std::nullopt;
    }

    return rec_opt->as_record().field(field_name);
}

// TODO: doc
inline std::optional<reflected_type> reflect_by_typeid(const std::type_info &ti) {
    const auto opt =
        detail::registry::instance().type_from_type_id_hash(ti.hash_code());
    return opt ? std::make_optional(reflected_type(*opt)) : std::nullopt;
}
} // namespace archimedes

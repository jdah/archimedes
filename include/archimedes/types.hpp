#pragma once

#include <string_view>
#include <string>
#include <bit>

#include "type_id.hpp"
#include "any.hpp"
#include "errors.hpp"
#include "type_kind.hpp"
#include "access_specifier.hpp"
#include "type_info.hpp"
#include "invoke.hpp"
#include "cast.hpp"
#include "regex.hpp"
#include "basic.hpp"
#include "ds.hpp"

// TODO: remove
#include <iostream>
#include "../lib/nameof/include/nameof.hpp"

namespace archimedes {
// wrapper for some reference for invoke()-ing functions
struct ref_wrapper {
    void *ptr;
    type_id id;
};

// utility function to ensure invoke argument is passed as const ref
template <typename T>
inline auto cref(const T &t) {
    return ref_wrapper {
        const_cast<void*>(reinterpret_cast<const void *>(&t)),
        type_id::from<const T&>()
    };
}

// utility function to ensure invoke argument is passed as ref
template <typename T>
inline auto ref(T &t) { return ref_wrapper { &t, type_id::from<T&>() }; }
//
// utility function to ensure invoke argument is passed as rref
template <typename T>
inline auto rref(T &&t) { return ref_wrapper { &t, type_id::from<T&&>() }; }

namespace detail {
    struct registry;
}

struct reflected_type;
struct reflected_record_type;
struct reflected_field;
struct reflected_function_type;
struct reflected_array_type;
struct reflected_enum_type;
struct reflected_member_ptr_type;
struct qualified_reflected_type;

// reflected base class of some record type
struct reflected_base {
    reflected_base() = default;
    explicit reflected_base(const detail::struct_base_type_info *info)
        : info(info) {}

    bool operator==(const reflected_base &rhs) const {
        return this->info == rhs.info;
    }

    // type of which this is a base
    reflected_record_type parent() const;

    // type of base itself
    reflected_record_type type() const;

    // base access specifier
    AccessSpecifier access() const {
        return this->info->access;
    }

    // true if is virtual base
    bool is_virtual() const {
        return this->info->is_virtual;
    }

    // true if is the primary base
    bool is_primary() const {
        return this->info->is_primary;
    }

    // true if is a vbase (virtual base with storage on this class)
    bool is_vbase() const {
        return this->info->is_vbase;
    }

    // offset of storage for this base class into parent class
    size_t offset() const {
        return this->info->offset;
    }

    // cast from parent to this base
    void *cast_up(void *parent) const {
         if (!this->info->dyncast_up) {
            return reinterpret_cast<void*>(
                reinterpret_cast<char*>(parent)
                    + this->offset());
         } else {
            return this->info->dyncast_up(parent);
         }
    }

    // cast from this base to parent
    void *cast_down(void *base) const {
         if (!this->info->dyncast_down) {
            return reinterpret_cast<void*>(
                reinterpret_cast<char*>(base)
                    - this->offset());
         } else {
            return this->info->dyncast_down(base);
         }
    }

private:
    const detail::struct_base_type_info *info;
};

// reflected static field on some record type
struct reflected_static_field {
    reflected_static_field() = default;
    explicit reflected_static_field(const detail::static_field_type_info *info)
        : info(info) {}

    bool operator==(const reflected_static_field &rhs) const {
        return this->info == rhs.info;
    }

    reflected_record_type parent() const;

    qualified_reflected_type type() const;

    std::string_view name() const {
        return this->info->name;
    }

    AccessSpecifier access() const {
        return this->info->access;
    }

    bool is_constexpr() const {
        return this->info->is_constexpr;
    }

    result<const any*, constexpr_error> constexpr_value() const {
        return result<const any*, constexpr_error>::make(
            this->is_constexpr()
                && this->info->constexpr_value_index != detail::NO_ARRAY_INDEX,
            &this->info->constexpr_value,
            constexpr_error::IS_NOT_CONSTEXPR);
    }

    auto annotations() const {
        return detail::transform<vector<std::string_view>>(
            this->info->annotations,
            [](const auto &s) { return std::string_view(s); });
    }

private:
    const detail::static_field_type_info *info;
};

// reflected parameter for some function declaration
struct reflected_parameter {
    reflected_parameter() = default;
    reflected_parameter(
        const detail::function_parameter_info *info,
        const detail::function_type_info *function_info)
        : info(info),
          function_info(function_info) {}

    bool operator==(const reflected_parameter &rhs) const {
        return this->info == rhs.info;
    }

    // qualified type of parameter
    qualified_reflected_type type() const;

    // parameter name
    std::string_view name() const {
        return this->info->name;
    }

    // parameter index
    size_t index() const {
        return this->info->index;
    }

    // true if parameter has default value
    bool is_defaulted() const {
        return this->info->is_defaulted;
    }

private:
    const detail::function_parameter_info *info;
    const detail::function_type_info *function_info;
};

// reflected function declaration
struct reflected_function {
    reflected_function() = default;
    explicit reflected_function(const detail::function_type_info *info)
        : info(info) {}

    // NOTE: can't compare on *info because functions are copied in registry
    bool operator==(const reflected_function &rhs) const {
        return this->info->id == rhs.info->id
            && this->info->parent_id == rhs.info->parent_id
            && this->info->qualified_name == rhs.info->qualified_name;
    }

    // parent of this function if present (function is a method)
    std::optional<reflected_record_type> parent() const;

    // function type
    reflected_function_type type() const;

    // function name
    std::string_view name() const {
        return this->info->name;
    }

    // qualified function name
    std::string_view qualified_name() const {
        return this->info->qualified_name;
    }

    // path of file in which function is defined
    std::string_view definition_path() const {
        return this->info->definition_path;
    }

    // true if function is member
    bool is_member() const {
        return this->info->is_member;
    }

    // true if function is static
    bool is_static() const {
        return this->info->is_static;
    }

    // true if function is ctor
    bool is_ctor() const {
        return this->info->is_ctor;
    }

    // true if function is dtor
    bool is_dtor() const {
        return this->info->is_dtor;
    }

    // true if function is converter
    bool is_converter() const {
        return this->info->is_converter;
    }

    // true if function is explicit
    bool is_explicit() const {
        return this->info->is_explicit;
    }

    // true if function is virtual
    bool is_virtual() const {
        return this->info->is_virtual;
    }

    // true if function is deleted
    bool is_deleted() const {
        return this->info->is_deleted;
    }

    // true if function is defaulted
    bool is_defaulted() const {
        return this->info->is_defaulted;
    }

    // parameters for function
    auto parameters() const {
        return detail::transform<vector<reflected_parameter>>(
            this->info->parameters,
            [this](const auto &p) {
                return reflected_parameter(&p, this->info);
            });
    }

    // parameter by regex
    std::optional<reflected_parameter> parameter(std::string_view regex) const {
        return detail::find(
            this->parameters(),
            [&](const auto &p) {
                return regex_matches(regex, p.name());
            });
    }

    // annotations on function
    auto annotations() const {
        return detail::transform<vector<std::string_view>>(
            this->info->annotations,
            [](const auto &s) { return std::string_view(s); });
    }

    // returns true if function can be invoked
    bool can_invoke() const {
        return this->info->invoker;
    }

    // attempt to invoke function with specified arguments
    template <typename ...Ts>
    archimedes::invoke_result invoke(Ts&& ...ts) const {
        if (!this->info->invoker) {
            return archimedes::invoke_result::NO_ACCESS;
        }

        std::vector<archimedes::any> args;
        args.reserve(sizeof...(ts));

        // TODO: emplace_back
        ([&](auto &&t) {
            using T = decltype(t);
            if constexpr (std::is_same_v<std::decay_t<T>, ref_wrapper>) {
                args.push_back(
                    archimedes::any::make_reference_from_ptr(
                        t.ptr,
                        t.id));
            } else {
                args.push_back(
                    archimedes::any::make(std::forward<T>(t)));
            }
         }(ts), ...);

        return this->info->invoker(std::span { args });
    }

    // attempt to invoke function with specified arguments
    archimedes::invoke_result invoke_with(
        std::span<archimedes::any, std::dynamic_extent> args) const {
        if (!this->info->invoker) {
            return archimedes::invoke_result::NO_ACCESS;
        }

        return this->info->invoker(args);
    }

    // attempt to invoke function with specified arguments
    archimedes::invoke_result invoke_with(
        std::span<const archimedes::any> args) const {
        return this->invoke_with(
            std::span<archimedes::any>(
                const_cast<archimedes::any*>(&args[0]),
                args.size()));
    }

//private:
    const detail::function_type_info *info;
};

// reflected type
struct reflected_type {
    reflected_type() = default;
    explicit reflected_type(const detail::type_info *info)
        : info(info) {}

    bool operator==(const reflected_type &rhs) const {
        return this->id() == rhs.id();
    }

    auto operator<=>(const reflected_type &rhs) const {
        return this->id() <=> rhs.id();
    }

    // unique index of type
    type_id id() const {
        return this->info->id;
    }

    // kind of type
    type_kind kind() const {
        return this->info->kind;
    }

    // typeid(...).hash_code() for this type if present
    std::optional<size_t> type_id_hash() const {
        return this->info->type_id_hash ?
            std::make_optional(this->info->type_id_hash)
            : std::nullopt;
    }

    // name of type
    std::string_view name() const {
        return this->info->type_name;
    }

    // mangled name of type
    std::string_view mangled_name() const {
        return this->info->mangled_type_name;
    }

    // pointed-to/referenced type for references/pointers/arrays/member pointers
    std::optional<qualified_reflected_type> type() const;

    // size of type, 0 if type is size-less
    size_t size() const {
        return this->info->size;
    }

    // align of type, 0 if type is align-less
    size_t align() const {
        return this->info->align;
    }

    // annotations on type
    auto annotations() const {
        return detail::transform<vector<std::string_view>>(
            this->info->annotations,
            [](const auto &s) { return std::string_view(s); });
    }

    // path of file in which record is defined
    std::string_view definition_path() const {
        return this->info->definition_path;
    }

    // returns true if numeric type (bool/integer/float/emum)
    bool is_numeric() const {
        switch (this->kind()) {
            case BOOL:
            case CHAR:
            case U_CHAR:
            case U_SHORT:
            case U_INT:
            case U_LONG:
            case U_LONG_LONG:
            case I_CHAR:
            case I_SHORT:
            case I_INT:
            case I_LONG:
            case I_LONG_LONG:
            case FLOAT:
            case DOUBLE:
            case ENUM:
                return true;
            default:
                return false;
        }
    }

    // returns true if struct/union type
    bool is_record() const {
        switch (this->kind()) {
            case STRUCT:
            case UNION:
                return true;
            default:
                return false;
        }
    }

    // returns this type as a record type, crashes or throws if !is_record()
    reflected_record_type as_record() const;

    // returns true if function type
    bool is_function() const {
        return this->kind() == FUNC;
    }

    // returns this type as a function type, crashes or throws if !is_function()
    reflected_function_type as_function() const;

    // returns true if array type
    bool is_array() const {
        return this->kind() == ARRAY;
    }

    // returns this type as an array type, crashes or throws if !is_array()
    reflected_array_type as_array() const;

    // returns true if array type
    bool is_enum() const {
        return this->kind() == ENUM;
    }

    // returns this type as an enum type, crashes or throws if !is_enum()
    reflected_enum_type as_enum() const;

    // returns true if member pointer type
    bool is_member_ptr() const {
        return this->kind() == MEMBER_PTR;
    }

    // returns this type as an member ptr type, crashes or throws if
    // !is_member_ptr()
    reflected_member_ptr_type as_member_ptr() const;

protected:
    friend struct detail::registry;
    const detail::type_info *info;
};

// reflected type wrapper which can have qualifiers
struct qualified_reflected_type {
    qualified_reflected_type() = default;
    explicit qualified_reflected_type(const detail::qualified_type_info *info)
        : rtype(&*info->id),
          info(info) {}

    bool operator==(const qualified_reflected_type &rhs) const {
        return this->id() == rhs.id()
            && this->is_const() == rhs.is_const()
            && this->is_volatile() == rhs.is_volatile();
    }

    // internal type of qualified type
    reflected_type type() const {
        return this->rtype;
    }

    // dereference to internal type
    reflected_type operator*() const {
        return this->rtype;
    }

    // pointer to internal type
    const reflected_type *operator->() const {
        return &this->rtype;
    }

    // unique id of type
    type_id id() const {
        return this->info->id;
    }

    // true if type is "const" qualified
    bool is_const() const {
        return this->info->is_const;
    }

    // true if type is "volatile" qualified
    bool is_volatile() const {
        return this->info->is_const;
    }

private:
    reflected_type rtype;
    const detail::qualified_type_info *info;
};

// reflected typedef
struct reflected_type_alias {
    explicit reflected_type_alias(const detail::typedef_info *info)
        : info(info) {}

    // name of type alias
    std::string_view name() const {
        return this->info->name;
    }

    qualified_reflected_type type() const {
        return qualified_reflected_type(&this->info->aliased_type);
    }

private:
    const detail::typedef_info *info;
};

// reflected_type with kind FUNC
struct reflected_function_type : public reflected_type {
    using reflected_type::reflected_type;

    // list of parameter types
    auto parameters() const {
        return detail::transform<vector<qualified_reflected_type>>(
            this->info->function.parameters,
            [](const auto &p) {
                return qualified_reflected_type(&p); });
    }

    // function return type
    qualified_reflected_type return_type() const {
        return qualified_reflected_type(
            &this->info->function.return_type);
    }

    // true if function is const qualified
    bool is_const() const {
        return this->info->function.is_const;
    }

    // true if function is rvalue-reference qualified
    bool is_rref() const {
        return this->info->function.is_rref;
    }

    // true if function is lvalue-reference qualified
    bool is_ref() const {
        return this->info->function.is_ref;
    }
};

// reflection over a set of overloaded functions
struct reflected_function_set {
    using value_type = reflected_function;

    reflected_function_set() = default;

    explicit reflected_function_set(const detail::function_overload_set *set)
        : set(set) {}

    bool operator==(const reflected_function_set &rhs) const {
        return this->set == rhs.set;
    }

    // function set name
    std::string_view name() const {
        return this->set->name;
    }

    // function set quaified name
    std::string_view qualified_name() const {
        return this->set->qualified_name;
    }

    // iterable
    auto begin() const {
        return detail::make_transformed_it(
            this->set->functions,
            reflected_function_set::transform);
    }

    auto end() const {
      return detail::make_transformed_end(
            this->set->functions,
            reflected_function_set::transform);
    }

    // number of functions in set
    size_t size() const {
        return this->set->functions.size();
    }

    // true if 0 functions in this set
    bool empty() const {
        return this->size() == 0;
    }

    // nth function in set
    template <typename T>
    auto operator[](const T &t) const {
        return reflected_function(this->set->functions[t]);
    }

    // get function by type
    template <typename T>
    std::optional<reflected_function> get() const {
        const auto i = type_id::from<T>();
        auto it =
            std::find_if(
                this->begin(),
                this->end(),
                [&](const reflected_function &f) {
                    return f.type().id() == i;
                });
        return it == this->end() ?
            std::nullopt
            : std::make_optional(*it);
    }

    // first function in list of functions, if present
    std::optional<reflected_function> first() const {
        if (this->empty()) {
            return std::nullopt;
        }
        return *this->begin();
    }

    // list of functions
    auto functions() const {
        return detail::iterable(
            this->begin(),
            this->end());
    }

private:
    static inline reflected_function transform(
        const detail::function_type_info &info) {
        return reflected_function(&info);
    }

    const detail::function_overload_set *set;
};

// reflected template parameter type
struct reflected_template_parameter {
    reflected_template_parameter() = default;
    reflected_template_parameter(const detail::template_parameter_info *info)
        : info(info) {}

    // name of template parameter
    std::string_view name() const {
        return this->info->name;
    }

    // type of parameter (literal type if typename, otherwise type of value)
    qualified_reflected_type type() const {
        return qualified_reflected_type(&this->info->type);
    }

    // true if parameter names a type (typename/class/etc.)
    bool is_typename() const {
        return this->info->is_typename;
    }

    // value if parameter is not typename
    result<const any*, template_parameter_error> value() const {
        if (this->info->is_typename) {
            return template_parameter_error::NO_VALUE;
        }

        return &this->info->value;
    }

private:
    const detail::template_parameter_info *info;
};


// reflected_type with kind STRUCT or UNION
struct reflected_record_type : public reflected_type {
    using reflected_type::reflected_type;

    // name of declaration, entirely unqualified and without template parameters
    std::string declaration_name() const {
        auto name = std::string(this->name());

        // remove template
        const auto l_angle = name.find('<');
        if (l_angle != std::string::npos) {
            name = name.substr(0, l_angle);
        }

        // remove namespace
        const auto ns = name.rfind(':');
        if (ns != std::string::npos) {
            name = name.substr(ns + 1);
        }

        return name;
    }

    // get qualfied name of record type
    std::string_view qualified_name() const {
        return this->info->record.qualified_name;
    }

    // list of template paramters
    auto template_parameters() const {
        return detail::transform<vector<reflected_template_parameter>>(
            this->info->record.template_parameters,
            [](const auto &p) {
                return reflected_template_parameter(&p);
            });
    }

    // list of typedef-s/using-s/"type aliases"
    auto type_aliases() const {
        return detail::transform<vector<reflected_type_alias>>(
            detail::values(this->info->record.typedefs),
            [](const detail::typedef_info &p) {
                return reflected_type_alias(&p);
            });
    }

    // get typedef/using/"type alias" by regex
    std::optional<reflected_type_alias> type_alias(std::string_view regex) const {
        // use prefix as typedefs are always done by qualified name
        const auto prefix_regex =
             escape_for_regex(this->qualified_name())
                + "::" + std::string(regex);
        return detail::find(
            this->type_aliases(),
            [&](const auto &p) {
                return regex_matches(regex, p.name())
                    || regex_matches(prefix_regex, p.name());
            });
    }

    // get template parameter by regex
    std::optional<reflected_template_parameter> template_parameter(
        std::string_view regex) const {
        return detail::find(
            this->template_parameters(),
            [&](const auto &p) {
                return regex_matches(regex, p.name());
            });
    }

    // list of bases
    // NOTE: does not include vbases (virtual ancestors which have storage on
    // this class)
    auto bases() const {
        return detail::transform_if<vector<reflected_base>>(
            this->info->record.bases,
            [](const auto &p) {
                return !p.is_vbase;
            },
            [](const auto &p) {
                return reflected_base(&p);
            });
    }

    // base by regex on qualified type name (does not include vbases)
    std::optional<reflected_base> base(std::string_view regex) const {
        return detail::find(
            this->bases(),
            [&](const auto &b) {
                return regex_matches(regex, b.type().name());
            });
    }

    // list of vbases
    // NOTE: does not include regular bases
    auto vbases() const {
        return detail::transform_if<vector<reflected_base>>(
            this->info->record.bases,
            [](const auto &p) {
                return p.is_vbase;
            },
            [](const auto &p) {
                return reflected_base(&p);
            });
    }

    // vbase by regex on qualified type name (does not include regular bases)
    std::optional<reflected_base> vbase(std::string_view regex) const {
        return detail::find(
            this->vbases(),
            [&](const auto &b) {
                return regex_matches(regex, b.type().name());
            });
    }

    // list of bases which are inherited virtually (not *necessarily* the same
    // as vbases, which are virtual bases with storage on this class)
    auto virtual_bases() const {
        return detail::transform_if<vector<reflected_base>>(
            this->info->record.bases,
            [](const auto &p) {
                return p.is_virtual;
            },
            [](const auto &p) {
                return reflected_base(&p);
            });
    }

    // virtual bases by regex on qualified name
    std::optional<reflected_base> virtual_base(std::string_view regex) const {
        return detail::find(
            this->virtual_bases(),
            [&](const auto &b) {
                return regex_matches(regex, b.type().name());
            });
    }

    // list of bases (bases + vbases)
    auto all_bases() const {
        return detail::transform<vector<reflected_base>>(
            this->info->record.bases,
            [](const auto &p) {
                return reflected_base(&p);
            });
    }

    // depth-first traversal of inheritance tree of this record type,
    // including virtual bases
    // NOTE: virtual bases *will* be traversed multiple times for each class of
    // which they are a parent and/or are stored on
    // optional "ptr" which will be dynamically casted to base types as the
    // hierarchy is traversed
    template <typename F, typename T = void>
        requires (
            requires (F f,
                      const reflected_base &base,
                      const archimedes::any &any) {
                { f(base, any) };
            })
    auto traverse_bases(F &&f, const T *ptr = nullptr) const {
        // g is internal traversal function
        std::function<
            void(
                reflected_record_type,
                std::optional<reflected_base>,
                const void*)> g;
        g = [&g, this, f = std::forward<F>(f), start_ptr = ptr](
            reflected_record_type rec,
            std::optional<reflected_base> rec_base,
            const void *ptr) {
            for (const auto &base : rec.all_bases()) {
                const void *ptr_base = nullptr;

                if (start_ptr) {
                    if (base.is_vbase()) {
                        // check if vbase of last vbase base
                        const auto vbases = this->vbases();
                        const auto it =
                            std::find_if(
                                vbases.begin(),
                                vbases.end(),
                                [&base](const reflected_base &vbase) {
                                    return vbase.type() == base.type();
                                });

                        if (it == vbases.end()) {
                            ARCHIMEDES_FAIL(
                                "failed to traverse inheritance hierarchy");
                        }

                        // virtual bases are stored on start_ptr
                        ptr_base =
                            reinterpret_cast<const char*>(start_ptr)
                                + it->offset();
                    } else {
                        // regular base
                        ptr_base =
                            reinterpret_cast<const char*>(ptr)
                                + base.offset();
                    }
                }

                f(base,
                    ptr_base ?
                        any::make(ptr_base, base.type().id())
                        : any::none());
                g(base.type(), base, ptr_base);
            }

        };

        g(*this, std::nullopt, ptr);
    }

    // returns some reflected base with type "rec" if it is in this record's
    // inheritance hierarchy
    std::optional<reflected_base> find_base(
        reflected_record_type rec) const {
        std::optional<reflected_base> result;
        this->traverse_bases(
            [&](const reflected_base &b, const any&) {
                if (result) {
                    return;
                } else if (b.type() == rec) {
                    result = b;
                }
            });
        return result;
    }

    // returns true if "rec" is in this record's inheritance hierarchy
    // also checks if rec == this if include_this is true
    bool in_hierarchy(
        reflected_record_type rec,
        bool include_this = true) const {
        return (include_this && rec == *this)
            || this->find_base(rec).has_value();
    }

    // list of fields
    vector<reflected_field> fields() const;

    // field by name
    std::optional<reflected_field> field(std::string_view regex) const;

    // list of static fields
    auto static_fields() const {
        return detail::transform<vector<reflected_static_field>>(
            detail::values(this->info->record.static_fields),
            [](const auto &p) {
                return reflected_static_field(&p); });
    }

    // static field by regex
    std::optional<reflected_static_field> static_field(
        std::string_view regex) const {
        return detail::find(
            this->static_fields(),
            [&](const auto &f) {
                return regex_matches(regex, f.name());
            });
    }

    // list of function overload
    auto function_sets() const {
        return detail::transform<vector<reflected_function_set>>(
            detail::values(this->info->record.functions),
            [](const detail::function_overload_set &p) {
                return reflected_function_set(&p); });
    }

    // function overload set by regex
    std::optional<reflected_function_set> function_set(
        std::string_view regex) const {
        return detail::find(
            this->function_sets(),
            [&](const auto &f) {
                return regex_matches(regex, f.name());
            });
    }

    // function by regex
    // returns the first function found if multiple functions in set
    std::optional<reflected_function> function(std::string_view regex) const {
        return detail::map_opt(
            this->function_set(regex),
            [](const auto &fs) {
                return *fs.begin();
            });
    }

    // all functions, including overloads
    vector<reflected_function> functions() const {
        vector<reflected_function> v;
        for (const auto &fs : this->function_sets()) {
            detail::collect(fs, v);
        }
        return v;
    }

    // functions by type
    template <typename T>
    vector<reflected_function> functions() const {
        const auto id = type_id::from<T>();
        return detail::collect_if<vector<reflected_function>>(
            this->functions(),
            [&](const auto &f) {
                return f.type().id() == id;
            });
    }

    // first found function by type
    template <typename T>
    std::optional<reflected_function> function() const {
        auto fs = this->functions<T>();
        return fs.empty() ? std::nullopt : std::make_optional(fs[0]);
    }

    // true if record is POD/"plain old data"
    bool is_pod() const {
        return this->info->record.is_pod;
    }

    // true if record has trivial default constructor
    bool has_trivial_default_ctor() const {
        return this->info->record.has_trivial_default_ctor;
    }

    // true if record has trivial copy constructor
    bool has_trivial_copy_ctor() const {
        return this->info->record.has_trivial_copy_ctor;
    }

    // true if record has trivial move assignment operator
    bool has_trivial_copy_assign() const {
        return this->info->record.has_trivial_copy_assign;
    }

    // true if record has trivial move constructor
    bool has_trivial_move_ctor() const {
        return this->info->record.has_trivial_move_ctor;
    }

    // true if record has trivial move assignment operator
    bool has_trivial_move_assign() const {
        return this->info->record.has_trivial_move_assign;
    }

    // true if record has trivial destructor
    bool has_trivial_dtor() const {
        return this->info->record.has_trivial_dtor;
    }

    // true if record is abstract (has a deleted virtual function)
    bool is_abstract() const {
        return this->info->record.is_abstract;
    }

    // true if record is polymorphic (contains or inherits something virtual)
    bool is_polymorphic() const {
        return this->info->record.is_polymorphic;
    }

    // size of record
    size_t size() const {
        return this->info->record.size;
    }

    // alignment of record
    size_t alignment() const {
        return this->info->record.alignment;
    }

    // true if record is dynamic
    // (is polymorphic or has at least one virtual base)
    bool is_dynamic() const {
        return this->info->record.is_dynamic;
    }

    // true if this is a struct (not union)
    bool is_struct() const {
        return this->kind() == STRUCT;
    }

    // true if this is a union (not struct)
    bool is_union() const {
        return this->kind() == UNION;
    }

    // returns set of constructors for this record, nullopt if there are none
    std::optional<reflected_function_set> constructors() const {
        return this->function_set(this->declaration_name());
    }

    // attempt to get the default constructor for this type
    // returns nullopt if default constructor is trivial or does not exist
    std::optional<reflected_function> default_constructor() const {
        return detail::flat_map_opt(
            this->constructors(),
            [&](const reflected_function_set &fs) {
                return detail::find(
                    fs,
                    [](const auto &f) {
                        const auto ps = f.parameters();
                        return ps.size() == 0
                            || detail::all_of(
                                ps,
                                std::mem_fn(
                                    &reflected_parameter::is_defaulted));
                    });
            });
    }

    // attempt to get copy constructor for this type
    std::optional<reflected_function> copy_constructor() const {
        return detail::flat_map_opt(
            this->constructors(),
            [&](const reflected_function_set &fs) {
                return detail::find(
                    fs,
                    [&](const auto &f) {
                        return this->has_copy_parameter(f);
                    });
            });
    }

    // attempt to get move constructor for this type
    std::optional<reflected_function> move_constructor() const {
        return detail::flat_map_opt(
            this->constructors(),
            [&](const reflected_function_set &fs) {
                return detail::find(
                    fs,
                    [&](const auto &f) {
                        return this->has_move_parameter(f);
                    });
            });
    }

    // attempt to get copy assignment operator for this type
    std::optional<reflected_function> copy_assignment() const {
        return detail::flat_map_opt(
            this->function_set("operator="),
            [&](const reflected_function_set &fs) {
                return detail::find(
                    fs,
                    [&](const auto &f) {
                        return this->has_copy_parameter(f);
                    });
            });
    }

    // attempt to get move assignment operator for this type
    std::optional<reflected_function> move_assignment() const {
        return detail::flat_map_opt(
            this->function_set("operator="),
            [&](const reflected_function_set &fs) {
                return detail::find(
                    fs,
                    [&](const auto &f) {
                        return this->has_move_parameter(f);
                    });
            });
    }

    // attempt to destructor for this type
    std::optional<reflected_function> destructor() const {
        return this->function("~" + std::string(this->name()));
    }

private:
    inline bool has_copy_parameter(const reflected_function &f) const {
        // match with any qualifiers
        const auto ps = f.parameters();
        return ps.size() == 1
            && ps[0].type()->kind() == REF
            && **ps[0].type()->type() == *this;
    }

    inline bool has_move_parameter(const reflected_function &f) const {
        const auto ps = f.parameters();
        return ps.size() == 1
            && ps[0].type()->kind() == RREF
            && **ps[0].type()->type() == *this;
    }
};

// reflected field on some record type
struct reflected_field {
    explicit reflected_field(const detail::field_type_info *info)
        : info(info) {}

    bool operator==(const reflected_field &rhs) const {
        return this->info == rhs.info;
    }

    // type on which field is present
    reflected_record_type parent() const;

    // type of field
    qualified_reflected_type type() const;

    // name of field
    std::string_view name() const {
        return this->info->name;
    }

    // qualified name of field including record name
    std::string qualified_name() const {
        return std::string(this->parent().name()) + "::" + this->info->name;
    }

    // offset of field into parent tye
    size_t offset() const {
        return this->info->offset;
    }

    // size of field
    size_t size() const {
        return this->info->size;
    }

    // access specified on field
    AccessSpecifier access() const {
        return this->info->access;
    }

    // true if field is qualified "mutable"
    bool is_mutable() const {
        return this->info->is_mutable;
    }

    // true if bit field
    bool is_bit_field() const {
        return this->info->is_bit_field;
    }

    // if bit field, returns offset IN BITS into parent type where field is
    result<size_t, field_error> bit_offset() const {
        return result<size_t, field_error>::make(
            this->is_bit_field(),
            this->info->bit_offset,
            field_error::IS_NOT_BIT_FIELD);
    }

    // if bit field, returns size of field in bits
    result<size_t, field_error> bit_field_size() const {
        return result<size_t, field_error>::make(
            this->is_bit_field(),
            this->info->bit_size,
            field_error::IS_NOT_BIT_FIELD);
    }

    // annotations on field
    auto annotations() const {
        return detail::transform<vector<std::string_view>>(
            this->info->annotations,
            [](const auto &s) { return std::string_view(s); });
    }

    // get pointer to field as void* from some any pointer to T (const or not)
    result<any, field_error> get_any_ptr_from_any_ptr(const any &ptr) const {
        if (this->is_bit_field()) { return field_error::IS_BIT_FIELD; }
        return any::make<void*>(ptr.as<char*>() + this->offset());
    }

    // get pointer to field as F* from some any pointer to T (const or not)
    template <typename F>
    result<F*, field_error> get_ptr_from_any_ptr(const any &ptr) const {
        if (this->is_bit_field()) { return field_error::IS_BIT_FIELD; }
        return reinterpret_cast<F*>(ptr.as<char*>() + this->offset());
    }

    // get pointer to field as F* from some reference to T
    template <typename F, typename T>
        requires (!std::is_pointer_v<T>)
    result<F*, field_error> get_ptr(const T &t) const {
        return this->get_ptr_from_any_ptr<F>(any::make(&t));
    }

    // get pointer to field as any from some pointer to T
    template <typename T>
    result<any, field_error> get_any_ptr(const T *t) const {
        return this->get_any_ptr_from_any_ptr(any::make(t));
    }

    // get pointer to field as F* from some pointer to T
    template <typename F, typename T>
    result<F*, field_error> get_ptr(const T *t) const {
        return this->get_ptr_from_any_ptr<F>(any::make(t));
    }

    // get reference to field as F& from some any pointer to T (const or not)
    // does not type check
    // returns error if field is bit field, must use value for bitfields
    template <typename F>
    result<std::reference_wrapper<F>, field_error> get_from_any_ptr(
        const any &ptr) const {
        return result<std::reference_wrapper<F>, field_error>::make(
            !this->is_bit_field(),
            [&]() -> std::reference_wrapper<F> {
                return
                    *reinterpret_cast<F*>(
                        ptr.as<char*>() + this->offset());
            },
            field_error::IS_BIT_FIELD);
    }

    // get reference to field as F& from some any VALUE (not ptr!)
    // returns error if field is bit field, must use value for bitfields
    template <typename F>
    result<std::reference_wrapper<F>, field_error> get_from_any(
        const any &any_value) const {
        return this->get_from_any_ptr<F>(any::make(any_value.storage()));
    }

    // get reference to field as F& from some T
    // returns error if field is bit field, must use value for bitfields
    template <typename F, typename T>
    result<std::reference_wrapper<F>, field_error> get(
        const T &t) const {
        return this->get_from_any_ptr<F>(any::make(&t));
    }

    // get value (NOT REFERENCE) to field on some T
    // TODO: values from any
    template <typename F, typename T>
    result<std::reference_wrapper<F>, field_error> value(
        const T &t) const {
        if (this->is_bit_field()) {
            // TODO: fix for types larger than uintmax_t?
            constexpr auto UINTMAX_BITS = sizeof(uintmax_t) * CHAR_BIT;
            const auto bits = *this->bit_offset();
            auto p =
                *(reinterpret_cast<uintmax_t*>(&const_cast<T&>(t))
                    + (bits / UINTMAX_BITS));
            return
                static_cast<F>(
                    (p >> (bits % UINTMAX_BITS))
                        & ((1 << (bits + 1)) - 1));
        }

        return this->get<F>(t);
    }

    // set field on some object T (via copy assignment)
    template <typename T, typename F>
        requires std::is_copy_assignable_v<F>
    void set(const T &t, const F &f) const {
        if (this->is_bit_field()) {
            this->set_bit_field(t, f);
        } else {
            this->get<F>(t)->get() = f;
        }
    }

    // set field on some object T (via move assignment)
    template <typename T, typename F>
        requires std::is_move_assignable_v<F>
    void set(const T &t, F &&f) const {
        if (this->is_bit_field()) {
            this->set_bit_field(t, f);
        } else {
            this->get<F>(t)->get() = std::move(f);
        }
    }

private:
    template <typename T, typename F>
    void set_bit_field(const T &t, F f) const {
        if constexpr (std::is_integral_v<F>) {
            // TODO: fix for types larger than uintmax_t?
            constexpr auto UINTMAX_BITS = sizeof(uintmax_t) * CHAR_BIT;
            const auto
                offset = *this->bit_offset(),
                size = *this->bit_field_size();
            auto *p =
                (reinterpret_cast<uintmax_t*>(&const_cast<T&>(t))
                    + (offset / UINTMAX_BITS));
            const auto
                size_mask = (1 << (size + 1)) - 1,
                mask = size_mask << (offset % UINTMAX_BITS);
            *p = (*p & ~mask) | ((f & size_mask) << (offset % UINTMAX_BITS));
        }
    }

    const detail::field_type_info *info;
};

// reflected_type with kind ARRAY
struct reflected_array_type : public reflected_type {
    using reflected_type::reflected_type;

    // array length, 0 if unknown
    size_t length() const {
        return this->info->array.length;
    }
};

// reflected_type with kind ENUM
// NOTE: enums WILL BREAK if values are larger than std::uintmax_t
struct reflected_enum_type : public reflected_type {
    using reflected_type::reflected_type;

    // base type of enum
    qualified_reflected_type base_type() const {
        return qualified_reflected_type(&this->info->enum_.base_type);
    }

    // names/values in this enum
    auto values() const {
        return detail::transform<vector<std::tuple<std::string_view, size_t>>>(
            this->info->enum_.name_to_value,
            [](const auto &p) {
                const auto &[n, v] = p;
                return std::make_tuple(std::string_view(n), v);
            });
    }

    // enum value -> string(s)
    template <typename T>
    auto names_of(const T &t) {
        auto it = this->info->enum_.value_to_name.find(t);
        return
            it == this->info->enum_.value_to_name.end() ?
                vector<std::string_view>()
                : detail::transform<vector<std::string_view>>(
                    it->second,
                    [](const auto &s) { return std::string_view(s); });
    }

    // string -> enum value
    template <typename T>
    std::optional<T> value_of(std::string_view s) {
        auto it =
            std::find_if(
                this->info->enum_.name_to_value.begin(),
                this->info->enum_.name_to_value.end(),
                [&](const auto &p) {
                    const auto &[k, _] = p;
                    return k == s;
                });
        return it == this->info->enum_.name_to_value.end() ?
            std::nullopt
            : std::make_optional(
                static_cast<T>(it->second));
    }
};

// reflected_type with kind MEMBER_PTR
struct reflected_member_ptr_type : public reflected_type {
    using reflected_type::reflected_type;

    // class type of member pointer
    reflected_type class_type() const {
        return reflected_type(&*this->info->member_ptr.class_type);
    }
};

inline qualified_reflected_type reflected_field::type() const {
    return qualified_reflected_type(&this->info->type);
}

inline reflected_record_type reflected_static_field::parent() const {
    return reflected_type(&*this->info->parent_id).as_record();
}

inline qualified_reflected_type reflected_static_field::type() const {
    return qualified_reflected_type(&this->info->type);
}

inline qualified_reflected_type reflected_parameter::type() const {
    const auto &func = (*this->function_info->id);
    if (func.kind != FUNC) { ARCHIMEDES_FAIL("funtion is not function"); }
    return qualified_reflected_type(
        &func.function.parameters[this->index()]);
}

inline std::optional<qualified_reflected_type> reflected_type::type() const {
    switch (this->kind()) {
        case REF:
        case RREF:
        case PTR:
        case MEMBER_PTR:
        case ARRAY:
            return qualified_reflected_type(&this->info->type);
        default:
            return std::nullopt;
    }
}

inline reflected_record_type reflected_type::as_record() const {
    return reflected_record_type(this->info);
}

inline reflected_function_type reflected_type::as_function() const {
    return reflected_function_type(this->info);
}

inline reflected_array_type reflected_type::as_array() const {
    return reflected_array_type(this->info);
}

inline reflected_enum_type reflected_type::as_enum() const {
    return reflected_enum_type(this->info);
}

inline reflected_member_ptr_type reflected_type::as_member_ptr() const {
    return reflected_member_ptr_type(this->info);
}

inline reflected_record_type reflected_base::parent() const {
    return reflected_type(&*this->info->parent_id).as_record();
}

inline reflected_record_type reflected_base::type() const {
    return reflected_record_type(&*this->info->id);
}

inline reflected_record_type reflected_field::parent() const {
    return reflected_type(&*this->info->parent_id).as_record();
}

inline std::optional<reflected_record_type> reflected_function::parent() const {
    if (this->info->parent_id == type_id::none()) {
        return std::nullopt;
    }

    return reflected_type(&*this->info->parent_id).as_record();
}

inline reflected_function_type reflected_function::type() const {
    const auto *info = &*this->info->id;
    if (info->kind != FUNC) { ARCHIMEDES_FAIL("function is not function"); }
    return reflected_function_type(info);
}

// list of fields
inline vector<reflected_field> reflected_record_type::fields() const {
    return detail::transform<vector<reflected_field>>(
        detail::values(this->info->record.fields),
        [](const auto &p) {
            return reflected_field(&p); });
}

// field by regex
inline std::optional<reflected_field> reflected_record_type::field(
    std::string_view regex) const {
    return detail::find(
        this->fields(),
        [&](const auto &f) {
            return regex_matches(regex, f.name());
        });
}
}

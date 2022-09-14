#pragma once

#include <string>

#include "type_id.hpp"
#include "type_kind.hpp"
#include "access_specifier.hpp"
#include "any.hpp"
#include "invoke.hpp"
#include "ds.hpp"

namespace archimedes::detail {
inline constexpr auto NO_ARRAY_INDEX = std::numeric_limits<size_t>::max();

// forward decl, only defined in implementation
struct internal;
struct function_type_info_internal;
struct static_field_type_info_internal;
struct struct_base_type_info_internal;
struct template_parameter_info_internal;

// dynamic cast function signature
using dyncast_fn = std::function<void*(void*)>;

// info for const/volatile types
struct qualified_type_info {
    // id of type
    type_id id = type_id::none();

    // true if type is "const ..."
    bool is_const = false;

    // true if type is "volatile ..."
    bool is_volatile = false;

    static inline qualified_type_info none() {
        return qualified_type_info {
            .id = type_id::none(),
            .is_const = false,
            .is_volatile = false
        };
    }
};

// info about a typedef
// ALWAYS FULLY QUALIFIED
// fx. "using A = B<int>" -> { name = A, aliased_type = B<int> }
struct typedef_info {
    std::string name = "";
    qualified_type_info aliased_type = qualified_type_info::none();
};

// info about a namespace alias
// ALWAYS FULLY QUALIFIED, so both "name" and "aliased" start from the global
// namespace
// examples:
// "namespace ab = a::b" -> { name = ab, aliased = a::b }
// "namespace c { namespace ab = a::b; }" -> { name = c::ab, aliased = a::b }
struct namespace_alias_info {
    std::string name = "";
    std::string aliased = "";
};

// info about a namespace using directive
// ALWAYS FULL QUALIFIED (start from global namespace, do not use aliases)
// fx. "namespace a { namespace b { ... } using namespace b; }"
// will result in { containing = a, used = a::b }
// "containing" will be empty if the using directive appears in the global
// namespace
struct namespace_using_info {
    std::string containing = "";
    std::string used = "";
};
// type information for struct base types i.e. struct Foo : Bar {}
struct struct_base_type_info {
    // id of type inheriting
    type_id parent_id = type_id::none();

    // id of type being inherited from
    type_id id = type_id::none();

    // access specifier on inheritance
    AccessSpecifier access = AccessSpecifier::PRIVATE;

    // if true, this is a virtual base
    bool is_virtual = false;

    // if true, this is the primary base class
    bool is_primary = false;

    // true if this is a "vbase" (virtual base with storage on this record)
    // fx. in the hierarchy
    // A {}
    // B : virtual A {}
    // C: virtual A {}
    // D : B, C {}
    // B, C, *AND* D all have the virtual base "A"
    bool is_vbase = false;

    // offset of this base's fields
    size_t offset = 0;

    size_t dyncast_up_index = NO_ARRAY_INDEX;
    size_t dyncast_down_index = NO_ARRAY_INDEX;

    // dynamic cast functions from parent -> base and base parent, only appears
    // in emitted code
    dyncast_fn dyncast_up;
    dyncast_fn dyncast_down;

    // internal use only
    struct_base_type_info_internal *internal = nullptr;
};

// type info for a record field
struct field_type_info {
    // id of type this field is on
    type_id parent_id = type_id::none();

    // type of field
    qualified_type_info type = qualified_type_info::none();

    // name of field
    std::string name = "";

    // offset of field on parent
    size_t offset = 0;

    // size of field
    size_t size = 0;

    // access specifier to field
    AccessSpecifier access = AccessSpecifier::PRIVATE;

    // true if field is marked "mutable"
    bool is_mutable = false;

    // true if field is bit field
    bool is_bit_field = false;

    // if is_bit_field, width of bitfield i.e. "int x : 8;" -> 8
    size_t bit_size = 0;

    // if is_bit_field offset IN BITS into parent record type
    size_t bit_offset = 0;

    // annotations on field
    vector<std::string> annotations = {};
};

// type info for static record field
struct static_field_type_info {
    // id of type this field is on
    type_id parent_id = type_id::none();

    // type of field
    qualified_type_info type = qualified_type_info::none();

    // name of field
    std::string name = "";

    // access specifier to field
    AccessSpecifier access = AccessSpecifier::PRIVATE;

    // true if field is "constexpr" qualified
    bool is_constexpr = false;

    // INTERNAL USE ONLY
    // index of constexpr value in module constexpr value array
    size_t constexpr_value_index = 0;

    // if is_constexpr, value of constexpr field
    any constexpr_value = any::none();

    // annotations on field
    vector<std::string> annotations = {};

    // INTERNAL USE ONLY
    // extra data
    static_field_type_info_internal *internal = nullptr;
};

// type info for function parameter
struct function_parameter_info {
    // name of parameter in functino
    std::string name = "";

    // index in function parameters list
    size_t index = 0;

    // if true, parameter has a default value
    bool is_defaulted = false;
};

// type info for function declaration
struct function_type_info {
    // INTERNAL USE ONLY
    // index of invoker in module invoker array
    size_t invoker_index = NO_ARRAY_INDEX;

    // pointer to invoker, nullptr if not present
    detail::invoker_ptr invoker = nullptr;

    // id of type this function is on ("none" if free function)
    type_id parent_id = type_id::none();

    // type of function
    type_id id = type_id::none();

    // qualified name of function (namespace'd/class'd/etc.)
    std::string qualified_name = "";

    // name of function (no qualfiers)
    std::string name = "";

    // true if member function
    bool is_member = false;

    // true if static function
    bool is_static = false;

    // true if constructor
    bool is_ctor = false;

    // true if destructor
    bool is_dtor = false;

    // true if converter
    bool is_converter = false;

    // true if marked explicit
    bool is_explicit = false;

    // true if marked virtual
    bool is_virtual = false;

    // true if function is deleted
    bool is_deleted = false;

    // true if function is defaulted (explicitly OR IMPLICITLY)
    bool is_defaulted = false;

    // parameters list
    // NOTE: cannot be a name_map because it is possible that parameters are
    // unnamed (and would therefore map to the same name of {})
    vector<function_parameter_info> parameters = {};

    // annotations on function
    vector<std::string> annotations = {};

    // path to file which defines function
    std::string definition_path = "";

    // INTERNAL USE ONLY
    // extra data
    function_type_info_internal *internal = nullptr;
};

struct function_overload_set {
    // qualified name of set of function overloads (namespace'd/class'd/etc.)
    std::string qualified_name = "";

    // unqualified name of set of function overloads
    std::string name = "";

    // list of functions in set
    vector<function_type_info> functions = {};
};

struct template_parameter_info {
    // name of this template parameter
    std::string name;

    // type of this template parameter
    qualified_type_info type;

    // true if this is a type (typename/class) template parameter
    bool is_typename = false;

    // INTERNAL USE ONLY
    // index of value in module template parameter value array
    size_t value_index = NO_ARRAY_INDEX;

    // value if not is_typename
    any value = any::none();

    // INTERNAL USE ONLY
    // extra data
    template_parameter_info_internal *internal = nullptr;
};

struct type_info {
    // unique identifier for this type info
    type_id id = type_id::none();

    // TODO: allow RTTI disable
    // typeid(...).hash_code() for this type
    size_t type_id_hash = 0;

    // INTERAL USE ONLY
    // index of type index hash in module type index hash array
    size_t type_id_hash_index = NO_ARRAY_INDEX;

    // type kind
    type_kind kind = UNKNOWN;

    // type name
    std::string type_name = "";

    // mangled type name
    std::string mangled_type_name = "";

    // pointer/ref'd/array'd type
    // only valid for ptr, ref, rvalue ref, member ptr, array types
    qualified_type_info type = qualified_type_info::none();

    // size of type, 0 if sizeof(...) would not be valid
    size_t size = 0;

    // align of type, 0 if alignof(...) would not be valid
    size_t align = 0;

    // annotation attribute values
    vector<std::string> annotations = {};

    // definition file path
    // only valid for user-defined types, namely struct/union/enum
    std::string definition_path = "";

    struct {
        // fully namespace'd type name
        std::string qualified_name = "";

        // template parameters in order of declaration
        vector<template_parameter_info> template_parameters = {};

        // type bases in order of declaration
        vector<struct_base_type_info> bases = {};

        // typedef-s and using-s
        name_map<typedef_info> typedefs = {};

        // fields
        name_map<field_type_info> fields = {};

        // static fields
        name_map<static_field_type_info> static_fields = {};

        // functions
        name_map<function_overload_set> functions = {};

        // true if type is POD
        bool is_pod = false;

        // constructor/destructor info
        bool has_trivial_default_ctor = false;
        bool has_trivial_copy_ctor = false;
        bool has_trivial_copy_assign = false;
        bool has_trivial_move_ctor = false;
        bool has_trivial_move_assign = false;
        bool has_trivial_dtor = false;

        // true if record is trivially copyable
        bool is_trivially_copyable = false;

        // true if record is abstract
        bool is_abstract = false;

        // true if class is polymorphic (contains or inherits a virtual fn)
        bool is_polymorphic = false;

        // if true, record is polymorphic OR has virtual bases
        bool is_dynamic = false;

        // size of record in bytes
        size_t size = 0;

        // alignment of record in bytes
        size_t alignment = 0;
    } record;

    // function
    struct {
        qualified_type_info return_type = qualified_type_info::none();
        vector<qualified_type_info> parameters = {};

        // true if function is member and const qualified
        bool is_const = false;

        // true if function is member and rref (&&) qualified
        bool is_rref = false;

        // true if function is member and ref (&) qualified
        bool is_ref = false;
    } function;

    // array
    struct {
        // array length, 0 if unknown
        size_t length = 0;
    } array;

    // enum
    struct {
        // base type of enum, i.e. "enum E : uint8_t { ... }" -> uint8_t
        qualified_type_info base_type = qualified_type_info::none();

        // map of enum names to their values
        map<std::string, std::uintmax_t> name_to_value = {};

        // map of enum values to their names
        map<std::uintmax_t, vector<std::string>> value_to_name = {};
    } enum_;

    // member ptr
    struct {
        type_id class_type = type_id::none();
    } member_ptr;

    archimedes::detail::internal *internal = nullptr;
};
};

// extern'd templates for compile performance
extern template struct
    archimedes::name_map<archimedes::detail::function_overload_set>;
extern template struct
    archimedes::name_map<archimedes::detail::function_parameter_info>;
extern template struct
    archimedes::name_map<archimedes::detail::static_field_type_info>;
extern template struct
    archimedes::name_map<archimedes::detail::field_type_info>;

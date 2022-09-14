#pragma once

#include <optional>
#include <span>

#include "type_id.hpp"
#include "type_info.hpp"
#include "types.hpp"
#include "serialize.hpp"
#include "ds.hpp"

namespace archimedes {
// user registered callback for registering type collisions
using collision_callback =
    std::function<reflected_type(reflected_type, reflected_type)>;

namespace detail {
// global type registry
struct registry {
    // implemented in runtime/archimedes.cpp (must be linked!)
    static registry &instance();

    // dtor
    ~registry() {
        this->_loaded = false;
    }

    // returns true if registry is loaded
    bool loaded() const {
        return this->_loaded;
    }

    // load all loaders
    void load();

    // get matching types from annotations
    vector<const type_info*> find_by_annotations(
        std::span<const std::string_view> annotations);

    // get all types
    vector<const type_info*> all_types() const;

    // get type_info by typeid(...).hash_code()
    std::optional<const type_info*> type_from_type_id_hash(size_t hash) const;

    // get type_info by id
    std::optional<const type_info*> type_from_id(type_id id) const;

    // get type_info by name
    std::optional<const type_info*> type_from_name(
        std::string_view name) const;

    // get function by name
    std::optional<const vector<function_overload_set>*> function_by_name(
        std::string_view name) const;

    // get function by type
    std::optional<const vector<function_type_info>*> function_by_type(
        type_id id) const;

    // callback for type collision
    void set_collision_callback(collision_callback &&tcc);

    // NOTE: INTERNAL USE ONLY!
    // called from each archimedes translation unit to load stored data
    void load_module(
        const vector<std::function<void*(void*)>> &dyncasts,
        const vector<any> &constexpr_values,
        const vector<invoker_ptr> &invokers,
        const vector<any> &template_param_values,
        const vector<size_t> &type_id_hashes,
        std::span<const uint8_t> functions_data,
        std::span<const uint8_t> types_data,
        std::span<const uint8_t> typedefs_data,
        std::span<const uint8_t> aliases_data,
        std::span<const uint8_t> usings_data);

    // load a set of types into the registry
    // TODO: std::move values
    void load_types(const vector<type_info> is);

    // load a set of function overloads into the registry
    // TODO: move from map
    void load_functions(
        const name_map<function_overload_set> &fs);

    // expand namespaces according to namespace_aliases
    std::string expand_namespaces(std::string_view name) const;

    // invoke a function F with all possible expansions of the specified name
    // fx. if we have namespace a::b which uses namespace a::b::c::d, f is
    // invoked for f("a::b::<name>") and f("a::b::c::d::<name>") if name is
    // prefixed by "a::b".
    // F returns true if it has found what it is looking for, function returns
    // true if and F returned true
    template <typename F>
        requires std::is_invocable_r_v<bool, F, std::string_view>
    bool permute_namespaces(std::string_view name, F &&f) const {
        std::function<bool(std::string_view)> g;
        g = [this, &f, &g](std::string_view n) {
            if (f(n)) {
                return true;
            }

            for (const auto &nu : this->namespace_usings) {
                if (n.starts_with(nu.containing)
                        && !n.starts_with(nu.used)) {
                    // replace with used namespace, try again
                    auto replaced =
                        nu.used
                            + std::string(n.substr(nu.containing.length()));
                    if (g(replaced)) {
                        return true;
                    }
                    break;
                }
            }

            return false;
        };
        return g(name);
    }

    mutable bool _loaded = false;
    vector<std::function<void(void)>> loaders;

    // backing storage containers
    map<size_t, type_info*> types_by_type_id_hashes;
    map<type_id, type_info> types_by_id;
    map<type_id, vector<function_type_info>> functions_by_type;
    map<std::string, vector<function_overload_set>> functions_by_name;
    vector<namespace_alias_info> namespace_aliases;
    vector<namespace_using_info> namespace_usings;
    vector<typedef_info> typedefs;

    // collision callbacks
    collision_callback tcc =
        [](auto _0, auto _1) -> reflected_type {
            std::cout
                << "collision between "
                << _0.name()
                << "/"
                << _0.mangled_name()
                << " "
                << _1.name()
                << "/"
                << _1.mangled_name()
                << std::endl;
            ARCHIMEDES_FAIL("type collision");
        };
};
} // namespace detail
} // namespace archimedes

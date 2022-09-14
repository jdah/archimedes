#include <archimedes/registry.hpp>
#include <archimedes.hpp>

namespace archimedes {
namespace detail {
registry &registry::instance() {
    static registry instance;
    return instance;
}

void registry::load() {
    if (this->_loaded) {
        ARCHIMEDES_FAIL("load() called twice");
    }

    this->_loaded = true;

    for (const auto &l : this->loaders) {
        l();
    }

    // load typeid hash map
    for (auto &[_, t] : this->types_by_id) {
        if (t.type_id_hash == 0) {
            continue;
        }

        this->types_by_type_id_hashes[t.type_id_hash] = &t;
    }
}

vector<const type_info*> registry::find_by_annotations(
    std::span<const std::string_view> annotations) {
    vector<const type_info*> ts;
    for (const auto &[_, t] : this->types_by_id) {
        if (std::all_of(
                annotations.begin(),
                annotations.end(),
                [t = t](const auto &a) {
                return std::any_of(
                        t.annotations.begin(),
                        t.annotations.end(),
                        [&](const auto &b) {
                            return a == b;
                        });
                })) {
            ts.push_back(&t);
        }
    }
    return ts;
}

vector<const type_info*> registry::all_types() const {
    return transform<vector<const type_info*>>(
        values(this->types_by_id),
        [](const auto &t) {
            return &t;
        });
}

std::optional<const type_info*> registry::type_from_type_id_hash(size_t hash) const {
    auto it = this->types_by_type_id_hashes.find(hash);
    return it == this->types_by_type_id_hashes.end() ?
        std::nullopt
        : std::make_optional(it->second);
}

std::optional<const type_info*> registry::type_from_id(
    type_id id) const {
    auto it = this->types_by_id.find(id);
    return it == this->types_by_id.end() ?
        std::nullopt
        : std::make_optional(&it->second);
}

std::optional<const type_info*> registry::type_from_name(
    std::string_view name) const {
    // expand any namespace aliases in the name
    auto expanded = expand_namespaces(name);

    std::optional<const type_info*> result;
    this->permute_namespaces(
        expanded,
        [this, &result](std::string_view s) -> bool {
            // check if name is a typedef, if so return type directly
            for (const auto &t : this->typedefs) {
                if (s == t.name) {
                    result = &*t.aliased_type.id;
                    return true;
                }
            }

            // get by id
            result = this->type_from_id(type_id::from(s));
            return static_cast<bool>(result);
        });

    return result;
}

std::optional<const vector<function_overload_set>*> registry::function_by_name(
    std::string_view name) const {
    auto expanded = expand_namespaces(name);
    auto it = this->functions_by_name.find(std::string(expanded));
    return it == this->functions_by_name.end() ?
        std::nullopt
        : std::make_optional(&it->second);
}

std::optional<const vector<function_type_info>*> registry::function_by_type(
    type_id id) const {
    auto it = this->functions_by_type.find(id);
    return it == this->functions_by_type.end() ?
        std::nullopt
        : std::make_optional(&it->second);
}

void registry::set_collision_callback(collision_callback &&tcc) {
    this->tcc = tcc;
}

static void load_module_internal(
    registry &registry,
    const vector<std::function<void*(void*)>> &dyncasts,
    const vector<any> &constexpr_values,
    const vector<invoker_ptr> &invokers,
    const vector<any> &template_param_values,
    const vector<size_t> &type_id_hashes,
    std::span<const uint8_t> functions_data,
    std::span<const uint8_t> types_data,
    std::span<const uint8_t> typedefs_data,
    std::span<const uint8_t> aliases_data,
    std::span<const uint8_t> usings_data) {
    // load invokers from array for fuction set
    const auto load_invokers =
        [&](function_overload_set &fos) {
            for (auto &f : fos.functions) {
                if (f.invoker_index != NO_ARRAY_INDEX) {
                    f.invoker = invokers[f.invoker_index];
                }
            }
        };

    // std::move all of src onto dst
    const auto append =
        [](auto &dst, auto &src) {
            std::move(
                std::begin(src),
                std::end(src),
                std::back_inserter(dst));
            src.clear();
        };

    // load src_data (serialized vector of same type as dst) and append data
    // onto dst
    const auto load_basic =
        [&append](auto &dst, auto &src_data) {
            using V = std::remove_reference_t<decltype(dst)>;
            auto vec = vector<typename V::value_type>();
            auto is = bytestream(src_data);
            deserialize(is, vec);
            append(dst, vec);
        };

    auto types =
        deserialize<vector<type_info>>(types_data);

    auto functions =
        deserialize<name_map<function_overload_set>>(functions_data);

    // patch indexed values for each type, function
    for (auto &t : types) {
        if (t.type_id_hash_index != NO_ARRAY_INDEX) {
            t.type_id_hash = type_id_hashes[t.type_id_hash_index];
        }

        if (t.kind != STRUCT && t.kind != UNION) {
            continue;
        }

        for (auto &p : t.record.template_parameters) {
            if (p.value_index != NO_ARRAY_INDEX) {
                p.value = template_param_values[p.value_index];
            }
        }

        for (auto &b : t.record.bases) {
            if (b.dyncast_down_index != NO_ARRAY_INDEX) {
                b.dyncast_down = dyncasts[b.dyncast_down_index];
            }

            if (b.dyncast_up_index != NO_ARRAY_INDEX) {
                b.dyncast_up = dyncasts[b.dyncast_up_index];
            }
        }

        for (auto &[_, f] : t.record.static_fields) {
            if (f.constexpr_value_index != NO_ARRAY_INDEX) {
                f.constexpr_value =
                    constexpr_values[f.constexpr_value_index];
            }
        }

        for (auto &[_, fos] : t.record.functions) {
            load_invokers(fos);
        }
    }

    for (auto &[_, fos] : functions) {
        load_invokers(fos);
    }

    registry.load_types(types);
    registry.load_functions(functions);

    /* auto vec = vector<typedef_info>(); */
    /* auto is = bytestream(typedefs_data); */
    /* deserialize(is, vec); */
    /* for (const auto &v : vec) { */
    /*     std::cout */
    /*         << "got typedef " */
    /*         << v.name */
    /*         << " <- " */
    /*         << v.aliased_type.id */
    /*         << std::endl; */
    /* } */

    // insert typedefs, aliases, usings directly
    load_basic(registry.typedefs, typedefs_data);
    load_basic(registry.namespace_aliases, aliases_data);
    load_basic(registry.namespace_usings, usings_data);
}


void registry::load_module(
    const vector<std::function<void*(void*)>> &dyncasts,
    const vector<any> &constexpr_values,
    const vector<invoker_ptr> &invokers,
    const vector<any> &template_param_values,
    const vector<size_t> &type_id_hashes,
    std::span<const uint8_t> functions_data,
    std::span<const uint8_t> types_data,
    std::span<const uint8_t> typedefs_data,
    std::span<const uint8_t> aliases_data,
    std::span<const uint8_t> usings_data) {
    const auto f =
        [=, &dyncasts, &constexpr_values, &invokers]() {
            load_module_internal(
                *this,
                dyncasts,
                constexpr_values,
                invokers,
                template_param_values,
                type_id_hashes,
                functions_data,
                types_data,
                typedefs_data,
                aliases_data,
                usings_data);
        };

    this->loaders.push_back(f);
}

// load a set of types into the registry
// TODO: std::move values
void registry::load_types(const vector<type_info> is) {
    // returns true if type_info i can collide legally with an existing type
    const auto can_collide =
        [](const type_info &i) {
            return i.kind != STRUCT && i.kind != UNION && i.kind != ENUM;
        };

    for (const auto &i : is) {
        const auto it = this->types_by_id.find(i.id);
        if (it == this->types_by_id.end()) {
            this->types_by_id.emplace(i.id, i);
        } else {
            const auto &other = it->second;
            if (can_collide(i) && can_collide(other)) {
                // do nothing, collisions are fine here
            } else if (i.kind == UNKNOWN) {
                // do nothing, keep existing type
            } else if (other.kind == UNKNOWN) {
                // replace with new type
                // DOES THIS REPLACE??
                this->types_by_id[i.id] = i;
            } else {
                // resolve type collision
                const auto *info =
                    this->tcc(
                        reflected_type(&other),
                        reflected_type(&i)).info;

                this->types_by_id[info->id] = other;
            }
        }
    }
}

// load a set of function overloads into the registry
// TODO: move from map
void registry::load_functions(
    const name_map<function_overload_set> &fs) {
    for (const auto &[name, s] : fs) {
        const auto it_fbn = this->functions_by_name.find(name);

        vector<function_overload_set> *v_fbn;
        if (it_fbn == this->functions_by_name.end()) {
            v_fbn =
                &this->functions_by_name.emplace(
                    name,
                    vector<function_overload_set>())
                    .first->second;
        } else {
            v_fbn = &it_fbn->second;
        }

        // append set to list of sets with this name
        v_fbn->push_back(s);

        // add into set of functions by type
        // no need to resolve conflicts here as multiple functions can come
        // up for the same type (this is expected behavior)
        for (const auto &f : s.functions) {
            auto it = this->functions_by_type.find(f.id);
            auto &vec =
                it == this->functions_by_type.end() ?
                    (this->functions_by_type.emplace(
                        f.id,
                        vector<function_type_info>())
                        .first->second)
                    : it->second;
            vec.push_back(f);
        }
    }
}

// expand namespaces according to namespace_aliases
std::string registry::expand_namespaces(std::string_view name) const {
    auto expanded = std::string(name);
expand:
    for (const auto &na : this->namespace_aliases) {
        if (expanded.starts_with(na.name)) {
            std::string old = expanded;
            expanded =
                na.aliased + expanded.substr(na.name.length());
            goto expand;
        }
    }
    return expanded;
}
} // namespace detail
} // namespace archimedes

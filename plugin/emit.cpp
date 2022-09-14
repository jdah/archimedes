#include "emit.hpp"
#include "plugin.hpp"
#include "invoker.hpp"

#include <archimedes/serialize.hpp>

#include <iostream>

using namespace archimedes;
using namespace archimedes::detail;

// emit serialized data as an std::span<const uint8_t>
// F(std::ostream) is called to populate data
template <typename F>
    requires (requires (F f, std::ostream &os) { f(os); })
static std::string emit_serialized(
    std::string_view name,
    F &&f) {
    struct outbuf : public std::streambuf {
        vector<uint8_t> &buf;

        outbuf(vector<uint8_t> &buf) : buf(buf) {}

        int_type overflow(int_type c) override {
            if (c == EOF) {
                return EOF;
            }

            buf.push_back(static_cast<uint8_t>(c));
            return c;
        }
    };

    vector<uint8_t> data;
    outbuf ob(data);
    std::ostream os(&ob);
    f(os);
    return fmt::format(R"(
            static const uint8_t {0}_internal[] = {1};
            static const std::span<const uint8_t, {2}> {0} = {{ {0}_internal }};
        )",
        name,
        emit_data(std::span { data }),
        data.size());
}

// emit a vector of Ts with specified name and type name to module
// F(T) is cpp-ifier for Ts
template <typename T, typename F>
    requires (requires (F f, T t) {{ f(t) } -> std::same_as<std::string>; })
static std::string emit_module_vector(
    std::string_view name,
    std::string_view type_name,
    const vector<T> &vs,
    F &&f) {
    return fmt::format(R"(
            static const archimedes::vector<{}> {} = {{{}}};
        )",
        type_name, name, emit_vector(vs, std::forward<F>(f)));
}

std::string archimedes::emit(Context &ctx) {
    std::string output;

    // TODO: optional RTTI
    // preamble
    output +=
        fmt::format(R"(
                #include "{}"
                #include <typeinfo>
                // ignored diagnostics in generated code
                #pragma clang diagnostic ignored "-Wmain"
                #pragma clang diagnostic ignored "-Wimplicitly-unsigned-literal"
            )",
            fs::relative(
                ctx.header_path,
                ctx.output_path.parent_path()).string());

    // emit invokers
    for (const auto &i : ctx.invokers) {
        output += emit_invoker(ctx, *i);
    }

    // traverse context data and assign indices
    vector<std::tuple<const type_info*, const type_info*>> dyncasts;
    vector<const std::string*> constexpr_exprs;
    vector<const Invoker*> invokers;
    vector<const std::string*> template_param_exprs;
    vector<std::string> type_id_hash_exprs;

    // true if dyncasts can be generated between a and b
    const auto can_dyncast =
        [&ctx](const type_info &a, const type_info &b) {
            const auto
                *rd_a = clang::dyn_cast<clang::CXXRecordDecl>(a.internal->decl),
                *rd_b = clang::dyn_cast<clang::CXXRecordDecl>(b.internal->decl);
            return rd_a->isPolymorphic()
                && rd_b->isPolymorphic()
                && can_emit_type(ctx, a.internal->type)
                && can_emit_type(ctx, b.internal->type);
        };

    // register invokers for the function set
    const auto register_invokers =
        [&](function_overload_set &fos) {
            for (auto &f : fos.functions) {
                if (!f.internal->invoker) {
                    f.invoker_index = NO_ARRAY_INDEX;
                    continue;
                }

                f.invoker_index = invokers.size();
                invokers.push_back(f.internal->invoker);
            }
        };

    // record types
    for (const auto &t : ctx.types) {
        if ((t->kind != STRUCT && t->kind != UNION)
            || !t->internal->is_resolved) {
            continue;
        }

        // only emit type_id(...) exprs for struct/union types
        if (can_emit_type(ctx, t->internal->type)) {
            t->type_id_hash_index = type_id_hash_exprs.size();
            type_id_hash_exprs.push_back(
                fmt::format(
                    "typeid({}).hash_code()",
                    get_full_type_name(
                        ctx, *t->internal->type, TNF_ARRAYS_TO_POINTERS)));
        }

        for (auto &p : t->record.template_parameters) {
            if (p.is_typename) {
                continue;
            }

            // TODO: do we need to register?
            ctx.register_emitted_type(*p.type.id->internal->type);
            p.value_index = template_param_exprs.size();
            template_param_exprs.push_back(&p.internal->value_expr);
        }

        for (auto &b : t->record.bases) {
            if (!can_dyncast(*b.parent_id, *b.id)) {
                b.dyncast_up_index = NO_ARRAY_INDEX;
                b.dyncast_down_index = NO_ARRAY_INDEX;
                continue;
            }

            b.dyncast_up_index = dyncasts.size();
            dyncasts.push_back(
                std::make_tuple(
                    &*b.parent_id,
                    &*b.id));
            b.dyncast_down_index = dyncasts.size();
            dyncasts.push_back(
                std::make_tuple(
                    &*b.id,
                    &*b.parent_id));
        }

        for (auto &[_, f] : t->record.static_fields) {
            if (!f.is_constexpr
                    || f.internal->constexpr_expr.empty()) {
                f.constexpr_value_index = NO_ARRAY_INDEX;
                continue;
            }

            ctx.register_emitted_type(*t->internal->type);
            f.constexpr_value_index = constexpr_exprs.size();
            constexpr_exprs.push_back(&f.internal->constexpr_expr);
        }

        for (auto &[_, fos] : t->record.functions) {
            register_invokers(fos);
        }
    }

    // free functions
    for (auto &[_, fos] : ctx.functions) {
        register_invokers(fos);
    }

    constexpr auto
        DYNCASTS_NAME = "_module_dyncasts",
        CONSTEXPR_VALUES_NAME = "_module_constexpr_values",
        INVOKERS_NAME = "_module_invokers",
        TEMPLATE_PARAM_VALUES_NAME = "_module_template_param_values",
        TYPE_ID_HASHES_NAME = "_type_id_hashes";

    // emit dyncast array
    output +=
        emit_module_vector(
            DYNCASTS_NAME,
            "std::function<void*(void*)>",
            dyncasts,
            [&ctx](const auto &t) -> std::string {
                const auto &[from, to] = t;
                ctx.register_emitted_type(*from->internal->type);
                ctx.register_emitted_type(*to->internal->type);
                return
                    fmt::format(R"(
                            ([](void *p) -> void* {{
                                return dynamic_cast<{}*>(
                                    reinterpret_cast<{}*>(p));
                            }})
                        )",
                        emit_type(ctx, *to->internal->type),
                        emit_type(ctx, *from->internal->type));
            });

    // emit constexpr value array
    output +=
        emit_module_vector(
            CONSTEXPR_VALUES_NAME,
            NAMEOF_TYPE(archimedes::any),
            constexpr_exprs,
            [](const std::string *s) -> std::string {
                return emit_any(*s);
            });

    // emit invoker array
    output +=
        emit_module_vector(
            INVOKERS_NAME,
            "archimedes::detail::invoker_ptr",
            invokers,
            [](const Invoker *i) -> std::string {
                return fmt::format("&{}", i->generated_name());
            });

    // emit template parameter value array
    output +=
        emit_module_vector(
            TEMPLATE_PARAM_VALUES_NAME,
            NAMEOF_TYPE(archimedes::any),
            template_param_exprs,
            [](const std::string *s) -> std::string {
                return emit_any(*s);
            });

    // TODO: optional RTTI
    // emit type index hashes array
    output +=
        emit_module_vector(
            TYPE_ID_HASHES_NAME,
            "size_t",
            type_id_hash_exprs,
            [](const std::string &s) { return s; });

    constexpr auto
        FUNCTIONS_NAME = "_module_functions",
        TYPES_NAME = "_module_types",
        TYPEDEFS_NAME = "_module_typedefs",
        NAMESPACE_ALIASES_NAME = "_module_aliases",
        NAMESPACE_USINGS_NAME = "_module_usings";

    output +=
        emit_serialized(
            FUNCTIONS_NAME,
            [&](std::ostream &os) {
                serialize(os, ctx.functions);
            });

    output +=
        emit_serialized(
            TYPES_NAME,
            [&](std::ostream &os) {
                serialize(os, ctx.types);
            });

    output +=
        emit_serialized(
            TYPEDEFS_NAME,
            [&](std::ostream &os) {
                serialize(os, ctx.typedefs);
            });

    output +=
        emit_serialized(
            NAMESPACE_ALIASES_NAME,
            [&](std::ostream &os) {
                serialize(os, ctx.namespace_aliases);
            });

    output +=
        emit_serialized(
            NAMESPACE_USINGS_NAME,
            [&](std::ostream &os) {
                serialize(os, ctx.namespace_usings);
            });

    // emit loader
    output += fmt::format(R"(
        static const auto _module_loader =
            ({}::instance().load_module({}, {}, {}, {}, {}, {}, {}, {}, {}, {}), 0);
        )",
        NAMEOF_TYPE(archimedes::detail::registry),
        DYNCASTS_NAME,
        CONSTEXPR_VALUES_NAME,
        INVOKERS_NAME,
        TEMPLATE_PARAM_VALUES_NAME,
        TYPE_ID_HASHES_NAME,
        FUNCTIONS_NAME,
        TYPES_NAME,
        TYPEDEFS_NAME,
        NAMESPACE_ALIASES_NAME,
        NAMESPACE_USINGS_NAME);

    // add all emitted headers headers as #include-s at top of file
    std::string headers;
    // TODO: use emitted_headers
    for (const auto &h : ctx.found_headers) {
        headers +=
            fmt::format(
                "#include \"{}\"\n",
                fs::relative(h, ctx.output_path.parent_path())
                    .string());
    }

    output = headers + output;
    return output;
}

#include "invoker.hpp"
#include "plugin.hpp"
#include "emit.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclGroup.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Type.h>
#include <clang/AST/QualTypeNames.h>
#include <clang/AST/NestedNameSpecifier.h>

#include <nameof.hpp>

using namespace archimedes;
using namespace archimedes::detail;

static constexpr auto
    ARGS_NAME = "args";

static bool can_emit_parameters(
    const Context &ctx,
    const clang::FunctionProtoType &type) {
    for (const auto &qt : type.param_types()) {
        if (!can_emit_type(ctx, qt.getTypePtrOrNull())) {
            return false;
        }
    }

    return true;
}

bool archimedes::can_make_implicit_invoker(
    const Context &ctx,
    const clang::Type &type,
    const clang::CXXRecordDecl &parent_decl,
    ImplicitFunction function_kind) {
    if (!is_decl_public(parent_decl)
        || !can_emit_type(ctx, &type)
        || !can_emit_type(
            ctx,
            ctx.ast_ctx->getRecordType(
                clang::dyn_cast<clang::RecordDecl>(&parent_decl))
                .getTypePtrOrNull())
        || !ctx.is_in_found_header(parent_decl)) {
        return false;
    }

    // abstract? cannot generate any ctor/assign, but dtor can be generated if
    // it is virtual
    if (parent_decl.isAbstract()) {
        const auto *dd = parent_decl.getDestructor();
        return dd
            && function_kind == ImplicitFunction::DTOR
            && dd->isVirtual();
    }

    const auto *ft = clang::dyn_cast<clang::FunctionProtoType>(&type);
    return ft && can_emit_parameters(ctx, *ft);
}

bool archimedes::can_make_invoker(
    const Context &ctx,
    const clang::FunctionDecl &decl) {
    // cannot emit for:
    // /TODO
    // - no definition (don't want to generate invokers for functions which we
    //   wouldn't normally be able to see/force template instantiations)
    // /END TODO
    // - non-public functions
    // - deleted functions
    // - generally not-emittable functions (see can_emit_function)
    // - functions in anonymous namespaces
    if (!is_decl_public(decl)
        || decl.isDeleted()
        || !can_emit_function(ctx, decl)
        || decl.isInAnonymousNamespace()) {
        return false;
    }

    // no functions with "requires" constraints
    if (decl.isFunctionTemplateSpecialization()) {
        const auto *ftd = decl.getPrimaryTemplate();
        if (ftd->hasAssociatedConstraints()) {
            return false;
        }
    }

    const auto *md = clang::dyn_cast<clang::CXXMethodDecl>(&decl);

    // no static free functions
    if (!md
            && !ctx.is_in_found_header(decl)
            && decl.isStatic()) {
        return false;
    }

    // check if abstract ctor
    if (const auto *cd =
            clang::dyn_cast<clang::CXXConstructorDecl>(&decl)) {
        if (cd->getParent()->isAbstract()) {
            return false;
        }
    }

    // check if method and defining type is not visible in one of headers
    // we can't "extern" for types we aren't aware of, nor can we generate
    // constructors/etc.
    if (md && !can_emit_type(ctx, md->getParent()->getTypeForDecl())) {
        return false;
    }

    /* const auto *ft = */
    /*     clang::dyn_cast_or_null<clang::FunctionProtoType>( */
    /*         decl.getFunctionType()); */
    /* return ft && can_emit_parameters(ctx, *ft); */
    // TODO: should always be able to emit parameters if function is visible?
    return true;
}

// make comma separated list of parameters as archimedes::any::as<...> exprs
// "offset" is offset into "args" array
// "count" is number of parameters to generate - defaults to all
static std::string make_params(
    Context &ctx,
    clang::ArrayRef<clang::QualType> param_types,
    size_t offset,
    ssize_t count = -1) {
    if (count == -1) {
        count = param_types.size();
    }

    vector<std::string> ss;
    for (ssize_t i = 0; i < count; i++) {
        const auto &type = param_types[i];
        ctx.register_emitted_type(*type);
        ss.push_back(
            fmt::format(
                "{}[{}].as<{}>()",
                ARGS_NAME,
                offset + i,
                get_full_type_name(
                    ctx,
                    type,
                    TNF_ARRAYS_TO_POINTERS)));
    }
    return fmt::format("{}", fmt::join(ss, ","));
}

// make target for a function call (only method decls, not needed for free
// functions)
static std::string make_target(
    Context &ctx,
    const clang::CXXMethodDecl &md,
    bool with_call_operator = true,
    bool as_reference = false) {
    ctx.register_emitted_type(*md.getThisObjectType());

    if (md.getRefQualifier() == clang::RefQualifierKind::RQ_RValue) {
        return
            fmt::format(
                "std::move({}[0].as<{}&&>()){}",
                ARGS_NAME,
                get_full_type_name(ctx, md.getThisType()->getPointeeType()),
                with_call_operator ? "." : "");
    } else if (md.getRefQualifier() == clang::RefQualifierKind::RQ_LValue) {
        return
            fmt::format(
                "{}[0].as<{}&>(){}",
                ARGS_NAME,
                get_full_type_name(ctx, md.getThisType()->getPointeeType()),
                with_call_operator ? "." : "");
    } else {
        // "getThisType" includes pointer, so as<...> is pointer type
        if (as_reference) {
            return
                fmt::format(
                    "{}[0].as<{}&>(){}",
                    ARGS_NAME,
                    get_full_type_name(ctx, md.getThisType()->getPointeeType()),
                    with_call_operator ? "." : "");
        } else {
            return
                fmt::format(
                    "{}[0].as<{}>(){}",
                    ARGS_NAME,
                    get_full_type_name(ctx, *md.getThisType()),
                    with_call_operator ? "->" : "");
        }
    }
}

// the number of "extra" parameters (those not listed in the decl's type) needed
// to call the function specified by the decl.
// fx. constructors, method calls, etc. need an extra parameter for the target
// object
static size_t num_extra_parameters(const clang::FunctionDecl &decl) {
    if (const auto *md = clang::dyn_cast<clang::CXXMethodDecl>(&decl)) {
        return md->isStatic() ? 0 : 1;
    } else if (clang::isa<clang::CXXConstructorDecl>(&decl)) {
        return 1;
    }

    // offset is also 0 for converters, destructors, because they have no
    // parameters
    return 0;
}

// make a call format string for the specified function decl, leaving one format
// specifier in the string for the function call parameters
static std::string make_call_fmt(
    Context &ctx,
    const clang::FunctionDecl &decl) {

    // get funtion name for emit
    // use full name (Foo::bar::...) if:
    // - not a method (free function)
    // - is static method
    const auto base_name =
        !clang::isa<clang::CXXMethodDecl>(&decl) || decl.isStatic() ?
            decl.getQualifiedNameAsString()
            : decl.getNameAsString();

    std::string function_name = base_name;

    // TODO
    /* if (decl.isFunctionTemplateSpecialization()) { */
    /*     if (const auto *args_as_written = */
    /*             decl.getTemplateSpecializationArgsAsWritten()) { */
    /*         llvm::SmallVector<clang::TemplateArgument, 4> args; */
    /*         for (const auto &tal : args_as_written->arguments()) { */
    /*             args.push_back(tal.getArgument()); */
    /*         } */

    /*         if (args.size() > 0) { */
    /*             const auto args_str = get_template_args_for_emit(ctx, args); */
    /*             ASSERT(args_str, "bad args for {}", decl.getQualifiedNameAsString()); */
    /*             function_name = fmt::format("{}<{}>", base_name, *args_str); */
    /*         } */
    /*     } */
    /* } */

    if (decl.isFunctionTemplateSpecialization()) {
        const auto args_str =
            get_template_args_for_emit(
                ctx,
                decl.getTemplateSpecializationArgs()->asArray());
        ASSERT(args_str, "bad args for {}", decl.getQualifiedNameAsString());
        function_name = fmt::format("{}<{}>", base_name, *args_str);
    }

    std::string result;

    if (const auto *cd =
            clang::dyn_cast_or_null<clang::CXXConstructorDecl>(&decl)) {
        ctx.register_emitted_type(*cd->getThisObjectType());
        result =
            fmt::format(
                R"(
                    new (args[0].as<{}>()) {}({{}})
                )",
                escape_for_fmt(emit_type(ctx, cd->getThisType())),
                escape_for_fmt(emit_type(ctx, *cd->getThisObjectType())));
    }  else if (
        const auto *cd =
            clang::dyn_cast_or_null<clang::CXXConversionDecl>(&decl)) {
        // don't call by name (this would involve emitting the name of a
        // function which contains a type... nasty), just use constructor syntax
        ctx.register_emitted_type(*decl.getReturnType());
        result =
            fmt::format(
                R"(
                    static_cast<{}>({}){{}}
                )",
                escape_for_fmt(get_full_type_name(ctx, decl.getReturnType())),
                escape_for_fmt(make_target(ctx, *cd, false, true)));
    } else if (
        const auto *dd =
            clang::dyn_cast_or_null<clang::CXXDestructorDecl>(&decl)) {
        result =
            fmt::format(
                R"(
                    {}{}({{}})
                )",
                escape_for_fmt(make_target(ctx, *dd)),
                escape_for_fmt(function_name));
    } else if (
        const auto *md =
            clang::dyn_cast_or_null<clang::CXXMethodDecl>(&decl)) {
        if (md->isStatic()) {
            result =
                fmt::format(
                    R"(
                        {}({{}})
                    )",
                    escape_for_fmt(function_name));
        } else {
            result =
                fmt::format(
                    R"(
                        {}{}({{}})
                    )",
                    escape_for_fmt(make_target(ctx, *md)),
                    escape_for_fmt(function_name));
        }
    } else {
        // regular function decl (free function)
        result =
            fmt::format(
                R"(
                    {}({{}})
                )",
                escape_for_fmt(function_name));
    }

    return result;
}

// get return string with two format specifiers
// expected to be formatter with:
// (0: return value expression)
// (1: invoke result success function)
static std::string make_return_fmt(const Invoker &i) {
    std::string result;

    const auto return_type = i.type->getReturnType();
    if (return_type->isVoidType()) {
        // void type -> just return invoke_result::success();
        result =
            fmt::format(
                "{{1}}; return {{0}}();",
                NAMEOF_TYPE(invoke_result));
    } else if (return_type->isRValueReferenceType()) {
        // rvalue references must be std::move'd again
        result =
            fmt::format(
                "return {{}}({}::make_reference(std::move({{}})));",
                NAMEOF_TYPE(any));
    } else if (return_type->isReferenceType()) {
        // reference types -> make_reference (return as pointers internally)
        result =
            fmt::format(
                "return {{}}({}::make_reference({{}}));",
                NAMEOF_TYPE(any));
    } else {
        // all other types we rely on the compiler to properly std::move things
        // around
        result =
            fmt::format(
                "return {{}}({}::make({{}}));",
                NAMEOF_TYPE(any));
    }

    return result;
}

// make an "extern <...>" decl if function will not be visible in final
// translation unit
static std::string make_extern(const Context &ctx, const Invoker &i) {
    if (!i.decl) {
        return "";
    }

    const clang::FunctionDecl &decl = *i.decl;

    // if decl is found in header, don't worry about generating an extern
    // for it as its declaration will already be #include-d in the emitted
    // code
    if (!decl.getDefinition()
        || ctx.is_in_found_header(*i.decl)
        || clang::isa<clang::CXXMethodDecl>(decl)) {
        return std::string();
    }

    if (decl.isTemplateInstantiation()) {
        ASSERT(decl.getPrimaryTemplate());
        // emit template forward decl, no need to extern
        return
            fmt::format(R"(
                    {};
                )",
                get_full_function_template_decl(
                    ctx, *decl.getPrimaryTemplate()),
                "");

    } else {
        // just emit extern
        return
            fmt::format(R"(
                    extern {};
                )",
                get_full_function_name_and_type(ctx, decl));
    }
}

std::string archimedes::emit_invoker(Context &ctx, const Invoker &i) {
    PLUGIN_LOG("{} record is {}", fmt::ptr(&i), fmt::ptr(i.record));
    PLUGIN_LOG(
        "making an invoker for {} :: {} ({})",
        i.record ? i.record->getQualifiedNameAsString() : "(no record)",
        get_full_type_name(ctx, *i.type),
        i.if_kind ?
            std::string(NAMEOF_ENUM(*i.if_kind))
            : i.decl->getNameAsString());

    // count number of default arguments, need to generate overloads for all
    // possible scenarios
    const auto num_defaults =
        i.decl ?
            std::count_if(
                i.decl->param_begin(),
                i.decl->param_end(),
                [](const auto *p) { return p->hasDefaultArg(); })
            : 0;

    // build switch cases
    std::string cases;

    // return format string is same for all cases
    std::string return_fmt = make_return_fmt(i);

    if (i.decl) {
        ctx.register_emitted_decl(*i.decl);

        // total number of function decl params, INCLUDING DEFAULTS
        const auto num_function_params = i.decl->getNumParams();

        const auto num_extras = num_extra_parameters(*i.decl);
        std::string call_fmt = make_call_fmt(ctx, *i.decl);

        for (size_t j = num_function_params - num_defaults;
             j <= num_function_params;
             j++) {
            // make i params
            std::string params =
                make_params(ctx, i.type->getParamTypes(), num_extras, j);

            // make call
            std::string call =
                fmt::vformat(call_fmt, fmt::make_format_args(params));

            // generate case for (i + extras) params
            cases +=
                fmt::format(R"(
                        case {}:
                            {}
                    )",
                    j + num_extras,
                    fmt::vformat(
                        return_fmt,
                        fmt::make_format_args(
                            fmt::format(
                                "{}::success",
                                NAMEOF_TYPE(invoke_result)),
                            call)));
        }
    } else {
        ctx.register_emitted_type(*i.record->getTypeForDecl());

        std::string params =
            make_params(ctx, i.type->getParamTypes(), 1);
        const auto type_name =
            get_full_type_name(ctx, *i.record->getTypeForDecl());
        std::string function_name = "";
        switch (*i.if_kind) {
            case ImplicitFunction::DEFAULT_CTOR:
            case ImplicitFunction::COPY_CTOR:
            case ImplicitFunction::MOVE_CTOR:
                cases +=
                    fmt::format(R"(
                        case {}:
                            new ({}[0].as<{}*>()) {}({});
                            return {}::success();
                        )",
                        i.type->getNumParams() + 1,
                        ARGS_NAME,
                        type_name,
                        type_name,
                        params,
                        NAMEOF_TYPE(invoke_result));
                break;
            case ImplicitFunction::DTOR:
                function_name = fmt::format("~{}", i.record->getNameAsString());
            case ImplicitFunction::COPY_ASSIGN:
            case ImplicitFunction::MOVE_ASSIGN:
                if (function_name.empty()) {
                    function_name = "operator=";
                }

                cases +=
                    fmt::format(R"(
                        case {}:
                            ({}[0].as<{}*>())->{}({});
                            return {}::success();
                        )",
                        i.type->getNumParams() + 1,
                        ARGS_NAME,
                        type_name,
                        function_name,
                        params,
                        NAMEOF_TYPE(invoke_result));
                break;
            default: ASSERT(false);
        }
    }

    // generate default case
    cases +=
        fmt::format(R"(
                default:
                    return {}::{};
            )",
            NAMEOF_TYPE(invoke_result),
            NAMEOF_ENUM(invoke_result::BAD_ARG_COUNT));

    std::string body =
        fmt::format(R"(
                switch ({}.size()) {{
                    {}
                }}
            )",
            ARGS_NAME,
            cases);

    auto ffn =
        i.decl ?
            get_full_function_name_type_and_specialization(
                ctx,
                *i.decl)
            : "(implicit)";
    ffn = ffn ? *ffn : get_full_function_name_and_type(ctx, *i.decl);

    return fmt::format(R"(
            {}
            {}
            static {} {}(std::span<{}> {}) {{
                {}
            }}
        )",
        ctx.emit_verbose ?
            fmt::format(
                "// {} at {}",
                *ffn,
                i.decl ?
                    location_to_string(ctx, i.decl->getBeginLoc())
                    : "(implicit)")
            : "",
        make_extern(ctx, i),
        NAMEOF_TYPE(invoke_result),
        i.generated_name(),
        NAMEOF_TYPE(any),
        ARGS_NAME,
        body);
}

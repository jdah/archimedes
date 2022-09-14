#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <optional>
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <filesystem>
#include <numeric>
#include <cstdlib>

#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclGroup.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Attr.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/QualTypeNames.h>
#include <clang/AST/NestedNameSpecifier.h>
#include <clang/AST/Mangle.h>
#include <clang/Basic/Visibility.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/RecordLayout.h>
#include <clang/Sema/Template.h>
#include <clang/CodeGen/ModuleBuilder.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/Support/StringSaver.h>

#include <archimedes.hpp>

#include "implicit_function.hpp"
#include "compile.hpp"
#include "plugin.hpp"
#include "emit.hpp"
#include "util.hpp"

using namespace archimedes;
using namespace archimedes::detail;

template struct archimedes::name_map<archimedes::detail::function_overload_set>;
template struct archimedes::name_map<archimedes::detail::function_parameter_info>;
template struct archimedes::name_map<archimedes::detail::static_field_type_info>;
template struct archimedes::name_map<archimedes::detail::field_type_info>;
template class std::vector<std::string>;

// forward declarations
static qualified_type_info get_or_make_qualified_info(
    Context &ctx,
    const clang::QualType &type,
    const clang::Decl *decl);

static type_info &get_or_make_info(
    Context &ctx,
    const clang::Type &type,
    const clang::Decl *decl);

static type_info &make_info(
    Context &ctx,
    const clang::Type &type,
    const clang::Decl *decl);

static type_info &make_unknown(
    Context &ctx,
    const clang::Type &type,
    const clang::Decl *decl);

static void try_populate(
    Context &ctx,
    type_info &info,
    const clang::Type &type,
    const clang::Decl *decl);

// global context instance
Context &Context::instance() {
    static Context context;
    return context;
}

// ONLY DEFINED AS SUCH IN PLUGIN
// type_id is abused to have each index refer to an index into the Context's
// type array
const type_info &type_id::operator*() const {
    return Context::instance().by_id(*this);
}

type_id::operator bool() const {
    return *this != type_id::none();
}

// clang access specifier -> our access specifier
static AccessSpecifier from_clang_access_specifier(clang::AccessSpecifier as) {
    switch (as) {
        case clang::AccessSpecifier::AS_none:
        case clang::AccessSpecifier::AS_private:
            return PRIVATE;
        case clang::AccessSpecifier::AS_protected:
            return PROTECTED;
        case clang::AccessSpecifier::AS_public:
            return PUBLIC;
    }
}

// clang type -> our kind
static type_kind type_to_kind(const Context &ctx, const clang::Type &type) {
    const auto &base = canonical_type(type);

    if (base.isStructureOrClassType()) {
        return STRUCT;
    } else if (base.isUnionType()) {
        return UNION;
    } else if (base.isMemberPointerType()) {
        return MEMBER_PTR;
    } else if (base.isRValueReferenceType()) {
        return RREF;
    } else if (base.isReferenceType()) {
        return REF;
    } else if (base.isArrayType()) {
        return ARRAY;
    } else if (base.isPointerType()) {
        return PTR;
    } else if (base.isFunctionType()) {
        return FUNC;
    } else if (base.isEnumeralType()) {
        return ENUM;
    } else if (base.isBuiltinType()) {
        const auto *base_bi =
            clang::dyn_cast_or_null<clang::BuiltinType>(&base);
        switch (base_bi->getKind()) {
            case clang::BuiltinType::Bool:
                return BOOL;
            case clang::BuiltinType::Char_S:
            case clang::BuiltinType::Char_U:
                return CHAR;
            case clang::BuiltinType::UChar:
                return U_CHAR;
            case clang::BuiltinType::UShort:
                return U_SHORT;
            case clang::BuiltinType::UInt:
                return U_INT;
            case clang::BuiltinType::ULong:
                return U_LONG;
            case clang::BuiltinType::ULongLong:
                return U_LONG_LONG;
            case clang::BuiltinType::SChar:
                return I_CHAR;
            case clang::BuiltinType::Short:
                return I_SHORT;
            case clang::BuiltinType::Int:
                return I_INT;
            case clang::BuiltinType::Long:
                return I_LONG;
            case clang::BuiltinType::LongLong:
                return I_LONG_LONG;
            case clang::BuiltinType::Float:
                return FLOAT;
            case clang::BuiltinType::Double:
                return DOUBLE;
            case clang::BuiltinType::Void:
                return VOID;
            default:
                return UNKNOWN;
        }
    }

    // could not identify type but it is not invalid either
    return UNKNOWN;
}

// retrieve all annotations (clang::annotate) for a declaration
static vector<std::string> get_annotations(
    const Context &ctx,
    const clang::Decl &decl) {
    vector<std::string> annos;

    for (const auto *attr : decl.getAttrs()) {
        if (attr->getKind() != clang::attr::Annotate) {
            continue;
        }

        std::string s;
        llvm::raw_string_ostream os(s);
        attr->printPretty(os, ctx.ast_ctx->getPrintingPolicy());

        const auto q_left = s.find('\"'), q_right = s.rfind('\"');
        ASSERT(
            q_left != std::string::npos && q_right != std::string::npos,
            "failure to parse annotation {}",
            s);

        annos.push_back(s.substr(q_left + 1, q_right - q_left - 1));
    }

    return annos;
}

// hash a type based on qualifies + name
static size_t hash_type(
    const Context &ctx,
    clang::QualType qt) {
    return hash(qt.getQualifiers().getAsString(), get_full_type_name(ctx, *qt));
}

// check if a function overload already exists for the specified declaration
static bool overloads_contains(
    const Context &ctx,
    const name_map<function_overload_set> &map,
    const clang::FunctionDecl &decl) {
    auto it = map.find(decl.getQualifiedNameAsString());
    if (it == map.end()) {
        return false;
    }

    const auto &fos = it->second;
    return std::any_of(
        fos.functions.begin(),
        fos.functions.end(),
        [&](const function_type_info &f) {
            return f.internal->hash == hash_type(ctx, decl.getType());
        });
}

// get or create a function overload set in "map" and insert the specified
// function type info into the set
static function_overload_set &insert_into_overloads(
    name_map<function_overload_set> &map,
    function_type_info &&info) {
    function_overload_set *fos;
    ASSERT(
        info.id->kind == FUNC,
        "inserting function {} of kind {} into function overloads",
        info.qualified_name,
        NAMEOF_ENUM(info.id->kind));
    auto it = map.find(info.qualified_name);
    if (it != map.end()) {
        fos = &it->second;
    } else {
        fos =
            &map.emplace(
                info.qualified_name,
                function_overload_set {
                    .qualified_name = info.qualified_name,
                    .name = info.name
                }).first->second;
    }
    fos->functions.emplace_back(std::move(info));
    return *fos;
}

// synthesize function type information for implicit functions (default ctor,
// move/copy assign/ctor, dtor)
static function_type_info make_implicit_function_type_info(
    Context &ctx,
    const clang::Type &type,
    const clang::CXXRecordDecl &parent_decl,
    const type_info &parent,
    ImplicitFunction function_kind) {
    const auto *ft = clang::dyn_cast<clang::FunctionType>(&type);
    ASSERT(ft);

    function_type_info info;
    info.internal = &ctx.alloc<function_type_info_internal>();
    info.internal->hash = hash_type(ctx, clang::QualType(&type, 0));

    info.parent_id = parent.id;
    info.id = get_or_make_info(ctx, type, nullptr).id;

    std::string name;
    switch (function_kind) {
        case ImplicitFunction::DEFAULT_CTOR:
        case ImplicitFunction::COPY_CTOR:
        case ImplicitFunction::MOVE_CTOR:
            name = parent_decl.getNameAsString();
            break;
        case ImplicitFunction::COPY_ASSIGN:
        case ImplicitFunction::MOVE_ASSIGN:
            name = "operator=";
            break;
        case ImplicitFunction::DTOR:
            name = fmt::format("~{}", parent_decl.getNameAsString());
            break;
        default: ASSERT(false);
    }

    info.name = name;
    info.qualified_name =
        fmt::format(
            "{}::{}",
            parent.record.qualified_name,
            name);

    info.definition_path = parent.definition_path;
    info.is_member = true;
    info.is_static = false;
    info.is_ctor = function_kind != ImplicitFunction::DTOR;
    info.is_dtor = function_kind == ImplicitFunction::DTOR;
    info.is_converter = false;
    info.is_virtual = false;
    info.is_deleted = false;
    info.is_defaulted = true;

    function_parameter_info fpi;
    switch (function_kind) {
        case ImplicitFunction::COPY_CTOR:
        case ImplicitFunction::MOVE_CTOR:
        case ImplicitFunction::COPY_ASSIGN:
        case ImplicitFunction::MOVE_ASSIGN:
            fpi.is_defaulted = false;
            fpi.index = 0;
            fpi.name = "";
            info.parameters.push_back(fpi);
            break;
        default:
            break;
    }

    if (can_make_implicit_invoker(
            ctx,
            type,
            parent_decl,
            function_kind)) {
        Invoker *invoker = ctx.make_invoker();
        invoker->type = clang::dyn_cast<clang::FunctionProtoType>(&type);
        invoker->decl = nullptr;
        invoker->record = &parent_decl;
        invoker->if_kind = function_kind;
        info.internal->invoker = invoker;
    } else {
        info.internal->invoker = nullptr;
    }

    return info;
}

static function_type_info make_function_type_info(
    Context &ctx,
    const clang::Type &type,
    const clang::FunctionDecl *decl,
    std::optional<const type_info*> parent) {
    function_type_info info;

    const auto name =
        get_full_function_name_and_specialization(ctx, *decl);
    PLUGIN_LOG(
        "making function type info for {} with type {}",
        name ? *name : decl->getQualifiedNameAsString(),
        get_full_type_name(ctx, type));

    // try to find parent if not provided (fx. with free functions)
    if (!parent) {
        if (const auto *td =
                clang::dyn_cast_or_null<clang::TagDecl>(
                    decl->getParent())) {
            parent =
                &get_or_make_info(
                    ctx,
                    *td->getTypeForDecl(),
                    td);
        }
    }

    info.parent_id = parent ? (**parent).id : type_id::none();
    info.id =
        get_or_make_info(
            ctx,
            *decl->getType(),
            decl).id;
    info.qualified_name = decl->getQualifiedNameAsString();
    info.name = decl->getNameAsString();
    info.definition_path = location_to_string(ctx, decl->getLocation());

    if (const auto *md =
            clang::dyn_cast_or_null<clang::CXXMethodDecl>(decl)) {
        info.is_member = true;
        info.is_static = md->isStatic();

        if (const auto *cd =
                clang::dyn_cast_or_null<clang::CXXConstructorDecl>(md)) {
            ASSERT(!info.is_dtor);
            info.is_ctor = true;
            info.is_explicit = cd->isExplicit();
        } else if (
            const auto *cd =
                clang::dyn_cast_or_null<clang::CXXConversionDecl>(md)) {
            info.is_converter = true;
            info.is_explicit = cd->isExplicit();
        } else if (
            const auto *dd =
                clang::dyn_cast_or_null<clang::CXXDestructorDecl>(md)) {
            ASSERT(!info.is_ctor);
            info.is_dtor = true;
        }

        info.is_virtual = md->isVirtual();
        info.is_deleted = md->isDeleted();
        info.is_defaulted = md->isDefaulted();
    }

    std::optional<std::string> parameter_pack_name;

    if (decl->isTemplateInstantiation()) {
        if (const auto *pt = decl->getPrimaryTemplate()) {
            for (const auto *p : pt->getTemplatedDecl()->parameters()) {
                if (p->isParameterPack()) {
                    parameter_pack_name = p->getNameAsString();
                }
            }
        }
    }

    for (size_t i = 0; i < decl->param_size(); i++) {
        const auto *p = decl->getParamDecl(i);
        std::string name = p->getNameAsString();
        name =
            (parameter_pack_name && name == *parameter_pack_name) ?
                fmt::format("{}...{}", *parameter_pack_name, i)
                : name;
        info.parameters.emplace_back(
            function_parameter_info {
                .name = name,
                .index = i,
                .is_defaulted = p->hasDefaultArg()
            });
    }

    info.annotations = get_annotations(ctx, *decl);
    info.internal = &ctx.alloc<function_type_info_internal>();
    info.internal->hash = hash_type(ctx, decl->getType());

    // nullptr invoker for non-accessible functions
    PLUGIN_LOG("checking to make invoker for {}", decl_name(ctx, decl));
    if (can_make_invoker(ctx, *decl)) {
        Invoker *invoker = ctx.make_invoker();
        invoker->type = clang::dyn_cast<clang::FunctionProtoType>(&type);
        invoker->decl = decl;
        if (const auto *md = clang::dyn_cast<clang::CXXMethodDecl>(decl)) {
            invoker->record = md->getParent();
        }
        PLUGIN_LOG(
            "{} got parent? {} for {}",
            fmt::ptr(invoker),
            invoker->record != nullptr,
            decl->getQualifiedNameAsString(),
            get_full_type_name(ctx, decl->getType()));
        info.internal->invoker = invoker;
    } else {
        info.internal->invoker = nullptr;
    }

    return info;
}

static void populate_ref(
    Context &ctx,
    type_info &info,
    const clang::Type &type,
    const clang::Decl *decl) {
    const auto *rt = clang::dyn_cast_or_null<clang::ReferenceType>(&type);
    ASSERT(rt, "could not cast to reference type");
    info.type =
        get_or_make_qualified_info(
            ctx, rt->getPointeeType(), nullptr);
}

static void populate_ptr(
    Context &ctx,
    type_info &info,
    const clang::Type &type,
    const clang::Decl *decl) {
    const auto *pt = clang::dyn_cast_or_null<clang::PointerType>(&type);
    ASSERT(
        pt,
        "could not cast to pointer type");
    info.type =
        get_or_make_qualified_info(
            ctx, pt->getPointeeType(), nullptr);
}

static void populate_member_ptr(
    Context &ctx,
    type_info &info,
    const clang::Type &type,
    const clang::Decl *decl) {
    const auto *mpt =
        clang::dyn_cast_or_null<clang::MemberPointerType>(&type);
    ASSERT(mpt, "could not cast to member pointer type");
    info.type =
        get_or_make_qualified_info(ctx, mpt->getPointeeType(), nullptr);
    info.member_ptr.class_type =
        get_or_make_info(ctx, *mpt->getClass(), nullptr).id;
}

static void populate_array(
    Context &ctx,
    type_info &info,
    const clang::Type &type,
    const clang::Decl *decl) {
    const auto *at = clang::dyn_cast_or_null<clang::ArrayType>(&type);
    ASSERT(at, "could not cast to array type");
    info.type = get_or_make_qualified_info(ctx, at->getElementType(), nullptr);

    // unknown length if not covered in one of the cases below
    info.array.length = 0;

    // NOTE: VariableArrayType and IncompleteArrayType have unknown lengths,
    // they intentionally have an array.length of 0
    if (const auto *cat =
            clang::dyn_cast_or_null<clang::ConstantArrayType>(at)) {
        info.array.length = cat->getSize().getLimitedValue();
    } else if (
        const auto *dat =
            clang::dyn_cast_or_null<clang::DependentSizedArrayType>(at)) {
        const auto *size_expr = dat->getSizeExpr();
        if (size_expr->isIntegerConstantExpr(*ctx.ast_ctx)) {
            if (const auto val =
                    size_expr->getIntegerConstantExpr(
                        *ctx.ast_ctx)) {
                info.array.length = val->getLimitedValue();
            }
        }
    }
}

// TODO: doc
static clang::QualType make_implicit_function_type(
    const Context &ctx,
    const clang::CXXRecordDecl &record,
    ImplicitFunction if_kind) {

    // make return type
    clang::QualType return_type;
    switch (if_kind) {
        case ImplicitFunction::DEFAULT_CTOR:
        case ImplicitFunction::COPY_CTOR:
        case ImplicitFunction::MOVE_CTOR:
        case ImplicitFunction::DTOR:
            return_type = ctx.ast_ctx->VoidTy;
            break;
        case ImplicitFunction::COPY_ASSIGN:
        case ImplicitFunction::MOVE_ASSIGN:
            return_type =
                clang::QualType(
                    ctx.sema->BuildReferenceType(
                        clang::QualType(record.getTypeForDecl(), 0),
                        true,
                        record.getBeginLoc(),
                        clang::DeclarationName()));
            break;
        default: ASSERT(false);
    }

    // make parameter type
    std::optional<clang::QualType> param_type;
    switch (if_kind) {
        case ImplicitFunction::DEFAULT_CTOR:
        case ImplicitFunction::DTOR:
            break;
        case ImplicitFunction::COPY_CTOR:
        case ImplicitFunction::COPY_ASSIGN:
            param_type = clang::QualType(record.getTypeForDecl(), 0);
            param_type->addConst();
            param_type =
                clang::QualType(
                    ctx.sema->BuildReferenceType(
                        *param_type,
                        true,
                        record.getBeginLoc(),
                        clang::DeclarationName()));
            break;
        case ImplicitFunction::MOVE_CTOR:
        case ImplicitFunction::MOVE_ASSIGN:
            param_type =
                clang::QualType(
                    ctx.sema->BuildReferenceType(
                        clang::QualType(record.getTypeForDecl(), 0),
                        false,
                        record.getBeginLoc(),
                        clang::DeclarationName()));
            break;
        default: ASSERT(false);
    }

    const auto res = ctx.sema->BuildFunctionType(
        return_type,
        param_type ?
            llvm::MutableArrayRef<clang::QualType>(*param_type)
            : llvm::MutableArrayRef<clang::QualType>(),
        record.getBeginLoc(),
        clang::DeclarationName(),
        clang::FunctionProtoType::ExtProtoInfo());
    return res;
}

// TODO: doc
static bool function_has_constraints(
    const Context &ctx,
    const clang::FunctionDecl *decl) {
    return decl->getTrailingRequiresClause();
}

// TODO: doc
static bool function_satisfies_constraints(
    const Context &ctx,
    const clang::FunctionDecl *decl) {
    if (decl->isIneligibleOrNotSelected()) {
        return false;
    }

    if (decl->getTrailingRequiresClause()) {
        // method has requires clause - check if it is satisfied
        clang::ConstraintSatisfaction cs;
        if (ctx.sema->CheckFunctionConstraints(decl, cs)
            || !cs.IsSatisfied) {
            return false;
        }
    }

    return true;
}

static void populate_record(
    Context &ctx,
    type_info &info,
    const clang::Type &type,
    const clang::Decl *decl) {
    const auto *rt =
        clang::dyn_cast_or_null<clang::RecordType>(&type);
    ASSERT(rt, "could not cast to record type");

    if (!decl) {
        decl = rt->getDecl();
    }

    const auto *rd =
        clang::dyn_cast_or_null<clang::CXXRecordDecl>(decl);

    if (rd) {
        ctx.add_header_if_not_present(*rd);
    }

    if (!rd || !rd->hasDefinition()) {
        PLUGIN_LOG(
            "{} has no decl or definition, UNKNOWN-ing",
            get_full_type_name(ctx, type));
        info.kind = UNKNOWN;
        return;
    }

    rd = rd->getDefinition();
    ctx.add_header_if_not_present(*rd);

    PLUGIN_LOG(
        "populating record {}",
        get_full_type_name(ctx, type));

    // get template parameters
    if (const auto *ctsd =
            clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(rd)) {
        template_parameter_info tpi;
        tpi.internal = &ctx.alloc<template_parameter_info_internal>();

        const auto &params =
            *ctsd->getSpecializedTemplate()->getTemplateParameters();
        const auto &args = ctsd->getTemplateArgs();
        for (size_t i = 0; i < args.size(); i++) {
            const auto &ta = args[i];
            llvm::APSInt int_value;
            tpi.name = params.getParam(i)->getNameAsString();
            switch (ta.getKind()) {
                case clang::TemplateArgument::ArgKind::Type:
                    tpi.type =
                        get_or_make_qualified_info(
                            ctx, ta.getAsType(), nullptr);
                    tpi.is_typename = true;
                    break;
                case clang::TemplateArgument::ArgKind::Integral:
                    int_value = ta.getAsIntegral();
                    tpi.type =
                        get_or_make_qualified_info(
                            ctx,
                            ctx.ast_ctx->getIntTypeForBitwidth(
                                int_value.getBitWidth(),
                                int_value.isSigned()),
                            nullptr);
                    tpi.is_typename = false;
                    tpi.internal->value_expr =
                        f_ostream_to_string(
                            [&](auto &os) {
                                int_value.print(os, int_value.isSigned());
                            });
                    break;
                case clang::TemplateArgument::ArgKind::Expression:
                    tpi.type =
                        get_or_make_qualified_info(
                            ctx, ta.getAsExpr()->getType(), nullptr);
                    tpi.is_typename = false;
                    tpi.internal->value_expr =
                        get_fully_qualified_expr(ctx, *ta.getAsExpr());
                    break;
                case clang::TemplateArgument::ArgKind::NullPtr:
                    tpi.type =
                        get_or_make_qualified_info(
                            ctx, ctx.ast_ctx->NullPtrTy, nullptr);
                    tpi.is_typename = false;
                    tpi.internal->value_expr = "nullptr";
                    break;
                default:
                    // TODO: support more kinds of template arguments?
                    break;
            }

            PLUGIN_LOG(
                "got template param {} on {}",
                tpi.name,
                get_full_type_name(ctx, type));
            info.record.template_parameters.push_back(tpi);
        }
    }

    info.record.qualified_name = get_full_type_name(ctx, type);

    const auto &layout = ctx.ast_ctx->getASTRecordLayout(rd);
    info.record.size = layout.getSize().getQuantity();
    info.record.alignment = layout.getAlignment().getQuantity();
    info.record.is_dynamic = rd->isDynamicClass();
    info.record.is_pod = rd->isPOD();

    info.record.has_trivial_default_ctor = rd->hasTrivialDefaultConstructor();
    info.record.has_trivial_copy_ctor = rd->hasTrivialCopyConstructor();
    info.record.has_trivial_copy_assign = rd->hasTrivialCopyAssignment();
    info.record.has_trivial_move_ctor = rd->hasTrivialMoveConstructor();
    info.record.has_trivial_move_assign = rd->hasTrivialMoveAssignment();
    info.record.has_trivial_dtor = rd->hasTrivialDestructor();
    info.record.is_trivially_copyable = rd->isTriviallyCopyable();
    info.record.is_abstract = rd->isAbstract();
    info.record.is_polymorphic = rd->isPolymorphic();

    // get methods
    for (const auto *md : rd->methods()) {
        if (ctx.should_ignore(md)) {
            continue;
        } else if (
            /* function_has_constraints(ctx, md)) { */
            !function_satisfies_constraints(ctx, md)) {
            PLUGIN_LOG(
                "skipping {} {} because constraints are not satisfied",
                get_full_type_name(ctx, md->getType()),
                md->getQualifiedNameAsString());
            continue;
        }

        if (!overloads_contains(ctx, info.record.functions, *md)) {
            PLUGIN_LOG(
                "inserting new overload {}/{}",
                md->getQualifiedNameAsString(),
                get_full_type_name(ctx, md->getType()));
            insert_into_overloads(
                info.record.functions,
                make_function_type_info(
                    ctx,
                    *md->getType(),
                    md,
                    &info));
        }
    }

    // generate implicit function decls
    const auto add_implicit =
        [&](ImplicitFunction if_kind) {
            // TODO: add implicits??
            const auto type =
                make_implicit_function_type(
                    ctx, *rd, if_kind);

            // check that overloads don't already include this function
            auto it =
                info.record.functions.find(
                    get_implicit_name(if_kind, rd->getQualifiedNameAsString()));

            if (it != info.record.functions.end()) {
                auto &fos = it->second;
                if (std::any_of(
                        fos.functions.begin(),
                        fos.functions.end(),
                        [&](const function_type_info &f) {
                            return f.internal->hash == hash_type(ctx, type);
                        })) {
                    PLUGIN_LOG(
                        "not adding implicit of kind {} to type {}",
                        NAMEOF_ENUM(if_kind),
                        rd->getQualifiedNameAsString());
                    return;
                }
            }

            insert_into_overloads(
                info.record.functions,
                make_implicit_function_type_info(
                    ctx,
                    *type,
                    *rd,
                    info,
                    if_kind));
        };

    if (!rd->hasUserProvidedDefaultConstructor()
        && (rd->needsImplicitDefaultConstructor()
            || rd->hasTrivialDefaultConstructor())
        && !ctx.sema->LookupDefaultConstructor(
            const_cast<clang::CXXRecordDecl*>(rd))->isDeleted()) {
        add_implicit(ImplicitFunction::DEFAULT_CTOR);
    }

    if (!rd->hasUserDeclaredCopyConstructor()
        && (rd->needsImplicitCopyConstructor()
            || rd->hasTrivialCopyConstructor())
        && rd->hasSimpleCopyConstructor()) {
        add_implicit(ImplicitFunction::COPY_CTOR);
    }

    if (!rd->hasUserDeclaredMoveConstructor()
        && (rd->needsImplicitMoveConstructor()
            || rd->hasTrivialMoveConstructor())
        && rd->hasSimpleMoveConstructor()) {
        add_implicit(ImplicitFunction::MOVE_CTOR);
    }

    if (!rd->hasUserDeclaredCopyAssignment()
        && (rd->needsImplicitCopyAssignment()
            || rd->hasTrivialCopyAssignment())
        && rd->hasSimpleCopyAssignment()) {
        add_implicit(ImplicitFunction::COPY_ASSIGN);
    }

    if (!rd->hasUserDeclaredMoveAssignment()
        && (rd->needsImplicitMoveAssignment()
            || rd->hasTrivialMoveAssignment())
        && rd->hasSimpleMoveAssignment()) {
        add_implicit(ImplicitFunction::MOVE_ASSIGN);
    }

    if (!rd->hasUserDeclaredDestructor()
        && (rd->needsImplicitDestructor()
            || rd->hasTrivialDestructor())
        && rd->hasSimpleDestructor()) {
        add_implicit(ImplicitFunction::DTOR);
    }

    // get base classes
    for (const auto &base : rd->bases()) {
        const auto *base_rt =
            clang::dyn_cast_or_null<clang::RecordType>(
                &canonical_type(*base.getType()));
        ASSERT(
            base_rt,
            "could not get base record type for {} ({})",
            get_full_type_name(ctx, base.getType()),
            base.getType()->getTypeClassName());
        const auto *base_rd = base_rt->getAsCXXRecordDecl()->getDefinition();
        struct_base_type_info sbti;
        sbti.parent_id = info.id;
        sbti.id =
            get_or_make_info(ctx, *base.getType(), nullptr).id;
        sbti.access =
            from_clang_access_specifier(base.getAccessSpecifier());
        sbti.is_virtual = base.isVirtual();
        sbti.is_primary = base_rd == layout.getPrimaryBase();
        sbti.is_vbase = false;
        sbti.offset =
            (base.isVirtual() ?
                layout.getVBaseClassOffset(base_rd)
                : layout.getBaseClassOffset(base_rd)).getQuantity();
        sbti.internal = &ctx.alloc<struct_base_type_info_internal>();
        sbti.internal->definition = base_rd;
        info.record.bases.push_back(sbti);
    }

    // virtual bases
    for (const auto &vbase : rd->vbases()) {
        const auto *vbase_rd =
            clang::dyn_cast<clang::CXXRecordDecl>(
                vbase.getType()->getAsRecordDecl()->getDefinition());
        ASSERT(vbase_rd);

        // already found in regular bases? mark sbti as "is_vbase"
        bool exists = false;
        for (auto &sbti : info.record.bases) {
            if (sbti.internal->definition == vbase_rd) {
                sbti.is_vbase = true;
                exists = true;
            }
        }

         if (exists) {
             continue;
         }

         // not found -> add as new virtual base
        struct_base_type_info sbti;
        sbti.parent_id = info.id;
        sbti.id =
            get_or_make_info(ctx, *vbase.getType(), nullptr).id;
        sbti.access =
            from_clang_access_specifier(vbase.getAccessSpecifier());
        sbti.is_virtual = vbase.isVirtual();
        sbti.is_primary = false;
        sbti.is_vbase = true;
        sbti.offset = layout.getVBaseClassOffset(vbase_rd).getQuantity();
        sbti.internal = &ctx.alloc<struct_base_type_info_internal>();
        sbti.internal->definition = vbase_rd;
        info.record.bases.push_back(sbti);
    }

    // get fields
    for (const auto *field : rd->fields()) {
        field_type_info fti;
        fti.parent_id = info.id;
        fti.type =
            get_or_make_qualified_info(
                ctx,
                field->getType(),
                nullptr);
        fti.name = field->getNameAsString();
        fti.access = from_clang_access_specifier(field->getAccess());
        fti.is_mutable = field->isMutable();
        fti.is_bit_field = field->isBitField();
        fti.bit_size =
            field->isBitField() ? field->getBitWidthValue(*ctx.ast_ctx) : 0;
        fti.bit_offset =
            field->isBitField() ?
                layout.getFieldOffset(field->getFieldIndex())
                : 0;
        fti.annotations = get_annotations(ctx, *field);
        fti.size =
            ctx.ast_ctx->getTypeSizeInChars(field->getType())
                .getQuantity();
        fti.offset =
            ctx.ast_ctx->toCharUnitsFromBits(
                layout.getFieldOffset(field->getFieldIndex()))
                    .getQuantity();
        info.record.fields[fti.name] = fti;
    }

    // get static fields from VarDecls on the record decl
    for (const auto *d : clang::dyn_cast<clang::DeclContext>(rd)->decls()) {
        const auto *vd = clang::dyn_cast_or_null<clang::VarDecl>(d);
        if (!vd) {
            continue;
        }

        if (!vd->isStaticDataMember()) {
            continue;
        }

        PLUGIN_LOG("got static field {}", vd->getQualifiedNameAsString());
        static_field_type_info sfti;
        sfti.parent_id = info.id;
        sfti.type =
            get_or_make_qualified_info(
                ctx,
                vd->getType(),
                vd);
        sfti.name = vd->getNameAsString();
        sfti.access = from_clang_access_specifier(vd->getAccess());
        sfti.is_constexpr = vd->isConstexpr();
        sfti.annotations = get_annotations(ctx, *vd);
        sfti.internal = &ctx.alloc<static_field_type_info_internal>();

        if (vd->isConstexpr()) {
            sfti.internal->constexpr_type = vd->getType();
            PLUGIN_LOG(
                "  static field {} is constexpr",
                vd->getQualifiedNameAsString());

            // try to add header if static field def could be in another file
            if (!ctx.is_in_found_header(*vd)) {
                ctx.add_header_if_not_present(*vd);
            }

            // TODO: evaluate non-public constexprs, maybe try a hacky memcpy?
            if (is_decl_public(*vd)
                    && ctx.is_in_found_header(*vd)) {
                PLUGIN_LOG(
                    "  static field {} has value",
                    vd->getQualifiedNameAsString());
                // will this decl be visible in the final translation unit?
                // refer to it by name
                sfti.internal->constexpr_expr =
                    fmt::format(
                        //"static_cast<decltype({0})>({0})",
                        "({0})",
                        vd->getQualifiedNameAsString());
            } else {
                PLUGIN_LOG(
                    "  static field {} cannot emit value: {} {}",
                    vd->getQualifiedNameAsString(),
                    is_decl_public(*vd),
                    ctx.is_in_found_header(*vd));
            }
        }

        info.record.static_fields.emplace(sfti.name, std::move(sfti));
    }

    // get typedefs (typedef/using)
    for (const auto *decl : rd->decls()) {
        if (const auto *td = clang::dyn_cast<clang::TypedefNameDecl>(decl)) {
            td = td->getCanonicalDecl();
            const auto name = td->getQualifiedNameAsString();
            info.record.typedefs.emplace(
                name,
                typedef_info {
                    .name = name,
                    .aliased_type =
                        get_or_make_qualified_info(
                            ctx,
                            td->getUnderlyingType(),
                            nullptr)
                });
        }
    }

    // ensure that pointers, references to types are added
    get_or_make_info(
        ctx,
        *ctx.ast_ctx->getPointerType(clang::QualType(&type, 0)),
        nullptr);
    get_or_make_info(
        ctx,
        *ctx.ast_ctx->getPointerType(
            clang::QualType(&type, clang::Qualifiers::TQ::Const)),
        nullptr);
    get_or_make_info(
        ctx,
        *ctx.ast_ctx->getLValueReferenceType(
            clang::QualType(&type, 0)),
        nullptr);
    get_or_make_info(
        ctx,
        *ctx.ast_ctx->getLValueReferenceType(
            clang::QualType(&type, clang::Qualifiers::TQ::Const)),
        nullptr);
    get_or_make_info(
        ctx,
        *ctx.ast_ctx->getRValueReferenceType(
            clang::QualType(&type, 0)),
        nullptr);
}

static void populate_func(
    Context &ctx,
    type_info &info,
    const clang::Type &type,
    const clang::Decl *decl) {
    if (clang::isa<clang::FunctionNoProtoType>(&type)) {
        // cannot process without prototype, mark as unknown
        info.internal->is_resolved = false;
        return;
    }

    const auto *ft = clang::dyn_cast<clang::FunctionProtoType>(&type);
    info.function.return_type =
        get_or_make_qualified_info(ctx, ft->getReturnType(), nullptr);

    for (const auto &param_type : ft->getParamTypes()) {
        info.function.parameters.push_back(
            get_or_make_qualified_info(
                ctx,
                param_type,
                nullptr));
    }

    info.function.is_const = ft->isConst();
    info.function.is_rref =
    info.function.is_rref =
        ft->getRefQualifier() == clang::RefQualifierKind::RQ_RValue;
    info.function.is_ref =
        ft->getRefQualifier() == clang::RefQualifierKind::RQ_LValue;
}

static void populate_enum(
    Context &ctx,
    type_info &info,
    const clang::Type &type,
    const clang::Decl *decl) {
    auto *ed = clang::dyn_cast_or_null<clang::EnumDecl>(decl);

    if (!ed || !ed->getDefinition()) {
        info.internal->is_resolved = false;
        return;
    }

    ed = ed->getDefinition();

    info.enum_.base_type =
        get_or_make_qualified_info(ctx, ed->getIntegerType(), nullptr);

    for (const auto &e : ed->enumerators()) {
        const auto name = e->getNameAsString();
        const decltype(info.enum_.name_to_value)::mapped_type value =
            e->getInitVal().getLimitedValue(
                std::numeric_limits<std::uintmax_t>::max());
        info.enum_.name_to_value[name] = value;

        auto it = info.enum_.value_to_name.find(value);
        (it == info.enum_.value_to_name.end() ?
            info.enum_.value_to_name.emplace(
                value, vector<std::string>()).first
            : it)->second.push_back(name);
    }
}

qualified_type_info get_or_make_qualified_info(
    Context &ctx,
    const clang::QualType &type,
    const clang::Decl *decl) {
    return qualified_type_info {
        .id = get_or_make_info(ctx, *type, decl).id,
        .is_const = type.isConstQualified(),
        .is_volatile = type.isVolatileQualified()
    };
}

type_info &get_or_make_info(
    Context &ctx,
    const clang::Type &type,
    const clang::Decl *decl) {
    const auto &c_type = canonical_type(type);

    // try to get type decl if not present
    decl = decl ? decl : try_get_type_definition(c_type);

    // ignore
    if (decl && ctx.should_ignore(decl)) {
        return make_unknown(ctx, c_type, decl);
    }

    if (const auto info_opt = ctx.try_get(c_type, decl)) {
        auto &info = **info_opt;

        if (!info.internal->is_resolved) {
            try_populate(ctx, info, c_type, decl);
        }

        return info;
    }

    return make_info(ctx, c_type, decl);
}

void try_populate(
    Context &ctx,
    type_info &info,
    const clang::Type &type,
    const clang::Decl *decl) {
    info.kind = type_to_kind(ctx, type);

    if (info.kind == UNKNOWN) {
        info.internal->is_resolved = false;
        return;
    }

    // get size/align of type
    if (!type.isSizelessType()) {
        if (const auto *rt = clang::dyn_cast<clang::RecordType>(&type)) {
            const auto *rd = rt->getDecl();
            // guard against getASTRecordLayout crashing on incomplete record
            // definitions
            if (!rd
                || !rd->getDefinition()
                || rd->isInvalidDecl()
                || !rd->getDefinition()->isCompleteDefinition()) {
                info.size = 0;
                info.align = 0;
            } else {
                const auto &layout = ctx.ast_ctx->getASTRecordLayout(rd);
                info.size = layout.getSize().getQuantity();
                info.align = layout.getAlignment().getQuantity();
            }
        } else {
            const auto type_info = ctx.ast_ctx->getTypeInfoInChars(&type);
            info.size = type_info.Width.getQuantity();
            info.align = type_info.Align.getQuantity();
        }
    } else {
        info.size = 0;
        info.align = 0;
    }

    // ensure that records, ALWAYS have their definition as their decl
    bool need_decl = true, need_definition = true, have_definition = false;
    if (const auto *rd =
            clang::dyn_cast_or_null<clang::CXXRecordDecl>(decl)) {
        const auto *def = rd->getDefinition();
        decl = def ? def : decl;
        have_definition = def;
    } else if (
        const auto *ed =
            clang::dyn_cast_or_null<clang::EnumDecl>(decl)) {
        const auto *def = ed->getDefinition();
        decl = def ? def : decl;
        have_definition = def;
    } else {
        need_decl = false;
        need_definition = false;
    }

    // unresolvable? mark, will be resolved later
    // also reset kind to unknown since this type has not been populated at all
    if ((need_decl && !decl) || (need_definition && !have_definition)) {
        info.internal->is_resolved = false;
        return;
    }

    // tag decl types have one place of definition, so get path and annotations
    // from there
    if (decl && have_definition
        && (info.kind == ENUM || info.kind == STRUCT || info.kind == UNION)) {
        info.definition_path = location_to_string(ctx, decl->getLocation());
        info.annotations = get_annotations(ctx, *decl);
    }

    // mark as resolved before recursing, otherwise functions which reference
    // themselves will loop infinitely
    info.internal->is_resolved = true;

    // populate according to type
    switch (info.kind) {
        case REF:
        case RREF:
            populate_ref(ctx, info, type, decl);
            break;
        case PTR:
            populate_ptr(ctx, info, type, decl);
            break;
        case MEMBER_PTR:
            populate_member_ptr(ctx, info, type, decl);
            break;
        case ARRAY:
            populate_array(ctx, info, type, decl);
            break;
        case STRUCT:
        case UNION:
            populate_record(ctx, info, type, decl);
            break;
        case FUNC:
            populate_func(ctx, info, type, decl);
            break;
        case ENUM:
            populate_enum(ctx, info, type, decl);
            break;
        default:
            break;
    }
}

type_info &make_info(
    Context &ctx,
    const clang::Type &type,
    const clang::Decl *decl) {
    const auto &c_type = canonical_type(type);
    auto &info = ctx.new_info(c_type, decl);
    try_populate(ctx, info, c_type, decl);
    return info;
}

type_info &make_unknown(
    Context &ctx,
    const clang::Type &type,
    const clang::Decl *decl) {
    const auto &c_type = canonical_type(type);
    auto &info = ctx.new_info(c_type, decl);
    info.kind = UNKNOWN;
    info.internal->type = &type;
    info.internal->is_resolved = false;
    info.internal->is_unknown = true;
    return info;
}

// ASTConsumer for clang frontend plugin
struct ArchimedesASTConsumer
    : public clang::ASTConsumer {
    struct Visitor
        : public clang::RecursiveASTVisitor<Visitor> {
        ArchimedesASTConsumer *parent;

        explicit Visitor(ArchimedesASTConsumer *parent)
            : parent(parent) {}

        void ArchimedesVisitDecl(clang::Decl *decl) {
            auto &ctx = Context::instance();

            // don't add if should be ignored
            if (decl->isInvalidDecl() || ctx.should_ignore(decl)) {
                return;
            }

            // ensure header is included in final generated file
            ctx.add_header_if_not_present(*decl);

            if (const auto *def = try_get_decl_definition(*decl)) {
                ctx.add_header_if_not_present(*def);
            }

            // TODO: cleanup
            if (const auto *fd =
                    clang::dyn_cast<clang::FunctionDecl>(decl)) {
                /* if (function_has_constraints(ctx, fd)) { */
                if (!function_satisfies_constraints(ctx, fd)) {
                    PLUGIN_LOG(
                        "skipping {} {} because constraints are not satisfied",
                        get_full_type_name(ctx, fd->getType()),
                        fd->getQualifiedNameAsString());
                    return;
                }
            }

            if (const auto *md =
                    clang::dyn_cast_or_null<clang::CXXMethodDecl>(decl)) {
                // even though methods get added when record decls are being
                // traversed we need to cover methods here as well in case they
                // are templated (get visited via FunctionTemplateDecl)
                auto &parent =
                    get_or_make_info(
                        ctx,
                        *md->getParent()->getTypeForDecl(),
                        md->getParent());
                if (!overloads_contains(ctx, parent.record.functions, *md)) {
                    insert_into_overloads(
                        parent.record.functions,
                        make_function_type_info(
                            ctx,
                            *md->getType(),
                            md,
                            &parent));
                }
            } else if (
                const auto *fd =
                    clang::dyn_cast<clang::FunctionDecl>(decl)) {
                // is free function? insert into function list
                if (!clang::isa<clang::CXXMethodDecl>(fd)
                    && !overloads_contains(ctx, ctx.functions, *fd)) {
                    PLUGIN_LOG(
                        "adding a free function {}",
                        decl_name(ctx, fd));
                    insert_into_overloads(
                        ctx.functions,
                        make_function_type_info(
                            ctx,
                            *fd->getType(),
                            fd,
                            std::nullopt));
                }
            } else if (
                const auto *td =
                    clang::dyn_cast<clang::TypedefNameDecl>(decl)) {
                td = td->getCanonicalDecl();
                ctx.typedefs.emplace_back(
                    typedef_info {
                        .name = td->getQualifiedNameAsString(),
                        .aliased_type =
                            get_or_make_qualified_info(
                                ctx,
                                td->getUnderlyingType(),
                                nullptr)
                    });
            } else if (
                const auto *nd =
                    clang::dyn_cast<clang::NamespaceAliasDecl>(decl)) {
                nd = nd->getCanonicalDecl();
                ctx.namespace_aliases.emplace_back(
                    namespace_alias_info {
                        .name =
                            nd->getQualifiedNameAsString(),
                        .aliased =
                            nd->getNamespace()->getQualifiedNameAsString()
                    });
            } else if (
                const auto *ud =
                    clang::dyn_cast<clang::UsingDirectiveDecl>(decl)) {
                // get enclosing namespace of this decl
                ASSERT(ud->getDeclContext()->isNamespace());
                ctx.namespace_usings.emplace_back(
                    namespace_using_info {
                        .containing =
                            clang::dyn_cast<clang::NamespaceDecl>(
                                ud->getDeclContext())
                                    ->getQualifiedNameAsString(),
                        .used =
                            ud->getNominatedNamespace()
                                ->getQualifiedNameAsString()
                    });
            } else if (
                const auto *td =
                    clang::dyn_cast<clang::TypeDecl>(decl)) {
                get_or_make_info(
                    ctx,
                    *td->getTypeForDecl(),
                    decl);
            }
        }

        // TODO: record friend info, don't traverse types within them
        bool TraverseFriendDecl(clang::FriendDecl *decl) {
            return true;
        }

        bool VisitVarDecl(clang::VarDecl *decl) {
            // only check VarDecls at namespace/struct/TU level
            if (decl->getParentFunctionOrMethod()) {
                return true;
            }

            // check if we want to add type of decl
            const auto *type = decl->getType().getTypePtrOrNull();
            if (type && type_can_have_definition(*type)) {
                if (auto *decl = try_get_type_definition(*type)) {
                    this->ArchimedesVisitDecl(decl);
                }
            }

            return true;
        }

        bool VisitUsingDirectiveDecl(clang::UsingDirectiveDecl *decl) {
            // only visit using namespace <...>; when it is contained in a
            // namespace
            const auto *context = decl->getDeclContext();
            // TODO: currently we do not respect global using directives as they
            // would lead to namespace pollution when using archimedes across
            // multiple translation units. is there a nice workaround for this?
            // context->isTranslationUnit()
            if (context->isNamespace()) {
                this->ArchimedesVisitDecl(decl);
            }
            return true;
        }

        bool VisitTypedefNameDecl(clang::TypedefNameDecl *decl) {
            this->ArchimedesVisitDecl(decl);
            return true;
        }

        bool VisitNamespaceAliasDecl(clang::NamespaceAliasDecl *decl) {
            this->ArchimedesVisitDecl(decl);
            return true;
        }

        bool TraverseNamespaceDecl(clang::NamespaceDecl *decl) {
            // try to skip entire excluded namespace
            const auto &ctx = Context::instance();
            const auto qual_name = decl->getQualifiedNameAsString();
            if (ctx.should_ignore_by_namespace(qual_name)) {
                return true;
            }

            // TODO: find a more intelligent way to do this for explicit enable
            // mode
             /* else if (ctx.should_ignore(decl)) { */
             /*    return true; */
            /* } */

            return RecursiveASTVisitor::TraverseNamespaceDecl(decl);
        }

        // skip method decl bodies
        bool TraverseCXXMethodDecl(clang::CXXMethodDecl *decl) {
            this->ArchimedesVisitDecl(decl);
            return true;
        }

        // skip function decl bodies
        bool TraverseFunctionDecl(clang::FunctionDecl *decl) {
            this->ArchimedesVisitDecl(decl);
            return true;
        }

        bool VisitCXXRecordDecl(clang::CXXRecordDecl *decl) {
            this->ArchimedesVisitDecl(decl);
            return true;
        }

        bool VisitEnumDecl(clang::EnumDecl *decl) {
            this->ArchimedesVisitDecl(decl);
            return true;
        }

        bool VisitClassTemplateDecl(clang::ClassTemplateDecl *decl) {
            for (auto *spec : decl->specializations()) {
                this->ArchimedesVisitDecl(spec);
            }
            return true;
        }

        bool VisitFunctionTemplateDecl(clang::FunctionTemplateDecl *decl) {
            for (auto *spec : decl->specializations()) {
                this->ArchimedesVisitDecl(spec);
            }
            return true;
        }
    };

    Visitor visitor;

    explicit ArchimedesASTConsumer()
        : visitor(this) {}

    void HandleTranslationUnit(clang::ASTContext &ast_ctx) override {
        auto &ctx = Context::instance();
        if (ctx.internal_invoke) {
            return;
        }

        PLUGIN_LOG("start for TU");

        // nothign to reflect if there are no decls
        if (ast_ctx.getTranslationUnitDecl()->decls_empty()) {
            PLUGIN_LOG("no decls, exit early");
            return;
        }

        ctx.ast_ctx = &ast_ctx;
        ctx.sema = &ctx.compiler->getSema();
        ctx.mangle_ctx = ctx.ast_ctx->createMangleContext(
            &ast_ctx.getTargetInfo());

        // check if TU decl has any ARCHIMEDES_ARGs at the top level
        auto *tu_decl = ast_ctx.getTranslationUnitDecl();
        const auto &sm = ast_ctx.getSourceManager();
        for (const auto *decl : tu_decl->decls()) {
            // skip ignore-able namespaces
            if (const auto *nd = clang::dyn_cast<clang::NamespaceDecl>(decl)) {
                if (ctx.should_ignore_by_namespace(
                        nd->getQualifiedNameAsString())) {
                    continue;
                }
            }

            // add to include list if decl comes from a header #include-d in
            // main file (regardless of if we care about the decl or not!)
            const auto location = decl->getLocation();
            if (!location.isInvalid() && location.isFileID()) {
                const auto full_location = clang::FullSourceLoc(location, sm);
                const auto include_location =
                    sm.getIncludeLoc(full_location.getFileID());
                if (!include_location.isInvalid()
                        && sm.isInMainFile(include_location)) {
                    ctx.add_header_if_not_present(*decl);
                }
            }

            if (const auto *vd = clang::dyn_cast<clang::VarDecl>(decl)) {
                const auto name = vd->getNameAsString();
                const auto &as = get_annotations(*vd);

                if (name.starts_with(ARCHIMEDES_STRINGIFY(ARCHIMEDES_ARG_NAME))
                    && as.size() == 1) {
                    const auto &as = get_annotations(*vd);
                    if (as.size() != 1) {
                        ASSERT(
                            "invalid ARCHIMEDES_ARG at {}",
                            vd->getBeginLoc().printToString(
                                vd->getASTContext().getSourceManager()));
                    }

                    ctx.process_arg(as[0]);
                } else if (
                    name.starts_with(
                        ARCHIMEDES_STRINGIFY(ARCHIMEDES_REFLECT_TYPE_NAME))
                    && as.size() == 1) {
                   // get template_arg type
                   const auto qt = vd->getType();
                   ASSERT(
                       qt->isStructureOrClassType(),
                       "malformed ARCHIMEDES_REFLECT_TYPE");
                   const auto &arg_type =
                       *clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(
                           qt->getAsStructureType()->getDecl())
                            ->getTemplateArgs()[0].getAsType();
                   const auto rx =
                        escape_for_regex(
                            get_full_type_name(ctx, arg_type));
                  PLUGIN_LOG(
                       "got explicit enable via ARCHIMEDES_REFLECT_TYPE {}",
                       rx);
                   ctx.explicit_enabled_types.push_back(
                       std::regex("^" + rx + "$"));
                }
            }
        }

        if (ctx.disabled) {
            return;
        }

        visitor.TraverseDecl(tu_decl);

        // exit early if nothing will be emitted
        if (ctx.functions.empty()
            && ctx.types.empty()
            && ctx.namespace_aliases.empty()
            && ctx.namespace_usings.empty()
            && ctx.typedefs.empty()) {
            PLUGIN_LOG("all empty, exit early");
            return;
        }

        const auto emitted = emit(ctx);

        if (ctx.emit_source) {
            // write directly to output path
            std::ofstream out(ctx.output_path);
            out << emitted;
            out.close();

            // try to clang-format if requested
            if (ctx.format_emitted_code) {
                const auto exec =
                    [](std::string_view cmd) -> std::tuple<int, std::string> {
                        FILE *pipe = popen(&cmd[0], "r");

                        if (!pipe) {
                            return std::make_tuple(1, "");
                        }

                        std::string out;
                        char c;
                        while ((c = std::fgetc(pipe)) != EOF) {
                            out += c;
                        }

                        return std::make_tuple(pclose(pipe), out);
                    };

                // try to find clang-format executable, format if possible
                const auto [which_exit_code, which_output] =
                    exec("which clang-format");
                if (!which_exit_code && !which_output.empty()) {
                    constexpr auto CLANG_FORMAT_STYLE =
                        R"({
                            BasedOnStyle: LLVM,
                            BreakStringLiterals: false,
                            UseTab: Never,
                            TabWidth: 2,
                            IndentWidth: 2,
                            ContinuationIndentWidth: 2,
                            ColumnLimit: 120
                        })";

                    std::string style = CLANG_FORMAT_STYLE;

                    // replace newlines in style
                    auto n = std::string::npos;
                    while ((n = style.find('\n')) != std::string::npos) {
                        style.replace(n, 1, " ");
                    }

                    exec(
                        fmt::format(
                            "clang-format -i -style=\"{}\" {}",
                                style,
                                ctx.output_path.string()));
                }
            }
        } else {
            // retrieve argumetns used to invoke this instance of clang
            llvm::BumpPtrAllocator bp_alloc;
            llvm::StringSaver string_pool(bp_alloc);
            auto sv_args = llvm::SmallVector<const char*>();
            ctx.compiler->getInvocation().generateCC1CommandLine(
                sv_args,
                [&](const llvm::Twine &arg) {
                    return string_pool.save(arg).data();
                });

            // convert from C strings
            vector<std::string_view> args;
            for (size_t i = 0; i < sv_args.size(); i++) {
                std::string_view sv = sv_args[i];
                if (!sv.empty()) {
                    args.push_back(sv);
                }
            }

            // remove anything pointing to the current main file path
            const auto &sm = ctx.ast_ctx->getSourceManager();
            const auto main_file_path =
                fs::path(
                    std::string_view(
                        sm.getFileEntryForID(sm.getMainFileID())
                            ->tryGetRealPathName()));

            // remove undesired arguments (those specifying file paths) from our
            // args list
            for (auto it = args.begin();
                 it != args.end();) {
                if (*it == "-main-file-name"
                        || *it == "-o"
                        || *it == "-MT"
                        || *it == "-dependency-file"
                        || *it == "-load"
                        || *it == "-plugin-arg-archimedes"
                        || (it->starts_with("plugin=")
                            && it->find("libarchimedes") != std::string::npos)
                        || it->starts_with("-fplugin-arg-archimedes")) {
                    it = args.erase(it);
                    it = args.erase(it);
                } else if (
                    fs::exists(*it)
                    && fs::equivalent(
                        *it, main_file_path)) {
                    it = args.erase(it);
                } else {
                    ++it;
                }
            }

            // emit "internal-invoke" so we don't invoke ourselves again
            // even though the plugin is not directly included in the arguments,
            // as we are still loaded, our pass gets automatically included
            args.push_back("-plugin-arg-archimedes");
            args.push_back("internal-invoke");

            std::string errs;
            llvm::raw_string_ostream os(errs);

            // TODO: more robust error handling
            ASSERT(
                compile(
                    args,
                    ctx.output_path,
                    emitted,
                    os),
                "ARCHIMEDES FAILED TO COMPILE:\n{}", errs);
        }
    }
};

// AST action for clang frontend plugin
struct ArchimedesASTAction
    : public clang::PluginASTAction {
    using Base = clang::PluginASTAction;

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &instance,
        llvm::StringRef) override {
        auto &ctx = Context::instance();
        ctx.compiler = const_cast<clang::CompilerInstance*>(&instance);
        return std::make_unique<ArchimedesASTConsumer>();
    }

    bool ParseArgs(
        const clang::CompilerInstance &instance,
        const std::vector<std::string> &args) override {
        auto &ctx = Context::instance();

        auto &diagnostics_engine = instance.getDiagnostics();

        for (const auto &arg : args) {
            const auto err_opt = ctx.process_arg(arg);

            if (err_opt) {
                const auto argument_error_id =
                    diagnostics_engine.getCustomDiagID(
                        clang::DiagnosticsEngine::Error,
                        "%0 '%1'");
                diagnostics_engine.Report(argument_error_id)
                    << *err_opt
                    << arg;
            }
        }

        if (ctx.output_path.empty()) {
            const auto no_output_path_id =
                diagnostics_engine.getCustomDiagID(
                    clang::DiagnosticsEngine::Error,
                    "no archimedes output path specified");
            diagnostics_engine.Report(no_output_path_id);
        } else if (ctx.header_path.empty()) {
            const auto no_header_path_id =
                diagnostics_engine.getCustomDiagID(
                    clang::DiagnosticsEngine::Error,
                    "no archimedes header path specified");
            diagnostics_engine.Report(no_header_path_id);
        }  else if (!fs::exists(ctx.header_path)) {
            const auto no_header_id =
                diagnostics_engine.getCustomDiagID(
                    clang::DiagnosticsEngine::Error,
                    "archimedes header does not exist");
            diagnostics_engine.Report(no_header_id);
        }

        return true;
    }

    ActionType getActionType() override {
        return AddAfterMainAction;
    }
};

// clang frontend plugin registrar
static clang::FrontendPluginRegistry::Add<ArchimedesASTAction>
    REGISTER_FRONTEND("archimedes", "archimedes");

// pragma recognizer
class ArchimedesPragmaHandler : public clang::PragmaHandler {
public:
    ArchimedesPragmaHandler()
        : clang::PragmaHandler("archimedes") {}

    void HandlePragma(
        clang::Preprocessor &pp,
        clang::PragmaIntroducer introducer,
        clang::Token &tok) override {
        auto &ctx = Context::instance();
        auto &diagnostics_engine = pp.getDiagnostics();

        // gather tokens
        std::string directive;
        while (tok.isNot(clang::tok::eod)) {
            directive += pp.getSpelling(tok);
            pp.Lex(tok);
        }

        PLUGIN_LOG("got pragma {}", directive);

        std::string::size_type pos;
        if ((pos = directive.find(_ARCHIMEDES_REFLECT_TYPE_REGEX_TEXT))
                != std::string::npos) {
            // process ARCHIMEDES_REFLECT_TYPE_REGEX(...)
            constexpr auto rttr_len =
                std::string(_ARCHIMEDES_REFLECT_TYPE_REGEX_TEXT).length();
            const auto quoted = directive.substr(pos + rttr_len);
            if (!quoted.starts_with('"') || !quoted.ends_with('"')) {
                const auto invalid_regex_id =
                    diagnostics_engine.getCustomDiagID(
                        clang::DiagnosticsEngine::Error,
                        "invalid type regex for pragma '%0'");
                diagnostics_engine.Report(invalid_regex_id) << directive;
            }
            const auto rx = quoted.substr(1, quoted.length() - 2);
            PLUGIN_LOG("adding explicit enable for {}", rx);
            ctx.explicit_enabled_types.emplace_back(
                std::regex("^" + rx + "$"));
        } else {
            const auto unrecognized_pragma_id =
                diagnostics_engine.getCustomDiagID(
                    clang::DiagnosticsEngine::Error,
                    "unrecognized pragma '%0'");
            diagnostics_engine.Report(unrecognized_pragma_id) << directive;
        }
    }
};

// clang frontend pragma registrar
static clang::PragmaHandlerRegistry::Add<ArchimedesPragmaHandler>
    REGISTER_PRAGMA("archimedes", "archimedes");

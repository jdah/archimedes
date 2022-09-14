#pragma once

#include <clang/AST/Decl.h>

#include <archimedes/type_info.hpp>
#include <nameof.hpp>

#include "implicit_function.hpp"
#include "util.hpp"

namespace archimedes {
struct Context;

// information needed to generate invoker for a function decl
struct Invoker {
    const clang::FunctionProtoType *type = nullptr;
    const clang::FunctionDecl *decl = nullptr;
    const clang::CXXRecordDecl *record = nullptr;

    // only applicable if invoker is for implicit function (no decl)
    std::optional<ImplicitFunction> if_kind = std::nullopt;

    std::string generated_name() const {
        return fmt::format(
            "_invoker_{:08X}",
            hash(
                decl ?
                    decl->getQualifiedNameAsString()
                    : std::string(NAMEOF_ENUM(*if_kind)),
                reinterpret_cast<uintptr_t>(this),
                reinterpret_cast<uintptr_t>(this->decl)));
    }
};

// returns true if an invoker can be emitted for the implicit function
bool can_make_implicit_invoker(
    const Context &ctx,
    const clang::Type &type,
    const clang::CXXRecordDecl &parent_decl,
    ImplicitFunction function_kind);

// returns true if invoker can be emitted for specified funciton decl
bool can_make_invoker(const Context &ctx, const clang::FunctionDecl &decl);

// emit invoker as function definition
std::string emit_invoker(Context &ctx, const Invoker&);
} // namespace archimedes

#pragma once

#include <vector>
#include <string_view>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/IR/Module.h>

#include <archimedes/ds.hpp>

#include "plugin.hpp"
#include "util.hpp"

namespace archimedes {
// result of compile() compiler instance and module it has generated
struct compiled_module {
    std::unique_ptr<clang::CompilerInstance> compiler;
    std::unique_ptr<llvm::Module> module;
};

// invoke another compiler instance on some code
// args_ is arguments to pass to compiler instance
// out is final output path for object file
// code is actual C++ code to compile
// optional error stream errs in case std::nullopt is returned
inline bool compile(
    const std::vector<std::string_view> &args_,
    const std::filesystem::path &out,
    std::string_view code,
    llvm::raw_ostream &errs = llvm::errs()) {
    PLUGIN_LOG("compiling...");

    // get temp filename in same directory as output
    const auto dir = out.parent_path();
    auto tmp_name =
        fmt::format(
            "{:08X}_{}.tmp",
            hash(&dir),
            out.filename().string());
    while (std::filesystem::exists(dir / tmp_name)) {
        tmp_name = char('A' + std::rand() % 52) + tmp_name;
    }

    // write code contents to file
    const auto tmp_path = dir / tmp_name;
    PLUGIN_LOG("writing code to {}", tmp_path.string());
    std::FILE *f = std::fopen(tmp_path.c_str(), "w");
    std::fputs(std::string(code).c_str(), f);
    std::fclose(f);

    // append output, inputs to args
    auto args = args_;
    args.push_back("-o");
    args.push_back(out.c_str());
    args.push_back(tmp_path.c_str());
    PLUGIN_LOG("invoking with args {}", fmt::join(args, " "));

    bool result = false;
    {
        // invoke compiler on temp file
        auto compiler =
            std::make_unique<clang::CompilerInstance>();
        auto invocation =
            std::make_shared<clang::CompilerInvocation>();
        auto diags =
            llvm::makeIntrusiveRefCnt<clang::DiagnosticIDs>();
        auto diag_options =
            llvm::makeIntrusiveRefCnt<clang::DiagnosticOptions>();
        auto diag_printer =
            std::make_unique<clang::TextDiagnosticPrinter>(
                errs,
                diag_options.get());

        // allocate via new, ownership is transferred to compiler instance
        auto diag_engine =
            new clang::DiagnosticsEngine(
                diags,
                diag_options);
        diag_engine->setClient(diag_printer.get(), false);

        // TODO: why do these warnings come up in the first place?
        constexpr auto IGNORED_WARNINGS =
            make_array(
                "expansion-to-defined",
                "nullability-completeness");

        for (const auto &w : IGNORED_WARNINGS) {
            diag_engine->setSeverityForGroup(
                clang::diag::Flavor::WarningOrError,
                w,
                clang::diag::Severity::Ignored);
        }

        PLUGIN_LOG("creating invocation");
        clang::CompilerInvocation::CreateFromArgs(
            *invocation,
            detail::transform<std::vector<const char*>>(
                args,
                [](const auto &view) {
                    return &view[0];
                }),
            *diag_engine);

        compiler->setInvocation(invocation);
        compiler->setDiagnostics(diag_engine);

        // EmitObjAction will use the default specified "-o" argument as its
        // output path
        PLUGIN_LOG("invoking");
        auto action =
        std::unique_ptr<clang::CodeGenAction>(
            new clang::EmitObjAction());
        compiler->ExecuteAction(*action);
        result = static_cast<bool>(action->takeModule());
    }

    std::filesystem::remove(tmp_path);
    return result;
}
}

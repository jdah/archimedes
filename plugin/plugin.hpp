#pragma once

#include <filesystem>
#include <memory>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <regex>

#include <clang/AST/Decl.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Mangle.h>
#include <clang/Sema/Sema.h>
#include <clang/Basic/FileEntry.h>
#include <clang/Frontend/CompilerInstance.h>

#include "ext/ast.hpp"

#include <archimedes.hpp>

#include "invoker.hpp"
#include "util.hpp"

using namespace archimedes;
using namespace archimedes::detail;
namespace fs = std::filesystem;

namespace archimedes {
struct Context;

// hidden internals
namespace detail {
// internal data allocated for each type_info
struct internal {
    bool is_resolved : 1;
    bool is_unknown : 1;
    const clang::Type *type;
    const clang::Decl *decl;
};

// internal data allocated for each function_type_info
struct function_type_info_internal {
    Invoker *invoker = nullptr;
    size_t hash = 0;
};

struct static_field_type_info_internal {
    std::string constexpr_expr;
    clang::QualType constexpr_type;
};

struct struct_base_type_info_internal {
    const clang::CXXRecordDecl *definition;
};

struct template_parameter_info_internal {
    std::string value_expr;
};
} // namespace detail

// flags for retrieving type names
enum TypeNameFlags : uint8_t {
    TNF_NONE = 0,
    TNF_ARRAYS_TO_POINTERS = (1 << 1)
};

// forward decls of type name functinons
inline std::string get_full_type_name(
    const Context&, const clang::Type&, TypeNameFlags = TNF_NONE);
inline std::string get_full_type_name(
    const Context&, const clang::ValueDecl&, TypeNameFlags = TNF_NONE);
inline std::string get_full_type_name(
    const Context&, const clang::TypeDecl&, TypeNameFlags = TNF_NONE);
inline std::string get_full_function_type_name(
    const Context&, const clang::FunctionDecl&, TypeNameFlags = TNF_NONE);
inline std::string get_full_function_name_and_type(
    const Context&, const clang::FunctionDecl&, TypeNameFlags = TNF_NONE);
inline std::optional<std::string> get_full_function_name_and_specialization(
    const Context &ctx,
    const clang::FunctionDecl &decl);
inline std::optional<std::string> get_full_function_name_type_and_specialization(
    const Context &ctx,
    const clang::FunctionDecl &decl);
inline std::optional<std::string> get_template_args_for_emit(
    const Context &ctx,
    clang::ArrayRef<clang::TemplateArgument> list);

inline std::string decl_name(const Context &ctx, const clang::Decl *decl) {
    if (!decl) {
        return "nullptr";
    } else if (const auto *fd = clang::dyn_cast<clang::FunctionDecl>(decl)) {
        const auto sn =
            get_full_function_name_type_and_specialization(ctx, *fd);
        return sn ? *sn : get_full_function_name_and_type(ctx, *fd);
    } else if (const auto *td = clang::dyn_cast<clang::TypeDecl>(decl)) {
        if (td->getTypeForDecl()) {
            return get_full_type_name(
                ctx, *td->getTypeForDecl(), TNF_ARRAYS_TO_POINTERS);
        } else {
            return td->getQualifiedNameAsString();
        }
    } else if (const auto *nd = clang::dyn_cast<clang::NamedDecl>(decl)) {
        return fmt::format("{}", nd->getQualifiedNameAsString());
    } else {
        return fmt::format("(no name) {}", decl->getDeclKindName());
    }
}

// returns true if decl is top level (not nested in some other definition)
inline bool is_decl_top_level(const clang::Decl &decl) {
    const auto *decl_context = decl.getLexicalDeclContext();

    if (!decl_context) {
        return false;
    } if (clang::isa<clang::TranslationUnitDecl>(decl_context)) {
        return true;
    } else if (
        clang::isa<clang::NamespaceDecl>(decl_context)
            || clang::isa<clang::ExternCContextDecl>(decl_context)) {
        if (const auto *p = decl_context->getLexicalParent()) {
            if (const auto *d = clang::dyn_cast_or_null<clang::Decl>(p)) {
                return is_decl_top_level(*d);
            }
        }
    }

    return false;
}

// TODO: doc
inline bool in_accessible_context(const clang::Decl &decl) {
    const auto *dc = decl.getDeclContext();
    while (!dc->isTranslationUnit()) {
        if (!dc->isRecord() && !dc->isNamespace()) {
            return false;
        }
        dc = dc->getParent();
    }
    return true;
}

// true if path is probably a header
inline bool is_likely_header_path(const fs::path &path) {
    constexpr auto HEADER_EXTENSIONS =
        make_array(
            "hpp", "HPP",
            "h", "H",
            "hh", "HH",
            "hxx", "HXX");

    if (!path.has_extension()) {
        return true;
    }

    std::string ext = path.extension().string();

    auto pos = std::string::npos;
    if ((pos = ext.rfind('.')) != std::string::npos) {
        ext = ext.substr(pos + 1);
    }

    return
        std::find(
            HEADER_EXTENSIONS.begin(),
            HEADER_EXTENSIONS.end(),
            ext)
                != HEADER_EXTENSIONS.end();
}

// returns true if two paths are equivalent
// TODO: hack around some filesystem permissions errors, fix later
inline bool paths_equivalent(
    const fs::path &a,
    const fs::path &b) {
    try {
        return fs::equivalent(a, b);
    } catch (...) {
        return a.string() == b.string();
    }
}

// get base/canonical type
inline const clang::Type &canonical_type(const clang::Type &type) {
    if (const auto *et =
            clang::dyn_cast_or_null<clang::ElaboratedType>(&type)) {
        if (et->isSugared()) {
            return canonical_type(*et->desugar().getTypePtr());
        }
    }

    return *type.getCanonicalTypeUnqualified()->getTypePtr();
}

// returns true if decl type can have a definition
inline bool decl_can_have_definition(const clang::Decl &decl) {
    return
        clang::isa<clang::CXXRecordDecl>(&decl)
        || clang::isa<clang::EnumDecl>(&decl)
        || clang::isa<clang::FunctionDecl>(&decl);
}

// returns true if type can have a defining decl
inline bool type_can_have_definition(const clang::Type &type) {
    // notice functions are not included as function types (fx. int(int, int))
    // cannot be "defined" somewhere
    return
        clang::isa<clang::RecordType>(&type)
        || clang::isa<clang::EnumType>(&type);
}

// try to get definition of decl if applicable, otherwise returning nullptr
inline clang::Decl *try_get_decl_definition(const clang::Decl &decl) {
    if (const auto *rd = clang::dyn_cast<clang::CXXRecordDecl>(&decl)) {
        return rd->getDefinition();
    } else if (
        const auto *ed =
            clang::dyn_cast<clang::EnumDecl>(&decl)) {
        return ed->getDefinition();
    } else if (
        const auto *fd =
            clang::dyn_cast<clang::FunctionDecl>(&decl)) {
        const auto *def = fd->getDefinition();

        // we consider "= default" and "= delete"/abstract definitions as well
        // where clang does not
        if (fd->isThisDeclarationADefinition()
            || (def && def->isThisDeclarationADefinition())
            || fd->isPure()
            || (def && def->isPure())) {
            return const_cast<clang::Decl*>(def ? def : &decl);
        }

        return const_cast<clang::FunctionDecl*>(def);
    }

    return nullptr;
}

// try to get definition of a type if possible, otherwise returning nullptr
inline clang::Decl *try_get_type_definition(const clang::Type &type) {
    const clang::Type &c_type = canonical_type(type);

    if (const auto *rt = clang::dyn_cast<clang::RecordType>(&c_type)) {
        return rt->getDecl()->getDefinition();
    } else if (
        const auto *et =
            clang::dyn_cast<clang::EnumType>(&c_type)) {
        return et->getDecl()->getDefinition();
    }

    return nullptr;
}

// retrieve all annotations (clang::annotate) for a declaration
inline vector<std::string> get_annotations(const clang::Decl &decl) {
    vector<std::string> annos;

    for (const auto *attr : decl.getAttrs()) {
        if (attr->getKind() != clang::attr::Annotate) {
            continue;
        }

        std::string s;
        llvm::raw_string_ostream os(s);
        attr->printPretty(os, decl.getASTContext().getPrintingPolicy());

        const auto q_left = s.find('\"'), q_right = s.rfind('\"');
        annos.push_back(s.substr(q_left + 1, q_right - q_left - 1));
    }

    return annos;
}

// true if definition is annotated with the specified annotation text
inline bool has_annotation(const clang::Decl &decl, std::string_view a) {
    const auto *def = try_get_decl_definition(decl);
    if (!def && decl_can_have_definition(decl)) {
        return false;
    }

    const auto *a_decl = def ? def : &decl;

    for (const auto *attr : a_decl->getAttrs()) {
        if (attr->getKind() != clang::attr::Annotate) {
            continue;
        }

        std::string s =
            f_ostream_to_string(
                [&](auto &os) {
                    attr->printPretty(
                        os,
                        a_decl->getASTContext().getPrintingPolicy());
                });

        const auto q_left = s.find('\"'), q_right = s.rfind('\"');
        if (s.substr(q_left + 1, q_right - q_left - 1) == a) {
            return true;
        }
    }

    return false;
}

// TODO: doc
template <typename F_t, typename F_d>
inline void traverse_type(const clang::Type *type, F_t &&f_t, F_d &&f_d) {
    if (!type) {
        return;
    }

    if (!f_t(type)) { return; }

    if (const auto *st =
            clang::dyn_cast_or_null<clang::SubstTemplateTypeParmType>(type)) {
        const auto *t = st->desugar().getTypePtrOrNull();
        if (!f_t(t)) { return; }
        traverse_type(t, std::forward<F_t>(f_t), std::forward<F_d>(f_d));
    } else if (const auto *tt =
            clang::dyn_cast_or_null<clang::TemplateSpecializationType>(type)) {
        for (const auto &ta : tt->template_arguments()) {
            const clang::Type *t = nullptr;
            clang::TemplateDecl *td = nullptr;
            switch (ta.getKind()) {
                case clang::TemplateArgument::ArgKind::Type:
                    t = ta.getAsType().getTypePtrOrNull();
                    if (!f_t(t)) { return; }
                    traverse_type(
                        t,
                        std::forward<F_t>(f_t),
                        std::forward<F_d>(f_d));
                    break;
                case clang::TemplateArgument::ArgKind::Declaration:
                    t = ta.getAsDecl()->getType().getTypePtrOrNull();
                    if (!f_t(t)) { return; }
                    if (!f_d(ta.getAsDecl())) { return; }
                    traverse_type(
                        t,
                        std::forward<F_t>(f_t),
                        std::forward<F_d>(f_d));
                    break;
                case clang::TemplateArgument::ArgKind::Template:
                case clang::TemplateArgument::ArgKind::TemplateExpansion:
                    td =
                        ta.getAsTemplateOrTemplatePattern()
                            .getAsTemplateDecl();
                    if (!f_d(td)) { return; }
                    break;
                case clang::TemplateArgument::ArgKind::Expression:
                case clang::TemplateArgument::ArgKind::Pack:
                case clang::TemplateArgument::ArgKind::NullPtr:
                case clang::TemplateArgument::ArgKind::Integral:
                default:
                    break;
            }

            if (t) {
                if (!f_t(t)) { return; }
                traverse_type(
                    t,
                    std::forward<F_t>(f_t),
                    std::forward<F_d>(f_d));
            }
        }
    } else if (
        const auto *ft =
            clang::dyn_cast_or_null<clang::FunctionProtoType>(type)) {
        for (const auto &qt : ft->getParamTypes()) {
            const auto *t = qt.getTypePtrOrNull();
            if (!f_t(t)) { return; }
            traverse_type(
                t,
                std::forward<F_t>(f_t),
                std::forward<F_d>(f_d));
        }

        const auto *t = ft->getReturnType().getTypePtrOrNull();
        if (!f_t(t)) { return; }
        traverse_type(
            t,
            std::forward<F_t>(f_t),
            std::forward<F_d>(f_d));
    }

    // contained type
    const clang::Type *t = nullptr;

    if(
        const auto *et =
            clang::dyn_cast_or_null<clang::ElaboratedType>(type)) {
        t = et->desugar().getTypePtrOrNull();
    } else if (
        const auto *at =
            clang::dyn_cast_or_null<clang::ArrayType>(type)) {
        t = at->getArrayElementTypeNoTypeQual();
    } else if (
        const auto *dt =
            clang::dyn_cast_or_null<clang::DecltypeType>(type)) {
        t = dt->desugar().getTypePtrOrNull();
    } else if (
        const auto *dt =
            clang::dyn_cast_or_null<clang::DeducedType>(type)) {
        t = dt->getDeducedType().getTypePtrOrNull();
    } else if (
        const auto *rt =
            clang::dyn_cast_or_null<clang::ReferenceType>(type)) {
        t = rt->getPointeeType().getTypePtrOrNull();
    } else if (
        const auto *pt =
            clang::dyn_cast_or_null<clang::PointerType>(type)) {
        t = pt->getPointeeType().getTypePtrOrNull();
    } else if (
        const auto *tt =
            clang::dyn_cast_or_null<clang::TagType>(type)) {
        if (!f_d(tt->getAsTagDecl())) { return; }
    }

    if (t) {
        if (!f_t(t)) { return; }
        traverse_type(
            t,
            std::forward<F_t>(f_t),
            std::forward<F_d>(f_d));
    }
}

struct Context {
    // file path filter (always absolute paths)
    set<fs::path> file_paths;

    // file name filter
    set<std::string> file_names;

    // included namespaces
    set<std::string> included_namespaces;

    // excluded namespaces
    set<std::string> excluded_namespaces;

    // regexes of included paths
    vector<std::regex> included_path_regexes;

    // regexes of excluded paths
    vector<std::regex> excluded_path_regexes;

    // regexes of explicitly included types
    vector<std::regex> explicit_enabled_types;

    // output path (should be .cpp if emit_source otherwise .o)
    fs::path output_path;

    // archimedes header path
    fs::path header_path;

    // all headers encountered throughout processing
    set<fs::path> found_headers;

    // all headers which contain decls which are in emitted code
    set<fs::path> emitted_headers;

    // ast context
    clang::ASTContext *ast_ctx;

    // sema
    clang::Sema *sema;

    // mangle context
    clang::MangleContext *mangle_ctx;

    // compiler instance
    clang::CompilerInstance *compiler;

    // list of registered types
    vector<std::unique_ptr<type_info>> types;

    // invokers to be generated
    vector<Invoker*> invokers;

    // free function overload sets
    name_map<function_overload_set> functions;

    // all encountered typedefs
    vector<typedef_info> typedefs;

    // all encountered namespace aliases
    vector<namespace_alias_info> namespace_aliases;

    // all encountered namespace using directives
    vector<namespace_using_info> namespace_usings;

    // enable verbose logging
    bool verbose = false;

    // true if archimedes has been disabled (don't reflect or emit anything)
    bool disabled = false;

    // disables archimedes entirely, won't even respond to "enable" args
    // used when emitting .o outputs to disable the plugin when the
    // compiler is invoked for a second time
    bool internal_invoke = false;

    // true if emitted code should include comments, field names, newlines, etc.
    bool emit_verbose = false;

    // true if emitted code should be source instead of an object
    bool emit_source = false;

    // true if emitted code should be formatted with clang-format
    bool format_emitted_code = false;

    // if "explicit enable" is on, only functions/types which are explicitly
    // marked with ARCHIMEDES_REFLECT will be reflected
    bool explicit_enable = false;

    // TODO: allocate in large block of memory
    // list of allocated objects (lifetimes are tied to context)
    struct alloc_container { virtual ~alloc_container() = default; };
    vector<std::unique_ptr<alloc_container>> allocations;

    // allocate a T with lifetime attached to this context
    template <typename T>
    T &alloc() {
        struct alloc_container_t : alloc_container { T t; };
        auto &t =
            dynamic_cast<alloc_container_t&>(
                *this->allocations.emplace_back(
                    std::make_unique<alloc_container_t>())).t;
        PLUGIN_LOG(
            "allocated type {} at ptr {}",
            type_name<T>(),
            fmt::ptr(&t));
        return t;
    }

    // process argument, either from command line OR from ARCHIMEDES_ARG
    // return non-nullopt on error
    // expects arguments in string format "<name>-<value>"
    std::optional<std::string> process_arg(const std::string &arg) {
        if (this->internal_invoke) {
            return std::nullopt;
        }

        PLUGIN_LOG("got argument {}", arg);

        const auto try_make_regex =
            [](std::string_view str) {
                try {
                    return std::regex(
                        std::string(str),
                        std::regex_constants::ECMAScript);
                } catch (const std::regex_error &e) {
                    LOG("archimedes regex error for {}: {}", str, e.what());
                    ARCHIMEDES_FAIL("regex error");
                }
            };

        if (arg.starts_with("verbose")) {
            this->verbose = true;
        } else if (arg.starts_with("file-")) {
            // just add as regex
            this->included_path_regexes.push_back(
                try_make_regex(
                    "^.*"
                        + arg.substr(std::strlen("file-"), std::string::npos)
                        + "$"));
        } else if (arg.starts_with("out-")) {
            this->output_path =
                arg.substr(std::strlen("out-"), std::string::npos);
        } else if (arg.starts_with("header-")) {
            this->header_path =
                arg.substr(std::strlen("header-"), std::string::npos);
        } else if (arg.starts_with("include-ns-")) {
            const auto ns =
                arg.substr(std::strlen("include-ns-"), std::string::npos);
            this->included_namespaces.insert(ns);
            std::erase_if(
                this->excluded_namespaces,
                [&](const auto &n) {
                    return n == ns;
                });
        } else if (arg.starts_with("exclude-ns-")) {
            auto ns =
                arg.substr(std::strlen("exclude-ns-"), std::string::npos);
            this->excluded_namespaces.emplace(ns);
            std::erase_if(
                this->included_namespaces,
                [&](const auto &n) {
                    return n == ns;
                });
        } else if (arg.starts_with("include-path-regex-")) {
            const auto rx =
                arg.substr(
                    std::strlen("include-path-regex-"), std::string::npos);
            this->included_path_regexes.push_back(
                try_make_regex(
                    "^" + rx + "$"));
        } else if (arg.starts_with("exclude-path-regex-")) {
            const auto rx =
                arg.substr(
                    std::strlen("exclude-path-regex-"), std::string::npos);
            this->excluded_path_regexes.push_back(
                try_make_regex(
                    "^" + rx + "$"));
        } else if (arg.starts_with("config-")) {
            const auto path =
                arg.substr(std::strlen("config-"), std::string::npos);
            // load args from config
            std::ifstream f(path);
            std::string line;
            while (std::getline(f, line)) {
                if (std::all_of(
                        line.begin(), line.end(),
                        [](char c) { return std::isspace(c); })) {
                    continue;
                }

                // get first non-space, last non-space, process
                constexpr auto WHITESPACE = " \t\n\r";
                auto pos_l = line.find_first_not_of(WHITESPACE),
                     pos_r = line.find_last_not_of(WHITESPACE);

                if (line[pos_l] == '#') {
                    continue;
                }

                ASSERT(pos_l != std::string::npos);
                ASSERT(pos_r != std::string::npos);
                process_arg(line.substr(pos_l, pos_r - pos_l + 1));
            }
        } else if (arg == "internal-invoke") {
            this->internal_invoke = true;
        } else if (arg == "disable") {
            this->disabled = true;
        } else if (arg == "enable") {
            this->disabled = false;
        } else if (arg == "format") {
            this->format_emitted_code = true;
        } else if (arg == "emit-verbose") {
            this->emit_verbose = true;
        } else if (arg == "emit-source") {
            this->emit_source = true;
        } else if (arg == "explicit-enable") {
            this->explicit_enable = true;

            // disabled until explicitly enabled for TU
            this->disabled = true;
        } else {
            return "invalid argument";
        }

        return std::nullopt;
    }

    // attempts to get the path to the file a decl occured in
    std::optional<fs::path> decl_file_path(
        const clang::Decl &decl) const {
        const auto source_location = decl.getLocation();
        if (!source_location.isValid()) {
            return std::nullopt;
        }
        auto full_source_loc =
            clang::FullSourceLoc(
                source_location,
                decl.getASTContext().getSourceManager());

        if (!full_source_loc.isFileID()) {
            full_source_loc = full_source_loc.getFileLoc();
        }

        const auto *file_entry = full_source_loc.getFileEntry();
        if (!file_entry) {
            return std::nullopt;
        }

        std::string name = file_entry->getName().str();
        return name.empty() ?
            std::nullopt
            : std::make_optional(
                fs::path(name));
    }

    // register some decl which has been emitted, ensuring that its path is
    // included in the final list of emitted headers
    void register_emitted_decl(const clang::Decl &decl) {
        const auto path_opt = this->decl_file_path(decl);
        if (!path_opt) {
            return;
        } else if (!is_likely_header_path(*path_opt)) {
            // check if in system header
            const auto source_location = decl.getLocation();
            if (!source_location.isValid()) {
                return;
            }
            const auto full_source_loc =
                clang::FullSourceLoc(
                    source_location,
                    decl.getASTContext().getSourceManager());
            if (!full_source_loc.isInSystemHeader()) {
                return;
            }
        }

        this->emitted_headers.emplace(fs::absolute(*path_opt));
    }

    // register some type, ensuring that if it has a decl someplace that it is
    // then included in the final list of emitted headers
    void register_emitted_type(const clang::Type &type) {
        traverse_type(
            &type,
            [&](const clang::Type *type) {
                if (const auto *tt = clang::dyn_cast<clang::TagType>(type)) {
                    this->register_emitted_decl(*tt->getDecl());
                }

                return true;
            },
            [&](const clang::Decl*) { return true; });
    }

    // returns true if decl is in one of the found headers
    bool is_in_found_header(const clang::Decl &decl) const {
        const auto path_opt = this->decl_file_path(decl);
        if (!path_opt) {
            return false;
        }

        // no need to check path equivalence because set stores absolute paths
        const auto path = fs::absolute(*path_opt);
        return this->found_headers.contains(path);
    }

    // check if a name should be ignored according to its namespace
    // TODO: merge with below function
    bool should_ignore_by_namespace(std::string_view name) const {
        // no namespace, cannot ignore
        if (name.find("::") == std::string::npos) {
            return false;
        }

        bool
            has_included = !this->included_namespaces.empty(),
            in_included =
                std::any_of(
                    this->included_namespaces.begin(),
                    this->included_namespaces.end(),
                    [&name](const auto &ns) { return name.starts_with(ns); }),
            has_excluded = !this->excluded_namespaces.empty(),
            in_excluded =
                std::any_of(
                    this->excluded_namespaces.begin(),
                    this->excluded_namespaces.end(),
                    [&name](const auto &ns) { return name.starts_with(ns); });

        // ignore if included OR excluded and not explicitly included
        return
            (has_included && !in_included)
            || (has_excluded && in_excluded && !in_included);
    }

    // TODO: merge with above function
    bool should_ignore_by_path(std::string_view path) const {
        bool
            has_included = !this->included_path_regexes.empty(),
            in_included =
                std::any_of(
                    this->included_path_regexes.begin(),
                    this->included_path_regexes.end(),
                    [&path](const auto &rx) {
                        return std::regex_match(std::string(path), rx);
                    }),
            has_excluded = !this->excluded_path_regexes.empty(),
            in_excluded =
                std::any_of(
                    this->excluded_path_regexes.begin(),
                    this->excluded_path_regexes.end(),
                    [&path](const auto &rx) {
                        return std::regex_match(std::string(path), rx);
                    });

        // ignore if included OR excluded and not explicitly included
        return (has_included && !in_included)
            || (has_excluded && in_excluded && !in_included);
    }

    // returns true if this declaration or any of its contexts are annotated
    // with "a"
    bool annotation_in_hierarchy(
        const clang::Decl &decl,
        std::string_view a) const {
        if (has_annotation(decl, a)) {
            PLUGIN_LOG("decl {} had annotation {}", decl_name(*this, &decl), a);
            return true;
        }

        const auto *dc = decl.getDeclContext();
        while (!dc->isTranslationUnit()) {
            const clang::Decl *dc_decl =
                clang::dyn_cast_or_null<clang::Decl>(dc);

            if (has_annotation(*dc_decl, a)) {
                PLUGIN_LOG(
                    "decl context {} had annotation {}",
                    decl_name(*this, dc_decl),
                    a);
                return true;
            }

            dc = dc->getParent();
        }

        return false;
    }

    // checks if a decl (and NOT its hierarchy) has been explicitly enabled for
    // reflection
    bool is_explicitly_enabled(const clang::Decl *decl) const {
        const auto *def = try_get_decl_definition(*decl);
        const auto name = decl_name(*this, decl);
        const auto def_name =
            def ?
                std::make_optional(decl_name(*this, def))
                : std::nullopt;

        if (detail::any_of(
                this->explicit_enabled_types,
                [&](const auto &rx) {
                    return std::regex_match(name, rx)
                        || (def_name && std::regex_match(*def_name, rx));
                })) {
            // cannot ignore types which are enabled by name
            return true;
        } else if (has_annotation(*decl, _ARCHIMEDES_REFLECT_TEXT)) {
            return true;
        }
        return false;
    }

    // TODO: doc
    bool enabled_in_hierarchy(const clang::Decl *decl) const {
        if (this->is_explicitly_enabled(decl)) {
            return true;
        }

        const auto *dc = decl->getDeclContext();
        while (dc && !dc->isTranslationUnit()) {
            if (this->is_explicitly_enabled(clang::dyn_cast<clang::Decl>(dc))) {
                return true;
            }
            dc = dc->getParent();
        }

        return false;
    }

    // returns true if declaration should be ignored for reflection generation
    bool should_ignore(const clang::Decl *decl) const {
        // check hierarchy for no/reflect annotations so that children get
        // no/reflected if contained by something which is marked
        if (!decl) {
            return true;
        }

        if (clang::isa<clang::TemplateDecl>(decl)) {
            return true;
        }

        // cannot do anything inside of anonymous namespaces
        if (decl->isInAnonymousNamespace()) {
            return true;
        }

        // not accessible? skip
        if (!in_accessible_context(*decl)) {
            return true;
        }

        // ignore unnamed decls
        const auto *nd = clang::dyn_cast_or_null<clang::NamedDecl>(decl);
        if (!nd) {
            PLUGIN_LOG(
                "{} ignored because it is not named",
                decl_name(*this, decl));
            return true;
        }

        // check that there are no template descriptions in the hierarchy AND
        // that this is not a template
        // ALSO check if there is an ARCHIMEDES_NO_REFLECT somewhere
        const auto *dc = decl;
        while (dc && !clang::isa<clang::TranslationUnitDecl>(dc)) {
            if (has_annotation(*dc, _ARCHIMEDES_NO_REFLECT_TEXT)) {
                PLUGIN_LOG(
                    "{} ignored because no reflect in hierarchy",
                    decl_name(*this, decl));
                return true;
            }

            const auto *fd = clang::dyn_cast<clang::FunctionDecl>(dc);
            if (dc->isTemplateDecl()
                || dc->getDescribedTemplate()
                || dc->getDescribedTemplateParams()
                || clang::isa<clang::TemplateDecl>(dc)
                || (fd && fd->getDescribedFunctionTemplate())) {
                PLUGIN_LOG(
                    "skipping {} because {} is a template (with kind {})",
                    decl_name(*this, decl),
                    decl_name(*this, dc),
                    dc->getDeclKindName());
                return true;
            }
            dc = clang::dyn_cast<clang::Decl>(dc->getDeclContext());
        }

        // check that we have the definition if this is a tag
        // functions without definitions are fine, only need a decl
        const clang::Decl *def = nullptr;

        const clang::FunctionDecl *fd;
        if (const auto *ctsd =
                clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl)) {
            def = ctsd->getSpecializedTemplate();
        } else if (
            (fd = clang::dyn_cast<clang::FunctionDecl>(decl))
                && fd->getPrimaryTemplate()) {
            def = fd->getPrimaryTemplate();
        } else {
            def = try_get_decl_definition(*decl);
        }

        if (!def && clang::isa<clang::TagDecl>(*decl)) {
            PLUGIN_LOG(
                "{} ignored because decl needs definition and could not find",
                decl_name(*this, decl));
            return true;
        }

        const auto *decl_or_def = def ? def : decl;

        const auto path_opt = this->decl_file_path(*decl_or_def);

        // can't find a file? must be builtin or something, IGNORE!
        if (!path_opt) {
            PLUGIN_LOG(
                "{} ignored because no file",
                decl_name(*this, decl_or_def));
            return true;
        }

        if (this->explicit_enable) {
            // check also if the decl itself is explicitly enabled - that is,
            // for class template specializations (for example) we also want to
            // check if the specicialization is enabled explicitly where its
            // definition (fx. some Template<T> instead of Template<int>) may
            // not be
            const auto res =
                !this->is_explicitly_enabled(decl)
                    && !this->enabled_in_hierarchy(decl_or_def);
            PLUGIN_LOG(
                "explicit_enable for {}: {}",
                decl_name(*this, decl),
                res);
            return res;
        }

        // explicit enable must check decls in all files, check path after
        if (should_ignore_by_path((*path_opt).string())) {
            PLUGIN_LOG(
                "{} ignored by path ({})",
                decl_name(*this, decl_or_def),
                path_opt->string());
            return true;
        }

        // not top level? check if we want to ignore its context UNLESS we're
        /* if (!is_decl_top_level(*decl)) { */
        /*     if (const auto *d = */
        /*             clang::dyn_cast_or_null<clang::Decl>( */
        /*                 decl->getDeclContext())) { */
        /*         return should_ignore(d); */
        /*     } */
        /* } */

        // if excluded OR there are included namespaces and it is not included
        if (this->should_ignore_by_namespace(nd->getQualifiedNameAsString())) {
            PLUGIN_LOG(
                "{} ignored by namespace",
                decl_name(*this, decl_or_def));
            return true;
        }

        // cannot ignore
        PLUGIN_LOG(
            "{} not ignored",
            decl_name(*this, decl_or_def));
        return false;
    }

    // add header to list of found headers if not already present
    void add_header_if_not_present(const clang::Decl &decl) {
        // check decl origin - header we are not aware of? add to include
        // list
        const auto full_source_loc =
            clang::FullSourceLoc(
                decl.getLocation(),
                this->ast_ctx->getSourceManager());

        // system header OR likely header -> found headers if not already
        // in set
        const auto path_opt = this->decl_file_path(decl);

        if (path_opt &&
                (full_source_loc.isInSystemHeader()
                    || is_likely_header_path(*path_opt))) {
            const auto path = fs::absolute(*path_opt);
            if (!this->found_headers.contains(path)) {
                this->found_headers.emplace(path);
            }
        }
    }

    // get type by type index
    type_info &by_id(type_id i) const {
        return *this->types[i.value()];
    }

    // returns true if type info exists for clang type
    std::optional<type_info*> try_get(
        const clang::Type &type,
        const clang::Decl *decl) const {
        for (auto &t : this->types) {
            // use canonical types as we can compare by pointer
            if (&canonical_type(type) ==
                    &canonical_type(*t->internal->type)) {
                return t.get();
            }
        }
        return std::nullopt;
    }

    // make an invoker saved on global context
    Invoker *make_invoker() {
        // TODO: deduplicate invokers if they invoke the same function (can
        // happen for constructors which are used globally AND in the record
        // type reflection)
        return this->invokers.emplace_back(&this->alloc<Invoker>());
    }

    // create new type info for type
    type_info &new_info(const clang::Type &type, const clang::Decl *decl) {
        type_info &info =
            *this->types.emplace_back(std::make_unique<type_info>());
        info.id = type_id::from(this->types.size() - 1);
        info.type_name = get_full_type_name(*this, type);
        info.mangled_type_name =
            f_ostream_to_string(
                [&](auto &os) {
                    this->mangle_ctx->mangleCXXRTTIName(
                        clang::QualType(&type, 0), os);
                });
        info.internal = &this->alloc<archimedes::detail::internal>();
        info.internal->type = &type;
        info.internal->decl = decl;
        info.internal->is_resolved = false;
        return info;
    }

    // global instance of context
    static Context &instance();
};

inline std::string location_to_string(
    const Context &ctx,
    const clang::SourceLocation &l) {
    if (l.isInvalid()) {
        return "";
    }
    return l.printToString(ctx.ast_ctx->getSourceManager());
}

// TODO: doc
inline std::string location_to_path(
    const Context &ctx,
    const clang::SourceLocation &l) {
    const auto str = location_to_string(ctx, l);
    return str.substr(0, str.find(":"));
}

// retrieve full/qualified name for a qualified type
inline std::string get_full_type_name(
    const Context &ctx,
    const clang::QualType &q,
    TypeNameFlags flags = TNF_NONE) {
    if (!q.getTypePtrOrNull()) {
        return std::string();
    }

    /* clang::PrintingPolicy policy(ctx.ast_ctx->getPrintingPolicy()); */
    /* policy.FullyQualifiedName = 1; */
    /* policy.SuppressInlineNamespace = 0; */
    /* policy.SuppressUnwrittenScope = 0; */
    /* policy.SuppressTemplateArgsInCXXConstructors = 0; */
    /* policy.SuppressScope = 0; */
    /* policy.SuppressDefaultTemplateArgs = 1; */
    /* policy.SuppressSpecifiers = 0; */
    /* policy.AnonymousTagLocations = 1; */
    /* policy.PrintInjectedClassNameWithArguments = 1; */
    /* policy.UsePreferredNames = 1; */
    /* std::string name = q.getAsString(policy); */

    clang::PrintingPolicy policy(ctx.ast_ctx->getPrintingPolicy());
    policy.SuppressScope = false;
    policy.AnonymousTagLocations = true;
    policy.FullyQualifiedName = true;
    policy.SuppressUnwrittenScope = false;
    policy.SuppressScope = false;
    policy.SuppressSpecifiers = false;
    policy.SuppressDefaultTemplateArgs = true;

    /* // USE PREFERRED NAMES! this will prevent us from getting different results */
    /* // when the type_name<T>/__PRETTY_FUNCTION__ hack is used to get type names */
    /* // and IDs */
    policy.UsePreferredNames = true;

    std::string name =
        cling::utils::TypeName::GetFullyQualifiedName(
            q,
            *ctx.ast_ctx,
            policy);

    // function type names *do not* include the noexcept specifier
    if (q->isFunctionType()) {
        const auto noexcept_specifier = std::string("noexcept");
        auto noexcept_pos = name.find(noexcept_specifier);

        if (noexcept_pos != std::string::npos) {
            // check if expr is noexcept(...)
            const auto has_expr =
                name[noexcept_pos + noexcept_specifier.length()] == '(';

            if (has_expr) {
                name.erase(
                    noexcept_pos,
                    name.find(')', noexcept_pos) - noexcept_pos + 1);
            } else {
                name.erase(noexcept_pos, noexcept_specifier.length());
            }
        }
    }

    // convert arrays to pointers if specified
    // (but NOT inside of template args, these must always match as they were
    // declared)
    if (flags & TNF_ARRAYS_TO_POINTERS) {
        size_t template_depth = 0;
        for (size_t i = 0; i < name.length(); i++) {
            const auto &c = name[i];
            if (c == '<') {
                template_depth++;
            } else if (c == '>') {
                template_depth--;
            } else if (c == '[' && template_depth == 0) {
                name =
                    name.substr(0, i)
                        + "*"
                        + name.substr(name.find("]", i) + 1);
            }
        }
    }

    return name;
}

// retrieve full/qualified name for a type
inline std::string get_full_type_name(
    const Context &ctx,
    const clang::Type &t,
    TypeNameFlags flags) {
    return get_full_type_name(ctx, clang::QualType(&t, 0), flags);
}

// retrieve full name for a value decl
inline std::string get_full_type_name(
    const Context &ctx,
    const clang::ValueDecl &decl,
    TypeNameFlags flags) {
    return get_full_type_name(ctx, decl.getType(), flags);
}

// retrieve full name for a type decl
inline std::string get_full_type_name(
    const Context &ctx,
    const clang::TypeDecl &decl,
    TypeNameFlags flags) {
    return get_full_type_name(ctx, *decl.getTypeForDecl());
}

// get full name of function name, no type info
inline std::string get_full_function_type_name(
    const Context &ctx,
    const clang::FunctionDecl &fd,
    TypeNameFlags flags) {
    auto name = get_full_type_name(ctx, fd);
    return name.substr(0, name.rfind('('));
}

// get full function name and type
inline std::string get_full_function_name_and_type(
    const Context &ctx,
    const clang::FunctionDecl &fd,
    TypeNameFlags flags) {
    auto name = get_full_type_name(ctx, fd);
    const auto lparen_pos = name.rfind('(');
    return name.substr(0, lparen_pos)
        + fd.getQualifiedNameAsString()
        + name.substr(lparen_pos);
}

// retrieve the function template decl including its template parameters
inline std::string get_full_function_template_decl(
    const Context &ctx,
    const clang::FunctionTemplateDecl &decl) {
    auto pp = clang::PrintingPolicy(ctx.ast_ctx->getLangOpts());
    pp.FullyQualifiedName = 1;
    pp.SuppressScope = 0;
    pp.PrintCanonicalTypes = 1;

    std::string template_prefix;
    llvm::raw_string_ostream os(template_prefix);
    decl.getTemplateParameters()->print(os, *ctx.ast_ctx, pp);
    os.flush();

    ASSERT(decl.getTemplatedDecl());
    return template_prefix
        + " "
        + get_full_function_name_and_type(ctx, *decl.getTemplatedDecl());
}

// print fully qualified expr
inline std::string get_fully_qualified_expr(
    const Context &ctx,
    const clang::Expr &expr) {
    auto pp = clang::PrintingPolicy(ctx.ast_ctx->getLangOpts());
    pp.FullyQualifiedName = 1;
    pp.SuppressSpecifiers = 0;
    pp.SuppressInlineNamespace = 0;
    pp.SuppressScope = 0;
    pp.PrintCanonicalTypes = 1;

    std::string result;
    llvm::raw_string_ostream stream(result);
    expr.printPretty(stream, nullptr, pp);
    stream.flush();
    return result;
}

// checks that a decl and all of its containing contexts are public
inline bool is_decl_public(const clang::Decl &decl) {
    const auto as_decl = decl.getAccess();
    if (as_decl != clang::AccessSpecifier::AS_public
        && as_decl != clang::AccessSpecifier::AS_none) {
        return false;
    }

    const auto *dc = decl.getDeclContext();
    while (!dc->isTranslationUnit()) {
        if (dc->isRecord()) {
            const auto as =
                clang::dyn_cast<clang::RecordDecl>(dc)->getAccess();
            return as == clang::AccessSpecifier::AS_public
                     || as == clang::AccessSpecifier::AS_none;
        }

        dc = dc->getParent();
    }
    return true;
}

// a type can be emitted iff:
// - it does not contain any lambdas
// - all of its types are in a visible header
inline bool can_emit_type(const Context &ctx, const clang::Type *type) {
    if (!type) {
        return false;
    }

    // TODO: lambda/unnamed are hacks
    const auto name = get_full_type_name(ctx, *type);

    // check if name has lambdas - if so, cannot emit
    if (name.find("lambda at") != std::string::npos) {
        return false;
    }

    // check if name has "unnamed/anonymous", if so cannot emit
    if (name.find("unnamed struct at") != std::string::npos
        || name.find("unnamed union at") != std::string::npos
        || name.find("anonymous struct at") != std::string::npos
        || name.find("anonymous union at") != std::string::npos) {
        return false;
    }

    // TODO: another hack: we cannot emit anything with literal NTTPs
    if (name.find("{") != std::string::npos
        || name.find("}") != std::string::npos) {
        return false;
    }

    bool fail = false;
    traverse_type(
        type,
        [](const auto*) { return true; },
        [&](const clang::Decl *decl) {
            if (!decl || !is_decl_public(*decl)) {
                fail = true;
            }

            // TODO: make this much more robust, check that expansion locations
            // match one of our headers
            if (const auto *td =
                    clang::dyn_cast<clang::TagDecl>(decl)) {
                const auto path = ctx.decl_file_path(*td);

                if (!path) {
                    PLUGIN_LOG(
                        "cannot emit (no path) {}",
                        td->getQualifiedNameAsString());
                    fail = true;
                }

                const auto *tld = td->getDescribedTemplate();
                if (tld) {
                    const auto tld_path = ctx.decl_file_path(*tld);
                    if (!is_likely_header_path(*path)
                        && (!tld_path || !is_likely_header_path(*tld_path))) {
                        PLUGIN_LOG(
                            "cannot emit {}/{}",
                            td->getQualifiedNameAsString(),
                            tld->getQualifiedNameAsString());
                        fail = true;
                    }
                } else if (!is_likely_header_path(*path)) {
                    PLUGIN_LOG(
                        "cannot emit {}",
                        td->getQualifiedNameAsString());
                    fail = true;
                }
            }

            return !fail;
        });

    return !fail;
 }

// check if a list of template args can be emitted
inline bool can_emit_template_args(
    const Context &ctx,
    const clang::TemplateArgumentList &list) {
    for (const auto &ta : list.asArray()) {
        switch (ta.getKind()) {
            clang::TemplateDecl *td;
            case clang::TemplateArgument::ArgKind::Type:
                if (!can_emit_type(ctx, ta.getAsType().getTypePtr())) {
                    return false;
                }
                break;
            case clang::TemplateArgument::ArgKind::Declaration:
                if (!is_decl_public(*ta.getAsDecl())) {
                    return false;
                }
                break;
            case clang::TemplateArgument::ArgKind::Template:
            case clang::TemplateArgument::ArgKind::TemplateExpansion:
                td = ta.getAsTemplateOrTemplatePattern().getAsTemplateDecl();
                if (!td || !is_decl_public(*td)) {
                    return false;
                }
                break;
            case clang::TemplateArgument::ArgKind::Expression:
                // TODO
                break;
            default:
                break;
        }
    }

    return true;
}

// a function decl can be emitted iff:
// - all of its types can be emitted
// - all of its template args can be emitted
inline bool can_emit_function(
    const Context &ctx,
    const clang::FunctionDecl &decl) {
    if (!can_emit_type(ctx, decl.getType().getTypePtr())) {
        return false;
    }

    if (!can_emit_type(ctx, decl.getReturnType().getTypePtr())) {
        return false;
    }

    for (const auto *param : decl.parameters()) {
        if (!can_emit_type(ctx, param->getType().getTypePtr())) {
            return false;
        }
    }

    // check template args if present
    if (decl.isFunctionTemplateSpecialization()) {
        return can_emit_template_args(
            ctx,
            *decl.getTemplateSpecializationArgs());
    }

    return true;
}

// get template args for emitting from template argument list
// returns nullopt if not possible to emit
inline std::optional<std::string> get_template_args_for_emit(
    const Context &ctx,
    clang::ArrayRef<clang::TemplateArgument> list) {
    vector<std::string> ss;
    for (const auto &ta : list) {
        std::string s;
        llvm::raw_string_ostream os(s);
        clang::TemplateName tn;
        switch (ta.getKind()) {
            case clang::TemplateArgument::ArgKind::Type:
                s = get_full_type_name(ctx, ta.getAsType());
                break;
            case clang::TemplateArgument::ArgKind::Declaration:
                s = ta.getAsDecl()->getQualifiedNameAsString();
                break;
            case clang::TemplateArgument::ArgKind::NullPtr:
                s = "nullptr";
                break;
            case clang::TemplateArgument::ArgKind::Integral:
                ta.print(ctx.ast_ctx->getPrintingPolicy(), os, false);
                break;
            case clang::TemplateArgument::ArgKind::Template:
            case clang::TemplateArgument::ArgKind::TemplateExpansion:
                tn = ta.getAsTemplateOrTemplatePattern();
                tn.print(os, ctx.ast_ctx->getPrintingPolicy());
                break;
            case clang::TemplateArgument::ArgKind::Expression:
                s = get_fully_qualified_expr(ctx, *ta.getAsExpr());
                break;
            case clang::TemplateArgument::ArgKind::Pack:
                if (auto args =
                        get_template_args_for_emit(
                            ctx,
                            ta.pack_elements())) {
                    s = *args;

                    // possible to get an argument pack with no arguments -
                    // in that case, just continue on to the next
                    if (s.empty()) {
                        continue;
                    }
                } else {
                    return std::nullopt;
                }
                break;
            default:
                break;
        }

        ss.push_back(s);
    }
    return fmt::format("{}", fmt::join(ss, ","));
}

inline std::optional<std::string> get_full_function_name_and_specialization(
    const Context &ctx,
    const clang::FunctionDecl &decl) {
    const auto name = decl.getQualifiedNameAsString();
    if (decl.isFunctionTemplateSpecialization()) {
        const auto args =
            get_template_args_for_emit(
                ctx,
                decl.getTemplateSpecializationArgs()->asArray());

        if (!args) {
            return std::nullopt;
        }

        return fmt::format("{}<{}>", name, *args);
    } else {
        return name;
    }
}

inline std::optional<std::string> get_full_function_name_type_and_specialization(
    const Context &ctx,
    const clang::FunctionDecl &decl) {
    const auto name = decl.getQualifiedNameAsString();
    if (decl.getTemplateSpecializationArgs()) {
        const auto args =
            get_template_args_for_emit(
                ctx,
                decl.getTemplateSpecializationArgs()->asArray());

        if (!args) {
            return std::nullopt;
        }

        const auto type = get_full_type_name(ctx, decl.getType());
        const auto full_name = fmt::format("{}<{}>", name, *args);
        return type.substr(0, type.find('('))
            + full_name
            + type.substr(type.find(')'));
    } else {
        return name;
    }
}
} // namespace archimedes

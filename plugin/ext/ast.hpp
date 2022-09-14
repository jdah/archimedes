//--------------------------------------------------------------------*- C++ -*-
// CLING - the C++ LLVM-based InterpreterG :)
// author:  Vassil Vassilev <vasil.georgiev.vasilev@cern.ch>
//
// This file is dual-licensed: you can choose to license it under the University
// of Illinois Open Source License or the GNU Lesser General Public License. See
// LICENSE.TXT for details.
//------------------------------------------------------------------------------

#ifndef CLING_UTILS_AST_H
#define CLING_UTILS_AST_H

#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"

namespace clang {
  class ASTContext;
  class Expr;
  class Decl;
  class DeclContext;
  class DeclarationName;
  class FunctionDecl;
  class GlobalDecl;
  class IntegerLiteral;
  class LookupResult;
  class NamedDecl;
  class NamespaceDecl;
  class NestedNameSpecifier;
  class QualType;
  class Sema;
  class TagDecl;
  class TemplateDecl;
  class Type;
  class TypedefNameDecl;
  class TemplateName;
  struct PrintingPolicy;
}

namespace cling {
namespace utils {
  ///\brief Class containing static utility functions transforming AST nodes or
  /// types.
  ///
  namespace Transform {

    ///\brief Class containing the information on how to configure the
    /// transformation
    ///
    struct Config {
      typedef llvm::SmallSet<const clang::Decl*, 4> SkipCollection;
      typedef const clang::Type cType;
      typedef llvm::DenseMap<cType*, cType*> ReplaceCollection;

      SkipCollection    m_toSkip;
      ReplaceCollection m_toReplace;

      bool empty() const { return m_toSkip.size()==0 && m_toReplace.empty(); }
    };

    ///\brief Remove one layer of sugar, but only some kinds.
    bool SingleStepPartiallyDesugarType(clang::QualType& QT,
                                        const clang::ASTContext& C);

    ///\brief "Desugars" a type while skipping the ones in the set.
    ///
    /// Desugars a given type recursively until strips all sugar or until gets a
    /// sugared type, which is to be skipped.
    ///\param[in] Ctx - The ASTContext.
    ///\param[in] QT - The type to be partially desugared.
    ///\param[in] TypeConfig - The set of sugared types which shouldn't be
    ///                        desugared and those that should be replaced.
    ///\param[in] fullyQualify - if true insert Elaborated where needed.
    ///\returns Partially desugared QualType
    ///
    clang::QualType
    GetPartiallyDesugaredType(const clang::ASTContext& Ctx, clang::QualType QT,
                              const Config& TypeConfig,
                              bool fullyQualify = true);

  }

  namespace TypeName {
    ///\brief Convert the type into one with fully qualified template
    /// arguments.
    ///\param[in] QT - the type for which the fully qualified type will be
    /// returned.
    ///\param[in] Ctx - the ASTContext to be used.
    clang::QualType GetFullyQualifiedType(clang::QualType QT,
                                          const clang::ASTContext& Ctx);

    ///\brief Get the fully qualified name for a type. This includes full
    /// qualification of all template parameters etc.
    ///
    ///\param[in] QT - the type for which the fully qualified name will be
    /// returned.
    ///\param[in] Ctx - the ASTContext to be used.
    ///\param[in] Policy - NEW!!!
    std::string GetFullyQualifiedName(clang::QualType QT,
                                      const clang::ASTContext &Ctx,
                                      const clang::PrintingPolicy &Policy);

    ///\brief Create a NestedNameSpecifier for Namesp and its enclosing
    /// scopes.
    ///
    ///\param[in] Ctx - the AST Context to be used.
    ///\param[in] Namesp - the NamespaceDecl for which a NestedNameSpecifier
    /// is requested.
    clang::NestedNameSpecifier*
    CreateNestedNameSpecifier(const clang::ASTContext& Ctx,
                              const clang::NamespaceDecl* Namesp);

    ///\brief Create a NestedNameSpecifier for TagDecl and its enclosing
    /// scopes.
    ///
    ///\param[in] Ctx - the AST Context to be used.
    ///\param[in] TD - the TagDecl for which a NestedNameSpecifier is
    /// requested.
    ///\param[in] FullyQualify - Convert all template arguments into fully
    /// qualified names.
    clang::NestedNameSpecifier*
    CreateNestedNameSpecifier(const clang::ASTContext& Ctx,
                              const clang::TagDecl *TD, bool FullyQualify);

    ///\brief Create a NestedNameSpecifier for TypedefDecl and its enclosing
    /// scopes.
    ///
    ///\param[in] Ctx - the AST Context to be used.
    ///\param[in] TD - the TypedefDecl for which a NestedNameSpecifier is
    /// requested.
    ///\param[in] FullyQualify - Convert all template arguments (of possible
    /// parent scopes) into fully qualified names.
    clang::NestedNameSpecifier*
    CreateNestedNameSpecifier(const clang::ASTContext& Ctx,
                              const clang::TypedefNameDecl *TD,
                              bool FullyQualify);

    // MODIFIED
    bool GetFullyQualifiedTemplateName(const clang::ASTContext &Ctx,
                                        clang::TemplateName &tname);

  } // end namespace TypeName
} // end namespace utils
} // end namespace cling
#endif // CLING_UTILS_AST_H

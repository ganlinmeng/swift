//===--- ParameterList.h - Functions & closures parameter lists -*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file defines the ParameterList class and support logic.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_AST_PARAMETERLIST_H
#define SWIFT_AST_PARAMETERLIST_H

#include "swift/AST/Decl.h"
#include "swift/Basic/OptionSet.h"
#include "llvm/Support/TrailingObjects.h"

namespace swift {

/// This describes a list of parameters.  Each parameter descriptor is tail
/// allocated onto this list.
class alignas(ParamDecl *) ParameterList final :
    private llvm::TrailingObjects<ParameterList, ParamDecl *> {
  friend TrailingObjects;

  void *operator new(size_t Bytes) throw() = delete;
  void operator delete(void *Data) throw() = delete;
  void *operator new(size_t Bytes, void *Mem) throw() = delete;
  void *operator new(size_t Bytes, ASTContext &C,
                     unsigned Alignment = 8);

  SourceLoc LParenLoc, RParenLoc;
  size_t numParameters;

  ParameterList(SourceLoc LParenLoc, size_t numParameters, SourceLoc RParenLoc)
    : LParenLoc(LParenLoc), RParenLoc(RParenLoc), numParameters(numParameters){}
  void operator=(const ParameterList&) = delete;
public:
  /// Create a parameter list with the specified parameters.
  static ParameterList *create(const ASTContext &C, SourceLoc LParenLoc,
                               ArrayRef<ParamDecl*> params,
                               SourceLoc RParenLoc);

  /// Create a parameter list with the specified parameters, with no location
  /// info for the parens.
  static ParameterList *create(const ASTContext &C,
                               ArrayRef<ParamDecl*> params) {
    return create(C, SourceLoc(), params, SourceLoc());
  }
 
  /// Create an empty parameter list.
  static ParameterList *createEmpty(const ASTContext &C,
                                    SourceLoc LParenLoc = SourceLoc(),
                                    SourceLoc RParenLoc = SourceLoc()) {
    return create(C, LParenLoc, {}, RParenLoc);
  }
  
  /// Create a parameter list for a single parameter lacking location info.
  static ParameterList *createWithoutLoc(ParamDecl *decl) {
    return create(decl->getASTContext(), decl);
  }

  SourceLoc getLParenLoc() const { return LParenLoc; }
  SourceLoc getRParenLoc() const { return RParenLoc; }
  
  typedef MutableArrayRef<ParamDecl*>::iterator iterator;
  typedef ArrayRef<ParamDecl*>::iterator const_iterator;
  iterator begin() { return getArray().begin(); }
  iterator end() { return getArray().end(); }
  const_iterator begin() const { return getArray().begin(); }
  const_iterator end() const { return getArray().end(); }
  
  MutableArrayRef<ParamDecl*> getArray() {
    return {getTrailingObjects<ParamDecl*>(), numParameters};
  }
  ArrayRef<ParamDecl*> getArray() const {
    return {getTrailingObjects<ParamDecl*>(), numParameters};
  }

  size_t size() const {
    return numParameters;
  }
  
  const ParamDecl *get(unsigned i) const {
    return getArray()[i];
  }
  
  ParamDecl *&get(unsigned i) {
    return getArray()[i];
  }

  const ParamDecl *operator[](unsigned i) const { return get(i); }
  ParamDecl *&operator[](unsigned i) { return get(i); }
  bool hasInternalParameter(StringRef prefix) const;

  /// Change the DeclContext of any contained parameters to the specified
  /// DeclContext.
  void setDeclContextOfParamDecls(DeclContext *DC);
  
  
  /// Flags used to indicate how ParameterList cloning should operate.
  enum CloneFlags {
    /// The cloned ParamDecls should be marked implicit.
    Implicit = 0x01,
    /// The cloned pattern is for an inherited constructor; mark default
    /// arguments as inherited, and mark unnamed arguments as named.
    Inherited = 0x02,
    /// The cloned pattern will strip type information.
    WithoutTypes = 0x04,
  };

  friend OptionSet<CloneFlags> operator|(CloneFlags flag1, CloneFlags flag2) {
    return OptionSet<CloneFlags>(flag1) | flag2;
  }

  /// Make a duplicate copy of this parameter list.  This allocates copies of
  /// the ParamDecls, so they can be reparented into a new DeclContext.
  ParameterList *clone(const ASTContext &C,
                       OptionSet<CloneFlags> options = None) const;

  /// Return a list of function parameters for this parameter list,
  /// based on the interface types of the parameters in this list.
  void getParams(SmallVectorImpl<AnyFunctionType::Param> &params) const;

  /// Return a list of function parameters for this parameter list,
  /// based on types provided by a callback.
  void getParams(SmallVectorImpl<AnyFunctionType::Param> &params,
                 llvm::function_ref<Type(ParamDecl *)> getType) const;


  /// Return the full source range of this parameter.
  SourceRange getSourceRange() const;
  SourceLoc getStartLoc() const { return getSourceRange().Start; }
  SourceLoc getEndLoc() const { return getSourceRange().End; }

  void dump() const;
  void dump(raw_ostream &OS, unsigned Indent = 0) const;
  
  //  void print(raw_ostream &OS) const;
};

} // end namespace swift

#endif

//===--- CodeCompletionResultPrinter.cpp ----------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2020 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "swift/Basic/LLVM.h"
#include "swift/IDE/CodeCompletionResultPrinter.h"
#include "swift/IDE/CodeCompletion.h"
#include "swift/Markup/XMLUtils.h"
#include "llvm/Support/raw_ostream.h"

using namespace swift;
using namespace swift::ide;

using ChunkKind = CodeCompletionString::Chunk::ChunkKind;

void swift::ide::printCodeCompletionResultDescription(
    const CodeCompletionResult &result,

    raw_ostream &OS, bool leadingPunctuation) {
  auto str = result.getCompletionString();
  bool isOperator = result.isOperator();

  auto FirstTextChunk = str->getFirstTextChunkIndex(leadingPunctuation);
  int TextSize = 0;
  if (FirstTextChunk.hasValue()) {
    auto Chunks = str->getChunks().slice(*FirstTextChunk);
    for (auto I = Chunks.begin(), E = Chunks.end(); I != E; ++I) {
      const auto &C = *I;

      using ChunkKind = CodeCompletionString::Chunk::ChunkKind;

      if (C.is(ChunkKind::TypeAnnotation) ||
          C.is(ChunkKind::CallParameterClosureType) ||
          C.is(ChunkKind::CallParameterClosureExpr) ||
          C.is(ChunkKind::Whitespace))
        continue;

      if (isOperator && C.is(ChunkKind::CallParameterType))
        continue;
      if (isOperator && C.is(ChunkKind::CallParameterTypeBegin)) {
        auto nestingLevel = C.getNestingLevel();
        ++I;
        while (I != E && I->endsPreviousNestedGroup(nestingLevel))
          ++I;
        --I;
        continue;
      }
      if (C.hasText()) {
        TextSize += C.getText().size();
        OS << C.getText();
      }
    }
  }
  assert((TextSize > 0) &&
         "code completion result should have non-empty description!");
}

namespace {
class AnnotatingDescriptionPrinter {
  raw_ostream &OS;

  /// Print \p content enclosing with \p tag.
  void printWithTag(StringRef tag, StringRef content) {
    // Trim whitepsaces around the non-whitespace characters.
    // (i.e. "  something   " -> "  <tag>something</tag>   ".
    auto ltrimIdx = content.find_first_not_of(' ');
    auto rtrimIdx = content.find_last_not_of(' ') + 1;
    assert(ltrimIdx != StringRef::npos && rtrimIdx != StringRef::npos &&
           "empty or whitespace only element");

    OS << content.substr(0, ltrimIdx);
    OS << "<" << tag << ">";
    swift::markup::appendWithXMLEscaping(
        OS, content.substr(ltrimIdx, rtrimIdx - ltrimIdx));
    OS << "</" << tag << ">";
    OS << content.substr(rtrimIdx);
  }

  void printTextChunk(CodeCompletionString::Chunk C) {
    if (!C.hasText())
      return;

    switch (C.getKind()) {
    case ChunkKind::Keyword:
    case ChunkKind::OverrideKeyword:
    case ChunkKind::AccessControlKeyword:
    case ChunkKind::ThrowsKeyword:
    case ChunkKind::RethrowsKeyword:
    case ChunkKind::DeclIntroducer:
      printWithTag("keyword", C.getText());
      break;
    case ChunkKind::DeclAttrKeyword:
    case ChunkKind::Attribute:
      printWithTag("attribute", C.getText());
      break;
    case ChunkKind::BaseName:
      printWithTag("name", C.getText());
      break;
    case ChunkKind::TypeIdSystem:
      printWithTag("typeid.sys", C.getText());
      break;
    case ChunkKind::TypeIdUser:
      printWithTag("typeid.user", C.getText());
      break;
    case ChunkKind::CallParameterName:
      printWithTag("callarg.label", C.getText());
      break;
    case ChunkKind::CallParameterInternalName:
      printWithTag("callarg.param", C.getText());
      break;
    case ChunkKind::TypeAnnotation:
    case ChunkKind::CallParameterClosureType:
    case ChunkKind::CallParameterClosureExpr:
    case ChunkKind::Whitespace:
      // ignore;
      break;
    default:
      swift::markup::appendWithXMLEscaping(OS, C.getText());
      break;
    }
  }

  void printCallArg(ArrayRef<CodeCompletionString::Chunk> chunks) {
    OS << "<callarg>";
    for (auto i = chunks.begin(), e = chunks.end(); i != e; ++i) {
      using ChunkKind = CodeCompletionString::Chunk::ChunkKind;

      if (i->is(ChunkKind::CallParameterTypeBegin)) {
        OS << "<callarg.type>";
        auto nestingLevel = i->getNestingLevel();
        i++;
        for (; i != e; ++i) {
          if (i->endsPreviousNestedGroup(nestingLevel))
            break;
          if (i->hasText())
            printTextChunk(*i);
        }
        OS << "</callarg.type>";
        if (i == e)
          break;
      }

      printTextChunk(*i);
    }
    OS << "</callarg>";
  }

public:
  AnnotatingDescriptionPrinter(raw_ostream &OS) : OS(OS) {}

  void print(const CodeCompletionResult &result, bool leadingPunctuation) {
    auto str = result.getCompletionString();
    bool isOperator = result.isOperator();

    auto FirstTextChunk = str->getFirstTextChunkIndex(leadingPunctuation);
    if (FirstTextChunk.hasValue()) {
      auto chunks = str->getChunks().slice(*FirstTextChunk);
      for (auto i = chunks.begin(), e = chunks.end(); i != e; ++i) {
        using ChunkKind = CodeCompletionString::Chunk::ChunkKind;
        if (i->is(ChunkKind::CallParameterBegin)) {
          auto start = i++;
          for (; i != e; ++i) {
            if (i->endsPreviousNestedGroup(start->getNestingLevel()))
              break;
          }
          if (!isOperator)
            printCallArg({start, i});
          if (i == e)
            break;
        }
        if (isOperator && i->is(ChunkKind::CallParameterType))
          continue;
        printTextChunk(*i);
      }
    }
  }
};

} // namespace

void swift::ide::printCodeCompletionResultDescriptionAnnotated(
    const CodeCompletionResult &Result, raw_ostream &OS,
    bool leadingPunctuation) {
  AnnotatingDescriptionPrinter printer(OS);
  printer.print(Result, leadingPunctuation);
}

#pragma once

#include "../parser/AST.h"
#include "SymbolTable.h"
#include <iostream>

namespace cc30 {

class TypeChecker {
public:
  TypeChecker() = default;

  bool check(Module *module) {
    // register globals
    for (auto &decl : module->declarations) {
      if (auto func = dynamic_cast<FunctionDecl *>(decl.get())) {
        symbols.declare(func->name.text, func);
      } else if (auto st = dynamic_cast<StructDecl *>(decl.get())) {
        symbols.declare(st->name.text, st);
      }
    }

    // check bodies
    for (auto &decl : module->declarations) {
      checkDecl(decl.get());
    }
    return !hadError;
  }

private:
  void checkDecl(Decl *decl) {
    if (auto func = dynamic_cast<FunctionDecl *>(decl)) {
      symbols.enterScope();
      for (auto &param : func->params) {
        // TODO: declare params
      }
      checkBlock(func->body.get());
      symbols.exitScope();
    }
  }

  void checkBlock(Block *block) {
    symbols.enterScope();
    for (auto &stmt : block->statements) {
      checkStmt(stmt.get());
    }
    symbols.exitScope();
  }

  void checkStmt(Stmt *stmt) {
    if (auto let = dynamic_cast<LetStmt *>(stmt)) {
      if (let->initializer) {
        // check type compatibility
      }
      symbols.declare(let->name.text,
                      nullptr); // Decl is not stored in SymbolTable properly in
                                // this stub yet
    } else if (auto ret = dynamic_cast<ReturnStmt *>(stmt)) {
      // check return type
    } else if (auto expr = dynamic_cast<ExprStmt *>(stmt)) {
      // check expr
    } else if (auto blk = dynamic_cast<Block *>(stmt)) {
      checkBlock(blk);
    }
  }

private:
  SymbolTable symbols;
  bool hadError = false;
};

} // namespace cc30

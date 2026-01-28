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
        symbols.declare(param.first.text, nullptr, param.second); 
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
      std::optional<Type> inferred;
      if (let->initializer) {
        inferred = checkExpr(let->initializer->get());
      }
      // If type is explicit, check compatibility
      if (let->type && inferred) {
          // Todo: check compatibility
      }
      symbols.declare(let->name.text, nullptr, let->type ? let->type : inferred);
    } else if (auto ret = dynamic_cast<ReturnStmt *>(stmt)) {
      if (ret->value)
        checkExpr(ret->value->get());
    } else if (auto expr = dynamic_cast<ExprStmt *>(stmt)) {
      checkExpr(expr->expression.get());
    } else if (auto ifStmt = dynamic_cast<IfStmt *>(stmt)) {
      checkExpr(ifStmt->condition.get());
      checkBlock(ifStmt->thenBranch.get());
      if (ifStmt->elseBranch) {
        if (auto elseBlk = dynamic_cast<Block *>(ifStmt->elseBranch->get())) {
          checkBlock(elseBlk);
        } else if (auto elseIf =
                       dynamic_cast<IfStmt *>(ifStmt->elseBranch->get())) {
          checkStmt(elseIf);
        }
      }
    } else if (auto whileStmt = dynamic_cast<WhileStmt *>(stmt)) {
      checkExpr(whileStmt->condition.get());
      checkBlock(whileStmt->body.get());
    } else if (auto blk = dynamic_cast<Block *>(stmt)) {
      checkBlock(blk);
    }
  }

  Type checkExpr(Expr *expr) {
    if (auto id = dynamic_cast<IdentifierExpr *>(expr)) {
      auto sym = symbols.resolve(id->name.text);
      if (!sym) {
        // std::cerr << "Semantic Error: " << id->name.text << " not found.\n";
        // hadError = true;
        return Type("error");
      }
      return sym->type.value_or(Type("unknown"));
    } else if (auto bin = dynamic_cast<BinaryExpr *>(expr)) {
      checkExpr(bin->left.get());
      checkExpr(bin->right.get());
      return Type("i32"); // Placeholder
    } else if (auto call = dynamic_cast<CallExpr *>(expr)) {
      checkExpr(call->callee.get());
      for (auto &arg : call->args)
        checkExpr(arg.get());
      return Type("unknown"); // Needs function lookup
    } else if (auto match = dynamic_cast<MatchExpr *>(expr)) {
       Type targetType = checkExpr(match->target.get());
       // Verify patterns and bodies
       for(auto& arm : match->arms) {
           checkExpr(arm.pattern.get());
           checkExpr(arm.body.get());
       }
       return Type("unknown"); // Should unify body types
    } else if (auto tryExpr = dynamic_cast<TryExpr *>(expr)) {
        Type target = checkExpr(tryExpr->target.get());
        // Verify target is Result
        return target; 
    } else if (auto member = dynamic_cast<MemberAccessExpr *>(expr)) {
      Type objType = checkExpr(member->object.get());
      // For now, return unknown or lookup field type if struct
      return Type("unknown");
    } else if (auto idx = dynamic_cast<IndexExpr *>(expr)) {
      checkExpr(idx->array.get());
      checkExpr(idx->index.get());
      return Type("unknown"); // Should be inner array type
    } else if (auto path = dynamic_cast<PathExpr *>(expr)) {
      return Type("unknown"); 
    } else if (auto lit = dynamic_cast<LiteralExpr *>(expr)) {
        if(lit->literal.kind == TokenKind::Integer) return Type("i32");
        if(lit->literal.kind == TokenKind::Float) return Type("f32");
        if(lit->literal.kind == TokenKind::String) return Type("string");
        return Type("bool");
    } else if (auto st = dynamic_cast<StructLiteralExpr *>(expr)) {
        // Check fields
        for(auto& f : st->fields) {
            checkExpr(f.second.get());
        }
        return Type(st->typeName.text);
    }
    
    return Type("void"); 
  }

private:
  SymbolTable symbols;
  bool hadError = false;
};

} // namespace cc30

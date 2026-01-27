#pragma once

#include "../lexer/Token.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cc30 {

// --- Base Nodes ---

struct Node {
  virtual ~Node() = default;
};

struct Stmt : Node {};
struct Expr : Node {};
struct Decl : Stmt {}; // Declarations are statements in many contexts

using StmtPtr = std::unique_ptr<Stmt>;
using ExprPtr = std::unique_ptr<Expr>;
using DeclPtr = std::unique_ptr<Decl>;

// --- Expressions ---

struct IdentifierExpr : Expr {
  Token name;
  IdentifierExpr(Token name) : name(std::move(name)) {}
};

struct LiteralExpr : Expr {
  Token literal; // Integer, Float, String, Bool
  LiteralExpr(Token literal) : literal(std::move(literal)) {}
};

struct BinaryExpr : Expr {
  ExprPtr left;
  Token op;
  ExprPtr right;
  BinaryExpr(ExprPtr left, Token op, ExprPtr right)
      : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
};

struct CallExpr : Expr {
  ExprPtr callee;
  std::vector<ExprPtr> args;
  CallExpr(ExprPtr callee, std::vector<ExprPtr> args)
      : callee(std::move(callee)), args(std::move(args)) {}
};

// --- Types ---

struct Type {
  std::string name;
  // TODO: Add support for generic params, pointers, arrays
};

// --- Statements ---

struct Block : Stmt {
  std::vector<StmtPtr> statements;
};

struct ReturnStmt : Stmt {
  Token keyword;
  std::optional<ExprPtr> value;
  ReturnStmt(Token keyword, std::optional<ExprPtr> value)
      : keyword(std::move(keyword)), value(std::move(value)) {}
};

struct LetStmt : Stmt {
  Token name;
  std::optional<Type> type;
  std::optional<ExprPtr> initializer;
  bool isMut;

  LetStmt(Token name, std::optional<Type> type, std::optional<ExprPtr> init,
          bool isMut)
      : name(std::move(name)), type(std::move(type)),
        initializer(std::move(init)), isMut(isMut) {}
};

struct ExprStmt : Stmt {
  ExprPtr expression;
  ExprStmt(ExprPtr expr) : expression(std::move(expr)) {}
};

// --- Declarations ---

struct FunctionDecl : Decl {
  Token name;
  std::vector<std::pair<Token, Type>> params; // name: Type
  std::optional<Type> returnType;
  std::unique_ptr<Block> body;
  bool isPublic;

  FunctionDecl(Token name, bool isPublic)
      : name(std::move(name)), isPublic(isPublic) {}
};

struct StructDecl : Decl {
  Token name;
  std::vector<std::pair<Token, Type>> fields;
  bool isPublic;

  StructDecl(Token name, bool isPublic)
      : name(std::move(name)), isPublic(isPublic) {}
};

struct Module : Node {
  std::string name;
  std::vector<DeclPtr> declarations;
};

} // namespace cc30

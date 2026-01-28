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

struct PathExpr : Expr {
  std::vector<Token> segments;
  PathExpr(std::vector<Token> segs) : segments(std::move(segs)) {}
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

struct MemberAccessExpr : Expr {
  ExprPtr object;
  Token member;
  MemberAccessExpr(ExprPtr obj, Token mem)
      : object(std::move(obj)), member(std::move(mem)) {}
};

struct IndexExpr : Expr {
  ExprPtr array;
  ExprPtr index;
  IndexExpr(ExprPtr arr, ExprPtr idx)
      : array(std::move(arr)), index(std::move(idx)) {}
};

struct StructLiteralExpr : Expr {
  Token typeName;
  std::vector<std::pair<Token, ExprPtr>> fields; // name: expr

  StructLiteralExpr(Token type, std::vector<std::pair<Token, ExprPtr>> f)
      : typeName(std::move(type)), fields(std::move(f)) {}
};

// ... (StructLiteralExpr above)

struct TryExpr : Expr {
  ExprPtr target;
  TryExpr(ExprPtr t) : target(std::move(t)) {}
};

struct MatchArm {
  ExprPtr pattern;
  ExprPtr body;
  MatchArm(ExprPtr p, ExprPtr b) : pattern(std::move(p)), body(std::move(b)) {}
};

struct MatchExpr : Expr {
  ExprPtr target;
  std::vector<MatchArm> arms;
  MatchExpr(ExprPtr t, std::vector<MatchArm> a)
      : target(std::move(t)), arms(std::move(a)) {}
};

// --- Types ---

struct Type {
  std::string name;
  bool isArray = false;
  std::shared_ptr<Type> innerType; // Changed to shared_ptr for easier copy or
                                   // use vector for generics
  // MVP Generic Support:
  std::vector<Type> generics;
  int arraySize = 0;

  Type() = default;
  Type(std::string n) : name(std::move(n)) {}

  // Re-implement copy constructor/assignment if using raw pointers or
  // unique_ptr Switching to shared_ptr simplifies this but let's stick to deep
  // copy if unique_ptr or just use vector<Type> which is copyable.

  // Simplified for MVP:
  // If name is "Result", look at generics[0] and generics[1].

  static Type makeArray(Type inner, int size) {
    Type t;
    t.name = "[" + inner.name + ";" + std::to_string(size) + "]";
    t.isArray = true;
    t.generics.push_back(inner); // Store inner in generics for uniformity?
    // t.innerType = std::make_unique<Type>(std::move(inner)); // Deprecate
    // specific field?
    t.arraySize = size;
    return t;
  }
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

// --- Control Flow ---

struct IfStmt : Stmt {
  ExprPtr condition;
  std::unique_ptr<Block> thenBranch;
  std::optional<StmtPtr> elseBranch; // Can be Block or IfStmt (else if)

  IfStmt(ExprPtr cond, std::unique_ptr<Block> thenB,
         std::optional<StmtPtr> elseB)
      : condition(std::move(cond)), thenBranch(std::move(thenB)),
        elseBranch(std::move(elseB)) {}
};

struct WhileStmt : Stmt {
  ExprPtr condition;
  std::unique_ptr<Block> body;

  WhileStmt(ExprPtr cond, std::unique_ptr<Block> body)
      : condition(std::move(cond)), body(std::move(body)) {}
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

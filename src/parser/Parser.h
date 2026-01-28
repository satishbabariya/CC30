#pragma once

#include "../lexer/Lexer.h"
#include "AST.h"
#include <memory>
#include <string>
#include <vector>

namespace cc30 {

class Parser {
public:
  explicit Parser(Lexer &lexer);

  std::unique_ptr<Module> parseModule();

private:
  // Declarations
  DeclPtr parseDecl();
  DeclPtr parseFunctionDecl(bool isPublic);
  DeclPtr parseStructDecl(bool isPublic);

  // Statements
  StmtPtr parseStmt();
  StmtPtr parseLetStmt();
  StmtPtr parseReturnStmt();
  StmtPtr parseIfStmt();
  StmtPtr parseWhileStmt();
  std::unique_ptr<Block> parseBlock();

  // Expressions
  ExprPtr parseExpr();
  ExprPtr parsePrimary();
  ExprPtr parseBinary(int precedence, ExprPtr left);
  int getPrecedence(TokenKind kind);

  // Helpers
  bool match(TokenKind kind);
  bool check(TokenKind kind);
  Token consume(TokenKind kind, std::string message);
  Token advance();
  Token peek();
  Token previous();
  bool isAtEnd();
  void synchronize();

  Type parseType();

private:
  Lexer &m_lexer;
  Token m_current;
  Token m_previous; // Useful for error reporting
};

} // namespace cc30

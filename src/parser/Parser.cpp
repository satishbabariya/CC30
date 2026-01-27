#include "Parser.h"
#include <iostream>

namespace cc30 {

Parser::Parser(Lexer &lexer) : m_lexer(lexer) {
  // Prime the pump
  m_current = m_lexer.next();
}

Token Parser::peek() { return m_current; }

Token Parser::previous() {
  // Note: This requires tracking previous token which we didn't fully implement
  // in Lexer interface yet simple workaround: store previous in advance()
  return m_previous;
}

Token Parser::advance() {
  if (!isAtEnd()) {
    m_previous = m_current;
    m_current = m_lexer.next();
  }
  return m_previous;
}

bool Parser::isAtEnd() { return m_current.kind == TokenKind::Eof; }

bool Parser::check(TokenKind kind) {
  if (isAtEnd())
    return false;
  return m_current.kind == kind;
}

bool Parser::match(TokenKind kind) {
  if (check(kind)) {
    advance();
    return true;
  }
  return false;
}

Token Parser::consume(TokenKind kind, std::string message) {
  if (check(kind))
    return advance();

  std::cerr << "Parser Error at " << m_current.line << ":" << m_current.column
            << " - " << message << "\n";
  // For now, return a placeholder or throw.
  // Ideally we enter panic mode. Simple return for now.
  return m_current;
}

// --- Parsing Logic ---

std::unique_ptr<Module> Parser::parseModule() {
  auto module = std::make_unique<Module>();

  // Optional: module name;
  if (match(TokenKind::KwModule)) {
    Token name = consume(TokenKind::Identifier, "Expect module name.");
    module->name = name.text;
    consume(TokenKind::SemiColon, "Expect ';' after module name.");
  } else {
    module->name = "main";
  }

  // Imports
  while (match(TokenKind::KwImport)) {
    consume(TokenKind::Identifier, "Expect import name.");
    consume(TokenKind::SemiColon, "Expect ';' after import.");
    // TODO: Store imports in AST
  }

  // Declarations
  while (!isAtEnd()) {
    try {
      module->declarations.push_back(parseDecl());
    } catch (...) {
      synchronize();
    }
  }

  return module;
}

DeclPtr Parser::parseDecl() {
  bool isPublic = match(TokenKind::KwExport);

  if (match(TokenKind::KwFn))
    return parseFunctionDecl(isPublic);
  if (match(TokenKind::KwStruct))
    return parseStructDecl(isPublic);

  // Error or other decls
  consume(TokenKind::Identifier, "Expect declaration.");
  return nullptr;
}

DeclPtr Parser::parseFunctionDecl(bool isPublic) {
  Token name = consume(TokenKind::Identifier, "Expect function name.");
  consume(TokenKind::OpenParen, "Expect '(' after function name.");

  // Params
  auto func = std::make_unique<FunctionDecl>(name, isPublic);

  if (!check(TokenKind::CloseParen)) {
    do {
      Token paramName =
          consume(TokenKind::Identifier, "Expect parameter name.");
      consume(TokenKind::Colon, "Expect ':' after parameter name.");
      Type type = parseType();
      func->params.push_back({paramName, type});
    } while (match(TokenKind::Comma));
  }
  consume(TokenKind::CloseParen, "Expect ')' after parens.");

  // Return Type
  if (match(TokenKind::Arrow)) {
    func->returnType = parseType();
  }

  consume(TokenKind::OpenBrace, "Expect '{' before function body.");
  func->body = parseBlock();

  return func;
}

DeclPtr Parser::parseStructDecl(bool isPublic) {
  Token name = consume(TokenKind::Identifier, "Expect struct name.");
  consume(TokenKind::OpenBrace, "Expect '{' before struct body.");

  auto st = std::make_unique<StructDecl>(name, isPublic);

  while (!check(TokenKind::CloseBrace) && !isAtEnd()) {
    Token fieldName = consume(TokenKind::Identifier, "Expect field name.");
    consume(TokenKind::Colon, "Expect ':' after field name.");
    Type fieldType = parseType();
    st->fields.push_back({fieldName, fieldType});
    consume(TokenKind::Comma,
            "Expect ',' after field."); // Optional? Enforce for now
  }

  consume(TokenKind::CloseBrace, "Expect '}' after struct body.");
  return st;
}

Type Parser::parseType() {
  Token token = consume(TokenKind::Identifier, "Expect type name.");
  Type t;
  t.name = token.text;
  return t;
}

// --- Statements ---

std::unique_ptr<Block> Parser::parseBlock() {
  auto block = std::make_unique<Block>();

  while (!check(TokenKind::CloseBrace) && !isAtEnd()) {
    block->statements.push_back(parseStmt());
  }

  consume(TokenKind::CloseBrace, "Expect '}' after block.");
  return block;
}

StmtPtr Parser::parseStmt() {
  if (match(TokenKind::KwLet))
    return parseLetStmt();
  if (match(TokenKind::KwReturn))
    return parseReturnStmt();
  if (match(TokenKind::OpenBrace))
    return parseBlock(); // Nested block

  // Expr Stmt
  ExprPtr expr = parseExpr();
  consume(TokenKind::SemiColon, "Expect ';' after expression.");
  return std::make_unique<ExprStmt>(std::move(expr));
}

StmtPtr Parser::parseLetStmt() {
  bool isMut = match(TokenKind::KwMut);
  Token name = consume(TokenKind::Identifier, "Expect variable name.");

  std::optional<Type> type;
  if (match(TokenKind::Colon)) {
    type = parseType();
  }

  std::optional<ExprPtr> init;
  if (match(TokenKind::Equal)) {
    init = parseExpr();
  }

  consume(TokenKind::SemiColon, "Expect ';' after variable declaration.");
  return std::make_unique<LetStmt>(name, type, std::move(init), isMut);
}

StmtPtr Parser::parseReturnStmt() {
  Token keyword = previous();
  std::optional<ExprPtr> value;
  if (!check(TokenKind::SemiColon)) {
    value = parseExpr();
  }
  consume(TokenKind::SemiColon, "Expect ';' after return.");
  return std::make_unique<ReturnStmt>(keyword, std::move(value));
}

// --- Expressions ---

ExprPtr Parser::parseExpr() { return parseBinary(0, parsePrimary()); }

ExprPtr Parser::parsePrimary() {
  if (match(TokenKind::Integer) || match(TokenKind::Float) ||
      match(TokenKind::String)) {
    return std::make_unique<LiteralExpr>(previous());
  }

  if (match(TokenKind::Identifier)) {
    return std::make_unique<IdentifierExpr>(previous());
  }

  // Parenthesis
  if (match(TokenKind::OpenParen)) {
    ExprPtr expr = parseExpr();
    consume(TokenKind::CloseParen, "Expect ')' after expression.");
    return expr;
  }

  consume(TokenKind::Eof, "Expect expression."); // Force error
  return nullptr;
}

ExprPtr Parser::parseBinary(int precedence, ExprPtr left) {
  while (true) {
    if (match(TokenKind::Plus) || match(TokenKind::Minus) ||
        match(TokenKind::Star) || match(TokenKind::Slash)) {
      Token op = previous();
      ExprPtr right = parsePrimary();
      // Note: This ignores precedence for MVP, treating all as same precedence
      // left-associative
      left =
          std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
      continue;
    }
    break;
  }
  return left;
}

void Parser::synchronize() {
  advance();
  while (!isAtEnd()) {
    if (previous().kind == TokenKind::SemiColon)
      return;
    switch (peek().kind) {
    case TokenKind::KwFn:
    case TokenKind::KwStruct:
    case TokenKind::KwLet:
    case TokenKind::KwReturn:
      return;
    default:
      advance();
    }
  }
}

} // namespace cc30

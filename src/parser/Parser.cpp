#include "Parser.h"
#include <iostream>

namespace cc30 {

Parser::Parser(Lexer &lexer) : m_lexer(lexer) { m_current = m_lexer.next(); }

Token Parser::peek() { return m_current; }
Token Parser::previous() { return m_previous; }

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
  return m_current;
}

// --- Parsing Logic ---

std::unique_ptr<Module> Parser::parseModule() {
  auto module = std::make_unique<Module>();

  if (match(TokenKind::KwModule)) {
    Token name = consume(TokenKind::Identifier, "Expect module name.");
    module->name = name.text;
    consume(TokenKind::SemiColon, "Expect ';' after module name.");
  } else {
    module->name = "main";
  }

  while (match(TokenKind::KwImport)) {
    consume(TokenKind::Identifier, "Expect import name.");
    consume(TokenKind::SemiColon, "Expect ';' after import.");
  }

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
  bool isPublic = false;
  if (match(TokenKind::KwPub)) {
    isPublic = true;
  }
  if (match(TokenKind::KwFn))
    return parseFunctionDecl(isPublic);
  if (match(TokenKind::KwStruct))
    return parseStructDecl(isPublic);
  advance();
  return nullptr;
}

DeclPtr Parser::parseFunctionDecl(bool isPublic) {
  Token name = consume(TokenKind::Identifier, "Expect function name.");
  consume(TokenKind::OpenParen, "Expect '(' after function name.");
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
    if (!match(TokenKind::Comma))
      break;
  }
  consume(TokenKind::CloseBrace, "Expect '}' after struct body.");
  return st;
}

Type Parser::parseType() {
  // Array Type: [T; N]
  if (match(TokenKind::OpenBracket)) {
    Type inner = parseType();
    consume(TokenKind::SemiColon, "Expect ';' after array type.");
    Token size = consume(TokenKind::Integer, "Expect array size.");
    consume(TokenKind::CloseBracket, "Expect ']' after array size.");
    return Type::makeArray(inner, std::stoi(size.text));
  }

  Token token = consume(TokenKind::Identifier, "Expect type name.");
  Type t;
  t.name = token.text;

  // Generics: <T, E>
  if (match(TokenKind::Less)) {
    do {
      t.generics.push_back(parseType());
    } while (match(TokenKind::Comma));
    consume(TokenKind::Greater, "Expect '>' after generic arguments.");
  }
  return t;
}

// --- Statements ---

std::unique_ptr<Block> Parser::parseBlock() {
  auto block = std::make_unique<Block>();
  while (!check(TokenKind::CloseBrace) && !isAtEnd()) {
    Token start = peek();
    block->statements.push_back(parseStmt());
    Token end = peek();
    
    // Infinite loop guard: If parseStmt consumed nothing and we aren't at '}', force advance.
    if (start.line == end.line && start.column == end.column) {
         // In a real compiler we'd log an error here.
         // For now, assume simple panic recovery: skip token
         advance(); 
    }
  }
  consume(TokenKind::CloseBrace, "Expect '}' after block.");
  return block;
}



StmtPtr Parser::parseStmt() {
  if (match(TokenKind::KwLet))
    return parseLetStmt();
  if (match(TokenKind::KwReturn))
    return parseReturnStmt();
  if (match(TokenKind::KwIf))
    return parseIfStmt();
  if (match(TokenKind::KwLoop))
    return parseWhileStmt(); // Treating KwLoop as placeholder
  if (match(TokenKind::OpenBrace))
    return parseBlock();

  ExprPtr expr = parseExpr();
  consume(TokenKind::SemiColon, "Expect ';' after expression.");
  return std::make_unique<ExprStmt>(std::move(expr));
}

StmtPtr Parser::parseLetStmt() {
  bool isMut = match(TokenKind::KwMut);
  Token name = consume(TokenKind::Identifier, "Expect variable name.");
  std::optional<Type> type;
  if (match(TokenKind::Colon))
    type = parseType();
  std::optional<ExprPtr> init;
  if (match(TokenKind::Equal))
    init = parseExpr();
  consume(TokenKind::SemiColon, "Expect ';' after variable declaration.");
  return std::make_unique<LetStmt>(name, type, std::move(init), isMut);
}

StmtPtr Parser::parseReturnStmt() {
  Token keyword = previous();
  std::optional<ExprPtr> value;
  if (!check(TokenKind::SemiColon))
    value = parseExpr();
  consume(TokenKind::SemiColon, "Expect ';' after return.");
  return std::make_unique<ReturnStmt>(keyword, std::move(value));
}

StmtPtr Parser::parseIfStmt() {
  ExprPtr condition = parseExpr();
  consume(TokenKind::OpenBrace, "Expect '{' after if condition.");
  std::unique_ptr<Block> thenBranch = parseBlock();
  std::optional<StmtPtr> elseBranch;
  if (match(TokenKind::KwElse)) {
    if (match(TokenKind::KwIf)) {
      elseBranch = parseIfStmt();
    } else {
      consume(TokenKind::OpenBrace, "Expect '{' after else.");
      elseBranch = parseBlock();
    }
  }
  return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch),
                                  std::move(elseBranch));
}

StmtPtr Parser::parseWhileStmt() {
  ExprPtr condition = parseExpr();
  consume(TokenKind::OpenBrace, "Expect '{' after while condition.");
  std::unique_ptr<Block> body = parseBlock();
  return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

// --- Expressions & Precedence ---

int Parser::getPrecedence(TokenKind kind) {
  switch (kind) {
  case TokenKind::OpenBracket:
    return 40; // Indexing (postfix) - wait, Pratt handles prefix/infix. Postfix
               // needs specific care or treat as Infix.
  case TokenKind::Dot:
    return 40; // Member access
  case TokenKind::Star:
  case TokenKind::Slash:
    return 20;
  case TokenKind::Plus:
  case TokenKind::Minus:
    return 10;
  case TokenKind::Less:
  case TokenKind::Greater:
  case TokenKind::LessEqual:
  case TokenKind::GreaterEqual:
  case TokenKind::EqualEqual:
  case TokenKind::NotEqual:
    return 5;
  default:
    return 0;
  }
}

ExprPtr Parser::parseExpr() { return parseBinary(0, parsePrimary()); }

ExprPtr Parser::parsePrimary() {
  ExprPtr expr;

  // 1. Prefix / Atom
  if (match(TokenKind::Integer) || match(TokenKind::Float) ||
      match(TokenKind::String)) {
    expr = std::make_unique<LiteralExpr>(previous());
  } else if (match(TokenKind::Identifier)) {
    Token name = previous();

    // Check for Path: Identifier :: Identifier
    if (check(TokenKind::ColonColon)) {
      std::vector<Token> segments;
      segments.push_back(name);
      while (match(TokenKind::ColonColon)) {
        Token seg =
            consume(TokenKind::Identifier, "Expect identifier after '::'.");
        segments.push_back(seg);
      }
      expr = std::make_unique<PathExpr>(std::move(segments));
    }
    // Struct Literal Check: `Point {`
    else if (check(TokenKind::OpenBrace)) {
      advance(); // Consume '{'
      std::vector<std::pair<Token, ExprPtr>> fields;
      if (!check(TokenKind::CloseBrace)) {
        do {
          Token fieldName =
              consume(TokenKind::Identifier, "Expect field name.");
          consume(TokenKind::Colon, "Expect ':' after field name.");
          ExprPtr init = parseExpr();
          fields.push_back({fieldName, std::move(init)});
          if (!match(TokenKind::Comma))
            break;
        } while (true);
      }
      consume(TokenKind::CloseBrace, "Expect '}' after struct fields.");
      expr = std::make_unique<StructLiteralExpr>(name, std::move(fields));
    } else {
      expr = std::make_unique<IdentifierExpr>(name);
    }
  } else if (match(TokenKind::KwMatch)) {
    ExprPtr target = parseExpr();
    consume(TokenKind::OpenBrace, "Expect '{' after match target.");
    std::vector<MatchArm> arms;
    while (!check(TokenKind::CloseBrace) && !isAtEnd()) {
      ExprPtr pattern = parseExpr();
      consume(TokenKind::Arrow, "Expect '->' after pattern.");
      ExprPtr body = parseExpr();
      arms.push_back(MatchArm(std::move(pattern), std::move(body)));
      if (!match(TokenKind::Comma))
        break;
    }
    consume(TokenKind::CloseBrace, "Expect '}' after match arms.");
    expr = std::make_unique<MatchExpr>(std::move(target), std::move(arms));
  } else if (match(TokenKind::OpenParen)) {
    expr = parseExpr();
    consume(TokenKind::CloseParen, "Expect ')' after expression.");
  } else {
    consume(TokenKind::Eof, "Expect expression.");
    return nullptr;
  }

  // 2. Postfix / Infix Loop (Call, Dot, Index)
  while (true) {
    if (match(TokenKind::Question)) { // Try operator
      expr = std::make_unique<TryExpr>(std::move(expr));
    } else if (match(TokenKind::OpenParen)) { // Call
      std::vector<ExprPtr> args;
      if (!check(TokenKind::CloseParen)) {
        do {
          args.push_back(parseExpr());
        } while (match(TokenKind::Comma));
      }
      consume(TokenKind::CloseParen, "Expect ')' after arguments.");
      expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
    } else if (match(TokenKind::Dot)) { // Member Access
      Token member =
          consume(TokenKind::Identifier, "Expect member name after '.'.");
      expr = std::make_unique<MemberAccessExpr>(std::move(expr),
                                                std::move(member));
    } else if (match(TokenKind::OpenBracket)) { // Indexing
      ExprPtr index = parseExpr();
      consume(TokenKind::CloseBracket, "Expect ']' after index.");
      expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
    } else {
      break;
    }
  }

  return expr;
}

ExprPtr Parser::parseBinary(int minPrecedence, ExprPtr left) {
  while (true) {
    TokenKind kind = peek().kind;
    int precedence = getPrecedence(kind);
    if (precedence <= minPrecedence)
      break;

    advance();
    Token op = previous();
    ExprPtr right = parsePrimary();

    TokenKind nextKind = peek().kind;
    int nextPrecedence = getPrecedence(nextKind);
    if (nextPrecedence > precedence) {
      right = parseBinary(precedence, std::move(right));
    }
    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
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
    case TokenKind::KwIf:
    case TokenKind::KwLoop:
      return;
    default:
      advance();
    }
  }
}

} // namespace cc30

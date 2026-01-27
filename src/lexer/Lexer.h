#pragma once

#include "Token.h"
#include <string>
#include <string_view>

namespace cc30 {

class Lexer {
public:
  explicit Lexer(std::string_view source);

  Token next();

  // Peek at the next token without consuming it (simple lookahead)
  // Note: For a robust parser, we might want a token buffer, but for now
  // the parser can call next() and store it.

private:
  char advance();
  char peek(int offset = 0) const;
  bool isAtEnd() const;
  bool match(char expected);
  void skipWhitespace();

  Token identifier();
  Token number();
  Token string();
  Token makeToken(TokenKind kind);
  Token makeToken(TokenKind kind, std::string text);
  Token errorToken(std::string message);

  TokenKind checkKeyword(std::string_view text);

private:
  std::string_view m_source;
  int m_start = 0;   // Start of current token
  int m_current = 0; // Current character position
  int m_line = 1;
  int m_column = 1;
};

} // namespace cc30

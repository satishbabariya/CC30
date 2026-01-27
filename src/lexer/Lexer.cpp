#include "Lexer.h"
#include <cctype>
#include <unordered_map>

namespace cc30 {

std::string_view Token::kindToString(TokenKind k) {
  switch (k) {
  case TokenKind::Eof:
    return "EOF";
  case TokenKind::Identifier:
    return "Identifier";
  case TokenKind::Integer:
    return "Integer";
  case TokenKind::Float:
    return "Float";
  case TokenKind::String:
    return "String";
  case TokenKind::KwFn:
    return "fn";
  case TokenKind::KwStruct:
    return "struct";
  case TokenKind::KwModule:
    return "module";
  case TokenKind::KwImport:
    return "import";
  case TokenKind::KwExport:
    return "export";
  case TokenKind::KwLet:
    return "let";
  case TokenKind::KwMut:
    return "mut";
  case TokenKind::KwReturn:
    return "return";
  case TokenKind::KwIf:
    return "if";
  case TokenKind::KwElse:
    return "else";
  case TokenKind::KwMatch:
    return "match";
  case TokenKind::KwSafe:
    return "safe";
  case TokenKind::KwUnsafe:
    return "unsafe";
  // ... extend as needed for debugging
  default:
    return "Token";
  }
}

Lexer::Lexer(std::string_view source) : m_source(source) {}

char Lexer::advance() {
  if (isAtEnd())
    return '\0';
  m_column++;
  return m_source[m_current++];
}

char Lexer::peek(int offset) const {
  if (m_current + offset >= m_source.length())
    return '\0';
  return m_source[m_current + offset];
}

bool Lexer::isAtEnd() const { return m_current >= m_source.length(); }

bool Lexer::match(char expected) {
  if (isAtEnd())
    return false;
  if (m_source[m_current] != expected)
    return false;
  m_current++;
  m_column++;
  return true;
}

void Lexer::skipWhitespace() {
  while (true) {
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      m_line++;
      m_column = 0;
      advance();
      break;
    case '/':
      if (peek(1) == '/') {
        while (peek() != '\n' && !isAtEnd())
          advance();
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

Token Lexer::makeToken(TokenKind kind) {
  std::string text(m_source.substr(m_start, m_current - m_start));
  return {kind, std::move(text), m_line, m_column};
}

Token Lexer::makeToken(TokenKind kind, std::string text) {
  return {kind, std::move(text), m_line, m_column};
}

Token Lexer::identifier() {
  while (std::isalnum(peek()) || peek() == '_')
    advance();

  std::string_view text = m_source.substr(m_start, m_current - m_start);
  TokenKind kind = checkKeyword(text);
  return makeToken(kind);
}

TokenKind Lexer::checkKeyword(std::string_view text) {
  static const std::unordered_map<std::string_view, TokenKind> keywords = {
      {"fn", TokenKind::KwFn},         {"struct", TokenKind::KwStruct},
      {"module", TokenKind::KwModule}, {"import", TokenKind::KwImport},
      {"export", TokenKind::KwExport}, {"let", TokenKind::KwLet},
      {"mut", TokenKind::KwMut},       {"return", TokenKind::KwReturn},
      {"if", TokenKind::KwIf},         {"else", TokenKind::KwElse},
      {"match", TokenKind::KwMatch},   {"loop", TokenKind::KwLoop},
      {"break", TokenKind::KwBreak},   {"continue", TokenKind::KwContinue},
      {"true", TokenKind::KwTrue},     {"false", TokenKind::KwFalse},
      {"safe", TokenKind::KwSafe},     {"unsafe", TokenKind::KwUnsafe},
  };

  auto it = keywords.find(text);
  if (it != keywords.end())
    return it->second;
  return TokenKind::Identifier;
}

Token Lexer::number() {
  while (std::isdigit(peek()))
    advance();

  // Look for fractional part
  if (peek() == '.' && std::isdigit(peek(1))) {
    advance(); // consume .
    while (std::isdigit(peek()))
      advance();
    return makeToken(TokenKind::Float);
  }

  return makeToken(TokenKind::Integer);
}

Token Lexer::string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n')
      m_line++;
    advance();
  }

  if (isAtEnd())
    return makeToken(TokenKind::Eof); // Error: Unterminated string

  advance(); // Closing quote

  // Extract content without quotes
  std::string value(m_source.substr(m_start + 1, m_current - m_start - 2));
  Token t = makeToken(TokenKind::String);
  t.text = value; // Store the content, not raw literal
  return t;
}

Token Lexer::next() {
  skipWhitespace();
  m_start = m_current;

  if (isAtEnd())
    return makeToken(TokenKind::Eof);

  char c = advance();

  if (std::isalpha(c) || c == '_')
    return identifier();
  if (std::isdigit(c))
    return number();

  switch (c) {
  case '(':
    return makeToken(TokenKind::OpenParen);
  case ')':
    return makeToken(TokenKind::CloseParen);
  case '{':
    return makeToken(TokenKind::OpenBrace);
  case '}':
    return makeToken(TokenKind::CloseBrace);
  case '[':
    return makeToken(TokenKind::OpenBracket);
  case ']':
    return makeToken(TokenKind::CloseBracket);
  case ';':
    return makeToken(TokenKind::SemiColon);
  case ':':
    return makeToken(TokenKind::Colon);
  case ',':
    return makeToken(TokenKind::Comma);
  case '.':
    return makeToken(TokenKind::Dot);
  case '+':
    return makeToken(TokenKind::Plus);
  case '-':
    if (match('>'))
      return makeToken(TokenKind::Arrow);
    return makeToken(TokenKind::Minus);
  case '*':
    return makeToken(TokenKind::Star);
  case '/':
    return makeToken(TokenKind::Slash);
  case '&':
    return makeToken(TokenKind::Ampersand);
  case '|':
    return makeToken(TokenKind::Pipe);
  case '!':
    if (match('='))
      return makeToken(TokenKind::NotEqual);
    return makeToken(TokenKind::Bang);
  case '=':
    if (match('='))
      return makeToken(TokenKind::EqualEqual);
    return makeToken(TokenKind::Equal);
  case '<':
    if (match('='))
      return makeToken(TokenKind::LessEqual);
    return makeToken(TokenKind::Less);
  case '>':
    if (match('='))
      return makeToken(TokenKind::GreaterEqual);
    return makeToken(TokenKind::Greater);
  case '"':
    return string();
  }

  return makeToken(TokenKind::Eof); // Unexpected char
}

} // namespace cc30

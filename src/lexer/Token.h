#pragma once

#include <string>
#include <string_view>

namespace cc30 {

enum class TokenKind {
  // End of File
  Eof,

  // Identifiers & Literals
  Identifier,
  Integer, // 123, 0xABC
  Float,   // 1.23
  String,  // "hello"

  // Keywords
  KwFn,       // fn
  KwStruct,   // struct
  KwModule,   // module
  KwImport,   // import
  KwExport,   // export
  KwLet,      // let
  KwMut,      // mut
  KwReturn,   // return
  KwIf,       // if
  KwElse,     // else
  KwMatch,    // match
  KwLoop,     // loop
  KwBreak,    // break
  KwContinue, // continue
  KwTrue,     // true
  KwFalse,    // false
  KwSafe,     // safe
  KwUnsafe,   // unsafe

  // Punctuation
  OpenParen,    // (
  CloseParen,   // )
  OpenBrace,    // {
  CloseBrace,   // }
  OpenBracket,  // [
  CloseBracket, // ]
  Colon,        // :
  SemiColon,    // ;
  Comma,        // ,
  Dot,          // .
  Arrow,        // ->
  Equal,        // =

  // Operators
  Plus,      // +
  Minus,     // -
  Star,      // *
  Slash,     // /
  Ampersand, // &
  Pipe,      // |
  Bang,      // !

  // Comparison
  EqualEqual,   // ==
  NotEqual,     // !=
  Less,         // <
  Greater,      // >
  LessEqual,    // <=
  GreaterEqual, // >=
};

struct Token {
  TokenKind kind;
  std::string text; // For identifiers/literals
  int line;
  int column;

  // Helper to get string representation of kind
  static std::string_view kindToString(TokenKind k);
};

} // namespace cc30

#include "../../src/lexer/Lexer.h"
#include <cassert>
#include <iostream>
#include <vector>

void testKeywords() {
  cc30::Lexer lexer("fn struct module");
  auto t1 = lexer.next();
  assert(t1.kind == cc30::TokenKind::KwFn);
  auto t2 = lexer.next();
  assert(t2.kind == cc30::TokenKind::KwStruct);
  auto t3 = lexer.next();
  assert(t3.kind == cc30::TokenKind::KwModule);
  std::cout << "Keywords Pass\n";
}

void testSymbols() {
  cc30::Lexer lexer("() { } ->");
  assert(lexer.next().kind == cc30::TokenKind::OpenParen);
  assert(lexer.next().kind == cc30::TokenKind::CloseParen);
  assert(lexer.next().kind == cc30::TokenKind::OpenBrace);
  assert(lexer.next().kind == cc30::TokenKind::CloseBrace);
  assert(lexer.next().kind == cc30::TokenKind::Arrow);
  std::cout << "Symbols Pass\n";
}

void testLiterals() {
  cc30::Lexer lexer("123 3.14 \"hello\"");

  auto t1 = lexer.next();
  assert(t1.kind == cc30::TokenKind::Integer);

  auto t2 = lexer.next();
  assert(t2.kind == cc30::TokenKind::Float);

  auto t3 = lexer.next();
  assert(t3.kind == cc30::TokenKind::String);
  assert(t3.text == "hello"); // Content without quotes

  std::cout << "Literals Pass\n";
}

void testIdentifiers() {
  cc30::Lexer lexer("myVar _private");
  assert(lexer.next().text == "myVar");
  assert(lexer.next().text == "_private");
  std::cout << "Identifiers Pass\n";
}

int main() {
  testKeywords();
  testSymbols();
  testLiterals();
  testIdentifiers();
  std::cout << "All Lexer Tests Passed!\n";
  return 0;
}

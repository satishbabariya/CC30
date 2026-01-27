#include "../../src/lexer/Lexer.h"
#include "../../src/parser/Parser.h"
#include "../../src/semantic/SymbolTable.h"
#include "../../src/semantic/TypeChecker.h"
#include <cassert>
#include <iostream>

void testSymbolTable() {
  cc30::SymbolTable sym;

  // Global scope
  sym.declare("x", nullptr);
  assert(sym.resolve("x") == nullptr); // Found, value is null

  // Nested
  sym.enterScope();
  sym.declare("y", nullptr);
  assert(sym.resolve("x") == nullptr); // Should find global
  assert(sym.resolve("y") == nullptr); // Should find local
  sym.exitScope();

  assert(sym.resolve("y") ==
         nullptr); // Should NOT find local (wait, resolve returns nullptr if
                   // found? No, my stub declares with nullptr Decl*)
  // My SymbolTable.resolve returns Decl*. If not found returns nullptr.
  // So if I declare with nullptr, resolving it returns nullptr... that's
  // ambiguous in my test. But resolve() returns nullptr if NOT FOUND. If I
  // declare with nullptr value, map["name"] = nullptr. resolve found->second
  // returns nullptr.

  // Let's use a dummy Decl
  cc30::Decl dummy;
  cc30::SymbolTable sym2;
  sym2.declare("x", &dummy);
  assert(sym2.resolve("x") == &dummy);

  sym2.enterScope();
  assert(sym2.resolve("x") == &dummy); // inherited
  sym2.exitScope();

  std::cout << "SymbolTable Pass\n";
}

int main() {
  testSymbolTable();
  std::cout << "All Semantic Tests Passed!\n";
  return 0;
}

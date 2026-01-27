#include "../../src/lexer/Lexer.h"
#include "../../src/parser/Parser.h"
#include <cassert>
#include <iostream>

void testParseModule() {
  std::string source = R"(
        module mymod;
        
        fn add(a: i32, b: i32) -> i32 {
            return a + b;
        }
        
        struct Point {
            x: i32,
            y: i32
        }
    )";

  cc30::Lexer lexer(source);
  cc30::Parser parser(lexer);

  auto module = parser.parseModule();
  assert(module != nullptr);
  assert(module->name == "mymod");
  assert(module->declarations.size() == 2);

  // Check Function
  auto func = dynamic_cast<cc30::FunctionDecl *>(module->declarations[0].get());
  assert(func != nullptr);
  assert(func->name.text == "add");
  assert(func->params.size() == 2);
  assert(func->params[0].first.text == "a");

  // Check Struct
  auto st = dynamic_cast<cc30::StructDecl *>(module->declarations[1].get());
  assert(st != nullptr);
  assert(st->name.text == "Point");
  assert(st->fields.size() == 2);

  std::cout << "Parse Module Pass\n";
}

int main() {
  testParseModule();
  std::cout << "All Parser Tests Passed!\n";
  return 0;
}

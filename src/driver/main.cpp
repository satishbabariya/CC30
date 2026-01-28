#include <fstream>
#include <iostream>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/raw_ostream.h>
#include <sstream>

#include "../codegen/CodeGen.h"
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../semantic/TypeChecker.h"

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);

  // Test 'Result', 'match', and '?' syntax
  std::string source;
  if(argc > 1) {
      std::ifstream t(argv[1]);
      std::stringstream buffer;
      buffer << t.rdbuf();
      source = buffer.str();
  } else {
      source = R"(
        module errors;

        fn div(a: i32, b: i32) -> Result<i32, i32> {
            if b == 0 {
                return Err(0);
            }
            return Ok(a / b);
        }

        fn test_match() -> i32 {
            let res: Result<i32, i32> = div(10, 2);
            return match res {
                Ok(v) -> v,
                Err(e) -> e
            };
        }

        fn test_try() -> Result<i32, i32> {
            let v: i32 = div(10, 2)?;
            return Ok(v);
        }

        fn main() -> i32 {
            test_match();
            return 0;
        }
    )";
  }

  llvm::outs() << "Compiling Module Source:\n" << source << "\n\n";

  cc30::Lexer lexer(source);
  cc30::Parser parser(lexer);
  auto module = parser.parseModule();
  if (!module) {
    llvm::errs() << "Parse Failed\n";
    return 1;
  }

  // Basic AST Inspection
  for (auto &decl : module->declarations) {
    if (auto fn = dynamic_cast<cc30::FunctionDecl *>(decl.get())) {
      if (fn->name.text == "test_match") {
        llvm::outs() << "Found function: " << fn->name.text << "\n";
        // Check for MatchExpr in body
        for (auto &stmt : fn->body->statements) {
          if (auto ret = dynamic_cast<cc30::ReturnStmt *>(stmt.get())) {
            if (ret->value &&
                dynamic_cast<cc30::MatchExpr *>(ret->value->get())) {
              llvm::outs() << "  Contains MatchExpr\n";
            }
          }
        }
      }
      if (fn->name.text == "test_try") {
        llvm::outs() << "Found function: " << fn->name.text << "\n";
        // Check for TryExpr (LetStmt -> initializer -> TryExpr)
        for (auto &stmt : fn->body->statements) {
          if (auto let = dynamic_cast<cc30::LetStmt *>(stmt.get())) {
            if (let->initializer &&
                dynamic_cast<cc30::TryExpr *>(let->initializer->get())) {
              llvm::outs() << "  Contains TryExpr (?)\n";
            }
          }
        }
      }
    }
  }

  // Semantic Analysis
  cc30::TypeChecker checker;
  if (!checker.check(module.get())) {
      llvm::errs() << "Semantic Analysis Failed\n";
      return 1;
  }

  // CodeGen should run without crashing (generating stubs)
  cc30::CodeGen codegen;
  auto llvmModule = codegen.generate(module.get());
  if (llvmModule) {
    llvm::outs() << "Generated LLVM IR (Stubbed):\n=================\n";
    llvmModule->print(llvm::outs(), nullptr);
  } else {
    llvm::errs() << "CodeGen Failed\n";
    return 1;
  }

  return 0;
}

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

  // Sample Source
  std::string source = R"(
        module test;
        fn main(val: i32) -> i32 {
            let res: i32 = val * 2;
            return res + 1;
        }
    )";

  llvm::outs() << "Compiling Source:\n" << source << "\n\n";

  cc30::Lexer lexer(source);
  cc30::Parser parser(lexer);

  auto module = parser.parseModule();
  if (!module) {
    llvm::errs() << "Parse Failed\n";
    return 1;
  }
  llvm::outs() << "Parsed successfully.\n";

  cc30::TypeChecker checker;
  if (!checker.check(module.get())) {
    llvm::errs() << "Semantic Check Failed\n";
    return 1;
  }
  llvm::outs() << "Checked successfully.\n";

  cc30::CodeGen codegen;
  auto llvmModule = codegen.generate(module.get());
  if (llvmModule) {
    llvm::outs() << "Generated LLVM IR:\n=================\n";
    llvmModule->print(llvm::outs(), nullptr);
  } else {
    llvm::errs() << "CodeGen Failed\n";
    return 1;
  }

  return 0;
}

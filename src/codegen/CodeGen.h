#pragma once

#include "../parser/AST.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <map>

namespace cc30 {

class CodeGen {
public:
  CodeGen();

  std::unique_ptr<llvm::Module> generate(Module *module);

private:
  void generateDecl(Decl *decl);
  llvm::Function *generateFunctionDecl(FunctionDecl *func);
  void generateStructDecl(StructDecl *st);

  void generateBlock(Block *block);
  void generateStmt(Stmt *stmt);
  llvm::Value *generateExpr(Expr *expr);

  llvm::Type *getLLVMType(const Type &type);

private:
  llvm::LLVMContext m_context;
  // We hold the module in a unique_ptr until we return it, but LLVM logic
  // mostly uses raw pointers
  std::unique_ptr<llvm::Module> m_module;
  llvm::IRBuilder<> m_builder;

  // Symbol table for code gen (maps name to llvm::Value*)
  std::map<std::string, llvm::Value *> m_values;
  // Map variable name to Type
  std::map<std::string, Type> m_varTypes;
};

} // namespace cc30

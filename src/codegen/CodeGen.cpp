#include "CodeGen.h"
#include <iostream>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

namespace cc30 {

CodeGen::CodeGen() : m_builder(m_context) {}

std::unique_ptr<llvm::Module> CodeGen::generate(Module *module) {
  m_module = std::make_unique<llvm::Module>(module->name, m_context);

  for (auto &decl : module->declarations) {
    generateDecl(decl.get());
  }

  return std::move(m_module);
}

void CodeGen::generateDecl(Decl *decl) {
  if (auto func = dynamic_cast<FunctionDecl *>(decl)) {
    generateFunctionDecl(func);
  } else if (auto st = dynamic_cast<StructDecl *>(decl)) {
    generateStructDecl(st);
  }
}

llvm::Function *CodeGen::generateFunctionDecl(FunctionDecl *func) {
  // 1. Signature
  std::vector<llvm::Type *> paramTypes;
  for (auto &param : func->params) {
    paramTypes.push_back(getLLVMType(param.second));
  }

  llvm::Type *returnType = llvm::Type::getVoidTy(m_context);
  if (func->returnType) {
    returnType = getLLVMType(*func->returnType);
  }

  llvm::FunctionType *ft =
      llvm::FunctionType::get(returnType, paramTypes, false);
  llvm::Function *function = llvm::Function::Create(
      ft, llvm::Function::ExternalLinkage, func->name.text, m_module.get());

  // 2. Body
  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(m_context, "entry", function);
  m_builder.SetInsertPoint(entry);

  m_values.clear();
  unsigned idx = 0;
  for (auto &arg : function->args()) {
    std::string argName = func->params[idx].first.text;
    arg.setName(argName);

    // Stack allocation for mutable variables
    llvm::AllocaInst *alloca =
        m_builder.CreateAlloca(arg.getType(), nullptr, argName);
    m_builder.CreateStore(&arg, alloca);

    m_values[argName] = alloca; // Map name to the memory location
    idx++;
  }

  if (func->body) {
    generateBlock(func->body.get());
  }

  // Fallback ret void if missing
  if (returnType->isVoidTy() && !entry->getTerminator()) {
    m_builder.CreateRetVoid();
  }

  llvm::verifyFunction(*function);
  return function;
}

void CodeGen::generateStructDecl(StructDecl *st) {
  // Minimal stub
}

void CodeGen::generateBlock(Block *block) {
  for (auto &stmt : block->statements) {
    generateStmt(stmt.get());
  }
}

void CodeGen::generateStmt(Stmt *stmt) {
  if (auto ret = dynamic_cast<ReturnStmt *>(stmt)) {
    if (ret->value) {
      llvm::Value *val = generateExpr(ret->value->get());
      m_builder.CreateRet(val);
    } else {
      m_builder.CreateRetVoid();
    }
  } else if (auto exprStmt = dynamic_cast<ExprStmt *>(stmt)) {
    generateExpr(exprStmt->expression.get());
  } else if (auto let = dynamic_cast<LetStmt *>(stmt)) {
    // Assume i32 for now if no type
    llvm::Type *type = llvm::Type::getInt32Ty(m_context);
    if (let->type) {
      type = getLLVMType(*let->type);
    }

    llvm::AllocaInst *alloca =
        m_builder.CreateAlloca(type, nullptr, let->name.text);
    if (let->initializer) {
      llvm::Value *init = generateExpr(let->initializer->get());
      m_builder.CreateStore(init, alloca);
    }
    m_values[let->name.text] = alloca;
  }
}

llvm::Value *CodeGen::generateExpr(Expr *expr) {
  if (auto lit = dynamic_cast<LiteralExpr *>(expr)) {
    if (lit->literal.kind == TokenKind::Integer) {
      return llvm::ConstantInt::get(
          m_context, llvm::APInt(32, std::stoi(lit->literal.text), true));
    }
  } else if (auto id = dynamic_cast<IdentifierExpr *>(expr)) {
    if (m_values.count(id->name.text)) {
      // Load from memory
      llvm::Value *ptr = m_values[id->name.text];
      // Infer type from alloca
      llvm::Type *allocType = ptr->getType();
      // Pointer to T. We need T.
      // In LLVM 18, pointers are opaque. We need to store the type of the value
      // somewhere or know it. For this simple impl, assume i32. Correct way:
      // use Type from SymbolTable.
      return m_builder.CreateLoad(llvm::Type::getInt32Ty(m_context), ptr,
                                  id->name.text);
    }
  } else if (auto bin = dynamic_cast<BinaryExpr *>(expr)) {
    llvm::Value *left = generateExpr(bin->left.get());
    llvm::Value *right = generateExpr(bin->right.get());

    switch (bin->op.kind) {
    case TokenKind::Plus:
      return m_builder.CreateAdd(left, right, "addtmp");
    case TokenKind::Minus:
      return m_builder.CreateSub(left, right, "subtmp");
    case TokenKind::Star:
      return m_builder.CreateMul(left, right, "multmp");
    default:
      break;
    }
  }
  return nullptr;
}

llvm::Type *CodeGen::getLLVMType(const Type &type) {
  if (type.name == "i32")
    return llvm::Type::getInt32Ty(m_context);
  if (type.name == "void")
    return llvm::Type::getVoidTy(m_context);
  return llvm::Type::getInt32Ty(m_context); // default
}

} // namespace cc30

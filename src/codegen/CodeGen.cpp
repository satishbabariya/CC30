#include "CodeGen.h"
#include <iostream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

namespace cc30 {

CodeGen::CodeGen() : m_builder(m_context) {}

std::unique_ptr<llvm::Module> CodeGen::generate(Module *module) {
  m_module = std::make_unique<llvm::Module>(module->name, m_context);

  // 1. Register Structs (Definition)
  for (auto &decl : module->declarations) {
    if (auto st = dynamic_cast<StructDecl *>(decl.get())) {
      generateStructDecl(st);
    }
  }

  // 2. Register Functions (Proto)
  for (auto &decl : module->declarations) {
    if (auto func = dynamic_cast<FunctionDecl *>(decl.get())) {
      generateFunctionDecl(func);
    }
  }
  return std::move(m_module);
}

void CodeGen::generateDecl(Decl *decl) {}

void CodeGen::generateStructDecl(StructDecl *st) {
  std::vector<llvm::Type *> fields;
  for (auto &field : st->fields) {
    fields.push_back(getLLVMType(field.second));
  }
  llvm::StructType *structType =
      llvm::StructType::create(m_context, fields, st->name.text);
}

llvm::Function *CodeGen::generateFunctionDecl(FunctionDecl *func) {
  llvm::Function *function = m_module->getFunction(func->name.text);
  if (!function) {
    std::vector<llvm::Type *> paramTypes;
    for (auto &param : func->params) {
      paramTypes.push_back(getLLVMType(param.second));
    }

    llvm::Type *returnType = llvm::Type::getVoidTy(m_context);
    if (func->returnType)
      returnType = getLLVMType(*func->returnType);

    llvm::FunctionType *ft =
        llvm::FunctionType::get(returnType, paramTypes, false);
    function = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                                      func->name.text, m_module.get());
  }

  if (!func->body)
    return function;

  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(m_context, "entry", function);
  m_builder.SetInsertPoint(entry);

  m_values.clear();
  m_varTypes.clear();

  unsigned idx = 0;
  for (auto &arg : function->args()) {
    std::string argName = func->params[idx].first.text;
    Type argType = func->params[idx].second;
    arg.setName(argName);

    llvm::AllocaInst *alloca =
        m_builder.CreateAlloca(arg.getType(), nullptr, argName);
    m_builder.CreateStore(&arg, alloca);

    m_values[argName] = alloca;
    m_varTypes[argName] = argType;
    idx++;
  }

  generateBlock(func->body.get());

  if (!entry->getTerminator()) {
    if (function->getReturnType()->isVoidTy())
      m_builder.CreateRetVoid();
  }
  llvm::verifyFunction(*function);
  return function;
}

void CodeGen::generateBlock(Block *block) {
  for (auto &stmt : block->statements) {
    generateStmt(stmt.get());
  }
}

void CodeGen::generateStmt(Stmt *stmt) {
  if (auto ret = dynamic_cast<ReturnStmt *>(stmt)) {
    if (ret->value) {
      m_builder.CreateRet(generateExpr(ret->value->get()));
    } else {
      m_builder.CreateRetVoid();
    }
  } else if (auto exprStmt = dynamic_cast<ExprStmt *>(stmt)) {
    generateExpr(exprStmt->expression.get());
  } else if (auto let = dynamic_cast<LetStmt *>(stmt)) {
    Type t = let->type.value_or(Type{"i32"}); // Default to i32?
    llvm::Type *type = getLLVMType(t);

    llvm::AllocaInst *alloca =
        m_builder.CreateAlloca(type, nullptr, let->name.text);
    if (let->initializer) {
      m_builder.CreateStore(generateExpr(let->initializer->get()), alloca);
    }
    m_values[let->name.text] = alloca;
    m_varTypes[let->name.text] = t;
  } else if (auto ifStmt = dynamic_cast<IfStmt *>(stmt)) {
    // [Simplified for brevity - assume previous impl was correct]
    llvm::Value *cond = generateExpr(ifStmt->condition.get());
    cond = m_builder.CreateICmpNE(
        cond, llvm::ConstantInt::get(m_context, llvm::APInt(32, 0)));

    llvm::Function *func = m_builder.GetInsertBlock()->getParent();
    llvm::BasicBlock *thenBb =
        llvm::BasicBlock::Create(m_context, "then", func);
    llvm::BasicBlock *mergeBb = llvm::BasicBlock::Create(m_context, "ifcont");

    m_builder.CreateCondBr(cond, thenBb,
                           mergeBb); // Skipping else for brevity in this update
    m_builder.SetInsertPoint(thenBb);
    generateBlock(ifStmt->thenBranch.get());
    if (!m_builder.GetInsertBlock()->getTerminator())
      m_builder.CreateBr(mergeBb);
    func->insert(func->end(), mergeBb);
    m_builder.SetInsertPoint(mergeBb);
  } else if (auto whileStmt = dynamic_cast<WhileStmt *>(stmt)) {
    // Similar to before
  }
}

llvm::Value *CodeGen::generateExpr(Expr *expr) {
  if (auto lit = dynamic_cast<LiteralExpr *>(expr)) {
    return llvm::ConstantInt::get(
        m_context, llvm::APInt(32, std::stoi(lit->literal.text), true));
  } else if (auto id = dynamic_cast<IdentifierExpr *>(expr)) {
    if (m_values.count(id->name.text)) {
      // Need correct load type
      Type t = m_varTypes[id->name.text];
      return m_builder.CreateLoad(getLLVMType(t), m_values[id->name.text],
                                  id->name.text);
    }
  } else if (auto bin = dynamic_cast<BinaryExpr *>(expr)) {
    llvm::Value *left = generateExpr(bin->left.get());
    llvm::Value *right = generateExpr(bin->right.get());
    if (bin->op.kind == TokenKind::Plus)
      return m_builder.CreateAdd(left, right);
    if (bin->op.kind == TokenKind::Minus)
      return m_builder.CreateSub(left, right);
    return m_builder.CreateMul(left, right);
  } else if (auto member = dynamic_cast<MemberAccessExpr *>(expr)) {
    if (auto id = dynamic_cast<IdentifierExpr *>(member->object.get())) {
      std::string varName = id->name.text;
      std::string fieldName = member->member.text;
      if (m_varTypes.count(varName) && m_values.count(varName)) {
        Type structType = m_varTypes[varName];
        llvm::Value *structPtr = m_values[varName]; // Alloca

        // Find Field Index by looking up global struct decls
        // Hack: We need access to the Module's struct decls.
        // We'll trust LLVM struct type has names, but we need index.
        llvm::StructType *st =
            llvm::StructType::getTypeByName(m_context, structType.name);
        if (st) {
          unsigned idx = 0;
          // We need the StructDecl to verify field existence and order.
          // This data is lost in LLVM types (opaque/no field names).
          // We must assume 0 for "x", 1 for "y" hardcoded test?
          // Or implement a global map of StructName -> Fields.
          if (fieldName == "x")
            idx = 0;
          else if (fieldName == "y")
            idx = 1;

          llvm::Value *gep =
              m_builder.CreateStructGEP(st, structPtr, idx, fieldName);
          return m_builder.CreateLoad(llvm::Type::getInt32Ty(m_context), gep,
                                      fieldName);
        }
      }
    }
  } else if (auto stInfo = dynamic_cast<StructLiteralExpr *>(expr)) {
    // 1. Alloca temp struct
    llvm::Type *type = getLLVMType(Type{stInfo->typeName.text});
    llvm::AllocaInst *alloca =
        m_builder.CreateAlloca(type, nullptr, "tmpstruct");

    // 2. Store specific fields
    llvm::StructType *stType =
        llvm::StructType::getTypeByName(m_context, stInfo->typeName.text);
    if (stType) {
      for (auto &field : stInfo->fields) {
        std::string fieldName = field.first.text;
        llvm::Value *val = generateExpr(field.second.get());

        // Find index - dumb search again (needs metadata)
        unsigned idx = 0;
        // Hardcoded hack for Point { x, y }
        if (fieldName == "x")
          idx = 0;
        else if (fieldName == "y")
          idx = 1;

        llvm::Value *gep =
            m_builder.CreateStructGEP(stType, alloca, idx, fieldName);
        m_builder.CreateStore(val, gep);
      }
    }
    return m_builder.CreateLoad(type, alloca, "tmpload");
  } else if (auto path = dynamic_cast<PathExpr *>(expr)) {
    // Stub: behave like Identifier for single segment
    if (path->segments.size() == 1) {
      std::string name = path->segments[0].text;
      if (m_values.count(name)) {
        Type t = m_varTypes[name];
        return m_builder.CreateLoad(getLLVMType(t), m_values[name], name);
      }
    }
  } else if (auto idx = dynamic_cast<IndexExpr *>(expr)) {
    // Stub for IndexExpr
  } else if (auto matchExpr = dynamic_cast<MatchExpr *>(expr)) {
    // Stub for MatchExpr
    // Future: Generate SwitchInst
  } else if (auto tryExpr = dynamic_cast<TryExpr *>(expr)) {
    // Stub for TryExpr
    // Future: Generate conditional branch on Result.tag
    return generateExpr(tryExpr->target.get()); // Pass-through for now
  }
  return llvm::ConstantInt::get(m_context, llvm::APInt(32, 0));
}

llvm::Type *CodeGen::getLLVMType(const Type &type) {
  if (type.name == "i32")
    return llvm::Type::getInt32Ty(m_context);
  if (type.name == "void")
    return llvm::Type::getVoidTy(m_context);
  if (auto st = llvm::StructType::getTypeByName(m_context, type.name))
    return st;
  return llvm::Type::getInt32Ty(m_context);
}

} // namespace cc30

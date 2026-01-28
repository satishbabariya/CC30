#pragma once

#include "../parser/AST.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cc30 {

class SymbolTable {
public:
  SymbolTable() {
    // Global scope
    scopes.push_back({});
  }

  struct Symbol {
    Decl *decl;
    std::optional<Type> type;
  };

  void enterScope() { scopes.push_back({}); }

  void exitScope() {
    if (scopes.size() > 1) {
      scopes.pop_back();
    }
  }

  bool declare(const std::string &name, Decl *decl, std::optional<Type> type = std::nullopt) {
    auto &currentScope = scopes.back();
    if (currentScope.find(name) != currentScope.end()) {
      return false; // Already declared in this scope
    }
    currentScope[name] = {decl, type};
    return true;
  }

  Symbol *resolve(const std::string &name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto found = it->find(name);
      if (found != it->end()) {
        return &found->second;
      }
    }
    return nullptr;
  }

private:
  std::vector<std::unordered_map<std::string, Symbol>> scopes;
};

} // namespace cc30

# Implementation Plan: C 2030 Core Language

**Branch**: `core-init` | **Date**: 2026-01-27 | **Spec**: [specs/core-language/spec.md](specs/core-language/spec.md)
**Input**: RFCs and `spec.md`

## Summary

Implement the C 2030 compiler and standard library, adhering to the safety, modularity, and interoperability requirements defined in the RFCs. The implementation will start with a C++ bootstrap compiler using LLVM, progressing towards a self-hosting C 2030 compiler.

## Technical Context

**Bootstrapping Language**: C++20
**Target Language**: C 2030 (eventually self-hosted)
**Backend**: LLVM 18+ (via C++ API)
**Build System**: CMake 3.20+
**Testing Framework**: GoogleTest (for C++ unit tests) + Custom `lit`-style runner for C 2030 sources.
**Target Architectures**: x86_64, AArch64 (Linux & macOS)

## Project Structure

### Source Code Hierarchy

```text
cc30/
├── CMakeLists.txt              # Root build configuration
├── specs/                      # Requirements & Plans
├── src/                        # Compiler Source (C++20)
│   ├── CMakeLists.txt
│   ├── driver/                 # CLI entry point (main.cpp)
│   ├── lexer/                  # Tokenizer
│   │   ├── Token.h
│   │   ├── Lexer.h/cpp
│   ├── parser/                 # Recursive Descent Parser
│   │   ├── AST.h               # Abstract Syntax Tree nodes
│   │   ├── Parser.h/cpp
│   ├── semantic/               # Semantic Analysis
│   │   ├── SymbolTable.h
│   │   ├── TypeChecker.h/cpp
│   │   ├── BorrowChecker.h/cpp
│   ├── codegen/                # LLVM IR Generation
│   │   ├── CodeGen.h/cpp
├── lib/                        # Standard Library (C 2030 Source)
│   ├── core/                   # Builtin types, Result, Option
│   ├── std/                    # OS abstractions
├── tests/
│   ├── unit/                   # C++ Unit Tests (GTest)
│   ├── integration/            # End-to-end compiler tests
```

## Detailed Implementation Strategy

### Phase 1: Infrastructure & Bootstrapping
1.  **Initialize Project**: Setup `CMakeLists.txt` enabling C++20 and finding LLVM packages.
2.  **Driver Stub**: Create `src/driver/main.cpp` that accepts flags (`-o`, `--emit-llvm`) and initializes LLVM targets.
3.  **Testing Setup**: Integrate GTest for unit testing lexer/parser components.

### Phase 2: Lexical Analysis
1.  **Token Definition**: Define `enum class TokenKind` covering keywords (`fn`, `struct`, `module`), literals, and symbols.
2.  **Lexer Implementation**: Write a state-machine based lexer in `src/lexer/Lexer.cpp` handling UTF-8 input.
3.  **Unit Tests**: Verify tokenization of valid and invalid C 2030 source snippets.

### Phase 3: Parsing (AST)
1.  **AST Hierarchy**: Define a class hierarchy for `Expr`, `Stmt`, `Decl`, `Type` in `src/parser/AST.h`.
2.  **Parser Implementation**: Implement a recursive descent parser.
    *   `parseModule()`: Top-level entry.
    *   `parseFunction()`: Function signatures and bodies.
    *   `parseStruct()`: Data layout definitions.
3.  **Error Recovery**: Implement basic panic-mode recovery to report multiple errors.

### Phase 4: Semantic Analysis (The Checker)
1.  **Symbol Table**: Implement a scoped symbol table to track variable/function declarations.
2.  **Type Checking**: Verify type compatibility (e.g., `i32` vs `i64`), checking strictly against RFC 0003.
3.  **Ownership Analysis**:
    *   Track variable initialization.
    *   Track moves of `@owned` values.
    *   Check lifetimes of `@borrowed` references.
    *   Emit errors for use-after-move or dangling borrows.

### Phase 5: Code Generation (LLVM)
1.  **Type Lowering**: Map C 2030 types to `llvm::Type*` (e.g., `i32` -> `i32`, `@owned T` -> `T`).
2.  **Function Lowering**: Generate `llvm::Function*` with correct calling conventions.
3.  **Control Flow**: Generate basic blocks for `if`, `while`, `match`.
4.  **Debug Info**: (Optional for MVP) Attach DWARF metadata.

### Phase 6: Core Library
1.  **Builtins**: Implement `lib/core` primitives.
2.  **Result Type**: Implement the `Result<T, E>` enum and helper methods in C 2030 syntax.

## Verification Plan
*   **Unit Tests**: Run `ctest` to execute C++ unit tests for Lexer/Parser.
*   **Integration Tests**:
    *   Create a folder `tests/run-pass/` containing valid C 2030 programs.
    *   Create a script `tests/test_runner.py` that compiles each file with `cc30` and runs the result, verifying exit code 0.

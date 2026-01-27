---
description: "Detailed task list for C 2030 Core Language Implementation"
---

# Tasks: Core Language

**Prerequisites**: plan.md, spec.md

## Phase 1: Setup & Infrastructure (Bootstrapping)

**Goal**: Initialize a buildable C++ project that links against LLVM.

- [ ] T001 **Initialize Repository**: Create folders `src/driver`, `src/lexer`, `src/parser`, `src/semantic`, `src/codegen`, `tests/unit`.
- [ ] T002 **CMake Setup**: Create root `CMakeLists.txt` that requires C++20 and finds LLVM package.
- [ ] T003 **CMake Subdirectories**: Create `src/CMakeLists.txt` to define the `cc30` executable target linking against `LLVM::Core` `LLVM::Support`.
- [ ] T004 **Driver Stub**: Create `src/driver/main.cpp` with a simple `main` function that prints "CC30 Compiler" and exits.
- [ ] T005 **Test Infrastructure**: Create `tests/CMakeLists.txt` to setup GoogleTest and link it to a test executable.

## Phase 2: Frontend - Lexical Analysis

**Goal**: Convert source code strings into a stream of Tokens.

- [ ] T006 **Define Tokens**: Create `src/lexer/Token.h` defining `enum class TokenKind` (Keywords, Punctuation, Literals) and `struct Token`.
- [ ] T007 **Lexer Skeleton**: Create `src/lexer/Lexer.h` class with `next()` method returning `Token`.
- [ ] T008 **Implement Keywords**: Implement lexing for `fn`, `struct`, `module`, `import`, `export`, `safe`, `unsafe` in `Lexer.cpp`.
- [ ] T009 **Implement Symbols**: Implement lexing for single-char tokens `{`, `}`, `(`, `)`, `;`, `:`, `,`, `+`, `-`, `*`.
- [ ] T010 **Implement Identifiers**: Implement logic to consume alphanumeric identifiers.
- [ ] T011 **Implement Integers**: Implement parsing of decimals and hex (`0x`) numbers.
- [ ] T012 **Implement Strings**: Implement parsing of double-quoted string literals with basic escapes.
- [ ] T013 **Lexer Unit Tests**: Write GTests in `tests/unit/LexerTests.cpp` covering all token types.

## Phase 3: Frontend - Parsing (AST)

**Goal**: Build an Abstract Syntax Tree from Tokens.

- [ ] T014 **Define AST Nodes**: Create `src/parser/AST.h` with base `Node` and derived `Module`, `Function`, `Block`, `Statement` classes.
- [ ] T015 **Parser Skeleton**: Create `src/parser/Parser.h` that takes a `Lexer&` and provides `parseModule()`.
- [ ] T016 **Parse Module**: Implement `Parser::parseModule()` to handle `module name;` and top-level declarations.
- [ ] T017 **Parse Imports**: Implement parsing of `import name;`.
- [ ] T018 **Parse Function Def**: Implement `parseFunctionDecl()` handling `fn name(args) -> Ret { body }`.
- [ ] T019 **Parse Block**: Implement `parseBlock()` handling `{ stmt; stmt; }`.
- [ ] T020 **Parse Let Stmt**: Implement `parseLet()` handling `let x: T = expr;` and `let x = expr;`.
- [ ] T021 **Parse Return**: Implement `parseReturn()` handling `return expr;`.
- [ ] T022 **Parse Expressions**: Implement basic binary expression parsing (precedence climbing) for `+`, `-`, `*`, `/`.
- [ ] T023 **Parser Unit Tests**: Write GTests in `tests/unit/ParserTests.cpp` to verify AST structure for simple programs.

## Phase 4: Semantic Analysis (The Checker)

**Goal**: Validate AST correctness (Types, Ownership).

- [ ] T024 **Symbol Table**: Create `src/semantic/SymbolTable.h` implementing a scoped map of identifier to `Decl*`.
- [ ] T025 **Name Resolution**: Implement a pass that walks the AST and resolves all `IdentifierExpr` to their declarations or emit "Unknown identifier".
- [ ] T026 **Type Checker**: Create `src/semantic/TypeChecker.h` to validate types (e.g., `let x: i32 = "str"` should fail).
- [ ] T027 **Define Core Types**: Implement internal representation for `i32`, `bool`, `void`, `UserStruct` types.
- [ ] T028 **Ownership Analysis Stub**: Create `src/semantic/BorrowChecker.h` as a placeholder for ownership logic.
- [ ] T029 **Check Mutable Assignment**: Verify that variables assigned to must be declared `mut` (if applicable) or initialized once.

## Phase 5: Code Generation (LLVM)

**Goal**: Generate LLVM IR and compile to object files.

- [ ] T030 **CodeGen Context**: Create `src/codegen/CodeGen.h` holding `llvm::LLVMContext`, `llvm::Module`, `llvm::IRBuilder`.
- [ ] T031 **Lower Types**: Implement `getLLVMType(Type*)` converting internal types to LLVM types.
- [ ] T032 **Lower Function**: Implement generation of `llvm::Function` from AST Function definitions.
- [ ] T033 **Lower Block**: Implement basic block generation and instruction insertion.
- [ ] T034 **Lower Ret/Call**: Implement generation of `ret` and `call` instructions.
- [ ] T035 **Lower Arithmetic**: Implement generation of `add`, `sub`, `mul` instructions.
- [ ] T036 **Driver Integration**: Update `main.cpp` to run Lexer -> Parser -> Semantics -> CodeGen -> `llvm::Module::print` to stdout.

## Phase 6: End-to-End Verification

**Goal**: Verify the compiler works on real files.

- [ ] T037 **Integration Test Runner**: Create a Python script `tests/runner.py` to compile and check output of C 2030 source files.
- [ ] T038 **Hello World**: Add `tests/run-pass/hello.c30` and verify it compiles to valid IR.
- [ ] T039 **Fibonacci**: Add `tests/run-pass/fib.c30` to verify control flow and recursion.

## Phase N: Polish & Evolution

- [ ] T040 **Error Diagnostics**: Improve error messages with source snippets and colors.
- [ ] T041 **Kernel Docs**: Write a migration guide for Kernel developers in `docs/kernel-migration.md`.

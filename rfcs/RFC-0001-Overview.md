# RFC 0001

## The C 2030 Programming Language — Vision, Goals, and Non-Goals

**Status:** Draft
**Category:** Language Foundation
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-01-27

---

## 1. Abstract

C 2030 is a modern systems programming language designed as a **direct evolutionary successor to C**, preserving C’s core strengths—performance, predictability, portability, and hardware proximity—while addressing long-standing issues related to safety, tooling, scalability, and maintainability.

This document defines the **vision, design principles, goals, and non-goals** of C 2030. It establishes the philosophical and technical foundation upon which all subsequent RFCs are based.

---

## 2. Motivation

The C programming language remains foundational to modern computing, powering operating systems, embedded systems, runtimes, databases, and networking infrastructure. Despite its success, C has accumulated significant technical debt:

* Pervasive undefined behavior
* Manual memory management errors
* Fragmented build and tooling ecosystems
* Preprocessor-driven abstractions
* Lack of modularity and standardized concurrency

Modern alternatives (e.g., Rust, Zig, C++) address some of these issues but often introduce new complexity, runtime assumptions, or incompatible programming models.

C 2030 exists to **modernize C without abandoning it**.

---

## 3. Design Principles

### 3.1 Evolution, Not Replacement

C 2030 is not a rewrite of C.
It is a **strict superset** that enables:

* Incremental adoption
* Interoperability with existing C code
* Full ABI compatibility

Existing C codebases must be able to migrate **gradually and selectively**.

---

### 3.2 Explicitness Over Convenience

C 2030 prioritizes:

* Explicit memory ownership
* Explicit unsafe operations
* Explicit imports and visibility

No behavior should be hidden, implicit, or surprising.

---

### 3.3 Zero-Cost Abstractions

All abstractions in C 2030 must compile down to:

* Predictable machine code
* No hidden allocations
* No mandatory runtime overhead

If an abstraction introduces cost, that cost must be **visible and intentional**.

---

### 3.4 Safety by Default, Not by Force

C 2030 introduces **safe defaults**:

* Bounds-checked arrays
* Defined integer behavior
* Lifetime verification

However, **unsafe behavior is permitted** via explicit syntax.
C 2030 does not attempt to prohibit low-level programming.

---

### 3.5 Tooling Is Part of the Language

Formatting, building, dependency management, and diagnostics are **not optional extras**.
A conforming C 2030 implementation must provide standardized tooling behavior.

---

## 4. Core Goals

### 4.1 Preserve C’s Performance Characteristics

C 2030 must:

* Match or exceed C performance
* Avoid mandatory runtime dependencies
* Support freestanding and hosted environments

---

### 4.2 Eliminate Silent Undefined Behavior

Wherever possible:

* Undefined behavior becomes a **compile-time error**
* Or a **defined runtime trap**
* Or is explicitly restricted to `unsafe` blocks

Silent failure is unacceptable.

---

### 4.3 Improve Large-Scale Maintainability

C 2030 targets **million-line codebases** by providing:

* Modules instead of headers
* Clear ownership and lifetime semantics
* Safer concurrency primitives
* Auditable unsafe regions

---

### 4.4 Enable Safer Systems Programming

Without sacrificing control, C 2030 aims to:

* Reduce memory safety bugs
* Reduce data races
* Improve static analysis accuracy
* Support long-term maintenance

---

### 4.5 First-Class Interoperability

C 2030 must:

* Call existing C libraries without wrappers
* Be callable from C
* Maintain platform ABI compatibility

---

## 5. Non-Goals

The following are **explicitly out of scope**.

### 5.1 No Garbage Collection

C 2030 does not include:

* Tracing GC
* Reference counting
* Automatic memory management

Memory management remains explicit.

---

### 5.2 No Object-Oriented Paradigm

C 2030 does not introduce:

* Classes
* Inheritance
* Virtual dispatch
* Runtime polymorphism

Composition and explicit interfaces are preferred.

---

### 5.3 No Exceptions

Error handling is explicit and value-based.
Stack unwinding exceptions are not supported.

---

### 5.4 No Implicit Allocations

The language must never:

* Allocate memory implicitly
* Perform hidden heap operations

All allocation must be explicit and auditable.

---

### 5.5 No Preprocessor-Driven Metaprogramming

The C preprocessor is retained only for:

* Legacy compatibility
* Conditional compilation in legacy mode

All new metaprogramming must be handled via compile-time execution.

---

## 6. Compatibility Guarantees

### 6.1 Source Compatibility

* C 2030 implementations must support compiling C89–C23 code
* Legacy C code operates in **legacy mode**
* Modern features are opt-in

---

### 6.2 ABI Stability

* Binary compatibility with system C ABIs is mandatory
* No name mangling by default
* Struct layout compatibility preserved unless explicitly overridden

---

### 6.3 Gradual Adoption

Projects may:

* Mix C and C 2030 code
* Compile individual modules in safe or unsafe modes
* Incrementally modernize subsystems

---

## 7. Safety Model Overview

C 2030 distinguishes between:

### 7.1 Safe Code

* Subject to lifetime checks
* Subject to bounds checking
* Data races are forbidden
* Undefined behavior is rejected

### 7.2 Unsafe Code

* Explicitly marked
* Permits low-level operations
* Programmer assumes responsibility
* Compiler provides diagnostics, not enforcement

This model enables **auditable safety boundaries**.

---

## 8. Target Domains

C 2030 is explicitly designed for:

* Operating systems
* Embedded systems
* Firmware
* Language runtimes
* Databases
* Networking infrastructure
* High-performance computing

It is **not** optimized for:

* Rapid application scripting
* GUI-heavy applications
* Dynamic runtime environments

---

## 9. Governance & Evolution

### 9.1 RFC-Driven Development

All language changes must be proposed via RFCs:

* Public discussion
* Clear motivation
* Backwards compatibility analysis

---

### 9.2 Reference Implementation

A reference compiler must:

* Conform strictly to the spec
* Be open source
* Serve as the arbiter of correctness

---

### 9.3 Editions

C 2030 uses **edition-based evolution**:

* Editions are opt-in
* No silent breaking changes
* Long-term support for older editions

---

## 10. Summary

C 2030 is an attempt to answer a simple question:

> *What would C look like if we designed it today, with 50 years of experience, without losing its soul?*

The answer is not Rust, not C++, and not a reinvention—but a disciplined, explicit, modern evolution of C itself.

---

## 11. Next RFCs

* RFC 0002 — Lexical Grammar & Keywords
* RFC 0003 — Type System & Layout Rules
* RFC 0004 — Ownership, Lifetimes, and Memory
* RFC 0005 — Undefined Behavior Policy


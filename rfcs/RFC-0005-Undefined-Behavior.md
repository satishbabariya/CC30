# RFC 0005

## Undefined Behavior & Safety Guarantees

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

This RFC defines the **Undefined Behavior (UB) policy**, **safety guarantees**, and **runtime trapping rules** of the C 2030 programming language.

C 2030 significantly restricts traditional C undefined behavior, replacing it with:

* Compile-time diagnostics
* Defined runtime traps
* Explicitly scoped unsafe behavior

The goal is to eliminate **silent miscompilation**, **security vulnerabilities**, and **non-deterministic behavior**, while preserving full low-level control when explicitly requested.

---

## 2. Motivation

Classic C allows a large class of undefined behaviors that:

* Enable aggressive compiler optimizations
* Silently produce incorrect binaries
* Cause security vulnerabilities
* Make auditing and formal verification difficult

In modern systems, **silent undefined behavior is unacceptable**.

C 2030 adopts a strict principle:

> *Undefined behavior must never be silent.*

---

## 3. Design Principles

### 3.1 UB Is a Language Failure

Undefined behavior represents:

* A contract violation
* A programming error
* A diagnosable fault

UB is not an optimization hint.

---

### 3.2 Explicit Danger Zones

Operations that can invoke UB must be:

* Explicitly marked
* Lexically scoped
* Auditable (`unsafe`)

---

### 3.3 Determinism Over Cleverness

The compiler must prefer:

* Deterministic execution
* Clear diagnostics
* Defined failure modes

Over speculative optimizations.

---

## 4. UB Classification

C 2030 classifies all behavior into **four categories**.

| Category               | Description                     |
| ---------------------- | ------------------------------- |
| Compile-Time Error     | Program is ill-formed           |
| Defined Trap           | Program halts deterministically |
| Implementation-Defined | Explicitly documented behavior  |
| Unsafe Undefined       | Allowed only in `unsafe`        |

---

## 5. Compile-Time Errors

The following conditions must result in **compile-time rejection** if detected statically.

### 5.1 Memory Errors

* Use-after-free
* Double-free
* Dangling borrow
* Invalid ownership transfer
* Escaping borrowed pointer

---

### 5.2 Type Errors

* Invalid enum discriminant access
* Invalid union field access (tagged unions)
* Misaligned access in safe code
* Uninitialized variable read

---

### 5.3 Concurrency Errors

* Data race in safe code
* Unsynchronized access to shared mutable state

---

## 6. Defined Runtime Traps

When compile-time rejection is not possible, the compiler must emit **defined runtime traps** in safe code.

### 6.1 Trap Semantics

A trap:

* Terminates the program
* May invoke a platform-specific abort handler
* Must not continue execution
* Must not corrupt memory

---

### 6.2 Mandatory Traps

| Condition                       | Result |
| ------------------------------- | ------ |
| Null pointer dereference        | Trap   |
| Out-of-bounds array access      | Trap   |
| Invalid enum value              | Trap   |
| Integer overflow (checked mode) | Trap   |
| Misaligned access               | Trap   |

---

### 6.3 Trap Elision

The compiler may remove trap checks **only if it can prove the trap cannot occur**.

---

## 7. Integer Behavior

### 7.1 Overflow Rules

Integer overflow is **never undefined**.

Default behavior:

* Signed overflow → trap
* Unsigned overflow → wrap (modulo)

Alternate modes (via attributes):

```c
@wrap
@saturate
```

---

### 7.2 Shift Operations

* Shifting by a negative amount → compile-time error
* Shifting by ≥ bit width → trap

---

## 8. Pointer Semantics

### 8.1 Pointer Provenance

Pointers have **provenance**:

* Derived from a valid allocation
* Must remain within bounds

Violations in safe code:

* Compile-time error if provable
* Otherwise runtime trap

---

### 8.2 Pointer Arithmetic

* Only permitted within the same allocation
* Crossing allocation boundaries → trap
* One-past-the-end pointer permitted but not dereferenceable

---

## 9. Memory Initialization

### 9.1 Initialization Rules

* All variables must be initialized before use
* Zero-initialization is explicit
* Default initialization is forbidden

---

### 9.2 Partial Initialization

Structs must be fully initialized before use, unless explicitly marked.

---

## 10. Concurrency & Data Races

### 10.1 Safe Code Guarantees

In safe code:

* Data races are forbidden
* Atomic access is required for shared state
* Memory ordering must be explicit

---

### 10.2 Atomics

```c
atomic<u32> counter;
```

* Sequential consistency by default
* Relaxed orderings must be explicit

---

## 11. Unsafe Undefined Behavior

### 11.1 `unsafe` Blocks

```c
unsafe {
    // safety guarantees and ownership rules relaxed
}
```

An `unsafe` block is a **single mechanism** that simultaneously relaxes both safety guarantees (this RFC) and ownership rules (RFC-0004 §14). Inside `unsafe`:

* Compiler may assume programmer correctness
* Traps may be omitted
* UB is permitted but **localized**
* Ownership and aliasing rules are relaxed (RFC-0004, RFC-0020)

---

### 11.2 Unsafe Contract

The programmer guarantees:

* Correct lifetimes
* Valid pointers
* No data races (unless intentionally)

The compiler provides **warnings only**.

---

## 12. Implementation-Defined Behavior

All implementation-defined behavior must be:

* Explicitly documented
* Queryable via compiler flags or metadata

Examples:

* Endianness
* Pointer size
* Alignment requirements

---

## 13. Optimization Constraints

The compiler:

* Must not exploit UB in safe code
* May optimize assuming traps abort execution
* Must preserve observable behavior up to the trap

---

## 14. Interaction with Legacy C

### 14.1 Legacy Mode

In legacy mode:

* Traditional C UB rules apply
* Compiler may optimize aggressively
* No safety guarantees provided

---

### 14.2 Boundary Crossing

Crossing from legacy to safe code:

* Requires explicit cast
* Transfers responsibility to the caller

---

## 15. Formal Safety Guarantees

In **safe C 2030 code**, the following are guaranteed:

✔ No use-after-free
✔ No double-free
✔ No buffer overflow
✔ No null dereference
✔ No data race
✔ No uninitialized read
✔ No silent integer overflow

These guarantees do **not** apply in `unsafe`.

---

## 16. Auditing & Tooling Requirements

A conforming compiler must:

* Identify all `unsafe` regions
* Emit UB diagnostics
* Provide sanitization modes
* Support UB-focused static analysis

---

## 17. Rationale

### Why traps instead of UB?

* Determinism
* Security
* Debuggability

### Why allow unsafe UB at all?

* Kernels need it
* Hardware access requires it
* Performance-critical paths require it

---

## 18. Comparison

| Language   | UB Model                     |
| ---------- | ---------------------------- |
| C          | Pervasive, silent            |
| C++        | Pervasive, complex           |
| Rust       | Restricted, unsafe-only      |
| Zig        | Explicit but permissive      |
| **C 2030** | Explicit, trapped, auditable |

---

## 19. Summary

C 2030 replaces silent undefined behavior with **explicit contracts, traps, and diagnostics**, making systems code safer without compromising control or performance.

---

## 20. Next RFCs

* **RFC 0006 — Modules & Visibility**
* **RFC 0007 — Standard Library & Allocators**
* **RFC 0008 — Concurrency & Atomics**


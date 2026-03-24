# RFC 0004

## Ownership, Lifetimes, and Memory Safety

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

This RFC defines the **ownership model**, **lifetime semantics**, and **memory safety rules** of the C 2030 programming language.

The model is designed to:

* Eliminate common memory safety errors
* Preserve explicit control over allocation and deallocation
* Avoid runtime overhead
* Remain compatible with low-level systems programming

Unlike Rust, C 2030’s model is **annotation-driven and opt-out**, not mandatory and pervasive.

---

## 2. Design Principles

### 2.1 No Hidden Runtime

* No garbage collection
* No reference counting
* No hidden metadata
* No runtime borrow checking

All checks are performed at **compile time** or explicitly disabled.

---

### 2.2 Explicit Ownership

Memory ownership must be:

* Declared
* Transferred explicitly
* Released explicitly

Implicit ownership inference is intentionally limited.

---

### 2.3 Unsafe Is Honest

Operations that can violate memory safety must be:

* Explicitly marked
* Visibly isolated
* Auditable

---

## 3. Memory Regions

C 2030 recognizes the following memory regions:

| Region  | Description                      |
| ------- | -------------------------------- |
| Stack   | Function-local automatic storage |
| Heap    | Explicitly allocated memory      |
| Static  | Global and static variables      |
| Foreign | Memory not owned by the program  |

Ownership rules apply uniformly across regions.

---

## 4. Ownership Annotations

Ownership is expressed via **type annotations**.

### 4.1 Ownership Qualifiers

```c
@owned
@borrowed
@nullable
```

These qualifiers apply only to **pointer types**.

---

### 4.2 `@owned`

```c
@owned T*
```

Semantics:

* The pointer owns the referenced allocation
* Exactly one owner exists at a time
* The owner is responsible for freeing the memory

Rules:

* Copying an `@owned` pointer is forbidden
* Ownership must be transferred explicitly
* Dropping an `@owned` pointer without freeing is an error

---

### 4.3 `@borrowed`

```c
@borrowed T*
```

Semantics:

* The pointer does not own the memory
* The memory must outlive the borrow
* Borrowed pointers may be copied freely

Rules:

* Borrowed pointers cannot escape their lifetime
* Borrowed pointers cannot be freed

---

### 4.4 `@nullable`

```c
@nullable T*
```

Semantics:

* Pointer may be `null`
* Dereference requires explicit null check

`@nullable` may be combined with `@owned` or `@borrowed`.

---

## 5. Default Ownership Rules

| Context                  | Default          |
| ------------------------ | ---------------- |
| Function parameters      | `@borrowed`      |
| Function return pointers | `@owned`         |
| Local pointer variables  | Inferred         |
| Struct fields            | Must be explicit |

Example:

```c
fn foo(i32* p);          // @borrowed
fn alloc() -> i32*;     // @owned
```

---

## 6. Ownership Transfer

Ownership transfer must be **explicit**.

### 6.1 Move Semantics

```c
@owned i32* a = alloc();
@owned i32* b = move(a); // a becomes invalid
```

Rules:

* Using a moved-from pointer is a compile-time error
* Moves are shallow (pointer value only)

---

### 6.2 Function Calls

```c
fn consume(@owned Buffer* buf);

consume(move(my_buf));
```

Ownership is transferred to the callee.

---

## 7. Lifetimes

### 7.1 Lifetime Concept

A **lifetime** represents the scope during which a memory location is valid.

Lifetimes are:

* Lexical
* Inferred
* Never written explicitly (by design)

---

### 7.2 Borrow Rules

A borrowed pointer:

* Must not outlive the owned pointer
* Must not escape its defining scope

Example (illegal):

```c
@borrowed i32* leak;

fn bad() {
    i32 x = 10;
    leak = &x; // error: escapes lifetime
}
```

---

### 7.3 Multiple Borrows (Aliasing Rule)

C 2030 enforces the **aliasing rule**: at any given time, a value may have EITHER:

* **Any number of immutable borrows** (`@borrowed T*`), OR
* **Exactly one mutable borrow** (`@mut @borrowed T*`)

But **never both simultaneously**. See RFC-0020 for the complete specification of mutable borrows, non-lexical lifetimes, and aliasing rules.

```c
var i32 x = 10;
let p1 = &x;            // immutable borrow
let p2 = &x;            // OK: multiple immutable borrows
// let q = &mut x;      // ERROR: cannot take mutable borrow while immutable borrows exist
```

This is enforced conservatively in safe code and relaxed in `unsafe` blocks.

---

## 8. Freeing Memory

### 8.1 Explicit Free

```c
free(ptr);
```

Rules:

* Only `@owned` pointers may be freed
* Double-free is a compile-time error if provable
* Otherwise, a runtime trap in safe mode

---

### 8.2 Automatic Scope Cleanup (`defer`)

```c
@owned Buffer* buf = alloc();
defer free(buf);
```

Rules:

* `defer` executes on scope exit (including early returns via `?` operator)
* Deferred actions are canceled on ownership transfer via `move()`
* Order is LIFO (last `defer` executes first)
* `defer` runs even during panic unwinding (cleanup is guaranteed)

### 8.3 Conditional Defer (`defer_on_err`)

```c
@owned Socket* sock = socket_create()?;
defer_on_err close(sock);  // only runs if the function returns Err
```

* `defer_on_err` executes only when the enclosing function returns an `Err` variant
* Useful for partial initialization where cleanup depends on overall success
* Order relative to `defer` follows LIFO as if interleaved

---

## 9. Use-After-Free Prevention

The compiler must reject:

* Use of freed pointers
* Use of moved-from pointers
* Borrowing from freed memory

Example:

```c
free(p);
*p = 10; // compile-time error
```

---

## 10. Arrays and Slices

### 10.1 Borrowed Slices

```c
fn read(@borrowed array<u8> buf);
```

Rules:

* Length is part of the borrow
* Bounds checked in safe code

---

### 10.2 Pointer Arithmetic

* Forbidden on borrowed arrays
* Allowed on raw pointers in `unsafe`

---

## 11. Struct Ownership

Struct fields must declare ownership explicitly.

```c
struct File {
    @owned Buffer* data;
}
```

Rules:

* Struct owns its owned fields
* Moving the struct moves ownership
* Copying such structs is forbidden

---

## 12. Global & Static Memory

### 12.1 Static Pointers

```c
static @owned i32* global;
```

Rules:

* Must be initialized exactly once
* Lifetime is program-wide
* Freeing is optional but explicit

---

## 13. Foreign Memory

### 13.1 External Ownership

```c
extern fn c_alloc() -> i32*;
```

Defaults:

* Returned pointers are assumed `@owned`
* May be overridden via attributes

```c
extern fn get_buf() -> @borrowed u8*;
```

---

## 14. Unsafe Blocks

### 14.1 Semantics

```c
unsafe {
    // ownership and safety rules relaxed
}
```

An `unsafe` block simultaneously relaxes **both** ownership rules (this RFC) and safety guarantees (RFC-0005). Within `unsafe`:

* Ownership: lifetime violations permitted, raw pointer arithmetic permitted, borrow aliasing rules relaxed
* Safety: UB is permitted but localized, trap checks may be omitted (RFC-0005 §11)
* Compiler emits warnings, not errors

Outside `unsafe`, all rules are enforced strictly. There is no way to relax ownership without also entering `unsafe` — they are the same mechanism.

---

## 15. Interaction with Legacy C

* Legacy code bypasses ownership checking
* Crossing boundary requires explicit casts
* Mixed-mode compilation is permitted

---

## 16. Diagnostics Requirements

A conforming compiler must:

* Emit clear ownership errors
* Highlight lifetime boundaries
* Identify unsafe operations
* Avoid false positives where provably safe

---

## 17. Formal Guarantees (Safe Code)

In safe C 2030 code:

* No use-after-free
* No double-free
* No null dereference
* No out-of-bounds access
* No dangling borrows

These guarantees do **not** apply to `unsafe` blocks.

---

## 18. Rationale & Comparison

### Why not Rust-style borrow checker?

* Rust’s model is too restrictive for kernel code
* Annotation-driven ownership fits C’s mental model
* Unsafe remains usable and explicit

### Why annotations instead of inference?

* Predictability
* Auditing
* ABI clarity

---

## 19. Summary

C 2030’s ownership model provides **real memory safety guarantees** without sacrificing:

* Performance
* Control
* Compatibility

It formalizes what disciplined C programmers already *try* to do—now with compiler enforcement.

---

## 20. Next RFC

* **RFC 0005 — Undefined Behavior & Safety Guarantees**
* **RFC 0006 — Modules & Visibility**
* **RFC 0007 — Standard Library & Allocators**


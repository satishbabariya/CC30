# RFC 0020

## Mutable Borrows & Aliasing Rules

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

This RFC specifies the **mutable borrow syntax**, **aliasing rules**, and **non-lexical lifetime** semantics for C 2030. RFC-0004 §7.3 stated "Single mutable borrow (future RFC may refine mutability)" — this is that RFC.

The aliasing rule is the fundamental guarantee that enables C 2030's memory safety and data-race freedom.

---

## 2. Motivation

Pointer aliasing bugs are among the most dangerous in C:

* **Data races** — Two threads writing to the same memory
* **Iterator invalidation** — Modifying a container while iterating
* **Stale references** — Reading memory that was silently modified through another pointer
* **Undefined behavior** — C's `restrict` is advisory; violations are UB

C 2030 eliminates these at compile time via a **statically enforced aliasing rule**.

---

## 3. The Aliasing Rule

**At any given point in the program, a value may have EITHER:**

1. **Any number of immutable borrows** (`@borrowed T*`), OR
2. **Exactly one mutable borrow** (`@mut @borrowed T*`)

**But never both simultaneously.**

This rule is the single most important invariant in C 2030's safety model. It guarantees:

* No data races in safe code (RFC-0008)
* No iterator invalidation
* No stale references
* Compiler can optimize assuming no aliasing

---

## 4. Mutable Borrow Syntax

### 4.1 Creating a Mutable Borrow

```c
var i32 x = 10;
@mut @borrowed i32* p = &mut x;   // explicit annotation
*p = 20;
```

With `let`/`var` inference (RFC-0015, RFC-0019):

```c
var i32 x = 10;
let p = &mut x;    // inferred as @mut @borrowed i32*
*p = 20;
```

### 4.2 Creating an Immutable Borrow

```c
var i32 x = 10;
@borrowed i32* p = &x;            // explicit annotation
let q = &x;                        // inferred
```

### 4.3 Shorthand

| Full form | Shorthand |
|-----------|-----------|
| `@borrowed T*` | `&T` in type position (future consideration) |
| `@mut @borrowed T*` | `&mut T` in type position (future consideration) |

---

## 5. Non-Lexical Lifetimes (NLL)

### 5.1 Concept

A borrow is active from its **creation** to its **last use** — not until the end of the lexical scope. This is called Non-Lexical Lifetimes (NLL).

```c
var i32 x = 10;

let p = &x;              // immutable borrow starts
printf("%d\n", *p);      // last use of p — borrow ends HERE

let q = &mut x;          // OK: no active immutable borrows
*q = 20;                  // mutable borrow active
printf("%d\n", *q);      // last use of q — mutable borrow ends
```

### 5.2 Without NLL (This Would Fail)

If borrows lasted until end of scope (lexical lifetimes):

```c
var i32 x = 10;
let p = &x;              // borrow would last until closing }
printf("%d\n", *p);
let q = &mut x;          // ERROR: p is still "alive" in lexical model
*q = 20;                  // This is safe with NLL
```

### 5.3 NLL Algorithm

The compiler performs a **liveness analysis** on borrows:

1. For each borrow, compute the set of program points where it is **live** (may still be used)
2. Two borrows conflict if their live ranges overlap and at least one is mutable
3. Conflicts are compile-time errors

---

## 6. Function Parameters

### 6.1 Immutable Borrow (Default)

```c
fn read_value(@borrowed i32* val) -> i32 {
    return *val;
}
```

Default for pointer parameters (RFC-0004 §5).

### 6.2 Mutable Borrow

```c
fn increment(@mut @borrowed i32* val) {
    *val += 1;
}
```

Caller must have mutable access:

```c
var i32 x = 10;
increment(&mut x);
printf("%d\n", x);  // 11
```

### 6.3 Ownership Transfer

```c
fn consume(@owned Buffer* buf) {
    // buf is owned — caller loses access
    free(buf);
}
```

---

## 7. Aliasing Rules in Practice

### 7.1 Multiple Immutable Borrows (OK)

```c
var i32 x = 42;
let a = &x;
let b = &x;
let c = &x;
printf("%d %d %d\n", *a, *b, *c);  // OK: all immutable
```

### 7.2 Mutable Borrow Excludes Others

```c
var i32 x = 42;
let p = &mut x;
// let q = &x;      // ERROR: cannot borrow 'x' while mutably borrowed
// let r = &mut x;  // ERROR: cannot mutably borrow 'x' more than once
*p = 100;
// After p's last use, x can be borrowed again:
let s = &x;         // OK: p is no longer live
```

### 7.3 Cannot Read Original While Mutably Borrowed

```c
var i32 x = 42;
let p = &mut x;
// printf("%d\n", x);  // ERROR: cannot read 'x' while mutably borrowed
*p = 100;
printf("%d\n", x);     // OK: p is no longer live
```

---

## 8. Interaction with `let`/`var` (RFC-0015)

### 8.1 Binding Immutability vs. Borrow Mutability

These are **orthogonal concepts**:

| Binding | Borrow | Meaning | Valid? |
|---------|--------|---------|--------|
| `let` | `@borrowed` | Immutable binding to immutable borrow | Yes |
| `var` | `@borrowed` | Mutable binding to immutable borrow (can reassign the pointer, not the data) | Yes |
| `var` | `@mut @borrowed` | Mutable binding to mutable borrow | Yes |
| `let` | `@mut @borrowed` | Immutable binding to mutable borrow (can mutate data, cannot reassign pointer) | Yes |

### 8.2 Cannot Take Mutable Borrow of `let` Binding

```c
let i32 x = 42;
// let p = &mut x;  // ERROR: cannot mutably borrow immutable binding 'x'
let q = &x;         // OK: immutable borrow of immutable binding
```

### 8.3 `var` Required for Mutation Target

```c
var i32 x = 42;
let p = &mut x;     // OK: x is 'var', mutable borrow allowed
*p = 100;
```

---

## 9. Aliasing in Structs

### 9.1 Conservative Rule

Borrowing any field of a struct borrows the **entire struct**:

```c
struct Pair { i32 a; i32 b; }

var Pair p = { .a = 1, .b = 2 };
let ref_a = &mut p.a;
// let ref_b = &p.b;  // ERROR: 'p' is already mutably borrowed via 'p.a'
*ref_a = 10;
```

### 9.2 Rationale

The conservative rule prevents complex interactions where field borrows overlap or the struct is moved while fields are borrowed. It keeps the borrow checker simple and predictable.

### 9.3 Disjoint Field Borrows (Future)

A future RFC may relax this rule to allow **disjoint field borrows**:

```c
// FUTURE (not in this RFC):
let ref_a = &mut p.a;
let ref_b = &mut p.b;  // Would be OK: a and b are disjoint fields
```

This requires the compiler to prove non-overlapping access — complex but feasible.

---

## 10. Interaction with Concurrency (RFC-0008)

### 10.1 Thread Safety from Aliasing

The aliasing rule **prevents data races by construction**:

* If a value has multiple borrows, they are all immutable — no writes, no race
* If a value has a mutable borrow, it is exclusive — only one accessor, no race

### 10.2 Send and Sync Concepts

* **Sendable**: A type whose values can be safely transferred to another thread. All `@owned` types are sendable.
* **Shareable**: A type that can be safely accessed from multiple threads via immutable references. Requires the type to be free of interior mutability.
* `@mut @borrowed T*` is **not sendable** — it cannot cross thread boundaries in safe code
* `@borrowed T*` is sendable only if `T` is shareable (e.g., contains no mutable state)
* `atomic<T>` is both sendable and shareable

### 10.3 Mutex Interaction

A `Mutex<T>` provides mutable access through an immutable reference (interior mutability):

```c
let guard = mutex.lock();   // returns @mut @borrowed T* (exclusive access)
guard.data += 1;            // mutation through the lock
// guard dropped → lock released
```

The mutex ensures the aliasing rule holds at runtime (only one lock holder at a time).

---

## 11. Reborrowing

### 11.1 Implicit Reborrowing

A mutable borrow can be temporarily reborrowed as immutable:

```c
fn read_only(@borrowed i32* val) -> i32 { return *val; }
fn mutate(@mut @borrowed i32* val) { *val += 1; }

var i32 x = 10;
let p = &mut x;
let v = read_only(p);  // p is temporarily reborrowed as @borrowed
mutate(p);              // p is still valid as @mut @borrowed
```

### 11.2 Reborrow Duration

The reborrow lasts for the duration of the function call. After the call returns, the original mutable borrow is restored.

---

## 12. Container Interaction

### 12.1 Borrowing Into a Container

Borrowing an element of a container while mutating the container is forbidden:

```c
var Vec<i32> items = Vec::new();
items.push(1);
items.push(2);

let first = &items[0];   // borrows 'items'
// items.push(3);         // ERROR: 'items' is borrowed
printf("%d\n", *first);  // OK
items.push(3);            // OK: first is no longer live
```

### 12.2 Rationale

`push` may reallocate the backing buffer, invalidating existing pointers. The aliasing rule prevents this by treating `push` as a mutable operation on `items`.

---

## 13. `unsafe` Escape Hatch

Inside `unsafe`, aliasing rules are not enforced:

```c
unsafe {
    var i32 x = 42;
    let p: i32* = &x;
    let q: i32* = &x;
    // Both p and q can read and write — programmer responsibility
    *p = 100;
    *q = 200;
}
```

Raw pointers (`T*` without `@borrowed` or `@mut @borrowed`) bypass the borrow checker entirely. This is necessary for:

* FFI with C code
* Low-level data structures (intrusive linked lists)
* Hardware register access
* Performance-critical code where aliasing is known safe

---

## 14. Compiler Diagnostics

### 14.1 Borrow Conflict

```
error[E2001]: cannot borrow 'x' as mutable because it is also borrowed as immutable
  --> src/main.c30:8:15
   |
 6 | let p = &x;
   |         -- immutable borrow occurs here
 7 | let q = &mut x;
   |         ^^^^^^ mutable borrow occurs here
 8 | printf("%d", *p);
   |              -- immutable borrow later used here
   |
   = help: consider using the immutable borrow before taking a mutable borrow
```

### 14.2 Mutation While Borrowed

```
error[E2002]: cannot mutate 'items' because it is borrowed
  --> src/main.c30:12:5
   |
10 | let first = &items[0];
   |              ----- immutable borrow of 'items' occurs here
11 |
12 | items.push(3);
   | ^^^^^^^^^^^^^ mutation of 'items' occurs here
13 | printf("%d", *first);
   |              ------ immutable borrow later used here
```

### 14.3 Mutable Borrow of Immutable Binding

```
error[E2003]: cannot borrow 'x' as mutable, as it is declared as 'let'
  --> src/main.c30:5:15
   |
 4 | let i32 x = 42;
   |     --- 'x' declared as immutable with 'let'
 5 | let p = &mut x;
   |         ^^^^^^ cannot mutably borrow
   |
   = help: consider changing 'let' to 'var' if mutation is intended
```

---

## 15. Complete Examples

### 15.1 Buffer Resize Safety

```c
fn safe_resize(@mut @borrowed Vec<u8>* buf, usize new_cap) -> Result<(), MemError> {
    // No external borrows can exist while we hold &mut buf
    let new_ptr = realloc(buf->ptr, new_cap)?;
    buf->ptr = new_ptr;
    buf->capacity = new_cap;
    return Ok(());
}
```

### 15.2 Iterator Pattern

```c
fn sum(@borrowed array<i32> items) -> i64 {
    var i64 total = 0;
    // items is immutably borrowed — cannot be modified during iteration
    for (var usize i = 0; i < items.length; i++) {
        total += (i64)items[i];
    }
    return total;
}
```

### 15.3 Swap Function

```c
fn swap<T>(@mut @borrowed T* a, @mut @borrowed T* b) {
    let tmp = *a;
    *a = *b;
    *b = tmp;
}

var i32 x = 1;
var i32 y = 2;
swap(&mut x, &mut y);  // OK: x and y are distinct variables
// swap(&mut x, &mut x);  // ERROR: cannot mutably borrow 'x' twice
```

---

## 16. Rejected Alternatives

### 16.1 Rust-Identical Syntax (`&` and `&mut`)

Considered but modified. C 2030 uses `@borrowed` and `@mut @borrowed` annotations to be consistent with the existing annotation system (`@owned`, `@nullable`). The `&x` and `&mut x` expression syntax is adopted for creating borrows.

### 16.2 C `restrict` Keyword

Rejected as the primary mechanism. `restrict` is:
* Advisory only — violations are UB, not errors
* Applies only to function parameters
* Not checked by any mainstream compiler

C 2030 enforces aliasing rules statically in safe code.

### 16.3 No Mutable Borrows (Immutable Only)

Rejected. Pure immutability is impractical for systems code. Mutation through borrowed references is essential for in-place updates, lock-guarded data, and output parameters.

---

## 17. Open Questions

1. **Disjoint field borrows** — Should a future RFC allow `&mut s.a` and `&mut s.b` simultaneously when `a` and `b` are disjoint fields?
2. **Interior mutability** — Should C 2030 provide `Cell<T>` / `RefCell<T>` equivalents for controlled mutation through immutable references?
3. **Two-phase borrows** — Should `items.push(items.len())` be allowed? (Rust supports this via two-phase borrows)
4. **Borrow splitting** — Can a mutable borrow of an array be split into non-overlapping sub-borrows?

---

## 18. References

* RFC-0004: Ownership, Lifetimes & Memory Safety (§7.3 — multiple borrows)
* RFC-0005: Undefined Behavior & Safety Guarantees
* RFC-0008: Concurrency & Atomics (§8 — data race rules)
* RFC-0015: Swift-Inspired Syntax Refinements (§3 — `let`/`var`)
* Rust Reference — [References and Borrowing](https://doc.rust-lang.org/book/ch04-02-references-and-borrowing.html)
* Rust RFC 2094 — [Non-Lexical Lifetimes](https://rust-lang.github.io/rfcs/2094-nll.html)
* C Standard — `restrict` qualifier (C99 §6.7.3.1)

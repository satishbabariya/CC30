# RFC 0017

## Closures, Lambdas & Capture Rules

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

This RFC specifies the **closure and lambda** syntax, type system, and capture semantics for C 2030. Closures are anonymous functions that can capture variables from their enclosing scope.

RFC-0008 (Concurrency) uses closures in `spawn(|| { ... })` without formal specification. This RFC provides the complete definition.

---

## 2. Motivation

Systems code requires first-class callable values for:

* **Callbacks** — event handlers, interrupt handlers, completion routines
* **Concurrency** — thread entry points via `spawn`
* **Iterators** — `map`, `filter`, `fold` operations
* **Sorting** — custom comparators
* **Resource management** — deferred cleanup with context

C uses function pointers for these, but function pointers cannot capture state from their environment. C 2030 closures add state capture while integrating with the ownership model.

---

## 3. Closure Syntax

### 3.1 Basic Forms

```c
// No parameters, multi-statement body
|| { printf("hello\n"); }

// Single parameter, expression body
|i32 x| x * 2

// Multiple parameters, block body
|i32 x, i32 y| { return x + y; }

// No parameters, expression body
|| 42
```

### 3.2 Type Annotations

Parameter types can be explicit or inferred (see RFC-0019):

```c
// Explicit types
|i32 x, i32 y| x + y

// Inferred types (when context provides enough information)
|x, y| x + y    // types inferred from usage context
```

Return types are always inferred from the body.

---

## 4. Closure Types

### 4.1 Function Pointer Type (No Captures)

```c
type Comparator = fn(i32, i32) -> i32;
```

A function pointer is a plain pointer to a function. It cannot capture variables.

### 4.2 Closure Type (With Captures)

```c
type Callback = closure(i32) -> bool;
```

A `closure` type represents a callable value that **may** capture variables. It is represented internally as a struct containing captured data plus a function pointer.

### 4.3 Coercion

A closure with **no captures** can be implicitly coerced to a function pointer:

```c
let f: fn(i32) -> i32 = |i32 x| x * 2;  // OK: no captures
```

A closure **with captures** cannot be coerced to a function pointer:

```c
var i32 offset = 10;
let f: fn(i32) -> i32 = |i32 x| x + offset;  // ERROR: captures 'offset'
```

### 4.4 Size

* Function pointers: platform pointer size (8 bytes on 64-bit)
* Closures: variable size depending on captured data. Use `@dynamic` protocol for type-erased storage (see §10).

---

## 5. Capture Modes

### 5.1 Borrow Capture (Default)

By default, closures **borrow** variables from the enclosing scope:

```c
var i32 x = 10;
let c = || { printf("%d\n", x); };  // borrows x
c();  // prints 10
x = 20;
c();  // prints 20
```

Borrow capture creates an `@borrowed` reference to the captured variable. The closure must not outlive the borrowed data.

### 5.2 Move Capture

`move` before the parameter list transfers ownership of captured variables into the closure:

```c
var @owned Buffer* buf = alloc<Buffer>(1);
let c = move || {
    use(buf);       // buf is owned by the closure
    free(buf);
};
// buf is invalid here — ownership moved to closure
c();
```

### 5.3 Copy Capture

For `Copy` types (primitives, small structs), captures can be explicitly copied:

```c
var i32 x = 10;
let c = copy || { printf("%d\n", x); };  // copies x
x = 20;
c();  // prints 10 — captured the old value
```

### 5.4 Explicit Capture List

For fine-grained control, specify capture mode per variable:

```c
var i32 x = 10;
var @owned Buffer* buf = alloc<Buffer>(1);

let c = [copy x, move buf] || {
    printf("x = %d\n", x);  // x was copied
    use(buf);                 // buf was moved
};
// x is still valid (was copied)
// buf is invalid (was moved)
```

### 5.5 Capture List Syntax

```ebnf
capture_list ::= '[' capture (',' capture)* ']'
capture      ::= capture_mode identifier
capture_mode ::= 'borrow' | 'move' | 'copy'
```

---

## 6. Closures and Concurrency (RFC-0008)

### 6.1 `spawn` Requires Move Capture

The `spawn` function creates a new thread. Because `@borrowed` references cannot escape thread boundaries (RFC-0004), `spawn` requires a **move-capture** closure:

```c
fn spawn(closure() -> void @move) -> @owned Thread;
```

```c
var @owned Buffer* data = alloc<Buffer>(1024);

@owned Thread t = spawn(move || {
    process(data);   // data ownership transferred to thread
    free(data);
});
// data is invalid here
t.join();
```

### 6.2 Compiler Enforcement

If a closure passed to `spawn` borrows instead of moving, the compiler rejects it:

```
error[E1701]: closure passed to 'spawn' must use move capture
  --> src/main.c30:12:25
   |
12 | @owned Thread t = spawn(|| {
   |                         ^^ this closure borrows 'data'
   |
   = note: borrowed references cannot escape to another thread
   = help: use 'move ||' to transfer ownership
```

### 6.3 Scoped Threads

`spawn_scoped` permits borrow-capture closures because the thread is guaranteed to join before the scope exits:

```c
fn scoped_example(@borrowed i32* data) {
    spawn_scoped(|| {
        printf("%d\n", *data);  // OK: scoped thread borrows safely
    });
    // implicit join here — data is still valid
}
```

---

## 7. Closures and Ownership (RFC-0004)

### 7.1 Closure Ownership

Closures themselves are values with ownership semantics:

* A closure created with `move` captures is `@owned` — it owns the captured data
* Dropping an `@owned` closure drops its captured data
* Closures can be moved, borrowed, or stored in structs

### 7.2 Mutable Capture

Closures that mutate captured variables require `@mut` borrow capture:

```c
var i32 count = 0;
let mut_c = mut || { count += 1; };  // mutable borrow capture
mut_c();
mut_c();
printf("%d\n", count);  // prints 2
```

While `mut_c` exists, no other borrows of `count` are permitted (aliasing rule, RFC-0020).

---

## 8. Lifetime Rules

### 8.1 Borrow-Capture Closures

A borrow-capture closure must not outlive any borrowed variable:

```c
fn bad() -> closure() -> i32 {
    var i32 x = 42;
    return || x;  // ERROR: closure borrows x, which is local
}
```

### 8.2 Move-Capture Closures

A move-capture closure is independent — it owns its data and can be returned, stored, or passed to other threads:

```c
fn make_adder(i32 n) -> closure(i32) -> i32 {
    return move |i32 x| x + n;  // n is copied (i32 is Copy)
}

let add5 = make_adder(5);
printf("%d\n", add5(10));  // 15
```

---

## 9. Function Pointer Compatibility

### 9.1 Conversion Rules

| From | To | Allowed |
|------|----|---------|
| `closure` (no captures) | `fn(...)` | Yes (implicit coercion) |
| `closure` (with captures) | `fn(...)` | No |
| `fn(...)` | `closure(...)` | Yes (implicit widening) |
| Named function | `fn(...)` | Yes |
| Named function | `closure(...)` | Yes |

### 9.2 C Interop

When passing closures to C functions expecting function pointers, only capture-less closures are compatible:

```c
extern "C" fn qsort(void* base, usize count, usize size, fn(void*, void*) -> i32 cmp);

// OK: no captures
qsort(data, n, sizeof(i32), |void* a, void* b| {
    return *(i32*)a - *(i32*)b;
});
```

---

## 10. Closures in Structs

### 10.1 With Known Type

If the closure type is known (e.g., a specific function signature with no captures):

```c
struct Handler {
    fn(i32) -> bool callback;  // function pointer, no captures
}
```

### 10.2 With Type Erasure (`@dynamic` Protocol)

For closures with captures, use the `Callable` protocol (RFC-0015 §5):

```c
@dynamic protocol Callable<Ret> {
    fn call(@borrowed Self* self) -> Ret;
}

struct EventLoop {
    @owned Callable<void>* handler;  // type-erased closure
}
```

### 10.3 With Generic Type

```c
struct Mapper<F> {
    F transform;  // F is a specific closure type, monomorphized
}

fn map_items<F: Callable<i32>>(i32[] items, usize count, F transform) -> i32[] {
    // ...
}
```

---

## 11. Compiler Diagnostics

### 11.1 Borrow Escaping

```
error[E1702]: closure borrows 'x' which does not live long enough
  --> src/main.c30:8:12
   |
 5 | fn make_closure() -> closure() -> i32 {
 6 |     var i32 x = 42;
 7 |     return || x;
   |            ^^^^^ 'x' borrowed here
 8 | }
   | - 'x' dropped here while still borrowed
   |
   = help: use 'move ||' to take ownership of 'x'
```

### 11.2 Missing Move for Spawn

```
error[E1703]: cannot pass borrow-capture closure to 'spawn'
  --> src/thread.c30:10:23
   |
10 |     let t = spawn(|| { use(data); });
   |                   ^^^^^^^^^^^^^^^^^^
   |
   = note: 'spawn' requires move capture to prevent dangling references
   = help: use 'move || { use(data); }' instead
```

---

## 12. Complete Examples

### 12.1 Iterator-Style Processing

```c
fn filter<T>(
    @borrowed array<T> items,
    closure(@borrowed T*) -> bool predicate
) -> @owned array<T> {
    var @owned array<T> result = array_new<T>(items.length);
    for (var usize i = 0; i < items.length; i++) {
        if (predicate(&items[i])) {
            result.push(items[i]);
        }
    }
    return result;
}

// Usage:
let evens = filter(numbers, |@borrowed i32* n| *n % 2 == 0);
```

### 12.2 Event Registration

```c
struct Button {
    @owned closure() -> void on_click;
}

fn create_counter_button() -> @owned Button* {
    var @owned Button* btn = alloc<Button>(1);
    var @owned i32* count = alloc<i32>(1);
    *count = 0;

    btn->on_click = move || {
        *count += 1;
        printf("Clicked %d times\n", *count);
    };

    return btn;
}
```

### 12.3 Sorting

```c
fn sort_by<T>(
    @borrowed T[] items,
    usize count,
    fn(@borrowed T*, @borrowed T*) -> i32 compare
) {
    // ... sorting algorithm using compare
}

// Usage:
sort_by(users, user_count, |@borrowed User* a, @borrowed User* b| {
    return strcmp(a->name, b->name);
});
```

---

## 13. Rejected Alternatives

### 13.1 C++ Lambda Syntax `[&](){}`

Rejected. The `[capture](params){}` syntax is verbose and the capture-specifier characters (`&`, `=`) are overloaded with other meanings in C. The `|params|` syntax is cleaner and well-established (Rust, Ruby, Smalltalk).

### 13.2 Implicit Capture Only

Rejected. Explicit capture modes (`move`, `copy`, `borrow`) are essential for ownership safety. Implicit capture (like JavaScript) would undermine C 2030's explicit ownership model.

### 13.3 No Closures (Function Pointers Only)

Rejected. Function pointers cannot capture state, forcing users to pass context via `void*` — a major source of type-safety bugs in C. Closures are needed for ergonomic concurrency (`spawn`).

---

## 14. Open Questions

1. **Recursive closures** — Should closures be able to call themselves? (Requires naming the closure, which contradicts anonymity)
2. **Async closures** — If C 2030 adds async/await, how do closures interact? (Deferred to async RFC)
3. **Closure size optimization** — Should the compiler guarantee small-closure optimization (inline storage up to N bytes)?
4. **Partial application** — Should C 2030 support partial function application via closures?

---

## 15. References

* RFC-0004: Ownership, Lifetimes & Memory Safety
* RFC-0008: Concurrency & Atomics (§3.1 — `spawn`)
* RFC-0015: Swift-Inspired Syntax Refinements (§5 — protocols)
* RFC-0020: Mutable Borrows & Aliasing Rules
* Rust Reference — [Closures](https://doc.rust-lang.org/reference/types/closure.html)
* Swift Programming Language — [Closures](https://docs.swift.org/swift-book/documentation/the-swift-programming-language/closures/)

# RFC 0003

## Type System & Layout Rules

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

This RFC defines the **type system**, **type semantics**, and **memory layout rules** for the C 2030 programming language.

The type system is designed to:

* Preserve C’s low-level control and ABI compatibility
* Eliminate ambiguous and unsafe type behavior
* Provide predictable, auditable memory layouts
* Enable static analysis and safe-by-default behavior

This RFC intentionally avoids runtime polymorphism and hidden behavior.

---

## 2. Design Goals

The C 2030 type system must:

1. Be **predictable**
2. Be **ABI-stable**
3. Be **layout-transparent**
4. Enable **compile-time verification**
5. Remain **compatible with C**

No type feature may obscure memory representation.

---

## 3. Primitive Types

### 3.1 Integer Types (Mandatory)

| Type  | Size   | Signed |
| ----- | ------ | ------ |
| `i8`  | 8-bit  | Yes    |
| `i16` | 16-bit | Yes    |
| `i32` | 32-bit | Yes    |
| `i64` | 64-bit | Yes    |
| `u8`  | 8-bit  | No     |
| `u16` | 16-bit | No     |
| `u32` | 32-bit | No     |
| `u64` | 64-bit | No     |

Rules:

* Two’s complement required
* Integer overflow behavior is **defined** (see RFC 0005)
* No implicit integer widening

---

### 3.2 Floating-Point Types

| Type  | Standard |
| ----- | -------- |
| `f32` | IEEE-754 |
| `f64` | IEEE-754 |

Floating-point behavior must be deterministic unless explicitly relaxed.

---

### 3.3 Boolean Type

```c
bool
```

* Stored as `u8`
* Values restricted to `true` or `false`

---

### 3.4 Void Type

```c
void
```

* Represents absence of value
* May not be instantiated

---

## 4. Pointer Types

### 4.1 Raw Pointers

```c
T*
```

Properties:

* Addressable
* Nullable by default
* No bounds information

Raw pointers are **unsafe by default**.

---

### 4.2 Annotated Pointers

```c
@owned T*
@borrowed T*
@mut @borrowed T*
@nullable T*
T*?               // shorthand for @nullable T* (RFC-0015 §6)
```

Rules:

* Ownership annotations are enforced in safe code
* `@mut @borrowed` denotes a mutable borrow (see RFC-0020)
* `T*?` and `@nullable T*` are interchangeable (see RFC-0015 §6)
* Violations are compile-time errors
* Ignored in legacy mode

---

### 4.3 Pointer Arithmetic

* Permitted only on raw pointers
* Forbidden on safe arrays
* Must remain within allocation bounds (unless `unsafe`)

---

## 5. Arrays

### 5.1 Fixed-Size Arrays

```c
T[N]
```

Rules:

* Size known at compile time
* Contiguous memory
* Bounds checked in safe code

---

### 5.2 Dynamic Arrays

```c
array<T>
```

Properties:

* Pointer + length
* Explicit allocation
* No implicit resizing

---

### 5.3 Array Layout Guarantees

```text
struct array<T> {
    T* data;
    usize length;
}
```

Layout is guaranteed and ABI-stable.

---

## 6. Strings

```c
string
```

Properties:

* UTF-8 encoded
* Immutable
* Length-aware
* Not null-terminated by default

Legacy interoperability:

```c
char* c_str(string s);
```

---

## 7. Struct Types

### 7.1 Definition

```c
struct Vec2 {
    f32 x;
    f32 y;
}
```

---

### 7.2 Layout Rules

* Field order is preserved
* Padding is deterministic
* Compiler must warn on implicit padding
* Layout is ABI-compatible unless overridden

---

### 7.3 Attributes

```c
@packed
@aligned(16)
struct Header { ... }
```

Rules:

* `@packed` removes padding (may affect performance)
* Misaligned access in safe code is forbidden

---

## 8. Enum Types

### 8.1 Definition

```c
enum Color : u8 {
    Red,
    Green,
    Blue,
}
```

Rules:

* Underlying type must be specified
* Values are contiguous unless explicitly assigned
* Exhaustive checking required in `match`

---

### 8.2 C Compatibility

Enums are layout-compatible with C enums of the same size.

---

## 9. Union Types

### 9.1 Raw Unions (Legacy)

```c
union U {
    i32 a;
    f32 b;
}
```

* Unsafe
* No active field tracking

---

### 9.2 Tagged Unions (Sum Types)

```c
union Result<T> {
    Ok(T),
    Err(i32),
}
```

Layout:

```text
struct {
    tag: enum;
    payload: union;
}
```

Rules:

* Tag must be checked before access
* Compiler enforces correctness

---

## 10. Type Aliases

```c
type UserId = u64;
```

Rules:

* Strong typedefs
* Not implicitly interchangeable with base type

Explicit cast required.

---

## 11. Generics

### 11.1 Definition

```c
fn swap<T>(T* a, T* b);
```

Rules:

* Monomorphized at compile time
* No runtime type information
* No specialization ambiguity

### 11.2 Protocol Constraints

Generic type parameters may be constrained by protocols (see RFC-0015 §5):

```c
fn sort<T: Comparable>(T[] items, usize count);
fn create_map<K: Hashable + Comparable, V>(usize capacity) -> Result<@owned Map<K, V>*, MemError>;
```

* Constrained generics are checked at compile time
* Unconstrained generics accept any type (existing behavior)

---

## 12. Const & Immutability

### 12.1 Constants

```c
const i32 MAX = 100;
```

* Compile-time evaluated
* Immutable

---

### 12.2 Immutable Fields

```c
struct Config {
    const i32 port;
}
```

* Cannot be reassigned after initialization

---

## 13. Type Inference

* Local inference permitted (see RFC-0019 for complete rules)
* Function signatures must be explicit
* No global type inference
* `let` / `var` declarations enable inferred types: `let x = 42;` (see RFC-0015 §3, RFC-0019)

---

## 14. Casting Rules

### 14.1 Explicit Casts Only

```c
i32 x = (i32)y;
```

Forbidden:

* Implicit pointer casts
* Implicit integer narrowing

---

### 14.2 Bit Casts

```c
bitcast<T>(value);
```

* Requires equal size
* Explicitly unsafe

---

## 15. Zero-Sized Types

* Permitted
* Occupy no storage
* Useful for markers and compile-time logic

---

## 16. ABI Compatibility Rules

C 2030 guarantees:

* Struct layout matches C unless attributes differ
* Function calling conventions preserved
* Name mangling disabled by default

---

## 17. Error Conditions

The compiler must reject:

* Uninitialized reads
* Invalid enum access
* Out-of-bounds array access
* Misaligned access in safe code

---

## 18. Rationale & Design Notes

* Explicit integer sizes prevent portability bugs
* Strong typedefs prevent unit confusion
* Tagged unions eliminate error-prone conventions
* Layout transparency enables kernel use

---

## 19. Summary

The C 2030 type system preserves C’s power while eliminating ambiguity and undefined behavior. It is intentionally conservative, explicit, and layout-driven.


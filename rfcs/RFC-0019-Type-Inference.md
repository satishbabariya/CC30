# RFC 0019

## Type Inference Rules

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

This RFC defines the **type inference rules** for the C 2030 programming language. Type inference allows the compiler to deduce the type of a local variable from its initializer and surrounding usage context, eliminating redundant type annotations without sacrificing type safety.

Inference is **strictly local** — it applies only within function bodies. All function signatures, struct fields, module-level declarations, and exported symbols require explicit type annotations. This constraint preserves C ABI compatibility, enables separate compilation, and ensures that public interfaces remain self-documenting.

This RFC builds on RFC-0003 (Type System), RFC-0004 (Ownership), RFC-0007 (Error Handling), and RFC-0015 (`let`/`var` declarations). It formalizes the "local inference permitted" provision mentioned in RFC-0003 and fulfills the forward reference in RFC-0015 Section 3.5.

---

## 2. Motivation

C 2030 already provides a rich type system (RFC-0003) with ownership qualifiers (RFC-0004) and `let`/`var` declarations (RFC-0015). However, requiring explicit types on every local variable leads to verbose, repetitive code:

```c
let i32 x = 42;
let f64 pi = 3.14159;
let @owned Buffer* buf = alloc<Buffer>(1)?;
let Vec<i32> items = Vec::new();
let Result<Config, IoError> cfg = parse_config(path);
```

In each case, the type on the left is fully determined by the expression on the right. The programmer gains nothing from writing it out, and the visual noise obscures the logic of the code.

With type inference, the same code becomes:

```c
let x = 42;
let pi = 3.14159;
let buf = alloc<Buffer>(1)?;
let items = Vec::new();
let cfg = parse_config(path);
```

### 2.1 Design Goals

1. **Reduce verbosity** — Eliminate redundant annotations in local code
2. **Preserve clarity** — All public interfaces remain explicitly typed
3. **Maintain ABI stability** — No inferred types cross module boundaries
4. **Keep compilation simple** — No global inference, no cross-function deduction
5. **Interoperate with C** — Exported symbols always have concrete C-compatible types

---

## 3. Scope of Inference

### 3.1 Where Inference Is Permitted

Type inference applies exclusively to **local variable declarations** using `let` or `var` that include an initializer:

```c
let x = 42;            // OK: local with initializer
var y = some_func();    // OK: local with initializer
```

Bare declarations (without `let`/`var`) may also use inference when an initializer is present, though `let`/`var` is the recommended style.

### 3.2 Where Inference Is Prohibited

The following declarations **must** carry explicit type annotations. The compiler shall reject any declaration in these positions that omits a type:

| Context                     | Reason                                      |
| --------------------------- | ------------------------------------------- |
| Function parameters         | Required for overload resolution and ABI     |
| Function return types        | Required for separate compilation            |
| Struct / union fields        | Required for layout computation              |
| Enum variant payloads        | Required for layout computation              |
| Module-level variables       | Required for ABI stability                   |
| Exported (`pub`) symbols     | Required for cross-module and C interop      |
| `extern` declarations        | Required for FFI linkage                     |

### 3.3 Rationale

Local-only inference is a deliberate design constraint. Global inference (as in Haskell or ML) can produce surprising types at module boundaries, makes error messages harder to understand, and is fundamentally incompatible with C's compilation model. By restricting inference to function bodies, C 2030 guarantees that every inter-module contract is explicit and that header files (or module interfaces) remain human-readable.

---

## 4. Inference Rules

### 4.1 Integer Literals

An unsuffixed integer literal is inferred as **`i32`**:

```c
let x = 42;          // i32
let y = -1;           // i32
let z = 0xFF;         // i32
```

A suffixed integer literal takes the type indicated by its suffix:

```c
let a = 42_u8;        // u8
let b = 42_i64;       // i64
let c = 42_u64;       // u64
```

If the target type is known from context (e.g., assignment to a typed variable or function argument), the literal is checked for compatibility but the context type takes precedence. An error is emitted if the literal value does not fit the target type.

### 4.2 Floating-Point Literals

An unsuffixed floating-point literal is inferred as **`f64`**:

```c
let pi = 3.14159;    // f64
let e = 2.718;        // f64
```

A suffixed literal takes the indicated type:

```c
let x = 3.14_f32;    // f32
let y = 1.0_f64;     // f64
```

The default of `f64` follows the principle of least surprise — `f64` is the natural precision for most computations, matching the behavior of C's `double` for unsuffixed floating-point constants.

### 4.3 Boolean Literals

Boolean literals are always inferred as **`bool`**:

```c
let flag = true;      // bool
let done = false;     // bool
```

### 4.4 String Literals

String literals are inferred as **`string`**:

```c
let name = "C 2030";  // string
```

For raw byte strings or C-compatible strings, explicit typing or suffixes may be required (see RFC-0003 Section on string types).

### 4.5 Function Call Expressions

The inferred type is the **return type** of the called function:

```c
fn compute() -> f64 { return 3.14; }

let result = compute();   // f64
```

For functions returning `Result<T, E>`, the inferred type is `Result<T, E>`. When the `?` operator is applied, the inferred type is `T` (see Section 10).

### 4.6 Pointer Expressions

The address-of operator `&` produces a pointer type derived from its operand:

```c
var i32 n = 10;
let p = &n;           // i32* (specifically @borrowed i32*)
```

Pointer arithmetic and casts require explicit types.

### 4.7 Array Literals

An array literal infers both its element type and its length:

```c
let nums = [1, 2, 3];           // i32[3]
let floats = [1.0, 2.0];        // f64[2]
let bytes = [0_u8, 1_u8, 2_u8]; // u8[3]
```

All elements must have a common type. If element types conflict, the compiler emits an error rather than inserting implicit conversions:

```c
let bad = [1, 2.0];   // ERROR: conflicting element types i32 and f64
```

### 4.8 Struct Construction

When a struct is constructed by name, the type is inferred from the constructor:

```c
struct Point { f64 x; f64 y; }

let origin = Point { .x = 0.0, .y = 0.0 };  // Point
```

### 4.9 Generic Instantiation

Generic type parameters can be inferred from the arguments provided or from subsequent usage:

```c
let v = Vec::new();     // Vec<???> — type parameter pending
v.push(42);             // Vec<i32> resolved via bidirectional inference
```

When the generic type can be deduced from the constructor arguments:

```c
let pair = Pair::new(1, "hello");  // Pair<i32, string>
```

If the type parameter cannot be resolved by the end of the enclosing block, the compiler emits an error (see Section 7).

---

## 5. `let` and `var` with Inference

RFC-0015 introduced `let` (immutable) and `var` (mutable) bindings. Type inference integrates naturally with both:

```c
let x = 42;                    // i32, immutable
var y = 3.14;                  // f64, mutable
let buf = alloc<Buffer>(1)?;   // @owned Buffer*, immutable
let items = Vec::new();        // Vec<???> — needs usage to resolve
var count = 0_u64;             // u64, mutable
```

### 5.1 Explicit Type with `let`/`var`

Explicit types remain permitted and are sometimes necessary:

```c
let i64 big = 42;              // i64, not i32 (override default)
var f32 precise = 3.14;        // f32, not f64 (override default)
let u8[256] buffer = [0];      // explicit array type with fill
```

When an explicit type is provided, it takes precedence over what inference would deduce. The initializer must be compatible with the declared type.

### 5.2 Uninitialized Declarations

Declarations without initializers **cannot** use inference and must provide an explicit type:

```c
var i32 x;                     // OK: explicit type, no initializer
var x;                         // ERROR: no type, no initializer
let x;                         // ERROR: let requires initializer
```

---

## 6. Bidirectional Inference

Type information flows in two directions:

1. **Forward** — from the initializer expression to the variable's type
2. **Backward** — from usage context back to the declaration

### 6.1 Forward Inference

The standard case: the variable's type is determined by its initializer.

```c
let x = 42;          // forward: literal → i32
let y = compute();   // forward: return type → f64
```

### 6.2 Backward Inference (Contextual Resolution)

When a variable's type is not fully determined by its initializer, subsequent usage within the same function body can resolve pending type parameters:

```c
let items = Vec::new();   // type parameter unknown
items.push(42_i32);       // backward: argument type resolves Vec<i32>
```

Another example with generic functions:

```c
let result = parse(input);     // parse<T>(string) -> Result<T, Error>
let config: Config = result?;  // backward: Config resolves T
```

### 6.3 Resolution Ordering

The compiler processes inference in a **single forward pass** through the function body. When a type variable remains unresolved after its declaration, the compiler records a constraint and continues. Each subsequent use of the variable adds constraints. All constraints must be satisfied by the end of the enclosing block. If a type variable remains unresolved at the end of its scope, the compiler emits a diagnostic.

---

## 7. Inference Limitations

Type inference is not always possible. The compiler shall emit a clear error in the following cases:

### 7.1 Ambiguous Numeric Context

```c
let x = 42;
let y = x + 1_u64;   // ERROR: cannot add i32 and u64 without explicit cast
```

Resolution: provide an explicit type or use a suffix on the literal.

### 7.2 Empty Containers Without Usage

```c
let v = Vec::new();
// v is never used with a concrete type
return v;             // ERROR: unable to infer type parameter for Vec<?>
```

Resolution: provide an explicit type annotation.

```c
let Vec<i32> v = Vec::new();   // OK
```

### 7.3 Multiple Conflicting Constraints

```c
let x = 42;
foo(x);               // foo expects u32
bar(x);               // bar expects i64
// ERROR: conflicting type constraints on x (u32 vs i64)
```

Resolution: provide an explicit type and cast where necessary.

### 7.4 Function Parameter Types

Function parameters are **never** inferred, even when a default value is present:

```c
fn add(a, b) -> i32 { return a + b; }   // ERROR: parameters require types
fn add(i32 a, i32 b) -> i32 { return a + b; }  // OK
```

### 7.5 Recursive Definitions

A variable's initializer cannot reference the variable itself for type deduction:

```c
let x = x + 1;       // ERROR: x used in its own initializer
```

---

## 8. Interaction with Ownership (RFC-0004)

Ownership qualifiers are inferred from the expression's provenance:

### 8.1 Owned Pointers

```c
let buf = alloc<Buffer>(1)?;   // inferred: @owned Buffer*
```

The `alloc` family of functions is declared to return `@owned T*`. The ownership qualifier propagates to the variable.

### 8.2 Borrowed Pointers

```c
var i32 n = 42;
let p = &n;                    // inferred: @borrowed i32*
```

The address-of operator always produces a `@borrowed` pointer.

### 8.3 Function Return Annotations

When a function's return type carries an ownership qualifier, it flows into the inferred type:

```c
fn create_resource() -> @owned Resource* { ... }

let r = create_resource();     // inferred: @owned Resource*
// r must be freed or moved before scope exit
```

### 8.4 Ownership Transfer

```c
let a = alloc<Node>(1)?;      // @owned Node*
let b = a;                     // @owned Node* — ownership moves from a to b
// a is now invalid (use-after-move is a compile error)
```

The inferred type of `b` includes the ownership qualifier. The compiler tracks the move per RFC-0004 rules.

---

## 9. Interaction with Generics (RFC-0003)

Generic type parameters are inferred from call-site arguments using unification:

### 9.1 Generic Functions

```c
fn identity<T>(T x) -> T { return x; }

let y = identity(42);          // T = i32 inferred from argument
let z = identity("hello");     // T = string inferred from argument
```

### 9.2 Generic Structs

```c
fn pair<A, B>(A a, B b) -> Pair<A, B> { return Pair { .first = a, .second = b }; }

let p = pair(1, "two");        // Pair<i32, string>
```

### 9.3 Constrained Generics

When a generic parameter has trait/protocol bounds, inference must satisfy those bounds:

```c
fn sum<T: Numeric>(T a, T b) -> T { return a + b; }

let s = sum(1, 2);             // T = i32 (i32 satisfies Numeric)
let t = sum(1.0, 2.0);         // T = f64 (f64 satisfies Numeric)
let u = sum(1, 2.0);           // ERROR: no single T satisfies both i32 and f64
```

---

## 10. Interaction with Error Handling (RFC-0007)

### 10.1 Result Type Inference

Functions returning `Result<T, E>` infer naturally:

```c
fn read_file(string path) -> Result<string, IoError> { ... }

let result = read_file(path);  // Result<string, IoError>
```

### 10.2 The `?` Operator and Inference

The `?` operator unwraps the `Ok` variant, so the inferred type is the success type:

```c
let data = read_file(path)?;   // string (unwrapped from Result)
```

This is equivalent to:

```c
let string data = match read_file(path) {
    Ok(val) => val,
    Err(e)  => return Err(e),
};
```

### 10.3 Error Type Propagation

The enclosing function's return type constrains which error types can be propagated via `?`:

```c
fn load_config(string path) -> Result<Config, AppError> {
    let text = read_file(path)?;       // IoError must convert to AppError
    let cfg = parse_config(text)?;     // ParseError must convert to AppError
    return Ok(cfg);
}
```

---

## 11. Algorithm

### 11.1 Overview

C 2030 uses a **Hindley-Milner style local inference** algorithm with the following characteristics:

1. **Local scope only** — inference does not cross function boundaries
2. **Single forward pass** — the function body is processed top-to-bottom
3. **Deferred resolution** — unresolved type variables are recorded as constraints and resolved lazily
4. **Unification** — when two type expressions must be equal, they are unified; failure produces a type error

### 11.2 Process

1. For each `let`/`var` declaration without an explicit type, generate a fresh type variable `?T`
2. Process the initializer expression. If it produces a concrete type, unify `?T` with that type
3. If the initializer involves generics, record type parameter constraints
4. Continue processing subsequent statements. Each use of the variable adds constraints to `?T`
5. At the end of the enclosing block, verify all type variables are resolved to concrete types
6. If any `?T` remains unresolved, emit a diagnostic

### 11.3 No Global Inference

The algorithm does **not** perform any of the following:

* Inferring function parameter types from call sites
* Inferring return types from function bodies
* Propagating type information across module boundaries
* Iterative fixed-point computation across multiple functions

This keeps compilation fast and predictable, with clear error messages localized to the function in question.

---

## 12. Compiler Diagnostics

The compiler shall produce clear, actionable diagnostics for inference failures:

### 12.1 Unable to Infer Type

```
error[E0301]: unable to infer type for `items`
  --> src/main.c2030:12:9
   |
12 |     let items = Vec::new();
   |         ^^^^^ type annotation needed
   |
   = help: consider giving `items` an explicit type: `let Vec<i32> items = Vec::new();`
```

### 12.2 Conflicting Type Constraints

```
error[E0302]: conflicting type constraints on `x`
  --> src/main.c2030:15:5
   |
13 |     let x = 42;
   |         - inferred as i32 here
14 |     foo(x);
   |         - used as u32 here (foo expects u32)
15 |     bar(x);
   |         - used as i64 here (bar expects i64)
   |
   = note: provide an explicit type for `x` to resolve the ambiguity
```

### 12.3 Ambiguous Numeric Type

```
error[E0303]: ambiguous numeric type for literal `42`
  --> src/main.c2030:8:13
   |
 8 |     let x = 42;
   |             ^^ cannot determine type from context
   |
   = help: add a type suffix: `42_i32`, `42_u64`, etc.
   = note: this error occurs when the literal is used in contexts expecting different types
```

---

## 13. Examples

### 13.1 Complete Function with Mixed Inference

```c
fn process_data(string path, u64 max_items) -> Result<Summary, AppError> {
    // Inferred types
    let data = read_file(path)?;               // string
    let items = parse_items(data)?;            // Vec<Item>
    let count = items.len();                   // u64
    var total = 0.0;                           // f64
    var processed = 0_u64;                     // u64

    // Explicit types where needed
    let f64 threshold = 0.95;

    for (let item in items) {                  // Item (inferred from Vec<Item>)
        if (processed >= max_items) {
            break;
        }
        let score = item.compute_score();      // f64
        if (score > threshold) {
            total += score;
            processed += 1;
        }
    }

    // Struct construction — type inferred
    let summary = Summary {
        .total = total,
        .count = processed,
        .average = total / (processed as f64),
    };

    return Ok(summary);
}
```

### 13.2 Generic Container Usage

```c
fn build_index(@borrowed Entry[*] entries) -> HashMap<string, Vec<u64>> {
    var index = HashMap::new();                // HashMap<string, Vec<u64>> from return type

    for (let entry in entries) {               // Entry
        let key = entry.name.clone();          // string
        let id = entry.id;                     // u64

        if (!index.contains_key(&key)) {
            index.insert(key, Vec::new());     // Vec<u64> inferred from map type
        } else {
            index.get_mut(&key).push(id);
        }
    }

    return index;
}
```

---

## 14. Rejected Alternatives

### 14.1 Global Type Inference

Languages like Haskell and ML perform whole-program type inference. This was rejected for C 2030 because:

* It is incompatible with separate compilation and C ABI requirements
* Error messages become non-local and difficult to understand
* It encourages omitting type annotations on public interfaces
* Compilation times would increase significantly for large codebases

### 14.2 `auto` Keyword

C++ uses `auto` for type inference. C 2030 rejected this approach because:

* `auto` in legacy C had a different meaning (storage class specifier)
* Reusing the keyword would create confusion for C programmers migrating to C 2030
* `let`/`var` already serve the declaration role and naturally accommodate inference
* `auto` does not convey mutability intent; `let`/`var` do

### 14.3 No Default Types for Literals

An alternative design would require all literals to be suffixed. This was rejected because:

* It would make C 2030 more verbose than C for simple cases
* `i32` and `f64` are the overwhelmingly common cases in practice
* C already has implicit `int` and `double` defaults for literals
* Pragmatism: `let x = 42;` should just work without ceremony

### 14.4 Return Type Inference

Some languages (e.g., Rust with `-> impl Trait`, or Kotlin) allow return types to be inferred. This was rejected because:

* Function signatures are API contracts and must be explicit
* Inferred return types break ABI stability
* They make documentation generation unreliable
* C interop requires concrete types at all boundaries

---

## 15. Open Questions

1. **Should inference extend to `for` loop variables?** — Currently proposed as yes (see Section 13.1), but this needs further review for interaction with iterators.

2. **Integer literal promotion rules** — Should `let x = 42; let y = x + 1_u64;` silently promote `x` to `u64`, or require an explicit cast? The current proposal requires a cast.

3. **Inference across `match` arms** — When all arms of a `match` return the same type, should the compiler infer that as the result type? Proposed yes, but edge cases with generic arms need specification.

4. **Partial type annotations** — Should `let Vec<?> items = Vec::new();` be valid syntax where `?` is a wildcard for "infer this parameter"? This could be useful for complex generic types where only some parameters are ambiguous.

5. **Interaction with `typeof` operator** — If C 2030 adopts a `typeof` operator (as in GCC extensions), how does it interact with inferred types?

---

## 16. References

1. **RFC-0003** — C 2030 Type System & Layout Rules
2. **RFC-0004** — C 2030 Ownership, Lifetimes, and Memory Safety
3. **RFC-0007** — C 2030 Error Handling
4. **RFC-0015** — C 2030 Swift-Inspired Syntax Refinements (`let`/`var`)
5. **Hindley, R. (1969)** — "The Principal Type-Scheme of an Object in Combinatory Logic." *Transactions of the American Mathematical Society*, 146, 29-60.
6. **Milner, R. (1978)** — "A Theory of Type Polymorphism in Programming." *Journal of Computer and System Sciences*, 17(3), 348-375.
7. **Rust Reference: Type Inference** — https://doc.rust-lang.org/reference/type-system.html
8. **Swift Programming Language: Type Safety and Type Inference** — https://docs.swift.org/swift-book/documentation/the-swift-programming-language/thebasics/#Type-Safety-and-Type-Inference
9. **Pierce, B. C. (2002)** — *Types and Programming Languages*. MIT Press. Chapters 22 (Type Reconstruction).

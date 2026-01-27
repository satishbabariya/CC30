# RFC 0011

## Language Tooling & Static Analysis

**Status:** Draft
**Category:** Language Infrastructure
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-01-27

---

## 1. Abstract

C 2030 introduces **first-class compiler and tooling support** to enforce:

* Ownership & lifetime correctness (RFC 0004)
* Undefined behavior prevention (RFC 0005)
* Module encapsulation (RFC 0006)
* Explicit error handling (RFC 0007)
* Safe concurrency & atomic access (RFC 0008)
* Memory safety and allocator usage (RFC 0009)

Tooling includes:

* **Compiler front-end checks**
* **Static analysis and linting**
* **Module DAG validation**
* **Error propagation auditing**
* **Integration with IDEs for code navigation and refactoring**

---

## 2. Motivation

C 2030 introduces modern features that **cannot be verified by traditional compilers**:

* Ownership violations
* Lifetime misuse across threads
* Improper `Result` error propagation
* Unsafe memory access
* Module import/export mismatches

Without tooling, developers would:

* Still introduce dangling pointers or leaks
* Have subtle concurrency bugs
* Risk undefined behavior
* Lose performance due to over-cautious patterns

The compiler and tooling must **catch violations at compile time** wherever possible.

---

## 3. Compiler Enforcement

### 3.1 Ownership & Lifetime Checker

* Tracks **@owned, @borrowed, and raw pointers**
* Ensures **exclusive ownership**, **no double frees**, **no dangling references**
* Integrates with **defer** and **Result** propagation

Example:

```c
fn process(@owned Buffer* buf) -> Result<(), MemError> {
    let slice = &buf->data; // compiler ensures slice does not outlive buf
    do_work(slice)?;
    return Ok(());
}
```

* Violating lifetime constraints → compile-time error

---

### 3.2 Undefined Behavior Traps

* Compiler instruments **trap instructions** for:

  * Null dereference
  * Out-of-bounds access
  * Use-after-free
  * Integer overflow (optional)
  * Misaligned memory access

* Safe code **cannot compile if UB is possible**

* Unsafe code can compile with warnings

---

### 3.3 Error Propagation Enforcement

* Functions returning `Result<T,E>` **must** handle errors via:

  * `?` operator
  * `match` exhaustiveness

* Ignored errors → **compile-time warning/error**

Example:

```c
fn read_file(@owned string path) -> Result<array<u8>, IOError> {
    let buf = alloc<u8>(1024)?; // must handle error
    return Ok(buf);
}
```

---

### 3.4 Module DAG Validation

* Compiler builds **module dependency graph (DAG)**
* Cyclic imports → **compile-time error**
* Ensures:

  * Correct import/export visibility
  * Deterministic incremental compilation
  * Tooling can track API changes efficiently

---

### 3.5 Concurrency Safety Checks

* Ensures **data race freedom** in safe code:

  * Only atomic variables may be shared
  * Borrowed references cannot escape threads
  * Scoped threads respect lifetime boundaries

* Unsafe code emits **audit warnings** for potential races

---

## 4. Static Analysis & Linting

### 4.1 Built-in Linters

* **Ownership violations**
* **Dangling borrows**
* **Unchecked Result propagation**
* **Unsafe memory access patterns**
* **Deadlocks and improper lock nesting**

### 4.2 Optional Checks

* Performance anti-patterns (e.g., unnecessary allocations)
* Macro leakage in legacy C imports
* Unused exports or private symbols
* Inefficient atomic usage

---

## 5. IDE & Developer Tooling

### 5.1 Code Navigation

* Module-aware symbol resolution
* Jump-to-definition across modules
* Exported vs private symbols highlighted

### 5.2 Refactoring Support

* Safe rename of symbols in module
* Safe extraction of functions maintaining ownership correctness
* Automatic error propagation insertion (`?`)

### 5.3 Real-time Diagnostics

* Ownership and lifetime warnings during editing
* Data race and UB detection
* Inline documentation and type hints

---

## 6. Build System Integration

* Module DAG drives **incremental compilation**
* Only affected modules recompiled
* Compiler produces **dependency graphs for CI**

```text
module mm.page -> mm.vma -> kernel.main
```

* Enables caching of **interface-only changes**

---

## 7. Runtime Instrumentation

* Optional debug builds emit:

  * Ownership/borrow logging
  * Reference count tracking
  * Allocation/deallocation statistics
  * Thread creation and join events

* Compatible with kernel, embedded, and user-space applications

---

## 8. Legacy C Integration

* Legacy C code imported as `unsafe` modules
* Compiler analyzes:

  * Macro pollution
  * Function signatures against exported interfaces
  * Potential lifetime issues for pointers passed into safe code

---

## 9. Example: Compiler Diagnostics

```c
fn example(@owned Buffer* buf) -> Result<(), MemError> {
    let slice = &buf->data;
    spawn_scoped(|| {
        use(slice); // ❌ compile-time error: slice may outlive buf
    });
    return Ok(());
}
```

Compiler emits:

```
error[E-LT-001]: borrowed reference `slice` escapes thread lifetime
note: use `@owned` or copy to thread-local buffer
```

---

## 10. Comparison Table

| Feature                        | C 2030 | C  | C++ | Rust |
| ------------------------------ | ------ | -- | --- | ---- |
| Ownership checking             | ✅      | ❌  | ⚠️  | ✅    |
| Lifetime analysis              | ✅      | ❌  | ⚠️  | ✅    |
| Module DAG validation          | ✅      | ❌  | ⚠️  | ✅    |
| Result propagation enforcement | ✅      | ❌  | ⚠️  | ✅    |
| Thread-safety checking         | ✅      | ❌  | ⚠️  | ✅    |
| UB trap instrumentation        | ✅      | ⚠️ | ⚠️  | ✅    |

---

## 11. Summary

C 2030 tooling is **integral to the language**, providing:

* Compile-time **ownership & lifetime enforcement**
* **UB detection** for safe code
* **Exhaustive error propagation enforcement**
* **Module DAG validation** for predictable builds
* **Data race and concurrency auditing**
* IDE integration for navigation, refactoring, and real-time diagnostics

Together, compiler and tooling guarantee that **C 2030’s modern language features are safe, auditable, and efficient**, enabling developers to confidently migrate complex codebases, such as the Linux kernel, to C 2030.

---

Next steps (future RFCs):

* **RFC 0012 — ABI & Linker Contracts**
* **RFC 0013 — Optional Runtime Diagnostics**
* **RFC 0014 — Language Evolution & Versioning**


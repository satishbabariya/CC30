# RFC 0016

## Compile-Time Execution (`comptime`)

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

This RFC specifies the **compile-time execution model** (`comptime`) for C 2030. `comptime` enables arbitrary computation during compilation, replacing the C preprocessor for constants, conditional compilation, and code generation while providing full type safety and determinism.

`comptime` was listed as a keyword in RFC-0002 §5.5 and referenced in RFC-0001 as a preprocessor replacement, but was never formally specified. This RFC fills that gap.

---

## 2. Motivation

The C preprocessor is responsible for:

* Constants (`#define PAGE_SIZE 4096`)
* Conditional compilation (`#ifdef DEBUG`)
* Macro functions (`#define MAX(a,b) ((a)>(b)?(a):(b))`)
* Include guards (`#ifndef HEADER_H`)
* Stringification and token pasting

These are powerful but fundamentally **untyped, unhygienic, and error-prone**. C++ `constexpr` addresses some issues but is limited in scope and cannot replace conditional compilation.

C 2030 `comptime` provides:

* **Type-safe** compile-time computation
* **Full language subset** available at compile time
* **Deterministic** and **platform-independent** results
* **Code generation** through compile-time loops
* **Type computation** — functions that return types
* **Complete replacement** for `#define`, `#if`, `#ifdef`

---

## 3. Core Design Principles

1. **Determinism** — `comptime` evaluation must produce identical results across all conforming compilers and platforms
2. **Type safety** — All `comptime` expressions are fully typed
3. **Subset execution** — `comptime` runs a restricted subset of the language (no I/O, no heap, no unsafe)
4. **Transparency** — `comptime` results are inlined as constants; no runtime cost
5. **Composability** — `comptime` functions can call other `comptime` functions

---

## 4. `comptime` Constants

### 4.1 Syntax

```c
comptime usize PAGE_SIZE = 4096;
comptime usize PAGE_MASK = PAGE_SIZE - 1;
comptime f64 PI = 3.14159265358979323846;
comptime bool ENABLE_LOGGING = true;
```

### 4.2 Rules

* `comptime` variables must be initialized with expressions evaluable at compile time
* They are **immutable** — assignment after initialization is forbidden
* They are **inlined** at every use site — no runtime storage allocated
* They may be `export`ed from modules

### 4.3 Replaces `#define` Constants

```c
// C (old)
#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE - 1)

// C 2030
comptime usize PAGE_SIZE = 4096;
comptime usize PAGE_MASK = PAGE_SIZE - 1;
```

---

## 5. `comptime` Functions

### 5.1 Syntax

```c
comptime fn max(i32 a, i32 b) -> i32 {
    if (a > b) return a;
    return b;
}

comptime fn factorial(u64 n) -> u64 {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
```

### 5.2 Rules

* `comptime fn` must be evaluable entirely at compile time
* May call other `comptime fn` functions
* May use control flow: `if`, `else`, `match`, `for`, `while`
* May use local variables (stack-allocated within the compile-time evaluator)
* **Must not**: perform I/O, allocate heap memory, use `unsafe`, call non-`comptime` functions, access global mutable state

### 5.3 Recursion Limit

* Default recursion depth: **256**
* Configurable via compiler flag: `--comptime-recursion-limit=N`
* Exceeding the limit is a compile-time error

### 5.4 Iteration Limit

* Default iteration limit: **65536** steps per `comptime` evaluation
* Prevents infinite loops at compile time
* Configurable via compiler flag: `--comptime-iteration-limit=N`

---

## 6. `comptime` Parameters

### 6.1 Syntax

Functions can accept `comptime` parameters that must be known at compile time:

```c
fn create_buffer(comptime usize N) -> u8[N] {
    var u8[N] buf;
    for (var usize i = 0; i < N; i++) {
        buf[i] = 0;
    }
    return buf;
}

// Usage:
let buf = create_buffer(1024);  // N = 1024, resolved at compile time
```

### 6.2 Rules

* `comptime` parameters must be provided as compile-time-known values
* The function is **monomorphized** for each unique set of `comptime` arguments
* `comptime` parameters can be used in type positions (e.g., array sizes)
* Non-`comptime` functions may have `comptime` parameters — only the parameter is evaluated at compile time

### 6.3 Interaction with Generics (RFC-0003)

`comptime` parameters and generic type parameters serve different purposes:

```c
// Generic: works with any type T
fn swap<T>(T* a, T* b);

// Comptime: array size known at compile time
fn zero_array(comptime usize N) -> i32[N];

// Both: generic type with comptime size
fn create_array<T>(comptime usize N) -> T[N];
```

* **Generics** abstract over types
* **`comptime`** abstracts over values known at compile time
* Both trigger monomorphization

---

## 7. `comptime` Blocks

### 7.1 Syntax

```c
comptime {
    // Code executed at compile time
    // Results can be used as constants
}
```

### 7.2 Use Cases

**Lookup table generation:**

```c
comptime fn build_crc_table() -> u32[256] {
    var u32[256] table;
    for (var usize i = 0; i < 256; i++) {
        var u32 crc = (u32)i;
        for (var usize j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
        table[i] = crc;
    }
    return table;
}

comptime u32[256] CRC_TABLE = build_crc_table();
```

**Compile-time assertions:**

```c
comptime {
    static_assert(sizeof(Header) == 16, "Header must be 16 bytes");
    static_assert(PAGE_SIZE >= 4096, "Minimum page size is 4K");
}
```

---

## 8. Type Computation

### 8.1 `type` as a First-Class Compile-Time Value

`comptime` functions can accept and return types:

```c
comptime fn select_int_type(bool wide) -> type {
    if (wide) return i64;
    return i32;
}

type Counter = select_int_type(USE_WIDE_COUNTERS);
```

### 8.2 Rules

* `type` is a valid value **only** in `comptime` context
* Types cannot exist at runtime — they are resolved during compilation
* Type computation enables conditional type selection without preprocessor

### 8.3 Examples

```c
// Platform-specific pointer size type
comptime fn ptr_int() -> type {
    if (sizeof(void*) == 8) return u64;
    return u32;
}

type uptr = ptr_int();

// Conditional struct layout
comptime fn alignment_for(type T) -> usize {
    if (sizeof(T) >= 64) return 64;
    if (sizeof(T) >= 16) return 16;
    return 8;
}
```

---

## 9. Compile-Time Loops (Code Generation)

### 9.1 Syntax

```c
comptime for (var usize i = 0; i < N; i++) {
    // Body is emitted N times with i substituted
}
```

### 9.2 Examples

**Register initialization:**

```c
comptime fn init_registers() {
    comptime for (var usize i = 0; i < 16; i++) {
        register_handler(i, default_handler);
    }
}
```

**Struct field iteration (future):**

```c
// Note: Full reflection is an open question (§16)
comptime for (field in fields_of(MyStruct)) {
    printf("Field: %s\n", field.name);
}
```

### 9.3 Unrolling

`comptime for` loops are **always fully unrolled** — the loop does not exist at runtime.

---

## 10. Conditional Compilation

### 10.1 Built-In Compile-Time Constants

```c
comptime Architecture ARCH;      // x86_64, aarch64, riscv64, ...
comptime OS TARGET_OS;           // linux, none (bare-metal), ...
comptime bool IS_DEBUG;          // true in debug builds
comptime bool IS_RELEASE;        // true in release builds
comptime usize POINTER_SIZE;    // 4 or 8
comptime Endian BYTE_ORDER;     // little, big
```

### 10.2 Conditional Compilation Syntax

```c
comptime if (ARCH == Architecture::x86_64) {
    fn fast_memcpy(@borrowed u8* dst, @borrowed u8* src, usize len) {
        // x86-specific SIMD implementation
    }
} else if (ARCH == Architecture::aarch64) {
    fn fast_memcpy(@borrowed u8* dst, @borrowed u8* src, usize len) {
        // ARM NEON implementation
    }
} else {
    fn fast_memcpy(@borrowed u8* dst, @borrowed u8* src, usize len) {
        // Generic fallback
    }
}
```

### 10.3 Replaces `#if` / `#ifdef`

```c
// C (old)
#ifdef CONFIG_SMP
    void smp_init(void);
#endif

// C 2030
comptime if (CONFIG.smp) {
    export fn smp_init();
}
```

---

## 11. Interaction with Modules (RFC-0006)

* `comptime` constants and functions can be `export`ed
* Module-level `comptime` blocks run during module compilation
* Imported `comptime` values are available for use in importing module's `comptime` context

```c
module config;

export comptime usize MAX_CPUS = 256;
export comptime fn cache_line_size() -> usize {
    if (ARCH == Architecture::x86_64) return 64;
    if (ARCH == Architecture::aarch64) return 128;
    return 64;
}
```

```c
module scheduler;
import config;

comptime usize SCHED_SLOTS = config::MAX_CPUS * 4;
```

---

## 12. Restrictions Summary

| Allowed in `comptime` | Forbidden in `comptime` |
|----------------------|------------------------|
| Arithmetic, logic | I/O (file, network, console) |
| Control flow (if, match, for, while) | Heap allocation |
| Local variables | `unsafe` blocks |
| Function calls to other `comptime fn` | Calls to non-`comptime` functions |
| Type computation | Global mutable state access |
| Array/struct construction | Pointer arithmetic |
| String literals | System calls |
| `static_assert` | Thread creation |

---

## 13. Compiler Diagnostics

### 13.1 Non-`comptime` Call in `comptime` Context

```
error[E1601]: cannot call non-comptime function 'read_file' in comptime context
  --> src/config.c30:15:20
   |
15 |     comptime let data = read_file("config.txt");
   |                         ^^^^^^^^^ this function performs I/O
   |
   = note: comptime functions cannot perform I/O
   = help: read the file at runtime instead
```

### 13.2 Recursion Limit Exceeded

```
error[E1602]: comptime recursion limit (256) exceeded
  --> src/math.c30:8:12
   |
 8 |     return n * factorial(n - 1);
   |            ^^^^^^^^^^^^^^^^^^^^
   |
   = note: the default limit is 256; use --comptime-recursion-limit=N to increase
```

### 13.3 Iteration Limit Exceeded

```
error[E1603]: comptime iteration limit (65536) exceeded
  --> src/gen.c30:12:5
   |
12 |     while (true) { ... }
   |     ^^^^^ infinite loop detected during comptime evaluation
```

---

## 14. Complete Examples

### 14.1 Type-Safe Format String

```c
comptime fn count_format_args(string fmt) -> usize {
    var usize count = 0;
    for (var usize i = 0; i < fmt.length; i++) {
        if (fmt[i] == '%' && i + 1 < fmt.length && fmt[i + 1] != '%') {
            count += 1;
        }
    }
    return count;
}

fn printf_checked(comptime string fmt, ...) {
    comptime {
        static_assert(
            count_format_args(fmt) == ARG_COUNT,
            "format string argument count mismatch"
        );
    }
    // ... actual printf implementation
}
```

### 14.2 Kernel Configuration

```c
module kernel.config;

// These are set via compiler flags: -D CONFIG_SMP=true -D CONFIG_PREEMPT=false
export comptime bool CONFIG_SMP = true;
export comptime bool CONFIG_PREEMPT = false;
export comptime usize CONFIG_NR_CPUS = 64;
export comptime usize CONFIG_HZ = 1000;

export comptime fn tick_period_ns() -> u64 {
    return 1000000000 / CONFIG_HZ;
}
```

### 14.3 Compile-Time Hash Map (Lookup Table)

```c
comptime fn perfect_hash(string[] keys) -> u32[256] {
    var u32[256] table;
    // Generate perfect hash at compile time
    for (var usize i = 0; i < keys.length; i++) {
        let h = hash(keys[i]) % 256;
        table[h] = (u32)i;
    }
    return table;
}

comptime string[4] KEYWORDS = ["if", "else", "for", "while"];
comptime u32[256] KEYWORD_HASH = perfect_hash(KEYWORDS);
```

---

## 15. Rejected Alternatives

### 15.1 C++ `constexpr` / `consteval`

* Too limited — cannot do conditional compilation, type computation, or code generation
* Incrementally bolted onto C++ over 5 standards — inconsistent semantics
* C 2030 provides a unified `comptime` mechanism

### 15.2 Zig-style `comptime` (Implicit)

* Zig's approach is powerful but **too implicit** — any function can be called at compile time if its inputs are known
* C 2030 requires explicit `comptime` annotation for clarity and auditability
* Explicit marking makes it clear which code runs when

### 15.3 Full Interpreter

* Running arbitrary code (including I/O) at compile time creates reproducibility issues
* Build results would depend on filesystem state, network, etc.
* C 2030 restricts `comptime` to a pure, deterministic subset

---

## 16. Open Questions

1. **Compile-time string manipulation** — Should `comptime` support string concatenation, formatting, and slicing? (Likely yes, with restrictions)
2. **Compile-time allocation arena** — Should `comptime` have a temporary allocator for complex data structures during evaluation? (Useful for lookup tables)
3. **Compile-time reflection** — Should `comptime` be able to inspect struct fields, function signatures, and module contents? (Powerful but complex; may warrant a separate RFC)
4. **`comptime` in generic constraints** — Should `comptime` expressions be usable in `where` clauses?
5. **Cross-compilation** — When `comptime` queries platform properties (pointer size, endianness), it must use the **target** platform, not the host

---

## 17. References

* RFC-0002: Lexical Grammar (§5.5 — `comptime` keyword)
* RFC-0001: Overview (§5.5 — preprocessor replacement)
* RFC-0006: Modules & Visibility (conditional compilation)
* Zig Programming Language — [Comptime](https://ziglang.org/documentation/master/#comptime)
* C++ Standard — `constexpr` (C++11), `consteval` (C++20), `constinit` (C++20)
* D Programming Language — [CTFE](https://dlang.org/spec/function.html#interpretation)

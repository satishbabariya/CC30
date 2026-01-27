# Feature Specification: C 2030 Core Language

**Feature Branch**: `core-init`
**Created**: 2026-01-27
**Status**: Draft
**Input**: RFCs 0001-0014

## User Scenarios & Testing

### User Story 1 - Kernel Developer Migration (Priority: P1) (RFC 0010)
As a Linux Kernel developer, I want to migrate a subsystem (e.g., VFS) to C 2030 so that I can eliminate undefined behavior and memory leaks while maintaining performance.
**Why this priority**: The primary driver for C 2030 is safe systems programming (RFC 0010).
**Acceptance Scenarios**:
1.  **Given** a C 2030 module with `@owned` pointers, **When** I attempt to use a pointer after it is freed, **Then** the compiler emits a compile-time error.
2.  **Given** a C 2030 module importing legacy C headers, **When** I call a C function, **Then** I must use an `unsafe` block or a safe wrapper.
3.  **Given** an arena allocator, **When** I allocate memory within a scoped request handler, **Then** all memory is automatically freed when the arena is reset/dropped.

### User Story 2 - Safe Concurrency (Priority: P2) (RFC 0008)
As a systems programmer, I want to write multi-threaded applications without data races so that I can ensure reliability in high-concurrency environments.
**Acceptance Scenarios**:
1.  **Given** a shared mutable variable, **When** I access it from two threads without synchronization (`atomic` or `Mutex`), **Then** the compiler emits an error.
2.  **Given** a `spawn`ed thread, **When** I try to capture a reference that outlives the thread, **Then** the compiler prevents it (lifetime error).

### User Story 3 - Modular Build (Priority: P3) (RFC 0006)
As a library author, I want to expose a clean public API using modules instead of headers so that my library is encapsulated and builds faster.
**Acceptance Scenarios**:
1.  **Given** a module with `private` helper functions, **When** I import it in another file, **Then** I cannot call the private functions.
2.  **Given** a cyclic dependency between modules, **When** I compile, **Then** the compiler reports an error at the module DAG stage.

---

## Requirements

### RFC 0001: Overview & Goals
- **FR-0001.1**: The language MUST be a strict superset of C semantics where possible, optimizing for performance and hardware control.
- **FR-0001.2**: The language MUST NOT include a garbage collector or mandatory runtime.
- **FR-0001.3**: The language MUST compile to efficient machine code with zero-cost abstractions for safe features.

### RFC 0002: Lexical Structure
- **FR-0002.1**: Source encoding MUST be UTF-8.
- **FR-0002.2**: Identifiers MUST be ASCII-only for compatibility and tooling safety.
- **FR-0002.3**: Keywords MUST be context-free where possible. Reserved keywords include `fn`, `struct`, `module`, `import`, `export`, `match`.
- **FR-0002.4**: Literals MUST support suffixes (`u8`, `i32`, `f64`) and bases (0x, 0b, 0o).

### RFC 0003: Type System
- **FR-0003.1**: Primitive types MUST be explicit in size: `i8`..`i64`, `u8`..`u64`, `f32`, `f64`, `bool`.
- **FR-0003.2**: Pointers MUST correspond to `T*` but carry compile-time metadata for ownership.
- **FR-0003.3**: Arrays `T[N]` MUST be fixed-size and value types. Slices `array<T>` MUST consist of a pointer and length.
- **FR-0003.4**: Structs MUST use C-compatible layout by default. `enum` MUST support byte-sized discriminants. Tagged unions `Result<T,E>` MUST be supported.
- **FR-0003.5**: No implicit integer widening or unsafe casts without `unsafe` blocks or explicit casts.

### RFC 0004: Ownership & Lifetimes
- **FR-0004.1**: Pointer types MUST support annotations: `@owned`, `@borrowed`, `@nullable`.
- **FR-0004.2**: The compiler MUST enforce that `@owned` pointers have exactly one owner and are moved, not copied.
- **FR-0004.3**: The compiler MUST enforce that `@borrowed` pointers do not outlive their referent.
- **FR-0004.4**: Deep copies MUST be explicit (`.clone()`).
- **FR-0004.5**: `defer` statement MUST be guaranteed to execute at scope exit for resource cleanup.

### RFC 0005: Undefined Behavior (UB) Policy
- **FR-0005.1**: In `safe` code, the compiler MUST insert traps (aborts) for:
    - Null pointer dereference
    - Out-of-bounds array access
    - Use-after-free (if not caught statically)
    - Signed integer overflow
- **FR-0005.2**: `unsafe` blocks MUST be required for operations that can cause UB (e.g., raw pointer arithmetic, FFI).
- **FR-0005.3**: UB MUST be deterministically defined (e.g., panics) in safe mode.

### RFC 0006: Modules & Visibility
- **FR-0006.1**: Compilation units MUST be defined by `module <name>`.
- **FR-0006.2**: `import <module>` MUST replace `#include`. Cyclic imports are forbidden.
- **FR-0006.3**: Symbols are `private` by default. `export` keyword is REQUIRED to make symbols visible to importers.
- **FR-0006.4**: Macros MUST be module-scoped and not leak unless explicitly exported (discouraged).

### RFC 0007: Error Handling
- **FR-0007.1**: Exceptions (stack unwinding) MUST NOT be supported.
- **FR-0007.2**: All fallible functions MUST return `Result<T, E>`.
- **FR-0007.3**: The `?` operator MUST be implemented to propagate errors.
- **FR-0007.4**: Unused `Result` return values MUST trigger a compile-time warning/error.

### RFC 0008: Concurrency & Atomics
- **FR-0008.1**: The language MUST support `spawn` for thread creation with ownership transfer of captures.
- **FR-0008.2**: `atomic<T>` types MUST be provided with explicit memory ordering (Relaxed, SeqCst).
- **FR-0008.3**: Data races in `safe` code MUST be treated as a compile-time error.
- **FR-0008.4**: Thread-local storage (`thread_local`) MUST be supported.

### RFC 0009: Standard Library & Allocators
- **FR-0009.1**: The standard library MUST be ownership-aware. `Vec<T>`, `String`, `HashMap` must manage their own memory.
- **FR-0009.2**: Allocators MUST be pluggable. `alloc<T>` and `dealloc<T>` primitives MUST be provided.
- **FR-0009.3**: Arena allocators MUST be supported for region-based memory management (critical for kernels).

### RFC 0010: Kernel Migration
- **FR-0010.1**: The compiler MUST support "freestanding" mode (no OS).
- **FR-0010.2**: Compilation MUST allow mixing C 2030 modules with legacy C objects.
- **FR-0010.3**: `unsafe` escape hatches MUST be sufficient to implement low-level drivers.

### RFC 0011: Tooling & Static Analysis
- **FR-0011.1**: The compiler MUST include a borrow checker.
- **FR-0011.2**: A Language Server (LSP) MUST be capable of resolving module imports and ownership information.
- **FR-0011.3**: Static analysis MUST detect potential concurrency deadlocks where possible.

### RFC 0012: ABI & Linker Contracts
- **FR-0012.1**: The ABI MUST be stable and documented for `safe` functions.
- **FR-0012.2**: The compiler MUST support `extern "C"` for legacy C function calls (using System V ABI).
- **FR-0012.3**: Mangling schemes MUST include module names to prevent collisions.

### RFC 0013: Runtime Diagnostics
- **FR-0013.1**: Debug builds MUST allow optional instrumentation for allocation tracking (leak detection).
- **FR-0013.2**: Sanitizers (Address, Thread) integration properties MUST be emitted in debug mode.

### RFC 0014: Language Evolution
- **FR-0014.1**: Breaking changes MUST be gated behind `edition` flags (e.g., `edition = "2030"`).
- **FR-0014.2**: Deprecation warnings MUST be supported.

## Success Criteria

### Measurable Outcomes

- **SC-001**: **Self-Hosting**: The compiler can eventually compile itself (long-term).
- **SC-002**: **Kernel Module**: A strictly safe C 2030 implementation of a Linux driver (e.g., standard `e1000` network driver) runs without crashing.
- **SC-003**: **Performance**: Generated code matches C performance within 5% for standard benchmarks (when runtime checks are disabled).
- **SC-004**: **Safety**: The test suite demonstrates 100% catch rate for synthetic memory safety violations in `safe` mode.

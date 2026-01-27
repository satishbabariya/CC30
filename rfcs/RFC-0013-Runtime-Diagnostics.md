# RFC 0013

## Optional Runtime Diagnostics & Profiling

**Status:** Draft
**Category:** Tooling & Runtime
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-01-27

---

## 1. Abstract

C 2030 introduces **optional runtime diagnostics** to:

* Audit ownership and lifetimes
* Detect undefined behavior in debug builds
* Log allocator usage and performance
* Profile concurrency, thread usage, and atomic operations
* Provide actionable metrics for kernel and embedded systems

These features are **optional**, **zero-cost in safe code** when disabled, and fully integrated with:

* Ownership & lifetimes (RFC 0004)
* Undefined behavior detection (RFC 0005)
* Concurrency primitives (RFC 0008)
* Allocators and containers (RFC 0009)

---

## 2. Motivation

Compile-time checks catch most errors, but **runtime events** still need observability:

* Ownership violations in `unsafe` blocks
* Thread races not detected statically
* Allocation/deallocation hotspots
* Error handling patterns and propagation
* Kernel subsystems and embedded memory usage

Runtime diagnostics allow developers to **monitor, debug, and optimize** without changing language semantics.

---

## 3. Diagnostics Modes

1. **Development Mode**

   * Full runtime checks
   * UB traps, ownership validation, allocation logging
   * Detailed thread and atomic monitoring

2. **Production Mode**

   * Lightweight logging
   * Only performance-critical checks
   * Zero-cost for safe code where diagnostics are disabled

3. **Embedded Mode**

   * Minimal runtime footprint
   * Optional scoped allocation or arena tracking
   * No heap overhead if disabled

---

## 4. Ownership & Lifetime Auditing

* `@owned` and `@borrowed` pointers instrumented with **metadata**:

  * Allocation site
  * Lifetime scope
  * Module of origin

* Violations trigger **runtime traps** in development mode:

```c
@owned Buffer* buf = alloc<u8>(1024)?;
// Ownership moved incorrectly
@borrowed Buffer* alias = buf; // runtime trap: invalid borrow
```

* Runtime metadata disabled in production for zero-cost

---

## 5. Undefined Behavior Traps

* Safe code **cannot produce UB**, but `unsafe` blocks can be instrumented

* Optional runtime checks for:

  * Null dereference
  * Out-of-bounds access
  * Use-after-free
  * Integer overflow (optional)
  * Misaligned memory access

* Reports include **file, line, module, and allocation context**

---

## 6. Allocator Profiling

* Every allocation/deallocation optionally logged:

```c
struct AllocationEvent {
    ModuleID module;
    TypeID type;
    usize size;
    usize timestamp;
    AllocatorID allocator;
}
```

* Runtime reports:

  * Hot memory paths
  * Peak usage
  * Memory leaks
  * Lifetime of allocations

* Supports **arenas, pools, and global allocator tracking**

---

## 7. Thread & Concurrency Profiling

* Optional tracking for:

  * Thread creation and join times
  * Scoped thread lifetimes
  * Atomic operations counts per variable
  * Lock acquisition/release times
  * Potential deadlocks in development mode

* Visual tooling can aggregate per-CPU or per-module statistics

---

## 8. Error Propagation Logging

* Every `Result<T,E>` in runtime mode can optionally log:

  * Function name
  * Error type
  * Source module
  * Propagation chain

Example:

```c
let result: Result<@owned File, IOError> = open_file("config");
log_result(result); // optional, development mode
```

* Enables debugging **silent failure chains** in complex kernel or embedded code

---

## 9. Scoped Diagnostics

* Scoped diagnostics can be enabled per module or function:

```c
#[diagnostics(enable)]
fn debug_file_access() -> Result<(), IOError> {
    let f = open_file("config")?;
    defer close(f);
    Ok(())
}
```

* Useful for **hotspot profiling** without global overhead

---

## 10. IDE Integration

* Runtime metrics can be visualized in IDE:

  * Allocation hotspots
  * Thread lifetimes
  * Ownership violations
  * Error propagation graphs

* Supports **interactive debugging** and **live performance analysis**

---

## 11. Kernel & Embedded Considerations

* Diagnostics can be **compiled out** in production to eliminate overhead
* Embedded systems can selectively enable:

  * Arena allocation logging
  * Scoped thread tracking
  * Error propagation logging
* Kernel builds can log **per-subsystem allocations, locks, and thread usage** without compromising real-time performance

---

## 12. Example Usage

```c
#[diagnostics(enable)]
fn process_files() -> Result<(), IOError> {
    @owned File f = open_file("config")?;
    defer close(f);

    let buffer = alloc<u8>(1024)?;
    defer dealloc(buffer, 1024);

    read(f, buffer)?;
    Ok(())
}
```

Runtime report might include:

```
[Diagnostics] Module: fs.vfs, Function: process_files
- Allocated 1024 bytes via global allocator
- File handle opened and closed correctly
- No ownership violations detected
```

---

## 13. Performance Guarantees

* Diagnostics **zero-cost in safe code** when disabled
* Scoped checks incur minimal overhead
* Allocator and concurrency profiling are optional, compiled in development builds
* Compiler enforces **safe code optimization** while retaining instrumentation hooks

---

## 14. Summary

C 2030 optional runtime diagnostics provide:

* Auditable **ownership, lifetime, and UB monitoring**
* **Allocator and memory profiling** for kernel and embedded systems
* **Thread and concurrency statistics**
* **Error propagation logging** for complex Result chains
* **Scoped, module-specific, and zero-cost options** for production and embedded builds

Combined with prior RFCs, this completes the **C 2030 ecosystem**, giving developers:

* Compile-time safety guarantees
* Optional runtime observability
* High-performance execution
* Safe migration path for critical systems (kernel, embedded, multi-threaded)


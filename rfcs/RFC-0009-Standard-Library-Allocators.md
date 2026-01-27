# RFC 0009

## Standard Library & Allocators

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-01-27

---

## 1. Abstract

C 2030 introduces a **modern standard library** and **allocator framework** designed for:

* Predictable, auditable memory management
* Tight integration with ownership and lifetimes (RFC 0004)
* Kernel and embedded system friendliness
* Minimal runtime overhead
* Explicit error handling (RFC 0007)

This RFC formalizes:

* Core data types
* Memory allocation primitives
* Container libraries
* File, IO, and string abstractions
* Interaction with ownership and concurrency

---

## 2. Motivation

Classic C standard library suffers from:

* Implicit memory ownership (`malloc`/`free`)
* Unchecked error handling (`fopen`, `strtol`)
* Unsafe string and container APIs
* Inconsistent cross-platform behavior

C 2030 seeks:

1. Safe and explicit memory ownership
2. Integrable error handling (`Result<T, E>`)
3. Deterministic performance
4. Kernel / embedded friendly design

---

## 3. Design Principles

1. **Ownership-aware API** — Allocated memory is `@owned`
2. **Explicit deallocation** — No hidden GC or leaks
3. **Typed errors** — All fallible operations return `Result<T, E>`
4. **Modular design** — Only exported symbols are available
5. **Zero-cost abstractions** — No runtime overhead in safe code

---

## 4. Memory Allocators

### 4.1 Global Allocator

```c
fn alloc<T>(usize count) -> Result<@owned T*, MemError>;
fn dealloc<T>(@owned T* ptr, usize count);
```

* `alloc` returns `@owned` pointer
* Ownership is explicit and checked
* Errors are returned via `Result`

Example:

```c
@owned i32* buffer = alloc<i32>(1024)?;
dealloc(buffer, 1024);
```

---

### 4.2 Custom Allocators

Custom allocators implement:

```c
interface Allocator {
    fn alloc<T>(usize count) -> Result<@owned T*, MemError>;
    fn dealloc<T>(@owned T* ptr, usize count);
}
```

* Users may provide **arena allocators**, **pools**, or **bump allocators**
* Default allocator uses system heap (`malloc`/`free`)

---

### 4.3 Stack and Arena Allocators

```c
fn arena(size: usize) -> Arena;
fn arena_alloc<T>(Arena*, usize count) -> @owned T*;
fn arena_reset(Arena*);
```

* Lifetime of allocated memory bound to **arena lifetime**
* Perfect for temporary allocations in kernel or embedded loops

---

## 5. Containers

C 2030 provides **ownership-aware containers**.

### 5.1 Dynamic Array

```c
struct Vec<T> {
    @owned T* ptr;
    usize length;
    usize capacity;
}
```

* Automatically resizes using `alloc`
* `drop` frees memory when vector goes out of scope
* `push`, `pop`, `get` enforce lifetime and ownership rules

Example:

```c
let mut v = Vec<i32>::new();
v.push(10);
v.push(20);
```

---

### 5.2 Hash Map

```c
struct HashMap<K, V> { ... }
```

* Uses explicit allocator for entries
* Keys and values are `@owned` unless borrowed
* Error handling via `Result` for insertions and lookups

---

### 5.3 Strings

```c
struct String {
    @owned u8* data;
    usize length;
}
```

* UTF-8 by default
* Memory managed via allocator
* Provides concatenation, slicing, and formatting
* Error handling integrated

```c
let s = String::from_utf8(buf)?; // propagates error
```

---

## 6. File I/O

C 2030 standard library provides **ownership-aware file handles**:

```c
struct File {
    @owned i32 fd;
}

fn open(@owned string path, mode: FileMode) -> Result<@owned File, IOError>;
fn read(@borrowed File* f, buf: @owned array<u8>) -> Result<usize, IOError>;
fn write(@borrowed File* f, buf: @borrowed array<u8>) -> Result<usize, IOError>;
fn close(@owned File* f);
```

* Ownership guarantees safe closing
* Error handling via `Result`
* Compatible with kernel and embedded file abstractions

---

## 7. Memory Safety Integration

* All allocations produce `@owned` pointers
* `defer` ensures automatic cleanup in scope exit
* Containers drop automatically on scope exit
* Borrowed references cannot escape container lifetime

```c
fn example() -> Result<(), MemError> {
    @owned Vec<i32> v = Vec::new();
    defer drop(v);
    v.push(42)?;
    Ok(())
}
```

---

## 8. Error Propagation

* All fallible APIs return `Result<T, E>`
* Standard library uses **typed errors**:

```c
enum IOError { NotFound, PermissionDenied, Unknown }
enum MemError { OutOfMemory, InvalidFree }
```

* Error handling integrates with `?` operator (RFC 0007)

---

## 9. Thread-Safe Allocators

* Default allocator is **thread-safe**
* Arena and pool allocators can be single-threaded for performance
* Compiler enforces **ownership and lifetimes** to prevent data races (RFC 0008)

---

## 10. Compatibility & Legacy C

* Standard library provides **wrappers** for C functions:

```c
fn malloc(usize) -> Result<@owned u8*, MemError>;
fn free(@owned u8* ptr);
```

* Legacy C code may be imported as modules
* Wrappers translate unsafe memory usage into ownership-aware APIs

---

## 11. Kernel and Embedded Considerations

* All allocations can be statically sized or arena-backed
* No hidden global state
* Containers can be configured with **compile-time capacity**

Example:

```c
static mut arena_buf: [u8; 4096];
let a = Arena::from_buf(&arena_buf);
```

* Deterministic allocation and deallocation

---

## 12. Comparison Table

| Feature                    | C 2030 | C | C++ | Rust | Zig |
| -------------------------- | ------ | - | --- | ---- | --- |
| Ownership-aware memory     | ✅      | ❌ | ✅   | ✅    | ✅   |
| Error-aware allocators     | ✅      | ❌ | ⚠️  | ✅    | ✅   |
| Thread-safe default        | ✅      | ❌ | ⚠️  | ✅    | ⚠️  |
| Scoped resource cleanup    | ✅      | ❌ | ✅   | ✅    | ✅   |
| Kernel / embedded friendly | ✅      | ✅ | ⚠️  | ⚠️   | ✅   |

---

## 13. Summary

C 2030’s **standard library and allocator system**:

* Provides **ownership-aware memory management**
* Integrates **typed error handling**
* Provides **safe, deterministic containers**
* Supports **thread-safe and arena-based allocation**
* Enables **kernel and embedded-grade abstractions**
* Avoids hidden runtime overhead

This RFC completes the foundation for **safe, modern, auditable C programming**.

---

Next RFCs could include:

* **RFC 0010 — Linux Kernel Migration Playbook**
* **RFC 0011 — Language Tooling & Static Analysis**
* **RFC 0012 — ABI & Linker Contracts**


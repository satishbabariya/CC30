# RFC 0012

## ABI & Linker Contracts

**Status:** Draft
**Category:** Language Infrastructure
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-01-27

---

## 1. Abstract

This RFC defines **Application Binary Interface (ABI)** and **linker contracts** for C 2030:

* Symbol resolution, visibility, and naming conventions
* Module and cross-module function calling conventions
* Data layout and struct packing rules (interfacing with C and kernel code)
* Versioning and compatibility rules
* Guarantees for ownership, lifetime, and memory safety across binaries

The goal is to ensure **interoperability with legacy C**, **kernel stability**, and **safe cross-module integration**.

---

## 2. Motivation

Modern language features in C 2030 require explicit ABI rules:

* Ownership-aware types (`@owned`, `@borrowed`)
* `Result<T,E>` for error handling
* Scoped and unsafe functions
* Module-based visibility
* Concurrency primitives

Without a well-defined ABI:

* Cross-module calls could violate ownership or lifetimes
* Linking against legacy C libraries may be unsafe
* Kernel and embedded systems would face undefined behavior

---

## 3. Symbol Naming & Visibility

### 3.1 Mangling

* All exported symbols are **mangled** with module and type info:

```
<module>_<function>_<signature_hash>
```

Example:

```c
module fs.vfs;

export fn open_file(@owned string path) -> Result<@owned File, IOError>;
```

Generates:

```
fs_vfs_open_file_R<owned_string>_R<Result<owned_File,IOError>>
```

* Mangling ensures **unique symbol names**
* Facilitates **cross-module linking and overloads**

---

### 3.2 Visibility Rules

* `export` → public symbol visible to linker
* `private` → internal to module, not exposed
* Compiler enforces **no accidental leaks**

---

## 4. Calling Conventions

### 4.1 Safe Functions

* All `safe` functions follow **C-compatible ABI**:

  * Return values and `Result<T,E>` on stack or registers
  * Ownership moves are represented by pointer + metadata
  * `@borrowed` pointers passed as normal references

* Example calling a `Result` function:

```c
Result<@owned File, IOError> f = fs_vfs_open_file(path);
```

* Compiler automatically handles **Result propagation via `?`**

---

### 4.2 Unsafe Functions

* Can bypass standard ABI for:

  * Hardware registers
  * Low-level system calls
  * Performance-critical routines

* Must explicitly be marked `unsafe`

* Compiler emits **warning when mixing safe and unsafe ABI**

---

### 4.3 Cross-Language Interop

* Functions marked `extern "C"` adhere to **traditional C ABI**:

```c
extern "C" fn legacy_read(int fd, void* buf, usize len) -> i32;
```

* Safe wrappers can convert to `Result` and ownership semantics

---

## 5. Struct & Data Layout

* Default **struct layout** compatible with C `packed` rules:

  ```c
  struct MyStruct {
      u32 a;
      u16 b;
      u16 c;
  }
  ```

* Optional attributes for **alignment and padding**:

```c
#[align(8)]
struct AlignedStruct { ... }
```

* Ownership and `@owned` metadata stored **outside struct** to avoid runtime overhead
* Compiler guarantees **safe struct copying, moving, and dropping**

---

## 6. Modules & Cross-Module Contracts

* Module exports define **ABI contracts**:

```c
module net.tcp;

export fn connect(addr: SocketAddr) -> Result<Connection*, TcpError>;
```

* Linker validates:

  * Symbol visibility
  * Signature matching
  * Result and ownership types
  * Module-level versioning (see §8)

* Importing modules automatically resolves symbols using **module DAG**

---

## 7. Memory Ownership Across Binaries

* `@owned` types may be transferred across module boundaries

* Compiler emits **metadata descriptors** for ownership:

  * Allocator used
  * Type size and alignment
  * Drop/deallocation function

* Ensures that memory allocated in one module is safely freed in another

---

## 8. Versioning & Compatibility

* ABI version embedded in module metadata
* Modules may declare:

```c
module fs.vfs;
version 1.2.0;
```

* Linker enforces **compatible versions**

Rules:

1. Minor version bump → backward-compatible ABI
2. Major version bump → may break ABI, compiler warning emitted
3. Cross-version linking requires explicit conversion adapters

---

## 9. Kernel & Embedded Considerations

* Deterministic layout and calling conventions ensure **interrupt and ISR compatibility**
* Optional **no-RTTI** mode for embedded systems
* Unsafe functions may directly interface with memory-mapped IO

---

## 10. Tooling & Diagnostics

* Compiler produces **ABI reports** for modules:

  * Exported symbols
  * Mangled names
  * Result type layouts
  * Ownership annotations
  * Version info

* Link-time checks verify:

  * Cross-module ownership integrity
  * Type signature compatibility
  * Error propagation contracts
  * Atomic and concurrency compliance

---

## 11. Legacy C Interoperability

* Legacy C code can be linked using `extern "C"`
* Safe wrappers convert raw pointers and error codes to **ownership-aware, Result-based APIs**
* Compiler and linker validate:

  * Memory safety at boundary
  * Correct calling convention
  * Proper cleanup of allocated resources

---

## 12. Example: Cross-Module Call

```c
// fs.vfs module
export fn open_file(@owned string path) -> Result<@owned File, IOError>;

// user module
import fs.vfs;

fn example() -> Result<(), IOError> {
    @owned File f = fs_vfs_open_file("config")?;
    defer close(f);
    Ok(())
}
```

* Compiler and linker ensure:

  * Symbol exists (`fs_vfs_open_file`)
  * Ownership is respected across module boundary
  * `Result` type is correctly returned

---

## 13. Comparison Table

| Feature                         | C 2030 | C | C++ | Rust |
| ------------------------------- | ------ | - | --- | ---- |
| Module-level ABI                | ✅      | ❌ | ⚠️  | ✅    |
| Ownership-aware ABI             | ✅      | ❌ | ⚠️  | ✅    |
| Versioned symbols               | ✅      | ❌ | ⚠️  | ⚠️   |
| Cross-module Result propagation | ✅      | ❌ | ⚠️  | ✅    |
| Legacy C interop                | ✅      | ✅ | ✅   | ✅    |

---

## 14. Summary

C 2030 ABI & linker contracts provide:

* **Deterministic cross-module function calls**
* **Ownership and lifetime guarantees** across binaries
* **Safe integration with legacy C code**
* **Versioned modules for backward compatibility**
* **Full support for kernel and embedded deterministic execution**

Combined with prior RFCs, C 2030 is **fully production-ready**, enabling:

* Safe Linux kernel migration
* Auditable, deterministic embedded systems
* Cross-module and cross-binary correctness
* Zero-cost abstractions for performance-critical code

---

Next RFCs (optional, for completeness):

* **RFC 0013 — Optional Runtime Diagnostics & Profiling**
* **RFC 0014 — Language Evolution & Versioning Policy**
* **RFC 0015 — Standard Library Extensions**


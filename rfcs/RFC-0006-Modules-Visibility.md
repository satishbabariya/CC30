# RFC 0006 — Modules & Visibility

*C 2030 Language Specification*

---

## Status

**Draft**

## Author(s)

C 2030 Working Group

## Target

C 2030 Compiler & Toolchain

## Motivation

C’s historical compilation model is built around **textual inclusion** (`#include`) and **global symbol visibility**. This leads to:

* ODR-like violations without diagnostics
* Header / source duplication and divergence
* Macro pollution across translation units
* Slow, order-dependent builds
* Unclear API boundaries

Modern systems (Linux kernel, embedded firmware, kernels, runtimes) need:

* Explicit dependency graphs
* Fast incremental builds
* Clear ABI and API boundaries
* Zero-overhead abstractions

C 2030 introduces **first-class modules** while preserving C’s performance, link model, and interoperability.

---

## Goals

1. Eliminate textual headers as the primary interface mechanism
2. Provide **explicit visibility and exports**
3. Preserve predictable object layout and ABI
4. Enable fast compilation and tooling
5. Maintain compatibility with existing C and linker models

---

## Non-Goals

* No runtime module loading (handled externally)
* No reflection or RTTI
* No mandatory namespaces for C legacy code
* No dependency on C++ modules or toolchains

---

## Core Concepts

### Module

A **module** is the fundamental compilation and encapsulation unit.

Each module defines:

* Its exported interface
* Its private implementation
* Its dependencies

---

## Module Declaration

```c
module net.ipv4;
```

Rules:

* Must appear at the top of the file
* One module per source file
* Module names are hierarchical identifiers

---

## Exporting Symbols

### Explicit Export

```c
export struct ip_header {
    u8  version;
    u8  ihl;
    u16 length;
};

export int ipv4_send(packet_t *pkt);
```

Rules:

* Only explicitly `export`ed symbols are visible outside the module
* Unexported symbols are module-private

---

## Importing Modules

```c
import net.ipv4;
import crypto.sha256;
```

Rules:

* Imports are **semantic**, not textual
* Import order has no semantic meaning
* Cyclic imports are forbidden (compile-time error)

---

## Visibility Rules

| Symbol Type | Default Visibility |
| ----------- | ------------------ |
| Functions   | Private            |
| Structs     | Private            |
| Enums       | Private            |
| Globals     | Private            |
| Macros      | Private            |
| Types       | Private            |

Visibility must be **explicitly elevated** using `export`.

---

## Private Symbols

Symbols not marked `export`:

* Are invisible to other modules
* Cannot be referenced, even via `extern`
* May be renamed or eliminated freely by the compiler

```c
static int fast_checksum(...); // module-private
```

---

## Replacing Headers

### Legacy Model

```c
// ipv4.h
struct ip_header;
int ipv4_send(...);
```

### C 2030 Model

```c
module net.ipv4;

export struct ip_header { ... };
export int ipv4_send(...);
```

No duplication. No `#ifdef`. No macro leakage.

---

## Macros and Modules

### Macro Containment

* Macros are **module-local by default**
* Macros do NOT leak across module boundaries

```c
#define PAGE_SHIFT 12 // private
```

### Exported Macros (discouraged)

```c
export macro PAGE_SHIFT 12;
```

Rules:

* Exported macros must be explicitly declared
* Tooling must warn on exported macros

---

## Module Initialization

Optional module initializer:

```c
module fs.vfs;

init {
    register_filesystem(&vfs);
}
```

Rules:

* Runs before `main()`
* Execution order follows dependency DAG
* No global constructor ordering hacks

---

## ABI & Linking Model

* Modules compile to standard object files
* Exported symbols map directly to linker symbols
* No name mangling beyond module prefixing

Example linker symbol:

```
net_ipv4_ipv4_send
```

ABI remains stable and inspectable.

---

## Forward Declarations

Forward declaration across modules is **not allowed**.

```c
// ❌ illegal
extern struct ip_header;
```

Rationale:

* Prevents type mismatches
* Forces explicit dependency declaration

---

## Conditional Compilation

Modules replace most `#ifdef` usage.

Allowed uses:

* Platform detection
* Compiler feature checks

```c
#if arch(x86_64)
import arch.x86;
#endif
```

---

## Header Interoperability (Transitional)

Legacy headers may be imported:

```c
import legacy "linux/list.h";
```

Rules:

* Legacy imports are isolated
* Symbols must be explicitly re-exported to escape

---

## Linux Kernel Impact (Illustrative)

### Today

* `include/linux/*.h`
* Widespread macro leakage
* Implicit dependencies

### With C 2030

```
kernel/
 ├── mm/
 │   ├── module mm.page
 │   ├── module mm.vma
 ├── net/
 │   ├── module net.ipv4
 │   ├── module net.tcp
```

Benefits:

* Compile-time dependency validation
* Faster incremental builds
* Cleaner subsystem boundaries

---

## Tooling Benefits

* IDE symbol resolution without preprocessing
* Accurate dependency graphs
* Deterministic builds
* Cacheable module interfaces

---

## Comparison Snapshot

| Feature           | C 2030 | Rust | Zig | C3 |
| ----------------- | ------ | ---- | --- | -- |
| Explicit exports  | ✅      | ✅    | ✅   | ✅  |
| Headerless        | ✅      | ✅    | ✅   | ✅  |
| Macro containment | ✅      | ❌    | ❌   | ❌  |
| ABI transparency  | ✅      | ❌    | ✅   | ✅  |
| Kernel-friendly   | ✅      | ⚠️   | ⚠️  | ⚠️ |

---

## Open Questions

1. Should re-exports be allowed?
2. Should module aliasing exist?
3. Should module versioning be language-level or tooling-level?

---

## Summary

C 2030 modules:

* Eliminate header hell
* Enforce encapsulation
* Preserve C’s zero-cost model
* Scale to kernel-level systems

They modernize C **without turning it into C++ or Rust**.


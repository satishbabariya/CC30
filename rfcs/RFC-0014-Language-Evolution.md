# RFC 0014

## Language Evolution & Versioning Policy

**Status:** Draft
**Category:** Language Governance
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-01-27

---

## 1. Abstract

This RFC defines **policies for language evolution and versioning** in C 2030, including:

* Module versioning and ABI compatibility
* Deprecation and removal policies
* Feature flags and optional extensions
* Compiler and standard library evolution
* Long-term stability for kernel, embedded, and production systems

Goals:

* Ensure **safe, predictable evolution** of the language
* Avoid **breakage of existing code**
* Enable **incremental adoption of new features**

---

## 2. Motivation

Modern system programming requires:

* Long-lived codebases (Linux kernel, embedded firmware)
* Strong guarantees for **memory safety, ownership, and concurrency**
* Predictable compiler and library behavior over decades

Without a formal evolution and versioning policy:

* ABI breaks may occur unexpectedly
* Compiler or library changes could silently violate ownership guarantees
* Modules may become incompatible across projects

---

## 3. Module Versioning

* Every module must declare **semantic version**:

```c
module fs.vfs;
version 1.2.0;
```

### Rules:

1. **Major version bump** → incompatible changes (ABI or API-breaking)
2. **Minor version bump** → new features, backward-compatible
3. **Patch version bump** → bug fixes, backward-compatible

### Linker Behavior:

* Module DAG validates **compatible versions**
* Cross-module calls fail if **major version mismatch**
* Minor/patch upgrades automatically accepted

---

## 4. Deprecation Policy

* Deprecated APIs must be annotated:

```c
#[deprecated(since="1.2.0", removal="1.4.0", note="Use open_file_v2 instead")]
export fn open_file(...) -> Result<...> { ... }
```

* Compiler emits **warnings** when using deprecated features
* Deprecated APIs may be **removed in a future major version**
* Allows developers to **gradually migrate**

---

## 5. Feature Flags & Optional Extensions

* Compiler supports **feature flags** for experimental or optional features:

```c
#[feature("concurrent_allocator")]
fn allocate_concurrent<T>() -> Result<@owned T*, MemError>;
```

* Feature flags:

  * Must be explicitly enabled
  * Can be **turned off** without breaking safe code
  * Enable testing of **new language capabilities** in development builds

---

## 6. Compiler Evolution

* Compiler may introduce:

  * New optimizations
  * Additional static analysis or lints
  * New diagnostic modes

* **Backward compatibility guaranteed** for **safe code**

* Unsafe code may require **audit when compiler adds new UB checks**

---

## 7. Standard Library Evolution

* Standard library versioned independently from compiler:

```c
std.version = 1.3.0;
```

* Backward-compatible extensions allowed in minor versions
* Deprecated APIs flagged in major version changes
* Allocator, concurrency, and diagnostics libraries follow **semver rules**

---

## 8. ABI Stability

* ABI version embedded in module metadata
* Major version bump may change ABI
* Minor and patch versions maintain **cross-module binary compatibility**
* Compiler emits **warnings or errors** for mismatched ABI usage

---

## 9. Kernel and Embedded Policies

* Kernel and embedded modules must **pin versions** for critical subsystems
* Optional runtime diagnostics and unsafe features can be toggled per build
* Strict backward compatibility enforced for **long-lived binaries**

---

## 10. Governance

* Language evolution overseen by **C 2030 Working Group**

* Proposed changes must be documented as RFCs

* RFCs include:

  * Rationale for feature
  * Compatibility guarantees
  * Migration plan
  * Deprecation schedule

* Community review ensures **auditable, safe adoption**

---

## 11. Migration Strategy for Breaking Changes

1. **Deprecate** feature in one minor version
2. **Warn** in subsequent minor versions
3. **Remove** in next major version

* Example:

```
v1.2.0: #[deprecated] old_allocator
v1.3.0: compile-time warnings for old_allocator
v2.0.0: old_allocator removed
```

* Compiler may provide **automated migration suggestions**

---

## 12. Future-Proofing Extensions

* **Experimental features** isolated via `#[feature(...)]`

* Safe code continues to compile even if features are removed

* Unsafe code flagged for **audit and review**

* Ensures **kernel and embedded code remain stable**

---

## 13. Example: Deprecation & Migration

```c
#[deprecated(since="1.2.0", removal="1.4.0", note="Use Vec<T>::new() instead")]
export fn create_array<T>(usize n) -> @owned T* { ... }

fn main() {
    let arr = create_array<i32>(10); // Compiler warning
    let v = Vec<i32>::new();          // Recommended replacement
}
```

* Warning includes:

```
warning[DEPR-001]: 'create_array' is deprecated since 1.2.0 and will be removed in 1.4.0. Use Vec<T>::new() instead.
```

---

## 14. Summary

C 2030’s evolution and versioning policy ensures:

* **Stable ABI and API** for kernels, embedded, and production systems
* **Explicit module versioning** for safe cross-module linking
* **Deprecation policy** for gradual migration
* **Optional features** for safe experimentation
* **Compiler and standard library backward compatibility**

Together with RFCs 0001–0013, this completes the **C 2030 specification**, providing:

* Safe memory and concurrency guarantees
* Ownership and lifetime tracking
* Module encapsulation and visibility
* Result-based error handling
* Compiler, tooling, and runtime diagnostics
* Kernel- and embedded-ready ABI and linker contracts
* A formal policy for language evolution




# RFC 0010

## Linux Kernel Migration Playbook

**Status:** Draft
**Category:** Systems / Migration Guide
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-01-27

---

## 1. Abstract

This RFC presents a **practical roadmap** for porting the Linux kernel to **C 2030**, integrating:

* Ownership & lifetimes (RFC 0004)
* Undefined behavior traps (RFC 0005)
* Modules & visibility (RFC 0006)
* Error handling (RFC 0007)
* Concurrency & atomics (RFC 0008)
* Standard library & allocators (RFC 0009)

Goals:

1. Eliminate silent undefined behavior in kernel code
2. Introduce safe concurrency defaults
3. Replace legacy C patterns with modern abstractions
4. Maintain kernel performance and ABI stability

---

## 2. Migration Principles

1. **Incremental Migration** — Kernel subsystems are migrated independently
2. **Safe First** — Start with non-critical modules using `safe` mode
3. **`unsafe` Reserved** — Hardware access and performance-critical code may remain `unsafe`
4. **Automated Diagnostics** — Use compiler warnings, lifetime checks, and UB traps
5. **Interoperability** — Legacy C headers can be imported as modules

---

## 3. Module Organization

C 2030 modules replace textual headers:

```
kernel/
 ├── mm/
 │   ├── module mm.page
 │   ├── module mm.vma
 ├── fs/
 │   ├── module fs.vfs
 │   ├── module fs.ext4
 ├── net/
 │   ├── module net.ipv4
 │   ├── module net.tcp
```

**Steps:**

1. Identify each `.c` file’s logical module
2. Declare `module <subsystem>.<component>` at top
3. Export only public API symbols (`export`)
4. Convert `#include` dependencies to `import`

**Benefits:**

* Explicit visibility boundaries
* Reduced macro leakage
* Deterministic build order

---

## 4. Memory & Allocators

### 4.1 Replace `kmalloc` / `kfree`

* Map to `C2030::alloc<T>` and `dealloc<T>`
* Use `@owned` pointers for automatic lifetime management

```c
@owned Page* page = alloc<Page>(1)?; // propagates MemError
dealloc(page, 1);
```

### 4.2 Arena Allocators

* Ideal for temporary allocations (per syscall, per IRQ)
* Reset arena at scope exit for zero-cost cleanup

---

## 5. Ownership & Lifetimes

* Convert raw pointers into `@owned` or `@borrowed`
* Enforce lifetime constraints:

```c
fn allocate_buffer(@owned struct Buffer* buf) -> Result<(), MemError> {
    buf->data = alloc<u8>(buf->size)?;
    return Ok(());
}
```

* Prevent dangling references and use-after-free in safe code
* `unsafe` blocks reserved for direct hardware mapping

---

## 6. Error Handling

* Replace legacy error codes (`-EINVAL`, `NULL` checks) with **typed `Result<T,E>`**

Example:

```c
fn open_device(@owned string name) -> Result<Device*, DeviceError> {
    let dev = find_device(name)?;
    configure(dev)?; // propagates errors
    return Ok(dev);
}
```

* Compiler enforces exhaustive error handling
* `?` operator reduces boilerplate

---

## 7. Concurrency & Atomics

* Convert `spin_lock`, `mutex`, and atomic variables to **C2030 atomics and locks**

```c
atomic<u32> counter;

fn increment() {
    counter.fetch_add(1, SeqCst);
}
```

* Scoped threads for per-CPU tasks
* Compiler prevents data races in safe code
* `unsafe` retains access for hardware IRQ handlers

---

## 8. Kernel Subsystem Migration Strategy

| Step | Action                                                                             |
| ---- | ---------------------------------------------------------------------------------- |
| 1    | Memory subsystem (`mm/`) — start safe code conversion with arena allocators        |
| 2    | Filesystem (`fs/`) — convert VFS layer to modules and Result-based error handling  |
| 3    | Networking (`net/`) — convert protocols to module boundaries, atomics for counters |
| 4    | Drivers (`drivers/`) — maintain `unsafe` for direct hardware access                |
| 5    | Scheduler (`kernel/sched/`) — integrate safe concurrency primitives                |
| 6    | Legacy C integration — wrap `extern` APIs in `unsafe` modules                      |
| 7    | Build system migration — enforce module DAG and incremental compilation            |
| 8    | Testing — integrate with UB traps and error handling validation                    |

---

## 9. Compiler & Tooling

* **Safe code:** Compile-time detection of UB, dangling references, and data races
* **Unsafe code:** Warnings for potential UB, required explicit marking
* Use static analysis tools to verify:

  * Ownership correctness
  * Lifetime consistency
  * Atomic and lock correctness

---

## 10. Performance Considerations

* Safe abstractions are **zero-cost when possible**
* Atomics and locks only emit CPU instructions when needed
* Arena allocators reduce dynamic heap pressure
* Legacy `unsafe` code preserves high-performance paths

---

## 11. Testing & Verification

* Leverage existing kernel tests (KUnit, LTP)
* Enable UB traps in development builds
* Track Result propagation for all APIs
* Static analysis for lifetime violations

---

## 12. Stepwise Migration Example (VFS)

**Original:**

```c
struct file* f = open("/tmp/file");
if (!f) return -ENOENT;
read(f, buf, len);
close(f);
```

**C2030 Conversion:**

```c
fn open_file(@owned string path) -> Result<@owned File, IOError> {
    let f = open(path)?; // propagates error
    return Ok(f);
}

fn process_file() -> Result<(), IOError> {
    @owned File f = open_file("/tmp/file")?;
    defer close(f);
    read(f, buffer)?;
    Ok(())
}
```

* Ownership ensures proper cleanup
* `?` propagates errors
* `defer` guarantees resource release

---

## 13. Migration Summary

* **Modules:** Replace headers with explicit modules (`RFC 0006`)
* **Memory:** Replace `kmalloc`/`kfree` with `alloc`/`dealloc` (`RFC 0009`)
* **Error Handling:** Replace integer codes with `Result` (`RFC 0007`)
* **Concurrency:** Replace manual spinlocks and atomics with safe C2030 concurrency (`RFC 0008`)
* **Undefined Behavior:** UB is trapped in safe code (`RFC 0005`)

**Result:** Linux kernel in C 2030 is **safer, auditable, modular, and deterministic**, while retaining **performance and ABI stability**.

---

## 14. Open Questions

1. Should driver APIs remain fully `unsafe` or gradually adopt safe patterns?
2. How to handle architecture-specific memory barriers while enforcing safety?
3. How to integrate legacy kernel build scripts with module DAG?

---

## 15. Benefits

* **Safer kernel code**: prevent UB, data races, and leaks
* **Faster incremental builds**: modules and DAG-based compilation
* **Auditable memory management**: ownership and lifetimes
* **Deterministic concurrency**: atomics, locks, and scoped threads
* **Backward-compatible with legacy C**: unsafe escape hatches

---

## 16. Summary

C 2030 provides a **practical migration path for Linux kernel**:

* Modules isolate subsystems
* Ownership and lifetime checks prevent dangling pointers
* Result-based error handling removes silent failure
* Atomics and safe concurrency prevent data races
* UB traps ensure runtime correctness

This RFC demonstrates that **C 2030 is kernel-ready**, enabling modern safety guarantees **without compromising performance or control**.


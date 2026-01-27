# RFC 0008

## Concurrency & Atomics

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-01-27

---

## 1. Abstract

C 2030 introduces a **first-class concurrency and atomic model** designed to:

* Enable **deterministic multithreading**
* Integrate with **ownership and memory safety rules** (RFC 0004)
* Eliminate **data races** in safe code
* Provide **fine-grained control** in `unsafe` code
* Preserve **low-level kernel and embedded semantics**

This RFC specifies:

* Threads & tasks
* Atomic types and operations
* Memory ordering
* Synchronization primitives
* Concurrency safety rules

---

## 2. Motivation

Traditional C concurrency is:

* Unstructured (POSIX threads)
* Data-race-prone
* Compiler optimizations often break assumptions
* No ownership or lifetime integration

C 2030 ensures:

* Safe defaults (`safe` code is race-free)
* Predictable memory models
* Full control in `unsafe` code
* Auditability for kernel-level correctness

---

## 3. Threads & Tasks

### 3.1 Thread Creation

```c
fn spawn(fn() -> void) -> Thread;
```

* Returns a **handle** representing the thread
* Thread runs independently
* Handle is `@owned`

Example:

```c
@owned Thread t = spawn(|| {
    printf("Hello from thread\n");
});
```

---

### 3.2 Thread Join

```c
t.join(); // waits for completion
```

* Ensures thread’s resources are cleaned
* Returns `Result<(), ThreadError>` if thread panics

---

### 3.3 Scoped Threads

```c
fn scoped_example(@borrowed int* data) {
    spawn_scoped(|| {
        *data += 1; // safe, cannot escape scope
    });
}
```

* Scoped threads enforce **lifetime constraints**
* Borrowed data cannot escape thread
* Compiler guarantees memory safety

---

## 4. Atomics

C 2030 introduces **first-class atomic types**:

```c
atomic<i32> counter;
```

* Atomic operations are **memory-safe**
* Default memory ordering is **sequentially consistent**
* Explicit orderings are available

---

### 4.1 Supported Types

| Atomic Type    | Backing Type    |
| -------------- | --------------- |
| `atomic<u8>`   | 8-bit unsigned  |
| `atomic<i32>`  | 32-bit signed   |
| `atomic<u64>`  | 64-bit unsigned |
| `atomic<bool>` | boolean         |

* Atomic struct or pointer types are **unsupported** (use locks or `atomic<T*>`)

---

### 4.2 Operations

```c
load(a)           // atomic read
store(a, val)     // atomic write
fetch_add(a, val) // atomic addition
compare_exchange(a, expected, desired) // CAS
```

Optional memory order:

```c
load(a, Relaxed);
store(a, Release);
fetch_add(a, AcquireRelease);
```

---

## 5. Memory Ordering

C 2030 supports standard memory orderings:

| Ordering               | Semantics                                             |
| ---------------------- | ----------------------------------------------------- |
| Relaxed                | No ordering guarantees, atomic only                   |
| Acquire                | Prevents later reads/writes from moving before load   |
| Release                | Prevents earlier reads/writes from moving after store |
| AcquireRelease         | Combines acquire and release semantics                |
| SequentiallyConsistent | Full global ordering                                  |

Rules:

* Safe code defaults to **SequentiallyConsistent**
* Relaxed operations allowed only in `unsafe`
* Compiler may reorder non-atomic accesses respecting ownership & lifetime

---

## 6. Locks & Mutexes

### 6.1 Mutex

```c
struct Mutex<T> {
    atomic<bool> locked;
    T data;
}
```

* Lock is **scoped**:

```c
let guard = mutex.lock();
guard.data += 1;
```

* Unlock occurs when guard goes out of scope (`RAII-style`)

---

### 6.2 Spinlocks

* Provided for low-level kernel or embedded use
* Only in `unsafe` code

---

### 6.3 Deadlock Prevention

* Safe code prohibits nested lock acquisition
* `unsafe` code may acquire multiple locks, responsibility is on programmer

---

## 7. Channels & Message Passing

Optional lightweight channels:

```c
fn channel<T>(capacity: usize) -> (Sender<T>, Receiver<T>);
```

* Sender is `@owned`
* Receiver is `@owned`
* Supports **blocking**, **non-blocking**, **try-send** semantics
* Messages are **moved**, enforcing ownership rules

---

## 8. Data Race Rules

* Safe code is **race-free by design**

* Compiler enforces:

  1. Only atomic types may be shared across threads
  2. Borrowed pointers cannot escape thread boundaries
  3. Mutating shared state requires atomic or lock protection

* Unsafe code allows data races but **requires explicit `unsafe` block**

---

## 9. Thread-Local Storage

```c
thread_local u32 counter;
```

* Initialized at thread start
* Not visible to other threads
* Lifetime bound to thread

---

## 10. Integration with Ownership & Lifetimes

* Ownership prevents **dangling references** across threads
* `@borrowed` pointers cannot escape threads
* `@owned` resources must be moved or consumed by threads

Example:

```c
fn process(@owned Buffer* buf) {
    spawn(move || {
        use(buf); // ownership transferred
    });
}
```

---

## 11. Panics in Multithreaded Code

* Panic inside a thread triggers **defined trap** for that thread
* Join returns `Err(ThreadError::Panic)`
* Does **not abort the process** unless main thread panics

---

## 12. Legacy C Interoperability

* POSIX threads can be wrapped using `unsafe spawn`
* Atomic operations map to `stdatomic.h` primitives
* Compiler provides verification for `safe` thread usage

---

## 13. Compiler Enforcement

* Safe code: data race-free, atomic-only shared mutation, scoped threads only
* Unsafe code: warnings on potential races, but compilation allowed
* Lifetime checker ensures no cross-thread dangling pointers

---

## 14. Comparison Table

| Feature               | C 2030 | Rust | Zig | C  |
| --------------------- | ------ | ---- | --- | -- |
| Safe threads          | ✅      | ✅    | ⚠️  | ❌  |
| Atomic types          | ✅      | ✅    | ✅   | ⚠️ |
| Memory ordering       | ✅      | ✅    | ✅   | ⚠️ |
| Scoped threads        | ✅      | ✅    | ⚠️  | ❌  |
| Ownership integration | ✅      | ✅    | ⚠️  | ❌  |

---

## 15. Summary

C 2030 concurrency:

* Provides **thread-safety in safe code by default**
* Integrates **atomics, locks, channels, and thread-local storage**
* Enforces **lifetime and ownership rules** across threads
* Preserves **kernel and embedded low-level control** in `unsafe`
* Provides **auditable, deterministic multithreading**

---

Next RFCs would include:

* **RFC 0009 — Standard Library & Allocators**
* **RFC 0010 — Linux Kernel Migration Playbook**


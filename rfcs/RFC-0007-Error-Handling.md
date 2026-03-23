# RFC 0007

## Error Handling (No Exceptions)

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

C 2030 introduces a **modern error-handling model** that:

* Eliminates silent error propagation
* Avoids exceptions and stack unwinding
* Is zero-cost by default
* Explicitly separates **success**, **failure**, and **resource cleanup**
* Integrates with ownership, lifetimes, and modules

This RFC formalizes **error types, propagation mechanisms, and tooling contracts**.

---

## 2. Motivation

Traditional C error handling uses:

* Return codes (`int`, `errno`) — untyped, easy to ignore
* Setjmp/longjmp — unsafe, unstructured
* Exceptions (in C++) — hidden stack unwinding, incompatible with kernels

C 2030 aims to **make errors explicit, typed, and auditable** while retaining C’s performance and simplicity.

---

## 3. Core Design Principles

1. **Explicit Error Type** — All fallible operations return a defined `Result<T, E>` type
2. **No Implicit Propagation** — No exceptions, no longjmp
3. **Safe Defaults** — Functions returning `Result` must be handled explicitly
4. **Integration with Ownership** — Resource cleanup integrates with `defer` and lifetimes
5. **Optional Unsafe Escape Hatches** — `unsafe` blocks may ignore error contracts

---

## 4. The `Result` Type

C 2030 introduces a **tagged union** for error handling:

```c
union Result<T, E> {
    Ok(T),
    Err(E),
}
```

* `T` is the success type
* `E` is the error type
* Must be **exhaustively matched** when read

Example:

```c
fn read_file(@owned string path) -> Result<array<u8>, IOError> {
    @owned array<u8> data = try_alloc_file(path);
    if (!data) return Err(IOError::NotFound);
    return Ok(data);
}
```

---

## 5. Propagating Errors

### 5.1 `?` Operator

```c
fn parse_config(@owned string path) -> Result<Config, IOError> {
    let raw = read_file(path)?; // propagates Err automatically
    return Ok(parse(raw));
}
```

* `?` propagates `Err` to the caller
* Must be used in functions returning `Result`
* Does not hide ownership or lifetime constraints

---

### 5.2 Explicit Match

```c
let result = read_file("cfg.toml");
match result {
    Ok(data) => use_data(data),
    Err(e) => handle_error(e),
}
```

* Compiler enforces exhaustive handling
* Avoids unchecked errors

---

## 6. Panic vs Error

### 6.1 Panic

* Represents **unrecoverable failure**
* Triggered by `panic("message")`
* Invokes runtime trap (RFC 0005)
* Cannot be caught in safe code

Example:

```c
if (ptr == null) panic("unexpected null");
```

### 6.2 Error

* Recoverable
* Returned as `Err(E)`
* Handled by caller explicitly

---

## 7. Defer and Error Cleanup

Errors integrate with **resource management**:

```c
fn process_file(@owned string path) -> Result<(), IOError> {
    @owned File* f = open(path)?;
    defer close(f);       // always executed on scope exit
    read(f)?;             // propagate errors automatically
    write(f)?;            // propagate errors automatically
    return Ok(());
}
```

Rules:

* `defer` executes even when returning early via `?`
* Ensures no leaks

### 7.1 Conditional Defer (`defer_on_err`)

For partial initialization, `defer_on_err` runs cleanup **only when the function returns `Err`**:

```c
fn setup() -> Result<@owned Connection*, NetError> {
    @owned Socket* sock = socket_create()?;
    defer_on_err close(sock);  // only runs if we return Err

    @owned Connection* conn = connect(sock)?;
    return Ok(conn);  // success: defer_on_err does NOT run
}
```

* `defer_on_err` is defined in RFC-0004 §8.3
* Order follows LIFO interleaved with regular `defer`

### 7.2 Error Handling with `guard`

The `guard` statement (RFC-0015 §4) integrates naturally with error propagation:

```c
fn process(@borrowed Packet* pkt) -> Result<(), NetError> {
    guard hdr = pkt->header else {
        return Err(NetError::MalformedPacket);
    }
    // hdr is non-null for the rest of the function
    process_header(hdr);
    return Ok(());
}
```

`guard let` with pattern matching (RFC-0018 §14):

```c
guard let Ok(data) = parse(input) else {
    return Err(ParseError::Invalid);
}
// data is in scope and bound to the Ok value
```

---

## 8. Standard Library Error Types

C 2030 standardizes core error types:

* `IOError` — filesystem, device, network
* `MemError` — allocation failures
* `ParseError` — invalid input
* `ValueError` — domain violations
* `SystemError` — OS calls

Custom error types are encouraged:

```c
enum MyError {
    InvalidConfig,
    NotInitialized,
}
```

---

## 9. Legacy C Integration

### 9.1 C Functions Returning Codes

```c
extern fn c_read(int fd, void* buf, usize len) -> i32;
```

C 2030 wrapper:

```c
fn read(fd: i32, buf: array<u8>) -> Result<usize, IOError> {
    int ret = c_read(fd, buf.ptr, buf.length);
    if (ret < 0) return Err(IOError::from_errno(-ret));
    return Ok(ret);
}
```

* Legacy errors are translated to `Result`
* Ownership remains explicit

---

### 9.2 Optional Unsafe Wrappers

`unsafe` may bypass `Result` propagation:

```c
unsafe fn unchecked_read(@owned string path) -> @owned array<u8> { ... }
```

* Caller assumes responsibility for errors

---

## 10. Compiler Enforcement

* Functions returning `Result` **must** handle `Err` via match or `?`
* Ignoring errors → compile-time warning or error
* Exhaustiveness is enforced for enums used as error types

---

## 11. Error Propagation and Modules

* Errors are **module-local** unless exported
* Module boundaries do not hide errors
* Cross-module error types must be exported

```c
module net.tcp;

export enum TcpError { Timeout, ConnectionRefused }

export fn connect(addr: SocketAddr) -> Result<Connection*, TcpError>;
```

---

## 12. Design Rationale

* **No exceptions**: avoids hidden control flow and unsafe stack unwinding
* **Typed errors**: prevents misuse of `errno`-style integers
* **`?` operator**: combines explicit propagation with minimal boilerplate
* **Integration with ownership**: eliminates resource leaks even on error

Comparison with other languages:

| Language  | Model        | Panic vs Error | Ownership Integration  |
| --------- | ------------ | -------------- | ---------------------- |
| C         | return codes | n/a            | none                   |
| C++       | exceptions   | yes            | RAII only              |
| Rust      | Result + ?   | panic!         | borrow checker         |
| Zig       | Error unions | panic          | manual                 |
| **C2030** | Result + ?   | panic          | full ownership & defer |

---

## 13. Summary

C 2030 error handling provides:

* **Explicit**, **typed**, **auditable** error paths
* **No hidden exceptions**
* **Zero-cost propagation in safe code**
* **Resource-safe integration with ownership and defer**
* **Transition-friendly integration with legacy C APIs**

It modernizes C error management without compromising predictability, kernel-friendliness, or performance.

---

Next logical RFCs in the series:

* **RFC 0008 — Concurrency & Atomics**
* **RFC 0009 — Standard Library & Allocators**
* **RFC 0010 — Linux Kernel Migration Playbook**


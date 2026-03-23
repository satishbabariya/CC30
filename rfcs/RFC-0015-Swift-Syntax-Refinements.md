# RFC 0015

## Swift-Inspired Syntax Refinements

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

This RFC proposes **selective adoption of Swift-inspired syntax features** into C 2030 to improve ergonomics for safety-critical constructs — without abandoning C's familiar look and feel.

The changes are **additive and backwards-compatible** with existing C 2030 syntax as defined in RFCs 0002–0007. Each feature is independently adoptable, and the RFC explicitly identifies which constructs replace, supplement, or coexist with existing syntax.

---

## 2. Motivation

C 2030 already borrows concepts from Rust (ownership, `match`, `?` operator) and Zig (explicit safety). However, several Swift syntax innovations map naturally onto C 2030's semantics and solve real ergonomic pain points:

1. **`let` / `var` declarations** — Make mutability intent obvious at the declaration site, reducing bugs from accidental mutation
2. **`guard` statements** — Eliminate deeply nested null-check pyramids common in systems code
3. **`protocol` keyword** — Formalizes the currently unspecified `interface` concept from RFC-0003
4. **Optional `?` suffix for nullable pointers** — Replaces the verbose `@nullable` annotation with a well-understood shorthand
5. **Parameter labels** — Enable self-documenting call sites without sacrificing brevity

These features were chosen because they **improve safety and readability** in the exact areas C 2030 targets, while preserving C-style type-first declarations, semicolons, braces, and `fn` syntax.

### 2.1 What This RFC Does NOT Propose

* Replacing `fn` with `func`
* Removing semicolons
* Switching to camelCase conventions
* Adopting Swift's `throws`/`try` exception-like syntax
* Removing C-style type-first declarations
* Any changes to the ownership model (RFC-0004)

---

## 3. Feature: `let` and `var` Declarations

### 3.1 Current Syntax

```c
const i32 x = 42;           // immutable
i32 y = 10;                  // mutable (implicit)
@owned Buffer* buf = alloc(); // mutable owned pointer
```

Mutability is expressed via the *absence* of `const`, which is easy to overlook.

### 3.2 Proposed Syntax

```c
let i32 x = 42;             // immutable (replaces const for locals)
var i32 y = 10;              // explicitly mutable
let @owned Buffer* buf = alloc();  // immutable owned pointer
var @owned Buffer* buf = alloc();  // mutable owned pointer
```

### 3.3 Rules

1. **`let`** declares an immutable binding. The value cannot be reassigned after initialization. Equivalent to `const` for local variables.
2. **`var`** declares a mutable binding. The value can be reassigned.
3. **Bare declarations** (no `let`/`var`) remain valid and behave as `var` for backwards compatibility with C code.
4. **`const`** remains valid for global constants, struct fields, and function parameters. `let` is restricted to local and block scope.
5. **`let` on pointers** makes the *binding* immutable, not the pointed-to data. Use `const T*` for pointer-to-const semantics.

### 3.4 Examples

```c
fn process(i32 count) -> Result<(), Error> {
    let i32 max = count * 2;       // cannot reassign max
    var i32 sum = 0;               // can reassign sum

    for (var i32 i = 0; i < max; i++) {
        sum += i;
    }

    let @owned Buffer* buf = alloc<Buffer>(1)?;
    defer free(buf);

    // buf = alloc<Buffer>(1);     // ERROR: buf is immutable binding
    // max = 100;                  // ERROR: max is immutable binding

    return Ok(());
}
```

### 3.5 Interaction with Type Inference

When combined with type inference (if adopted in a future RFC), `let` and `var` enable concise declarations:

```c
let x = 42;                 // inferred i32, immutable
var y = 10;                 // inferred i32, mutable
let buf = alloc<Buffer>(1)?; // inferred @owned Buffer*, immutable
```

Type inference is **not part of this RFC** but `let`/`var` are designed to support it.

---

## 4. Feature: `guard` Statement

### 4.1 Current Syntax

```c
fn process_packet(@borrowed Packet* pkt) -> Result<(), NetError> {
    @nullable Header* hdr = pkt->header;
    if (hdr == null) {
        return Err(NetError::MalformedPacket);
    }
    // hdr is still @nullable here — no narrowing

    @nullable Payload* payload = pkt->payload;
    if (payload == null) {
        return Err(NetError::NoPayload);
    }
    // Deep nesting begins...

    process(hdr, payload);
    return Ok(());
}
```

### 4.2 Proposed Syntax

```c
fn process_packet(@borrowed Packet* pkt) -> Result<(), NetError> {
    guard hdr = pkt->header else {
        return Err(NetError::MalformedPacket);
    }
    // hdr is non-null and in scope for the rest of the function

    guard payload = pkt->payload else {
        return Err(NetError::NoPayload);
    }

    process(hdr, payload);
    return Ok(());
}
```

### 4.3 Rules

1. **`guard <binding> = <expr> else { <diverging-block> }`** — The `else` block **must diverge**: it must `return`, `break`, `continue`, or `panic`. The compiler enforces this.
2. **Null narrowing** — When the expression is a nullable pointer, the bound variable has its `@nullable` stripped. The variable is guaranteed non-null after the `guard`.
3. **Scope** — The bound variable is in scope for the **remainder of the enclosing block**, not just inside the guard.
4. **Boolean guard** — `guard <condition> else { ... }` (without binding) is also valid:

```c
guard count > 0 else {
    return Err(Error::InvalidCount);
}
```

5. **Multiple bindings** — Multiple bindings in a single guard are **not supported**. Use separate `guard` statements for clarity.

### 4.4 Interaction with Ownership

`guard` respects ownership annotations:

```c
fn consume(@owned Buffer* buf) -> Result<(), Error> {
    guard inner = buf->data else {
        free(buf);
        return Err(Error::NoData);
    }
    // inner is @borrowed by default
    // buf is still @owned — caller must still manage it

    process(inner);
    free(buf);
    return Ok(());
}
```

---

## 5. Feature: `protocol` Keyword

### 5.1 Current Status

RFC-0003 mentions "interfaces" informally but does not specify a keyword or syntax. Generic functions use unconstrained type parameters:

```c
fn swap<T>(T* a, T* b);  // T is unconstrained — any type accepted
```

### 5.2 Proposed Syntax

```c
protocol Allocator {
    fn alloc<T>(usize count) -> Result<@owned T*, MemError>;
    fn dealloc<T>(@owned T* ptr, usize count);
}

protocol Hashable {
    fn hash(@borrowed Self* self) -> u64;
}

protocol Comparable {
    fn compare(@borrowed Self* self, @borrowed Self* other) -> i32;
}
```

### 5.3 Implementing a Protocol

```c
struct ArenaAllocator {
    @owned u8* base;
    usize offset;
    usize capacity;
}

impl Allocator for ArenaAllocator {
    fn alloc<T>(usize count) -> Result<@owned T*, MemError> {
        // arena allocation logic
    }

    fn dealloc<T>(@owned T* ptr, usize count) {
        // no-op for arena
    }
}
```

### 5.4 Protocol as Generic Constraint

```c
fn sort<T: Comparable>(T[] items, usize count) {
    // T must implement Comparable
}

fn create_map<K: Hashable + Comparable, V>(usize capacity) -> Result<@owned Map<K, V>*, MemError> {
    // K must implement both Hashable and Comparable
}
```

### 5.5 Rules

1. **`protocol`** declares a set of function signatures that a type must implement.
2. **`impl Protocol for Type`** provides the implementation. This is checked at compile time.
3. **`Self`** refers to the implementing type within protocol definitions.
4. **No vtables by default** — Protocol dispatch is monomorphized (static). Dynamic dispatch via protocol pointers is opt-in with `@dynamic`:

```c
@dynamic protocol Drawable {
    fn draw(@borrowed Self* self, @borrowed Canvas* canvas);
}

// Usage: dynamic dispatch through protocol pointer
fn render(@borrowed Drawable* obj, @borrowed Canvas* canvas) {
    obj->draw(canvas);  // vtable call
}
```

5. **No protocol inheritance** in this RFC. Future RFCs may add protocol composition beyond `+` constraints.
6. **No default implementations** in this RFC. All protocol functions must be implemented by conforming types.

### 5.6 Rationale: `protocol` vs `interface` vs `trait`

| Keyword | Origin | Connotation |
|---------|--------|-------------|
| `interface` | Java/C#/Go | Implies dynamic dispatch, virtual methods |
| `trait` | Rust/Scala | Implies default implementations, mixins |
| `protocol` | Swift/Obj-C | Implies a contract — "you promise to provide these functions" |

`protocol` was chosen because it best matches C 2030's semantics: a **compile-time contract** with **no default behavior** and **static dispatch by default**.

---

## 6. Feature: Optional `?` Suffix for Nullable Pointers

### 6.1 Current Syntax

```c
@nullable @owned Buffer* buf = get_buffer();
if (buf != null) {
    use(buf);
}
```

### 6.2 Proposed Syntax

```c
@owned Buffer*? buf = get_buffer();
if (buf != null) {
    use(buf);
}
```

### 6.3 Rules

1. **`T*?`** is syntactic sugar for `@nullable T*`. They are interchangeable.
2. **`@nullable` remains valid** — the `?` suffix is an alternative, not a replacement.
3. **Position** — The `?` appears after the `*`: `i32*?`, `Buffer*?`, `@owned Buffer*?`.
4. **Non-pointer optionals are not supported** — `?` only applies to pointer types. There is no general `Optional<T>` wrapper type.
5. **Interaction with `guard`** — `?` pointers work naturally with `guard`:

```c
fn process(@owned Buffer*? buf) -> Result<(), Error> {
    guard b = buf else {
        return Err(Error::NullBuffer);
    }
    // b is @owned Buffer* (non-null)
    use(b);
    free(b);
    return Ok(());
}
```

### 6.4 Examples

```c
struct Node {
    i32 value;
    @owned Node*? next;   // nullable owned pointer to next node
}

fn find(i32 target, @borrowed Node*? head) -> @borrowed Node*? {
    var @borrowed Node*? current = head;
    while (current != null) {
        if (current->value == target) {
            return current;
        }
        current = current->next;
    }
    return null;
}
```

---

## 7. Feature: Parameter Labels

### 7.1 Current Syntax

```c
fn alloc<T>(usize count) -> Result<@owned T*, MemError>;
fn substring(string s, usize start, usize length) -> string;

// Call site:
alloc<u8>(1024);
substring(name, 0, 5);  // What do 0 and 5 mean?
```

### 7.2 Proposed Syntax

```c
fn alloc<T>(count: usize) -> Result<@owned T*, MemError>;
fn substring(string s, from start: usize, length: usize) -> string;

// Call site:
alloc<u8>(count: 1024);
substring(name, from: 0, length: 5);
```

### 7.3 Rules

1. **`label name: Type`** declares a parameter with an external label and internal name. At the call site, the label must be used: `from: 0`.
2. **`name: Type`** (single name) uses the same name as both label and internal name: `count: 1024`.
3. **`Type name`** (C-style, no label) remains the default for backwards compatibility. No label is required at the call site.
4. **`_ name: Type`** suppresses the label — the parameter has a name internally but no label at the call site.
5. **Mixing is allowed** — a function can have both labeled and unlabeled parameters.
6. **Labels are part of the function signature** for overload resolution purposes.

### 7.4 Examples

```c
// All three styles in one function:
fn copy(
    @borrowed Buffer* src,          // C-style, no label
    to dest: @borrowed Buffer* ,    // labeled: copy(src, to: dest)
    _ count: usize                  // suppressed: copy(src, to: dest, 1024)
) -> Result<usize, IOError>;

// Call site:
copy(source, to: destination, 1024);
```

### 7.5 Label Overloading

```c
fn move_to(to x: i32, _ y: i32);
fn move_by(by dx: i32, _ dy: i32);

move_to(to: 100, 200);
move_by(by: 10, 20);
```

### 7.6 Interaction with Function Pointers

Function pointer types **do not include labels**. Labels are erased at the type level:

```c
type CopyFn = fn(@borrowed Buffer*, @borrowed Buffer*, usize) -> Result<usize, IOError>;

CopyFn f = copy;   // valid — labels erased
f(src, dest, 1024); // no labels at call site through function pointer
```

---

## 8. Summary of Changes to Existing RFCs

| RFC | Section Affected | Change |
|-----|-----------------|--------|
| RFC-0002 (Lexical Grammar) | §4 Keywords | Add `let`, `var`, `guard`, `protocol`, `impl` as keywords |
| RFC-0002 (Lexical Grammar) | §6 Operators | Add `?` as pointer-type suffix |
| RFC-0003 (Type System) | §5 Pointer Types | Add `T*?` as alias for `@nullable T*` |
| RFC-0003 (Type System) | §9 Generics | Add protocol constraints (`<T: Protocol>`) |
| RFC-0004 (Ownership) | §3 Qualifiers | Note interaction with `let`/`var` binding immutability |
| RFC-0006 (Modules) | §4 Visibility | No changes — `export` remains the visibility keyword |
| RFC-0007 (Error Handling) | §5 Propagation | Note interaction between `guard` and `?` operator |

---

## 9. New Keywords

The following keywords are added to the reserved keyword list (RFC-0002 §4):

```
let     var     guard     protocol     impl
```

These are **context-free keywords** — they are reserved in all positions and cannot be used as identifiers. Existing C code using these as identifiers must be renamed during migration.

### 9.1 Migration Risk Assessment

| Keyword | Risk | Reason |
|---------|------|--------|
| `let` | Low | Rarely used as identifier in C codebases |
| `var` | Low | Rarely used as identifier in C codebases |
| `guard` | Medium | Used in some kernel code (e.g., `spin_lock_guard`) — but as part of a compound name, not standalone |
| `protocol` | Medium | Used in networking code — but typically as part of compounds like `protocol_id` |
| `impl` | Low | Rarely used as identifier in C |

---

## 10. Compiler Diagnostics

### 10.1 `let` Mutation Warning

```
error[E0401]: cannot assign to immutable binding 'max'
  --> src/main.c2030:12:5
   |
 8 | let i32 max = count * 2;
   |         --- binding declared as immutable with 'let'
   |
12 | max = 100;
   | ^^^^^^^^^ cannot assign
   |
   = help: consider using 'var' if mutation is intended
```

### 10.2 `guard` Non-Diverging Else

```
error[E0402]: 'guard' else block must diverge
  --> src/net.c2030:20:5
   |
20 | guard hdr = pkt->header else {
21 |     log_error("no header");
   |     ^^^^^^^^^^^^^^^^^^^^^^ this block does not return, break, continue, or panic
22 | }
   |
   = help: add 'return Err(...)' or 'panic(...)' to the else block
```

### 10.3 Missing Protocol Implementation

```
error[E0501]: type 'ArenaAllocator' does not fully implement protocol 'Allocator'
  --> src/alloc.c2030:30:1
   |
30 | impl Allocator for ArenaAllocator {
   | ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
   |
   = note: missing implementation for 'fn dealloc<T>(@owned T* ptr, usize count)'
```

---

## 11. Complete Example

A full example combining all proposed features:

```c
module net.tcp;

import net.socket;
import mem.alloc;

protocol Connectable {
    fn connect(@borrowed Self* self, @borrowed SocketAddr* addr) -> Result<@owned Connection*, NetError>;
    fn disconnect(@owned Self* self);
}

struct TcpClient {
    @owned Socket* sock;
    var bool connected;
}

impl Connectable for TcpClient {
    fn connect(@borrowed Self* self, @borrowed SocketAddr* addr) -> Result<@owned Connection*, NetError> {
        guard resolved = resolve(addr) else {
            return Err(NetError::DnsFailure);
        }

        let @owned Connection* conn = socket_connect(self->sock, resolved)?;
        return Ok(conn);
    }

    fn disconnect(@owned Self* self) {
        socket_close(self->sock);
        free(self);
    }
}

export fn create_client(at addr: @borrowed SocketAddr*) -> Result<@owned TcpClient*, NetError> {
    let @owned Socket* sock = socket_create(AF_INET, SOCK_STREAM)?;
    defer_on_err close(sock);  // only runs if function returns Err

    var @owned TcpClient* client = alloc<TcpClient>(count: 1)?;
    client->sock = move(sock);
    client->connected = false;

    guard conn = client->connect(addr) else {
        free(client);
        return Err(NetError::ConnectionRefused);
    }

    client->connected = true;
    return Ok(client);
}
```

---

## 12. Rejected Alternatives

### 12.1 Replace `fn` with `func`

Rejected. `fn` is already established in RFCs 0002–0007, is shorter, and is familiar to Rust developers. Changing it provides no safety or clarity benefit.

### 12.2 Remove Semicolons

Rejected. Semicolons enable unambiguous parsing, are expected by C developers, and removing them adds complexity to the grammar for minimal benefit.

### 12.3 Adopt `throws` / `try` for Error Handling

Rejected. While Swift's typed `throws` is elegant, it looks like exception-based error handling. C 2030's `Result<T, E>` with `?` is explicit, composable, and aligns with the kernel community's expectations. `guard` provides the early-return ergonomics that `throws` offers without the semantic confusion.

### 12.4 Type-After-Colon Declarations (`x: i32`)

Rejected for variable declarations. C 2030 retains type-first syntax (`i32 x`) for compatibility with C. However, parameter labels (§7) use `:` in a limited context where it improves readability without conflicting with C conventions.

### 12.5 Adopt `switch` / `case` Instead of `match`

Rejected. C's `switch` has fall-through semantics that are a major source of bugs. `match` (RFC-0002) is already exhaustive and non-fall-through. Reusing `switch` would create confusion about which semantics apply.

### 12.6 `if let` Syntax

Considered but deferred. `guard` covers the most common use case (early return on null). `if let` for conditional binding within a block may be proposed in a future RFC.

---

## 13. Implementation Priority

| Feature | Priority | Complexity | Depends On |
|---------|----------|------------|------------|
| `let` / `var` | P0 — High | Low | None |
| `guard` | P0 — High | Medium | Null narrowing in type checker |
| `T*?` sugar | P1 — Medium | Low | None |
| `protocol` + `impl` | P1 — Medium | High | Generics (RFC-0003) |
| Parameter labels | P2 — Low | Medium | Name mangling (RFC-0012) |

---

## 14. Open Questions

1. **Should `let` / `var` be required in new code?** Or should bare declarations (`i32 x = 10;`) remain idiomatic? A lint rule (`-Wno-bare-decl`) could encourage migration.

2. **Should `guard` support pattern matching?** e.g., `guard Ok(data) = read_file(path) else { ... }` — This overlaps with `match` and may add complexity.

3. **Should protocols support associated types?** e.g., `protocol Iterator { type Item; fn next(...) -> Item?; }` — Deferred to a future RFC but the design should not preclude it.

4. **Should parameter labels affect C ABI compatibility?** Labels are erased in function pointer types (§7.6), but should they also be erased in the exported symbol name?

---

## 15. References

* RFC-0002: Lexical Grammar, Tokens, and Keywords
* RFC-0003: Type System & Layout Rules
* RFC-0004: Ownership, Lifetimes & Memory Safety
* RFC-0007: Error Handling (No Exceptions)
* RFC-0012: ABI & Linker Contracts
* Swift Programming Language — [The Basics](https://docs.swift.org/swift-book/)
* Swift Evolution — [SE-0235: Typed Throws](https://github.com/apple/swift-evolution/blob/main/proposals/0235-add-result.md)

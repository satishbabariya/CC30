# RFC 0018

## `match` Expressions & Pattern Matching

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

This RFC specifies the **`match` expression** syntax, pattern types, exhaustiveness checking, and integration with C 2030's type system. `match` is used extensively in RFC-0007 (error handling) and RFC-0003 (enums/tagged unions) but was never formally specified.

---

## 2. Motivation

C's `switch` statement is notoriously error-prone:

* **Fall-through by default** — missing `break` causes silent bugs
* **No exhaustiveness checking** — missing cases go undetected
* **No destructuring** — cannot extract data from tagged unions
* **Statement, not expression** — cannot use in assignments

C 2030's `match` addresses all of these while enabling pattern matching for enums, tagged unions, structs, and error handling.

---

## 3. Basic Syntax

```c
match <scrutinee> {
    <pattern1> => <expr1>,
    <pattern2> => { <block> },
    <pattern3> => <expr3>,
}
```

* The **scrutinee** is the value being matched
* Each **arm** consists of a pattern, `=>`, and a body (expression or block)
* Arms are separated by commas
* The last comma is optional
* **No fall-through** — each arm is independent

---

## 4. `match` Is an Expression

`match` can be used anywhere an expression is expected:

```c
let label = match color {
    Color::Red   => "red",
    Color::Green => "green",
    Color::Blue  => "blue",
};

let abs_val = match x > 0 {
    true  => x,
    false => -x,
};
```

When used as an expression, all arms must return the same type.

---

## 5. Pattern Types

### 5.1 Literal Patterns

Match against compile-time known values:

```c
match status_code {
    200 => handle_ok(),
    404 => handle_not_found(),
    500 => handle_server_error(),
    _   => handle_unknown(),
}
```

Supported literals: integers, characters, booleans, string literals.

### 5.2 Wildcard Pattern

`_` matches any value without binding it:

```c
match result {
    Ok(_) => printf("success\n"),   // ignore the success value
    Err(_) => printf("failure\n"),
}
```

### 5.3 Identifier Pattern (Binding)

A bare identifier binds the matched value to a new variable:

```c
match get_value() {
    0 => printf("zero\n"),
    n => printf("non-zero: %d\n", n),  // n binds the value
}
```

### 5.4 Enum Variant Patterns

Match against enum variants:

```c
enum Direction : u8 { North, South, East, West }

match dir {
    Direction::North => move_up(),
    Direction::South => move_down(),
    Direction::East  => move_right(),
    Direction::West  => move_left(),
}
```

When matching within the same module, the enum name prefix may be omitted if unambiguous:

```c
match dir {
    North => move_up(),
    South => move_down(),
    East  => move_right(),
    West  => move_left(),
}
```

### 5.5 Tagged Union Destructuring

Extract data from tagged union variants:

```c
union Result<T, E> { Ok(T), Err(E) }

match result {
    Ok(data) => use_data(data),
    Err(e)   => handle_error(e),
}
```

Nested destructuring:

```c
union Option<T> { Some(T), None }

match get_connection() {
    Ok(Some(conn)) => use_connection(conn),
    Ok(None)       => printf("no connection available\n"),
    Err(e)         => handle_error(e),
}
```

### 5.6 Struct Destructuring

Extract fields from structs:

```c
struct Point { f32 x; f32 y; }

match origin {
    Point { x: 0.0, y: 0.0 } => printf("at origin\n"),
    Point { x, y: 0.0 }      => printf("on x-axis at %f\n", x),
    Point { x: 0.0, y }      => printf("on y-axis at %f\n", y),
    Point { x, y }            => printf("at (%f, %f)\n", x, y),
}
```

* `x: 0.0` matches a specific field value
* `x` alone binds the field value to a variable with the same name
* `x: val` binds the field to a differently named variable

### 5.7 Range Patterns

Match against a range of values using `..`:

```c
match ascii_char {
    'a'..'z' => printf("lowercase\n"),
    'A'..'Z' => printf("uppercase\n"),
    '0'..'9' => printf("digit\n"),
    _        => printf("other\n"),
}
```

Ranges are **inclusive on both ends**: `1..10` matches 1 through 10.

### 5.8 Or Patterns

Match against multiple patterns in a single arm using `|`:

```c
match color {
    Color::Red | Color::Blue => printf("primary\n"),
    Color::Green             => printf("also primary\n"),
}

match key {
    'q' | 'Q' => quit(),
    'h' | 'H' => show_help(),
    _          => ignore(),
}
```

### 5.9 Guard Clauses

Add boolean conditions to patterns using `if`:

```c
match result {
    Ok(n) if n > 0  => printf("positive: %d\n", n),
    Ok(n) if n == 0 => printf("zero\n"),
    Ok(n)           => printf("negative: %d\n", n),
    Err(e)          => handle_error(e),
}
```

Guards are evaluated **after** the pattern matches. The bound variables are available in the guard expression.

### 5.10 Reference Patterns

Dereference a pointer and match the underlying value:

```c
match &value {
    &0 => printf("zero\n"),
    &n => printf("value: %d\n", n),
}
```

---

## 6. Exhaustiveness Checking

### 6.1 Enums Must Be Exhaustive

The compiler rejects `match` expressions that do not cover all enum variants:

```c
enum State : u8 { Init, Running, Stopped }

match state {
    Init    => start(),
    Running => continue(),
    // ERROR: non-exhaustive — missing 'Stopped'
}
```

### 6.2 Wildcard as Catch-All

`_` matches all remaining cases:

```c
match state {
    Running => continue(),
    _       => idle(),  // covers Init and Stopped
}
```

### 6.3 Integer and Boolean Exhaustiveness

* `bool`: must cover `true` and `false` (or use `_`)
* Integers: exhaustiveness is impractical; `_` catch-all is required unless all values are enumerated

### 6.4 Unreachable Patterns

The compiler warns on patterns that can never be reached:

```c
match x {
    _ => default(),   // catches everything
    0 => special(),   // WARNING: unreachable pattern
}
```

---

## 7. Binding Modes and Ownership

### 7.1 Default: Borrow

By default, pattern matching **borrows** the matched data. Bound variables are `@borrowed`:

```c
match result {
    Ok(data) => use_data(data),  // data is @borrowed
    Err(e)   => log_error(e),
}
// result is still valid
```

### 7.2 Move Matching

Use `move` before the scrutinee to **consume** the matched value:

```c
match move result {
    Ok(data) => consume_data(data),  // data is @owned
    Err(e)   => handle_error(e),     // e is @owned
}
// result is invalid — ownership transferred
```

### 7.3 Ref Binding

Explicitly borrow a bound variable:

```c
match large_struct {
    MyStruct { ref field1, ref field2 } => {
        // field1, field2 are @borrowed references
    },
}
```

---

## 8. `if let` — Single-Pattern Match

### 8.1 Syntax

```c
if let <pattern> = <expr> {
    // body — pattern matched
} else {
    // optional — pattern did not match
}
```

### 8.2 Examples

```c
if let Ok(data) = read_file(path) {
    process(data);
} else {
    printf("read failed\n");
}

if let Some(value) = map.get(key) {
    use(value);
}
```

### 8.3 Scope

The bound variable is in scope only within the `if let` body, not the `else` branch.

---

## 9. `while let` — Loop While Pattern Matches

### 9.1 Syntax

```c
while let <pattern> = <expr> {
    // body — continues while pattern matches
}
```

### 9.2 Examples

```c
while let Some(item) = iter.next() {
    process(item);
}

while let Ok(packet) = receive() {
    handle(packet);
}
```

---

## 10. `guard let` — Early Return on Mismatch

Integrates with RFC-0015's `guard` statement:

```c
guard let Ok(data) = parse(input) else {
    return Err(ParseError::Invalid);
}
// data is in scope for the rest of the function
use(data);
```

Rules:

* The `else` block must diverge (return, break, continue, or panic)
* The bound variable is in scope for the remainder of the enclosing block
* Combines pattern matching with early-return ergonomics

---

## 11. Interaction with Error Handling (RFC-0007)

`match` is the primary mechanism for explicit `Result` handling:

```c
fn process(@owned string path) -> Result<(), Error> {
    let result = read_file(path);

    match result {
        Ok(data) => {
            transform(data);
            return Ok(());
        },
        Err(IOError::NotFound) => {
            return Err(Error::MissingFile);
        },
        Err(IOError::PermissionDenied) => {
            return Err(Error::AccessDenied);
        },
        Err(e) => {
            return Err(Error::IO(e));
        },
    }
}
```

The `?` operator (RFC-0007 §5.1) is syntactic sugar for:

```c
// x? desugars to:
match x {
    Ok(val) => val,
    Err(e)  => return Err(e),
}
```

---

## 12. Compiler Diagnostics

### 12.1 Non-Exhaustive Match

```
error[E1801]: non-exhaustive match expression
  --> src/state.c30:15:5
   |
15 | match state {
   |       ^^^^^ pattern 'Stopped' not covered
   |
   = help: add a 'Stopped => ...' arm or use '_' as a catch-all
```

### 12.2 Unreachable Pattern

```
warning[W1802]: unreachable pattern
  --> src/main.c30:20:5
   |
18 |     _ => default(),
   |     - this arm catches all remaining values
19 |     0 => special(),
   |     ^ unreachable pattern
```

### 12.3 Type Mismatch in Expression Match

```
error[E1803]: match arms have incompatible types
  --> src/main.c30:12:5
   |
12 |     true  => 42,        // i32
13 |     false => "hello",   // string
   |              ^^^^^^^ expected 'i32', found 'string'
```

---

## 13. Complete Examples

### 13.1 State Machine

```c
enum State : u8 {
    Idle,
    Connecting,
    Connected,
    Disconnecting,
    Error,
}

fn handle_event(State state, Event event) -> State {
    return match state {
        Idle => match event {
            Event::Connect => State::Connecting,
            _              => State::Idle,
        },
        Connecting => match event {
            Event::Success => State::Connected,
            Event::Timeout => State::Error,
            _              => State::Connecting,
        },
        Connected => match event {
            Event::Disconnect => State::Disconnecting,
            Event::Error(e)   => State::Error,
            _                 => State::Connected,
        },
        Disconnecting => match event {
            Event::Success => State::Idle,
            Event::Timeout => State::Error,
            _              => State::Disconnecting,
        },
        Error => State::Idle,  // reset
    };
}
```

### 13.2 AST Walker

```c
union Expr {
    Literal(i64),
    Add(@owned Expr*, @owned Expr*),
    Mul(@owned Expr*, @owned Expr*),
    Neg(@owned Expr*),
}

fn eval(@borrowed Expr* expr) -> i64 {
    return match *expr {
        Literal(n)    => n,
        Add(lhs, rhs) => eval(lhs) + eval(rhs),
        Mul(lhs, rhs) => eval(lhs) * eval(rhs),
        Neg(inner)     => -eval(inner),
    };
}
```

### 13.3 Packet Parser

```c
fn parse_packet(@borrowed u8[] data) -> Result<Packet, ParseError> {
    guard data.length >= 4 else {
        return Err(ParseError::TooShort);
    }

    let version = data[0];
    let ptype = data[1];

    match ptype {
        0x01 => {
            guard let Ok(ping) = parse_ping(data[2..]) else {
                return Err(ParseError::MalformedPing);
            }
            return Ok(Packet::Ping(ping));
        },
        0x02 => {
            guard let Ok(pong) = parse_pong(data[2..]) else {
                return Err(ParseError::MalformedPong);
            }
            return Ok(Packet::Pong(pong));
        },
        0x03 => {
            guard let Ok(payload) = parse_data(data[2..]) else {
                return Err(ParseError::MalformedData);
            }
            return Ok(Packet::Data(payload));
        },
        _ => return Err(ParseError::UnknownType(ptype)),
    }
}
```

---

## 14. Rejected Alternatives

### 14.1 Enhance C `switch`

Rejected. C's `switch` has fall-through semantics deeply embedded in existing code. Changing `switch` behavior would break backwards compatibility. `match` is a new construct with clean semantics.

### 14.2 Use `:` Instead of `=>`

Rejected. `:` is already used for type annotations (RFC-0015 §7), struct field access, and enum backing types. `=>` is visually distinct and avoids ambiguity.

### 14.3 Allow Fall-Through

Rejected. Fall-through is the #1 source of `switch` bugs in C. Each `match` arm is independent. For shared logic, use or-patterns (`|`) instead.

### 14.4 Implicit Wildcard

Rejected. Requiring exhaustiveness or explicit `_` prevents accidentally forgetting cases when new enum variants are added.

---

## 15. Open Questions

1. **Binding `@` patterns** — Should `x @ Pattern` be supported to bind the full value while also destructuring? (e.g., `p @ Point { x, y } => use_both(p, x)`)
2. **Constant patterns** — Should `comptime` constants be usable as patterns? (e.g., `match x { MAX_VALUE => ... }`)
3. **Slice patterns** — Should array/slice patterns be supported? (e.g., `[first, .., last]`)
4. **`match` without braces for single arm** — Should `match x { Ok(v) => v, _ => default }` be allowed on one line?

---

## 16. References

* RFC-0002: Lexical Grammar (§5.1 — `match` keyword, §7.7 — `=>` operator)
* RFC-0003: Type System (§8 — enums, §9 — tagged unions)
* RFC-0007: Error Handling (§5.2 — explicit match)
* RFC-0015: Swift-Inspired Syntax Refinements (§4 — `guard`)
* Rust Reference — [Patterns](https://doc.rust-lang.org/reference/patterns.html)
* Swift — [Pattern Matching](https://docs.swift.org/swift-book/documentation/the-swift-programming-language/patterns/)

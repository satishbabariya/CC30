# RFC 0002

## Lexical Grammar, Tokens, and Keywords

**Status:** Draft
**Category:** Language Core
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

This RFC defines the **lexical structure** of the C 2030 programming language, including character sets, tokens, keywords, identifiers, literals, comments, and whitespace handling.

The lexical grammar of C 2030 is designed to be:

* Largely compatible with existing C
* Deterministic and unambiguous
* Friendly to tooling and static analysis
* Resistant to preprocessor abuse

---

## 2. Character Set & Encoding

### 2.1 Source Encoding

* Source files must be encoded in **UTF-8**
* ASCII is a strict subset
* Non-ASCII characters are permitted in:

  * Comments
  * String literals
* Identifiers are restricted (see §4)

---

### 2.2 Line Endings

* `\n` (LF) is the canonical line ending
* Implementations must accept `\r\n` and normalize internally

---

## 3. Tokens

C 2030 source code is tokenized into the following categories:

* Identifiers
* Keywords
* Literals
* Operators
* Punctuation
* Comments
* Whitespace

Tokenization follows the **maximal munch rule**.

---

## 4. Identifiers

### 4.1 Syntax

```ebnf
identifier ::= letter ( letter | digit | '_' )*
letter     ::= 'A'..'Z' | 'a'..'z'
digit      ::= '0'..'9'
```

### 4.2 Restrictions

* Identifiers must not begin with `_` followed by an uppercase letter
* Identifiers starting with `__` are reserved
* Unicode identifiers are **not permitted** (by design)

---

### 4.3 Case Sensitivity

Identifiers are **case-sensitive**.

---

## 5. Keywords

Keywords are reserved and may not be used as identifiers.

---

### 5.1 Control Flow Keywords

```
if
else
switch
match
for
while
do
break
continue
return
goto
defer
guard
```

---

### 5.2 Declaration & Visibility Keywords

```
fn
struct
union
enum
type
const
static
extern
export
import
module
let
var
protocol
impl
```

---

### 5.3 Type & Memory Keywords

```
void
bool
unsafe
closure
```

> Note: Primitive types (`i32`, `u64`, etc.) are **builtin identifiers**, not keywords.

---

### 5.4 Concurrency Keywords

```
thread
spawn
join
atomic
```

---

### 5.5 Compile-Time Keywords

```
comptime
```

---

### 5.6 Legacy C Keywords (Reserved)

The following keywords are reserved for compatibility and may be used only in legacy mode:

```
auto
register
volatile
restrict
sizeof
typedef
inline
_Alignas
_Alignof
_Static_assert
```

---

## 6. Literals

### 6.1 Integer Literals

Supported bases:

* Decimal: `42`
* Hexadecimal: `0x2A`
* Binary: `0b101010`
* Octal: `0o52`

Suffixes:

```
u, i, l, ll
```

Examples:

```c
42
0xff_u8
0b1010_i32
```

---

### 6.2 Floating-Point Literals

```c
3.14
1.0e-9
```

Suffixes:

```
f32
f64
```

---

### 6.3 Boolean Literals

```
true
false
```

---

### 6.4 Character Literals

```c
'a'
'\n'
'\x41'
```

* UTF-8 codepoints allowed
* Size is implementation-defined unless explicitly cast

---

### 6.5 String Literals

```c
"hello"
"hello\nworld"
```

Properties:

* UTF-8 encoded
* Immutable
* Null-terminated **only in legacy mode**

Raw strings:

```c
r"no escaping here"
```

---

## 7. Operators

### 7.1 Arithmetic

```
+  -  *  /  %
```

---

### 7.2 Comparison

```
==  !=  <  <=  >  >=
```

---

### 7.3 Logical

```
&&  ||  !
```

---

### 7.4 Bitwise

```
&  |  ^  ~  <<  >>
```

---

### 7.5 Assignment

```
=  +=  -=  *=  /=  %=  <<=  >>=  &=  |=  ^=
```

---

### 7.6 Pointer & Addressing

```
*   &   ?
```

* `?` as a **type suffix** on pointers denotes nullable: `T*?` is equivalent to `@nullable T*` (see RFC-0015 §6)
* `?` as a **postfix operator** on expressions propagates errors from `Result` types (see RFC-0007 §5.1)

---

### 7.7 Range & Pattern Operators

```
..
```

(Used in `match` and compile-time expressions.)

---

## 8. Punctuation

```
( ) { } [ ]
; , .
: ::
-> =>
@
```

* `@` introduces attributes and annotations
* `=>` used in `match` expressions
* `::` used for module paths

---

## 9. Comments

### 9.1 Single-Line Comments

```c
// this is a comment
```

---

### 9.2 Multi-Line Comments

```c
/* this is a comment */
```

* Nesting is permitted

---

### 9.3 Documentation Comments

```c
/// Function documentation
/** Struct documentation */
```

Used by tooling and documentation generators.

---

## 10. Whitespace

* Whitespace separates tokens but has no semantic meaning
* Newlines are not significant
* Indentation is stylistic only

---

## 11. Attributes & Annotations

Attributes are introduced with `@`.

```c
@packed
@aligned(8)
@deprecated("use new_api")
```

Annotations may apply to:

* Types
* Variables
* Functions
* Parameters

---

## 12. Preprocessing

### 12.1 Legacy Preprocessor Support

* `#include`, `#define`, `#if` supported in legacy mode
* Preprocessing occurs **before tokenization**

---

### 12.2 Modern Code Restrictions

* Preprocessor macros are forbidden in C 2030 modules
* Compile-time logic must use `comptime`

---

## 13. Error Handling in Lexing

A conforming compiler must:

* Provide clear diagnostics
* Reject invalid UTF-8
* Reject unknown tokens
* Avoid token recovery that changes semantics

---

## 14. Rationale & Design Notes

* ASCII-only identifiers improve tooling and readability
* Explicit keywords reduce ambiguity
* Keeping legacy keywords reserved avoids breakage
* `@` attributes replace compiler-specific pragmas

---

## 15. Summary

This RFC defines a **stable, explicit, and tool-friendly lexical foundation** for C 2030, preserving C familiarity while enabling modern language features.

---

## 16. Next RFC

* **RFC 0003 — Type System & Layout Rules**


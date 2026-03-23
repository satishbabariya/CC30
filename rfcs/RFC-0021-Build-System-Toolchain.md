# RFC 0021

## Build System & Toolchain

**Status:** Draft
**Category:** Language Infrastructure
**Edition:** 2030
**Author:** C 2030 Working Group
**Last Updated:** 2026-03-23

---

## 1. Abstract

This RFC specifies the **standard build system**, **compiler invocation**, **file conventions**, and **toolchain** for C 2030. No previous RFC defines file extensions, project structure, compiler flags, or build configuration.

---

## 2. Motivation

C has no standard build system. The ecosystem is fragmented across Make, CMake, Autotools, Meson, Ninja, and custom scripts. This leads to:

* Inconsistent build configuration across projects
* Difficulty onboarding new contributors
* No standard test discovery or documentation generation
* Build system bugs that are separate from language bugs

C 2030 provides a **standard toolchain** that handles compilation, testing, formatting, and documentation, while remaining interoperable with existing C build systems.

---

## 3. File Extensions

| Extension | Purpose |
|-----------|---------|
| `.c30` | C 2030 source file |
| `.c30i` | Module interface file (compiler-generated) |
| `.c30o` | C 2030 object file |
| `.c` / `.h` | Legacy C source/header files (interop) |
| `project.c30.toml` | Project manifest |

### 3.1 Source File Naming

* One module per `.c30` file
* File path mirrors module path: `net/tcp.c30` contains `module net.tcp;`
* Alternative: `net/tcp/mod.c30` for modules with sub-modules

---

## 4. Compiler Invocation

### 4.1 The `c30c` Compiler

```
c30c <source-files> [options]          # compile specific files
c30c build [options]                    # build the current project
c30c check [options]                    # type-check without codegen
c30c test [options]                     # build and run tests
c30c fmt [options]                      # format source code
c30c doc [options]                      # generate documentation
c30c clean                              # remove build artifacts
```

### 4.2 Compilation Modes

```
c30c build                              # debug build (default)
c30c build --release                    # optimized release build
c30c build --target=aarch64-linux       # cross-compilation
```

---

## 5. Project Manifest (`project.c30.toml`)

### 5.1 Format

The project manifest uses TOML format:

```toml
[project]
name = "my_network_lib"
version = "1.0.0"
edition = "2030"
authors = ["Alice <alice@example.com>"]
description = "A safe networking library"

[dependencies]
crypto = { version = "2.1.0" }
io     = { version = "1.0.0", path = "../io" }

[build]
optimization = "debug"         # "debug", "release", "size"
target = "x86_64-linux"        # default: host platform

[comptime]
# Compile-time constants (RFC-0016) settable from manifest
CONFIG_SMP = true
CONFIG_NR_CPUS = 64

[test]
filter = "*"                   # default test filter pattern
timeout = 60                   # per-test timeout in seconds
```

### 5.2 Dependency Resolution

* Dependencies are resolved from a **local path** or a **package registry**
* Version constraints use semantic versioning: `">=1.0.0, <2.0.0"`
* Lock file (`project.c30.lock`) ensures reproducible builds

---

## 6. Compiler Flags

### 6.1 Optimization

| Flag | Level | Description |
|------|-------|-------------|
| `-O0` | None | No optimization (default for debug) |
| `-O1` | Basic | Local optimizations |
| `-O2` | Standard | Most optimizations (default for release) |
| `-O3` | Aggressive | All optimizations including auto-vectorization |
| `-Os` | Size | Optimize for binary size |

### 6.2 Debugging

| Flag | Description |
|------|-------------|
| `-g` | Emit debug information (DWARF) |
| `-g0` | No debug info |
| `--sanitize=address` | Enable AddressSanitizer |
| `--sanitize=thread` | Enable ThreadSanitizer |
| `--sanitize=undefined` | Enable UBSanitizer |

### 6.3 Warnings

| Flag | Description |
|------|-------------|
| `-Wall` | Enable all standard warnings |
| `-Werror` | Treat warnings as errors |
| `-Wno-<name>` | Disable specific warning |
| `-Wpedantic` | Enable strict conformance warnings |
| `-Wno-bare-decl` | Suppress warnings for bare declarations (no `let`/`var`) |

### 6.4 Code Generation

| Flag | Description |
|------|-------------|
| `--emit=asm` | Emit assembly |
| `--emit=obj` | Emit object file (default) |
| `--emit=llvm-ir` | Emit LLVM IR |
| `--emit=c30i` | Emit module interface only |

### 6.5 Compile-Time Constants

```
c30c build -D CONFIG_SMP=true -D CONFIG_NR_CPUS=64
```

`-D` flags set `comptime` constants (RFC-0016), replacing C's `-D` for preprocessor defines.

### 6.6 Cross-Compilation

```
c30c build --target=aarch64-linux --sysroot=/path/to/sysroot
```

Target triples follow the format: `<arch>-<os>[-<abi>]`

| Component | Examples |
|-----------|---------|
| `arch` | `x86_64`, `aarch64`, `riscv64`, `arm` |
| `os` | `linux`, `none` (bare-metal), `freebsd` |
| `abi` | `gnu`, `musl`, `eabi` (optional) |

---

## 7. Module Resolution

### 7.1 Search Order

When the compiler encounters `import net.tcp;`, it searches:

1. **Project source tree**: `<project-root>/net/tcp.c30`
2. **Dependencies**: `<dep-path>/net/tcp.c30` (as declared in manifest)
3. **Standard library**: `<stdlib-path>/net/tcp.c30`

### 7.2 Module Interface Files

* The compiler generates `.c30i` files containing the **public interface** of each module
* Downstream modules compile against `.c30i` files, not source
* If a module's interface hasn't changed, dependents skip recompilation

### 7.3 Module Path Mapping

```
module net.tcp;        →  net/tcp.c30
module mm.page;        →  mm/page.c30
module kernel.sched;   →  kernel/sched.c30
```

---

## 8. Incremental Compilation

### 8.1 Strategy

* Module DAG (RFC-0006) drives recompilation order
* **Content-hash invalidation** — recompile only when source content changes (not timestamps)
* Interface stability — if a module's `.c30i` is unchanged, dependents are not recompiled
* Build cache stored in `.c30cache/`

### 8.2 Build Cache

```
.c30cache/
├── net/
│   ├── tcp.c30i          # module interface
│   ├── tcp.c30o          # object file
│   └── tcp.hash          # content hash
├── mm/
│   ├── page.c30i
│   ├── page.c30o
│   └── page.hash
└── build.lock            # build state
```

### 8.3 Parallel Compilation

Independent modules in the DAG are compiled in parallel. The compiler automatically determines the maximum parallelism from the dependency graph.

---

## 9. Linking

### 9.1 Default: Static Linking

```
c30c build                          # static binary
c30c build --shared                 # shared library (.so / .dylib)
c30c build --static-lib             # static library (.a)
```

### 9.2 C Interop Linking

```
c30c build --link-lib=pthread --link-lib=ssl
c30c build --link-path=/usr/local/lib
```

* Can link against `.o`, `.a`, `.so` files
* Symbol mangling follows RFC-0012

### 9.3 Link-Time Optimization (LTO)

```
c30c build --release --lto          # cross-module optimization
c30c build --release --lto=thin     # faster LTO with some trade-offs
```

---

## 10. Testing Framework

### 10.1 Test Declaration

```c
@test
fn test_addition() {
    assert_eq(add(2, 3), 5);
}

@test
fn test_overflow() {
    assert_panic(|| { add(i32::MAX, 1); });
}
```

### 10.2 Test Assertions

| Assertion | Description |
|-----------|-------------|
| `assert(expr)` | Fails if `expr` is false |
| `assert_eq(a, b)` | Fails if `a != b`, prints both values |
| `assert_ne(a, b)` | Fails if `a == b` |
| `assert_panic(closure)` | Fails if closure does not panic |
| `assert_err(result)` | Fails if result is `Ok` |
| `assert_ok(result)` | Fails if result is `Err` |

### 10.3 Running Tests

```
c30c test                           # run all tests
c30c test --filter "test_add*"      # run matching tests
c30c test --filter "net::*"         # run all tests in net module
c30c test --jobs=4                  # parallel test execution
c30c test --verbose                 # show all test output
```

### 10.4 Test Output

```
Running 42 tests from 8 modules...

  net::tcp::test_connect ............ OK (2ms)
  net::tcp::test_timeout ............ OK (1003ms)
  mm::page::test_alloc .............. OK (0ms)
  mm::page::test_double_free ........ OK (0ms)
  mm::page::test_oom ................ FAIL (1ms)
    assertion failed: assert_err(result)
    at mm/page.c30:145
    expected Err, got Ok(Page { ... })

Results: 41 passed, 1 failed, 0 skipped (1.2s)
```

---

## 11. Documentation Generation

### 11.1 Doc Comments

```c
/// Connects to a remote server at the given address.
///
/// Returns a connection handle on success, or a `TcpError`
/// if the connection cannot be established.
///
/// ## Example
///
/// ```c
/// let conn = connect(addr)?;
/// defer disconnect(conn);
/// ```
export fn connect(@borrowed SocketAddr* addr) -> Result<@owned Connection*, TcpError>;
```

### 11.2 Generation

```
c30c doc                            # generate HTML documentation
c30c doc --format=markdown          # generate Markdown
c30c doc --open                     # generate and open in browser
```

### 11.3 Cross-References

Doc comments can reference other symbols:

```c
/// See [`connect`] for establishing a connection.
/// Returns errors defined in [`TcpError`].
```

---

## 12. Formatter

### 12.1 Canonical Style

`c30c fmt` enforces a single canonical style (like `gofmt`):

| Rule | Convention |
|------|-----------|
| Indentation | 4 spaces |
| Functions, variables | `snake_case` |
| Types, protocols | `PascalCase` |
| `comptime` constants | `UPPER_SNAKE_CASE` |
| Braces | Same-line opening brace |
| Line length | 100 characters (soft limit) |
| Trailing commas | Required in multi-line lists |

### 12.2 Usage

```
c30c fmt                            # format all project files
c30c fmt --check                    # check without modifying
c30c fmt src/net/tcp.c30            # format specific file
```

---

## 13. Integration with Existing Build Systems

### 13.1 Makefile Generation

```
c30c build --emit-makefile > Makefile
```

### 13.2 CMake Integration

```cmake
find_package(C2030 REQUIRED)
add_c30_library(mylib src/lib.c30)
add_c30_executable(myapp src/main.c30)
target_link_c30_libraries(myapp mylib)
```

### 13.3 Kernel Build System (Kbuild)

C 2030 can be invoked as a compiler within Kbuild:

```makefile
# Kbuild integration
CC30 = c30c
obj-y += net/tcp.c30o

%.c30o: %.c30
	$(CC30) -c $< -o $@ $(C30FLAGS)
```

---

## 14. Project Layout Example

```
my_project/
├── project.c30.toml
├── project.c30.lock
├── src/
│   ├── main.c30                    # module main
│   ├── net/
│   │   ├── tcp.c30                 # module net.tcp
│   │   └── udp.c30                 # module net.udp
│   └── crypto/
│       └── sha256.c30              # module crypto.sha256
├── tests/
│   ├── net_test.c30                # integration tests
│   └── crypto_test.c30
├── .c30cache/                      # build cache (gitignored)
└── target/                         # build output (gitignored)
    ├── debug/
    │   └── my_project
    └── release/
        └── my_project
```

---

## 15. Rejected Alternatives

### 15.1 Use CMake As-Is

Rejected. CMake is powerful but:
* Syntax is archaic and error-prone
* Not purpose-built for C 2030's module system
* Cannot leverage module DAG for incremental compilation
* C 2030 needs first-class test/doc/fmt support

### 15.2 Cargo Clone

Rejected. Cargo is excellent but:
* C 2030 must interoperate with C build systems (Make, Kbuild)
* Cargo's opinion about project structure is too rigid for kernel code
* C 2030's `c30c` is both compiler and build tool — simpler model

### 15.3 No Standard Build System

Rejected. Fragmentation is C's #1 ecosystem problem. A standard toolchain lowers the barrier to adoption and ensures consistent project structure.

---

## 16. Open Questions

1. **Package registry** — Should C 2030 have a centralized package registry (like crates.io)? Or rely on Git URLs and local paths?
2. **Vendoring** — Should dependencies be vendored (copied into the project) by default?
3. **Build scripts** — Should projects support custom build scripts for code generation? (`build.c30` that runs before compilation?)
4. **Workspace support** — Should multi-project workspaces (monorepos) be first-class?
5. **IDE protocol** — Should `c30c` implement LSP (Language Server Protocol) directly?

---

## 17. References

* RFC-0006: Modules & Visibility
* RFC-0011: Language Tooling & Static Analysis
* RFC-0012: ABI & Linker Contracts
* RFC-0014: Language Evolution & Versioning
* RFC-0016: Compile-Time Execution
* Cargo — [The Cargo Book](https://doc.rust-lang.org/cargo/)
* Go Build System — [How to Write Go Code](https://go.dev/doc/code)
* Zig Build System — [Build System](https://ziglang.org/documentation/master/#Build-System)
* Meson Build System — [meson-build.com](https://mesonbuild.com/)

# CC30 - C 2030

## CC30 - C 2030 Compiler Repo Structure (C++ + LLVM) 

```
TBD
```

## CC30 Compiler Repository Structure (Proposed) (Self-contained)
```
cc30/                          # Root of the compiler repo
├── rfcs/                      # All RFC documents
│   ├── RFC-0001-Overview.md
│   ├── RFC-0002-Lexical-Grammar.md
│   ├── ...
│   └── RFC-0014-Language-Evolution.md
│
├── src/                        # Compiler core source code
│   ├── compiler/               # Compiler internals
│   │   ├── lexer/
│   │   ├── parser/
│   │   ├── semantic/
│   │   ├── codegen/
│   │   ├── backend/            # Target-specific backends (x86, ARM, WASM)
│   │   └── main.cc30           # Compiler entry point
│   │
│   └── boot/                   # Compiler bootstrapping and prelude
│
├── lib/                        # Libraries (standard, optional, experimental)
│   ├── std/                     # Standard library (collections, I/O, string, etc.)
│   ├── alloc/                   # Allocators (global, arenas, pools)
│   ├── concurrency/             # Threading, atomics, synchronization
│   ├── diagnostics/             # Runtime diagnostics, profiling
│   ├── math/                    # Math utilities
│   ├── fs/                      # File system utilities / VFS
│   ├── net/                     # Networking (TCP/UDP, sockets)
│   ├── experimental/            # Optional or experimental libraries
│   └── third_party/             # External libraries packaged for C2030
│
├── tools/                       # CLI and developer tooling
│   ├── cc30-cli/               # Compiler CLI wrapper
│   │   └── main.cc30
│   ├── cc30-format/            # Code formatter (like clang-format)
│   ├── cc30-lint/              # Static analysis / linter
│   ├── cc30-debug/             # Debugger interface / runtime debugging hooks
│   ├── cc30-profiler/          # Runtime profiler & memory/ownership tracking
│   ├── cc30-langserver/        # Language server for IDE integration
│   └── cc30-pkg/               # Package manager for C2030 libraries
│
├── include/                     # Public headers for C interop / embedding
│
├── tests/                       # Unit, integration, and system tests
│   ├── compiler/                # Compiler internal tests
│   ├── lib/                     # Library tests
│   ├── examples/                # Sample C2030 code used for testing
│   └── kernel/                  # Kernel/embedded target tests
│
├── examples/                    # Example programs for users
│   ├── hello_world/
│   ├── fs_demo/
│   ├── concurrency_demo/
│   └── embedded_demo/
│
├── scripts/                     # Build scripts, CI/CD helpers
│   ├── build.sh
│   ├── test.sh
│   └── package.sh
│
├── docs/                        # Documentation
│   ├── getting_started.md
│   ├── compiler_architecture.md
│   ├── libraries.md
│   ├── tooling.md
│   └── migration_playbook.md   # From RFC 0010
│
├── benchmarks/                  # Performance tests and benchmarks
│
├── ci/                          # Continuous integration configuration
│
├── .gitignore
├── LICENSE
└── README.md
```


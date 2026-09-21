# Nyan

**Nyan** is an experimental systems programming language targeting Linux x86-64. The compiler is written in C11 and emits executable ELF files directly, without generating intermediate C code or invoking a system linker.

The language is currently in pre-alpha state. The current implementation supports basic I/O, integer expressions evaluated at compile time, and exit status return.

---

## Table of Contents

- [Installation](#installation)
- [Quick Start](#quick-start)
- [Documentation](#documentation)
- [Current Features](#current-features)
- [Compiler Architecture](#compiler-architecture)
- [Development Status](#development-status)

---

## Installation

### Requirements

- Linux x86-64
- C11 compiler (GCC 11+ or Clang 14+)
- CMake 3.20 or newer

### Building the Compiler

```sh
# Clone the repository (if you haven't already)
git clone https://github.com/your-repo/nyan.git
cd nyan

# Build the compiler
cmake -S . -B build -G Ninja
cmake --build build
```

Alternatively, without Ninja:
```sh
cmake -S . -B build
cmake --build build
```

Result: `build/nyanc` — the Nyan compiler.

### Verify the Build

```sh
./build/nyanc --help
```

Should output:
```
usage: nyanc <input.nyan> [-o <output>]
```

---

## Quick Start

Create a file `hello.nyan`:

```nyan
link <std.io>

fn main() {
    io.print("Hello, World!\n")
}
```

Compile and run:

```sh
./build/nyanc hello.nyan
./hello
```

The compiler creates an executable file named `hello`. Use `-o` to specify a different output name:

```sh
./build/nyanc hello.nyan -o nyan-hello
./nyan-hello
```

---

## Documentation

The complete language specification is available in [SYNTAX_SPEC.md](docs.md).

It covers:
- [Syntax](#) — literals, expressions, operators, statements
- [Semantics](#) — expression evaluation, execution order
- [Standard Library](#) — std.io module
- [Diagnostics](#) — common compilation errors
- [Examples](#) — from simple to more complex programs
- [Limitations](#) — current implementation constraints
- [Roadmap](#) — planned features

---

## Current Features

### Supported Syntax

#### Literals

- **Integer**: decimal (`42`), hexadecimal (`0xFF`), binary (`0b1010`), octal (`0o755`)
- **String**: with escape sequences (`\n`, `\t`, `\xNN`, etc.)

#### Expressions

- Arithmetic: `+`, `-`, `*`, `/`, `%`, `**` (exponentiation)
- Bitwise: `<<`, `>>`, `&`, `|`, `^`, `~`
- Unary: `+`, `-`, `!`, `~`
- Parentheses for grouping: `(expr)`

#### Statements

- `io.print("string")` — output a string to stdout
- `return expression` — return an exit status (must be the final statement)

#### Modules

- `link <std.io>` — import the standard I/O module

### Features

- **Compile-time evaluation**: All integer expressions are evaluated during compilation
- **Error checking**: Overflow, division by zero, invalid shifts are caught at compile time
- **Minimal ELF**: Generates compact executable files with no external dependencies
- **System calls**: Uses Linux syscalls for output and process exit

### Example Program

```nyan
link <std.io>

fn main() {
    io.print("2 + 2 = ")
    io.print("4\n")
    return (2 ** 5) + 10  # 42
}
```

---

## Compiler Architecture

The compiler follows a simple pipeline:

```
source → lexer → parser/AST → constant evaluator → ELF emitter
```

### Project Structure

```
.
├── CMakeLists.txt          # Build configuration
├── README.md               # This documentation
├── SYNTAX_SPEC.md          # Language specification
└── src/
    ├── main.c              # CLI and pipeline orchestration
    ├── lexer.c/h           # Lexical analyzer
    ├── parser.c/h          # Syntax parser
    ├── token.c/h           # Token definitions
    ├── ast.c/h             # Abstract syntax tree
    ├── evaluator.c/h       # Constant expression evaluator
    ├── elf.c/h             # ELF file generator
    └── diagnostic.c/h      # Error diagnostics system
```

### Compilation Stages

1. **Lexer** (`lexer.c`) — tokenizes the source code
2. **Parser** (`parser.c`) — builds AST from tokens
3. **Evaluator** (`evaluator.c`) — evaluates integer expressions at compile time
4. **ELF Emitter** (`elf.c`) — generates executable ELF file with embedded code and data

---

## Development Status

### Current Status: Pre-Alpha

The language is not yet ready for real-world use. The current implementation demonstrates the basic capabilities of the compiler.

### Next Milestones

1. Local variables
2. Runtime code generation for expressions
3. User-defined functions
4. Type system

### CI/CD

The project is built using GitHub Actions with:
- GCC
- Clang
- CMake
- Ninja

Both GCC and Clang compilers are supported.

---

## Contributing

Contributions are welcome:
- Bug reports via Issues
- Fixes via Pull Requests
- Language design discussions

Before submitting a PR, ensure:
1. Code matches the project style (see existing code)
2. All changes are tested
3. Build passes with both GCC and Clang

---

## License

The project is licensed under the MIT License (or as specified in the LICENSE file, if present).

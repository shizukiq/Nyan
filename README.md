# Nyan

Nyan is an experimental systems programming language for Linux x86-64. The compiler is written in C11 and emits ELF executables directly, without generating C or invoking a system linker.

The language is still small. Its first working slice can print strings, evaluate integer expressions at compile time, and return an exit status.

## Hello world

Create `hello.nyan`:

```nyan
link <std.io>

fn main() {
    io.print("Hello, World!\n")
}
```

Compile and run it:

```sh
./build/nyanc hello.nyan
./hello
```

`nyanc` names the output after the source file by default. Use `-o` when you want another path:

```sh
./build/nyanc hello.nyan -o nyan-hello
```

## Building the compiler

Nyan currently targets Linux x86-64. Building it requires a C11 compiler and CMake 3.20 or newer.

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Ninja is optional. Omit `-G Ninja` to use CMake's default generator.

## Current language

The compiler currently understands one `main` function containing `io.print` calls and an optional final `return` statement.

```nyan
link <std.io>

fn main() {
    io.print("nyan\n")
    return (2 ** 5) + 10
}
```

Implemented syntax:

- decimal, hexadecimal, binary, and octal integer literals;
- string literals with common escapes and `\xNN` bytes;
- unary `+`, `-`, `!`, and `~`;
- arithmetic, shifts, and bitwise expressions;
- `#` line comments;
- `link <std.io>` and `io.print("...")`;
- direct generation of Linux x86-64 ELF executables.

Integer expressions are checked during compilation. Overflow, division by zero, invalid shifts, malformed literals, and syntax errors produce diagnostics with the source path, line, and column.

The broader language design, including types, references, structures, modules, FFI, and the runtime model, lives in [SYNTAX_SPEC.md](SYNTAX_SPEC.md). Those sections describe the intended language and are not all implemented yet.

## Compiler layout

The compiler follows a small pipeline:

```text
source → lexer → parser/AST → constant evaluator → ELF emitter
```

The implementation is split by compiler responsibility under `src/`. `main.c` owns the CLI and pipeline orchestration. The ELF emitter writes a minimal `_start` routine that uses Linux system calls for output and process exit.

## Command line

```text
nyanc <input.nyan> [-o <output>]
nyanc --help
```

Errors are written to stderr and return a non-zero status. Successful compilation returns zero and creates an executable file.

## Development status

Nyan is pre-alpha. The next useful milestones are local variables, runtime expression code generation, user-defined functions, and the core type system. Changes should extend the existing compiler stages instead of adding feature-specific compilation paths.

Pushes and pull requests are compiled with both GCC and Clang in GitHub Actions.

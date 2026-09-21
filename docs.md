# Nyan Language Specification

**Nyan** is an experimental systems programming language targeting Linux x86-64. The compiler is written in C11 and emits ELF executables directly, without generating intermediate C code or invoking a system linker.

---

## Table of Contents

- [1. Overview](#1-overview)
- [2. Syntax](#2-syntax)
  - [2.1. Program Structure](#21-program-structure)
  - [2.2. Comments](#22-comments)
  - [2.3. Identifiers](#23-identifiers)
  - [2.4. Literals](#24-literals)
  - [2.5. Expressions](#25-expressions)
  - [2.6. Operators](#26-operators)
  - [2.7. Statements](#27-statements)
  - [2.8. Modules](#28-modules)
- [3. Semantics](#3-semantics)
  - [3.1. Expression Evaluation](#31-expression-evaluation)
  - [3.2. Execution Order](#32-execution-order)
  - [3.3. Exit Status](#33-exit-status)
- [4. Standard Library](#4-standard-library)
- [5. Diagnostics](#5-diagnostics)
- [6. Examples](#6-examples)
- [7. Current Limitations](#7-current-limitations)
- [8. Planned Features](#8-planned-features)
- [Appendix A: ELF Format](#appendix-a-elf-format)
- [Appendix B: Building the Compiler](#appendix-b-building-the-compiler)

---

## 1. Overview

Nyan is a minimal systems programming language with the following characteristics:

- **Static typing** — all expressions have known types at compile time
- **Direct machine code generation** — produces executable ELF files without intermediate representation
- **Minimal standard library** — only basic I/O through Linux system calls
- **Compile-time evaluation** — integer expressions are evaluated during compilation

The language aims to combine low-level control with expressive syntax and safety.

---

## 2. Syntax

### 2.1. Program Structure

A Nyan program consists of an optional module link directive and a required `main` function:

```nyan
link <std.io>

fn main() {
    io.print("Hello, World!\n")
    return 0
}
```

- The `link` directive must appear at the beginning of the file
- `main` is the only entry point
- Function body is enclosed in curly braces `{}`.

### 2.2. Comments

Single-line comments start with `#` and continue to the end of the line:

```nyan
# This is a comment
fn main() {
    io.print("Hello")  # also a comment
}
```

### 2.3. Identifiers

Identifiers can contain:
- Latin letters (a-z, A-Z)
- Digits (0-9)
- Underscore (`_`)

An identifier cannot start with a digit. The following words are reserved and cannot be used as identifiers:

- `fn` — function declaration
- `link` — module linking
- `main` — entry point name
- `return` — return statement

### 2.4. Literals

#### Integer Literals

Four integer literal formats are supported:

| Format | Example | Note |
|--------|---------|------|
| Decimal | `42`, `-123`, `0` | Signed integers |
| Hexadecimal | `0xFF`, `0x1A2B` | Prefix `0x` or `0X` |
| Binary | `0b1010`, `0b11110000` | Prefix `0b` or `0B` |
| Octal | `0o755`, `0o12` | Prefix `0o` or `0O` |

All integer literals have `int64` type (64-bit signed integer).

#### String Literals

String literals are enclosed in double quotes:

```nyan
"Hello, World!"
"Multiline\nString"
```

Supported escape sequences:

| Sequence | Character |
|----------|-----------|
| `\n` | Line feed (LF) |
| `\r` | Carriage return (CR) |
| `\t` | Tab |
| `\0` | Null byte |
| `\\` | Backslash |
| `\"` | Double quote |
| `\xNN` | Byte with hexadecimal value NN |

Example:
```nyan
io.print("Line 1\nLine 2\tTabbed\x41")  # A = 0x41
```

### 2.5. Expressions

An expression can be:

- A literal (integer or string)
- A unary operation
- A binary operation
- A parenthesized expression

Examples:
```nyan
42
-42
2 + 3 * 4
(1 + 2) * 3
!0
~0xFF
```

### 2.6. Operators

#### Unary Operators

| Operator | Name | Example | Result |
|----------|------|---------|--------|
| `+` | Unary plus | `+5` | 5 |
| `-` | Unary minus | `-5` | -5 |
| `!` | Logical NOT | `!0` | 1 |
| `!` | Logical NOT | `!5` | 0 |
| `~` | Bitwise NOT | `~0` | 0xFFFFFFFFFFFFFFFF |

Unary operators have the highest precedence.

#### Binary Operators

Operators ordered by precedence (highest to lowest):

| Precedence | Operator | Name | Associativity |
|------------|----------|------|---------------|
| 7 | `**` | Exponentiation | Right |
| 6 | `*`, `/`, `%` | Multiplication, division, remainder | Left |
| 5 | `+`, `-` | Addition, subtraction | Left |
| 4 | `<<`, `>>` | Bitwise shift left, right | Left |
| 3 | `&` | Bitwise AND | Left |
| 2 | `^` | Bitwise XOR | Left |
| 1 | `|` | Bitwise OR | Left |

Examples:
```nyan
2 ** 3 ** 2   # 2^(3^2) = 512 (right associativity)
2 + 3 * 4    # 2 + (3 * 4) = 14
1 << 2 + 3   # 1 << (2 + 3) = 32
```

#### Arithmetic Operators

- `+` — addition
- `-` — subtraction
- `*` — multiplication
- `/` — integer division (rounds toward zero)
- `%` — remainder
- `**` — exponentiation

#### Bitwise Operators

- `<<` — left shift (multiply by 2^n)
- `>>` — right shift (divide by 2^n)
- `&` — bitwise AND
- `|` — bitwise OR
- `^` — bitwise XOR
- `~` — bitwise NOT (unary)

### 2.7. Statements

#### Print Statement

```nyan
io.print("Hello, World!\n")
```

- Accepts a single string literal argument
- Outputs the string to standard output
- Semicolon is optional

#### Return Statement

```nyan
return 0
return 2 ** 5 + 10  # 42
```

- Terminates the `main` function
- Returns the exit status (0 = success, non-zero = error)
- Must be the final statement in the function
- The expression is evaluated at compile time

### 2.8. Modules

The current implementation supports only one module:

```nyan
link <std.io>
```

- The `link` directive must appear at the beginning of the program
- Imports the standard I/O module
- Required for using `io.print()`

---

## 3. Semantics

### 3.1. Expression Evaluation

In the current implementation, all integer expressions are evaluated at compile time. The compiler checks for:

- **Integer overflow** — when an operation result exceeds int64 limits
- **Division by zero** — for `/` and `%` operations
- **Negative exponent** — for `**` operator
- **Invalid shift** — shift by negative count or count >= 64

When an error is detected, compilation aborts with a diagnostic message.

### 3.2. Execution Order

1. All expressions in the `return` statement are evaluated (at compile time)
2. All string literals for `io.print` are collected
3. An ELF file is generated with embedded code and data
4. At program runtime:
   - All strings are output via the `write` system call
   - Exit status is returned via the `exit` system call

### 3.3. Exit Status

The program's exit status is determined by the `return` statement:

- If `return` is absent — exit status is 0
- If `return` is present — exit status equals the expression value
- The status is truncated to 8 bits (0-255)

---

## 4. Standard Library

### std.io Module

Functions:

```nyan
io.print(string)
```

- **Purpose**: Output a string to standard output
- **Arguments**: A single string literal
- **Return value**: None
- **Requires**: `link <std.io>`

---

## 5. Diagnostics

The compiler reports errors with:

- File path
- Line number
- Column number
- Error description

Common errors:

| Error | Cause |
|-------|-------|
| `unterminated string literal` | String not closed with a quote |
| `expected digits after integer prefix` | No digits after prefix (0x, 0b, 0o) |
| `integer literal overflow` | Integer literal exceeds int64 range |
| `division by zero` | Division by zero in expression |
| `shift count must be between 0 and 63` | Invalid shift amount |
| `io.print requires link <std.io>` | Using io.print without linking the module |

---

## 6. Examples

### Hello, World!

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

### Compile-Time Calculation

```nyan
link <std.io>

fn main() {
    io.print("2^8 + 2^7 + 2^6 = ")
    io.print("448\n")
    return (2 ** 8) + (2 ** 7) + (2 ** 6)
}
```

Check exit status:
```sh
./build/nyanc calc.nyan
./calc
echo $?  # Prints 240 (448 mod 256)
```

### Bitwise Operations

```nyan
link <std.io>

fn main() {
    io.print("0xFF & 0x0F = ")
    io.print("15\n")
    return 0xFF & 0x0F  # 15
}
```

### Escape Sequences

```nyan
link <std.io>

fn main() {
    io.print("Tab:\tValue\n")
    io.print("Hex byte A: \x41\n")
    io.print("Quote: \"Hello\"\n")
}
```

---

## 7. Current Limitations

The current compiler implementation supports only:

- Single `main` function
- `io.print()` and `return` statements
- Integer expressions evaluated at compile time
- Linking only the `std.io` module

Not supported (planned for future):

- Local variables
- User-defined functions
- Data types (other than int64)
- Structs
- Arrays
- Conditional statements (if/else)
- Loops
- Modules (other than std.io)
- FFI (C function calls)
- Dynamic memory allocation

---

## 8. Planned Features

In order of priority:

1. **Local variables** — declaration and use of variables within functions
2. **Runtime code generation** — code generation for expressions that cannot be evaluated at compile time
3. **User-defined functions** — function declaration and invocation
4. **Type system** — explicit and implicit types, type checking
5. **Structs** — composite data types
6. **Modules** — separate compilation, import/export
7. **FFI** — interoperability with C code
8. **Memory management** — manual memory management without GC

---

## Appendix A: ELF Format

The compiler generates a minimal executable ELF file for Linux x86-64:

- **Entry point**: `0x400000` + ELF header size + program header size
- **Code**: Embedded machine code that:
  1. Invokes `write` system call to output all strings
  2. Invokes `exit` system call with the specified status
- **Data**: String literals are stored immediately after the code
- **Permissions**: Executable and readable (`PF_R | PF_X`)

---

## Appendix B: Building the Compiler

Requirements:
- Linux x86-64
- C11 compiler (GCC or Clang)
- CMake 3.20 or newer

Build:
```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Or without Ninja:
```sh
cmake -S . -B build
cmake --build build
```

Result: `build/nyanc` — the Nyan compiler.

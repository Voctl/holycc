# holycc — HolyC Compiler for Linux

<p align="center">
  <img src="docs/logo.jpg" alt="HolyC Logo" width="350">
</p>

**HolyC → C17 transpiler**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![C17](https://img.shields.io/badge/language-C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![Build](https://img.shields.io/badge/build-cmake%20%7C%20gcc%20%7C%20clang-green.svg)]()

`holycc` is a modern HolyC-to-C17 transpiler that runs natively on Linux.
It reads HolyC source files (`.HC`) and generates clean, readable,
production-quality C17 code — then compiles it with GCC or Clang.

This is **NOT** based on TempleOS internals. The goal is **language compatibility**,
not operating system compatibility.

---

## Quick Start

```bash
# Clone
git clone https://github.com/YOU/holycc.git && cd holycc

# Build
cmake -B build && cmake --build build

# Install (adds to ~/.local/bin, sets up 'hc' alias)
./install.sh

# Compile HolyC to C
holycc program.HC          # or just: hc program.HC

# Compile generated C with GCC
gcc program.c -o program

# Or run directly
holycc program.HC -o out.c && gcc out.c -o program && ./program
```

---

## Example

**HolyC input** (`math.HC`):
```c
I64 Square(I64 x)
{
    return x * x;
}

F64 Half(F64 x)
{
    return x / 2.0;
}

class Vec2 {
    F64 x;
    F64 y;
};
```

**Generated C17** (`math.c`):
```c
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

int64_t Square(int64_t x) {
    return (x * x);
}

double Half(double x) {
    return (x / 2);
}

typedef struct {
    double x;
    double y;
} Vec2;
```

---

## Supported Features

### Types
| HolyC | C17 |
|---|---|
| `I8` / `I16` / `I32` / `I64` | `int8_t` / `int16_t` / `int32_t` / `int64_t` |
| `U8` / `U16` / `U32` / `U64` | `uint8_t` / `uint16_t` / `uint32_t` / `uint64_t` |
| `F64` | `double` |
| `Bool` | `bool` |
| `Char` | `char` |
| `void` | `void` |
| Pointers, Arrays, Structs, Unions, Enums | Direct mapping |

### Statements
- [x] `if` / `else`
- [x] `for` / `while` / `do`-`while`
- [x] `switch` / `case` / `default`
- [x] `return` / `break` / `continue`
- [x] `goto` / labels
- [x] `class` (mapped to `typedef struct`)
- [x] `union` / `enum`
- [x] `asm` / `_asm` (inline assembly — comment in C)
- [ ] `#include`
- [x] `#define` (maps to C #define)

### Expressions
- [x] Arithmetic (`+` `-` `*` `/` `%`)
- [x] Bitwise (`&` `|` `^` `~` `<<` `>>`)
- [x] Logical (`&&` `||` `!`)
- [x] Comparison (`==` `!=` `<` `>` `<=` `>=`)
- [x] Assignment (`=` `+=` `-=` `*=` `/=` `%=` `&=` `|=` `^=` `<<=` `>>=`)
- [x] Ternary (`a ? b : c`)
- [x] `sizeof`
- [x] Cast expressions `(Type)expr`
- [x] Member access (`.` `->`)
- [x] Array indexing `[]`
- [x] Function calls
- [x] `++` / `--` (prefix and postfix)

### Semantic Checks
- [x] Undeclared variable detection
- [x] Duplicate definition detection
- [x] Basic type checking (numeric, pointer, aggregate)
- [x] `break`/`continue` outside loop detection
- [x] Lvalue validation for assignments

---

## Compiler Pipeline

```
Source (.HC) → Lexer → Parser → AST → Semantic → CodeGen → C17 (.c)
```

| Stage | Module | Description |
|---|---|---|
| **Lexer** | `src/lexer/` | Tokenizes HolyC source into 114 token kinds |
| **Parser** | `src/parser/` | Recursive-descent parser, produces AST |
| **AST** | `src/ast/` | Tree node types with visitor pattern |
| **Semantic** | `src/semantic/` | Type checking, symbol resolution, scoping |
| **Symbol Table** | `src/symbol/` | Lexical scopes (global, function, block, struct) |
| **Type System** | `src/types/` | Type representation, size, alignment, C name mapping |
| **Code Generator** | `src/codegen/` | Walks AST, emits readable C17 code |
| **Runtime** | `runtime/` | `Print`, `MAlloc`, `Free`, `StrLen`, etc. |
| **CLI Driver** | `src/driver/` | Argument parsing, pipeline orchestration |

---

## Architecture

```
holycc/
├── include/holyc/     # Public headers (one per module)
├── src/
│   ├── lexer/         # Lexer implementation
│   ├── parser/        # Recursive descent parser
│   ├── ast/           # AST node types and visitors
│   ├── semantic/      # Semantic analysis and type checking
│   ├── symbol/        # Symbol table with lexical scoping
│   ├── types/         # Type system
│   ├── codegen/       # C17 code generation
│   ├── driver/        # CLI driver
│   ├── utils/         # String buffer, file I/O
│   ├── token.c        # Token enum table
│   ├── diag.c         # Diagnostics / error reporting
│   └── main.c         # Entry point
├── runtime/           # HolyC runtime library
├── tests/             # Test suite
├── examples/          # Example HolyC programs
├── docs/              # Documentation (contains logo.jpg, terry.jpg)
├── cmake/             # CMake modules
├── CMakeLists.txt     # Build configuration
└── README.md
```

## Coding Standards

- **Language**: C17
- **Compiler**: GCC 12+ / Clang 15+
- **Build**: CMake 3.16+
- **Warnings**: `-Wall -Wextra -Wpedantic` — zero warnings
- **Style**: Small functions, descriptive names, const correctness
- **No macros** (minimal), no globals, no god objects

---

## CLI Options

```
Usage: holycc [options] <input.HC>

Options:
  -o <file>      Output file
  -c             Compile only (no link)
  --emit-c       Emit C code (default)
  --tokens       Dump token stream
  --ast          Dump AST tree
  --help         Show help
  --version      Show version
```

---

## Testing

```bash
cmake --build build && ./build/tests/test_lexer
```

All lexer tests pass (161/161). Parser and codegen test suites coming soon.

---

## Roadmap

See [ROADMAP.md](ROADMAP.md) for planned features and future direction.

---

## License

GNU General Public License v3.0. See [LICENSE](LICENSE).

Free Software, Free Society. &mdash; RMS

---

## Credits

<p align="center">
  <img src="docs/terry.jpg" alt="Terry A. Davis" width="400">
</p>

HolyC is the programming language created by **Terry A. Davis** for TempleOS.
This project is a tribute to his work — reimplemented as a clean, portable,
modern compiler for Linux.

*"An operating system is a work of art." — Terry A. Davis*

# holycc &mdash; HolyC Compiler for Linux

<img align="right" src="docs/logo.jpg" alt="HolyC Logo" width="220">

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![C17](https://img.shields.io/badge/language-C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![Build](https://img.shields.io/badge/build-cmake%20%7C%20gcc%20%7C%20clang-green.svg)]()
[![Tests](https://img.shields.io/badge/tests-161%2F161-brightgreen.svg)]()

**A modern, clean-room HolyC &rarr; C17 transpiler.**  
Reads `.HC` source, understands it deeply, and produces
production-grade C17 &mdash; then hands it off to GCC or Clang
for the heavy lifting. All on Linux. All native. All free.

No TempleOS internals. No emulation. No runtime VM.
Just a compiler that treats HolyC as a real language worthy
of the modern FOSS toolchain.

<br clear="right"/>

---

## &sect; Philosophy

> *"Azad proqram təminatı, azad cəmiyyət."*  
> &mdash; Free software, free society.

`holycc` is built on four simple convictions:

| Principle | What it means in practice |
|---|---|
| **Portability over purity** | Target C17, not x86-64 ring 0. Run anywhere GCC runs. |
| **Readability is a feature** | Generated C should look handwritten. No mangled names, no spaghetti. |
| **Modularity or bust** | Every pass stands alone. Swap the backend, keep the frontend. |
| **Zero warnings, zero excuses** | `-Wall -Wextra -Wpedantic` with zero diagnostics. Always. |

---

## &sect; Quick Start

```bash
git clone https://github.com/YOU/holycc.git && cd holycc

cmake -B build && cmake --build build

./install.sh          # → ~/.local/bin, sets up 'hc' alias

holycc program.HC     # HolyC → C17
gcc program.c -o program && ./program
```

Or in one shot:

```bash
holycc program.HC -o out.c && gcc out.c -o program && ./program
```

---

## &sect; A Taste

<table>
<tr>
<td width="50%">

**`math.HC`** &mdash; HolyC input
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

</td>
<td width="50%">

**`math.c`** &mdash; Generated C17
```c
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

int64_t Square(int64_t x) {
    return (x * x);
}

double Half(double x) {
    return (x / 2.0);
}

typedef struct {
    double x;
    double y;
} Vec2;
```

</td>
</tr>
</table>

Clean. Predictable. Compiles with zero warnings.

---

## &sect; Feature Matrix

### &rarr; Type System

| HolyC type | Maps to C17 | Notes |
|---|---|---|
| `I8`, `I16`, `I32`, `I64` | `int8_t` &hellip; `int64_t` | Fixed-width signed integers |
| `U8`, `U16`, `U32`, `U64` | `uint8_t` &hellip; `uint64_t` | Fixed-width unsigned integers |
| `F64` | `double` | IEEE 754 double precision |
| `Bool` | `bool` | Via `<stdbool.h>` |
| `Char` | `char` | Single byte character |
| `void` | `void` | Same semantics |
| `class` / `union` / `enum` | `typedef struct` / `union` / `enum` | Direct structural mapping |
| Pointers, arrays, function pointers | Identical | `*`, `[]`, `(*)(...)` |

### &rarr; Statements

| Status | Statement | Notes |
|:---:|---|---|
| [ok] | `if` / `else` / `else if` | Fully implemented |
| [ok] | `for` / `while` / `do`-`while` | All three loop forms |
| [ok] | `switch` / `case` / `default` | With fallthrough |
| [ok] | `return` / `break` / `continue` | Loop context validation |
| [ok] | `goto` / labels | Unconditional jump |
| [ok] | `class` | Mapped to `typedef struct` |
| [ok] | `union` / `enum` | Direct C mapping |
| [ok] | `asm` / `_asm` | Preserved as comments in generated C |
| [ok] | `#define` | Simple macro mapping |
| [ok] | `#include` | File inlining with proper path resolution |

### &rarr; Expressions

| Status | Category | Operators |
|:---:|---|---|
| [ok] | Arithmetic | `+` `-` `*` `/` `%` |
| [ok] | Bitwise | `&` `\|` `^` `~` `<<` `>>` |
| [ok] | Logical | `&&` `\|\|` `!` |
| [ok] | Comparison | `==` `!=` `<` `>` `<=` `>=` |
| [ok] | Assignment | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` |
| [ok] | Ternary | `a ? b : c` |
| [ok] | Unary ops | `++` `--` (prefix &amp; postfix), `-`, `!`, `~` |
| [ok] | Cast | `(Type)expression` |
| [ok] | Member access | `.` and `->` |
| [ok] | Subscript | `array[index]` |
| [ok] | Address / deref | `&expr`, `*ptr` |
| [ok] | sizeof | `sizeof(type)` and `sizeof expr` |
| [ok] | Function call | `name(args...)` |

### &rarr; Semantic Analysis

| Status | Check | Description |
|:---:|---|---|
| [ok] | Undeclared variable | Catches use-before-declare |
| [ok] | Duplicate definition | Same name, same scope |
| [ok] | Type compatibility | Numeric, pointer, aggregate |
| [ok] | Loop context | `break`/`continue` only valid inside loops |
| [ok] | Lvalue validation | Left side of `=` must be assignable |
| [ok] | Return type check | Return expression vs function return type |

---

## &sect; Compiler Pipeline

```
  ┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
  │  Source  │ ──▶ │  Lexer   │ ──▶ │  Parser  │ ──▶ │ Semantic │ ──▶ │ CodeGen  │ ──▶ C17 (.c)
  │  (.HC)   │     │ 114 tok. │     │   AST    │     │  checks  │     │  emitter │
  └──────────┘     └──────────┘     └──────────┘     └──────────┘     └──────────┘
```

| # | Stage | Location | Responsibility |
|:--:|---|---|---|
| 1 | **Lexer** | `src/lexer/` | Character stream &rarr; 114 token kinds, keyword tables, literal parsing |
| 2 | **Parser** | `src/parser/` | Recursive-descent with precedence climbing, tokens &rarr; AST (45+ node kinds) |
| 3 | **AST** | `src/ast/` | Tree nodes, child/sibling pointers, visitor pattern for traversal |
| 4 | **Semantic** | `src/semantic/` | Symbol resolution, type checking, lexical scoping, error detection |
| 5 | **Symbol Table** | `src/symbol/` | Scope chain: Global &rarr; Function &rarr; Block &rarr; Struct |
| 6 | **Type System** | `src/types/` | Primitive + compound types, sizes, alignments, C name mapping |
| 7 | **Code Generator** | `src/codegen/` | Tree-walk emitter producing readable, indented C17 |
| 8 | **Runtime** | `runtime/` | `Print`, `MAlloc`, `Free`, `StrLen`, `StrCompare`, `AtoI`, `AtoF`, etc. |
| 9 | **CLI Driver** | `src/driver/` | Arg parsing, pipeline orchestration, debug flags (`--tokens`, `--ast`) |

---

## &sect; Project Layout

```
holycc/
├── include/holyc/        Public headers — one clean .h per module
├── src/
│   ├── lexer/            Lexer: characters → tokens
│   ├── parser/           Parser: tokens → AST
│   ├── ast/              AST node types & visitor infrastructure
│   ├── semantic/         Semantic analysis & type checking
│   ├── symbol/           Symbol table with lexical scoping
│   ├── types/            HolyC type representation & queries
│   ├── codegen/          C17 code emitter
│   ├── driver/           CLI entry point & argument handling
│   ├── utils/            String buffer, file I/O helpers
│   ├── token.c           Token kind ↔ string tables
│   ├── diag.c            Diagnostic engine with source snippets
│   └── main.c            Application entry point
├── runtime/              HolyC runtime — statically linked
├── tests/                Unit & integration test suites
├── examples/             Example .HC programs (hello, math, arrays, etc.)
├── docs/                 Architecture docs, images
├── cmake/                CMake helper modules
├── CMakeLists.txt        Top-level build definition
├── install.sh            One-command user install script
└── README.md             You are here
```

---

## &sect; Coding Covenant

Every line committed to `main` must meet these standards &mdash; no exceptions, no
negotiations, no `// TODO: fix later`.

| Rule | The standard |
|---|---|
| **Language** | C17 &mdash; no GNU extensions, no C11 leftovers |
| **Compiler** | GCC &ge; 12 or Clang &ge; 15 |
| **Build** | CMake &ge; 3.16 |
| **Warnings** | `-Wall -Wextra -Wpedantic` &mdash; zero diagnostics |
| **Functions** | Small, single-purpose, well-named |
| **State** | No globals. No mutable static state. |
| **Macros** | Almost never. Inline functions and enums instead. |
| **Nesting** | Three levels deep is a warning sign |
| **Const** | Everywhere it applies |

---

## &sect; CLI Reference

```
holycc — HolyC → C17 transpiler

Usage:
  holycc [options] <input.HC>

Options:
  -o <file>        Output to <file> (default: stdout)
  -c               Compile only, do not link
  --emit-c         Emit C code (default behaviour)
  --tokens         Dump token stream to stderr (for debugging)
  --ast            Dump AST tree to stderr (for debugging)
  --help           Show this message and exit
  --version        Print version string and exit

Examples:
  holycc game.HC                        # write C to stdout
  holycc game.HC -o game.c              # write C to file
  holycc game.HC --tokens               # debug: show tokens
  holycc game.HC --ast                  # debug: show AST
  holycc game.HC -o game.c && gcc game.c -o game && ./game
```

---

## &sect; Testing

```bash
cmake --build build
./build/tests/test_lexer
```

**Lexer suite:** 161/161 tests passing.

---

## &sect; Roadmap

See [ROADMAP.md](ROADMAP.md) for planned features and future direction.

---

## &sect; Contributing

We welcome contributors. Seriously.

`holycc` is designed to be approachable: every subsystem has clean boundaries,
no single file exceeds a few hundred lines, and the recursive-descent parser is
something you can literally read top-to-bottom and understand.

Before diving in, take 10 minutes with [CONTRIBUTING.md](CONTRIBUTING.md).
The short version:

1. Fork &amp; branch
2. Write code &mdash; zero warnings, please
3. Add tests for your change
4. Open a PR with a clear description

First time? Look for issues tagged `good first issue`. We'll help you land it.

---

## &sect; License

**GNU General Public License v3.0**

```
holycc — HolyC compiler for Linux
Copyright (C) 2024–2026  holycc contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
```

See [LICENSE](LICENSE) for the full terms.

> *"Free software, free society."* &mdash; Richard M. Stallman

---

## &sect; In Memoriam

<p align="center">
  <img src="docs/terry.jpg" alt="Terry A. Davis" width="380">
</p>

HolyC is the language created by **Terry A. Davis** (1969&ndash;2018) as
the systems programming language for TempleOS &mdash; an operating system
he built, entirely alone, from scratch, as an act of devotion.

`holycc` exists to carry that language forward. Not the operating system,
not the ring-0 architecture, not the biblical references &mdash; but the
*language itself*. A simple, clean, expressive systems language that
deserves to live beyond its original home.

This project is a humble tribute. We reimplement with respect, ship with
care, and release as free software &mdash; so that HolyC remains in the
commons, forever.

> *"An operating system is a work of art."*  
> &mdash; Terry A. Davis

---

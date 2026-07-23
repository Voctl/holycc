# holycc &mdash; HolyC Compiler for Linux

<img align="right" src="docs/logo.jpg" alt="HolyC Logo" width="220">

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![C17](https://img.shields.io/badge/language-C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![Build](https://img.shields.io/badge/build-cmake%20%7C%20gcc%20%7C%20clang-green.svg)]()
[![Tests](https://img.shields.io/badge/tests-161%2F161-brightgreen.svg)]()

**Just another HolyC &rarr; C17 transpiler.**  
Or actually, the only one that works on Linux without requiring
a ring-0 operating system written entirely by one guy.

Reads `.HC` source, produces readable C17, compiles with GCC/Clang,
and runs. No TempleOS VM, no emulator, no "but actually you need
a 640x480 VGA output". Just code.

<br clear="right"/>

---

## &sect; Why? Because I Can

> *"I'd like to see RMS writing a HolyC compiler."*  
> &mdash; Probably nobody, ever.

But here we are. Terry A. Davis built TempleOS and HolyC as an
operating system. We built `holycc` so you can run HolyC programs
on actual modern Linux machines without sacrificing your firstborn
to the VGA gods.

No TempleOS internals. No ring 0. No biblical references.  
**Just a compiler that works.**

---

## &sect; Quick Start

```bash
git clone https://github.com/YOU/holycc.git && cd holycc

cmake -B build && cmake --build build

./install.sh          # → ~/.local/bin, sets up 'hc' alias

# Write HolyC like a real TempleOS chad:
echo 'Print("Hello, world!\n");
return 0;' > hello.HC

holycc hello.HC       # HolyC → C17 → binary
./hello               # it just works
```

---



## &sect; Feature Matrix

### Type System

| HolyC | C17 |
|---|---|
| `I8`/`I16`/`I32`/`I64` | `int8_t`/`int16_t`/`int32_t`/`int64_t` |
| `U8`/`U16`/`U32`/`U64` | `uint8_t`/`uint16_t`/`uint32_t`/`uint64_t` |
| `F64` | `double` |
| `Bool` | `bool` |
| `Char` | `char` |
| `U0` | `void` |
| `class`/`union`/`enum` | `typedef struct`/`union`/`enum` |

### Statements

| | Statement |
|:---:|---|
| [ok] | `if`/`else`, `for`, `while`, `do`-`while` |
| [ok] | `switch`/`case`/`default` + implicit + range |
| [ok] | `return`, `break`, `goto`, labels |
| [ok] | `class`, `union`, `enum` |
| [ok] | `asm`/`_asm`, `#define`, `#include` |
| [ok] | `#if`/`#else`/`#endif`, `#ifdef`, `#ifndef` |
| [ok] | `try`/`catch`/`throw` |

### Expressions

| | Category |
|:---:|---|
| [ok] | Arithmetic, bitwise, logical, comparison |
| [ok] | Assignment, ternary, unary, cast |
| [ok] | `.` `->` `[]` `()` `&` `*` `sizeof` |
| [ok] | Power operator `` ` `` |

### HolyC-Specific

| | Feature |
|:---:|---|
| [ok] | String auto-print: `"hello\n";` |
| [ok] | Bare function call: `MyFunc;` |
| [ok] | Chained comparisons: `13 <= age < 20` |
| [ok] | Switch implicit case: `case:` |
| [ok] | Switch range: `case 4...10:` |
| [ok] | `U0` procedure type |
| [ok] | `public`/`private` |
| [ok] | `no_warn`, `reg`/`noreg` |
| [ok] | `offset(Vec2.y)` |
| [ok] | `offsetof` |
| [ok] | Power `` ` `` |
| [ok] | `has` keyword |
| [ok] | argc/argv variadic |
| [ok] | Color literals `$FF$` |
| [ok] | Class methods |
| [ok] | Top-level code (no `main()` needed) |

### Runtime

| | Function |
|:---:|---|
| [ok] | `Print`, `PrintLn` |
| [ok] | `MAlloc`, `Free`, `MSize` |
| [ok] | `StrLen`, `StrCompare` |
| [ok] | `AtoI`, `AtoF` |
| [ok] | `MemSet`, `MemCpy`, `MemCompare` |
| [ok] | `CDelay`, `GetCh`, `PutChar` |
| [ok] | `Exit`, `SPrint` |
| [ok] | `pow()` via `<math.h>` |

---

## &sect; Compiler Pipeline

```
  ┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
  │  Source  │ ──▶ │  Lexer   │ ──▶ │  Parser  │ ──▶ │ Semantic │ ──▶ │ CodeGen  │ ──▶ C17 (.c)
  │  (.HC)   │     │ 114 tok. │     │   AST    │     │  checks  │     │  emitter │
  └──────────┘     └──────────┘     └──────────┘     └──────────┘     └──────────┘
```

| # | Stage | Location |
|:--:|---|---|
| 1 | **Lexer** | `src/lexer/` |
| 2 | **Parser** | `src/parser/` |
| 3 | **AST** | `src/ast/` |
| 4 | **Semantic** | `src/semantic/` |
| 5 | **Symbol Table** | `src/symbol/` |
| 6 | **Type System** | `src/types/` |
| 7 | **Code Generator** | `src/codegen/` |
| 8 | **Runtime** | `runtime/` |
| 9 | **CLI Driver** | `src/driver/` |

---

## &sect; Coding Covenant

Every line committed to `main` must meet these standards:

| Rule | The standard |
|---|---|
| **Language** | C17 &mdash; no GNU extensions |
| **Compiler** | GCC &ge; 12 or Clang &ge; 15 |
| **Build** | CMake &ge; 3.16 |
| **Warnings** | `-Wall -Wextra -Wpedantic` &mdash; zero |
| **Functions** | Small, single-purpose |
| **State** | No globals. No mutable static state. |
| **Macros** | Almost never |
| **Nesting** | Three levels deep is a warning sign |

---

## &sect; CLI Reference

```
Usage: holycc [options] <input.HC>

Options:
  -o <file>        Output file
  -c, --emit-c     Emit C only (keeps .c in /tmp/)
  --compile        Compile to executable (default)
  --run            Compile and run immediately
  --keep-c         Keep generated .c file in /tmp/
  --tokens         Dump token stream
  --ast            Dump AST tree
  --help           Show this message
  --version        Print version
```

---

## &sect; Testing

```bash
cmake --build build
./build/tests/test_lexer
```

**Lexer suite:** 161/161 tests passing.  
Parser, semantic, and codegen test suites coming soon.  
Yes, we know. We'll get there. RMS doesn't believe in deadlines anyway.

---

---

## &sect; Contributing

We welcome contributors. Seriously. Even if you use Emacs.

Before diving in, take 10 minutes with [CONTRIBUTING.md](CONTRIBUTING.md).
The short version:

1. Fork &amp; branch
2. Write code &mdash; zero warnings
3. Add tests for your change
4. Open a PR

First time? Look for issues tagged `good first issue`.  
We'll help you land it. Unless you use proprietary software, in which
case RMS will personally appear at your door to have a chat.

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
See [this](https://stallman.org/) for whatever RMS is doing today.

> *"Free software, free society."*  
> &mdash; Richard M. Stallman, probably while using a ThinkPad with
>   a freedom-compatible WiFi card that he had to source from
>   a guy in Finland

---

## &sect; In Memoriam

<p align="center">
  <img src="docs/terry.jpg" alt="Terry A. Davis" width="380">
</p>

HolyC is the language created by **Terry A. Davis** (1969&ndash;2018) as
the systems programming language for TempleOS &mdash; an operating system
he built, entirely alone, from scratch, as an act of devotion.

`holycc` exists to carry that language forward. Not the operating system,
not the ring-0 architecture &mdash; but the *language itself*. A simple,
clean, expressive systems language that deserves to live beyond its
original home.

This project is a tribute. We reimplement with respect, ship with
care, and release as free software &mdash; so that HolyC remains in
the commons, forever.

> *"An operating system is a work of art."*  
> &mdash; Terry A. Davis  
> *"Also it runs in ring 0 and has 640x480 VGA."*  
> &mdash; Also Terry A. Davis, probably

---

> *"I'm not a compiler developer, I just wrote a HolyC transpiler
>   because someone had to and it wasn't going to be me, but here we are."*  
> &mdash; The authors

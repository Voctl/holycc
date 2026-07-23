# HolyC Language Reference

**HolyC** is the systems programming language created by **Terry A. Davis**
for TempleOS. `holycc` is a clean-room transpiler that converts HolyC
source (`.HC`) into C17, then compiles it with GCC or Clang.

This document covers the HolyC language as implemented by `holycc`.

---

## Table of Contents

1. [Quick Start](#1-quick-start)
2. [Hello, World](#2-hello-world)
3. [Type System](#3-type-system)
4. [Variables](#4-variables)
5. [Functions](#5-functions)
6. [Control Flow](#6-control-flow)
7. [Classes, Structs, Unions, Enums](#7-classes-structs-unions-enums)
8. [Operators](#8-operators)
9. [Strings](#9-strings)
10. [Preprocessor](#10-preprocessor)
11. [Runtime Library](#11-runtime-library)
12. [HolyC-Specific Features](#12-holyc-specific-features)
13. [Compiler Options](#13-compiler-options)
14. [Error Handling](#14-error-handling)
15. [Advanced Examples](#15-advanced-examples)

---

## 1. Quick Start

```bash
# Install holycc
git clone https://github.com/Voctl/holycc.git && cd holycc
cmake -B build && cmake --build build
./install.sh

# Write and run HolyC
echo 'Print("Hello, world!\n"); return 0;' > hello.HC
holycc hello.HC && ./hello
```

### Command line

```bash
holycc program.HC              # compile to binary
holycc program.HC --run        # compile and run
holycc program.HC --emit-c     # only generate .c file
holycc program.HC --keep-c     # keep .c file after compile
holycc program.HC --tokens     # dump token stream
holycc program.HC --ast        # dump AST tree
```

The generated `.c` file is placed in `/tmp/` and deleted after compilation.
Use `--keep-c` to preserve it.

---

## 2. Hello, World

### Minimal HolyC program

```c
Print("Hello, world!\n");
return 0;
```

No `main()` function required. No includes. No boilerplate.
Top-level code is the entry point.

### Explicit entry point

```c
I64 main()
{
    Print("Hello, world!\n");
    return 0;
}
```

### Using string auto-print

```c
"Hello, world!\n";
return 0;
```

Bare string literals as statements auto-print. Equivalent to
`Print("Hello, world!\n")`.

---

## 3. Type System

HolyC has fixed-width integer types, one floating-point type, and no
implicit type promotion games. All values are 64-bit when accessed
from memory.

### Primitive Types

| Type | Size | C17 Equivalent | Description |
|------|------|---------------|-------------|
| `I8` | 1 byte | `int8_t` | Signed 8-bit |
| `I16` | 2 bytes | `int16_t` | Signed 16-bit |
| `I32` | 4 bytes | `int32_t` | Signed 32-bit |
| `I64` | 8 bytes | `int64_t` | Signed 64-bit (default) |
| `U8` | 1 byte | `uint8_t` | Unsigned 8-bit |
| `U16` | 2 bytes | `uint16_t` | Unsigned 16-bit |
| `U32` | 4 bytes | `uint32_t` | Unsigned 32-bit |
| `U64` | 8 bytes | `uint64_t` | Unsigned 64-bit |
| `F64` | 8 bytes | `double` | IEEE 754 double |
| `Bool` | 1 byte | `bool` | Boolean (`TRUE` / `FALSE`) |
| `Char` | 1 byte | `char` | Single character |
| `void` | 0 | `void` | No type |
| `U0` | 0 | `void` | Procedure type (zero-sized void) |

### Derived Types

| Syntax | Description |
|--------|-------------|
| `Type*` | Pointer to Type |
| `Type[size]` | Array of Type |
| `Type (*)(args)` | Function pointer |
| `class Name { ... }` | Named struct |
| `union Name { ... }` | Union |
| `enum Name { ... }` | Enumeration |

### Type Literals

```c
I64 x = 42;           // decimal
I64 h = 0xFF;         // hexadecimal
I64 b = 0b1010;       // binary
F64 pi = 3.14159;     // float
F64 e = 2.718e0;      // scientific
Bool flag = TRUE;     // boolean
Char c = 'A';         // character
```

---

## 4. Variables

### Declaration

```c
I64 x;                  // uninitialized
I64 y = 42;            // initialized
I64 a, b, c;           // comma-separated (basic support)
```

### Storage Class Modifiers

```c
static I64 hidden;      // file-local (private)
extern I64 external;    // external linkage
public I64 api;         // public API (absorbed)
private I64 internal;   // private = static
const I64 fixed = 5;   // read-only
reg I64 R15 fast;      // register hint (absorbed)
noreg I64 slow;        // no register (absorbed)
no_warn I64 temp;      // suppress warnings (absorbed)
```

### Scope

HolyC uses lexical scoping:

```c
I64 global = 1;                    // global scope

I64 func() {
    I64 local = 2;                 // function scope
    {
        I64 block = 3;             // block scope
    }
}
```

---

## 5. Functions

### Function Definition

```c
I64 Add(I64 a, I64 b)
{
    return a + b;
}
```

Return type, name, parameters, and body. No forward declarations needed
— functions are visible from anywhere in the file.

### Procedure Type (U0)

```c
U0 Greet()
{
    "Hello!\n";
}
```

`U0` is a procedure that returns nothing. Maps to `void` in C.

### Variadic Functions (argc/argv)

HolyC uses `argc` and `argv` instead of C's `va_list`:

```c
I64 Sum(I64 count, ...)
{
    I64 i, res = 0;
    for (i = 0; i < count; i++)
        res += argv[i];
    return res;
}

Print("%lld\n", Sum(3, 10, 20, 30));   // 60
```

- `argc` — built-in variable: number of variadic arguments
- `argv` — built-in array: the variadic arguments as `I64`

### Bare Function Call

Functions with no arguments can be called without parentheses:

```c
I64 SayHello() { "Hello!\n"; }

SayHello;       // same as: SayHello();
```

### Main Entry Point

HolyC does not require a `main()` function. Code at the top level
is the entry point:

```c
"starting...\n";
I64 x = 42;
return 0;
```

If an explicit `I64 main()` or `int main()` function is defined,
it becomes the entry point.

---

## 6. Control Flow

### if / else

```c
if (x > 0) {
    "positive\n";
} else if (x == 0) {
    "zero\n";
} else {
    "negative\n";
}
```

### Chained Comparisons

HolyC supports chained comparison operators, unlike C:

```c
if (13 <= age < 20)
    "teenager\n";

// equivalent to: (13 <= age) && (age < 20)
```

### for

```c
for (i = 0; i < 10; i++) { ... }
for (I64 i = 0; i < 10; i++) { ... }
```

### while / do-while

```c
while (condition) { ... }
do { ... } while (condition);
```

### switch

HolyC's switch statement has several extensions over C:

```c
switch (x) {
    case 0:
        "zero\n"; break;
    case 1:
        "one\n"; break;
    default:
        "other\n"; break;
}
```

#### Implicit Cases

```c
switch (x) {
    case:
        "zero\n"; break;     // matches 0
    case:
        "one\n"; break;      // matches 1
    case:
        "two\n"; break;      // matches 2
}
```

Cases without explicit values auto-increment from 0.

#### Range Cases

```c
switch (x) {
    case 4...10:
        "between 4 and 10\n"; break;
    default:
        "outside range\n"; break;
}
```

The range `case 4...10:` expands to `case 4: case 5: ... case 10:`
in the generated C.

### return / break / continue / goto

```c
return;               // void return
return value;         // value return
break;                // exit loop or switch
continue;             // next iteration (warning: not HolyC-standard)
goto label;           // unconditional jump
label:                // label target
```

Note: `continue` is not part of standard HolyC — use `goto` instead.
The compiler emits a warning when `continue` is used.

---

## 7. Classes, Structs, Unions, Enums

### class

HolyC uses `class` instead of C's `typedef struct`:

```c
class Vec2 {
    F64 x;
    F64 y;
};
```

This generates:
```c
typedef struct {
    double x;
    double y;
} Vec2;
```

### Class Methods

Methods can be defined inside a class:

```c
class Vec2 {
    F64 x;
    F64 y;
    F64 Length() {
        return sqrt(x*x + y*y);
    }
};
```

Methods are emitted as `ClassName_MethodName(ClassName *this)`.
Inside a method, fields are accessed by bare name or with `this->`.

### union

```c
union Value {
    I64 i;
    F64 f;
};
```

### enum

```c
enum Color {
    RED,
    GREEN,
    BLUE
};

enum HttpStatus {
    OK = 200,
    NOT_FOUND = 404,
    ERROR = 500
};
```

---

## 8. Operators

### Arithmetic

| Operator | Description |
|----------|-------------|
| `+` `-` `*` `/` `%` | Add, subtract, multiply, divide, modulo |
| `` ` `` | Power (a ` b = pow(a, b)) |
| `++` `--` | Increment, decrement (prefix/postfix) |

### Bitwise

| Operator | Description |
|----------|-------------|
| `&` `|` `^` `~` | AND, OR, XOR, NOT |
| `<<` `>>` | Left shift, right shift |

### Logical

| Operator | Description |
|----------|-------------|
| `&&` `||` `!` | AND, OR, NOT |

### Comparison

| Operator | Description |
|----------|-------------|
| `==` `!=` | Equal, not equal |
| `<` `>` `<=` `>=` | Less, greater, less-equal, greater-equal |

Chained: `13 <= age < 20` is valid HolyC.

### Assignment

| Operator | Description |
|----------|-------------|
| `=` | Simple assignment |
| `+=` `-=` `*=` `/=` `%=` | Compound arithmetic |
| `&=` `|=` `^=` `<<=` `>>=` | Compound bitwise |

### Member Access

| Operator | Description |
|----------|-------------|
| `.` | Direct member access |
| `->` | Pointer member access |
| `[]` | Array subscript |
| `()` | Function call |

### Other

| Operator | Description |
|----------|-------------|
| `&` | Address-of |
| `*` | Dereference |
| `(Type)expr` | Cast |
| `sizeof` | Size of type/expr |
| `offset(Type.field)` | Field offset (maps to `offsetof`) |
| `cond ? a : b` | Ternary conditional |

---

## 9. Strings

### String Literals

```c
"hello";
"hello\n";          // with newline
"path\\to\\file";   // escaped backslash
"say \"hi\"";       // escaped quote
```

### String Auto-Print

A bare string literal as a statement automatically prints:

```c
"Hello, world!\n";

// equivalent to: Print("Hello, world!\n");
```

### Color Literals

TempleOS color codes in strings are stripped:

```c
"$FF$red text$FG$\n";
//               → "red text\n"  ($FF$ = red, $FG$ = reset)
```

`$XX$` sequences (6 hex digits or 2 hex digits for VGA colors)
are removed from the output string. `$$` becomes a literal `$`.

### Character Literals

```c
'A';                // value 65
'\n';               // value 10 (newline)
'\t';               // value 9  (tab)
'\'';               // value 39 (single quote)
'\\';               // value 92 (backslash)
'\0';               // value 0  (null)
```

---

## 10. Preprocessor

### #define

```c
#define WIDTH  800
#define HEIGHT 600
#define DEBUG  1
```

Simple macro definitions. Function-like macros are **not** supported
(in line with HolyC design).

### #include

```c
#include "math.HC"
#include "myheader.HC"
```

Only `""` form is supported (no angle brackets). Files are included
textually. The include path is relative to the source file's directory.

### #if / #else / #endif

```c
#if DEBUG
    "debug mode\n";
#else
    "release mode\n";
#endif
```

Standard C preprocessor conditionals. Pass through to the generated C.

### #ifdef / #ifndef / #elif

```c
#ifdef FEATURE_X
    "feature x\n";
#endif

#ifndef FEATURE_Y
    "feature y disabled\n";
#endif
```

---

## 11. Runtime Library

All runtime functions are built-in. No imports or declarations needed.

### I/O

| Function | Description |
|----------|-------------|
| `Print(fmt, ...)` | Formatted output (like printf) |
| `PrintLn(fmt, ...)` | Print + newline |
| `PutChar(c)` | Print single character |
| `GetCh()` | Read single character from stdin |
| `SPrint(buf, fmt, ...)` | Print to string buffer |

### Memory

| Function | Description |
|----------|-------------|
| `MAlloc(size)` | Allocate memory |
| `Free(ptr)` | Deallocate memory (NULL-safe) |
| `MSize(ptr)` | Get allocated size |
| `MemSet(dst, val, count)` | Fill memory |
| `MemCpy(dst, src, count)` | Copy memory |
| `MemCompare(a, b, count)` | Compare memory |

### String

| Function | Description |
|----------|-------------|
| `StrLen(str)` | String length |
| `StrCompare(a, b)` | String equality |
| `AtoI(str)` | ASCII to integer |
| `AtoF(str)` | ASCII to float |

### Control

| Function | Description |
|----------|-------------|
| `CDelay(ms)` | Delay in milliseconds |
| `Exit(code)` | Exit program with code |

### Math

| Function | Description |
|----------|-------------|
| `a ` b` | Power (via pow()) |
| `sqrt(x)` | Square root (from <math.h>) |

---

## 12. HolyC-Specific Features

### Top-Level Code

HolyC code runs at the top level without a `main()` function:

```c
// This is a complete program:
"salam\n";
I64 x = 42;
return 0;
```

The transpiler wraps top-level statements in `int main() { }`.

### String Auto-Print

```c
"Hello\n";           // prints "Hello" + newline
"value: %d\n", x;    // future: formatted auto-print
```

### Bare Function Call

```c
MyFunc;              // calls MyFunc() with no args
```

### Chained Comparisons

```c
if (0 <= x < 10)     // valid: (0 <= x) && (x < 10)
```

### Switch Extensions

```c
case:                // implicit: 0, 1, 2...
case 4...10:         // range: 4 through 10
start:               // sub-switch begin (label)
end:                 // sub-switch end (label)
```

### U0 Type

```c
U0 Log() { "logged\n"; }
```

Zero-sized procedure type. Maps to `void` in C.

### argc/argv Variadic

```c
I64 Sum(I64 n, ...) {
    I64 i, res = 0;
    for (i = 0; i < argc; i++)
        res += argv[i];
    return res;
}
```

### Class Methods

```c
class Vec2 {
    F64 x;
    F64 y;
    F64 Length() { return sqrt(x*x + y*y); }
};

// call: Vec2_Length(&instance)
```

### try/catch/throw

```c
try {
    if (error) throw;
} catch {
    "error!\n";
}
```

Exceptions use `setjmp`/`longjmp` under the hood.

### reg/noreg

```c
reg I64 R15 fast;     // register hint (absorbed)
noreg I64 slow;       // no register (absorbed)
```

### public/private

```c
public I64 api;        // public API (just documentation)
private I64 internal;  // maps to static
```

### no_warn

```c
no_warn I64 temp;      // suppress warnings (absorbed)
```

### offset

```c
offset(Vec2.y);        // byte offset of field
                       // maps to offsetof(Vec2, y)
```

### Color Literals in Strings

```c
"$FF$red text$FG$ normal\n";
// $RRGGBB$ for true color
// $FG$ resets foreground
// $BG$ resets background
// $$ is a literal dollar sign
```

### has Keyword

```c
has Vec2.y            // compile-time member test (token only)
```

---

## 13. Compiler Options

```
Usage: holycc [options] <input.HC>

Options:
  -o <file>        Output binary to <file>
  -c, --emit-c     Emit C code only (keep .c file)
  --compile        Compile to executable (default)
  --run            Compile and run immediately
  --keep-c         Keep generated .c file in /tmp/
  --tokens         Dump token stream to stderr
  --ast            Dump AST tree to stderr
  --help           Show this help message
  --version        Print version string
```

### Output Behavior

- The generated `.c` file is created in `/tmp/`
- After successful compilation, the `.c` file is deleted
- Use `--keep-c` to preserve it
- The binary executable is placed in the current directory
  (or where `-o` specifies)

---

## 14. Error Handling

HolyC uses a diagnostic system for errors and warnings:

### Error Levels

| Level | Description | Exit |
|-------|-------------|------|
| Error | Fatal compilation error | non-zero |
| Warning | Non-fatal diagnostic | zero |

### Common Errors

```
undefined identifier 'foo'           — variable or function not declared
duplicate definition of 'bar'        — already defined in this scope
cannot dereference non-pointer type  — * used on non-pointer
'break' outside loop or switch       — break in wrong context
'continue' outside loop              — continue in wrong context
expected ';'                         — missing semicolon
```

### try/catch/throw

Runtime errors can be caught:

```c
try {
    if (divisor == 0)
        throw;
    result = a / divisor;
} catch {
    "division by zero!\n";
}
```

---

## 15. Advanced Examples

### Complete HolyC Program

```c
// Features: top-level code, auto-print, chain comp, switch, class

U0 ShowResult(I64 x)
{
    "result: ";
    Print("%lld\n", x);
}

I64 Classify(I64 v)
{
    switch (v) {
        case:
            "zero\n";  return 0;
        case:
            "one\n";   return 1;
        case 4...10:
            "range\n"; return 2;
        default:
            "other\n"; return -1;
    }
}

class Vec2 { F64 x; F64 y; };

I64 age = 17;

"Age check: ";
if (13 <= age < 20)
    "teenager\n";
else
    "not teenager\n";

"Classify test:\n";
Classify(0);
Classify(1);
Classify(7);
Classify(42);

"Vector test:\n";
Vec2 v;
v.x = 3.0;
v.y = 4.0;
Print("length = %f\n", sqrt(v.x*v.x + v.y*v.y));

"Variadic test:\n";
I64 Sum(I64 n, ...)
{
    I64 i, res = 0;
    for (i = 0; i < n; i++)
        res += argv[i];
    return res;
}
Print("sum = %lld\n", Sum(4, 5, 10, 15, 20));

return 0;
```

### Using Runtime Functions

```c
U64 len = StrLen("hello");

void *buf = MAlloc(1024);
MemSet(buf, 0, 1024);
Free(buf);

I64 val = AtoI("1234");
F64 pi = AtoF("3.14159");

CDelay(1000);             // wait 1 second

PutChar('A');             // print character
I64 ch = GetCh();         // read character
```

### Preprocessor Usage

```c
#define VERBOSE 1
#define MAX_BUF 4096

#if VERBOSE
    "verbose mode\n";
#endif

#include "mylib.HC"
```

---

## Differences from C

| Feature | C | HolyC |
|---------|---|-------|
| Entry point | `int main()` | Top-level code |
| Type aliasing | `typedef struct` | `class` keyword |
| Function call | `f()` always | `f;` for 0-arg |
| String print | `printf("...")` | `"...";` auto-prints |
| Variadic args | `va_list` macros | `argc`/`argv` built-in |
| Chained compare | Not valid | `13 <= x < 20` |
| Switch cases | Fixed values | Implicit + range |
| Void procedures | `void` | `U0` (zero-sized) |
| Field offset | `offsetof()` | `offset()` keyword |
| Power operator | `pow()` library | `` ` `` backtick |
| Includes | `<>` and `""` | Only `""` |
| `continue` | Yes | Use `goto` instead |
| Function-like macros | Yes | Not supported |

---

## License

HolyC is the language of TempleOS, created by Terry A. Davis.
`holycc` is a clean-room reimplementation released under the
GNU General Public License v3.0.

See [LICENSE](../LICENSE) for details.

---

*"An operating system is a work of art." — Terry A. Davis*

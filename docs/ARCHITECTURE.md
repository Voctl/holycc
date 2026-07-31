<!-- Architecture: like legos, but if you step on it, it segfaults. -->
# holycc Architecture

## Overview

holycc is a multi-pass compiler that transforms HolyC source code into C17:

```
Source (.HC)
    │
    ▼
┌─────────┐
│  Lexer  │  Tokenization: characters → tokens
└────┬────┘
     │ Token stream
     ▼
┌─────────┐
│ Parser  │  Syntax analysis: tokens → AST
└────┬────┘
     │ AST tree
     ▼
┌─────────┐
│ Semantic│  Semantic analysis, type checking, symbol resolution
└────┬────┘
     │ Decorated AST
     ▼
┌─────────┐
│ CodeGen │  Code generation: AST → C17 text
└────┬────┘
     │
     ▼
  C17 (.c) ──► GCC/Clang ──► Binary
```

## Module Descriptions

### Lexer (`src/lexer/`)
- **Input**: Source text (char*)
- **Output**: Token stream
- **Responsibility**: Character-level processing, keyword recognition, literal parsing
- **Error recovery**: Produces TOK_ERROR tokens for invalid input

The lexer operates character-by-character through the source buffer. It maintains a one-token lookahead buffer for the parser's needs. Keywords are matched via a binary-searchable sorted table of 53 HolyC keywords. Numeric literals support decimal, hexadecimal (`0x`), and binary (`0b`) formats. The lexer also handles `//` and `/* */` comments, stripping them during tokenisation.

**Key functions:**
- `lexer_next_token()` — consume and return the next token
- `lexer_peek_token()` — return next token without consuming
- `lexer_read_number()` — dispatch between integer/hex/binary/float
- `lexer_read_identifier()` — match identifiers and keywords
- `lexer_read_preprocessor()` — handle `#if`, `#else`, `#endif`, `#ifdef`, `#ifndef`, `#elif`

### Parser (`src/parser/`)
- **Input**: Token stream (from Lexer)
- **Output**: Abstract Syntax Tree (AST)
- **Algorithm**: Recursive descent with precedence climbing for expressions
- **No code generation**: Parser never emits C code

The parser is organised into layers that correspond to expression precedence levels:

1. `parser_parse_primary()` — literals, identifiers, parenthesised expressions, array initialisers
2. `parser_parse_postfix()` — indexing `[]`, calls `()`, member access `.`/`->`, postfix `++`/`--`
3. `parser_parse_prefix()` — unary `-`, `!`, `~`, `*`, `&`, `++`, `--`, `sizeof`, `offset`, casts
4. `parser_parse_binary()` — precedence climbing (powers `14` down to assignment `2`)
5. `parser_parse_expr()` — top-level entry point

Statements are parsed by `parser_parse_stmt()`, which dispatches on the current token kind to the appropriate parsing function. The parser supports HolyC-specific features like chained comparisons (`a < b <= c`), implicit switch cases (`case:`), and range cases (`case 4...10:`).

**Forward declarations:** The parser handles `#include` and `import` directives by recursively invoking a new lexer+parser on the referenced file, splicing the resulting AST nodes into the current translation unit.

### AST (`src/ast/`)
- **Node types**: 45+ node kinds covering all language constructs
- **Tree structure**: Each node has `first_child` / `last_child` / `next` pointers
- **Visitors**: Recursive pre/post-order traversal support
- **Attachments**: Type information, source locations, flags

The AST uses a sibling-linked tree structure. Each `AstNode` has:
- `first_child` / `last_child` — doubly-anchored child list
- `next` — pointer to the next sibling
- `parent` — pointer to parent node
- `data` — polymorphic union for literal values, operator kinds, string data

**Key operations:**
- `ast_node_create()` — allocate a new node (zero-initialised)
- `ast_add_child()` — append a child to the parent's linked list
- `ast_visit()` — pre-order + post-order traversal with callbacks
- `ast_clone_node()` — deep-copy a subtree (used for chained comparison desugaring)
- `ast_node_destroy_tree()` — recursive subtree cleanup

### Semantic Analysis (`src/semantic/`)
- **Symbol resolution**: Looks up identifiers in symbol table
- **Type checking**: Validates type compatibility for operations
- **Scope management**: Pushes/pops scopes as it enters/exits blocks
- **Error detection**: Undeclared variables, duplicate definitions, break/continue outside loops

The semantic analyser walks the AST recursively. For each node it:

1. Resolves type annotations to internal `Type*` via `semantic_resolve_type()`
2. Analyses expressions bottom-up via `semantic_analyze_expr()`, returning the inferred `Type*`
3. Processes statements top-down via `semantic_analyze_stmt()`, managing scope and registering symbols

**Type checking rules:**
- Dereference (`*`) requires pointer operand
- Address-of (`&`) wraps operand in pointer type
- Comparison operators produce `bool`
- Arithmetic promotes to `F64` if either operand is floating
- Assignment operators return the left-hand type
- Function calls look up the symbol and use its return type

**HolyC-specific handling:**
- `NULL`, `TRUE`, `FALSE` are recognised even without explicit declarations
- `continue` outside a loop emits a warning (not HolyC-standard)
- Chained comparisons are validated each segment individually
- Class methods are registered as function symbols with `ClassName_MethodName` naming

### Symbol Table (`src/symbol/`)
- **Scope hierarchy**: Global → Function → Block → Struct
- **Lookup**: Walks scope chain from current to global
- **Storage**: Symbols store name, type, kind, storage class, definition status

Scopes are linked lists of `Scope` structs, each containing a singly-linked list of `Symbol` entries. The symbol table maintains `current_scope` and `global_scope` pointers.

**Scope push/pop lifecycle:**
```
Translation Unit → scope_push(GLOBAL)
  Function → scope_push(FUNCTION)
    Block → scope_push(BLOCK)
      ... nested blocks ...
    scope_pop()
  scope_pop()
scope_pop()
```

**Duplicate detection:** `symbol_add()` checks the current scope for existing symbols with the same name before insertion, emitting an error and a note pointing to the previous declaration.

### Type System (`src/types/`)
- **Primitive types**: I8, I16, I32, I64, U8, U16, U32, U64, F64, Bool, Char, void
- **Compound types**: Pointer, Array, Function, Struct, Union, Enum
- **Type queries**: Size, alignment, C name mapping, equality, classification
- **Memory**: Each type is heap-allocated, owned by symbol table

The type system models HolyC types internally and provides bidirectional mapping to C17 names:

| HolyC | C17 | Size |
|-------|-----|------|
| I8 | int8_t | 1 |
| I16 | int16_t | 2 |
| I32 | int32_t | 4 |
| I64 | int64_t | 8 |
| U8 | uint8_t | 1 |
| U16 | uint16_t | 2 |
| U32 | uint32_t | 4 |
| U64 | uint64_t | 8 |
| F64 | double | 8 |
| Bool | bool | 1 |
| Char | char | 1 |
| void/U0 | void | 0 |

**Compound types** use recursive structure: pointers hold a `base` type, arrays hold `base` + `length`, functions hold `return_type` + `params` + `variadic`, and aggregates hold a linked list of `StructField`.

**Type predicates** classify types for semantic rules:
- `type_is_integer()` — Char, I8–I64, U8–U64
- `type_is_floating()` — F64 only
- `type_is_numeric()` — integer or floating
- `type_is_scalar()` — numeric, pointer, or bool
- `type_is_aggregate()` — struct, union, or array

### Code Generator (`src/codegen/`)
- **Input**: Semantically analyzed AST + Symbol Table
- **Output**: C17 source text
- **Strategy**: Recursive tree walk, emitting C syntax
- **Type mapping**: HolyC types → stdint.h types (`I64` → `int64_t`)
- **Special cases**: `main` function → `int main`, Bool → `bool`, etc.

The code generator (`codegen_emit_stmt()`) handles every AST node kind:

**Translation unit:**
- Emits `#include` directives for stdint, stdbool, stddef, stdio, stdlib, string, math, stdarg, setjmp
- Emits runtime function prototypes
- Scans for function names to enable bare-call syntax
- If top-level code exists without an explicit `main()`, wraps everything in `int main() { ... }`

**Function codegen:**
- Maps return type via `codegen_map_type_name()`
- `main()` gets return type `int` instead of `I64`
- Variadic functions get auto-generated `argc`/`argv` setup via `va_list`
- Class methods are prefixed with `ClassName_` and receive a `this` pointer

**HolyC-specific features:**
- String literals as expression statements → wrapped in `Print()`
- Bare function calls → appended with `()`
- Chained comparisons → desugared into `&&`-chains
- Switch range cases → expanded into individual `case` labels
- Try/catch/throw → implemented via `setjmp`/`longjmp`
- Inline assembly → emitted as comments (pass-through)
- Color literals → stripped from string output
- Power operator `` ` `` → emitted as `pow()`

### Runtime (`runtime/`)
- **Independence**: Does not depend on compiler internals
- **Functions**: `Print`, `PrintLn`, `MAlloc`, `Free`, `StrLen`, `StrCompare`, `AtoI`, `AtoF`, `MemSet`, `MemCpy`, `MemCompare`
- **Linking**: Statically linked with generated C programs

The runtime is a separate static library (`libholyc_runtime.a`) that provides HolyC's built-in functions. It avoids `<string.h>` for `MemSet`/`MemCpy`/`MemCompare` to maintain independence from the C standard library's implementation details. The runtime is linked into every compiled HolyC program via GCC invocation in the driver.

### CLI Driver (`src/driver/`)
- **Argument parsing**: Manual, no external dependency
- **Orchestration**: Lexer → Parser → Semantic → CodeGen pipeline
- **Debug flags**: `--tokens`, `--ast` for compiler debugging
- **Error handling**: Prints diagnostics with source snippets

The driver implements the full compilation pipeline:

1. **Argument parsing** (`driver_main()`): Iterates `argv`, sets `DriverOptions` flags
2. **File reading**: `read_file()` loads the source into memory
3. **Diagnostics**: `diagnostics_create()` initialises the error collector
4. **Lexer**: `lexer_create()` + optional `--tokens` dump
5. **Parser**: `parser_create()` + `parser_parse_translation_unit()` → AST
6. **Semantic analysis**: `semantic_create()` + `semantic_analyze()` + symbol table
7. **Code generation**: `codegen_create()` + `codegen_generate_file()` → `.c` file
8. **GCC invocation**: `compile_c_to_binary()` links runtime library
9. **Execution**: If `--run` flag, executes the produced binary
10. **Cleanup**: Destroys all compiler objects in reverse order, frees source buffer

**Temporary file handling:** Generated C files are placed in `/tmp/` with basename matching the input file, then deleted after compilation unless `--keep-c` is specified.

### Diagnostics (`src/diag.c`)
- **Error levels**: Error, Warning, Note, Internal Compiler Error
- **Output format**: `file:line:col: level: message` + source snippet
- **Collection**: All diagnostics collected during compilation, printed at end
- **Only one pass**: Once an error is emitted, compilation continues to find more errors

The diagnostics system stores up to `MAX_DIAGNOSTICS` (256) messages in a fixed-capacity array. Each diagnostic stores file, line, column, severity level, and a `strdup()`-ed message string. The `print()` function iterates stored diagnostics, emitting them to stderr with a source-line snippet and a `^` pointer under the relevant column.

**ICE handling:** Internal Compiler Errors immediately call `abort()` for debugging.

## Data Flow

```
┌─────────────────────────────────────────────────────┐
│                    Driver (main)                     │
│  ┌─────────┐  ┌────────┐  ┌──────────┐  ┌────────┐ │
│  │ Read file│→│ Lexer  │→│  Parser  │→ │Semantic│ │
│  └─────────┘  └────────┘  └──────────┘  └───┬────┘ │
│                                               │      │
│                    ┌──────────┐               │      │
│                    │  CodeGen │←──────────────┘      │
│                    └────┬─────┘                     │
│                         │                            │
│                    ┌────▼─────┐                     │
│                    │ Write .c │                     │
│                    └──────────┘                     │
└─────────────────────────────────────────────────────┘
```

## Design Decisions

### Why C17 as output?
- Maximum portability (any platform with a C compiler)
- Human-readable output aids debugging
- GCC/Clang optimizations benefit for free
- No need to implement register allocation, instruction selection

### Why recursive descent parser?
- Simple to implement and debug
- Easy to produce good error messages
- Sufficient for HolyC's grammar complexity
- No external parser generator dependency

### Why separate AST from Parser?
- Enables multiple passes over the same tree
- Allows adding optimization passes later
- Supports visitor pattern for tree transformations
- Makes testing easier (can construct AST manually for tests)

### Why pass-by-value for Diagnostics?
- Diagnostics struct is small (function pointers + one pointer)
- Eliminates need for dynamic allocation
- Multiple compilation units can have independent Diagnostics

### Why byte-by-byte memory operations in runtime?
The runtime's `MemSet`, `MemCpy`, and `MemCompare` use byte loops instead of library functions to avoid depending on `<string.h>` internals in generated code. This ensures the generated C17 is fully self-contained and portable across libc implementations.

### Why `setjmp`/`longjmp` for try/catch?
HolyC's `try`/`catch`/`throw` semantics map naturally to `setjmp`/`longjmp`. The try block initialises a global `jmp_buf`, the catch block runs when `longjmp` is called, and the throw expression value is passed as the `longjmp` value.

### Why wrap top-level code in `main()`?
HolyC allows top-level executable code (outside any function). This is not valid C, so the code generator scans for top-level statements and wraps them in an auto-generated `int main()` function. Function/type declarations are emitted before the synthetic main.

## Memory Management

Current strategy: explicit allocation with cleanup at each stage.

```
Lexer      → lexer_destroy()
Parser     → parser_destroy() (Lexer owned by caller)
AST        → ast_node_destroy_tree()
SymbolTable→ symbol_table_destroy()
Semantic   → semantic_destroy() (owns SymbolTable)
CodeGen    → codegen_destroy()
```

Future: Arena allocator for AST nodes and types.

## Error Handling

All errors go through the `Diagnostics` interface:

```c
diag.error(loc, "undefined identifier '%s'", name);
diag.warning(loc, "unused variable '%s'", name);
diag.note(loc, "declared here");
```

**Rule**: If `diag.had_error` is true after a pass, skip remaining passes and exit non-zero.

## Pipeline Error Flow

```
Read file → fail? → exit 1
Lexer → errors? → print → skip parse
Parser → errors? → print → skip semantic
Semantic → errors? → print → skip codegen
CodeGen → errors? → print → skip GCC
GCC → fail? → print compile error → exit 1
```

At each stage, if the previous stage set `had_error`, the pipeline short-circuits and the program exits with code 1.

## File-by-File Layout

```
include/holyc/        # Public API headers (opaque types, function declarations)
  token.h             # TokenKind, SourceLocation, Token
  lexer.h             # Lexer opaque type
  parser.h            # Parser opaque type
  ast.h               # AstKind, AstNode, visitor, lifecycle
  semantic.h          # Semantic opaque type
  symbol.h            # Scope, Symbol, SymbolTable
  types.h             # TypeKind, Type, StructField, FuncParam
  codegen.h           # CodeGen opaque type
  driver.h            # DriverOptions struct
  diag.h              # Diagnostics, Diagnostic
  utils.h             # StringBuffer, file I/O

src/                  # Implementation
  main.c              # Entry point
  token.c             # Token name/spelling lookup table
  diag.c              # Diagnostic storage and formatted output
  utils.c             # StringBuffer, read_file, write_file
  lexer/lexer.c       # Character-by-character tokeniser
  parser/parser.c     # Recursive descent parser
  ast/ast.c           # AST lifecycle and traversal
  semantic/semantic.c # Type checking and symbol resolution
  symbol/symbol.c     # Lexically scoped symbol table
  types/types.c       # Type factory functions and queries
  codegen/codegen.c   # C17 code emitter
  driver/driver.c     # CLI orchestration

runtime/              # HolyC runtime library
  holyc_runtime.h     # Runtime function declarations
  holyc_runtime.c     # Runtime function implementations

tests/                # Test suite
  test_lexer.c        # 161 lexer tests

docs/                 # Documentation
  ARCHITECTURE.md     # This file
  LANGUAGE.md         # HolyC language reference
```

## Testing Strategy

| Layer | Test Type | Tool |
|---|---|---|
| Lexer | Token stream verification | Unit tests |
| Parser | AST shape verification | Snapshot tests |
| Semantic | Pass/fail test cases | Integration tests |
| Codegen | Compile + diff + GCC check | End-to-end tests |
| CLI | Invocation + output | Shell script tests |

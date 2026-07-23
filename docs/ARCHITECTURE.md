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

### Parser (`src/parser/`)
- **Input**: Token stream (from Lexer)
- **Output**: Abstract Syntax Tree (AST)
- **Algorithm**: Recursive descent with precedence climbing for expressions
- **No code generation**: Parser never emits C code

### AST (`src/ast/`)
- **Node types**: 45+ node kinds covering all language constructs
- **Tree structure**: Each node has `first_child` / `last_child` / `next` pointers
- **Visitors**: Recursive pre/post-order traversal support
- **Attachments**: Type information, source locations, flags

### Semantic Analysis (`src/semantic/`)
- **Symbol resolution**: Looks up identifiers in symbol table
- **Type checking**: Validates type compatibility for operations
- **Scope management**: Pushes/pops scopes as it enters/exits blocks
- **Error detection**: Undeclared variables, duplicate definitions, break/continue outside loops

### Symbol Table (`src/symbol/`)
- **Scope hierarchy**: Global → Function → Block → Struct
- **Lookup**: Walks scope chain from current to global
- **Storage**: Symbols store name, type, kind, storage class, definition status

### Type System (`src/types/`)
- **Primitive types**: I8, I16, I32, I64, U8, U16, U32, U64, F64, Bool, Char, void
- **Compound types**: Pointer, Array, Function, Struct, Union, Enum
- **Type queries**: Size, alignment, C name mapping, equality, classification
- **Memory**: Each type is heap-allocated, owned by symbol table

### Code Generator (`src/codegen/`)
- **Input**: Semantically analyzed AST + Symbol Table
- **Output**: C17 source text
- **Strategy**: Recursive tree walk, emitting C syntax
- **Type mapping**: HolyC types → stdint.h types (`I64` → `int64_t`)
- **Special cases**: `main` function → `int main`, Bool → `bool`, etc.

### Runtime (`runtime/`)
- **Independence**: Does not depend on compiler internals
- **Functions**: `Print`, `PrintLn`, `MAlloc`, `Free`, `StrLen`, `StrCompare`, `AtoI`, `AtoF`, `MemSet`, `MemCpy`, `MemCompare`
- **Linking**: Statically linked with generated C programs

### CLI Driver (`src/driver/`)
- **Argument parsing**: Manual, no external dependency
- **Orchestration**: Lexer → Parser → Semantic → CodeGen pipeline
- **Debug flags**: `--tokens`, `--ast` for compiler debugging
- **Error handling**: Prints diagnostics with source snippets

### Diagnostics (`src/diag.c`)
- **Error levels**: Error, Warning, Note, Internal Compiler Error
- **Output format**: `file:line:col: level: message` + source snippet
- **Collection**: All diagnostics collected during compilation, printed at end
- **Only one pass**: Once an error is emitted, compilation continues to find more errors

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

## Testing Strategy

| Layer | Test Type | Tool |
|---|---|---|
| Lexer | Token stream verification | Unit tests |
| Parser | AST shape verification | Snapshot tests |
| Semantic | Pass/fail test cases | Integration tests |
| Codegen | Compile + diff + GCC check | End-to-end tests |
| CLI | Invocation + output | Shell script tests |

# Changelog

## [0.1.0] — Unreleased

### Added
- Lexer with 114 token kinds, full keyword/operator recognition
- Recursive descent parser supporting functions, structs, unions, enums, all statement types
- AST with 45+ node kinds and visitor pattern
- Symbol table with lexical scoping (global, function, block, struct)
- Type system: I8-I64, U8-U64, F64, Bool, Char, void, pointers, arrays, aggregates
- Semantic analysis: undeclared variable detection, duplicate definitions, break/continue checks
- C17 code generator with HolyC→C type mapping
- CLI driver with `--tokens`, `--ast`, `--emit-c`, `--help`, `--version`
- Runtime library: Print, MAlloc, Free, StrLen, StrCompare, AtoI, AtoF, MemSet, MemCpy, MemCompare
- Lexer test suite (161/161 tests passing)
- CMake build system with C17 standard, -Wall -Wextra -Wpedantic
- Source location tracking (file, line, column) in all tokens and AST nodes
- Human-friendly diagnostics with source snippets
- MIT License
- Comprehensive README, ROADMAP, architecture documentation

### Supported HolyC Features
- All basic types (I8, I16, I32, I64, U8, U16, U32, U64, F64, Bool, Char, void)
- Control flow: if/else, for, while, do-while, switch/case/default
- Jumps: return, break, continue, goto, labels
- Structs (class), unions, enums
- Arithmetic, bitwise, logical, comparison operators
- Compound assignments (+=, -=, etc.)
- Ternary operator, sizeof, casts
- Member access (. and ->)
- Array indexing, function calls
- Prefix/postfix increment/decrement
- Single and multi-line comments
- asm/_asm blocks

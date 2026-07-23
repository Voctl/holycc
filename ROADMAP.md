# Roadmap

## v0.1.0 — MVP (Current)

- [x] Lexer with 114 token kinds
- [x] Recursive descent parser
- [x] AST with visitor pattern
- [x] Symbol table with lexical scopes
- [x] Type system (I8-I64, U8-U64, F64, Bool, Char, void, pointer, array, struct, union, enum)
- [x] Semantic analysis (undeclared vars, duplicate defs, break/continue checks)
- [x] C17 code generation with type mapping
- [x] CLI driver with --tokens, --ast, --help, --version
- [x] Runtime library stub
- [x] Lexer test suite (161 tests)

---

## v0.2.0 — Language Completeness

- [ ] `#include` directive (file inlining)
- [ ] `#define` macro support (constant definitions)
- [ ] `import` system
- [ ] Function pointers
- [ ] Multi-dimensional arrays
- [ ] Array initializers `{1, 2, 3}`
- [ ] Struct initializers
- [ ] String escape handling in lexer (`\n`, `\t`, `\\`, etc.)
- [ ] `extern` / `_extern` linkage
- [ ] `U8 *` string type support
- [ ] Improved `asm` / `_asm` handling
- [ ] Ternary operator improvements
- [ ] Operator precedence full HolyC compliance

---

## v0.3.0 — Robustness

- [ ] Comprehensive error recovery in lexer
- [ ] Error recovery in parser (synchronization)
- [ ] Warning system (unused variables, type narrowing, etc.)
- [ ] Human-friendly diagnostics with source snippets and highlights
- [ ] Column-precise source locations
- [ ] Memory leak-free (valgrind clean)

---

## v0.4.0 — Tooling

- [ ] Parser test suite (snapshot-based)
- [ ] Codegen test suite (compile + diff)
- [ ] Semantic analysis test suite
- [ ] Regression test framework
- [ ] Fuzzer for lexer/parser
- [ ] Benchmark suite

---

## v0.5.0 — Advanced Features

- [ ] `#assert` support
- [ ] HolyC `class` methods
- [ ] Virtual methods
- [ ] `has` keyword
- [ ] `I64 Reg` pattern
- [ ] Color literal syntax (e.g., `BLACK`, `WHITE`)
- [ ] HolyC format strings (`"%d"`, `"%f"`, etc.)
- [ ] Built-in functions (full Print family, MemSet, etc.)

---

## v1.0.0 — Stable Release

- [ ] Complete HolyC language spec coverage
- [ ] Full regression suite (1000+ tests)
- [ ] Documentation (language reference, compiler internals, contributing)
- [ ] CI/CD pipeline (GitHub Actions)
- [ ] Package manager integration
- [ ] man page
- [ ] Installable via `brew`, `apt`, `pacman`

---

## Future (Beyond v1.0)

- [ ] LLVM backend (IR generation instead of C transpilation)
- [ ] Native x86-64 backend
- [ ] ARM64 backend
- [ ] JIT compilation
- [ ] Optimization passes (constant folding, dead code elimination, inlining)
- [ ] LSP server for IDE support
- [ ] HolyC formatter
- [ ] Debug info generation
- [ ] HolyC standard library
- [ ] Profile-guided optimization

---

## Design Principles (Non-Negotiable)

1. **Modularity** — Every subsystem replaceable without rewrite
2. **Zero warnings** — Always builds clean with `-Wall -Wextra -Wpedantic`
3. **Readable output** — Generated C should look like handwritten C
4. **No technical debt** — Fix bugs, don't patch
5. **Tests first** — Never fix a bug without adding a regression test
6. **Documentation** — Every major subsystem documented

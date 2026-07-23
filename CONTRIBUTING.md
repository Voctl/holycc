# Contributing to holycc

## Getting Started

```bash
git clone https://github.com/YOUR_USER/holycc.git
cd holycc
cmake -B build && cmake --build build
./build/tests/test_lexer
```

## Development Workflow

1. **Fork** the repository
2. Create a **feature branch**: `git checkout -b feature/my-feature`
3. Write your code
4. Build with **zero warnings**: `cmake --build build`
5. Run tests: `./build/tests/test_lexer`
6. Run valgrind: `valgrind --leak-check=full ./build/tests/test_lexer`
7. Commit with a descriptive message
8. Open a Pull Request

## Code Style

- **C17** standard, no GNU extensions
- Compile with `-Wall -Wextra -Wpedantic` — **zero warnings**
- Small functions (prefer <50 lines)
- Descriptive variable and function names
- Use `const` wherever possible
- No global mutable state
- Avoid macros — use inline functions or enums
- Single responsibility per function
- No deep nesting (>3 levels is a warning sign)
- Comments only when the "why" is not obvious from the code

## Module Boundaries

Never mix concerns across module boundaries:

- `lexer/` — Only tokenization
- `parser/` — Only AST production, no code generation
- `ast/` — Only tree data structures and traversal
- `semantic/` — Only analysis and checking, no code gen
- `codegen/` — Only C17 output, no analysis
- `driver/` — Only CLI orchestration

If you need to share data between modules, add a clean interface to `include/holyc/`.

## Testing

Every PR must include tests:

| Change | Required Tests |
|---|---|
| New token | Lexer snapshot test |
| New syntax | Parser test + codegen test |
| New semantic rule | Semantic test (pass and fail cases) |
| Bug fix | Regression test that fails before the fix |

## Commit Messages

```
area: short description in imperative mood

Longer explanation if needed. Reference issue numbers.
```

Examples:
```
lexer: add ^= compound assignment token
parser: fix infinite loop in binary expression parsing  
codegen: map I64 to int64_t in generated C
```

## Pull Request Checklist

- [ ] Builds with zero warnings
- [ ] All existing tests pass
- [ ] New tests added for new functionality
- [ ] No memory leaks (valgrind clean)
- [ ] Code follows style guide
- [ ] Commit messages are descriptive

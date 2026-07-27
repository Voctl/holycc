#ifndef HOLYC_CODEGEN_H
#define HOLYC_CODEGEN_H

#include "holyc/ast.h"
#include "holyc/symbol.h"
#include <stdio.h>

// Opaque code-generator state
typedef struct CodeGen CodeGen;

// Create/destroy the C17 code generator
CodeGen *codegen_create(SymbolTable *symtab);
void codegen_destroy(CodeGen *cg);

// Emit C17 source text – to an open FILE or directly to a file path
bool codegen_generate(CodeGen *cg, AstNode *ast, FILE *output);
bool codegen_generate_file(CodeGen *cg, AstNode *ast, const char *output_path);

#endif

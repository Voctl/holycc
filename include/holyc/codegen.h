#ifndef HOLYC_CODEGEN_H
#define HOLYC_CODEGEN_H

#include "holyc/ast.h"
#include "holyc/symbol.h"
#include <stdio.h>

typedef struct CodeGen CodeGen;

CodeGen *codegen_create(SymbolTable *symtab);
void codegen_destroy(CodeGen *cg);

bool codegen_generate(CodeGen *cg, AstNode *ast, FILE *output);
bool codegen_generate_file(CodeGen *cg, AstNode *ast, const char *output_path);

#endif

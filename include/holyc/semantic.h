#ifndef HOLYC_SEMANTIC_H
#define HOLYC_SEMANTIC_H

#include "holyc/ast.h"
#include "holyc/symbol.h"
#include "holyc/diag.h"

typedef struct Semantic Semantic;

Semantic *semantic_create(Diagnostics *diag);
void semantic_destroy(Semantic *semantic);

bool semantic_analyze(Semantic *semantic, AstNode *ast);
SymbolTable *semantic_get_symbol_table(Semantic *semantic);

#endif

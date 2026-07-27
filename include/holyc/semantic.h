#ifndef HOLYC_SEMANTIC_H
#define HOLYC_SEMANTIC_H

#include "holyc/ast.h"
#include "holyc/symbol.h"
#include "holyc/diag.h"

// Opaque semantic-analysis state
typedef struct Semantic Semantic;

// Create/destroy the semantic analyser
Semantic *semantic_create(Diagnostics *diag);
void semantic_destroy(Semantic *semantic);

// Run semantic analysis (type-checking, symbol resolution) on the AST
bool semantic_analyze(Semantic *semantic, AstNode *ast);

// Access the symbol table built during analysis
SymbolTable *semantic_get_symbol_table(Semantic *semantic);

#endif

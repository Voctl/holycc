#ifndef HOLYC_SYMBOL_H
#define HOLYC_SYMBOL_H

#include "holyc/token.h"
#include "holyc/types.h"
#include "holyc/diag.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_BLOCK,
    SCOPE_STRUCT,
    SCOPE_UNION,
} ScopeKind;

typedef enum {
    SYM_VARIABLE,
    SYM_FUNCTION,
    SYM_STRUCT,
    SYM_UNION,
    SYM_ENUM,
    SYM_CONSTANT,
    SYM_LABEL,
    SYM_TYPEDEF,
} SymbolKind;

typedef enum {
    STORAGE_NONE,
    STORAGE_STATIC,
    STORAGE_EXTERN,
} StorageClass;

typedef struct Symbol Symbol;
typedef struct Scope Scope;
typedef struct SymbolTable SymbolTable;

struct Symbol {
    const char *name;
    SymbolKind kind;
    Type *type;
    StorageClass storage;
    SourceLocation decl_loc;
    bool is_defined;
    Scope *owner;
    Symbol *next_in_scope;
    Symbol *next_in_table;
};

struct Scope {
    ScopeKind kind;
    Scope *parent;
    Symbol *first_symbol;
    Symbol *last_symbol;
    size_t symbol_count;
};

struct SymbolTable {
    Scope *global_scope;
    Scope *current_scope;
    Diagnostics *diag;
};

SymbolTable *symbol_table_create(Diagnostics *diag);
void symbol_table_destroy(SymbolTable *table);

void scope_push(SymbolTable *table, ScopeKind kind);
void scope_pop(SymbolTable *table);
Scope *scope_current(SymbolTable *table);
Scope *scope_global(SymbolTable *table);

Symbol *symbol_add(SymbolTable *table, const char *name, SymbolKind kind, Type *type, SourceLocation loc);
Symbol *symbol_lookup(SymbolTable *table, const char *name);
Symbol *symbol_lookup_current_scope(SymbolTable *table, const char *name);

const char *symbol_kind_name(SymbolKind kind);
const char *storage_class_name(StorageClass sc);

#endif

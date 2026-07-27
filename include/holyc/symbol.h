#ifndef HOLYC_SYMBOL_H
#define HOLYC_SYMBOL_H

#include "holyc/token.h"
#include "holyc/types.h"
#include "holyc/diag.h"
#include <stdbool.h>
#include <stddef.h>

// Kind of lexical scope
typedef enum {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_BLOCK,
    SCOPE_STRUCT,
    SCOPE_UNION,
} ScopeKind;

// What a symbol can represent
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

// Storage class qualifiers
typedef enum {
    STORAGE_NONE,
    STORAGE_STATIC,
    STORAGE_EXTERN,
} StorageClass;

typedef struct Symbol Symbol;
typedef struct Scope Scope;
typedef struct SymbolTable SymbolTable;

// A named symbol (variable, function, type, etc.)
struct Symbol {
    const char *name;
    SymbolKind kind;
    Type *type;
    StorageClass storage;
    SourceLocation decl_loc;
    bool is_defined;
    Scope *owner;          // the scope that contains this symbol
    Symbol *next_in_scope; // linked list within a single scope
    Symbol *next_in_table; // (unused currently)
};

// A lexical scope (linked list of scopes)
struct Scope {
    ScopeKind kind;
    Scope *parent;          // outer scope (NULL for global)
    Symbol *first_symbol;   // head of symbol list
    Symbol *last_symbol;    // tail of symbol list
    size_t symbol_count;
};

// The full symbol table
struct SymbolTable {
    Scope *global_scope;
    Scope *current_scope;
    Diagnostics *diag;
};

// Symbol table lifecycle
SymbolTable *symbol_table_create(Diagnostics *diag);
void symbol_table_destroy(SymbolTable *table);

// Scope management
void scope_push(SymbolTable *table, ScopeKind kind);
void scope_pop(SymbolTable *table);
Scope *scope_current(SymbolTable *table);
Scope *scope_global(SymbolTable *table);

// Symbol registration and lookup
Symbol *symbol_add(SymbolTable *table, const char *name, SymbolKind kind, Type *type, SourceLocation loc);
Symbol *symbol_lookup(SymbolTable *table, const char *name);           // walk scope chain
Symbol *symbol_lookup_current_scope(SymbolTable *table, const char *name); // current scope only

// Introspection
const char *symbol_kind_name(SymbolKind kind);
const char *storage_class_name(StorageClass sc);

#endif

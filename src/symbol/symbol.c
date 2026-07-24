#include "holyc/symbol.h"
#include <stdlib.h>
#include <string.h>

SymbolTable *symbol_table_create(Diagnostics *diag) {
    SymbolTable *table = calloc(1, sizeof(SymbolTable));
    table->diag = diag;
    table->global_scope = calloc(1, sizeof(Scope));
    table->global_scope->kind = SCOPE_GLOBAL;
    table->current_scope = table->global_scope;
    return table;
}

static void scope_free_symbols(Scope *scope) {
    Symbol *sym = scope->first_symbol;
    while (sym) {
        Symbol *next = sym->next_in_scope;
        free((void *)sym->name);
        free(sym);
        sym = next;
    }
}

static void scope_free(Scope *scope) {
    if (!scope) return;
    scope_free_symbols(scope);
    free(scope);
}

void symbol_table_destroy(SymbolTable *table) {
    if (!table) return;
    Scope *scope = table->global_scope;
    while (scope) {
        Scope *parent = scope->parent;
        scope_free(scope);
        scope = parent;
    }
    free(table);
}

void scope_push(SymbolTable *table, ScopeKind kind) {
    Scope *scope = calloc(1, sizeof(Scope));
    scope->kind = kind;
    scope->parent = table->current_scope;
    table->current_scope = scope;
}

void scope_pop(SymbolTable *table) {
    if (!table->current_scope) return;
    Scope *parent = table->current_scope->parent;
    if (!parent) return;
    scope_free(table->current_scope);
    table->current_scope = parent;
}

Scope *scope_current(SymbolTable *table) {
    return table->current_scope;
}

Scope *scope_global(SymbolTable *table) {
    return table->global_scope;
}

Symbol *symbol_add(SymbolTable *table, const char *name, SymbolKind kind,
                   Type *type, SourceLocation loc) {
    Scope *scope = table->current_scope;

    Symbol *existing = symbol_lookup_current_scope(table, name);
    if (existing) {
        if (table->diag) {
            table->diag->error(loc, "duplicate definition of '%s'", name);
        }
        if (existing->decl_loc.line > 0) {
            table->diag->note(existing->decl_loc, "previous definition here");
        }
        return NULL;
    }

    Symbol *sym = calloc(1, sizeof(Symbol));
    sym->name = strdup(name);
    sym->kind = kind;
    sym->type = type;
    sym->storage = STORAGE_NONE;
    sym->decl_loc = loc;
    sym->is_defined = false;
    sym->owner = scope;

    if (scope->last_symbol) {
        scope->last_symbol->next_in_scope = sym;
    } else {
        scope->first_symbol = sym;
    }
    scope->last_symbol = sym;
    scope->symbol_count++;

    return sym;
}

Symbol *symbol_lookup(SymbolTable *table, const char *name) {
    Scope *scope = table->current_scope;
    while (scope) {
        Symbol *sym = scope->first_symbol;
        while (sym) {
            if (strcmp(sym->name, name) == 0) {
                return sym;
            }
            sym = sym->next_in_scope;
        }
        scope = scope->parent;
    }
    return NULL;
}

Symbol *symbol_lookup_current_scope(SymbolTable *table, const char *name) {
    Scope *scope = table->current_scope;
    Symbol *sym = scope->first_symbol;
    while (sym) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next_in_scope;
    }
    return NULL;
}

const char *symbol_kind_name(SymbolKind kind) {
    switch (kind) {
        case SYM_VARIABLE:  return "variable";
        case SYM_FUNCTION:  return "function";
        case SYM_STRUCT:    return "struct";
        case SYM_UNION:     return "union";
        case SYM_ENUM:      return "enum";
        case SYM_CONSTANT:  return "constant";
        case SYM_LABEL:     return "label";
        case SYM_TYPEDEF:   return "typedef";
    }
    return "unknown";
}

const char *storage_class_name(StorageClass sc) {
    switch (sc) {
        case STORAGE_NONE:   return "none";
        case STORAGE_STATIC: return "static";
        case STORAGE_EXTERN: return "extern";
    }
    return "unknown";
}

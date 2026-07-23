#include "holyc/semantic.h"
#include <stdlib.h>
#include <string.h>

struct Semantic {
    SymbolTable *symtab;
    Diagnostics *diag;
    Type *current_func_return;
    int loop_depth;
    int switch_depth;
};

Semantic *semantic_create(Diagnostics *diag) {
    Semantic *s = calloc(1, sizeof(Semantic));
    s->symtab = symbol_table_create(diag);
    s->diag = diag;
    return s;
}

void semantic_destroy(Semantic *semantic) {
    if (semantic) {
        symbol_table_destroy(semantic->symtab);
        free(semantic);
    }
}

SymbolTable *semantic_get_symbol_table(Semantic *semantic) {
    return semantic->symtab;
}

static bool semantic_is_assign_op(TokenKind kind) {
    switch (kind) {
        case TOK_ASSIGN:          case TOK_PLUS_ASSIGN:   case TOK_MINUS_ASSIGN:
        case TOK_STAR_ASSIGN:     case TOK_SLASH_ASSIGN:  case TOK_PERCENT_ASSIGN:
        case TOK_AMPERSAND_ASSIGN: case TOK_PIPE_ASSIGN:  case TOK_CARET_ASSIGN:
        case TOK_LSHIFT_ASSIGN:   case TOK_RSHIFT_ASSIGN:
            return true;
        default:
            return false;
    }
}

static Type *semantic_analyze_expr(Semantic *s, AstNode *node);
static void semantic_analyze_stmt(Semantic *s, AstNode *node);

static Type *semantic_resolve_type(Semantic *s, AstNode *node) {
    if (!node) return NULL;

    switch (node->kind) {
        case AST_NAMED_TYPE: {
            const char *name = node->data.string_value;
            if (!name) return type_void();

            if (strcmp(name, "void") == 0) return type_void();
            if (strcmp(name, "U0") == 0) return type_u0();
            if (strcmp(name, "Bool") == 0) return type_bool();
            if (strcmp(name, "Char") == 0) return type_char();
            if (strcmp(name, "I8") == 0)  return type_i8();
            if (strcmp(name, "I16") == 0) return type_i16();
            if (strcmp(name, "I32") == 0) return type_i32();
            if (strcmp(name, "I64") == 0) return type_i64();
            if (strcmp(name, "U8") == 0)  return type_u8();
            if (strcmp(name, "U16") == 0) return type_u16();
            if (strcmp(name, "U32") == 0) return type_u32();
            if (strcmp(name, "U64") == 0) return type_u64();
            if (strcmp(name, "F64") == 0) return type_f64();

            Symbol *sym = symbol_lookup(s->symtab, name);
            if (sym && (sym->kind == SYM_STRUCT || sym->kind == SYM_UNION || sym->kind == SYM_ENUM)) {
                return sym->type;
            }

            Type *t = type_create(TYPE_UNRESOLVED);
            t->name = strdup(name);
            return t;
        }
        case AST_POINTER_TYPE: {
            Type *base = semantic_resolve_type(s, node->first_child);
            return type_pointer(base);
        }
        case AST_ARRAY_TYPE: {
            Type *base = semantic_resolve_type(s, node->first_child);
            return type_array(base, 0);
        }
        default:
            return NULL;
    }
}

static Type *semantic_analyze_expr(Semantic *s, AstNode *node) {
    if (!node) return type_void();

    switch (node->kind) {
        case AST_IDENTIFIER: {
            const char *name = node->data.string_value;
            Symbol *sym = symbol_lookup(s->symtab, name);
            if (!sym) {
                if (strcmp(name, "NULL") == 0 || strcmp(name, "TRUE") == 0 ||
                    strcmp(name, "FALSE") == 0) {
                    return (strcmp(name, "TRUE") == 0 || strcmp(name, "FALSE") == 0)
                           ? type_bool() : type_pointer(type_void());
                }
                s->diag->error(node->loc, "undefined identifier '%s'", name);
                return type_create(TYPE_ERROR);
            }
            return sym->type;
        }

        case AST_INTEGER_LITERAL:  return type_i64();
        case AST_FLOAT_LITERAL:    return type_f64();
        case AST_STRING_LITERAL:   return type_pointer(type_char());
        case AST_CHAR_LITERAL:     return type_char();
        case AST_BOOL_LITERAL:     return type_bool();
        case AST_NULL_LITERAL:     return type_pointer(type_void());

        case AST_UNARY_EXPR: {
            Type *operand = semantic_analyze_expr(s, node->first_child);
            TokenKind op = node->data.token_kind;

            switch (op) {
                case TOK_STAR:
                    if (operand->kind == TYPE_POINTER) return operand->base;
                    s->diag->error(node->loc, "cannot dereference non-pointer type");
                    return type_create(TYPE_ERROR);
                case TOK_AMPERSAND:
                    return type_pointer(operand);
                case TOK_MINUS:
                case TOK_TILDE:
                case TOK_INCREMENT:
                case TOK_DECREMENT:
                    return operand;
                case TOK_EXCLAIM:
                    return type_bool();
                default:
                    return operand;
            }
        }

        case AST_BINARY_EXPR: {
            TokenKind op = node->data.token_kind;
            Type *left = semantic_analyze_expr(s, node->first_child);
            Type *right = semantic_analyze_expr(s, node->first_child->next);

            if (semantic_is_assign_op(op)) {
                return left;
            }

            switch (op) {
                case TOK_EQ: case TOK_NE: case TOK_LT: case TOK_GT:
                case TOK_LE: case TOK_GE:
                case TOK_AMPERSAND_AMPERSAND: case TOK_PIPE_PIPE:
                    return type_bool();
                default:
                    if (type_is_floating(left) || type_is_floating(right)) {
                        return type_f64();
                    }
                    return left ? left : right;
            }
        }

        case AST_CONDITIONAL_EXPR: {
            semantic_analyze_expr(s, node->first_child);
            Type *t = semantic_analyze_expr(s, node->first_child->next);
            semantic_analyze_expr(s, node->first_child->next->next);
            return t;
        }

        case AST_CALL_EXPR: {
            AstNode *func_node = node->first_child;
            const char *func_name = NULL;
            if (func_node->kind == AST_IDENTIFIER) {
                func_name = func_node->data.string_value;
            }

            Symbol *sym = func_name ? symbol_lookup(s->symtab, func_name) : NULL;
            Type *return_type = type_void();
            if (sym && sym->type && sym->type->kind == TYPE_FUNCTION) {
                return_type = sym->type->function.return_type;
            }

            AstNode *arg = func_node->next;
            while (arg) {
                semantic_analyze_expr(s, arg);
                arg = arg->next;
            }

            return return_type;
        }

        case AST_INDEX_EXPR: {
            Type *base = semantic_analyze_expr(s, node->first_child);
            semantic_analyze_expr(s, node->first_child->next);
            if (base->kind == TYPE_POINTER) return base->base;
            if (base->kind == TYPE_ARRAY) return base->array.base;
            return type_create(TYPE_ERROR);
        }

        case AST_MEMBER_EXPR: {
            semantic_analyze_expr(s, node->first_child);
            return type_i64();
        }

        case AST_POINTER_MEMBER_EXPR: {
            semantic_analyze_expr(s, node->first_child);
            return type_i64();
        }

        case AST_SIZEOF_EXPR: {
            return type_u64();
        }

        case AST_CAST_EXPR: {
            Type *target = semantic_resolve_type(s, node->first_child);
            Type *expr_type = semantic_analyze_expr(s, node->first_child->next);
            return target ? target : expr_type;
        }

        default:
            return type_void();
    }
}

static void semantic_analyze_stmt(Semantic *s, AstNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_FUNC_DECL: {
            AstNode *type_node = node->first_child;
            AstNode *name_node = type_node->next;
            const char *name = name_node ? name_node->data.string_value : NULL;

            Type *return_type = semantic_resolve_type(s, type_node);

            FuncParam *params = NULL;
            FuncParam **last = &params;

            AstNode *child = name_node ? name_node->next : NULL;
            AstNode *body = NULL;

            if (child && child->kind == AST_FUNC_PARAM) {
                while (child && child->kind == AST_FUNC_PARAM) {
                    AstNode *ptype = child->first_child;
                    AstNode *pname = ptype ? ptype->next : NULL;
                    const char *pname_str = pname ? pname->data.string_value : NULL;

                    Type *pt = semantic_resolve_type(s, ptype);

                    FuncParam *fp = calloc(1, sizeof(FuncParam));
                    fp->name = pname_str ? strdup(pname_str) : strdup("");
                    fp->type = pt;
                    *last = fp;
                    last = &fp->next;

                    child = child->next;
                }
            }
            body = child;

            bool variadic = (node->flags & AST_FLAG_VARIADIC) != 0;
            Type *func_type = type_function(return_type, params, variadic);

            if (name) {
                Symbol *sym = symbol_add(s->symtab, name, SYM_FUNCTION, func_type, node->loc);
                if (sym) sym->is_defined = true;
            }

            if (body && body->kind == AST_BLOCK) {
                Type *prev_return = s->current_func_return;
                s->current_func_return = return_type;

                scope_push(s->symtab, SCOPE_FUNCTION);

                FuncParam *fp = params;
                while (fp) {
                    symbol_add(s->symtab, fp->name, SYM_VARIABLE, fp->type, node->loc);
                    fp = fp->next;
                }

                AstNode *stmt = body->first_child;
                while (stmt) {
                    semantic_analyze_stmt(s, stmt);
                    stmt = stmt->next;
                }

                scope_pop(s->symtab);
                s->current_func_return = prev_return;
            }
            break;
        }

        case AST_VAR_DECL: {
            AstNode *type_node = node->first_child;
            AstNode *name_node = type_node->next;
            const char *name = name_node ? name_node->data.string_value : NULL;
            Type *var_type = semantic_resolve_type(s, type_node);

            if (name) {
                Symbol *existing = symbol_lookup_current_scope(s->symtab, name);
                if (existing) {
                    s->diag->error(node->loc, "duplicate definition of '%s'", name);
                    s->diag->note(existing->decl_loc, "previous definition here");
                } else {
                    symbol_add(s->symtab, name, SYM_VARIABLE, var_type, node->loc);
                }
            }

            AstNode *init = name_node ? name_node->next : NULL;
            if (init) {
                semantic_analyze_expr(s, init);
            }
            break;
        }

        case AST_STRUCT_DECL:
        case AST_UNION_DECL: {
            AstNode *name_node = node->first_child;
            const char *name = (name_node && name_node->kind == AST_IDENTIFIER)
                               ? name_node->data.string_value : NULL;
            TypeKind tk = (node->kind == AST_UNION_DECL) ? TYPE_UNION : TYPE_STRUCT;
            Type *struct_type = type_create(tk);
            struct_type->name = name ? strdup(name) : NULL;
            struct_type->aggregate.complete = true;

            if (name) {
                symbol_add(s->symtab, name,
                           (node->kind == AST_UNION_DECL) ? SYM_UNION : SYM_STRUCT,
                           struct_type, node->loc);
            }

            AstNode *field = name_node ? name_node->next : node->first_child;
            StructField **last_field = &struct_type->aggregate.fields;
            uint64_t offset = 0;

            while (field) {
                if (field->kind != AST_STRUCT_FIELD) break;
                AstNode *ftype = field->first_child;
                Type *field_type = semantic_resolve_type(s, ftype);

                StructField *sf = calloc(1, sizeof(StructField));
                sf->type = field_type;
                if (ftype && ftype->next && ftype->next->kind == AST_IDENTIFIER) {
                    sf->name = strdup(ftype->next->data.string_value);
                }
                sf->offset = offset;
                if (tk == TYPE_UNION) {
                    sf->offset = 0;
                    if (field_type->size > struct_type->size) {
                        struct_type->size = field_type->size;
                    }
                } else {
                    offset += field_type->size;
                }
                *last_field = sf;
                last_field = &sf->next;
                field = field->next;
            }

            if (tk == TYPE_STRUCT) {
                struct_type->size = (uint32_t)offset;
            }
            break;
        }

        case AST_ENUM_DECL: {
            AstNode *name_node = node->first_child;
            const char *name = (name_node && name_node->kind == AST_IDENTIFIER)
                               ? name_node->data.string_value : NULL;
            Type *enum_type = type_create(TYPE_ENUM);
            enum_type->name = name ? strdup(name) : NULL;
            enum_type->size = 4;
            enum_type->alignment = 4;

            if (name) {
                symbol_add(s->symtab, name, SYM_ENUM, enum_type, node->loc);
            }

            AstNode *enumerator = name_node ? name_node->next : node->first_child;
            while (enumerator) {
                if (enumerator->kind != AST_ENUMERATOR) break;
                AstNode *ename = enumerator->first_child;
                const char *en_name = ename ? ename->data.string_value : NULL;

                if (en_name) {
                    symbol_add(s->symtab, en_name, SYM_CONSTANT, type_i64(), node->loc);
                }
                enumerator = enumerator->next;
            }
            break;
        }

        case AST_BLOCK: {
            scope_push(s->symtab, SCOPE_BLOCK);
            AstNode *child = node->first_child;
            while (child) {
                semantic_analyze_stmt(s, child);
                child = child->next;
            }
            scope_pop(s->symtab);
            break;
        }

        case AST_IF_STMT: {
            semantic_analyze_expr(s, node->first_child);
            AstNode *then_branch = node->first_child->next;
            AstNode *else_branch = then_branch ? then_branch->next : NULL;
            semantic_analyze_stmt(s, then_branch);
            if (else_branch) semantic_analyze_stmt(s, else_branch);
            break;
        }

        case AST_WHILE_STMT:
        case AST_DO_WHILE_STMT: {
            AstNode *cond = node->first_child;
            AstNode *body = cond ? cond->next : NULL;
            semantic_analyze_expr(s, cond);
            s->loop_depth++;
            semantic_analyze_stmt(s, body);
            s->loop_depth--;
            break;
        }

        case AST_FOR_STMT: {
            AstNode *init = node->first_child;
            AstNode *cond = init ? init->next : NULL;
            AstNode *incr = cond ? cond->next : NULL;
            AstNode *body = incr ? incr->next : NULL;

            scope_push(s->symtab, SCOPE_BLOCK);
            semantic_analyze_stmt(s, init);
            if (cond) semantic_analyze_expr(s, cond);
            if (incr) semantic_analyze_expr(s, incr);
            s->loop_depth++;
            semantic_analyze_stmt(s, body);
            s->loop_depth--;
            scope_pop(s->symtab);
            break;
        }

        case AST_SWITCH_STMT: {
            semantic_analyze_expr(s, node->first_child);
            s->switch_depth++;
            AstNode *child = node->first_child->next;
            while (child) {
                semantic_analyze_stmt(s, child);
                child = child->next;
            }
            s->switch_depth--;
            break;
        }

        case AST_CASE_STMT:
        case AST_DEFAULT_STMT: {
            AstNode *child = node->first_child;
            if (node->kind == AST_CASE_STMT) {
                semantic_analyze_expr(s, child);
                child = child->next;
            }
            while (child) {
                semantic_analyze_stmt(s, child);
                child = child->next;
            }
            break;
        }

        case AST_RETURN_STMT:
            if (node->first_child) {
                semantic_analyze_expr(s, node->first_child);
            }
            break;

        case AST_BREAK_STMT:
            if (s->loop_depth == 0 && s->switch_depth == 0) {
                s->diag->error(node->loc, "'break' outside loop or switch");
            }
            break;

        case AST_CONTINUE_STMT:
            if (s->loop_depth == 0) {
                s->diag->error(node->loc, "'continue' outside loop");
            } else {
                s->diag->warning(node->loc, "'continue' is not HolyC — use 'goto' instead");
            }
            break;

        case AST_GOTO_STMT:
            break;

        case AST_LABEL_STMT: {
            AstNode *label = node->first_child;
            if (label && label->kind == AST_IDENTIFIER) {
                const char *name = label->data.string_value;
                symbol_add(s->symtab, name, SYM_LABEL, NULL, node->loc);
            }
            break;
        }

        case AST_EXPR_STMT: {
            semantic_analyze_expr(s, node->first_child);
            break;
        }

        case AST_TRY_STMT:
        case AST_CATCH_STMT:
        case AST_THROW_STMT:
            break;

        default: {
            AstNode *child = node->first_child;
            while (child) {
                semantic_analyze_stmt(s, child);
                child = child->next;
            }
            break;
        }
    }
}

bool semantic_analyze(Semantic *semantic, AstNode *ast) {
    if (!ast || ast->kind != AST_TRANSLATION_UNIT) return false;

    scope_push(semantic->symtab, SCOPE_GLOBAL);

    AstNode *child = ast->first_child;
    while (child) {
        semantic_analyze_stmt(semantic, child);
        child = child->next;
    }

    scope_pop(semantic->symtab);
    return !semantic->diag->had_error;
}

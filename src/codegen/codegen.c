#include "holyc/codegen.h"
#include "holyc/utils.h"
#include <stdlib.h>
#include <string.h>

typedef struct FuncNameNode {
    char *name;
    struct FuncNameNode *next;
} FuncNameNode;

struct CodeGen {
    SymbolTable *symtab;
    int indent_level;
    StringBuffer buf;
    FuncNameNode *func_names;
};

static const char *codegen_map_type_name(const char *name) {
    if (!name) return "void";
    if (strcmp(name, "I8") == 0)   return "int8_t";
    if (strcmp(name, "I16") == 0)  return "int16_t";
    if (strcmp(name, "I32") == 0)  return "int32_t";
    if (strcmp(name, "I64") == 0)  return "int64_t";
    if (strcmp(name, "U8") == 0)   return "uint8_t";
    if (strcmp(name, "U16") == 0)  return "uint16_t";
    if (strcmp(name, "U32") == 0)  return "uint32_t";
    if (strcmp(name, "U64") == 0)  return "uint64_t";
    if (strcmp(name, "F64") == 0)  return "double";
    if (strcmp(name, "Bool") == 0) return "bool";
    if (strcmp(name, "Char") == 0) return "char";
    if (strcmp(name, "void") == 0) return "void";
    if (strcmp(name, "U0") == 0) return "void";
    return name;
}

CodeGen *codegen_create(SymbolTable *symtab) {
    CodeGen *cg = calloc(1, sizeof(CodeGen));
    cg->symtab = symtab;
    cg->buf = string_buffer_create();
    return cg;
}

void codegen_destroy(CodeGen *cg) {
    if (cg) {
        FuncNameNode *n = cg->func_names;
        while (n) {
            FuncNameNode *next = n->next;
            free(n->name);
            free(n);
            n = next;
        }
        string_buffer_destroy(&cg->buf);
        free(cg);
    }
}

static void codegen_emit_indent(CodeGen *cg) {
    string_buffer_indent(&cg->buf, cg->indent_level);
}

static void codegen_add_func_name(CodeGen *cg, const char *name) {
    FuncNameNode *n = malloc(sizeof(FuncNameNode));
    n->name = strdup(name);
    n->next = cg->func_names;
    cg->func_names = n;
}

static bool codegen_is_func_name(CodeGen *cg, const char *name) {
    FuncNameNode *n = cg->func_names;
    while (n) {
        if (strcmp(n->name, name) == 0) return true;
        n = n->next;
    }
    return false;
}

static void codegen_emit_runtime_protos(CodeGen *cg) {
    string_buffer_append_cstr(&cg->buf,
        "// HolyC runtime built-in functions\n"
        "void Print(const char *fmt, ...);\n"
        "void PrintLn(const char *fmt, ...);\n"
        "void *MAlloc(uint64_t size);\n"
        "void Free(void *ptr);\n"
        "uint64_t StrLen(const char *str);\n"
        "bool StrCompare(const char *a, const char *b);\n"
        "int64_t AtoI(const char *str);\n"
        "double AtoF(const char *str);\n"
        "void MemSet(uint8_t *dst, uint8_t value, uint64_t count);\n"
        "void MemCpy(uint8_t *dst, const uint8_t *src, uint64_t count);\n"
        "int64_t MemCompare(const uint8_t *a, const uint8_t *b, uint64_t count);\n"
        "uint64_t MSize(void *ptr);\n"
        "void CDelay(uint64_t ms);\n"
        "int GetCh(void);\n"
        "void PutChar(char c);\n"
        "void Exit(int64_t code);\n"
        "int SPrint(char *buf, const char *fmt, ...);\n\n"
    );
}

static void codegen_emit_expr(CodeGen *cg, AstNode *node);
static void codegen_emit_stmt(CodeGen *cg, AstNode *node);

static void codegen_emit_expr(CodeGen *cg, AstNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_IDENTIFIER:
            string_buffer_append_cstr(&cg->buf, node->data.string_value);
            break;

        case AST_INTEGER_LITERAL:
            string_buffer_append_fmt(&cg->buf, "%lld", (long long)node->data.int_value);
            break;

        case AST_FLOAT_LITERAL:
            string_buffer_append_fmt(&cg->buf, "%.16g", node->data.float_value);
            break;

        case AST_STRING_LITERAL:
            string_buffer_append_char(&cg->buf, '"');
            if (node->data.string_value) {
                string_buffer_append_cstr(&cg->buf, node->data.string_value);
            }
            string_buffer_append_char(&cg->buf, '"');
            break;

        case AST_CHAR_LITERAL: {
            string_buffer_append_char(&cg->buf, '\'');
            char c = (char)node->data.int_value;
            if (c == '\\') string_buffer_append_cstr(&cg->buf, "\\\\");
            else if (c == '\'') string_buffer_append_cstr(&cg->buf, "\\'");
            else if (c == '\n') string_buffer_append_cstr(&cg->buf, "\\n");
            else if (c == '\t') string_buffer_append_cstr(&cg->buf, "\\t");
            else if (c >= 32 && c < 127) string_buffer_append_char(&cg->buf, c);
            else string_buffer_append_fmt(&cg->buf, "\\x%02x", (unsigned char)c);
            string_buffer_append_char(&cg->buf, '\'');
            break;
        }

        case AST_BOOL_LITERAL:
            string_buffer_append_cstr(&cg->buf, node->data.int_value ? "true" : "false");
            break;

        case AST_NULL_LITERAL:
            string_buffer_append_cstr(&cg->buf, "NULL");
            break;

        case AST_UNARY_EXPR: {
            TokenKind op = node->data.token_kind;
            AstNode *operand = node->first_child;

            switch (op) {
                case TOK_MINUS:
                case TOK_EXCLAIM:
                case TOK_TILDE:
                    string_buffer_append_char(&cg->buf, token_kind_spelling(op)[0]);
                    codegen_emit_expr(cg, operand);
                    break;
                case TOK_STAR:
                    string_buffer_append_cstr(&cg->buf, "(*");
                    codegen_emit_expr(cg, operand);
                    string_buffer_append_char(&cg->buf, ')');
                    break;
                case TOK_AMPERSAND:
                    string_buffer_append_cstr(&cg->buf, "(&");
                    codegen_emit_expr(cg, operand);
                    string_buffer_append_char(&cg->buf, ')');
                    break;
                case TOK_INCREMENT:
                    string_buffer_append_cstr(&cg->buf, "++");
                    codegen_emit_expr(cg, operand);
                    break;
                case TOK_DECREMENT:
                    string_buffer_append_cstr(&cg->buf, "--");
                    codegen_emit_expr(cg, operand);
                    break;
                default:
                    codegen_emit_expr(cg, operand);
                    break;
            }
            break;
        }

        case AST_BINARY_EXPR: {
            TokenKind op = node->data.token_kind;
            AstNode *left = node->first_child;
            AstNode *right = left ? left->next : NULL;

            if (op == TOK_BACKTICK) {
                string_buffer_append_cstr(&cg->buf, "pow(");
                codegen_emit_expr(cg, left);
                string_buffer_append_cstr(&cg->buf, ", ");
                codegen_emit_expr(cg, right);
                string_buffer_append_char(&cg->buf, ')');
            } else {
                string_buffer_append_char(&cg->buf, '(');
                codegen_emit_expr(cg, left);
                string_buffer_append_char(&cg->buf, ' ');
                string_buffer_append_cstr(&cg->buf, token_kind_spelling(op));
                string_buffer_append_char(&cg->buf, ' ');
                codegen_emit_expr(cg, right);
                string_buffer_append_char(&cg->buf, ')');
            }
            break;
        }

        case AST_CONDITIONAL_EXPR: {
            AstNode *cond = node->first_child;
            AstNode *then_branch = cond ? cond->next : NULL;
            AstNode *else_branch = then_branch ? then_branch->next : NULL;

            string_buffer_append_char(&cg->buf, '(');
            codegen_emit_expr(cg, cond);
            string_buffer_append_cstr(&cg->buf, " ? ");
            codegen_emit_expr(cg, then_branch);
            string_buffer_append_cstr(&cg->buf, " : ");
            codegen_emit_expr(cg, else_branch);
            string_buffer_append_char(&cg->buf, ')');
            break;
        }

        case AST_CALL_EXPR: {
            AstNode *func = node->first_child;
            codegen_emit_expr(cg, func);
            string_buffer_append_char(&cg->buf, '(');
            AstNode *arg = func ? func->next : NULL;
            bool first = true;
            while (arg) {
                if (!first) string_buffer_append_cstr(&cg->buf, ", ");
                codegen_emit_expr(cg, arg);
                first = false;
                arg = arg->next;
            }
            string_buffer_append_char(&cg->buf, ')');
            break;
        }

        case AST_INDEX_EXPR: {
            AstNode *base = node->first_child;
            AstNode *index = base ? base->next : NULL;
            codegen_emit_expr(cg, base);
            string_buffer_append_char(&cg->buf, '[');
            codegen_emit_expr(cg, index);
            string_buffer_append_char(&cg->buf, ']');
            break;
        }

        case AST_MEMBER_EXPR: {
            AstNode *obj = node->first_child;
            AstNode *member = obj ? obj->next : NULL;
            codegen_emit_expr(cg, obj);
            string_buffer_append_char(&cg->buf, '.');
            codegen_emit_expr(cg, member);
            break;
        }

        case AST_POINTER_MEMBER_EXPR: {
            AstNode *obj = node->first_child;
            AstNode *member = obj ? obj->next : NULL;
            codegen_emit_expr(cg, obj);
            string_buffer_append_cstr(&cg->buf, "->");
            codegen_emit_expr(cg, member);
            break;
        }

        case AST_CAST_EXPR: {
            string_buffer_append_cstr(&cg->buf, "((");
            if (node->first_child && node->first_child->kind == AST_NAMED_TYPE) {
                codegen_emit_expr(cg, node->first_child);
            }
            string_buffer_append_cstr(&cg->buf, ")");
            codegen_emit_expr(cg, node->first_child ? node->first_child->next : NULL);
            string_buffer_append_char(&cg->buf, ')');
            break;
        }

        case AST_SIZEOF_EXPR: {
            string_buffer_append_cstr(&cg->buf, "sizeof(");
            codegen_emit_expr(cg, node->first_child);
            string_buffer_append_char(&cg->buf, ')');
            break;
        }

        case AST_OFFSET_EXPR: {
            string_buffer_append_cstr(&cg->buf, "offsetof(");
            codegen_emit_expr(cg, node->first_child);
            string_buffer_append_char(&cg->buf, ',');
            string_buffer_append_char(&cg->buf, ' ');
            codegen_emit_expr(cg, node->first_child->next);
            string_buffer_append_char(&cg->buf, ')');
            break;
        }

        case AST_ARRAY_INIT: {
            string_buffer_append_char(&cg->buf, '{');
            AstNode *child = node->first_child;
            bool first = true;
            while (child) {
                if (!first) string_buffer_append_cstr(&cg->buf, ", ");
                codegen_emit_expr(cg, child);
                first = false;
                child = child->next;
            }
            string_buffer_append_char(&cg->buf, '}');
            break;
        }

        default:
            break;
    }
}

static void codegen_emit_stmt(CodeGen *cg, AstNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_TRANSLATION_UNIT: {
            string_buffer_append_cstr(&cg->buf, "#include <stdint.h>\n");
            string_buffer_append_cstr(&cg->buf, "#include <stdbool.h>\n");
            string_buffer_append_cstr(&cg->buf, "#include <stddef.h>\n");
            string_buffer_append_cstr(&cg->buf, "#include <stdio.h>\n");
            string_buffer_append_cstr(&cg->buf, "#include <stdlib.h>\n");
            string_buffer_append_cstr(&cg->buf, "#include <string.h>\n");
            string_buffer_append_cstr(&cg->buf, "#include <math.h>\n\n");

            codegen_emit_runtime_protos(cg);

            AstNode *child = node->first_child;
            bool has_explicit_main = false;
            bool has_top_stmts = false;

            child = node->first_child;
            while (child) {
                if (child->kind == AST_FUNC_DECL) {
                    AstNode *t = child->first_child;
                    AstNode *n = t ? t->next : NULL;
                    if (n && n->kind == AST_IDENTIFIER) {
                        codegen_add_func_name(cg, n->data.string_value);
                        if (strcmp(n->data.string_value, "main") == 0) {
                            has_explicit_main = true;
                        }
                    }
                }
                if (!ast_kind_is_declaration(child->kind)) {
                    has_top_stmts = true;
                }
                child = child->next;
            }

            if (has_top_stmts && !has_explicit_main) {
                bool in_main = false;
                child = node->first_child;
                while (child) {
                    if (ast_kind_is_declaration(child->kind)) {
                        if (in_main) {
                            codegen_emit_indent(cg);
                            codegen_emit_stmt(cg, child);
                        } else {
                            codegen_emit_stmt(cg, child);
                        }
                    } else {
                        if (!in_main) {
                            string_buffer_append_cstr(&cg->buf, "\nint main() {\n");
                            cg->indent_level++;
                            in_main = true;
                        }
                        codegen_emit_indent(cg);
                        codegen_emit_stmt(cg, child);
                    }
                    child = child->next;
                }
                if (in_main) {
                    cg->indent_level--;
                    codegen_emit_indent(cg);
                    string_buffer_append_cstr(&cg->buf, "}\n");
                }
            } else {
                child = node->first_child;
                while (child) {
                    codegen_emit_stmt(cg, child);
                    child = child->next;
                }
            }
            break;
        }

        case AST_FUNC_DECL: {
            AstNode *type_node = node->first_child;
            AstNode *name_node = type_node->next;

            string_buffer_append_char(&cg->buf, '\n');

            if (type_node && type_node->kind == AST_NAMED_TYPE) {
                const char *name_str = name_node ? name_node->data.string_value : NULL;
                if (name_str && strcmp(name_str, "main") == 0) {
                    string_buffer_append_cstr(&cg->buf, "int");
                } else {
                    string_buffer_append_cstr(&cg->buf, codegen_map_type_name(type_node->data.string_value));
                }
            }

            string_buffer_append_char(&cg->buf, ' ');
            codegen_emit_expr(cg, name_node);
            string_buffer_append_char(&cg->buf, '(');

            AstNode *child = name_node ? name_node->next : NULL;
            bool first_param = true;
            while (child && child->kind == AST_FUNC_PARAM) {
                if (!first_param) string_buffer_append_cstr(&cg->buf, ", ");
                AstNode *ptype = child->first_child;
                AstNode *pname = ptype ? ptype->next : NULL;

                if (ptype && ptype->kind == AST_NAMED_TYPE) {
                    string_buffer_append_cstr(&cg->buf, codegen_map_type_name(ptype->data.string_value));
                    string_buffer_append_char(&cg->buf, ' ');
                }
                if (pname) {
                    codegen_emit_expr(cg, pname);
                }
                first_param = false;
                child = child->next;
            }

            string_buffer_append_cstr(&cg->buf, ") ");

            if (child && child->kind == AST_BLOCK) {
                codegen_emit_stmt(cg, child);
            } else {
                string_buffer_append_cstr(&cg->buf, ";\n");
            }
            break;
        }

        case AST_VAR_DECL: {
            AstNode *type_node = node->first_child;
            AstNode *name_node = type_node->next;
            AstNode *init = name_node ? name_node->next : NULL;

            if (node->flags & AST_FLAG_STATIC) {
                string_buffer_append_cstr(&cg->buf, "static ");
            }
            if (node->flags & AST_FLAG_EXTERN) {
                string_buffer_append_cstr(&cg->buf, "extern ");
            }

            if (type_node && type_node->kind == AST_FUNC_POINTER_TYPE) {
                AstNode *return_type = type_node->first_child;
                AstNode *fp_name = return_type ? return_type->next : NULL;
                if (return_type && return_type->kind == AST_NAMED_TYPE) {
                    string_buffer_append_cstr(&cg->buf, codegen_map_type_name(return_type->data.string_value));
                }
                string_buffer_append_cstr(&cg->buf, " (*");
                codegen_emit_expr(cg, fp_name);
                string_buffer_append_cstr(&cg->buf, ")(");
                AstNode *param = fp_name ? fp_name->next : NULL;
                bool first_param = true;
                while (param && param->kind == AST_FUNC_PARAM) {
                    if (!first_param) string_buffer_append_cstr(&cg->buf, ", ");
                    AstNode *ptype = param->first_child;
                    if (ptype && ptype->kind == AST_NAMED_TYPE) {
                        string_buffer_append_cstr(&cg->buf, codegen_map_type_name(ptype->data.string_value));
                    }
                    first_param = false;
                    param = param->next;
                }
                string_buffer_append_cstr(&cg->buf, ")");
            } else if (type_node && type_node->kind == AST_NAMED_TYPE) {
                string_buffer_append_cstr(&cg->buf, codegen_map_type_name(type_node->data.string_value));
                string_buffer_append_char(&cg->buf, ' ');
            } else if (type_node && type_node->kind == AST_ARRAY_TYPE) {
                AstNode *elem_type = type_node->first_child;
                while (elem_type && elem_type->kind == AST_ARRAY_TYPE) {
                    elem_type = elem_type->first_child;
                }
                if (elem_type && elem_type->kind == AST_NAMED_TYPE) {
                    string_buffer_append_cstr(&cg->buf, codegen_map_type_name(elem_type->data.string_value));
                }
                string_buffer_append_char(&cg->buf, ' ');
            }

            if (type_node->kind != AST_FUNC_POINTER_TYPE) {
                codegen_emit_expr(cg, name_node);
            }

            if (type_node && type_node->kind == AST_ARRAY_TYPE) {
                AstNode *arr = type_node;
                while (arr && arr->kind == AST_ARRAY_TYPE) {
                    string_buffer_append_char(&cg->buf, '[');
                    AstNode *size = arr->first_child->next;
                    if (size) {
                        codegen_emit_expr(cg, size);
                    }
                    string_buffer_append_char(&cg->buf, ']');
                    arr = arr->first_child;
                }
            }

            if (init) {
                string_buffer_append_cstr(&cg->buf, " = ");
                codegen_emit_expr(cg, init);
            }

            string_buffer_append_cstr(&cg->buf, ";\n");
            break;
        }

        case AST_BLOCK: {
            string_buffer_append_cstr(&cg->buf, "{\n");
            cg->indent_level++;
            AstNode *child = node->first_child;
            while (child) {
                codegen_emit_indent(cg);
                codegen_emit_stmt(cg, child);
                child = child->next;
            }
            cg->indent_level--;
            codegen_emit_indent(cg);
            string_buffer_append_cstr(&cg->buf, "}\n");

            if (cg->indent_level == 0) {
                string_buffer_append_char(&cg->buf, '\n');
            }
            break;
        }

        case AST_IF_STMT: {
            string_buffer_append_cstr(&cg->buf, "if (");
            codegen_emit_expr(cg, node->first_child);
            string_buffer_append_cstr(&cg->buf, ") ");

            AstNode *then_branch = node->first_child->next;
            codegen_emit_stmt(cg, then_branch);

            AstNode *else_branch = then_branch ? then_branch->next : NULL;
            if (else_branch) {
                codegen_emit_indent(cg);
                string_buffer_append_cstr(&cg->buf, "else ");
                codegen_emit_stmt(cg, else_branch);
            }
            break;
        }

        case AST_WHILE_STMT: {
            string_buffer_append_cstr(&cg->buf, "while (");
            codegen_emit_expr(cg, node->first_child);
            string_buffer_append_cstr(&cg->buf, ") ");
            codegen_emit_stmt(cg, node->first_child->next);
            break;
        }

        case AST_DO_WHILE_STMT: {
            string_buffer_append_cstr(&cg->buf, "do ");
            codegen_emit_stmt(cg, node->first_child);
            codegen_emit_indent(cg);
            string_buffer_append_cstr(&cg->buf, "while (");
            codegen_emit_expr(cg, node->first_child->next);
            string_buffer_append_cstr(&cg->buf, ");\n");
            break;
        }

        case AST_FOR_STMT: {
            string_buffer_append_cstr(&cg->buf, "for (");
            AstNode *init = node->first_child;
            if (init && init->kind != AST_NONE) {
                codegen_emit_stmt(cg, init);
            } else {
                string_buffer_append_char(&cg->buf, ';');
            }
            string_buffer_append_char(&cg->buf, ' ');
            AstNode *cond = init ? init->next : NULL;
            if (cond && cond->kind != AST_NONE) {
                codegen_emit_expr(cg, cond);
            }
            string_buffer_append_cstr(&cg->buf, "; ");
            AstNode *incr = cond ? cond->next : NULL;
            if (incr && incr->kind != AST_NONE) {
                codegen_emit_expr(cg, incr);
            }
            string_buffer_append_cstr(&cg->buf, ") ");
            codegen_emit_stmt(cg, (incr ? incr->next : NULL));
            break;
        }

        case AST_SWITCH_STMT: {
            string_buffer_append_cstr(&cg->buf, "switch (");
            codegen_emit_expr(cg, node->first_child);
            string_buffer_append_cstr(&cg->buf, ") {\n");
            cg->indent_level++;
            AstNode *child = node->first_child->next;
            while (child) {
                codegen_emit_indent(cg);
                codegen_emit_stmt(cg, child);
                child = child->next;
            }
            cg->indent_level--;
            codegen_emit_indent(cg);
            string_buffer_append_cstr(&cg->buf, "}\n");
            break;
        }

        case AST_CASE_STMT: {
            string_buffer_append_cstr(&cg->buf, "case ");
            codegen_emit_expr(cg, node->first_child);
            string_buffer_append_cstr(&cg->buf, ":\n");
            cg->indent_level++;
            AstNode *child = node->first_child->next;
            while (child) {
                codegen_emit_indent(cg);
                codegen_emit_stmt(cg, child);
                child = child->next;
            }
            cg->indent_level--;
            break;
        }

        case AST_CASE_RANGE: {
            AstNode *start = node->first_child;
            AstNode *end = start->next;
            uint64_t from = start->data.int_value;
            uint64_t to = end->data.int_value;
            for (uint64_t i = from; i <= to; i++) {
                string_buffer_append_fmt(&cg->buf, "case %llu:\n", (unsigned long long)i);
            }
            cg->indent_level++;
            AstNode *stmt = end->next;
            while (stmt) {
                codegen_emit_indent(cg);
                codegen_emit_stmt(cg, stmt);
                stmt = stmt->next;
            }
            cg->indent_level--;
            break;
        }

        case AST_DEFAULT_STMT: {
            string_buffer_append_cstr(&cg->buf, "default:\n");
            cg->indent_level++;
            AstNode *child = node->first_child;
            while (child) {
                codegen_emit_indent(cg);
                codegen_emit_stmt(cg, child);
                child = child->next;
            }
            cg->indent_level--;
            break;
        }

        case AST_RETURN_STMT: {
            string_buffer_append_cstr(&cg->buf, "return");
            if (node->first_child) {
                string_buffer_append_char(&cg->buf, ' ');
                codegen_emit_expr(cg, node->first_child);
            }
            string_buffer_append_cstr(&cg->buf, ";\n");
            break;
        }

        case AST_BREAK_STMT:
            string_buffer_append_cstr(&cg->buf, "break;\n");
            break;

        case AST_CONTINUE_STMT:
            string_buffer_append_cstr(&cg->buf, "continue;\n");
            break;

        case AST_GOTO_STMT:
            string_buffer_append_cstr(&cg->buf, "goto ");
            codegen_emit_expr(cg, node->first_child);
            string_buffer_append_cstr(&cg->buf, ";\n");
            break;

        case AST_LABEL_STMT: {
            codegen_emit_expr(cg, node->first_child);
            string_buffer_append_cstr(&cg->buf, ":\n");
            break;
        }

        case AST_EXPR_STMT: {
            AstNode *expr = node->first_child;
            if (expr && expr->kind == AST_STRING_LITERAL) {
                string_buffer_append_cstr(&cg->buf, "Print(");
                codegen_emit_expr(cg, expr);
                string_buffer_append_cstr(&cg->buf, ");\n");
            } else if (expr && expr->kind == AST_IDENTIFIER) {
                const char *name = expr->data.string_value;
                if (codegen_is_func_name(cg, name)) {
                    string_buffer_append_cstr(&cg->buf, name);
                    string_buffer_append_cstr(&cg->buf, "();\n");
                } else {
                    codegen_emit_expr(cg, expr);
                    string_buffer_append_cstr(&cg->buf, ";\n");
                }
            } else {
                codegen_emit_expr(cg, expr);
                string_buffer_append_cstr(&cg->buf, ";\n");
            }
            break;
        }

        case AST_STRUCT_DECL:
        case AST_UNION_DECL: {
            string_buffer_append_cstr(&cg->buf, "typedef ");
            string_buffer_append_cstr(&cg->buf, (node->kind == AST_UNION_DECL) ? "union" : "struct");
            string_buffer_append_cstr(&cg->buf, " {\n");
            cg->indent_level++;
            AstNode *field = node->first_child;
            if (field && field->kind == AST_IDENTIFIER) {
                field = field->next;
            }
            while (field) {
                if (field->kind != AST_STRUCT_FIELD) break;
                codegen_emit_indent(cg);
                AstNode *ftype = field->first_child;
                AstNode *fname = ftype ? ftype->next : NULL;
                if (ftype && ftype->kind == AST_NAMED_TYPE) {
                    string_buffer_append_cstr(&cg->buf, codegen_map_type_name(ftype->data.string_value));
                    string_buffer_append_char(&cg->buf, ' ');
                }
                codegen_emit_expr(cg, fname);
                string_buffer_append_cstr(&cg->buf, ";\n");
                field = field->next;
            }
            cg->indent_level--;
            string_buffer_append_cstr(&cg->buf, "} ");
            if (node->first_child && node->first_child->kind == AST_IDENTIFIER) {
                codegen_emit_expr(cg, node->first_child);
            }
            string_buffer_append_cstr(&cg->buf, ";\n\n");
            break;
        }

        case AST_ENUM_DECL: {
            string_buffer_append_cstr(&cg->buf, "typedef enum {\n");
            cg->indent_level++;
            AstNode *enumerator = node->first_child;
            if (enumerator && enumerator->kind == AST_IDENTIFIER) {
                enumerator = enumerator->next;
            }
            while (enumerator) {
                if (enumerator->kind != AST_ENUMERATOR) break;
                codegen_emit_indent(cg);
                AstNode *ename = enumerator->first_child;
                AstNode *eval = ename ? ename->next : NULL;
                codegen_emit_expr(cg, ename);
                if (eval) {
                    string_buffer_append_cstr(&cg->buf, " = ");
                    codegen_emit_expr(cg, eval);
                }
                string_buffer_append_cstr(&cg->buf, ",\n");
                enumerator = enumerator->next;
            }
            cg->indent_level--;
            string_buffer_append_cstr(&cg->buf, "} ");
            if (node->first_child && node->first_child->kind == AST_IDENTIFIER) {
                codegen_emit_expr(cg, node->first_child);
            }
            string_buffer_append_cstr(&cg->buf, ";\n\n");
            break;
        }

        case AST_ASM_STMT: {
            string_buffer_append_cstr(&cg->buf, "/* inline assembly */ ");
            if (node->first_child && node->first_child->kind == AST_STRING_LITERAL) {
                string_buffer_append_cstr(&cg->buf, "/* ");
                string_buffer_append_cstr(&cg->buf, node->first_child->data.string_value);
                string_buffer_append_cstr(&cg->buf, " */");
            }
            string_buffer_append_cstr(&cg->buf, ";\n");
            break;
        }

        case AST_DEFINE: {
            AstNode *name_node = node->first_child;
            AstNode *value_node = name_node ? name_node->next : NULL;
            string_buffer_append_cstr(&cg->buf, "#define ");
            codegen_emit_expr(cg, name_node);
            if (value_node) {
                string_buffer_append_char(&cg->buf, ' ');
                codegen_emit_expr(cg, value_node);
            }
            string_buffer_append_char(&cg->buf, '\n');
            break;
        }

        case AST_PP_IF:
        case AST_PP_IFDEF:
        case AST_PP_IFNDEF:
        case AST_PP_ELSE:
        case AST_PP_ELIF:
        case AST_PP_ENDIF: {
            AstNode *text = node->first_child;
            string_buffer_append_cstr(&cg->buf, text->data.string_value);
            string_buffer_append_char(&cg->buf, '\n');
            break;
        }

        case AST_INCLUDE: {
            AstNode *child = node->first_child->next;
            while (child) {
                codegen_emit_stmt(cg, child);
                child = child->next;
            }
            break;
        }

        default: {
            AstNode *child = node->first_child;
            while (child) {
                codegen_emit_stmt(cg, child);
                child = child->next;
            }
            break;
        }
    }
}

bool codegen_generate(CodeGen *cg, AstNode *ast, FILE *output) {
    if (!ast || !output) return false;

    codegen_emit_stmt(cg, ast);
    fprintf(output, "%s", cg->buf.data);
    return true;
}

bool codegen_generate_file(CodeGen *cg, AstNode *ast, const char *output_path) {
    FILE *f = fopen(output_path, "w");
    if (!f) return false;
    bool ok = codegen_generate(cg, ast, f);
    fclose(f);
    return ok;
}

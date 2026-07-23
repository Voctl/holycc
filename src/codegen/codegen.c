#include "holyc/codegen.h"
#include "holyc/utils.h"
#include <stdlib.h>
#include <string.h>

struct CodeGen {
    SymbolTable *symtab;
    int indent_level;
    StringBuffer buf;
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
        string_buffer_destroy(&cg->buf);
        free(cg);
    }
}

static void codegen_emit_indent(CodeGen *cg) {
    string_buffer_indent(&cg->buf, cg->indent_level);
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

            string_buffer_append_char(&cg->buf, '(');
            codegen_emit_expr(cg, left);
            string_buffer_append_char(&cg->buf, ' ');
            string_buffer_append_cstr(&cg->buf, token_kind_spelling(op));
            string_buffer_append_char(&cg->buf, ' ');
            codegen_emit_expr(cg, right);
            string_buffer_append_char(&cg->buf, ')');
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
            string_buffer_append_cstr(&cg->buf, "#include <string.h>\n\n");

            AstNode *child = node->first_child;
            while (child) {
                codegen_emit_stmt(cg, child);
                child = child->next;
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

            if (type_node && type_node->kind == AST_NAMED_TYPE) {
                string_buffer_append_cstr(&cg->buf, codegen_map_type_name(type_node->data.string_value));
                string_buffer_append_char(&cg->buf, ' ');
            }

            codegen_emit_expr(cg, name_node);

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
            codegen_emit_expr(cg, node->first_child);
            string_buffer_append_cstr(&cg->buf, ";\n");
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

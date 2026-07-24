#include "holyc/ast.h"
#include <stdlib.h>
#include <string.h>

AstNode *ast_node_create(AstKind kind, SourceLocation loc) {
    AstNode *node = calloc(1, sizeof(AstNode));
    node->kind = kind;
    node->loc = loc;
    node->flags = AST_FLAG_NONE;
    return node;
}

void ast_node_destroy(AstNode *node) {
    if (!node) return;
    if (node->kind == AST_STRING_LITERAL || node->kind == AST_IDENTIFIER) {
        free((void *)node->data.string_value);
    }
    free(node);
}

void ast_node_destroy_tree(AstNode *root) {
    if (!root) return;
    AstNode *child = root->first_child;
    while (child) {
        AstNode *next = child->next;
        ast_node_destroy_tree(child);
        child = next;
    }
    ast_node_destroy(root);
}

void ast_add_child(AstNode *parent, AstNode *child) {
    if (!parent || !child) return;
    child->parent = parent;
    if (parent->last_child) {
        parent->last_child->next = child;
    } else {
        parent->first_child = child;
    }
    parent->last_child = child;
}

void ast_set_type(AstNode *node, AstNode *type_node) {
    if (!node || !type_node) return;
    node->data.typed.type = type_node;
}

void ast_visit(AstNode *node, AstVisitor pre_visit, AstVisitor post_visit, void *ctx) {
    if (!node) return;

    if (pre_visit) {
        pre_visit(node, ctx);
    }

    AstNode *child = node->first_child;
    while (child) {
        ast_visit(child, pre_visit, post_visit, ctx);
        child = child->next;
    }

    if (post_visit) {
        post_visit(node, ctx);
    }
}

static const char *ast_kind_names[AST_COUNT] = {
    [AST_NONE]                  = "NONE",
    [AST_TRANSLATION_UNIT]      = "translation_unit",
    [AST_FUNC_DECL]             = "func_decl",
    [AST_STRUCT_DECL]           = "struct_decl",
    [AST_UNION_DECL]            = "union_decl",
    [AST_ENUM_DECL]             = "enum_decl",
    [AST_VAR_DECL]              = "var_decl",
    [AST_BLOCK]                 = "block",
    [AST_IF_STMT]               = "if_stmt",
    [AST_FOR_STMT]              = "for_stmt",
    [AST_WHILE_STMT]            = "while_stmt",
    [AST_DO_WHILE_STMT]         = "do_while_stmt",
    [AST_SWITCH_STMT]           = "switch_stmt",
    [AST_CASE_STMT]             = "case_stmt",
    [AST_CASE_RANGE]            = "case_range",
    [AST_DEFAULT_STMT]          = "default_stmt",
    [AST_BREAK_STMT]            = "break_stmt",
    [AST_CONTINUE_STMT]         = "continue_stmt",
    [AST_RETURN_STMT]           = "return_stmt",
    [AST_GOTO_STMT]             = "goto_stmt",
    [AST_LABEL_STMT]            = "label_stmt",
    [AST_EXPR_STMT]             = "expr_stmt",
    [AST_ASM_STMT]              = "asm_stmt",
    [AST_TRY_STMT]              = "try_stmt",
    [AST_CATCH_STMT]            = "catch_stmt",
    [AST_THROW_STMT]            = "throw_stmt",
    [AST_BINARY_EXPR]           = "binary_expr",
    [AST_UNARY_EXPR]            = "unary_expr",
    [AST_CONDITIONAL_EXPR]      = "conditional_expr",
    [AST_CALL_EXPR]             = "call_expr",
    [AST_INDEX_EXPR]            = "index_expr",
    [AST_MEMBER_EXPR]           = "member_expr",
    [AST_POINTER_MEMBER_EXPR]   = "pointer_member_expr",
    [AST_CAST_EXPR]             = "cast_expr",
    [AST_SIZEOF_EXPR]           = "sizeof_expr",
    [AST_OFFSET_EXPR]           = "offset_expr",
    [AST_IDENTIFIER]            = "identifier",
    [AST_INTEGER_LITERAL]       = "integer_literal",
    [AST_FLOAT_LITERAL]         = "float_literal",
    [AST_STRING_LITERAL]        = "string_literal",
    [AST_CHAR_LITERAL]          = "char_literal",
    [AST_BOOL_LITERAL]          = "bool_literal",
    [AST_NULL_LITERAL]          = "null_literal",
    [AST_ARRAY_INIT]            = "array_init",
    [AST_ARRAY_TYPE]            = "array_type",
    [AST_POINTER_TYPE]          = "pointer_type",
    [AST_FUNC_POINTER_TYPE]     = "func_pointer_type",
    [AST_NAMED_TYPE]            = "named_type",
    [AST_FUNC_PARAM]            = "func_param",
    [AST_ENUMERATOR]            = "enumerator",
    [AST_STRUCT_FIELD]          = "struct_field",
    [AST_DEFINE]                = "define",
    [AST_INCLUDE]               = "include",
    [AST_PP_IF]                 = "pp_if",
    [AST_PP_ELSE]               = "pp_else",
    [AST_PP_ENDIF]              = "pp_endif",
    [AST_PP_IFDEF]              = "pp_ifdef",
    [AST_PP_IFNDEF]             = "pp_ifndef",
    [AST_PP_ELIF]               = "pp_elif",
};

AstNode *ast_clone_node(const AstNode *node) {
    if (!node) return NULL;
    AstNode *clone = ast_node_create(node->kind, node->loc);
    clone->flags = node->flags;
    clone->data = node->data;
    if (node->data.string_value) {
        clone->data.string_value = strdup(node->data.string_value);
    }
    AstNode *child = node->first_child;
    AstNode *last = NULL;
    while (child) {
        AstNode *child_clone = ast_clone_node(child);
        child_clone->parent = clone;
        if (!clone->first_child) clone->first_child = child_clone;
        if (last) last->next = child_clone;
        last = child_clone;
        child = child->next;
    }
    clone->last_child = last;
    return clone;
}

const char *ast_kind_name(AstKind kind) {
    if (kind < AST_COUNT && ast_kind_names[kind]) {
        return ast_kind_names[kind];
    }
    return "unknown";
}

bool ast_kind_is_declaration(AstKind kind) {
    switch (kind) {
        case AST_FUNC_DECL:
        case AST_STRUCT_DECL:
        case AST_UNION_DECL:
        case AST_ENUM_DECL:
        case AST_VAR_DECL:
        case AST_DEFINE:
        case AST_INCLUDE:
        case AST_PP_IF:
        case AST_PP_ELSE:
        case AST_PP_ENDIF:
        case AST_PP_IFDEF:
        case AST_PP_IFNDEF:
        case AST_PP_ELIF:
            return true;
        default:
            return false;
    }
}

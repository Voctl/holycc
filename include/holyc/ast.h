#ifndef HOLYC_AST_H
#define HOLYC_AST_H

#include "holyc/token.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    AST_NONE = 0,

    AST_TRANSLATION_UNIT,

    AST_FUNC_DECL,
    AST_STRUCT_DECL,
    AST_UNION_DECL,
    AST_ENUM_DECL,
    AST_VAR_DECL,

    AST_BLOCK,
    AST_COMPOUND_STMT,

    AST_IF_STMT,
    AST_FOR_STMT,
    AST_WHILE_STMT,
    AST_DO_WHILE_STMT,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_CASE_RANGE,
    AST_DEFAULT_STMT,

    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_RETURN_STMT,
    AST_GOTO_STMT,
    AST_LABEL_STMT,

    AST_EXPR_STMT,
    AST_ASM_STMT,
    AST_TRY_STMT,
    AST_CATCH_STMT,
    AST_THROW_STMT,

    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_CONDITIONAL_EXPR,
    AST_CALL_EXPR,
    AST_INDEX_EXPR,
    AST_MEMBER_EXPR,
    AST_POINTER_MEMBER_EXPR,
    AST_CAST_EXPR,
    AST_SIZEOF_EXPR,
    AST_OFFSET_EXPR,

    AST_IDENTIFIER,
    AST_INTEGER_LITERAL,
    AST_FLOAT_LITERAL,
    AST_STRING_LITERAL,
    AST_CHAR_LITERAL,
    AST_BOOL_LITERAL,
    AST_NULL_LITERAL,

    AST_ARRAY_INIT,

    AST_ARRAY_TYPE,
    AST_POINTER_TYPE,
    AST_FUNC_POINTER_TYPE,
    AST_NAMED_TYPE,

    AST_FUNC_PARAM,
    AST_ENUMERATOR,
    AST_STRUCT_FIELD,

    AST_DEFINE,
    AST_INCLUDE,
    AST_PP_IF,
    AST_PP_ELSE,
    AST_PP_ENDIF,
    AST_PP_IFDEF,
    AST_PP_IFNDEF,
    AST_PP_ELIF,

    AST_COUNT
} AstKind;

typedef enum {
    AST_FLAG_NONE = 0,
    AST_FLAG_CONST = 1 << 0,
    AST_FLAG_STATIC = 1 << 1,
    AST_FLAG_EXTERN = 1 << 2,
    AST_FLAG_VARIADIC = 1 << 3,
} AstFlags;

typedef struct AstNode AstNode;

struct AstNode {
    AstKind kind;
    AstFlags flags;
    SourceLocation loc;
    AstNode *parent;
    AstNode *next;
    AstNode *first_child;
    AstNode *last_child;
    union {
        uint64_t int_value;
        double float_value;
        const char *string_value;
        TokenKind token_kind;
        struct {
            AstNode *type;
        } typed;
    } data;
};

typedef void (*AstVisitor)(AstNode *node, void *ctx);

AstNode *ast_node_create(AstKind kind, SourceLocation loc);
void ast_node_destroy(AstNode *node);
void ast_node_destroy_tree(AstNode *root);

void ast_add_child(AstNode *parent, AstNode *child);
void ast_set_type(AstNode *node, AstNode *type_node);
AstNode *ast_clone_node(const AstNode *node);

void ast_visit(AstNode *node, AstVisitor pre_visit, AstVisitor post_visit, void *ctx);

const char *ast_kind_name(AstKind kind);
bool ast_kind_is_declaration(AstKind kind);

#endif

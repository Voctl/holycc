#ifndef HOLYC_TOKEN_H
#define HOLYC_TOKEN_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    TOK_ERROR = 0,
    TOK_EOF,

    TOK_IDENTIFIER,

    TOK_INTEGER,
    TOK_FLOAT,
    TOK_STRING,
    TOK_CHARACTER,

    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,

    TOK_COMMA,
    TOK_DOT,
    TOK_ARROW,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_ELLIPSIS,

    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,

    TOK_AMPERSAND,
    TOK_PIPE,
    TOK_CARET,
    TOK_TILDE,
    TOK_EXCLAIM,

    TOK_AMPERSAND_AMPERSAND,
    TOK_PIPE_PIPE,

    TOK_LSHIFT,
    TOK_RSHIFT,

    TOK_EQ,
    TOK_NE,
    TOK_LT,
    TOK_GT,
    TOK_LE,
    TOK_GE,

    TOK_ASSIGN,

    TOK_PLUS_ASSIGN,
    TOK_MINUS_ASSIGN,
    TOK_STAR_ASSIGN,
    TOK_SLASH_ASSIGN,
    TOK_PERCENT_ASSIGN,
    TOK_AMPERSAND_ASSIGN,
    TOK_PIPE_ASSIGN,
    TOK_CARET_ASSIGN,
    TOK_LSHIFT_ASSIGN,
    TOK_RSHIFT_ASSIGN,

    TOK_INCREMENT,
    TOK_DECREMENT,

    TOK_QUESTION,

    TOK_KW_I8,
    TOK_KW_I16,
    TOK_KW_I32,
    TOK_KW_I64,
    TOK_KW_U8,
    TOK_KW_U16,
    TOK_KW_U32,
    TOK_KW_U64,
    TOK_KW_F64,
    TOK_KW_BOOL,
    TOK_KW_CHAR,
    TOK_KW_VOID,

    TOK_KW_IF,
    TOK_KW_ELSE,
    TOK_KW_FOR,
    TOK_KW_WHILE,
    TOK_KW_DO,
    TOK_KW_SWITCH,
    TOK_KW_CASE,
    TOK_KW_DEFAULT,
    TOK_KW_BREAK,
    TOK_KW_CONTINUE,
    TOK_KW_RETURN,
    TOK_KW_GOTO,

    TOK_KW_CLASS,
    TOK_KW_UNION,
    TOK_KW_ENUM,
    TOK_KW_SIZEOF,

    TOK_KW_STATIC,
    TOK_KW_EXTERN,
    TOK_KW__EXTERN,

    TOK_KW_ASM,
    TOK_KW__ASM,
    TOK_KW_IMPORT,
    TOK_KW_INCLUDE,

    TOK_KW_DEFINE,

    TOK_KW_NULL,
    TOK_KW_TRUE,
    TOK_KW_FALSE,

    TOK_KW_CONST,

    TOK_COUNT
} TokenKind;

typedef struct {
    const char *filename;
    uint32_t line;
    uint32_t column;
} SourceLocation;

typedef struct {
    TokenKind kind;
    const char *start;
    size_t length;
    SourceLocation loc;
} Token;

const char *token_kind_name(TokenKind kind);
const char *token_kind_spelling(TokenKind kind);

#endif

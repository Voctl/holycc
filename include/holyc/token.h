#ifndef HOLYC_TOKEN_H
#define HOLYC_TOKEN_H

#include <stddef.h>
#include <stdint.h>

// Token kinds – every lexeme the scanner can recognise
typedef enum {
    TOK_ERROR = 0,  // invalid / unrecognised input
    TOK_EOF,        // end of file

    TOK_IDENTIFIER, // user-defined name

    // Literals
    TOK_INTEGER,
    TOK_FLOAT,
    TOK_STRING,
    TOK_CHARACTER,

    // Brackets / grouping
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,

    // Punctuation
    TOK_COMMA,
    TOK_DOT,
    TOK_ARROW,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_ELLIPSIS,

    // Arithmetic operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,

    // Bitwise / logical operators
    TOK_AMPERSAND,
    TOK_PIPE,
    TOK_CARET,
    TOK_TILDE,
    TOK_EXCLAIM,

    // Boolean operators
    TOK_AMPERSAND_AMPERSAND,
    TOK_PIPE_PIPE,

    // Shift operators
    TOK_LSHIFT,
    TOK_RSHIFT,

    // Comparison operators
    TOK_EQ,
    TOK_NE,
    TOK_LT,
    TOK_GT,
    TOK_LE,
    TOK_GE,

    // Simple assignment
    TOK_ASSIGN,

    // Compound assignments (+=, -=, etc.)
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

    // Increment / decrement
    TOK_INCREMENT,
    TOK_DECREMENT,

    TOK_QUESTION, // ternary ?

    // Type keywords
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
    TOK_KW_U0,

    // Control-flow keywords
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

    // Aggregate keywords
    TOK_KW_CLASS,
    TOK_KW_UNION,
    TOK_KW_ENUM,
    TOK_KW_SIZEOF,

    // Storage-class keywords
    TOK_KW_STATIC,
    TOK_KW_EXTERN,
    TOK_KW__EXTERN,

    // Miscellaneous keywords
    TOK_KW_ASM,
    TOK_KW__ASM,
    TOK_KW_IMPORT,
    TOK_KW_INCLUDE,

    TOK_KW_DEFINE,

    // Built-in constant keywords
    TOK_KW_NULL,
    TOK_KW_TRUE,
    TOK_KW_FALSE,

    TOK_KW_CONST,

    // HolyC-specific keywords
    TOK_BACKTICK,     // power operator
    TOK_KW_PUBLIC,
    TOK_KW_PRIVATE,
    TOK_KW_OFFSET,
    TOK_KW_NO_WARN,

    TOK_KW_HAS,
    TOK_KW_REG,
    TOK_KW_NOREG,
    TOK_KW_TRY,
    TOK_KW_CATCH,
    TOK_KW_THROW,

    // Preprocessor directive tokens
    TOK_PP_IF,
    TOK_PP_ELSE,
    TOK_PP_ENDIF,
    TOK_PP_IFDEF,
    TOK_PP_IFNDEF,
    TOK_PP_ELIF,

    TOK_COUNT  // sentinel — number of token kinds
} TokenKind;

// Source location in a source file (1-indexed line/column)
typedef struct {
    const char *filename;
    uint32_t line;
    uint32_t column;
} SourceLocation;

// A single lexeme produced by the lexer
typedef struct {
    TokenKind kind;    // what kind of token
    const char *start; // pointer into source text
    size_t length;     // byte length of this token
    SourceLocation loc; // file/line/column
} Token;

// Human-readable name for a TokenKind (e.g. "I64")
const char *token_kind_name(TokenKind kind);
// The source-text spelling of a TokenKind (e.g. "I64")
const char *token_kind_spelling(TokenKind kind);

#endif

#include "holyc/token.h"

typedef struct {
    const char *name;
    const char *spelling;
} TokenInfo;

static const TokenInfo token_table[TOK_COUNT] = {
#define T(k, n, s) [k] = {n, s}
    T(TOK_ERROR,              "error",                "<error>"),
    T(TOK_EOF,                "EOF",                  "<eof>"),
    T(TOK_IDENTIFIER,         "identifier",           "<identifier>"),
    T(TOK_INTEGER,            "integer literal",      "<integer>"),
    T(TOK_FLOAT,              "float literal",        "<float>"),
    T(TOK_STRING,             "string literal",       "<string>"),
    T(TOK_CHARACTER,          "character literal",    "<char>"),
    T(TOK_LPAREN,             "(",                    "("),
    T(TOK_RPAREN,             ")",                    ")"),
    T(TOK_LBRACE,             "{",                    "{"),
    T(TOK_RBRACE,             "}",                    "}"),
    T(TOK_LBRACKET,           "[",                    "["),
    T(TOK_RBRACKET,           "]",                    "]"),
    T(TOK_COMMA,              ",",                    ","),
    T(TOK_DOT,                ".",                    "."),
    T(TOK_ARROW,              "->",                   "->"),
    T(TOK_COLON,              ":",                    ":"),
    T(TOK_SEMICOLON,          ";",                    ";"),
    T(TOK_ELLIPSIS,           "...",                  "..."),
    T(TOK_PLUS,               "+",                    "+"),
    T(TOK_MINUS,              "-",                    "-"),
    T(TOK_STAR,               "*",                    "*"),
    T(TOK_SLASH,              "/",                    "/"),
    T(TOK_PERCENT,            "%",                    "%"),
    T(TOK_AMPERSAND,          "&",                    "&"),
    T(TOK_PIPE,               "|",                    "|"),
    T(TOK_CARET,              "^",                    "^"),
    T(TOK_TILDE,              "~",                    "~"),
    T(TOK_EXCLAIM,            "!",                    "!"),
    T(TOK_AMPERSAND_AMPERSAND,"&&",                   "&&"),
    T(TOK_PIPE_PIPE,          "||",                   "||"),
    T(TOK_LSHIFT,             "<<",                   "<<"),
    T(TOK_RSHIFT,             ">>",                   ">>"),
    T(TOK_EQ,                 "==",                   "=="),
    T(TOK_NE,                 "!=",                   "!="),
    T(TOK_LT,                 "<",                    "<"),
    T(TOK_GT,                 ">",                    ">"),
    T(TOK_LE,                 "<=",                   "<="),
    T(TOK_GE,                 ">=",                   ">="),
    T(TOK_ASSIGN,             "=",                    "="),
    T(TOK_PLUS_ASSIGN,        "+=",                   "+="),
    T(TOK_MINUS_ASSIGN,       "-=",                   "-="),
    T(TOK_STAR_ASSIGN,        "*=",                   "*="),
    T(TOK_SLASH_ASSIGN,       "/=",                   "/="),
    T(TOK_PERCENT_ASSIGN,     "%=",                   "%="),
    T(TOK_AMPERSAND_ASSIGN,   "&=",                   "&="),
    T(TOK_PIPE_ASSIGN,        "|=",                   "|="),
    T(TOK_CARET_ASSIGN,       "^=",                   "^="),
    T(TOK_LSHIFT_ASSIGN,      "<<=",                  "<<="),
    T(TOK_RSHIFT_ASSIGN,      ">>=",                  ">>="),
    T(TOK_INCREMENT,          "++",                   "++"),
    T(TOK_DECREMENT,          "--",                   "--"),
    T(TOK_QUESTION,           "?",                    "?"),
    T(TOK_KW_I8,              "I8",                   "I8"),
    T(TOK_KW_I16,             "I16",                  "I16"),
    T(TOK_KW_I32,             "I32",                  "I32"),
    T(TOK_KW_I64,             "I64",                  "I64"),
    T(TOK_KW_U8,              "U8",                   "U8"),
    T(TOK_KW_U16,             "U16",                  "U16"),
    T(TOK_KW_U32,             "U32",                  "U32"),
    T(TOK_KW_U64,             "U64",                  "U64"),
    T(TOK_KW_F64,             "F64",                  "F64"),
    T(TOK_KW_BOOL,            "Bool",                 "Bool"),
    T(TOK_KW_CHAR,            "Char",                 "Char"),
    T(TOK_KW_VOID,            "void",                 "void"),
    T(TOK_KW_IF,              "if",                   "if"),
    T(TOK_KW_ELSE,            "else",                 "else"),
    T(TOK_KW_FOR,             "for",                  "for"),
    T(TOK_KW_WHILE,           "while",                "while"),
    T(TOK_KW_DO,              "do",                   "do"),
    T(TOK_KW_SWITCH,          "switch",               "switch"),
    T(TOK_KW_CASE,            "case",                 "case"),
    T(TOK_KW_DEFAULT,         "default",              "default"),
    T(TOK_KW_BREAK,           "break",                "break"),
    T(TOK_KW_CONTINUE,        "continue",             "continue"),
    T(TOK_KW_RETURN,          "return",               "return"),
    T(TOK_KW_GOTO,            "goto",                 "goto"),
    T(TOK_KW_CLASS,           "class",                "class"),
    T(TOK_KW_UNION,           "union",                "union"),
    T(TOK_KW_ENUM,            "enum",                 "enum"),
    T(TOK_KW_SIZEOF,          "sizeof",               "sizeof"),
    T(TOK_KW_STATIC,          "static",               "static"),
    T(TOK_KW_EXTERN,          "extern",               "extern"),
    T(TOK_KW__EXTERN,         "_extern",              "_extern"),
    T(TOK_KW_ASM,             "asm",                  "asm"),
    T(TOK_KW__ASM,            "_asm",                 "_asm"),
    T(TOK_KW_IMPORT,          "import",               "import"),
    T(TOK_KW_INCLUDE,         "#include",             "#include"),
    T(TOK_KW_DEFINE,          "#define",              "#define"),
    T(TOK_KW_NULL,            "NULL",                 "NULL"),
    T(TOK_KW_TRUE,            "TRUE",                 "TRUE"),
    T(TOK_KW_FALSE,           "FALSE",                "FALSE"),
    T(TOK_KW_CONST,           "const",                "const"),
#undef T
};

const char *token_kind_name(TokenKind kind) {
    if (kind < TOK_COUNT) {
        return token_table[kind].name;
    }
    return "unknown";
}

const char *token_kind_spelling(TokenKind kind) {
    if (kind < TOK_COUNT) {
        return token_table[kind].spelling;
    }
    return "???";
}

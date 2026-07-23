#include "holyc/lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char *keyword;
    size_t length;
    TokenKind kind;
} KeywordEntry;

static const KeywordEntry keywords[] = {
    {"I8",        2, TOK_KW_I8},
    {"I16",       3, TOK_KW_I16},
    {"I32",       3, TOK_KW_I32},
    {"I64",       3, TOK_KW_I64},
    {"U8",        2, TOK_KW_U8},
    {"U16",       3, TOK_KW_U16},
    {"U32",       3, TOK_KW_U32},
    {"U64",       3, TOK_KW_U64},
    {"F64",       3, TOK_KW_F64},
    {"Bool",      4, TOK_KW_BOOL},
    {"Char",      4, TOK_KW_CHAR},
    {"void",      4, TOK_KW_VOID},
    {"U0",        2, TOK_KW_U0},
    {"if",        2, TOK_KW_IF},
    {"else",      4, TOK_KW_ELSE},
    {"for",       3, TOK_KW_FOR},
    {"while",     5, TOK_KW_WHILE},
    {"do",        2, TOK_KW_DO},
    {"switch",    6, TOK_KW_SWITCH},
    {"case",      4, TOK_KW_CASE},
    {"default",   7, TOK_KW_DEFAULT},
    {"break",     5, TOK_KW_BREAK},
    {"continue",  8, TOK_KW_CONTINUE},
    {"return",    6, TOK_KW_RETURN},
    {"goto",      4, TOK_KW_GOTO},
    {"class",     5, TOK_KW_CLASS},
    {"union",     5, TOK_KW_UNION},
    {"enum",      4, TOK_KW_ENUM},
    {"sizeof",    6, TOK_KW_SIZEOF},
    {"static",    6, TOK_KW_STATIC},
    {"extern",    6, TOK_KW_EXTERN},
    {"_extern",   7, TOK_KW__EXTERN},
    {"asm",       3, TOK_KW_ASM},
    {"_asm",      4, TOK_KW__ASM},
    {"import",    6, TOK_KW_IMPORT},
    {"include",   7, TOK_KW_INCLUDE},
    {"define",    6, TOK_KW_DEFINE},
    {"NULL",      4, TOK_KW_NULL},
    {"TRUE",      4, TOK_KW_TRUE},
    {"FALSE",     5, TOK_KW_FALSE},
    {"const",     5, TOK_KW_CONST},
    {"public",    6, TOK_KW_PUBLIC},
    {"private",   7, TOK_KW_PRIVATE},
    {"offset",    6, TOK_KW_OFFSET},
    {"no_warn",   7, TOK_KW_NO_WARN},
    {"has",       3, TOK_KW_HAS},
    {"reg",       3, TOK_KW_REG},
    {"noreg",     5, TOK_KW_NOREG},
};

#define NUM_KEYWORDS (sizeof(keywords) / sizeof(keywords[0]))

struct Lexer {
    const char *filename;
    const char *source;
    const char *current;
    const char *line_start;
    size_t source_len;
    uint32_t line;
    uint32_t column;
    Token peek;
    bool has_peek;
    SourceLocation token_loc;
    Diagnostics *diag;
};

Lexer *lexer_create(const char *filename, const char *source, size_t source_len) {
    Lexer *l = calloc(1, sizeof(Lexer));
    l->filename = filename;
    l->source = source;
    l->current = source;
    l->line_start = source;
    l->source_len = source_len;
    l->line = 1;
    l->column = 1;
    l->token_loc.filename = filename;
    l->token_loc.line = 1;
    l->token_loc.column = 1;
    l->has_peek = false;
    return l;
}

void lexer_destroy(Lexer *lexer) {
    free(lexer);
}

void lexer_set_diagnostics(Lexer *lexer, Diagnostics *diag) {
    lexer->diag = diag;
}

static SourceLocation lexer_loc(const Lexer *lexer) {
    SourceLocation loc;
    loc.filename = lexer->filename;
    loc.line = lexer->line;
    loc.column = lexer->column;
    return loc;
}

static bool lexer_is_eof(const Lexer *lexer) {
    return (size_t)(lexer->current - lexer->source) >= lexer->source_len;
}

static char lexer_peek_char(const Lexer *lexer) {
    if (lexer_is_eof(lexer)) return '\0';
    return *lexer->current;
}

static char lexer_peek_char_n(const Lexer *lexer, int n) {
    if ((size_t)(lexer->current + n - lexer->source) >= lexer->source_len) return '\0';
    return lexer->current[n];
}

static char lexer_advance(Lexer *lexer) {
    if (lexer_is_eof(lexer)) return '\0';
    char c = *lexer->current;
    lexer->current++;
    lexer->column++;
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
        lexer->line_start = lexer->current;
    }
    return c;
}

static bool lexer_match(Lexer *lexer, char expected) {
    if (lexer_is_eof(lexer)) return false;
    if (*lexer->current != expected) return false;
    lexer_advance(lexer);
    return true;
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (!lexer_is_eof(lexer)) {
        char c = lexer_peek_char(lexer);
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                lexer_advance(lexer);
                break;
            case '/':
                if (lexer_peek_char_n(lexer, 1) == '/') {
                    while (!lexer_is_eof(lexer) && lexer_peek_char(lexer) != '\n') {
                        lexer_advance(lexer);
                    }
                } else if (lexer_peek_char_n(lexer, 1) == '*') {
                    lexer_advance(lexer);
                    lexer_advance(lexer);
                    while (!lexer_is_eof(lexer)) {
                        if (lexer_peek_char(lexer) == '*' &&
                            lexer_peek_char_n(lexer, 1) == '/') {
                            lexer_advance(lexer);
                            lexer_advance(lexer);
                            break;
                        }
                        lexer_advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token lexer_make_token(const Lexer *lexer, TokenKind kind,
                               const char *start, size_t length) {
    Token t;
    t.kind = kind;
    t.start = start;
    t.length = length;
    t.loc = lexer->token_loc;
    return t;
}

static TokenKind lexer_ident_kind(const char *start, size_t length) {
    for (size_t i = 0; i < NUM_KEYWORDS; i++) {
        if (keywords[i].length == length &&
            strncmp(keywords[i].keyword, start, length) == 0) {
            return keywords[i].kind;
        }
    }
    return TOK_IDENTIFIER;
}

static Token lexer_read_number(Lexer *lexer) {
    const char *start = lexer->current - 1;
    bool has_dot = false;
    bool has_exp = false;
    bool is_hex = false;
    bool is_bin = false;

    if (lexer_peek_char_n(lexer, -1) == '.') {
        has_dot = true;
    } else {
        char first = *start;
        if (first == '0') {
            char next = lexer_peek_char(lexer);
            if (next == 'x' || next == 'X') {
                is_hex = true;
                lexer_advance(lexer);
                while (!lexer_is_eof(lexer) && isxdigit((unsigned char)lexer_peek_char(lexer))) {
                    lexer_advance(lexer);
                }
            } else if (next == 'b' || next == 'B') {
                is_bin = true;
                lexer_advance(lexer);
                while (!lexer_is_eof(lexer) &&
                       (lexer_peek_char(lexer) == '0' || lexer_peek_char(lexer) == '1')) {
                    lexer_advance(lexer);
                }
            } else if (isdigit((unsigned char)next)) {
                while (!lexer_is_eof(lexer) && isdigit((unsigned char)lexer_peek_char(lexer))) {
                    lexer_advance(lexer);
                }
            }
        } else {
            while (!lexer_is_eof(lexer) && isdigit((unsigned char)lexer_peek_char(lexer))) {
                lexer_advance(lexer);
            }
        }
    }

    if (!is_hex && !is_bin) {
        if (lexer_peek_char(lexer) == '.' && lexer_peek_char_n(lexer, 1) != '.') {
            has_dot = true;
            lexer_advance(lexer);
            while (!lexer_is_eof(lexer) && isdigit((unsigned char)lexer_peek_char(lexer))) {
                lexer_advance(lexer);
            }
        }
        if (lexer_peek_char(lexer) == 'e' || lexer_peek_char(lexer) == 'E') {
            has_exp = true;
            lexer_advance(lexer);
            if (lexer_peek_char(lexer) == '+' || lexer_peek_char(lexer) == '-') {
                lexer_advance(lexer);
            }
            while (!lexer_is_eof(lexer) && isdigit((unsigned char)lexer_peek_char(lexer))) {
                lexer_advance(lexer);
            }
        }
    }

    size_t length = (size_t)(lexer->current - start);
    if (has_dot || has_exp) {
        return lexer_make_token(lexer, TOK_FLOAT, start, length);
    }
    return lexer_make_token(lexer, TOK_INTEGER, start, length);
}

static Token lexer_read_string(Lexer *lexer, char quote, TokenKind kind) {
    const char *start = lexer->current - 1;
    while (!lexer_is_eof(lexer) && lexer_peek_char(lexer) != quote) {
        if (lexer_peek_char(lexer) == '\\') {
            lexer_advance(lexer);
        }
        lexer_advance(lexer);
    }
    if (!lexer_is_eof(lexer)) {
        lexer_advance(lexer);
    }
    size_t length = (size_t)(lexer->current - start);
    return lexer_make_token(lexer, kind, start, length);
}

static Token lexer_read_identifier(Lexer *lexer) {
    const char *start = lexer->current - 1;
    while (!lexer_is_eof(lexer) && (isalnum((unsigned char)lexer_peek_char(lexer)) ||
                                    lexer_peek_char(lexer) == '_')) {
        lexer_advance(lexer);
    }
    size_t length = (size_t)(lexer->current - start);
    TokenKind kind = lexer_ident_kind(start, length);
    return lexer_make_token(lexer, kind, start, length);
}

static Token lexer_read_preprocessor(Lexer *lexer) {
    const char *start = lexer->current - 1;
    while (!lexer_is_eof(lexer) && (isalpha((unsigned char)lexer_peek_char(lexer)) ||
                                     lexer_peek_char(lexer) == '_')) {
        lexer_advance(lexer);
    }
    size_t length = (size_t)(lexer->current - start);
    TokenKind kind = lexer_ident_kind(start + 1, length - 1);
    if (kind == TOK_KW_IF) {
        while (!lexer_is_eof(lexer) && lexer_peek_char(lexer) != '\n') {
            lexer_advance(lexer);
        }
        length = (size_t)(lexer->current - start);
        kind = TOK_PP_IF;
    } else if (kind == TOK_KW_ELSE) {
        kind = TOK_PP_ELSE;
    } else if (kind == TOK_IDENTIFIER) {
        const char *text = start + 1;
        size_t tlen = length - 1;
        if (tlen == 5 && strncmp(text, "endif", 5) == 0) kind = TOK_PP_ENDIF;
        else if (tlen == 5 && strncmp(text, "ifdef", 5) == 0) kind = TOK_PP_IFDEF;
        else if (tlen == 6 && strncmp(text, "ifndef", 6) == 0) kind = TOK_PP_IFNDEF;
        else if (tlen == 4 && strncmp(text, "elif", 4) == 0) kind = TOK_PP_ELIF;
        else kind = TOK_ERROR;
    }
    return lexer_make_token(lexer, kind, start, length);
}

Token lexer_next_token(Lexer *lexer) {
    if (lexer->has_peek) {
        lexer->has_peek = false;
        return lexer->peek;
    }

    lexer_skip_whitespace(lexer);

    lexer->token_loc = lexer_loc(lexer);

    if (lexer_is_eof(lexer)) {
        Token t;
        t.kind = TOK_EOF;
        t.start = lexer->current;
        t.length = 0;
        t.loc = lexer->token_loc;
        return t;
    }

    char c = lexer_advance(lexer);
    const char *start = lexer->current - 1;

    if (isalpha((unsigned char)c) || c == '_') {
        return lexer_read_identifier(lexer);
    }

    if (isdigit((unsigned char)c)) {
        return lexer_read_number(lexer);
    }

    if (c == '.' && !lexer_is_eof(lexer) && isdigit((unsigned char)lexer_peek_char(lexer))) {
        return lexer_read_number(lexer);
    }

    switch (c) {
        case '#': return lexer_read_preprocessor(lexer);

        case '`': return lexer_make_token(lexer, TOK_BACKTICK, start, 1);

        case '(': return lexer_make_token(lexer, TOK_LPAREN, start, 1);
        case ')': return lexer_make_token(lexer, TOK_RPAREN, start, 1);
        case '{': return lexer_make_token(lexer, TOK_LBRACE, start, 1);
        case '}': return lexer_make_token(lexer, TOK_RBRACE, start, 1);
        case '[': return lexer_make_token(lexer, TOK_LBRACKET, start, 1);
        case ']': return lexer_make_token(lexer, TOK_RBRACKET, start, 1);
        case ',': return lexer_make_token(lexer, TOK_COMMA, start, 1);
        case ':': return lexer_make_token(lexer, TOK_COLON, start, 1);
        case ';': return lexer_make_token(lexer, TOK_SEMICOLON, start, 1);
        case '?': return lexer_make_token(lexer, TOK_QUESTION, start, 1);
        case '~': return lexer_make_token(lexer, TOK_TILDE, start, 1);
        case '^':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_CARET_ASSIGN, start, 2);
            return lexer_make_token(lexer, TOK_CARET, start, 1);

        case '"': return lexer_read_string(lexer, '"', TOK_STRING);
        case '\'': return lexer_read_string(lexer, '\'', TOK_CHARACTER);

        case '+':
            if (lexer_match(lexer, '+')) return lexer_make_token(lexer, TOK_INCREMENT, start, 2);
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_PLUS_ASSIGN, start, 2);
            return lexer_make_token(lexer, TOK_PLUS, start, 1);

        case '-':
            if (lexer_match(lexer, '-')) return lexer_make_token(lexer, TOK_DECREMENT, start, 2);
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_MINUS_ASSIGN, start, 2);
            if (lexer_match(lexer, '>')) return lexer_make_token(lexer, TOK_ARROW, start, 2);
            return lexer_make_token(lexer, TOK_MINUS, start, 1);

        case '*':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_STAR_ASSIGN, start, 2);
            return lexer_make_token(lexer, TOK_STAR, start, 1);

        case '/':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_SLASH_ASSIGN, start, 2);
            return lexer_make_token(lexer, TOK_SLASH, start, 1);

        case '%':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_PERCENT_ASSIGN, start, 2);
            return lexer_make_token(lexer, TOK_PERCENT, start, 1);

        case '&':
            if (lexer_match(lexer, '&')) return lexer_make_token(lexer, TOK_AMPERSAND_AMPERSAND, start, 2);
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_AMPERSAND_ASSIGN, start, 2);
            return lexer_make_token(lexer, TOK_AMPERSAND, start, 1);

        case '|':
            if (lexer_match(lexer, '|')) return lexer_make_token(lexer, TOK_PIPE_PIPE, start, 2);
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_PIPE_ASSIGN, start, 2);
            return lexer_make_token(lexer, TOK_PIPE, start, 1);

        case '!':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_NE, start, 2);
            return lexer_make_token(lexer, TOK_EXCLAIM, start, 1);

        case '=':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_EQ, start, 2);
            return lexer_make_token(lexer, TOK_ASSIGN, start, 1);

        case '<':
            if (lexer_match(lexer, '<')) {
                if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_LSHIFT_ASSIGN, start, 3);
                return lexer_make_token(lexer, TOK_LSHIFT, start, 2);
            }
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_LE, start, 2);
            return lexer_make_token(lexer, TOK_LT, start, 1);

        case '>':
            if (lexer_match(lexer, '>')) {
                if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_RSHIFT_ASSIGN, start, 3);
                return lexer_make_token(lexer, TOK_RSHIFT, start, 2);
            }
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOK_GE, start, 2);
            return lexer_make_token(lexer, TOK_GT, start, 1);

        case '.':
            if (lexer_match(lexer, '.') && lexer_match(lexer, '.')) {
                return lexer_make_token(lexer, TOK_ELLIPSIS, start, 3);
            }
            return lexer_make_token(lexer, TOK_DOT, start, 1);

        default: {
            Token t;
            t.kind = TOK_ERROR;
            t.start = start;
            t.length = 1;
            t.loc = lexer_loc(lexer);
            if (lexer->diag) {
                lexer->diag->error(t.loc, "unexpected character '%c'", c);
            }
            return t;
        }
    }
}

Token lexer_peek_token(Lexer *lexer) {
    if (!lexer->has_peek) {
        lexer->peek = lexer_next_token(lexer);
        lexer->has_peek = true;
    }
    return lexer->peek;
}

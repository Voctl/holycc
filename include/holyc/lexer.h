#ifndef HOLYC_LEXER_H
#define HOLYC_LEXER_H

#include "holyc/token.h"
#include "holyc/diag.h"
#include <stddef.h>

typedef struct Lexer Lexer;

Lexer *lexer_create(const char *filename, const char *source, size_t source_len);
void lexer_destroy(Lexer *lexer);
Token lexer_next_token(Lexer *lexer);
Token lexer_peek_token(Lexer *lexer);
void lexer_set_diagnostics(Lexer *lexer, Diagnostics *diag);

#endif

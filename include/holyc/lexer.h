#ifndef HOLYC_LEXER_H
#define HOLYC_LEXER_H

#include "holyc/token.h"
#include "holyc/diag.h"
#include <stddef.h>

// Opaque lexer state
typedef struct Lexer Lexer;

// Create/destroy a lexer over a source buffer
Lexer *lexer_create(const char *filename, const char *source, size_t source_len);
void lexer_destroy(Lexer *lexer);

// Token stream – pull next token or peek without consuming
Token lexer_next_token(Lexer *lexer);
Token lexer_peek_token(Lexer *lexer);

// Attach a diagnostics collector for error reporting
void lexer_set_diagnostics(Lexer *lexer, Diagnostics *diag);

#endif

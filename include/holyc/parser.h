#ifndef HOLYC_PARSER_H
#define HOLYC_PARSER_H

#include "holyc/ast.h"
#include "holyc/lexer.h"
#include "holyc/diag.h"

// Opaque parser state
typedef struct Parser Parser;

// Create/destroy a recursive-descent parser
Parser *parser_create(Lexer *lexer, Diagnostics *diag);
void parser_set_sourcedir(Parser *parser, const char *dir);
void parser_destroy(Parser *parser);

// Parse a full translation unit (the entire source file)
AstNode *parser_parse_translation_unit(Parser *parser);

#endif

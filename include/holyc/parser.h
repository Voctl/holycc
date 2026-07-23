#ifndef HOLYC_PARSER_H
#define HOLYC_PARSER_H

#include "holyc/ast.h"
#include "holyc/lexer.h"
#include "holyc/diag.h"

typedef struct Parser Parser;

Parser *parser_create(Lexer *lexer, Diagnostics *diag);
void parser_destroy(Parser *parser);

AstNode *parser_parse_translation_unit(Parser *parser);

#endif

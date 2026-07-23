#include "holyc/parser.h"
#include "holyc/types.h"
#include "holyc/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

struct Parser {
    Lexer *lexer;
    Diagnostics *diag;
    Token current;
    Token peek;
    char *sourcedir;
};

static void parser_advance(Parser *p) {
    p->current = p->peek;
    p->peek = lexer_next_token(p->lexer);
}

Parser *parser_create(Lexer *lexer, Diagnostics *diag) {
    Parser *p = calloc(1, sizeof(Parser));
    p->lexer = lexer;
    p->diag = diag;
    p->sourcedir = NULL;
    parser_advance(p);
    parser_advance(p);
    return p;
}

void parser_set_sourcedir(Parser *parser, const char *dir) {
    free(parser->sourcedir);
    parser->sourcedir = dir ? strdup(dir) : NULL;
}

void parser_destroy(Parser *parser) {
    free(parser->sourcedir);
    free(parser);
}

static bool parser_check(Parser *p, TokenKind kind) {
    return p->current.kind == kind;
}

static bool parser_match(Parser *p, TokenKind kind) {
    if (parser_check(p, kind)) {
        parser_advance(p);
        return true;
    }
    return false;
}

static void parser_expect(Parser *p, TokenKind kind, const char *msg) {
    if (parser_check(p, kind)) {
        parser_advance(p);
        return;
    }
    (void)msg;
    p->diag->error(p->current.loc, "expected %s, got %s",
                   token_kind_spelling(kind),
                   token_kind_name(p->current.kind));
}

static AstNode *parser_make_node(Parser *p, AstKind kind) {
    return ast_node_create(kind, p->current.loc);
}

static bool parser_is_type_keyword(TokenKind kind) {
    switch (kind) {
        case TOK_KW_I8:   case TOK_KW_I16:  case TOK_KW_I32:  case TOK_KW_I64:
        case TOK_KW_U8:   case TOK_KW_U16:  case TOK_KW_U32:  case TOK_KW_U64:
        case TOK_KW_F64:  case TOK_KW_BOOL: case TOK_KW_CHAR: case TOK_KW_VOID:
            return true;
        default:
            return false;
    }
}

static AstNode *parser_parse_expr(Parser *p);
static AstNode *parser_parse_stmt(Parser *p);
static AstNode *parser_parse_block(Parser *p);
static AstNode *parser_parse_decl(Parser *p);

static AstNode *parser_parse_primary(Parser *p) {
    switch (p->current.kind) {
        case TOK_IDENTIFIER: {
            AstNode *node = parser_make_node(p, AST_IDENTIFIER);
            node->data.string_value = strndup(p->current.start, p->current.length);
            parser_advance(p);
            return node;
        }
        case TOK_INTEGER: {
            AstNode *node = parser_make_node(p, AST_INTEGER_LITERAL);
            char *str = strndup(p->current.start, p->current.length);
            node->data.int_value = (uint64_t)strtoull(str, NULL, 0);
            free(str);
            parser_advance(p);
            return node;
        }
        case TOK_FLOAT: {
            AstNode *node = parser_make_node(p, AST_FLOAT_LITERAL);
            char *str = strndup(p->current.start, p->current.length);
            node->data.float_value = strtod(str, NULL);
            free(str);
            parser_advance(p);
            return node;
        }
        case TOK_STRING: {
            AstNode *node = parser_make_node(p, AST_STRING_LITERAL);
            node->data.string_value = strndup(p->current.start + 1, p->current.length - 2);
            parser_advance(p);
            return node;
        }
        case TOK_CHARACTER: {
            AstNode *node = parser_make_node(p, AST_CHAR_LITERAL);
            size_t len = p->current.length;
            const char *start = p->current.start;
            if (len >= 3 && start[1] == '\\') {
                node->data.int_value = (uint64_t)start[2];
            } else if (len >= 3) {
                node->data.int_value = (uint64_t)start[1];
            }
            parser_advance(p);
            return node;
        }
        case TOK_KW_TRUE: {
            AstNode *node = parser_make_node(p, AST_BOOL_LITERAL);
            node->data.int_value = 1;
            parser_advance(p);
            return node;
        }
        case TOK_KW_FALSE: {
            AstNode *node = parser_make_node(p, AST_BOOL_LITERAL);
            node->data.int_value = 0;
            parser_advance(p);
            return node;
        }
        case TOK_KW_NULL: {
            AstNode *node = parser_make_node(p, AST_NULL_LITERAL);
            parser_advance(p);
            return node;
        }
        case TOK_LBRACE: {
            AstNode *init = parser_make_node(p, AST_ARRAY_INIT);
            parser_advance(p);
            if (!parser_check(p, TOK_RBRACE)) {
                do {
                    ast_add_child(init, parser_parse_expr(p));
                } while (parser_match(p, TOK_COMMA));
            }
            parser_expect(p, TOK_RBRACE, "}");
            return init;
        }
        case TOK_LPAREN: {
            parser_advance(p);
            AstNode *expr = parser_parse_expr(p);
            parser_expect(p, TOK_RPAREN, "closing parenthesis");
            return expr;
        }
        default: {
            p->diag->error(p->current.loc, "unexpected token '%s' in expression",
                           token_kind_name(p->current.kind));
            AstNode *node = parser_make_node(p, AST_NONE);
            parser_advance(p);
            return node;
        }
    }
}

static AstNode *parser_parse_postfix(Parser *p) {
    AstNode *expr = parser_parse_primary(p);

    for (;;) {
        switch (p->current.kind) {
            case TOK_LBRACKET: {
                parser_advance(p);
                AstNode *index = parser_parse_expr(p);
                parser_expect(p, TOK_RBRACKET, "closing bracket");
                AstNode *node = parser_make_node(p, AST_INDEX_EXPR);
                ast_add_child(node, expr);
                ast_add_child(node, index);
                expr = node;
                break;
            }
            case TOK_LPAREN: {
                parser_advance(p);
                AstNode *call = parser_make_node(p, AST_CALL_EXPR);
                ast_add_child(call, expr);
                if (!parser_check(p, TOK_RPAREN)) {
                    do {
                        ast_add_child(call, parser_parse_expr(p));
                    } while (parser_match(p, TOK_COMMA));
                }
                parser_expect(p, TOK_RPAREN, "closing parenthesis");
                expr = call;
                break;
            }
            case TOK_DOT: {
                parser_advance(p);
                AstNode *member = parser_make_node(p, AST_MEMBER_EXPR);
                ast_add_child(member, expr);
                Token id = p->current;
                parser_expect(p, TOK_IDENTIFIER, "member name");
                AstNode *name = ast_node_create(AST_IDENTIFIER, id.loc);
                name->data.string_value = strndup(id.start, id.length);
                ast_add_child(member, name);
                expr = member;
                break;
            }
            case TOK_ARROW: {
                parser_advance(p);
                AstNode *member = parser_make_node(p, AST_POINTER_MEMBER_EXPR);
                ast_add_child(member, expr);
                Token id = p->current;
                parser_expect(p, TOK_IDENTIFIER, "member name");
                AstNode *name = ast_node_create(AST_IDENTIFIER, id.loc);
                name->data.string_value = strndup(id.start, id.length);
                ast_add_child(member, name);
                expr = member;
                break;
            }
            case TOK_INCREMENT: {
                parser_advance(p);
                AstNode *node = parser_make_node(p, AST_UNARY_EXPR);
                node->data.token_kind = TOK_INCREMENT;
                ast_add_child(node, expr);
                expr = node;
                break;
            }
            case TOK_DECREMENT: {
                parser_advance(p);
                AstNode *node = parser_make_node(p, AST_UNARY_EXPR);
                node->data.token_kind = TOK_DECREMENT;
                ast_add_child(node, expr);
                expr = node;
                break;
            }
            default:
                return expr;
        }
    }
}

static AstNode *parser_parse_prefix(Parser *p) {
    switch (p->current.kind) {
        case TOK_MINUS:
        case TOK_EXCLAIM:
        case TOK_TILDE:
        case TOK_STAR:
        case TOK_AMPERSAND:
        case TOK_INCREMENT:
        case TOK_DECREMENT: {
            TokenKind op = p->current.kind;
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_UNARY_EXPR);
            node->data.token_kind = op;
            ast_add_child(node, parser_parse_prefix(p));
            return node;
        }
        case TOK_KW_SIZEOF: {
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_SIZEOF_EXPR);
            if (parser_match(p, TOK_LPAREN) && parser_is_type_keyword(p->current.kind)) {
                AstNode *type_node = parser_make_node(p, AST_NAMED_TYPE);
                type_node->data.string_value = strndup(p->current.start, p->current.length);
                parser_advance(p);
                ast_add_child(node, type_node);
                parser_expect(p, TOK_RPAREN, ")");
            } else if (parser_check(p, TOK_LPAREN)) {
                parser_advance(p);
                ast_add_child(node, parser_parse_expr(p));
                parser_expect(p, TOK_RPAREN, ")");
            } else {
                ast_add_child(node, parser_parse_prefix(p));
            }
            return node;
        }
        case TOK_LPAREN: {
            if (parser_is_type_keyword(p->peek.kind) || p->peek.kind == TOK_KW_CLASS ||
                p->peek.kind == TOK_KW_UNION) {
                parser_advance(p);
                AstNode *cast = parser_make_node(p, AST_CAST_EXPR);
                AstNode *type_node = parser_make_node(p, AST_NAMED_TYPE);
                type_node->data.string_value = strndup(p->current.start, p->current.length);
                parser_advance(p);
                while (parser_match(p, TOK_STAR)) {
                    AstNode *ptr = parser_make_node(p, AST_POINTER_TYPE);
                    ast_add_child(ptr, type_node);
                    type_node = ptr;
                }
                ast_add_child(cast, type_node);
                parser_expect(p, TOK_RPAREN, ")");
                ast_add_child(cast, parser_parse_prefix(p));
                return cast;
            }
            break;
        }
        default:
            break;
    }
    return parser_parse_postfix(p);
}

static int parser_precedence(TokenKind kind) {
    switch (kind) {
        case TOK_STAR:        case TOK_SLASH:      case TOK_PERCENT:
            return 13;
        case TOK_PLUS:        case TOK_MINUS:
            return 12;
        case TOK_LSHIFT:      case TOK_RSHIFT:
            return 11;
        case TOK_LT:          case TOK_GT:
        case TOK_LE:          case TOK_GE:
            return 10;
        case TOK_EQ:          case TOK_NE:
            return 9;
        case TOK_AMPERSAND:
            return 8;
        case TOK_CARET:
            return 7;
        case TOK_PIPE:
            return 6;
        case TOK_AMPERSAND_AMPERSAND:
            return 5;
        case TOK_PIPE_PIPE:
            return 4;
        case TOK_QUESTION:
            return 3;
        case TOK_ASSIGN:      case TOK_PLUS_ASSIGN:   case TOK_MINUS_ASSIGN:
        case TOK_STAR_ASSIGN: case TOK_SLASH_ASSIGN:  case TOK_PERCENT_ASSIGN:
        case TOK_AMPERSAND_ASSIGN: case TOK_PIPE_ASSIGN: case TOK_CARET_ASSIGN:
        case TOK_LSHIFT_ASSIGN:   case TOK_RSHIFT_ASSIGN:
            return 2;
        default:
            return -1;
    }
}

static bool parser_is_assign_op(TokenKind kind) {
    switch (kind) {
        case TOK_ASSIGN:          case TOK_PLUS_ASSIGN:   case TOK_MINUS_ASSIGN:
        case TOK_STAR_ASSIGN:     case TOK_SLASH_ASSIGN:  case TOK_PERCENT_ASSIGN:
        case TOK_AMPERSAND_ASSIGN: case TOK_PIPE_ASSIGN:  case TOK_CARET_ASSIGN:
        case TOK_LSHIFT_ASSIGN:   case TOK_RSHIFT_ASSIGN:
            return true;
        default:
            return false;
    }
}

static AstNode *parser_parse_binary(Parser *p, int min_prec) {
    AstNode *left = parser_parse_prefix(p);

    for (;;) {
        TokenKind op = p->current.kind;
        int prec = parser_precedence(op);
        if (prec < min_prec) break;

        if (op == TOK_QUESTION) {
            parser_advance(p);
            AstNode *cond = parser_make_node(p, AST_CONDITIONAL_EXPR);
            ast_add_child(cond, left);
            ast_add_child(cond, parser_parse_expr(p));
            parser_expect(p, TOK_COLON, ":");
            ast_add_child(cond, parser_parse_binary(p, prec));
            left = cond;
            continue;
        }

        parser_advance(p);
        AstNode *right = parser_parse_binary(p, prec + 1);

        if (parser_is_assign_op(op)) {
            AstNode *assign = parser_make_node(p, AST_BINARY_EXPR);
            assign->data.token_kind = op;
            ast_add_child(assign, left);
            ast_add_child(assign, right);
            left = assign;
        } else if (op == TOK_AMPERSAND_AMPERSAND || op == TOK_PIPE_PIPE) {
            AstNode *node = parser_make_node(p, AST_BINARY_EXPR);
            node->data.token_kind = op;
            ast_add_child(node, left);
            ast_add_child(node, right);
            left = node;
        } else {
            AstNode *node = parser_make_node(p, AST_BINARY_EXPR);
            node->data.token_kind = op;
            ast_add_child(node, left);
            ast_add_child(node, right);
            left = node;
        }
    }

    return left;
}

static AstNode *parser_parse_expr(Parser *p) {
    return parser_parse_binary(p, 0);
}

static AstNode *parser_parse_stmt(Parser *p) {
    switch (p->current.kind) {
        case TOK_LBRACE:
            return parser_parse_block(p);

        case TOK_KW_IF: {
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_IF_STMT);
            parser_expect(p, TOK_LPAREN, "(");
            ast_add_child(node, parser_parse_expr(p));
            parser_expect(p, TOK_RPAREN, ")");
            ast_add_child(node, parser_parse_stmt(p));
            if (parser_match(p, TOK_KW_ELSE)) {
                ast_add_child(node, parser_parse_stmt(p));
            }
            return node;
        }

        case TOK_KW_WHILE: {
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_WHILE_STMT);
            parser_expect(p, TOK_LPAREN, "(");
            ast_add_child(node, parser_parse_expr(p));
            parser_expect(p, TOK_RPAREN, ")");
            ast_add_child(node, parser_parse_stmt(p));
            return node;
        }

        case TOK_KW_DO: {
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_DO_WHILE_STMT);
            ast_add_child(node, parser_parse_stmt(p));
            parser_expect(p, TOK_KW_WHILE, "while");
            parser_expect(p, TOK_LPAREN, "(");
            ast_add_child(node, parser_parse_expr(p));
            parser_expect(p, TOK_RPAREN, ")");
            parser_expect(p, TOK_SEMICOLON, ";");
            return node;
        }

        case TOK_KW_FOR: {
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_FOR_STMT);
            parser_expect(p, TOK_LPAREN, "(");
            if (parser_check(p, TOK_SEMICOLON)) {
                ast_add_child(node, parser_make_node(p, AST_NONE));
            } else if (parser_is_type_keyword(p->current.kind)) {
                ast_add_child(node, parser_parse_decl(p));
            } else {
                ast_add_child(node, parser_parse_expr(p));
            }
            parser_expect(p, TOK_SEMICOLON, ";");
            if (parser_check(p, TOK_SEMICOLON)) {
                ast_add_child(node, parser_make_node(p, AST_NONE));
            } else {
                ast_add_child(node, parser_parse_expr(p));
            }
            parser_expect(p, TOK_SEMICOLON, ";");
            if (parser_check(p, TOK_RPAREN)) {
                ast_add_child(node, parser_make_node(p, AST_NONE));
            } else {
                ast_add_child(node, parser_parse_expr(p));
            }
            parser_expect(p, TOK_RPAREN, ")");
            ast_add_child(node, parser_parse_stmt(p));
            return node;
        }

        case TOK_KW_SWITCH: {
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_SWITCH_STMT);
            parser_expect(p, TOK_LPAREN, "(");
            ast_add_child(node, parser_parse_expr(p));
            parser_expect(p, TOK_RPAREN, ")");
            parser_expect(p, TOK_LBRACE, "{");
            while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
                if (parser_match(p, TOK_KW_CASE)) {
                    AstNode *case_node = parser_make_node(p, AST_CASE_STMT);
                    ast_add_child(case_node, parser_parse_expr(p));
                    parser_expect(p, TOK_COLON, ":");
                    while (!parser_check(p, TOK_KW_CASE) &&
                           !parser_check(p, TOK_KW_DEFAULT) &&
                           !parser_check(p, TOK_RBRACE) &&
                           !parser_check(p, TOK_EOF)) {
                        ast_add_child(case_node, parser_parse_stmt(p));
                    }
                    ast_add_child(node, case_node);
                } else if (parser_match(p, TOK_KW_DEFAULT)) {
                    AstNode *def_node = parser_make_node(p, AST_DEFAULT_STMT);
                    parser_expect(p, TOK_COLON, ":");
                    while (!parser_check(p, TOK_KW_CASE) &&
                           !parser_check(p, TOK_KW_DEFAULT) &&
                           !parser_check(p, TOK_RBRACE) &&
                           !parser_check(p, TOK_EOF)) {
                        ast_add_child(def_node, parser_parse_stmt(p));
                    }
                    ast_add_child(node, def_node);
                } else {
                    ast_add_child(node, parser_parse_stmt(p));
                }
            }
            parser_expect(p, TOK_RBRACE, "}");
            return node;
        }

        case TOK_KW_RETURN: {
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_RETURN_STMT);
            if (!parser_check(p, TOK_SEMICOLON)) {
                ast_add_child(node, parser_parse_expr(p));
            }
            parser_expect(p, TOK_SEMICOLON, ";");
            return node;
        }

        case TOK_KW_BREAK: {
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_BREAK_STMT);
            parser_expect(p, TOK_SEMICOLON, ";");
            return node;
        }

        case TOK_KW_CONTINUE: {
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_CONTINUE_STMT);
            parser_expect(p, TOK_SEMICOLON, ";");
            return node;
        }

        case TOK_KW_GOTO: {
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_GOTO_STMT);
            Token id = p->current;
            parser_expect(p, TOK_IDENTIFIER, "label identifier");
            AstNode *label_id = ast_node_create(AST_IDENTIFIER, id.loc);
            label_id->data.string_value = strndup(id.start, id.length);
            ast_add_child(node, label_id);
            parser_expect(p, TOK_SEMICOLON, ";");
            return node;
        }

        case TOK_IDENTIFIER:
            if (p->peek.kind == TOK_COLON) {
                Token id = p->current;
                parser_advance(p);
                parser_advance(p);
                AstNode *node = parser_make_node(p, AST_LABEL_STMT);
                AstNode *label = ast_node_create(AST_IDENTIFIER, id.loc);
                label->data.string_value = strndup(id.start, id.length);
                ast_add_child(node, label);
                return node;
            }
            goto default_case;

        case TOK_KW_ASM:
        case TOK_KW__ASM: {
            parser_advance(p);
            AstNode *node = parser_make_node(p, AST_ASM_STMT);
            if (parser_match(p, TOK_LPAREN)) {
                if (p->current.kind == TOK_STRING) {
                    AstNode *str = parser_make_node(p, AST_STRING_LITERAL);
                    str->data.string_value = strndup(p->current.start + 1,
                                                     p->current.length - 2);
                    ast_add_child(node, str);
                    parser_advance(p);
                }
                parser_expect(p, TOK_RPAREN, ")");
            }
            parser_expect(p, TOK_SEMICOLON, ";");
            return node;
        }

        default:
        default_case: {
            if (parser_is_type_keyword(p->current.kind)) {
                return parser_parse_decl(p);
            }
            AstNode *node = parser_make_node(p, AST_EXPR_STMT);
            ast_add_child(node, parser_parse_expr(p));
            parser_expect(p, TOK_SEMICOLON, ";");
            return node;
        }
    }
}

static AstNode *parser_parse_block(Parser *p) {
    parser_expect(p, TOK_LBRACE, "{");
    AstNode *block = parser_make_node(p, AST_BLOCK);

    while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
        ast_add_child(block, parser_parse_stmt(p));
    }

    parser_expect(p, TOK_RBRACE, "}");
    return block;
}

static AstNode *parser_parse_type(Parser *p) {
    AstNode *type_node = parser_make_node(p, AST_NAMED_TYPE);
    type_node->data.string_value = strndup(p->current.start, p->current.length);

    if (parser_is_type_keyword(p->current.kind) ||
        p->current.kind == TOK_KW_CLASS ||
        p->current.kind == TOK_KW_UNION ||
        p->current.kind == TOK_IDENTIFIER) {
        type_node->data.string_value = strndup(p->current.start, p->current.length);
        parser_advance(p);
    } else if (p->current.kind == TOK_KW_ENUM) {
        parser_advance(p);
    } else {
        free((void *)type_node->data.string_value);
        type_node->data.string_value = strdup("void");
    }

    while (parser_match(p, TOK_STAR)) {
        AstNode *ptr = parser_make_node(p, AST_POINTER_TYPE);
        ast_add_child(ptr, type_node);
        type_node = ptr;
    }

    return type_node;
}

static AstNode *parser_parse_decl(Parser *p) {
    bool is_static = parser_match(p, TOK_KW_STATIC);
    bool is_extern = parser_match(p, TOK_KW_EXTERN);
    if (!is_extern) is_extern = parser_match(p, TOK_KW__EXTERN);

    AstNode *type_node = parser_parse_type(p);

    if (parser_check(p, TOK_LPAREN) && p->peek.kind == TOK_STAR) {
        AstNode *fp_type = parser_make_node(p, AST_FUNC_POINTER_TYPE);
        ast_add_child(fp_type, type_node);
        parser_advance(p);
        parser_advance(p);
        Token fp_name = p->current;
        parser_expect(p, TOK_IDENTIFIER, "function pointer name");
        AstNode *fp_name_node = ast_node_create(AST_IDENTIFIER, fp_name.loc);
        fp_name_node->data.string_value = strndup(fp_name.start, fp_name.length);
        ast_add_child(fp_type, fp_name_node);
        parser_expect(p, TOK_RPAREN, ")");
        parser_expect(p, TOK_LPAREN, "(");

        if (!parser_check(p, TOK_RPAREN)) {
            do {
                AstNode *ptype = parser_parse_type(p);
                AstNode *param = parser_make_node(p, AST_FUNC_PARAM);
                ast_add_child(param, ptype);
                if (parser_check(p, TOK_IDENTIFIER)) {
                    AstNode *pname = ast_node_create(AST_IDENTIFIER, p->current.loc);
                    pname->data.string_value = strndup(p->current.start, p->current.length);
                    ast_add_child(param, pname);
                    parser_advance(p);
                }
                ast_add_child(fp_type, param);
            } while (parser_match(p, TOK_COMMA));
        }
        parser_expect(p, TOK_RPAREN, ")");

        AstNode *var = parser_make_node(p, AST_VAR_DECL);
        if (is_static) var->flags |= AST_FLAG_STATIC;
        if (is_extern) var->flags |= AST_FLAG_EXTERN;
        ast_add_child(var, fp_type);
        ast_add_child(var, fp_name_node);

        if (parser_match(p, TOK_ASSIGN)) {
            ast_add_child(var, parser_parse_expr(p));
        }
        parser_expect(p, TOK_SEMICOLON, ";");
        return var;
    }

    Token name_tok = p->current;
    parser_expect(p, TOK_IDENTIFIER, "identifier");
    AstNode *name = ast_node_create(AST_IDENTIFIER, name_tok.loc);
    name->data.string_value = strndup(name_tok.start, name_tok.length);

    while (parser_match(p, TOK_LBRACKET)) {
        AstNode *arr_type = parser_make_node(p, AST_ARRAY_TYPE);
        ast_add_child(arr_type, type_node);
        if (!parser_check(p, TOK_RBRACKET)) {
            ast_add_child(arr_type, parser_parse_expr(p));
        }
        parser_expect(p, TOK_RBRACKET, "]");
        type_node = arr_type;
    }

    if (parser_check(p, TOK_LPAREN)) {
        AstNode *func = parser_make_node(p, AST_FUNC_DECL);
        if (is_static) func->flags |= AST_FLAG_STATIC;
        if (is_extern) func->flags |= AST_FLAG_EXTERN;
        ast_add_child(func, type_node);
        ast_add_child(func, name);
        parser_advance(p);

        if (!parser_check(p, TOK_RPAREN)) {
            do {
                AstNode *param_type = parser_parse_type(p);
                AstNode *param = parser_make_node(p, AST_FUNC_PARAM);
                ast_add_child(param, param_type);
                if (parser_check(p, TOK_IDENTIFIER)) {
                    AstNode *pname = ast_node_create(AST_IDENTIFIER, p->current.loc);
                    pname->data.string_value = strndup(p->current.start, p->current.length);
                    ast_add_child(param, pname);
                    parser_advance(p);
                }
                ast_add_child(func, param);
            } while (parser_match(p, TOK_COMMA));
        }
        parser_expect(p, TOK_RPAREN, ")");

        if (parser_check(p, TOK_LBRACE)) {
            ast_add_child(func, parser_parse_block(p));
        } else {
            parser_expect(p, TOK_SEMICOLON, ";");
        }
        return func;
    }

    AstNode *var = parser_make_node(p, AST_VAR_DECL);
    if (is_static) var->flags |= AST_FLAG_STATIC;
    if (is_extern) var->flags |= AST_FLAG_EXTERN;
    ast_add_child(var, type_node);
    ast_add_child(var, name);

    if (parser_match(p, TOK_ASSIGN)) {
        ast_add_child(var, parser_parse_expr(p));
    }

    parser_expect(p, TOK_SEMICOLON, ";");
    return var;
}

static AstNode *parser_parse_top_level(Parser *p) {
    switch (p->current.kind) {
        case TOK_KW_CLASS:
        case TOK_KW_UNION: {
            bool is_union = (p->current.kind == TOK_KW_UNION);
            AstKind kind = is_union ? AST_UNION_DECL : AST_STRUCT_DECL;
            parser_advance(p);
            AstNode *struct_node = parser_make_node(p, kind);
            if (parser_check(p, TOK_IDENTIFIER)) {
                AstNode *name = parser_make_node(p, AST_IDENTIFIER);
                name->data.string_value = strndup(p->current.start, p->current.length);
                ast_add_child(struct_node, name);
                parser_advance(p);
            }
            parser_expect(p, TOK_LBRACE, "{");
            while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
                AstNode *field_type = parser_parse_type(p);
                Token field_name = p->current;
                parser_expect(p, TOK_IDENTIFIER, "field name");
                AstNode *field = parser_make_node(p, AST_STRUCT_FIELD);
                ast_add_child(field, field_type);
                AstNode *fname = ast_node_create(AST_IDENTIFIER, field_name.loc);
                fname->data.string_value = strndup(field_name.start, field_name.length);
                ast_add_child(field, fname);
                parser_expect(p, TOK_SEMICOLON, ";");
                ast_add_child(struct_node, field);
            }
            parser_expect(p, TOK_RBRACE, "}");
            parser_expect(p, TOK_SEMICOLON, ";");
            return struct_node;
        }

        case TOK_KW_ENUM: {
            parser_advance(p);
            AstNode *enum_node = parser_make_node(p, AST_ENUM_DECL);
            if (parser_check(p, TOK_IDENTIFIER)) {
                AstNode *name = parser_make_node(p, AST_IDENTIFIER);
                name->data.string_value = strndup(p->current.start, p->current.length);
                ast_add_child(enum_node, name);
                parser_advance(p);
            }
            parser_expect(p, TOK_LBRACE, "{");
                int64_t value = 0;
            while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
                AstNode *enumerator = parser_make_node(p, AST_ENUMERATOR);
                Token id = p->current;
                parser_expect(p, TOK_IDENTIFIER, "enumerator name");
                AstNode *ename = ast_node_create(AST_IDENTIFIER, id.loc);
                ename->data.string_value = strndup(id.start, id.length);
                ast_add_child(enumerator, ename);
                if (parser_match(p, TOK_ASSIGN)) {
                    AstNode *val = parser_parse_expr(p);
                    ast_add_child(enumerator, val);
                    if (val->kind == AST_INTEGER_LITERAL) {
                        value = (int64_t)val->data.int_value + 1;
                    } else {
                        value = -1;
                    }
                } else {
                    AstNode *val = parser_make_node(p, AST_INTEGER_LITERAL);
                    val->data.int_value = (uint64_t)value;
                    ast_add_child(enumerator, val);
                    if (value >= 0) value++;
                }
                if (parser_check(p, TOK_COMMA)) {
                    parser_advance(p);
                }
                ast_add_child(enum_node, enumerator);
            }
            parser_expect(p, TOK_RBRACE, "}");
            parser_expect(p, TOK_SEMICOLON, ";");
            return enum_node;
        }

        case TOK_KW_INCLUDE: {
            parser_advance(p);
            if (parser_check(p, TOK_STRING)) {
                const char *filepath = strndup(p->current.start + 1, p->current.length - 2);
                parser_advance(p);

                AstNode *inc = parser_make_node(p, AST_INCLUDE);
                AstNode *str_node = parser_make_node(p, AST_STRING_LITERAL);
                str_node->data.string_value = filepath;
                ast_add_child(inc, str_node);

                char fullpath[1024];
                if (p->sourcedir && filepath[0] != '/') {
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", p->sourcedir, filepath);
                } else {
                    snprintf(fullpath, sizeof(fullpath), "%s", filepath);
                }

                size_t len;
                char *src = read_file(fullpath, &len);
                if (src) {
                    Lexer *inc_lexer = lexer_create(fullpath, src, len);
                    Parser *inc_parser = parser_create(inc_lexer, p->diag);
                    parser_set_sourcedir(inc_parser, p->sourcedir);
                    AstNode *inc_ast = parser_parse_translation_unit(inc_parser);
                    AstNode *child = inc_ast->first_child;
                    while (child) {
                        AstNode *next = child->next;
                        child->next = NULL;
                        child->parent = NULL;
                        ast_add_child(inc, child);
                        child = next;
                    }
                    inc_ast->first_child = NULL;
                    inc_ast->last_child = NULL;
                    ast_node_destroy_tree(inc_ast);
                    parser_destroy(inc_parser);
                    lexer_destroy(inc_lexer);
                    free(src);
                } else {
                    p->diag->error(p->current.loc, "cannot open include file '%s'", filepath);
                }
                return inc;
            }
            return parser_make_node(p, AST_NONE);
        }

        case TOK_KW_DEFINE: {
            parser_advance(p);
            AstNode *def = parser_make_node(p, AST_DEFINE);
            if (parser_check(p, TOK_IDENTIFIER)) {
                AstNode *name = parser_make_node(p, AST_IDENTIFIER);
                name->data.string_value = strndup(p->current.start, p->current.length);
                ast_add_child(def, name);
                parser_advance(p);
            }
            if (!parser_check(p, TOK_EOF) && !parser_check(p, TOK_SEMICOLON) &&
                p->current.start[0] != '\n') {
                ast_add_child(def, parser_parse_expr(p));
            }
            return def;
        }

        default:
            if (parser_is_type_keyword(p->current.kind) ||
                p->current.kind == TOK_KW_STATIC ||
                p->current.kind == TOK_KW_EXTERN ||
                p->current.kind == TOK_KW__EXTERN) {
                return parser_parse_decl(p);
            }
            return parser_parse_stmt(p);
    }
}

AstNode *parser_parse_translation_unit(Parser *p) {
    AstNode *tu = ast_node_create(AST_TRANSLATION_UNIT, p->current.loc);

    while (!parser_check(p, TOK_EOF)) {
        AstNode *node = parser_parse_top_level(p);
        if (node) {
            ast_add_child(tu, node);
        }
    }

    return tu;
}

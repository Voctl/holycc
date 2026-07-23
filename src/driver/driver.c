#include "holyc/driver.h"
#include "holyc/lexer.h"
#include "holyc/parser.h"
#include "holyc/ast.h"
#include "holyc/semantic.h"
#include "holyc/codegen.h"
#include "holyc/diag.h"
#include "holyc/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#define HOLYCC_VERSION "0.1.0"

static void print_usage(const char *prog) {
    printf("Usage: %s [options] <input.HC>\n", prog);
    printf("\nOptions:\n");
    printf("  -o <file>        Output file\n");
    printf("  -c               Compile only (no link)\n");
    printf("  -S               Assembly only (not implemented)\n");
    printf("  --emit-c         Emit C code (default, outputs .c file)\n");
    printf("  --tokens         Dump token stream\n");
    printf("  --ast            Dump AST\n");
    printf("  --help           Show this help\n");
    printf("  --version        Show version\n");
}

static void print_version(void) {
    printf("holycc version %s\n", HOLYCC_VERSION);
    printf("HolyC to C17 transpiler\n");
}

static void print_tokens(Lexer *lexer) {
    for (;;) {
        Token t = lexer_next_token(lexer);
        printf("%s:%u:%u: %s '%.*s'\n",
               t.loc.filename ? t.loc.filename : "-",
               t.loc.line, t.loc.column,
               token_kind_name(t.kind),
               (int)t.length, t.start ? t.start : "");
        if (t.kind == TOK_EOF) break;
        if (t.kind == TOK_ERROR) break;
    }
}

static void print_ast_node(AstNode *node, int depth, void *ctx) {
    (void)ctx;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("%s", ast_kind_name(node->kind));
    switch (node->kind) {
        case AST_IDENTIFIER:
        case AST_STRING_LITERAL:
        case AST_NAMED_TYPE:
            printf(" [%s]", node->data.string_value ? node->data.string_value : "");
            break;
        case AST_INTEGER_LITERAL:
            printf(" [%lld]", (long long)node->data.int_value);
            break;
        case AST_FLOAT_LITERAL:
            printf(" [%g]", node->data.float_value);
            break;
        case AST_BOOL_LITERAL:
            printf(" [%s]", node->data.int_value ? "true" : "false");
            break;
        case AST_BINARY_EXPR:
        case AST_UNARY_EXPR:
            printf(" [%s]", token_kind_spelling(node->data.token_kind));
            break;
        default:
            break;
    }
    printf("\n");
}

static void print_ast(AstNode *root) {
    if (!root) return;
    struct { AstNode *node; int depth; } stack[1024];
    int sp = 0;
    stack[sp].node = root;
    stack[sp].depth = 0;
    sp++;

    while (sp > 0) {
        sp--;
        AstNode *node = stack[sp].node;
        int depth = stack[sp].depth;

        print_ast_node(node, depth, NULL);

        AstNode *child = node->last_child;
        while (child) {
            if (sp < 1024) {
                stack[sp].node = child;
                stack[sp].depth = depth + 1;
                sp++;
            }
            child = child->next;
        }
    }
}

int driver_main(int argc, char **argv) {
    DriverOptions opts = {0};
    opts.input_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            opts.output_file = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0) {
            opts.compile_only = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            opts.assembly_only = true;
        } else if (strcmp(argv[i], "--emit-c") == 0) {
            opts.emit_c = true;
        } else if (strcmp(argv[i], "--tokens") == 0) {
            opts.dump_tokens = true;
        } else if (strcmp(argv[i], "--ast") == 0) {
            opts.dump_ast = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            opts.show_help = true;
        } else if (strcmp(argv[i], "--version") == 0) {
            opts.show_version = true;
        } else if (argv[i][0] != '-') {
            opts.input_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    if (opts.show_help) {
        print_usage(argv[0]);
        return 0;
    }

    if (opts.show_version) {
        print_version();
        return 0;
    }

    if (!opts.input_file) {
        fprintf(stderr, "Error: no input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    size_t source_len = 0;
    char *source = read_file(opts.input_file, &source_len);
    if (!source) {
        fprintf(stderr, "Error: cannot read file '%s'\n", opts.input_file);
        return 1;
    }

    Diagnostics diag = diagnostics_create();

    Lexer *lexer = lexer_create(opts.input_file, source, source_len);
    lexer_set_diagnostics(lexer, &diag);

    if (opts.dump_tokens) {
        print_tokens(lexer);
        diagnostics_destroy(&diag);
        lexer_destroy(lexer);
        free(source);
        return 0;
    }

    Parser *parser = parser_create(lexer, &diag);
    {
        char *path = strdup(opts.input_file);
        char *dir = dirname(path);
        parser_set_sourcedir(parser, dir);
        free(path);
    }
    AstNode *ast = parser_parse_translation_unit(parser);

    if (diag.had_error) {
        diagnostics_print(&diag, source);
        ast_node_destroy_tree(ast);
        diagnostics_destroy(&diag);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }

    if (opts.dump_ast) {
        print_ast(ast);
        ast_node_destroy_tree(ast);
        diagnostics_destroy(&diag);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(source);
        return 0;
    }

    Semantic *semantic = semantic_create(&diag);

    if (!semantic_analyze(semantic, ast)) {
        diagnostics_print(&diag, source);
        ast_node_destroy_tree(ast);
        semantic_destroy(semantic);
        diagnostics_destroy(&diag);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }

    SymbolTable *symtab = semantic_get_symbol_table(semantic);
    CodeGen *cg = codegen_create(symtab);

    const char *output_path = opts.output_file;
    char *auto_output = NULL;

    if (!output_path) {
        const char *dot = strrchr(opts.input_file, '.');
        size_t base_len = dot ? (size_t)(dot - opts.input_file) : strlen(opts.input_file);
        auto_output = malloc(base_len + 3);
        memcpy(auto_output, opts.input_file, base_len);
        auto_output[base_len] = '.';
        auto_output[base_len + 1] = 'c';
        auto_output[base_len + 2] = '\0';
        output_path = auto_output;
    }

    bool ok = codegen_generate_file(cg, ast, output_path);

    if (!ok) {
        fprintf(stderr, "Error: failed to write output to '%s'\n", output_path);
        free(auto_output);
        codegen_destroy(cg);
        ast_node_destroy_tree(ast);
        semantic_destroy(semantic);
        diagnostics_destroy(&diag);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }

    fprintf(stderr, "Generated: %s\n", output_path);
    free(auto_output);

    codegen_destroy(cg);
    ast_node_destroy_tree(ast);
    semantic_destroy(semantic);
    diagnostics_destroy(&diag);
    parser_destroy(parser);
    lexer_destroy(lexer);
    free(source);

    return 0;
}

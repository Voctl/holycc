#include "holyc/lexer.h"
#include "holyc/diag.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int test_count = 0;
static int pass_count = 0;

static void check(const char *name, bool condition) {
    test_count++;
    if (condition) {
        pass_count++;
        printf("  PASS: %s\n", name);
    } else {
        printf("  FAIL: %s\n", name);
    }
}

static void check_token_kind(const char *source, TokenKind expected_kinds[], int count) {
    Diagnostics diag = diagnostics_create();
    Lexer *lexer = lexer_create("-", source, strlen(source));
    lexer_set_diagnostics(lexer, &diag);

    for (int i = 0; i < count; i++) {
        Token t = lexer_next_token(lexer);
        char buf[256];
        snprintf(buf, sizeof(buf), "token[%d] should be %s", i, token_kind_name(expected_kinds[i]));
        check(buf, t.kind == expected_kinds[i]);
    }

    Token eof = lexer_next_token(lexer);
    check("final token should be EOF", eof.kind == TOK_EOF);

    lexer_destroy(lexer);
    diagnostics_destroy(&diag);
}

static void test_identifiers(void) {
    printf("\n--- Identifiers ---\n");
    TokenKind kinds[] = {TOK_IDENTIFIER, TOK_IDENTIFIER, TOK_IDENTIFIER};
    check_token_kind("foo bar baz", kinds, 3);
    check_token_kind("_hello _world123", (TokenKind[]){TOK_IDENTIFIER, TOK_IDENTIFIER}, 2);
    check_token_kind("x y1 z_2", (TokenKind[]){TOK_IDENTIFIER, TOK_IDENTIFIER, TOK_IDENTIFIER}, 3);
}

static void test_keywords(void) {
    printf("\n--- Keywords ---\n");
    check_token_kind("I64", (TokenKind[]){TOK_KW_I64}, 1);
    check_token_kind("I32 I16 I8", (TokenKind[]){TOK_KW_I32, TOK_KW_I16, TOK_KW_I8}, 3);
    check_token_kind("U64 U32 U16 U8", (TokenKind[]){TOK_KW_U64, TOK_KW_U32, TOK_KW_U16, TOK_KW_U8}, 4);
    check_token_kind("F64 Bool Char void", (TokenKind[]){TOK_KW_F64, TOK_KW_BOOL, TOK_KW_CHAR, TOK_KW_VOID}, 4);
    check_token_kind("if else for while do", (TokenKind[]){TOK_KW_IF, TOK_KW_ELSE, TOK_KW_FOR, TOK_KW_WHILE, TOK_KW_DO}, 5);
    check_token_kind("return break continue goto", (TokenKind[]){TOK_KW_RETURN, TOK_KW_BREAK, TOK_KW_CONTINUE, TOK_KW_GOTO}, 4);
    check_token_kind("class union enum sizeof", (TokenKind[]){TOK_KW_CLASS, TOK_KW_UNION, TOK_KW_ENUM, TOK_KW_SIZEOF}, 4);
    check_token_kind("static extern _extern", (TokenKind[]){TOK_KW_STATIC, TOK_KW_EXTERN, TOK_KW__EXTERN}, 3);
    check_token_kind("NULL TRUE FALSE", (TokenKind[]){TOK_KW_NULL, TOK_KW_TRUE, TOK_KW_FALSE}, 3);
}

static void test_integers(void) {
    printf("\n--- Integer Literals ---\n");
    check_token_kind("42", (TokenKind[]){TOK_INTEGER}, 1);
    check_token_kind("0xFF 0xABCD", (TokenKind[]){TOK_INTEGER, TOK_INTEGER}, 2);
    check_token_kind("0b1010 0b1111", (TokenKind[]){TOK_INTEGER, TOK_INTEGER}, 2);
    check_token_kind("1000000", (TokenKind[]){TOK_INTEGER}, 1);
}

static void test_floats(void) {
    printf("\n--- Float Literals ---\n");
    check_token_kind("3.14", (TokenKind[]){TOK_FLOAT}, 1);
    check_token_kind("1.0 2.5 0.001", (TokenKind[]){TOK_FLOAT, TOK_FLOAT, TOK_FLOAT}, 3);
    check_token_kind("1e10 2.5e-3", (TokenKind[]){TOK_FLOAT, TOK_FLOAT}, 2);
}

static void test_strings(void) {
    printf("\n--- String Literals ---\n");
    check_token_kind("\"hello\"", (TokenKind[]){TOK_STRING}, 1);
    check_token_kind("\"\"", (TokenKind[]){TOK_STRING}, 1);
    check_token_kind("\"abc\" \"def\"", (TokenKind[]){TOK_STRING, TOK_STRING}, 2);
}

static void test_chars(void) {
    printf("\n--- Character Literals ---\n");
    check_token_kind("'a'", (TokenKind[]){TOK_CHARACTER}, 1);
    check_token_kind("'\\n'", (TokenKind[]){TOK_CHARACTER}, 1);
}

static void test_operators(void) {
    printf("\n--- Operators ---\n");
    check_token_kind("+ - * / %", (TokenKind[]){
        TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT}, 5);
    check_token_kind("&& || ! == !=", (TokenKind[]){
        TOK_AMPERSAND_AMPERSAND, TOK_PIPE_PIPE, TOK_EXCLAIM, TOK_EQ, TOK_NE}, 5);
    check_token_kind("< > <= >=", (TokenKind[]){
        TOK_LT, TOK_GT, TOK_LE, TOK_GE}, 4);
    check_token_kind("<< >> & | ^ ~", (TokenKind[]){
        TOK_LSHIFT, TOK_RSHIFT, TOK_AMPERSAND, TOK_PIPE, TOK_CARET, TOK_TILDE}, 6);
    check_token_kind("++ -- ->", (TokenKind[]){
        TOK_INCREMENT, TOK_DECREMENT, TOK_ARROW}, 3);
}

static void test_assignment_ops(void) {
    printf("\n--- Assignment Operators ---\n");
    check_token_kind("= += -= *= /= %=", (TokenKind[]){
        TOK_ASSIGN, TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN,
        TOK_STAR_ASSIGN, TOK_SLASH_ASSIGN, TOK_PERCENT_ASSIGN}, 6);
    check_token_kind("&= |= ^= <<= >>=", (TokenKind[]){
        TOK_AMPERSAND_ASSIGN, TOK_PIPE_ASSIGN, TOK_CARET_ASSIGN,
        TOK_LSHIFT_ASSIGN, TOK_RSHIFT_ASSIGN}, 5);
}

static void test_punctuation(void) {
    printf("\n--- Punctuation ---\n");
    check_token_kind("( ) { } [ ]", (TokenKind[]){
        TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
        TOK_LBRACKET, TOK_RBRACKET}, 6);
    check_token_kind(", . : ; ?", (TokenKind[]){
        TOK_COMMA, TOK_DOT, TOK_COLON, TOK_SEMICOLON, TOK_QUESTION}, 5);
}

static void test_comments(void) {
    printf("\n--- Comments ---\n");
    check_token_kind("// comment\n42", (TokenKind[]){TOK_INTEGER}, 1);
    check_token_kind("/* block comment */ 42", (TokenKind[]){TOK_INTEGER}, 1);
    check_token_kind("/* multi\nline\ncomment */ 42", (TokenKind[]){TOK_INTEGER}, 1);
    check_token_kind("x // end of line comment\n y", (TokenKind[]){TOK_IDENTIFIER, TOK_IDENTIFIER}, 2);
}

static void test_token_content(void) {
    printf("\n--- Token Content ---\n");
    Diagnostics diag = diagnostics_create();
    Lexer *lexer = lexer_create("-", "I64 foo = 42;", 13);
    lexer_set_diagnostics(lexer, &diag);

    Token t1 = lexer_next_token(lexer);
    check("I64 kind", t1.kind == TOK_KW_I64);
    check("I64 length", t1.length == 3);
    check("I64 content", strncmp(t1.start, "I64", 3) == 0);

    Token t2 = lexer_next_token(lexer);
    check("identifier kind", t2.kind == TOK_IDENTIFIER);
    check("identifier length", t2.length == 3);
    check("identifier content", strncmp(t2.start, "foo", 3) == 0);

    Token t3 = lexer_next_token(lexer);
    check("assign kind", t3.kind == TOK_ASSIGN);

    lexer_destroy(lexer);
    diagnostics_destroy(&diag);
}

static void test_source_location(void) {
    printf("\n--- Source Location ---\n");
    Diagnostics diag = diagnostics_create();
    const char *src = "x\ny\nz";
    Lexer *lexer = lexer_create("test.HC", src, strlen(src));
    lexer_set_diagnostics(lexer, &diag);

    Token t1 = lexer_next_token(lexer);
    check("x line", t1.loc.line == 1);
    check("x column", t1.loc.column == 1);

    Token t2 = lexer_next_token(lexer);
    check("y line", t2.loc.line == 2);
    check("y column", t2.loc.column == 1);

    Token t3 = lexer_next_token(lexer);
    check("z line", t3.loc.line == 3);
    check("z column", t3.loc.column == 1);

    lexer_destroy(lexer);
    diagnostics_destroy(&diag);
}

int main(void) {
    printf("=== Lexer Tests ===\n\n");

    printf("--- Token Name Functions ---\n");
    check("TOK_EOF name", strcmp(token_kind_name(TOK_EOF), "EOF") == 0);
    check("TOK_KW_I64 name", strcmp(token_kind_name(TOK_KW_I64), "I64") == 0);
    check("TOK_KW_IF spelling", strcmp(token_kind_spelling(TOK_KW_IF), "if") == 0);
    check("TOK_PLUS spelling", strcmp(token_kind_spelling(TOK_PLUS), "+") == 0);

    test_identifiers();
    test_keywords();
    test_integers();
    test_floats();
    test_strings();
    test_chars();
    test_operators();
    test_assignment_ops();
    test_punctuation();
    test_comments();
    test_token_content();
    test_source_location();

    printf("\n=== Results: %d/%d tests passed ===\n", pass_count, test_count);
    return (pass_count == test_count) ? 0 : 1;
}

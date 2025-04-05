#ifndef LEXER_H
#define LEXER_H

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum{
    LET_TOK,
    IF_TOK,
    PRINT_TOK,
    PRINTLN_TOK,
    FOR_TOK,//for loop

    ID_TOK,
    NUM_TOK,
    TRUE_TOK,
    FALSE_TOK,

    PLUS_TOK,
    MINUS_TOK,
    MULT_TOK,
    DIV_TOK,
    POW_TOK,
    EQ_TOK,//==
    BIGGER_THAN_TOK,
    SMALLER_THAN_TOK,
    ASSIGN_TOK,//=
    COLON_ASSIGN_TOK,//:=
    ARROW_TOK, //->

    SEMICOLON_TOK,
    COLON_TOK,
    LBRACE_TOK,
    RBRACE_TOK,
    LPAREN_TOK,
    RPAREN_TOK,

    EOF_TOK,
    ERR_TOK,
} TokenT;

typedef struct{
    TokenT type;
    union {
        double num;
        char* str;
    } val;
    int line;
} Token;

typedef struct {
    const char* source;
    size_t source_len;
    int curr;
    int line;
    int had_error;
} Lexer;

typedef struct{
    Token* tokens;
    size_t count;
    size_t capacity;
} TokenArr;

Lexer init_lexer(const char* source);
Token scan_tok(Lexer* l);
void free_tok(Token tok);
void print_tok(Token tok);
TokenArr* tokenize_all(Lexer* l);
void free_token_arr(TokenArr* arr);

#endif

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    TokenArr* tokens;
    size_t curr;
    int had_error;
    char error_msg[256];
} Parser;

ASTNode* parse(TokenArr* tokens);
ASTNode* parse_file(const char* source);

#endif

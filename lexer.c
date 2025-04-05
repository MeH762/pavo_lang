#include "lexer.h"
#include "ast.h"
#include <stdio.h>
#include <string.h>

static int is_at_end(Lexer* l){
    return l->curr>=l->source_len;
}

static char peek(Lexer* l){
    if (is_at_end(l)) return '\0';
    return l->source[l->curr];
}

static char peek_next(Lexer* l){
    if (l->curr+1 >= l->source_len) return '\0';
    return l->source[l->curr+1];
}

static char advance(Lexer* l){
    l->curr++;
    return l->source[l->curr-1];
}

static int match(Lexer* l, char exp){
    if (is_at_end(l)) return 0;
    if (l->source[l->curr]!=exp) return 0;

    l->curr++;
    return 1;
}

static Token make_token(Lexer* l, TokenT t){
    Token tok;
    tok.type=t;
    tok.line=l->line;
    return tok;
}

static Token make_str_tok(Lexer* l, TokenT t, const char* start, int len){
    Token tok = make_token(l, t);
    tok.val.str = (char*)malloc(len+1);

    if (!tok.val.str){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    memcpy(tok.val.str, start, len);
    tok.val.str[len]='\0';

    return tok;
}

static Token make_num_tok(Lexer* l, double x){
    Token tok = make_token(l, NUM_TOK);
    tok.val.num=x;
    return tok;
}

static Token error_tok(Lexer* l, const char* msg){
    Token tok = make_token(l, ERR_TOK);
    tok.val.str = strdup(msg);
    l->had_error=1;
    return tok;
}

static void skip_wspace(Lexer* l){
    while (1){
        char c = peek(l);
        switch (c){
            case ' ':
            case '\r':
            case '\t': advance(l); break;
            case '\n':l->line++;advance(l);break;
            case '/':{
                if (peek_next(l)=='/'){
                    while (peek(l)!='\n' && !is_at_end(l)){
                        advance(l);
                    }
                } else {
                    return;
                }
            } break;
            default: return;
        }
    }
}

static int is_alpha(char c){
    return (c>='a'&&c<='z') || (c>='A'&&c<='Z') || c=='_';
}

static int is_digit(char c){
    return c>='0'&&c<='9';
}

static int is_alphanumeric(char c){
    return is_alpha(c)||is_digit(c);
}

static Token word(Lexer* l){
    const char* start = l->source+l->curr-1;
    int len = 1;

    while (is_alphanumeric(peek(l))){
        advance(l);
        len++;
    }

    if (len==3 && strncmp(start, "let", 3)==0){
        return make_token(l, LET_TOK);
    }
    if (len==2 && strncmp(start, "if", 2)==0){
        return make_token(l, IF_TOK);
    }
    if (len==3 && strncmp(start, "for", 3)==0){
        return make_token(l, FOR_TOK);
    }
    if (len==5 && strncmp(start, "print", 5)==0){
        return make_token(l, PRINT_TOK);
    }
    if (len==7 && strncmp(start, "println", 7)==0){
        return make_token(l, PRINTLN_TOK);
    }
    if (len==4 && strncmp(start, "true", 4)==0){
        return make_token(l, TRUE_TOK);
    }
    if (len==5 && strncmp(start, "false", 5)==0){
        return make_token(l, FALSE_TOK);
    }

    return make_str_tok(l, ID_TOK, start, len);
}

static Token number(Lexer* l){
    const char* start = l->source + l->curr-1;
    int len=1;

    while (is_digit(peek(l))){
        advance(l);
        len++;
    }

    if (peek(l)=='.' && is_digit(peek_next(l))){
        advance(l);
        len++;

        while (is_digit(peek(l))){
            advance(l);
            len++;
        }
    }

    char buffer[256];
    if (len>=sizeof(char)*256){
        return error_tok(l, "number too large");
    }

    strncpy(buffer, start, len);
    buffer[len]='\0';

    double val=strtod(buffer, NULL);
    return make_num_tok(l, val);
}

Token scan_tok(Lexer* l){
    skip_wspace(l);

    if (is_at_end(l)){
        return make_token(l, EOF_TOK);
    }

    char c = advance(l);

    if (is_alpha(c)) return word(l);
    if (is_digit(c)) return number(l);

    TokenT t=ERR_TOK;

    switch (c){
        case '(': t=LPAREN_TOK; break;
        case ')': t=RPAREN_TOK; break;
        case '{': t=LBRACE_TOK; break;
        case '}': t=RBRACE_TOK; break;
        case ':': {
            if (match(l, '=')){
                t=COLON_ASSIGN_TOK;
            } else {
                t=COLON_TOK;
            }
        } break;
        case ';': t=SEMICOLON_TOK; break;
        case '+': t=PLUS_TOK; break;
        case '-': {
            if (match(l, '>')){
                t=ARROW_TOK;
            } else {
                t=MINUS_TOK;
            }
        } break;
        case '/': t=DIV_TOK; break;
        case '*': {
            if (match(l, '*')){
                t=POW_TOK;
            } else {
                t=MULT_TOK;
            }
        } break;
        case '=': {
            if (match(l, '=')){
                t=EQ_TOK;
            } else {
                t=ASSIGN_TOK;
            }
        } break;
        case '>': t=BIGGER_THAN_TOK; break;
        case '<': t=SMALLER_THAN_TOK; break;
    }

    if (t==ERR_TOK){
        return error_tok(l, "unexpected character");
    } else {
        return make_token(l, t);
    }
}

static const char* token_type_to_string(TokenT type) {
    switch (type) {
        case LET_TOK: return "LET";
        case IF_TOK: return "IF";
        case PRINT_TOK: return "PRINT";
        case PRINTLN_TOK: return "PRINTLN";
        case NUM_TOK: return "NUM";
        case ID_TOK: return "IDENTIFIER";
        case PLUS_TOK: return "PLUS";
        case MINUS_TOK: return "MINUS";
        case MULT_TOK: return "MULTIPLY";
        case DIV_TOK: return "DIVIDE";
        case POW_TOK: return "POWER";
        case EQ_TOK: return "EQUAL";
        case BIGGER_THAN_TOK: return "GREATER_THAN";
        case SMALLER_THAN_TOK: return "LESS_THAN";
        case ASSIGN_TOK: return "ASSIGN";
        case SEMICOLON_TOK: return "SEMICOLON";
        case COLON_TOK: return "COLON";
        case LBRACE_TOK: return "LEFT_BRACE";
        case RBRACE_TOK: return "RIGHT_BRACE";
        case LPAREN_TOK: return "LEFT_PAREN";
        case RPAREN_TOK: return "RIGHT_PAREN";
        case EOF_TOK: return "EOF";
        case ERR_TOK: return "ERROR";
        case COLON_ASSIGN_TOK: return "COLON_ASSIGN";
        default: return "UNKNOWN";
    }
}

void print_tok(Token tok) {
    printf("%-15s ", token_type_to_string(tok.type));

    // Print token-specific values
    switch (tok.type) {
        case ID_TOK:
            printf("'%s'", tok.val.str);
            break;
        case NUM_TOK:
            printf("%g", tok.val.num);
            break;
        case ERR_TOK:
            printf("Error: %s", tok.val.str);
            break;
        default:
            // No additional value to print for other token types
            break;
    }

    // Print line information
    printf(" at line %d\n", tok.line);
}

Lexer init_lexer(const char* source){
    Lexer lexer;
    lexer.source=source;
    lexer.source_len=strlen(source);
    lexer.curr=0;
    lexer.line=1;
    lexer.had_error=0;
    return lexer;
}

void free_tok(Token tok){
    if (tok.type==ID_TOK || tok.type==ERR_TOK){
        free(tok.val.str);
    }
}

static TokenArr* init_token_arr(size_t initial_capacity){
    TokenArr* arr = (TokenArr*)malloc(sizeof(TokenArr));
    if (!arr){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    arr->tokens = (Token*)malloc(sizeof(Token)*initial_capacity);
    if (!arr->tokens){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    arr->count = 0;
    arr->capacity=initial_capacity;

    return arr;
}

static void add_token(TokenArr* arr, Token tok){
    if (arr->count>=arr->capacity){
        arr->capacity*=2;
        arr->tokens=(Token*)realloc(arr->tokens, sizeof(Token)*arr->capacity);
        if (!arr){
            fprintf(stderr, "memory reallocation failed\n");
            exit(EXIT_FAILURE);
        }
    }

    arr->tokens[arr->count++] = tok;
}

TokenArr* tokenize_all(Lexer* l){
    TokenArr* token_arr = init_token_arr(100);

    while (1){
        Token tok = scan_tok(l);

        add_token(token_arr, tok);

        if (tok.type==EOF_TOK || tok.type==ERR_TOK) break;
    }

    return token_arr;
}

void free_token_arr(TokenArr *arr){
    if (!arr) return;

    for (size_t i=0; i<arr->count; i++){
        free_tok(arr->tokens[i]);
    }

    free(arr->tokens);

    free(arr);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "map.h"

static Parser* init_parser(TokenArr* tokens){
    Parser* p = (Parser*)malloc(sizeof(Parser));
    if (!p){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    p->tokens=tokens;
    p->curr=0;
    p->had_error=0;
    p->error_msg[0]='\0';

    return p;
}

static void parser_error(Parser* p, const char* msg){
    p->had_error=1;
    strncpy(p->error_msg, msg, sizeof(p->error_msg)-1);
    p->error_msg[sizeof(p->error_msg)-1] = '\0';
    fprintf(stderr, "parser error: %s at line %d\n", msg, p->tokens->tokens[p->curr].line);
}

static Token peek(Parser* p){
    return p->tokens->tokens[p->curr];
}

static Token prev(Parser* p){
    return p->tokens->tokens[p->curr-1];
}

static int is_at_end(Parser* p){
    return peek(p).type==EOF_TOK;
}

static Token advance(Parser* p){
    if (!is_at_end(p)) p->curr++;
    return prev(p);
}

static int check(Parser* p, TokenT type){
    if (is_at_end(p)) return 0;
    return peek(p).type==type;
}

static int match(Parser* p, TokenT type){
    if (check(p, type)){
        advance(p);
        return 1;
    }

    return 0;
}

static int match_multiple(Parser* p, TokenT* types, int count){
    for (int i=0; i<count; i++){
        if (match(p, types[i])){
            return 1;
        }
    }

    return 0;
}

static void eat(Parser* p, TokenT type, const char* msg){
    if (check(p, type)){
        advance(p);
        return;
    }

    parser_error(p, msg);
}


static ASTNode* parse_expression(Parser* p);
static ASTNode* parse_declaration(Parser* p);
static ASTNode* parse_stmt(Parser* p);
static ASTNode* parse_block(Parser* p);

// static VarT infer_var_type(const char* id, ExecutionContext* ctx){//num as fallback
//     if (ctx->curr_scope){
//         Var* var = get_var_from_scope(ctx->curr_scope, id);
//         if (var) return var->type;
//     }

//     Var* var = get_var(ctx->global_vars, id);
//     if (var) return var->type;

//     return NUM;
// }

static ASTNode* parse_primary(Parser* p){//nums, bools, parentheses
    if (match(p, NUM_TOK)) return create_num_node(prev(p).val.num);
    if (match(p, TRUE_TOK)) return create_bool_node(1);
    if (match(p, FALSE_TOK)) return create_bool_node(0);
    if (match(p, ID_TOK)) {
        char* id = prev(p).val.str;

        return create_var_ref_node(id);
    }
    if (match(p, LPAREN_TOK)){
        ASTNode* expr = parse_expression(p);
        eat(p, RPAREN_TOK, "expected ')' after expression");
        return expr;
    }

    parser_error(p, "expected expression\n");
    return NULL;
}

static ASTNode* parse_pow(Parser* p){
    ASTNode* expr = parse_primary(p);

    while (match(p, POW_TOK)){
        ASTNode* right = parse_pow(p);
        expr = create_bin_op_node(POW, expr, right);
    }

    return expr;
}

static ASTNode* parse_factor(Parser* p){
    ASTNode* expr = parse_pow(p);

    while (match(p, MULT_TOK) || match(p, DIV_TOK)){
        BinOpT op = prev(p).type == MULT_TOK ? MULT : DIV;
        ASTNode* right = parse_pow(p);
        expr = create_bin_op_node(op, expr, right);
    }

    return expr;
}

static ASTNode* parse_term(Parser* p){
    ASTNode* expr = parse_factor(p);

    while (match(p, PLUS_TOK) || match(p, MINUS_TOK)){
        BinOpT op = prev(p).type == PLUS_TOK ? PLUS : MINUS;
        ASTNode* right = parse_factor(p);
        expr = create_bin_op_node(op, expr, right);
    }

    return expr;
}

static ASTNode* parse_comparison(Parser* p){
    ASTNode* expr = parse_term(p);

    if (match(p, EQ_TOK)){
        ASTNode* right = parse_term(p);
        return create_cond_node(EQ, expr, right);
    } else if (match(p, SMALLER_THAN_TOK)){
        ASTNode* right = parse_term(p);
        return create_cond_node(SMALLER_THAN, expr, right);
    } else if (match(p, BIGGER_THAN_TOK)){
        ASTNode* right = parse_term(p);
        return create_cond_node(BIGGER_THAN, expr, right);
    }

    return expr;
}

static ASTNode* parse_expression(Parser* p){
    return parse_comparison(p);
}

static ASTNode* parse_var_declaration(Parser* p){
    if (!check(p, ID_TOK)){
        parser_error(p, "expected variable name");
        return NULL;
    }

    char id[VAR_LEN];
    strncpy(id, peek(p).val.str, VAR_LEN-1);
    id[VAR_LEN-1]='\0';
    advance(p);

    //let x := 9;
    if (match(p, COLON_ASSIGN_TOK)){
        ASTNode* initializer = parse_expression(p);
        eat(p, SEMICOLON_TOK, "expected ';' after variable declaration");

        if (initializer->type==BOOL_VAL){
            return create_dec_node_bool(initializer, id);
        } else {
            return create_dec_node_num(initializer, id);
        }
    }

    if (match(p, COLON_TOK)){

        //let x: num = 9;
        if (check(p, ID_TOK)){
            const char* type_name = peek(p).val.str;
            advance(p);

            eat(p, ASSIGN_TOK, "expected '=' after type in variable declaration");
            ASTNode* initializer = parse_expression(p);
            eat(p, SEMICOLON_TOK, "expected ';' after variable declaration");

            if (strcmp(type_name, "num")==0){
                return create_dec_node_num(initializer, id);
            } else if (strcmp(type_name, "bool")==0){
                return create_dec_node_bool(initializer, id);
            } else {
                parser_error(p, "unknown variable");
                return NULL;
            }
        } else {
            parser_error(p, "expected type name after ':'");
            return NULL;
        }
    } else if (match(p, ASSIGN_TOK)){
        ASTNode* initializer = parse_expression(p);
        eat(p, SEMICOLON_TOK, "expected ';' after variable declaration");

        if (initializer->type==BOOL_VAL){
            return create_dec_node_bool(initializer, id);
        } else {
            return create_dec_node_num(initializer, id);
        }
    } else {
        parser_error(p, "expected ':' or '=' after variable name");
        return NULL;
    }
}

static ASTNode* parse_if_statement(Parser* p){
    ASTNode* cond = parse_expression(p);
    ASTNode* body = parse_block(p);

    return create_if_node(cond, body);
}

static ASTNode* parse_for_loop(Parser* p){
    if (!match(p, ID_TOK)){
        parser_error(p, "expected loop variable name");
        return NULL;
    }

    char iter_name[VAR_LEN];
    strncpy(iter_name, prev(p).val.str, VAR_LEN-1);
    iter_name[VAR_LEN-1]='\0';

    eat(p, COLON_TOK, "expected ':' after loop variable");

    if (!match(p, NUM_TOK)){
        parser_error(p, "expected num start value");
        return NULL;
    }

    int start = (int)prev(p).val.num;

    eat(p, ARROW_TOK, "expected '->' between loop bounds");

    if (!match(p, NUM_TOK)){
        parser_error(p, "expected num start value");
        return NULL;
    }

    int end = (int)prev(p).val.num;

    ASTNode* body = parse_block(p);

    return create_loop_node(body, iter_name, start, end);
}

static ASTNode* parse_print_statement(Parser* p){
    MacroT type;

    if (prev(p).type==PRINT_TOK){
        type=PRINT;
    } else {
        type=PRINTLN;
    }

    ASTNode* val = parse_expression(p);
    eat(p, SEMICOLON_TOK, "expected ';' after print statement");

    return create_macro_node(type, val);
}

static ASTNode* parse_block(Parser* p){
    eat(p, LBRACE_TOK, "expected '{' before block");

    ASTNode** statements = NULL;
    int stmt_count = 0;

    while (!check(p, RBRACE_TOK) && !is_at_end(p)){
        ASTNode* stmt = parse_stmt(p);

        if (stmt){
            stmt_count++;
            statements=(ASTNode**)realloc(statements, sizeof(ASTNode*) * stmt_count);
            if (!statements){
                fprintf(stderr, "memory allocation failed");
                exit(EXIT_FAILURE);
            }

            statements[stmt_count-1] = stmt;
        }
    }

    eat(p, RBRACE_TOK, "expected '}' after block");

    if (stmt_count==0){
        ASTNode* empty_block = create_block_node(statements, stmt_count, NULL);
        free(statements);
        return empty_block;
    }

    return create_block_node(statements, stmt_count, NULL);
}

static ASTNode* parse_assignment(Parser* p){
    if (check(p, ID_TOK)){
        size_t curr_pos = p->curr;

        Token id_tok = advance(p);
        char* id = id_tok.val.str;

        if (match(p, ASSIGN_TOK)){
            ASTNode* expr = parse_expression(p);
            eat(p, SEMICOLON_TOK, "expected ';' after statement");

            if (expr->type==BOOL_VAL || expr->type==BOOL_REF || expr->type==COND){
                return create_reassign_node_bool(id, expr);
            } else {
                return create_reassign_node_num(id, expr);
            }
        } else {
            p->curr = curr_pos;
        }
    }

    return NULL;
}

static ASTNode* parse_stmt(Parser* p){
    ASTNode* assign_stmt = parse_assignment(p);
    if (assign_stmt) return assign_stmt;

    if (match(p, LET_TOK)) return parse_var_declaration(p);
    if (match(p, IF_TOK)) return parse_if_statement(p);
    if (match(p, PRINT_TOK) || match(p,PRINTLN_TOK)) return parse_print_statement(p);
    if (match(p, FOR_TOK)) return parse_for_loop(p);

    ASTNode* expr = parse_expression(p);
    eat(p, SEMICOLON_TOK, "expected ';' after expression");
    return expr;
}

static ASTNode* parse_declaration(Parser* p){
    return parse_stmt(p);
}

ASTNode* parse(TokenArr* tokens){
    Parser* p = init_parser(tokens);

    ASTNode* program = create_scope_node(NULL); //global scope

    while (!is_at_end(p)){
        ASTNode* decl = parse_declaration(p);
        if (decl) {
            add_stmt_to_scope(program, decl);
        }

        if (p->had_error){
            while (!is_at_end(p) && !match(p, SEMICOLON_TOK)){
                advance(p);
            }

            p->had_error=0;
        }
    }

    free(p);
    return program;
}

ASTNode* parse_file(const char* source){
    Lexer lexer = init_lexer(source);
    TokenArr* tokens = tokenize_all(&lexer);

    if (lexer.had_error){
        fprintf(stderr, "lexer errors\n");
        free_token_arr(tokens);
        return NULL;
    }

    ASTNode* program = parse(tokens);

    return program;
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ast.h"
#include "map.h"

ScopeData* curr_scope = NULL;//global scope

static ASTNode* create_node(){
    ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));

    n->left=NULL;
    n->right=NULL;

    if (!n){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    return n;
}

ASTNode* create_dec_node_num(ASTNode* expr, const char id[VAR_LEN]){
    ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));

    n->val.num=0;
    strcpy(n->val.id, id);
    n->type=NUM_DEC;
    n->left=expr;
    n->right=NULL;

    return n;
}

ASTNode* create_ref_node_num(const char id[VAR_LEN]){
    ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));

    n->type=NUM_REF;
    n->left=NULL; n->right=NULL;
    strcpy(n->val.id, id);

    return n;
}

ASTNode* create_var_ref_node(const char id[VAR_LEN]){
    ASTNode* n = create_node();

    n->type=VAR_REF;
    strcpy(n->val.id, id);
    return n;
}

ASTNode* create_macro_node(MacroT t, ASTNode* left){
    ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));
    n->type=MACRO;
    n->val.mtype=t;

    n->left=left;
    n->right=NULL;

    return n;
}

ASTNode* create_num_node(double x){
    ASTNode* new = (ASTNode*)malloc(sizeof(ASTNode));
    if (!new) {
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    new->type=NUM_VAL;
    new->val.num=x;
    new->left=NULL;
    new->right=NULL;

    return new;
}

ASTNode* create_bool_node(int x){
    ASTNode* n = create_node();
    n->type=BOOL_VAL;
    n->val.bool_val=x;

    return n;
}

ASTNode* create_dec_node_bool(ASTNode* expr, const char id[VAR_LEN]){
    ASTNode* n = create_node();

    strcpy(n->val.id, id);
    n->type=BOOL_DEC;
    n->left=expr;

    return n;
}

ASTNode* create_ref_node_bool(const char id[VAR_LEN]){
    ASTNode* n = create_node();

    n->type=BOOL_REF;
    strcpy(n->val.id, id);

    return n;
}

ASTNode* create_bin_op_node(BinOpT t, ASTNode* left, ASTNode* right){
    ASTNode* new = (ASTNode*)malloc(sizeof(ASTNode));
    if (!new){
        fprintf(stderr, "memory allocatin failed\n");
        exit(EXIT_FAILURE);
    }

    new->type=B_OP;
    new->left=left;
    new->right=right;
    new->val.type=t;

    return new;
}

ASTNode* create_cond_node(CondT t, ASTNode* l, ASTNode* r){
    ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));

    if (!n){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    n->type=COND;
    n->left=l;
    n->right=r;
    n->val.ctype=t;

    return n;
}

ASTNode* create_if_node(ASTNode* cond, ASTNode* code){
    ASTNode* n = create_node();

    n->type=IF;
    n->left=cond;

    if (code->type!=SCOPE && code->type!=BLOCK){
        ASTNode* scope_node = create_scope_node(NULL);

        add_stmt_to_scope(scope_node, code);
        n->right=scope_node;
    } else {
        n->right=code;
    }

    return n;
}

ASTNode* create_loop_node(ASTNode* code, const char iter[VAR_LEN], int start, int end){
    ASTNode* n = create_node();
    n->type=LOOP;
    n->val.max_loop=end;
    strcpy(n->val.id, iter);

    ASTNode* scope_node = create_scope_node(NULL);
    ASTNode* iter_dec = create_dec_node_num(create_num_node(start), iter); //initializing the iter var

    add_stmt_to_scope(scope_node, iter_dec);

    if (code->type!=SCOPE && code->type != BLOCK){
        add_stmt_to_scope(scope_node, code);
    } else {
        for (int i=0; i<code->val.scope->stmt_count; i++){
            add_stmt_to_scope(scope_node, code->val.scope->statements[i]);
        }
    }

    n->right = scope_node;
    return n;
}

ASTNode* create_reassign_node_num(const char id[VAR_LEN], ASTNode* expr){
    ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));
    if (!n){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    n->type=NUM_REASSIGN;
    strcpy(n->val.id, id);
    n->left = expr;
    n->right = NULL;

    return n;
}

ASTNode* create_reassign_node_bool(const char id[VAR_LEN], ASTNode* expr){
    ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));
    if (!n){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    n->type=BOOL_REASSIGN;
    strcpy(n->val.id, id);
    n->left = expr;
    n->right = NULL;

    return n;
}

ExecutionContext* create_execution_context(){
    ExecutionContext* ctx = (ExecutionContext*)malloc(sizeof(ExecutionContext));
    if (!ctx){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    ctx->curr_scope=NULL;
    ctx->global_vars=create_map();
    return ctx;
}




ScopeData* init_scope_data(ScopeData* parent){
    ScopeData* scope = (ScopeData*)malloc(sizeof(ScopeData));
    if (!scope){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    scope->variables=create_map();
    scope->statements=NULL;
    scope->stmt_count=0;
    scope->parent=parent;

    return scope;
}

ASTNode* create_block_node(ASTNode** statements, int count, ScopeData* parent_scope){
    ASTNode* node = create_scope_node(parent_scope);
    node->type=BLOCK;

    for (int i=0; i<count; i++){
        add_stmt_to_scope(node, statements[i]);
    }

    return node;
}

ASTNode* create_scope_node(ScopeData* parent) {
    ASTNode* n = create_node();

    n->type = SCOPE;
    n->left = NULL;
    n->right = NULL;
    n->val.scope = init_scope_data(parent);

    return n;
}

void add_stmt_to_scope(ASTNode *scope, ASTNode *stmt){
    if (scope->type!=SCOPE && scope->type!=BLOCK){
        fprintf(stderr, "not a scope or a block node\n");
        return;
    }

    ScopeData* scoped = scope->val.scope;

    scoped->stmt_count++;
    scoped->statements=(ASTNode**)realloc(scoped->statements, sizeof(ASTNode*) * scoped->stmt_count);

    if (!scoped){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    scoped->statements[scoped->stmt_count-1]=stmt;
}

Var* get_var_from_scope(ScopeData* scope, const char id[VAR_LEN]){
    if (!scope) return NULL;

    Var* var = get_var(scope->variables, id);

    if (!var&&scope->parent){
        return get_var_from_scope(scope->parent, id);
    }

    return var;
}

void add_num_var_to_scope(ScopeData *scope, const char *id, double val){
    if (!scope){
        fprintf(stderr, "error: NULL scope\n");
        exit(EXIT_FAILURE);
    }

    Var* var = (Var*)malloc(sizeof(Var));
    if (!var){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    var->type = NUM;
    var->val.num=val;
    var->next=NULL;
    strcpy(var->id, id);

    insert_var(scope->variables, var);
}

void add_bool_var_to_scope(ScopeData *scope, const char *id, int val){
    if (!scope){
        fprintf(stderr, "error: NULL scope\n");
        exit(EXIT_FAILURE);
    }

    Var* var = (Var*)malloc(sizeof(Var));
    if (!var){
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    var->type=BOOL;
    var->val.b=val;
    var->next=NULL;
    strcpy(var->id, id);

    insert_var(scope->variables, var);
}




void execute_scope(ASTNode* scope, ExecutionContext* ctx){
    if (!scope) return;
    if (scope->type!=BLOCK && scope->type!=SCOPE){
        fprintf(stderr, "not a scope or a block node\n");
        exit(EXIT_FAILURE);
    }

    ScopeData* data = scope->val.scope;
    ScopeData* prev_scope = ctx->curr_scope;
    ctx->curr_scope=data;

    for (int i=0; i<data->stmt_count; i++){
        execute(data->statements[i], ctx);
    }

    ctx->curr_scope=prev_scope;
}

double execute_ref_num(ASTNode* node, ExecutionContext* ctx){
    if (node->type!=NUM_REF) return 0;

    // return get_ref(node->val.id)->val.num;
    if (ctx->curr_scope){
        Var* var=get_var_from_scope(ctx->curr_scope, node->val.id);
        if (var) return var->val.num;
    }

    //return get_ref(node->val.id)->val.num;
    Var* var = get_var(ctx->global_vars, node->val.id);
    if (!var){
        fprintf(stderr, "error: variable '%s' not found in scope\n", node->val.id);
        exit(EXIT_FAILURE);
    }

    return var->val.num;
}

int execute_ref_bool(ASTNode* node, ExecutionContext* ctx){
    if (node->type!=BOOL_REF) return 0;

    if (ctx->curr_scope){
        Var* var = get_var_from_scope(ctx->curr_scope, node->val.id);
        if (var && var->type==BOOL) return var->val.b;
    }

    Var* var = get_var(ctx->global_vars, node->val.id);
    if (!var||var->type!=BOOL){
        fprintf(stderr, "error: bool variable '%s' not found in scope\n", node->val.id);
        exit(EXIT_FAILURE);
    }

    return var->val.b;
}

Var* get_var_ref(const char* id, ExecutionContext* ctx){
    if (ctx->curr_scope){
        Var* var = get_var_from_scope(ctx->curr_scope, id);
        return var;
    }

    return get_var(ctx->global_vars, id);
}

double num_evaluate_ast(ASTNode* node, ExecutionContext* ctx){
    switch (node->type){
        case (NUM_VAL): return node->val.num;
        case (B_OP): {
            double a=num_evaluate_ast(node->left, ctx);
            //free(node->left);
            double b=num_evaluate_ast(node->right, ctx);
            //free(node->right);
            switch (node->val.type){
                case (PLUS): return a+b;
                case (MINUS): return a-b;
                case (MULT): return a*b;
                case (DIV): {
                    if (b!=0) return a/b;
                    fprintf(stderr, "error: division with 0!\n");
                    exit(EXIT_FAILURE);
                };
                case POW: return pow(a,b);
                default: return 0;
            }
        };
        case VAR_REF: {
            Var* var = get_var_ref(node->val.id, ctx);
            if (var&&var->type==NUM){
                return var->val.num;
            } else {
                fprintf(stderr, "error: expecred numeric variable '%s'\n", node->val.id);
                exit(EXIT_FAILURE);
            }
        }
        case NUM_REF: return get_var_ref(node->val.id, ctx)->val.num;
        default: return 0;
    }
}

int bool_evaluate_ast(ASTNode *node, ExecutionContext *ctx){
    if (!node||!ctx) {
        fprintf(stderr, "null");
        exit(EXIT_FAILURE);
    }

    switch (node->type){
        case BOOL_VAL: return node->val.bool_val;
        case COND: return execute_cond(node, ctx);
        case BOOL_REF: return get_var_ref(node->val.id, ctx)->val.b;
        case VAR_REF: {
            Var* var = get_var_ref(node->val.id, ctx);
            if (var && var->type==BOOL){
                return var->val.b;
            } else if (var && var->type==NUM){
                return var->val.num!=0;
            } else {
                fprintf(stderr, "error: expected boolean variable '%s'\n", node->val.id);
                exit(EXIT_FAILURE);
            }
            return 0;
        }
        case NUM_VAL:
        case NUM_REF:
        case B_OP:
            return num_evaluate_ast(node, ctx) != 0;
        default: {
            fprintf(stderr, "error: non-boolean expr\n");
            exit(EXIT_FAILURE);
        }
    }
}

void execute_dec_bool(ASTNode *node, ExecutionContext *ctx){
    if (node->type!=BOOL_DEC) return;

    int val = bool_evaluate_ast(node->left, ctx);

    if (ctx->curr_scope){
        add_bool_var_to_scope(ctx->curr_scope, node->val.id, val);
    } else {
        Var* n = (Var*)malloc(sizeof(Var));
        n->type=BOOL;
        n->val.b = val;
        n->next=NULL;
        strcpy(n->id, node->val.id);
        insert_var(ctx->global_vars, n);
    }
}

void execute_macro(ASTNode* node, ExecutionContext* ctx){
    if (node->type!=MACRO) return;

    //printf("print type: %d\n", node->left->type);

    if (node->left->type==VAR_REF){
        Var* var = get_var_ref(node->left->val.id, ctx);
        if (var){
            if (var->type==BOOL){
                printf("%s", var->val.b ? "true" : "false");
            } else if (var->type==NUM){
                printf("%f", var->val.num);
            }
            if (node->val.mtype==PRINTLN) printf("\n");
        }
        return;
    }

    switch (node->val.mtype){
        case PRINT: {
            if (node->left->type==NUM_VAL||node->left->type==NUM_REF||node->left->type==B_OP){
                printf("%f", num_evaluate_ast(node->left, ctx));
            } else if (node->left->type==BOOL_VAL||node->left->type==BOOL_REF||node->left->type==COND){
                printf("%s", bool_evaluate_ast(node->left, ctx) ? "true":"false");
            }
        } break;
        case PRINTLN: {
            if (node->left->type==NUM_VAL||node->left->type==NUM_REF||node->left->type==B_OP){
                printf("%f\n", num_evaluate_ast(node->left, ctx));
            } else if (node->left->type==BOOL_VAL||node->left->type==BOOL_REF||node->left->type==COND){
                printf("%s\n", bool_evaluate_ast(node->left, ctx) ? "true":"false");
            }
            } break;
        default: break;
    }
}

void execute_dec(ASTNode* node, ExecutionContext* ctx){
    if (node->type!=NUM_DEC) return; //!!!!

    double val = num_evaluate_ast(node->left, ctx);

    if (ctx->curr_scope){
        add_num_var_to_scope(ctx->curr_scope, node->val.id, val);
    } else {
        Var* n = (Var*)malloc(sizeof(Var));
        n->type=NUM;
        n->val.num=val;
        n->next=NULL;
        strcpy(n->id, node->val.id);
        insert_var(ctx->global_vars, n);
    }

    // switch (node->type){
    //     case NUM_DEC: add_var_num(num_evaluate_ast(node->left), node->val.id); break;
    //     default: break;
    // }
}

int execute_cond(ASTNode* node, ExecutionContext* ctx){
    if (!node) exit(EXIT_FAILURE);

    if (node->type!=COND) return 0;

    double a = num_evaluate_ast(node->left, ctx);
    double b = num_evaluate_ast(node->right, ctx);

    switch (node->val.ctype){
        case EQ: return a==b;
        case SMALLER_THAN: return a<b;
        case BIGGER_THAN: return a>b;
        default: printf("invalid COND"); break;
    }

    return 0;
}

void execute_if(ASTNode* n, ExecutionContext* ctx){
    if (!n) exit(EXIT_FAILURE);
    if (n->type!=IF) exit(EXIT_FAILURE);

    int condition=0;
    if (n->left->type==VAR_REF){//str!!!!!
        Var* var = get_var_ref(n->left->val.id, ctx);
        if (var->type==BOOL){
            condition = var->val.b;
        } else if (var->type==NUM){
            condition = var->val.num!=0;
        } else {
            fprintf(stderr, "invalid type for if\n");
            exit(EXIT_FAILURE);
        }
    } else if (n->left->type == COND) {
        condition = execute_cond(n->left, ctx);
    } else if (n->left->type == BOOL_REF || n->left->type == BOOL_VAL) {
        condition = bool_evaluate_ast(n->left, ctx);
    } else if (n->left->type == NUM_REF || n->left->type == NUM_VAL || n->left->type == B_OP) {
        condition = num_evaluate_ast(n->left, ctx) != 0;
    } else {
        fprintf(stderr, "Error: Invalid condition type in if statement\n");
        exit(EXIT_FAILURE);
    }

    if (condition == 1){
        if (n->right->type==SCOPE || n->right->type==BLOCK){
            n->right->val.scope->parent=ctx->curr_scope;
        }
        execute(n->right, ctx);
    }
}

void execute_loop(ASTNode* n, ExecutionContext* ctx){
    if (!n) exit(EXIT_FAILURE);
    if (n->type!=LOOP) exit(EXIT_FAILURE);

    if (n->right->type!=SCOPE && n->right->type!=BLOCK){
        fprintf(stderr, "loop must be a scope\n");
        exit(EXIT_FAILURE);
    }

    n->right->val.scope->parent=ctx->curr_scope;

    ASTNode* iter_dec = n->right->val.scope->statements[0];
    if (iter_dec->type!=NUM_DEC){
        fprintf(stderr, "first stmt in loop isnt num\n");
        exit(EXIT_FAILURE);
    }

    ScopeData* prev_scope = ctx->curr_scope;
    ctx->curr_scope = n->right->val.scope;
    execute_dec(iter_dec, ctx);
    ctx->curr_scope = prev_scope;

    while (1){
        Var* iter_var = get_var_from_scope(n->right->val.scope, n->val.id);
        if (!iter_var){
            fprintf(stderr, "iterator var not found\n");
            exit(EXIT_FAILURE);
        }

        if (iter_var->val.num>=n->val.max_loop-1){
            break;
        }

        ScopeData* prev_scope = ctx->curr_scope;
        ctx->curr_scope = n->right->val.scope;

        for (int i=1; i<n->right->val.scope->stmt_count; i++){
            execute(n->right->val.scope->statements[i], ctx);
        }

        ctx->curr_scope=prev_scope;

        iter_var = get_var_from_scope(n->right->val.scope, n->val.id);
        iter_var->val.num++;
    }
}

void execute_reassign_num(ASTNode *node, ExecutionContext *ctx){
    if (node->type!=NUM_REASSIGN) return;

    double new_val = num_evaluate_ast(node->left, ctx);

    if (ctx->curr_scope){
        Var* var = get_var_from_scope(ctx->curr_scope, node->val.id);
        if (var){
            if (var->type==NUM){
                var->val.num=new_val;
                return;
            } else {
                fprintf(stderr, "error: cannot assign numeric value to non-numeric variable '%s'\n", node->val.id);
                exit(EXIT_FAILURE);
            }
        }
    }

    Var* var = get_var(ctx->global_vars, node->val.id);
    if (!var){
        fprintf(stderr, "error: varible '%s' not found\n", node->val.id);
        exit(EXIT_FAILURE);
    }

    if (var->type!=NUM){
        fprintf(stderr, "error: cannot assign numeric value to non-numeric varible '%s'\n", node->val.id);
        exit(EXIT_FAILURE);
    }

    var->val.num = new_val;
}

void execute_reassign_bool(ASTNode* node, ExecutionContext* ctx) {
    if (node->type != BOOL_REASSIGN) return;

    int new_val = bool_evaluate_ast(node->left, ctx);

    if (ctx->curr_scope) {
        Var* var = get_var_from_scope(ctx->curr_scope, node->val.id);
        if (var) {
            if (var->type == BOOL) {
                var->val.b = new_val;
                return;
            } else {
                fprintf(stderr, "error: cannot assign boolean value to non-boolean variable '%s'\n", node->val.id);
                exit(EXIT_FAILURE);
            }
        }
    }

    Var* var = get_var(ctx->global_vars, node->val.id);
    if (!var) {
        fprintf(stderr, "error: variable '%s' not found for reassignment\n", node->val.id);
        exit(EXIT_FAILURE);
    }

    if (var->type != BOOL) {
        fprintf(stderr, "error: cannot assign boolean value to non-boolean variable '%s'\n", node->val.id);
        exit(EXIT_FAILURE);
    }

    var->val.b = new_val;
}

void execute(ASTNode* node, ExecutionContext* ctx){
    if (!node) {
        printf("Error: NULL node passed to execute\n");
        return;
    }

    switch (node->type){
        case BOOL_DEC:
            execute_dec_bool(node, ctx);
            return;
        case NUM_DEC:
            execute_dec(node, ctx);
            return;
        case MACRO:
            execute_macro(node, ctx);
            return;
        case IF:
            execute_if(node, ctx);
            return;
        case SCOPE:
        case BLOCK:
            execute_scope(node, ctx);
            //free_ast(node);
            return;
        case LOOP:
            execute_loop(node, ctx);
            return;
        case NUM_REASSIGN:
            execute_reassign_num(node, ctx);
            return;
        case BOOL_REASSIGN:
            execute_reassign_bool(node, ctx);
            return;
        default:
            printf("Unknown node type: %d\n", node->type);
            break;
    }
}

void free_scope(ScopeData* scope){
    if (!scope) return;

    //for (int i=0; i<scope->stmt_count; i++)
    free(scope->statements);
    free_map(scope->variables);
    free(scope);
}

void free_ast(ASTNode* node){
    if (!node) return;

    if (node->type==SCOPE||node->type==BLOCK){
        for (int i=0; i<node->val.scope->stmt_count; i++){
            free_ast(node->val.scope->statements[i]);
        }

        free_scope(node->val.scope);
    } else {
        free_ast(node->left);
        free_ast(node->right);
    }

    free(node);
}

void free_execution_context(ExecutionContext* ctx){
    if (!ctx) return;

    free_map(ctx->global_vars);
    free(ctx);
}

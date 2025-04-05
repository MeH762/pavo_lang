#ifndef AST_H
#define AST_H

#include <math.h>

#include "main.h"
#include "map.h"

typedef enum {
    PLUS,
    MINUS,
    MULT,
    DIV,
    POW,
} BinOpT;

typedef enum {
    PRINT,
    PRINTLN,
} MacroT;

typedef enum {
    EQ,
    SMALLER_THAN,
    BIGGER_THAN,
} CondT;

typedef enum{
    NUM_VAL,
    BOOL_VAL,
    B_OP,
    NUM_DEC,
    BOOL_DEC,
    NUM_REF,
    BOOL_REF,

    VAR_REF,

    MACRO,
    COND,
    IF,
    SCOPE,
    BLOCK,
    LOOP,//for loop
    NUM_REASSIGN,
    BOOL_REASSIGN,
} ASTNodeT;

typedef struct ScopeData ScopeData ;

typedef struct ASTNode{
    ASTNodeT type;
    union{
        double num;
        BinOpT type;
        char id[30+1];
        MacroT mtype;
        CondT ctype;
        ScopeData* scope;
        double max_loop;//for loops
        int bool_val;
    } val;
    struct ASTNode* left;
    struct ASTNode* right;
} ASTNode;

typedef struct ScopeData {
    Map* variables;
    struct ASTNode** statements;
    int stmt_count;
    struct ScopeData* parent;
} ScopeData;

typedef struct{
    ScopeData* curr_scope;
    Map* global_vars;
    //int max_iter->inf loops
    //error handling
} ExecutionContext;

ASTNode* create_var_ref_node(const char id[VAR_LEN]);

ASTNode* create_bool_node(int val);
ASTNode* create_dec_node_bool(ASTNode* expr, const char id[VAR_LEN]);
ASTNode* create_ref_node_bool(const char id[VAR_LEN]);

int bool_evaluate_ast(ASTNode* node, ExecutionContext* ctx);
void execute_dec_bool(ASTNode* node, ExecutionContext* ctx);
int execute_ref_bool(ASTNode* node, ExecutionContext* ctx);//!, &&, ||

ASTNode* create_num_node(double x);
ASTNode* create_bin_op_node(BinOpT t, ASTNode* left, ASTNode* right);
ASTNode* create_macro_node(MacroT t, ASTNode* left);
ASTNode* create_dec_node_num(ASTNode* expr, const char id[VAR_LEN]);//AST Node ----> exec: add_var_num
ASTNode* create_ref_node_num(const char id[VAR_LEN]);
ASTNode* create_cond_node(CondT t, ASTNode* l, ASTNode* r);
ASTNode* create_if_node(ASTNode* cond, ASTNode* code);
ASTNode* create_loop_node(ASTNode* code, const char iter[VAR_LEN], int start, int end);
ASTNode* create_reassign_node_num(const char id[VAR_LEN], ASTNode* expr);
ASTNode* create_reassign_node_bool(const char id[VAR_LEN], ASTNode* expr);

ASTNode* create_scope_node(ScopeData* parent);
ASTNode* create_block_node(ASTNode** statements, int count, ScopeData* parent);
void add_stmt_to_scope(ASTNode* scope, ASTNode* stmt);
Var* get_var_from_scope(ScopeData* scope, const char id[VAR_LEN]);
void add_num_var_to_scope(ScopeData* scope, const char id[VAR_LEN], double val);//num only
void add_bool_var_to_scope(ScopeData* scope, const char id[VAR_LEN], int val);

double num_evaluate_ast(ASTNode* node, ExecutionContext* ctx);
void execute_macro(ASTNode* node, ExecutionContext* ctx);
void execute(ASTNode* node, ExecutionContext* ctx);//->AST: type of evaluation depends on the type of var it goes into
void execute_dec(ASTNode* node, ExecutionContext* ctx);
double execute_ref_num(ASTNode* node, ExecutionContext* ctx);
int execute_cond(ASTNode* node, ExecutionContext* ctx);
void execute_if(ASTNode* n, ExecutionContext* ctx);
void execute_loop(ASTNode *node, ExecutionContext *ctx);
void execute_reassign_num(ASTNode* node, ExecutionContext* ctx);
void execute_reassign_bool(ASTNode *node, ExecutionContext *ctx);

void execute_scope(ASTNode* scope, ExecutionContext* ctx);
void execute_block(ASTNode* block, ExecutionContext* ctx);

void free_scope(ScopeData* scope);
void free_ast(ASTNode* node);

ExecutionContext* create_execution_context();
void free_execution_context(ExecutionContext* ctx);

#endif

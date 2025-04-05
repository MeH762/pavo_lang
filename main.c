#include <complex.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include "ast.h"
#include "map.h"
#include "lexer.h"
#include "parser.h"
#include "main.h"

Map* m;

extern ScopeData* curr_scope;

extern Var* get_ref(const char id[VAR_LEN]){
    if (curr_scope){
        Var* var=get_var_from_scope(curr_scope, id);
        if (var) return var;
    }

    Var* res = get_var(m, id);
    if (!res){
        fprintf(stderr, "error: variable '%s' not found in scope\n", id);
        exit(EXIT_FAILURE);
    }

    return res;
}

void add_var_num(double x, const char id[VAR_LEN]){
    Var* n = (Var*)malloc(sizeof(Var));
    n->type=NUM;
    n->val.num=x;
    n->next=NULL;

    strcpy(n->id, id);

    insert_var(m, n);
}



void debug_tokens(TokenArr* tokens) {
    printf("\n--- TOKEN DUMP ---\n");
    for (size_t i = 0; i < tokens->count; i++) {
        Token t = tokens->tokens[i];
        printf("%zu: ", i);
        print_tok(t);
    }
    printf("--- END TOKEN DUMP ---\n\n");
}

void debug_print_node_type(ASTNode* node) {
    if (!node) {
        printf("NULL node\n");
        return;
    }

    const char* type_str = "UNKNOWN";

    switch(node->type) {
        case NUM_VAL: type_str = "NUM_VAL"; break;
        //case BOOL_VAL: type_str = "BOOL_VAL"; break;
        case B_OP: type_str = "B_OP"; break;
        case NUM_DEC: type_str = "NUM_DEC"; break;
        //case BOOL_DEC: type_str = "BOOL_DEC"; break;
        case NUM_REF: type_str = "NUM_REF"; break;
        case BOOL_REF: type_str = "BOOL_REF"; break;
        case MACRO: type_str = "MACRO"; break;
        case COND: type_str = "COND"; break;
        case IF: type_str = "IF"; break;
        case SCOPE: type_str = "SCOPE"; break;
        case BLOCK: type_str = "BLOCK"; break;
        case LOOP: type_str = "LOOP"; break;
    }

    printf("Node type: %s\n", type_str);

    if (node->type == NUM_DEC || node->type == BOOL_DEC ||
        node->type == NUM_REF || node->type == BOOL_REF) {
        printf("  Variable name: %s\n", node->val.id);
    }

    if (node->type == SCOPE || node->type == BLOCK) {
        printf("  Statements: %d\n", node->val.scope->stmt_count);
    }
}


//TEST
void run_interpreter(const char* source, const char* description){
    printf("\n======RUNNING TEST: %s======\n", description);
    printf("[SOURCE]\n%s\n", source);

    Lexer l = init_lexer(source);
    TokenArr* tokens = tokenize_all(&l);

    if (l.had_error){
        printf("lexer errors!!!!!!!!!!\n");
        free_token_arr(tokens);
        return;
    }

    debug_tokens(tokens);

    ASTNode* program = parse(tokens);

    if (!program){
        printf("parser errors!!!!!!\n");
        free_token_arr(tokens);
        return;
    }

    printf("\nEXECUTING\n");
    ExecutionContext* ctx = create_execution_context();
    execute(program, ctx);

    printf("\nEXECUTION OVER\n");

    free_ast(program);
    free_execution_context(ctx);
    free_token_arr(tokens);
}

void test_var_dec(){
    const char* source =
    "let x:num = 12;\n"
    "println x;\n"
    "let y := 99;\n"
    "println y;\n";

    run_interpreter(source, "VAR DEC");
}

void test2(){
    const char* source =
    "let x := 5;\n"
    "for i : 0->5 {\n"
    "   println i;\n"
    "}\n";

    run_interpreter(source, "TEST2");
}

char* read_file(const char* filename){
    FILE* file = fopen(filename, "r");
    if (!file){
        fprintf(stderr, "error: could not open file '%s'\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(file_size+1);
    if (!buffer){
        fprintf(stderr, "error: memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read<file_size){
        fprintf(stderr, "error: could not read eniter file\n");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[file_size]='\0';

    fclose(file);
    return buffer;
}


int main(int argc, char* argv[]){
    clock_t start = clock();

    m=create_map();
    if (argc < 2){
        printf("usage: %s <filename.pavo>\n", argv[0]);
    } else {
        const char* filename = argv[1];

        const char* ext = strchr(filename, '.');
        if (!ext || strcmp(ext, ".pavo")!=0){
            fprintf(stderr, "error: file must have .pavo extension\n");
            free_map(m);
            return EXIT_FAILURE;
        }

        char* source = read_file(filename);
        if (!source){
            free_map(m);
            return EXIT_FAILURE;
        }

        //printf("\n=======RUNNING FILE=======\n");
        Lexer l = init_lexer(source);
        TokenArr* tokens = tokenize_all(&l);

        if (l.had_error){
            printf("lexer errors in file '%s'\n", filename);
            free_token_arr(tokens);
            free(source);
            free_map(m);
            return EXIT_FAILURE;
        }

        ASTNode* program = parse(tokens);

        if (!program){
            printf("parser errors in file '%s'\n", filename);
            free_token_arr(tokens);
            free(source);
            free_map(m);
            return EXIT_FAILURE;
        }

        ExecutionContext* ctx = create_execution_context();
        execute(program, ctx);

        free_ast(program);
        free_execution_context(ctx);
        free_token_arr(tokens);
        free(source);
    }

    free_map(m);


    clock_t end = clock();
    double t = ((double)(end-start))/CLOCKS_PER_SEC;
    printf("\nexecution time: %f \n", t);

    return 0;
}

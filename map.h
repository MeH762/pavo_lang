#ifndef MAP_H
#define MAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_VAR_COUNT 150
#define VAR_LEN 31

typedef enum {
    NUM,
    BOOL,
    STR,
} VarT;

typedef struct Var { //variable implementation
    char id[VAR_LEN];
    union{
        double num;
        int b;//bool
        char* str;
    } val;
    VarT type;
    struct Var* next;
} Var;

typedef struct Map {
    size_t size;
    Var **buckets;
} Map;

Map* create_map();
void free_map(Map* m);
void insert_var(Map* m, Var* n);
Var* new_var(const char id[VAR_LEN]);
Var* get_var(Map* m, const char id[VAR_LEN]);
int hash_fnv1a(const char* str);
void print_map(Map* m);

#endif

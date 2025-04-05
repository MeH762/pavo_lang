#include "map.h"

int hash_fnv1a(const char* str){
    const unsigned int FNV_PRIME = 16777619;
    const unsigned int FNV_OFFSET = 2166136261;

    unsigned int hash = FNV_OFFSET;
    for (int i=0; str[i]; i++){
        hash ^= (unsigned char)str[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

Map* create_map(){
    Map* m = (Map*)malloc(sizeof(Map));
    if (!m) {
        fprintf(stderr, "memory allocation failed\n");
        exit(1);
    }

    Var **b = (Var**)malloc(sizeof(Var*)*MAX_VAR_COUNT);
    if (!b){
        fprintf(stderr, "memory allocation failed\n");
        free(m);
        exit(1);
    }

    m->size=MAX_VAR_COUNT;
    m->buckets=b;
    for (int i=0; i<MAX_VAR_COUNT; i++){
        m->buckets[i]=NULL;
    }

    return m;
}

void free_map(Map* m){
    for (int i=0; i<m->size; i++){
        if (m->buckets[i]!=NULL){
            Var* curr = m->buckets[i];
            Var* next;

            while (curr){
                next = curr->next;
                free(curr);
                curr = next;
            }
        }
    }

    free(m->buckets);
    free(m);
}

void print_map(Map* m){
    printf("print map\n");
    for (int i=0; i<m->size; i++){
        if (m->buckets[i]){
            Var* curr = m->buckets[i];
            Var* next;

            while (curr) {
                next = curr->next;
                printf("%s: ", curr->id);
                if (curr->type==NUM) {
                    printf("%f\n", curr->val.num);
                } else if (curr->type==STR) {
                    printf("%s\n", curr->val.str);
                } else {
                    printf("%d\n", curr->val.b);
                }
            }
        }
    }
}

Var* new_var(const char id[VAR_LEN]){
    Var* new = (Var*)malloc(sizeof(Var));
    strcpy(new->id, id);

    return new;
}

void insert_var(Map* m, Var* n){
    int index = hash_fnv1a(n->id)%MAX_VAR_COUNT;
    Var* curr = m->buckets[index];
    Var* prev = NULL;

    while (curr){
        if (strcmp(curr->id, n->id)==0){
            if (prev==NULL){
                n->next = curr->next;
                m->buckets[index] = n;
            } else {
                n->next = curr->next;
                prev->next = n;
            }
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }

    if (!prev){
        m->buckets[index]=n;
    } else {
        prev->next = n;
    }
}

Var* get_var(Map* m, const char id[VAR_LEN]){
    Var* curr = m->buckets[hash_fnv1a(id)%MAX_VAR_COUNT];
    while (curr){
        if (strcmp(curr->id, id)==0){
            return curr;
        }

        curr=curr->next;
    }
    return NULL;
}

#include <igraph/igraph.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "hashtable.h"
#include "hash.h"



typedef struct _cga_ht_node {
    unsigned long key;
    igraph_integer_t value;
    struct _cga_ht_node *next;
} cga_ht_node_t;

struct _cga_hashtable {
    size_t size;
    size_t nelem;
    cga_ht_node_t **table;
};

static cga_ht_node_t* cga_ht_node_constructor(unsigned long key, igraph_integer_t value, cga_ht_node_t *next);
static void cga_ht_node_destructor(cga_ht_node_t *node);

static cga_ht_node_t* cga_ht_node_constructor(unsigned long key, igraph_integer_t value, cga_ht_node_t *next) {
    cga_ht_node_t *htn = NULL;
    if((htn = (cga_ht_node_t*)malloc(sizeof(cga_ht_node_t))) == NULL)
        return NULL;
    htn->key = key;
    htn->value = value;
    htn->next = next;
    return htn;
}

static void cga_ht_node_destructor(cga_ht_node_t *node) {
    free(node);
}

cga_hashtable_t* cga_ht_init(size_t size) {
    if(size == 0) return NULL;
    cga_hashtable_t *ht = (cga_hashtable_t*)malloc(sizeof(cga_hashtable_t));
    if(ht == NULL)
        return NULL;
    ht->size = size;
    ht->nelem = 0;
    if((ht->table = (cga_ht_node_t**)malloc(size * sizeof(cga_ht_node_t*))) == NULL)
        return NULL;
    for(size_t i = 0; i < size; i++)
        ht->table[i] = NULL;
    return ht;
}

void cga_ht_destroy(cga_hashtable_t *ht) {
    cga_ht_clear(ht);
    free(ht->table);
    free(ht);
}

void cga_ht_clear(cga_hashtable_t *ht) {
    for(size_t i = 0; i < ht->size; i++) {
        while(ht->table[i] != NULL) {
            cga_ht_node_t *temp = ht->table[i];
            ht->table[i] = temp->next;
            cga_ht_node_destructor(temp);
        }
    }
    ht->nelem = 0;
}

cga_status_t cga_ht_insert(cga_hashtable_t *ht, unsigned long key, igraph_integer_t value) {
    if(cga_ht_contains(ht, key)) return DPLKTKEY;
    size_t index = cga_hash(key) % ht->size;
    cga_ht_node_t *node = cga_ht_node_constructor(key,value, ht->table[index]);
    if(node == NULL )
        return NOMEM;
    ht->table[index] = node;
    ht->nelem = ht->nelem + 1;
    return SUCCESS;
}

igraph_integer_t* cga_ht_search(cga_hashtable_t *ht, unsigned long key) {
    size_t index = cga_hash(key) % ht->size;
    cga_ht_node_t *node = ht->table[index];
    while(node != NULL) {
        if(node->key == key)
            return &(node->value);
        node = node->next;
    }
    return NULL;
}

int cga_ht_contains(cga_hashtable_t *ht, unsigned long key) {
    size_t index = cga_hash(key) % ht->size;
    cga_ht_node_t *node = ht->table[index];
    while(node != NULL) {
        if(node->key == key)
            return 1;
        else node = node->next;
    }
    return 0;
}

cga_status_t cga_ht_delete(cga_hashtable_t *ht, unsigned long key) {
    size_t index = cga_hash(key) % ht->size;
    cga_ht_node_t *current = ht->table[index];
    cga_ht_node_t *prev = NULL;
    while(current != NULL) {
        if(current->key != key) {
            prev = current;
            current = current->next;
        }
        else {
            if(prev == NULL) {
                ht->table[index] = current->next;
            }
            else {
                prev->next = current->next;
            }
            cga_ht_node_destructor(current);
            ht->nelem = ht->nelem - 1;
            return SUCCESS;
        }
    }
    return NFOUND;
}

cga_status_t cga_ht_save_to_file(cga_hashtable_t *ht, FILE *outstream) {
    if((fcntl(fileno(outstream), F_GETFL) & O_ACCMODE) == O_RDONLY) {
        fprintf(stderr,"cga_ht_save_to_file permission denied. Have you opened the file in read mode?\n");
        return NWPERM;
    } 
    for(size_t i = 0; i < ht->size; i++) {
        if(ht->table[i] != NULL) {
            cga_ht_node_t *temp = ht->table[i];
            while(temp != NULL) {
                fprintf(outstream, "%lu %d\n", temp->key, temp->value);
                temp = temp->next;
            }
        }
    }
    return SUCCESS;
}

cga_status_t cga_ht_load_from_file(cga_hashtable_t *ht, FILE *instream) {
    if((fcntl(fileno(instream), F_GETFL) & O_ACCMODE) == O_WRONLY) {
        fprintf(stderr,"cga_ht_load_from_file permission denied. Have you opened the file in write mode?\n");
        return NRPERM;
    }
        
    unsigned long key;
    igraph_integer_t value;
    int line = 0;
    int r;
    do {
        r = fscanf(instream, "%lu %d\n", &key, &value);
        if(r == 2) {
            cga_ht_insert(ht, key, value);
            line++;
        }
        else if(r != EOF) {
            fprintf(stderr, "Error: line %u in wrong format!\n", line);
            return WRFORMAT;
        }
    } while(r != EOF);
    return SUCCESS;
}

size_t cga_ht_nelems(cga_hashtable_t *ht) {
    return ht->nelem;
}




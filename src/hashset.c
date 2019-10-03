#include <igraph/igraph.h>
#include <stdlib.h>
#include "hashset.h"
#include "hash.h"

typedef struct _hs_node {
    igraph_integer_t value;
    struct _hs_node *next;
} cga_hs_node_t;

struct _cga_hashset {
    size_t size;
    size_t nelem;
    cga_hs_node_t **set;
};

static cga_hs_node_t* cga_hs_node_constructor(igraph_integer_t value, cga_hs_node_t *next);
static void cga_hs_node_destructor(cga_hs_node_t *node);

static cga_hs_node_t* cga_hs_node_constructor(igraph_integer_t value, cga_hs_node_t *next) {
    cga_hs_node_t *hsn = NULL;
    if((hsn = (cga_hs_node_t*)malloc(sizeof(cga_hs_node_t))) == NULL)
        return NULL;
    hsn->value = value;
    hsn->next = next;
    return hsn;
}

static void cga_hs_node_destructor(cga_hs_node_t *node) {
    free(node);
}

cga_hashset_t* cga_hs_init(size_t size) {
    if(size == 0) return NULL;
    cga_hashset_t *hs = (cga_hashset_t*)malloc(sizeof(cga_hashset_t));
    if(hs == NULL)
        return NULL;
    hs->size = size;
    hs->nelem = 0;
    if((hs->set = (cga_hs_node_t**)malloc(size * sizeof(cga_hs_node_t*))) == NULL)
        return NULL;
    for(size_t i = 0; i < size; i++)
        hs->set[i] = NULL;
    return hs;
}

void cga_hs_destroy(cga_hashset_t *hs) {
    cga_hs_clear(hs);
    free(hs->set);
    free(hs);
}

void cga_hs_clear(cga_hashset_t *hs) {
    for(size_t i = 0; i < hs->size; i++) {
        while(hs->set[i] != NULL) {
            cga_hs_node_t *temp = hs->set[i];
            hs->set[i] = temp->next;
            cga_hs_node_destructor(temp);
        }
    }
}

cga_status_t cga_hs_insert(cga_hashset_t *hs, igraph_integer_t elem) {
    if(cga_hs_contains(hs, elem)) return DPLKTKEY;

    size_t index = (cga_hash(elem) % hs->size);
    cga_hs_node_t *node = cga_hs_node_constructor(elem, hs->set[index]);
    if(node == NULL )
        return NOMEM;
    hs->set[index] = node;
    hs->nelem = hs->nelem + 1;
    return SUCCESS;
}

int cga_hs_contains(cga_hashset_t *hs, igraph_integer_t elem) {
    size_t index = (cga_hash(elem) % hs->size);
    cga_hs_node_t *node = hs->set[index];
    while(node != NULL) {
        if(node->value == elem)
            return 1;
        node = node->next;
    }
    return 0;
}

cga_status_t cga_hs_delete(cga_hashset_t *hs, igraph_integer_t elem) {
    size_t index = (cga_hash(elem) % hs->size);
    cga_hs_node_t *current = hs->set[index];
    cga_hs_node_t *prev = NULL;
    while(current != NULL) {
        if(current->value != elem) {
            prev = current;
            current = current->next;
        }
        else {
            if(prev == NULL) {
                hs->set[index] = current->next;
            }
            else {
                prev->next = current->next;
            }
            cga_hs_node_destructor(current);
            hs->nelem = hs->nelem - 1;
            return SUCCESS;
        }
    }
    return NFOUND;
}

size_t cga_hs_nelems(cga_hashset_t *hs) {
    return hs->nelem;
}
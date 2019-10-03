#ifndef HASHSET_H_oddohcvozixcvpilkdsfapoi
#define HASHSET_H_oddohcvozixcvpilkdsfapoi

#include <igraph/igraph.h>
#include "status.h"

typedef struct _cga_hashset cga_hashset_t;

/**
 * This function created a cga_hashset_t object of the given size.
 * Every hashset object created by this function should be destroyed (ie. the memory allocated
 * for it should be freed) when it is not needed anymore with the function cga_hs_destroy().
 * 
 * Arguments:
 * size: The maximum size of the hashset
 * 
 * Returns a pointer to the newly created hashset object
 */
cga_hashset_t *cga_hs_init(size_t size);

/**
 * Destroys a hashset object.
 * All hashset object created by cga_hs_init() should be destroyed by this function.
 * 
 * Arguments:
 * hs: Pointer to the (previously initialized) hashset object to destroy
 */
void cga_hs_destroy(cga_hashset_t *hs);

/**
 * Deletes all the elements from the hashset.
 * This function deletes all the elements stored in the hashset, it does not free
 * the hashset object. For that, you have to call cga_hs_destroy()
 * 
 * Arguments:
 * hs: Pointer to the hashset object to clear
 */
void cga_hs_clear(cga_hashset_t *hs);

/**
 * Inserts an element in the hashset. Any duplicated element won't be inserted.
 * 
 * Arguments:
 * hs: Pointer to the hashset object
 * elem: The element to insert
 * 
 * Returns SUCCESS if the operation completed without errors, NOMEM if there's not enough memory,
 * DPLKTKEY if the given element is already in the hashset.
 */
cga_status_t cga_hs_insert(cga_hashset_t *hs, igraph_integer_t elem);

/**
 * Check whether the supplied element is contained in the hashset
 * 
 * Arguments:
 * hs: Pointer to the hashset object
 * elem: The element to look for
 * 
 * Returns 1 if the element is found, 0 otherwise
 */
int cga_hs_contains(cga_hashset_t *hs, igraph_integer_t elem);

/**
 * Deletes an element in the hashset. If the specified element is not found,
 * this function returns with an error.
 * 
 * Arguments:
 * hs: Pointer to the hashset object
 * elem: the element to delete
 * 
 * Returns SUCCESS if the operation completed without errors, NFOUND if the given element
 * is not in the hashset.
 */
cga_status_t cga_hs_delete(cga_hashset_t *hs, igraph_integer_t elem);

/**
 * Gives the number of elements stored in the hashset object
 * 
 * Arguments:
 * hs: Pointer to the hashset object
 * 
 * Returns the number of elements stored in the hashset object
 */
size_t cga_hs_nelems(cga_hashset_t *hs);

#endif
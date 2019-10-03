#ifndef HASHTABLE_H_soadifvhodkfjgfkjgpodfvjj
#define HASHTABLE_H_soadifvhodkfjgfkjgpodfvjj


#include "status.h"

typedef struct _cga_hashtable cga_hashtable_t;

/**
 * Creates a cga_hashtable_t object of the given size.
 * Every hashtable object created by this function shoudl be destroyed (ie. the memory allocated
 * for it should be freed) when it is not needed anymore with the function cga_ht_destroy()
 * 
 * Arguments:
 * size: The maximum size of the hashtable
 * 
 * Returns a pointer to the newly created hashtable object
 */
cga_hashtable_t* cga_ht_init(size_t size);

/**
 * Destroys a hashtable object
 * All hashtable objects created by cga_ht_init() should be destroyed by this function.
 * 
 * Arguments:
 * ht: Pointer to the (previously initialized) hashtable object to destroy
 */
void cga_ht_destroy(cga_hashtable_t *ht);

/**
 * Deletes all the <key, value> from the hashtable, it does not free the hashtable object.
 * For that you have to call cga_ht_destroy().
 * 
 * Arguments:
 * ht: Pointer to the hashtable object to clear
 */
void cga_ht_clear(cga_hashtable_t *ht);

/**
 * Inserts the assotiation <key, value> in the hashtable. Any duplicated key values won't be
 * inserted.
 * 
 * Arguments:
 * ht: Pointer to the hashtable object
 * key: The key to insert
 * value: The value associated with the key
 * 
 * Returns SUCCESS if the operation completed without errors, NOMEM if there's not enough memory,
 * DPLKTKEY if the given key is already in the hashtable.
 */
cga_status_t cga_ht_insert(cga_hashtable_t *ht, unsigned long key, igraph_integer_t value);

/**
 * Check wheter the supplied key is contained in the hashtable.
 * 
 * Arguments:
 * ht: Pointer to the hashtable object
 * key: The key to look for
 * 
 * Returns 1 if the key is found, 0 otherwise.
 */
int cga_ht_contains(cga_hashtable_t *ht, unsigned long key);

/**
 * Search the supplied key in the hashtable
 * 
 * Arguments:
 * ht: Pointer to the hashtable object
 * key: The key to look for
 * 
 * Returns the value to which the specified key is mapped, NULL otherwise.
 */
igraph_integer_t* cga_ht_search(cga_hashtable_t *ht, unsigned long key);

/**
 * Deletes the association <key, value> in the hashtable. If the specified key is not found,
 * this function returns with an error.
 * 
 * Arguments:
 * ht: Pointer to the hashtable object
 * key: The key to delete
 * 
 * Returns SUCCESS if the operation completed without errors, NFOUND if the given key is not
 * in the hashtable.
 */
cga_status_t cga_ht_delete(cga_hashtable_t *ht, unsigned long key);

/**
 * Gives the number of keys stored in the hashtable object
 * 
 * Arguments:
 * ht: Pointer to the hashtable object
 * 
 * Returns the number of elements stored in the hashtable object
 */
size_t cga_ht_nelems(cga_hashtable_t *ht);

/**
 * Saves all the association <key, value> of the hashtable in the specified file.
 * The format of the resulting file correspons to:
 *      key value\n
 *       ........
 *      key value\n
 * The file must be opened with the write privilege, or an error message will be displayed
 * in stderr warning the user.
 * 
 * Arguments:
 * ht: Pointer to the hashtable object
 * outstream: FILE pointer opened with the write privilege
 * 
 * Returns SUCCESS if the operation completed without errors, NWPERM if the given
 * file descriptor has no write privilege.
 */
cga_status_t cga_ht_save_to_file(cga_hashtable_t *ht, FILE *outstream);

/**
 * Loads all the association <key, value> from a given file into the hashtable.
 * The file must respect this format:
 * 
 *      key value\n
 *       .......
 *      key value\n
 * 
 * If there is a syntax error the hashtable will be partially loaded with the
 * <key, value> parsed before the error occurred, and an error string will be printed in
 * stderr showing the error and the line that caused it.
 * The file must be opened with the read privilege.
 * 
 * Arguments:
 * ht: Pointer to the hashtable object
 * instream: FILE pointer opened with the read privilege
 * 
 * Returns SUCCESS if the operation completed without errors, NRPERM if the given file pointer has
 * no read privilege, WRFORMAT if a syntax error occurred.
 */
cga_status_t cga_ht_load_from_file(cga_hashtable_t *ht, FILE *instream);

#endif
#include <stdlib.h>
#include "hash.h"
#define FNV_PRIME_64 1099511628211
#define FNV_OFFSET_64 14695981039346656037U

/** 
 * Start with an initial hash value of FNV offset basis. 
 * For each byte in the input, multiply hash by the FNV prime, then XOR it with the byte from the input.
 */
unsigned long cga_hash(unsigned long key) {
    unsigned long hash = FNV_OFFSET_64;
    for (size_t i = 0; i < sizeof(key); ++i) {
        // Convert to unsigned char* because a char is 1 byte in size.
        // That is guaranteed by the standard.
        unsigned char byte = *((unsigned char *)&key + i);
        hash = hash ^ byte;
        hash = hash * FNV_PRIME_64;
    }
    return hash;
}
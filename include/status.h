#ifndef STATUS_H_oacovzhxcovuhzcvxkasdofa
#define STATUS_H_oacovzhxcovuhzcvxkasdofa


/**
 * Some functions which can fail return a single integer error code.
 * The types of codes and their meaning are:
 * SUCCESS: nothing to report
 * NOMEM: there's no memory to do a memory allocation
 * DPLKTKEY: there's a duplicate key in data structure
 * NFOUND: the element is missing
 * WRFORMAT: wrong input format
 * NWPERM: no write permission
 * NRPERM: no read permission
 */
typedef enum _status {
    SUCCESS, NOMEM, DPLKTKEY, NFOUND, WRFORMAT, NWPERM, NRPERM
} cga_status_t;

#endif
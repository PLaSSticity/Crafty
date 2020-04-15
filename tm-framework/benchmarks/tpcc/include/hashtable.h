#ifndef TPCC_HASHTABLE_H
#define TPCC_HASHTABLE_H 1


#include "list.h"
#include "pair.h"
#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif


enum hashtable_config {
    HASHTABLE_DEFAULT_RESIZE_RATIO  = 3,
    HASHTABLE_DEFAULT_GROWTH_FACTOR = 3
};

typedef struct hashtable {
    list_t** buckets;
    long numBucket;
#ifdef HASHTABLE_SIZE_FIELD
    long size;
#endif
    ulong_t (*hash)(const void*);
    long (*comparePairs)(const pair_t*, const pair_t*);
    long resizeRatio;
    long growthFactor;
    /* comparePairs should return <0 if before, 0 if equal, >0 if after */
} hashtable_t;


typedef struct hashtable_iter {
    long bucket;
    list_iter_t it;
} hashtable_iter_t;


void hashtable_iter_reset (hashtable_iter_t* itPtr, hashtable_t* hashtablePtr);

bool_t hashtable_iter_hasNext (hashtable_iter_t* itPtr, hashtable_t* hashtablePtr);

void* hashtable_iter_next (hashtable_iter_t* itPtr, hashtable_t* hashtablePtr);

hashtable_t* hashtable_alloc (long initNumBucket,
                 ulong_t (*hash)(const void*),
                 long (*comparePairs)(const pair_t*, const pair_t*),
                 long resizeRatio,
                 long growthFactor);

void hashtable_free (hashtable_t* hashtablePtr);

bool_t hashtable_isEmpty (hashtable_t* hashtablePtr);

long hashtable_getSize (hashtable_t* hashtablePtr);

bool_t hashtable_containsKey (hashtable_t* hashtablePtr, void* keyPtr);

void* hashtable_find (hashtable_t* hashtablePtr, void* keyPtr);

bool_t hashtable_insert (hashtable_t* hashtablePtr, void* keyPtr, void* dataPtr);

bool_t hashtable_remove (hashtable_t* hashtablePtr, void* keyPtr);


#ifdef __cplusplus
}
#endif


#endif

#include <stdlib.h>
#include "hashtable.h"
#include "list.h"
#include "pair.h"
#include "types.h"

void hashtable_iter_reset (hashtable_iter_t* itPtr, hashtable_t* hashtablePtr) {
    itPtr->bucket = 0;
    list_iter_reset(&(itPtr->it), hashtablePtr->buckets[0]);
}

bool_t hashtable_iter_hasNext (hashtable_iter_t* itPtr, hashtable_t* hashtablePtr) {
    long bucket;
    long numBucket = hashtablePtr->numBucket;
    list_t** buckets = hashtablePtr->buckets;
    list_iter_t it = itPtr->it;

    for (bucket = itPtr->bucket; bucket < numBucket; /* inside body */) {
        list_t* chainPtr = buckets[bucket];
        if (list_iter_hasNext(&it, chainPtr)) {
            return TRUE;
        }
        /* May use dummy bucket; see allocBuckets() */
        list_iter_reset(&it, buckets[++bucket]);
    }

    return FALSE;
}

void* hashtable_iter_next (hashtable_iter_t* itPtr, hashtable_t* hashtablePtr) {
    long bucket;
    long numBucket = hashtablePtr->numBucket;
    list_t** buckets = hashtablePtr->buckets;
    list_iter_t it = itPtr->it;
    void* dataPtr = NULL;

    for (bucket = itPtr->bucket; bucket < numBucket; /* inside body */) {
        list_t* chainPtr = hashtablePtr->buckets[bucket];
        if (list_iter_hasNext(&it, chainPtr)) {
            pair_t* pairPtr = (pair_t*)list_iter_next(&it, chainPtr);
            dataPtr = pairPtr->secondPtr;
            break;
        }
        /* May use dummy bucket; see allocBuckets() */
        list_iter_reset(&it, buckets[++bucket]);
    }

    itPtr->bucket = bucket;
    itPtr->it = it;

    return dataPtr;
}

static list_t** allocBuckets (long numBucket, long (*comparePairs)(const pair_t*, const pair_t*)) {
    long i;
    list_t** buckets;

    /* Allocate bucket: extra bucket is dummy for easier iterator code */
    buckets = (list_t**)malloc((numBucket + 1) * sizeof(list_t*));
    if (buckets == NULL) {
        return NULL;
    }

    for (i = 0; i < (numBucket + 1); i++) {
        list_t* chainPtr =
            list_alloc((long (*)(const void*, const void*))comparePairs);
        if (chainPtr == NULL) {
            while (--i >= 0) {
                list_free(buckets[i]);
            }
            return NULL;
        }
        buckets[i] = chainPtr;
    }

    return buckets;
}

hashtable_t* hashtable_alloc (long initNumBucket,
                 ulong_t (*hash)(const void*),
                 long (*comparePairs)(const pair_t*, const pair_t*),
                 long resizeRatio,
                 long growthFactor) {
    hashtable_t* hashtablePtr;

    hashtablePtr = (hashtable_t*)malloc(sizeof(hashtable_t));
    if (hashtablePtr == NULL) {
        return NULL;
    }

    hashtablePtr->buckets = allocBuckets(initNumBucket, comparePairs);
    if (hashtablePtr->buckets == NULL) {
        free(hashtablePtr);
        return NULL;
    }

    hashtablePtr->numBucket = initNumBucket;
    hashtablePtr->hash = hash;
    hashtablePtr->comparePairs = comparePairs;
    hashtablePtr->resizeRatio = ((resizeRatio < 0) ?
                                  HASHTABLE_DEFAULT_RESIZE_RATIO : resizeRatio);
    hashtablePtr->growthFactor = ((growthFactor < 0) ?
                                  HASHTABLE_DEFAULT_GROWTH_FACTOR : growthFactor);

    return hashtablePtr;
}

static void freeBuckets (list_t** buckets, long numBucket) {
    long i;

    for (i = 0; i < numBucket; i++) {
        list_free(buckets[i]);
    }

    free(buckets);
}

void hashtable_free (hashtable_t* hashtablePtr) {
    freeBuckets(hashtablePtr->buckets, hashtablePtr->numBucket);
    free(hashtablePtr);
}

bool_t hashtable_isEmpty (hashtable_t* hashtablePtr) {
    long i;

    for (i = 0; i < hashtablePtr->numBucket; i++) {
        if (!list_isEmpty(hashtablePtr->buckets[i])) {
            return FALSE;
        }
    }

    return TRUE;
}

long hashtable_getSize (hashtable_t* hashtablePtr) {
    long i;
    long size = 0;

    for (i = 0; i < hashtablePtr->numBucket; i++) {
        size += list_getSize(hashtablePtr->buckets[i]);
    }

    return size;
}

bool_t hashtable_containsKey (hashtable_t* hashtablePtr, void* keyPtr) {
    long i = hashtablePtr->hash(keyPtr) % hashtablePtr->numBucket;
    pair_t* pairPtr;
    pair_t findPair;

    findPair.firstPtr = keyPtr;
    pairPtr = (pair_t*)list_find(hashtablePtr->buckets[i], &findPair);

    return ((pairPtr != NULL) ? TRUE : FALSE);
}

void* hashtable_find (hashtable_t* hashtablePtr, void* keyPtr) {
    long i = hashtablePtr->hash(keyPtr) % hashtablePtr->numBucket;
    pair_t* pairPtr;
    pair_t findPair;

    findPair.firstPtr = keyPtr;
    pairPtr = (pair_t*)list_find(hashtablePtr->buckets[i], &findPair);
    if (pairPtr == NULL) {
        return NULL;
    }

    return pairPtr->secondPtr;
}


#if defined(HASHTABLE_RESIZABLE)
/* =============================================================================
 * rehash
 * =============================================================================
 */
static list_t**
rehash (hashtable_t* hashtablePtr)
{
    list_t** oldBuckets = hashtablePtr->buckets;
    long oldNumBucket = hashtablePtr->numBucket;
    long newNumBucket = hashtablePtr->growthFactor * oldNumBucket;
    list_t** newBuckets;
    long i;

    newBuckets = allocBuckets(newNumBucket, hashtablePtr->comparePairs);
    if (newBuckets == NULL) {
        return NULL;
    }

    for (i = 0; i < oldNumBucket; i++) {
        list_t* chainPtr = oldBuckets[i];
        list_iter_t it;
        list_iter_reset(&it, chainPtr);
        while (list_iter_hasNext(&it, chainPtr)) {
            pair_t* transferPtr = (pair_t*)list_iter_next(&it, chainPtr);
            long j = hashtablePtr->hash(transferPtr->firstPtr) % newNumBucket;
            if (list_insert(newBuckets[j], (void*)transferPtr) == FALSE) {
                return NULL;
            }
        }
    }

    return newBuckets;
}
#endif /* HASHTABLE_RESIZABLE */


bool_t hashtable_insert (hashtable_t* hashtablePtr, void* keyPtr, void* dataPtr) {
    long numBucket = hashtablePtr->numBucket;
    long i = hashtablePtr->hash(keyPtr) % numBucket;
#if defined(HASHTABLE_RESIZABLE)
    long newSize;
#endif

    pair_t findPair;
    findPair.firstPtr = keyPtr;
    pair_t* pairPtr = (pair_t*)list_find(hashtablePtr->buckets[i], &findPair);
    if (pairPtr != NULL) {
        return FALSE;
    }

    pair_t* insertPtr = pair_alloc(keyPtr, dataPtr);
    if (insertPtr == NULL) {
        return FALSE;
    }

#ifdef HASHTABLE_RESIZABLE
    newSize = hashtable_getSize(hashtablePtr) + 1;
    /* Increase number of buckets to maintain size ratio */
    if (newSize >= (numBucket * hashtablePtr->resizeRatio)) {
        list_t** newBuckets = rehash(hashtablePtr);
        if (newBuckets == NULL) {
            return FALSE;
        }
        freeBuckets(hashtablePtr->buckets, numBucket);
        numBucket *= hashtablePtr->growthFactor;
        hashtablePtr->buckets = newBuckets;
        hashtablePtr->numBucket = numBucket;
        i = hashtablePtr->hash(keyPtr) % numBucket;

    }
#endif

    /* Add new entry  */
    if (list_insert(hashtablePtr->buckets[i], insertPtr) == FALSE) {
        pair_free(insertPtr);
        return FALSE;
    }

    return TRUE;
}

bool_t hashtable_remove (hashtable_t* hashtablePtr, void* keyPtr) {
    long numBucket = hashtablePtr->numBucket;
    long i = hashtablePtr->hash(keyPtr) % numBucket;
    list_t* chainPtr = hashtablePtr->buckets[i];
    pair_t* pairPtr;
    pair_t removePair;

    removePair.firstPtr = keyPtr;
    pairPtr = (pair_t*)list_find(chainPtr, &removePair);
    if (pairPtr == NULL) {
        return FALSE;
    }

    bool_t status = list_remove(chainPtr, &removePair);
    pair_free(pairPtr);


    return TRUE;
}

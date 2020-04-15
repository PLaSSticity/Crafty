#include <stdlib.h>
#include "memory.h"
#include "pair.h"

pair_t* pair_alloc (void* firstPtr, void* secondPtr) {
    pair_t* pairPtr;

    pairPtr = (pair_t*)malloc(sizeof(pair_t));
    if (pairPtr != NULL) {
        pairPtr->firstPtr = firstPtr;
        pairPtr->secondPtr = secondPtr;
    }

    return pairPtr;
}

void pair_free (pair_t* pairPtr) {
    free(pairPtr);
}

void pair_swap (pair_t* pairPtr) {
    void* tmpPtr = pairPtr->firstPtr;
    pairPtr->firstPtr = pairPtr->secondPtr;
    pairPtr->secondPtr = tmpPtr;
}

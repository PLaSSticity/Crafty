#ifndef TPCC_MEMORY_H
#define TPCC_MEMORY_H 1

#include <stddef.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif


enum {
    DEFAULT_INIT_BLOCK_CAPACITY = 16,
    DEFAULT_BLOCK_GROWTH_FACTOR = 2,
};

typedef struct memory memory_t;


bool_t memory_init (long numThread, size_t initBlockCapacity, long blockGrowthFactor);

void memory_destroy ();

void* memory_get (long threadId, size_t numByte);


#ifdef __cplusplus
}
#endif


#endif

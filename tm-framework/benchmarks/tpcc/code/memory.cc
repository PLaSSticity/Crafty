#include <stdlib.h>
#include "memory.h"
#include "types.h"

/* We want to use enum bool_t */
#ifdef FALSE
#  undef FALSE
#endif
#ifdef TRUE
#  undef TRUE
#endif


#define PADDING_SIZE 8
typedef struct block {
    long padding1[PADDING_SIZE];
    size_t size;
    size_t capacity;
    char* contents;
    struct block* nextPtr;
    long padding2[PADDING_SIZE];
} block_t;

typedef struct pool {
    block_t* blocksPtr;
    size_t nextCapacity;
    size_t initBlockCapacity;
    long blockGrowthFactor;
} pool_t;

struct memory {
    pool_t** pools;
    long numThread;
};

memory_t* global_memoryPtr = 0;


static block_t* allocBlock (size_t capacity) {
    block_t* blockPtr;

    blockPtr = (block_t*)malloc(sizeof(block_t));
    if (blockPtr == NULL) {
        return NULL;
    }

    blockPtr->size = 0;
    blockPtr->capacity = capacity;
    blockPtr->contents = (char*)malloc(capacity / sizeof(char) + 1);
    if (blockPtr->contents == NULL) {
        return NULL;
    }
    blockPtr->nextPtr = NULL;

    return blockPtr;
}

static void freeBlock (block_t* blockPtr) {
    free(blockPtr->contents);
    free(blockPtr);
}

static pool_t* allocPool (size_t initBlockCapacity, long blockGrowthFactor) {
    pool_t* poolPtr;

    poolPtr = (pool_t*)malloc(sizeof(pool_t));
    if (poolPtr == NULL) {
        return NULL;
    }

    poolPtr->initBlockCapacity =
        (initBlockCapacity > 0) ? initBlockCapacity : DEFAULT_INIT_BLOCK_CAPACITY;
    poolPtr->blockGrowthFactor =
        (blockGrowthFactor > 0) ? blockGrowthFactor : DEFAULT_BLOCK_GROWTH_FACTOR;

    poolPtr->blocksPtr = allocBlock(poolPtr->initBlockCapacity);
    if (poolPtr->blocksPtr == NULL) {
        return NULL;
    }

    poolPtr->nextCapacity = poolPtr->initBlockCapacity *
                            poolPtr->blockGrowthFactor;

    return poolPtr;
}

static void freeBlocks (block_t* blockPtr) {
    if (blockPtr != NULL) {
        freeBlocks(blockPtr->nextPtr);
        freeBlock(blockPtr);
    }
}

static void freePool (pool_t* poolPtr) {
    freeBlocks(poolPtr->blocksPtr);
    free(poolPtr);
}

bool_t memory_init (long numThread, size_t initBlockCapacity, long blockGrowthFactor)  {
    long i;

    global_memoryPtr = (memory_t*)malloc(sizeof(memory_t));
    if (global_memoryPtr == NULL) {
        return FALSE;
    }

    global_memoryPtr->pools = (pool_t**)malloc(numThread * sizeof(pool_t*));
    if (global_memoryPtr->pools == NULL) {
        return FALSE;
    }

    for (i = 0; i < numThread; i++) {
        global_memoryPtr->pools[i] = allocPool(initBlockCapacity, blockGrowthFactor);
        if (global_memoryPtr->pools[i] == NULL) {
            return FALSE;
        }
    }

    global_memoryPtr->numThread = numThread;

    return TRUE;
}

void memory_destroy (void) {
    long i;
    long numThread = global_memoryPtr->numThread;

    for (i = 0; i < numThread; i++) {
        freePool(global_memoryPtr->pools[i]);
    }
    free(global_memoryPtr->pools);
    free(global_memoryPtr);
}

static block_t* addBlockToPool (pool_t* poolPtr, long numByte) {
    block_t* blockPtr;
    size_t capacity = poolPtr->nextCapacity;
    long blockGrowthFactor = poolPtr->blockGrowthFactor;

    if ((size_t)numByte > capacity) {
        capacity = numByte * blockGrowthFactor;
    }

    blockPtr = allocBlock(capacity);
    if (blockPtr == NULL) {
        return NULL;
    }

    blockPtr->nextPtr = poolPtr->blocksPtr;
    poolPtr->blocksPtr = blockPtr;
    poolPtr->nextCapacity = capacity * blockGrowthFactor;

    return blockPtr;
}

static void* getMemoryFromBlock (block_t* blockPtr, size_t numByte) {
    size_t size = blockPtr->size;
    size_t capacity = blockPtr->capacity;

    blockPtr->size += numByte;

    return (void*)&blockPtr->contents[size];
}

static void* getMemoryFromPool (pool_t* poolPtr, size_t numByte) {
    block_t* blockPtr = poolPtr->blocksPtr;

    if ((blockPtr->size + numByte) > blockPtr->capacity) {
        blockPtr = addBlockToPool(poolPtr, numByte);
        if (blockPtr == NULL) {
            return NULL;
        }
    }

    return getMemoryFromBlock(blockPtr, numByte);
}

void* memory_get (long threadId, size_t numByte) {
    pool_t* poolPtr;
    void* dataPtr;
    size_t addr;
    size_t misalignment;

    poolPtr = global_memoryPtr->pools[threadId];
    dataPtr = getMemoryFromPool(poolPtr, (numByte + 7)); /* +7 for alignment */

    /* Fix alignment for 64 bit */
    addr = (size_t)dataPtr;
    misalignment = addr % 8;
    if (misalignment) {
        addr += (8 - misalignment);
        dataPtr = (void*)addr;
    }

    return dataPtr;
}

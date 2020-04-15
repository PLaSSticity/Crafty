#ifndef EXTRA_TYPES_H
#define EXTRA_TYPES_H

#include "nh_types.h"

typedef struct NVCheckpoint_ NVCheckpoint_s;
typedef struct NVMHTM_mem_ NVMHTM_mem_s;
// typedef struct NVLogEntry_ NVLogEntry_s;

typedef struct NVLogEntry_
{
	GRANULE_TYPE *addr;
	GRANULE_TYPE value;
} __attribute__((packed)) NVLogEntry_s; // check this 

typedef struct NVLog_
{
    char pad1[2*CACHE_LINE_SIZE];
	// these can be accessed concurrently non-transactionally
	int start, end;
	int start_tx;
	int end_last_tx;
	int tid;

	char pad2[2*CACHE_LINE_SIZE];
	
    int size_of_log;
    
	NVLogEntry_s *ptr; // TODO
} __attribute__((packed)) NVLog_s;

typedef struct NVLogLocation_
{
	int tid;
	int ptr;
    ts_s ts;
} __attribute__((packed)) NVLogLocation_s;

typedef struct NVLogLocal_
{
	int start;
	int end;
	int counter;
    int size_of_log;
} __attribute__((packed)) NVLogLocal_s;
	
#endif /* EXTRA_TYPES_H */

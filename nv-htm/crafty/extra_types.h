#ifndef EXTRA_TYPES_H
#define EXTRA_TYPES_H

#include "nh_types.h"
/*
#define UNDO_ENTRIES (NVMHTM_LOG_SIZE / sizeof(Crafty_undo_log_entry_s))

typedef struct Crafty_undo_log_entry_
{
	GRANULE_TYPE *addr;
	GRANULE_TYPE value;
} Crafty_undo_log_entry_s;

typedef struct Crafty_undo_log_
{
	Crafty_undo_log_entry_s entries[UNDO_ENTRIES];
} CL_ALIGN Crafty_undo_log_s;
*/

#define LOG_ENTRIES (NVMHTM_LOG_SIZE / sizeof(Crafty_log_entry_s))

typedef struct Crafty_log_entry_
{
	GRANULE_TYPE *addr;
	GRANULE_TYPE oldValue;
} Crafty_log_entry_s;

typedef struct Crafty_log_
{
	Crafty_log_entry_s entries[LOG_ENTRIES];
} CL_ALIGN Crafty_log_s;

struct Crafty_abort_stats {
    int total;
    int explicit_;
    int retry;
    int conflict;
    int capacity;
    int zero;
};

#endif /* EXTRA_TYPES_H */

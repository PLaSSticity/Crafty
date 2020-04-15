#ifndef PHTM_H
#define PHTM_H

#include "nh.h"

#ifdef __cplusplus
extern "C"
{
#endif

// TODO: move TM.h macros to here

// size in bytes but markers are per cache line
PHTM_marker_pool_s *PHTM_create_marker_pool(void*, size_t); // DO NOT CALL
void PHTM_init(int); // on the bootstrap of the application

PHTM_log_s *PHTM_create_log();  // DO NOT CALL
void PHTM_thr_init(int tid);

// return -2 on success, -1 on other transaction is using, OR
// the log entry index if already accessed
int PHTM_mark_addr(void* addr, int tid, int log_rec);

int PHTM_log_cache_line(int tid, void* addr, GRANULE_TYPE val);

void PHTM_rem__mark(void* addr);
void PHTM_log_clear(); // remove all markers in the log and set log size to 0
// int PHTM_log_size(int tid);
#define PHTM_log_size(tid) ({ phtm_log->size; })

    // TODO: move to macro
#define PHTM_instrument_read(tid, addr) ({ \
	if (!HTM_test()) { \
		/* in SGL search first int the logs */ \
	} \
    *((GRANULE_TYPE*) addr); \
})

// does it works with pointers and floats?
#define PHTM_instrument_write(tid, addr, val) ({ \
	if (HTM_test()) { \
        PHTM_log_cache_line(tid, addr, val); \
    } else { \
        /* TODO: postpone write */ \
	    PHTM_log_cache_line(tid, addr, val); \
	} \
}) \

    // GRANULE_TYPE PHTM_instrument_read(int tid, void* addr);
    // void PHTM_instrument_write(int tid, void* addr, GRANULE_TYPE val);

#ifdef __cplusplus
}
#endif

#endif /* PHTM_H */


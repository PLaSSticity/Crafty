#ifndef EXTRA_GLOBALS_H
#define EXTRA_GLOBALS_H

#include "extra_types.h"

#include <setjmp.h>

// L2 size divided by undo and redo log footprint for each program write
//#define MAX_UNIQUE_WRITES ((256 << 10) / (sizeof(Crafty_undo_log_entry_s) + sizeof(GRANULE_TYPE)))
//#define REDO_ENTRIES MAX_UNIQUE_WRITES

// ####################################################
// ### LOG VARIABLES ##################################
// ####################################################
// thread local
#ifdef CRAFTY_VALIDATE
extern __thread int crafty_is_logging;
extern __thread int crafty_validation_reattempts_left;
#endif
#ifdef ROLLOVER_BITS
extern __thread uint_fast8_t crafty_rollover_bit;
#endif
extern __thread Crafty_log_s *crafty_log;
extern __thread Crafty_log_entry_s* crafty_log_tx_start;
extern __thread Crafty_log_entry_s* crafty_log_curr_pos;
extern __thread Crafty_log_entry_s* crafty_log_upper_bound;
extern __thread jmp_buf setjmp_buf;
#ifdef NVM_LATENCY_RDTSCP
extern __thread unsigned long long clwb_end_time;
#ifndef NVM_WAIT_CYCLES
#error Number of cycles to emulate the wait is undefined
#endif
#else
extern __thread struct timespec clwb_end_time;
#endif
// TODO: Keep only values in the redo log?
extern __thread Crafty_log_entry_s redo_log[];
extern __thread Crafty_log_entry_s* redo_log_pos;
extern __thread int crafty_SGL_writes_seen;
extern __thread int crafty_SGL_writes_max;
#ifdef CRAFTY_REDO
extern int last_tid;
extern __thread u_int64_t last_log_time;
extern u_int64_t last_copy_time;
#endif

#ifdef CRAFTY_REDO
extern __thread void * crafty_free_log[];
extern __thread int crafty_free_last;
#endif


extern uint64_t recovery_ts_lower_bound;
extern __thread uint64_t last_commited_ts;
extern __thread uint64_t last_tx_ts;
extern uint64_t *last_tx_ts_arr[MAX_NB_THREADS];
extern Crafty_log_entry_s **log_curr_pos_arr[MAX_NB_THREADS];
extern Crafty_log_entry_s **log_tx_start_arr[MAX_NB_THREADS];
extern Crafty_log_s *log_arr[MAX_NB_THREADS];
#ifdef ROLLOVER_BITS
extern uint_fast8_t *rollover_bit_arr[MAX_NB_THREADS];
#endif

extern __thread void *crafty_alloc_log[];
extern __thread int crafty_alloc_index;
extern __thread int crafty_alloc_last;

extern __thread int crafty_in_SGL;


// Stats that we want to collect even with no-stats runs
extern __thread long crafty_aborts_in_redo;
extern __thread long crafty_aborts_in_SGL;
extern long crafty_global_aborts_in_redo;
extern long crafty_global_aborts_in_SGL;

#ifdef CRAFTY_STATS

extern __thread size_t crafty_alloc_high_mark;
extern __thread size_t crafty_free_high_mark;
extern size_t crafty_global_alloc_high_mark;
extern size_t crafty_global_free_high_mark;
// Number of calls to crafty_malloc
extern __thread size_t crafty_allocs_logged;
extern size_t crafty_global_allocs_logged;

extern __thread long crafty_write_count;
extern long crafty_global_write_count;
extern __thread size_t crafty_currenttx_write_count;
// Indices and write counts: 0, 1, 2, 3, 4-5, 6-10, 11+
extern __thread size_t crafty_write_histogram[7];
extern size_t crafty_global_write_histogram[7];

extern __thread struct Crafty_abort_stats crafty_aborts;
extern struct Crafty_abort_stats crafty_total_aborts;
extern int crafty_thread_count; // Counted at exits
extern __thread long crafty_aborts_in_logging;
extern __thread long crafty_aborts_in_validating;
extern long crafty_global_aborts_in_logging;
extern long crafty_global_aborts_in_validating;


extern __thread long crafty_SGL_in_logging;
extern __thread long crafty_SGL_in_validating;
extern long crafty_global_SGL_in_logging;
extern long crafty_global_SGL_in_validating;

extern __thread long crafty_valfail;
extern long crafty_global_valfail;

extern __thread long crafty_replayfail;
extern long crafty_global_replayfail;

extern __thread long crafty_singlewrite_SGL;
extern long crafty_global_singlewrite_SGL;
extern __thread long crafty_multiwrite_SGL;
extern long crafty_global_multiwrite_SGL;

extern __thread long crafty_log_sections;
extern long crafty_global_log_sections;
#endif

extern __thread long crafty_validate_success;
extern long crafty_global_validate_success;
extern __thread long crafty_replay_success;
extern long crafty_global_replay_success;
extern __thread long crafty_readonly_success;
extern long crafty_global_readonly_success;

extern unsigned long crafty_sgl_abort_conflict;
extern unsigned long crafty_sgl_abort_capacity;
extern unsigned long crafty_sgl_abort_zero;
extern unsigned long crafty_sgl_abort_explicit;

extern __thread unsigned long crafty_redo_abort_conflict;
extern __thread unsigned long crafty_redo_abort_capacity;
extern __thread unsigned long crafty_redo_abort_zero;
extern __thread unsigned long crafty_redo_abort_explicit;
extern unsigned long crafty_global_redo_abort_conflict;
extern unsigned long crafty_global_redo_abort_capacity;
extern unsigned long crafty_global_redo_abort_zero;
extern unsigned long crafty_global_redo_abort_explicit;

// ####################################################

#endif /* EXTRA_GLOBALS_H */

#include "extra_globals.h"
#include "nh_sol.h"

#ifdef CRAFTY_VALIDATE
__thread int crafty_is_logging;
__thread int crafty_validation_reattempts_left;
#endif
#ifdef ROLLOVER_BITS
__thread uint_fast8_t crafty_rollover_bit = 0;
#endif
__thread Crafty_log_s* crafty_log;
__thread Crafty_log_entry_s* crafty_log_tx_start;
__thread Crafty_log_entry_s* crafty_log_curr_pos;
__thread Crafty_log_entry_s* crafty_log_upper_bound;
__thread jmp_buf setjmp_buf;
#ifdef NVM_LATENCY_RDTSCP
__thread unsigned long long clwb_end_time;
#else
__thread struct timespec clwb_end_time;
#endif
__thread void *crafty_alloc_log[CRAFTY_ALLOC_LOG_SIZE];
__thread int crafty_alloc_index;
__thread int crafty_alloc_last;
__thread Crafty_log_entry_s redo_log[CRAFTY_REDO_LOG_SIZE];
__thread Crafty_log_entry_s* redo_log_pos;
__thread int crafty_SGL_writes_seen;
__thread int crafty_SGL_writes_max;
#ifdef CRAFTY_REDO
int last_tid = 0;
__thread u_int64_t last_log_time;
u_int64_t last_copy_time;
#endif

#ifdef CRAFTY_REDO
__thread void * crafty_free_log[CRAFTY_ALLOC_LOG_SIZE];
__thread int crafty_free_last;
#endif

uint64_t recovery_ts_lower_bound = 0;
__thread uint64_t last_commited_ts = 0;
__thread uint64_t last_tx_ts = 0;
uint64_t *last_tx_ts_arr[MAX_NB_THREADS] = {nullptr};
Crafty_log_entry_s **log_curr_pos_arr[MAX_NB_THREADS] = {nullptr};
Crafty_log_entry_s **log_tx_start_arr[MAX_NB_THREADS] = {nullptr};
Crafty_log_s *log_arr[MAX_NB_THREADS] = {nullptr};
#ifdef ROLLOVER_BITS
uint_fast8_t *rollover_bit_arr[MAX_NB_THREADS] = {nullptr};
#endif

__thread int crafty_in_SGL = 0;

__thread long crafty_aborts_in_redo;
__thread long crafty_aborts_in_SGL;
long crafty_global_aborts_in_redo = 0;
long crafty_global_aborts_in_SGL = 0;

#ifdef CRAFTY_STATS

__thread size_t crafty_alloc_high_mark;
__thread size_t crafty_free_high_mark;
size_t crafty_global_alloc_high_mark = 0;
size_t crafty_global_free_high_mark = 0;

__thread size_t crafty_allocs_logged;
size_t crafty_global_allocs_logged = 0;

__thread struct Crafty_abort_stats crafty_aborts;
struct Crafty_abort_stats crafty_total_aborts = {0};
int crafty_thread_count;

__thread long crafty_aborts_in_logging;
__thread long crafty_aborts_in_validating;
long crafty_global_aborts_in_logging = 0;
long crafty_global_aborts_in_validating = 0;

__thread long crafty_SGL_in_logging;
__thread long crafty_SGL_in_validating;
long crafty_global_SGL_in_logging = 0;
long crafty_global_SGL_in_validating = 0;

__thread size_t crafty_currenttx_write_count;
__thread long crafty_write_count;
long crafty_global_write_count = 0;
__thread size_t crafty_write_histogram[7];
size_t crafty_global_write_histogram[7] = {0};

__thread long crafty_valfail;
long crafty_global_valfail = 0;

__thread long crafty_replayfail;
long crafty_global_replayfail = 0;

__thread long crafty_singlewrite_SGL;
long crafty_global_singlewrite_SGL = 0;
__thread long crafty_multiwrite_SGL;
long crafty_global_multiwrite_SGL = 0;

__thread long crafty_log_sections;
long crafty_global_log_sections = 0;
#endif

__thread long crafty_validate_success;
long crafty_global_validate_success = 0;
__thread long crafty_replay_success;
long crafty_global_replay_success = 0;
__thread long crafty_readonly_success;
long crafty_global_readonly_success = 0;

unsigned long crafty_sgl_abort_conflict = 0;
unsigned long crafty_sgl_abort_capacity = 0;
unsigned long crafty_sgl_abort_zero = 0;
unsigned long crafty_sgl_abort_explicit = 0;

__thread unsigned long crafty_redo_abort_conflict = 0;
__thread unsigned long crafty_redo_abort_capacity = 0;
__thread unsigned long crafty_redo_abort_zero = 0;
__thread unsigned long crafty_redo_abort_explicit = 0;
unsigned long crafty_global_redo_abort_conflict = 0;
unsigned long crafty_global_redo_abort_capacity = 0;
unsigned long crafty_global_redo_abort_zero = 0;
unsigned long crafty_global_redo_abort_explicit = 0;
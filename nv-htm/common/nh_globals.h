#ifndef NH_GLOBALS_H
#define NH_GLOBALS_H

#include "nh_types.h"
#include <semaphore.h>

extern ts_s *NH_before_ts;

// ####################################################
// ### STATE VARIABLES ################################
// ####################################################
// global
extern CL_ALIGN long global_flushed_ts;
// thread local
// ####################################################

// ####################################################
// ### MISC VARIABLES #################################
// ####################################################
// global
extern CL_ALIGN int TM_nb_threads;
extern volatile int *NH_checkpointer_state;
extern sem_t *NH_chkp_sem;
//extern CL_ALIGN int MAX_PHYS_THRS;
//extern CL_ALIGN long long CPU_MAX_FREQ;
//extern CL_ALIGN int SPINS_PER_100NS;
// thread local
extern __thread CL_ALIGN int TM_tid_var;
// ####################################################

// ####################################################
// ### PERFORMANCE COUNTERS ###########################
// ####################################################
// global
extern ts_s NH_ts_init;
extern CL_ALIGN int NH_count_wraps;
extern CL_ALIGN ts_s NH_time_validate_total;
extern CL_ALIGN double NH_tx_time_total;
extern CL_ALIGN double NH_time_after_commit_total;
extern CL_ALIGN long long NH_nb_checkpoints;
extern CL_ALIGN long long NH_count_writes_total;
extern CL_ALIGN long long NH_count_blocks_total;
extern CL_ALIGN ts_s NH_time_blocked_total;
extern CL_ALIGN ts_s NH_manager_order_logs;
extern CL_ALIGN long long NH_nb_applied_txs;
// thread local
extern __thread ts_s NH_ts_last_snp;
extern volatile __thread ts_s ts_var;
extern __thread ts_s TM_ts1;
extern __thread ts_s TM_ts2;
extern __thread ts_s TM_ts3;
extern __thread ts_s TM_ts4;
extern __thread CL_ALIGN ts_s NH_time_validate;
extern __thread CL_ALIGN ts_s NH_tx_time;
extern __thread CL_ALIGN ts_s NH_time_after_commit;
extern __thread CL_ALIGN int NH_count_txs;
extern __thread CL_ALIGN long long NH_count_blocks;
extern __thread CL_ALIGN ts_s NH_time_blocked;
extern __thread long long NH_count_writes; // TODO: move to a struct
// ####################################################

extern CL_ALIGN int TM_SGL_var;
extern CL_ALIGN unsigned long long LOG_global_counter;
extern CL_ALIGN tx_counters_s *htm_tx_val_counters;
extern CL_ALIGN __thread int LOG_nb_writes;

// ##########################
// include from the solution
#include "extra_globals.h"
// ##########################

#endif /* NH_GLOBALS_H */

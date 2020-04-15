#include "nh_globals.h"

ts_s *NH_before_ts;

// ####################################################
// ### STATE VARIABLES ################################
// ####################################################
// global
CL_ALIGN long global_flushed_ts;
// thread local
// ####################################################

// ####################################################
// ### MISC VARIABLES #################################
// ####################################################
// global
CL_ALIGN int TM_nb_threads;
//CL_ALIGN int MAX_PHYS_THRS;
//CL_ALIGN long long CPU_MAX_FREQ;
//CL_ALIGN int SPINS_PER_100NS;
// thread local
__thread CL_ALIGN int TM_tid_var = -1;
// ####################################################

// ####################################################
// ### PERFORMANCE COUNTERS ###########################
// ####################################################
// global
ts_s NH_ts_init;
CL_ALIGN ts_s NH_time_validate_total;
CL_ALIGN int NH_count_wraps;
CL_ALIGN double NH_tx_time_total;
CL_ALIGN double NH_time_after_commit_total;
CL_ALIGN long long NH_nb_checkpoints;
CL_ALIGN long long NH_count_writes_total;
CL_ALIGN long long NH_count_blocks_total;
CL_ALIGN ts_s NH_time_blocked_total;
CL_ALIGN ts_s NH_manager_order_logs;
CL_ALIGN long long NH_nb_applied_txs;
// thread local
__thread ts_s NH_ts_last_snp;
volatile __thread ts_s ts_var;
__thread ts_s TM_ts1;
__thread ts_s TM_ts2;
__thread ts_s TM_ts3;
__thread ts_s TM_ts4;
__thread CL_ALIGN ts_s NH_tx_time;
__thread CL_ALIGN ts_s NH_time_after_commit;
__thread CL_ALIGN int NH_count_txs;
__thread CL_ALIGN ts_s NH_time_validate;
__thread long long NH_count_writes;
__thread CL_ALIGN long long NH_count_blocks;
__thread CL_ALIGN ts_s NH_time_blocked;
// ####################################################

CL_ALIGN int TM_SGL_var;
CL_ALIGN unsigned long long LOG_global_counter;
tx_counters_s CL_ALIGN *htm_tx_val_counters;
__thread CL_ALIGN int LOG_nb_writes;

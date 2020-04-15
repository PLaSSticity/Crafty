#ifndef NH_SOL_H
#define NH_SOL_H

#ifdef __cplusplus
extern "C"
{
  #endif

  #define MAXIMUM_OFFSET 400 // in cycles

// TODO: remove the externs
#undef BEFORE_HTM_BEGIN_spec
#define BEFORE_HTM_BEGIN_spec(tid, budget) \
    extern __thread int global_threadId_; \
    extern int nb_transfers; \
    extern long nb_of_done_transactions; \
    while (*NH_checkpointer_state == 1 && nb_of_done_transactions < nb_transfers) { \
      PAUSE(); \
    }

#undef BEFORE_TRANSACTION_i
#define BEFORE_TRANSACTION_i(tid, budget) \
  LOG_get_ts_before_tx(tid); \
  LOG_before_TX(); \
  TM_inc_local_counter(tid); \
  asm volatile ("sfence" ::: "memory"); /*__sync_synchronize();*/ \

#undef BEFORE_COMMIT
#define BEFORE_COMMIT(tid, budget, status) \
  ts_var = rdtscp(); /* must be the p version */  \
  if (LOG_count_writes(tid) > 0 && TM_nb_threads > 28) { \
    while ((rdtscp() - ts_var) < MAXIMUM_OFFSET); /* wait offset */ \
  }

#undef AFTER_TRANSACTION_i
#define AFTER_TRANSACTION_i(tid, budget) ({ \
  int nb_writes = LOG_count_writes(tid); \
  if (nb_writes) { \
    htm_tx_val_counters[tid].global_counter = ts_var; \
    asm volatile ("sfence" ::: "memory"); /*__sync_synchronize();*/ \
    NVMHTM_commit(tid, ts_var, nb_writes); \
  } \
  /*printf("nb_writes=%i\n", nb_writes);*/ \
  CHECK_AND_REQUEST(tid); \
  TM_inc_local_counter(tid); \
  if (nb_writes) { \
    LOG_after_TX(); \
  } \
})

#undef AFTER_ABORT
#define AFTER_ABORT(tid, budget, status) \
  /* NH_tx_time += rdtscp() - TM_ts1; */ \
  CHECK_LOG_ABORT(tid, status); \
  LOG_get_ts_before_tx(tid); \
  ts_var = rdtscp(); \
  htm_tx_val_counters[tid].global_counter = ts_var; \
  asm volatile ("sfence" ::: "memory"); /*__sync_synchronize();*/ \
  /*CHECK_LOG_ABORT(tid, status);*/ \
  /*if (status == _XABORT_CONFLICT) printf("CONFLICT: [start=%i, end=%i]\n", \
  NH_global_logs[TM_tid_var]->start, NH_global_logs[TM_tid_var]->end); */

#undef NH_before_write
#define NH_before_write(addr, val) ({ \
  LOG_nb_writes++; \
  LOG_push_addr(TM_tid_var, addr, val); \
})


  // TODO: comment for testing with STAMP
  /* #ifndef USE_MALLOC
  #if DO_CHECKPOINT == 5
  #undef  NH_alloc
  #undef  NH_free
  #define NH_alloc(size) malloc(size)
  #define NH_free(pool)  free(pool)
  #else
  #undef  NH_alloc
  #undef  NH_free
  #define NH_alloc(size) NVHTM_malloc(size)
  #define NH_free(pool)  NVHTM_free(pool)
  #endif
  #endif */

  #ifdef __cplusplus
}
#endif

#endif /* NH_SOL_H */

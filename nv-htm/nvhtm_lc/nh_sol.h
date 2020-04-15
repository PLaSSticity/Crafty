#ifndef NH_SOL_H
#define NH_SOL_H

#ifdef __cplusplus
extern "C"
{
#endif

#undef BEFORE_TRANSACTION_i
#define BEFORE_TRANSACTION_i(tid, budget) \
	LOG_before_TX(); \
	ts_var = 0

#undef BEFORE_COMMIT
#define BEFORE_COMMIT(tid, budget, status) \
	if (LOG_count_writes(tid) > 0) \
		ts_var = ++LOG_global_counter

#undef AFTER_TRANSACTION_i
#define AFTER_TRANSACTION_i(tid, budget) ({ \
	int nb_writes = LOG_count_writes(tid); \
  if (nb_writes) { \
	  NVMHTM_commit(tid, ts_var, nb_writes); \
  } \
	CHECK_AND_REQUEST(tid); \
	LOG_after_TX(); \
})

#undef AFTER_ABORT
#define AFTER_ABORT(tid, budget, status) \
	CHECK_LOG_ABORT(tid, status);

#undef NH_before_write
#define NH_before_write(addr, val) ({ \
	LOG_nb_writes++; \
	LOG_push_addr(TM_tid_var, addr, val); \
})


// TODO: comment just for testing with STAMP
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

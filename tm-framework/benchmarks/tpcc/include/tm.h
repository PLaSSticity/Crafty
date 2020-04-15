#ifndef TM_H
#define TM_H 1

#  include <stdio.h>
#  include "timer.h"

#ifndef MIN_BACKOFF
#define MIN_BACKOFF 1
#endif

#define PSTM_LOG_SIZE 0x4FFFF

extern TIMER_T exec_init_timer, exec_fini_timer;
extern int exec_nb_thrs;
extern __thread int PSTM_log[PSTM_LOG_SIZE];
extern __thread int PSTM_log_ptr;

#  define MAIN(argc, argv)              int main (int argc, char** argv)
#  define MAIN_RETURN(val)              return val

#  define GOTO_SIM()                    TIMER_READ(exec_init_timer)
#  define GOTO_REAL()                   TIMER_READ(exec_fini_timer)
#  define IS_IN_SIM()                   (0)

#  define SIM_GET_NUM_CPU(var)          /* nothing */

#  define TM_PRINTF                     printf
#  define TM_PRINT0                     printf
#  define TM_PRINT1                     printf
#  define TM_PRINT2                     printf
#  define TM_PRINT3                     printf

#  define P_MEMORY_STARTUP(numThread)   /* nothing */
#  define P_MEMORY_SHUTDOWN()           /* nothing */


#  include <string.h>
#  include <stm.h>
#  include "thread.h"

#include "min_nvm.h"

#    define TM_ARG                        /* nothing */
#    define TM_ARG_ALONE                  /* nothing */
#    define TM_ARGDECL                    /* nothing */
#    define TM_ARGDECL_ALONE              /* nothing */
#    define TM_CALLABLE                   /* nothing */

#      include <mod_mem.h>
#      include <mod_stats.h>

#if PERSISTENT_TM == 1
// commit marker and destroy
#define PSTM_COMMIT_MARKER MN_count_writes++; MN_count_writes++; SPIN_PER_WRITE(1); SPIN_PER_WRITE(1);
#else /* PERSISTENT_TM != 1 */
#define PSTM_COMMIT_MARKER
#endif

#      define TM_STARTUP(numThread)     if (sizeof(long) != sizeof(void *)) { \
                                          fprintf(stderr, "Error: unsupported long and pointer sizes\n"); \
                                          exit(1); \
                                        } \
                                        MN_learn_nb_nops(); \
                                        stm_init(); \
                                        mod_mem_init(0); \
                                        mod_stats_init();
#      define TM_SHUTDOWN()             unsigned long exec_commits, exec_aborts; \
                                        stm_get_global_stats("global_nb_commits", &exec_commits); \
                                        stm_get_global_stats("global_nb_aborts", &exec_aborts); \
                                        FILE *stats_file_fp = fopen("stats_file", "a"); \
                                      	if (ftell(stats_file_fp) < 8) { \
                                      		fprintf(stats_file_fp, "#" \
                                      		"THREADS\t"       \
                                      		"TIME\t"          \
                                      		"NB_FLUSHES\t"    \
                                      		"NB_WRITES\t"     \
                                      		"COMMITS\t"       \
                                      		"ABORTS\n"        \
                                          ); \
                                      	} \
                                        fprintf(stats_file_fp, "%i\t", exec_nb_thrs); \
                                      	fprintf(stats_file_fp, "%f\t", TIMER_DIFF_SECONDS(exec_init_timer, exec_fini_timer)); \
                                      	fprintf(stats_file_fp, "%lli\t", MN_count_spins_total); \
                                      	fprintf(stats_file_fp, "%lli\t", MN_count_writes_to_PM_total); \
                                      	fprintf(stats_file_fp, "%li\t", exec_commits); \
                                      	fprintf(stats_file_fp, "%li\n", exec_aborts); \
                                      	fclose(stats_file_fp); \
                                        stm_exit()

#      define TM_THREAD_ENTER()         MN_thr_enter(); stm_init_thread()
#      define TM_THREAD_EXIT()          MN_thr_exit(); stm_exit_thread()

#      define P_MALLOC(size)            malloc(size)
#      define P_FREE(ptr)               free(ptr)
#      define TM_MALLOC(size)           stm_malloc(size)
#      define TM_FREE(ptr)              stm_free(ptr, sizeof(stm_word_t))
#  define FAST_PATH_FREE(ptr)           TM_FREE(ptr)
#  define SLOW_PATH_FREE(ptr)           FAST_PATH_FREE(ptr)
# define SETUP_NUMBER_TASKS(n)
# define SETUP_NUMBER_THREADS(n)
# define PRINT_STATS()
# define AL_LOCK(idx)

#if PERSISTENT_TM == 1
// TODO: is crashing in TPC-C
#define PSTM_LOG_ENTRY(addr, val) ({\
  /*int log_buf = (int)val;*/ \
  MN_count_writes += 2; \
  /*memcpy(PSTM_log + PSTM_log_ptr, &log_buf, sizeof(int));*/ \
  SPIN_PER_WRITE(1); \
  /*PSTM_log_ptr += sizeof(int); \
  if (PSTM_log_ptr >= PSTM_LOG_SIZE - sizeof(int)) PSTM_log_ptr = 0;*/\
})

#define STM_ON_ABORT() ({ \
  int abort_marker = 1, val_buf = 1; \
  PSTM_LOG_ENTRY(&abort_marker, val_buf); }) \
//

#else
#define STM_ON_ABORT()
#endif

#    define TM_START(ro)                do { \
                                            int is_abort = 0;\
                                            sigjmp_buf buf; \
                                            sigsetjmp(buf, 0); \
                                            stm_tx_attr_t _a = {}; \
                                            _a.read_only = ro; \
                                            stm_start(_a, &buf); \
                                            sigsetjmp(buf, 0); \
                                            is_abort += 1; \
                                            if (is_abort > 1) STM_ON_ABORT(); \
                                        } while (0)
#    define TM_BEGIN_ID(id)             TM_START(0)
#    define TM_BEGIN()                  TM_START(0)
#    define TM_BEGIN_NO_LOG()           TM_START(0)
// # define TM_BEGIN_spec(b)              TM_BEGIN(0)
#    define TM_BEGIN_RO()               TM_START(1)
# define TM_BEGIN_EXT(b,a)              TM_BEGIN()
// # define TM_BEGIN_EXT_spec(b,a)        TM_BEGIN()
#    define TM_END()                    PSTM_COMMIT_MARKER; stm_commit()
#    define TM_END_NO_LOG()             stm_commit()
#    define TM_RESTART()                stm_abort(0)

#    define TM_EARLY_RELEASE(var)       /* nothing */

#  include <wrappers.h>

#  define TM_SHARED_READ(var)   stm_load((volatile stm_word_t *)(void *)&(var))
#  define TM_SHARED_READ_P(var) ((__typeof__(var)) stm_load_ptr((volatile void **)(void *)&(var)))
#  define TM_SHARED_READ_D(var) stm_load_double((volatile double *)(void *)&(var))
#  define TM_SHARED_READ_F(var) stm_load_float((volatile float *)(void *)&(var))

// simulates write to log
#if PERSISTENT_TM == 1

  #  define TM_SHARED_WRITE(var, val)   PSTM_LOG_ENTRY(&(var), val); stm_store((volatile stm_word_t *)(void *)&(var), (stm_word_t)val)
  #  define TM_SHARED_WRITE_P(var, val) PSTM_LOG_ENTRY(&(var), val); stm_store_ptr((volatile void **)(void *)&(var), val)
  #  define TM_SHARED_WRITE_D(var, val) PSTM_LOG_ENTRY(&(var), val); stm_store_double((volatile double *)(void *)&(var), val)
  #  define TM_SHARED_WRITE_F(var, val) PSTM_LOG_ENTRY(&(var), val); stm_store_float((volatile float *)(void *)&(var), val)
#else
#  define TM_SHARED_WRITE(var, val)   MN_count_writes++; stm_store((volatile stm_word_t *)(void *)&(var), (stm_word_t)val)
#  define TM_SHARED_WRITE_P(var, val) MN_count_writes++; stm_store_ptr((volatile void **)(void *)&(var), val)
#  define TM_SHARED_WRITE_D(var, val) MN_count_writes++; stm_store_double((volatile double *)(void *)&(var), val)
#  define TM_SHARED_WRITE_F(var, val) MN_count_writes++; stm_store_float((volatile float *)(void *)&(var), val)
#endif

#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_D(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_F(var, val)    ({var = val; var;})

#  define TM_LOCAL_READ(var)      ({*(var);})
#  define TM_LOCAL_READ_P(var)    ({*(var);})
#  define TM_LOCAL_READ_D(var)    ({*(var);})
#  define TM_LOCAL_READ_F(var)    ({*(var);})

# define FAST_PATH_SHARED_WRITE(var, val)   TM_SHARED_WRITE(var, val)
# define FAST_PATH_SHARED_WRITE_P(var, val) TM_SHARED_WRITE_P(var, val)
# define FAST_PATH_SHARED_WRITE_F(var, val) TM_SHARED_WRITE_F(var, val)
# define FAST_PATH_SHARED_WRITE_D(var, val) TM_SHARED_WRITE_D(var, val)

# define FAST_PATH_RESTART()          TM_RESTART()
# define FAST_PATH_SHARED_READ(var)   TM_SHARED_READ(var)
# define FAST_PATH_SHARED_READ_P(var) TM_SHARED_READ_P(var)
# define FAST_PATH_SHARED_READ_F(var) TM_SHARED_READ_F(var)
# define FAST_PATH_SHARED_READ_D(var) TM_SHARED_READ_D(var)

// not needed
# define SLOW_PATH_RESTART()                  FAST_PATH_RESTART()
# define SLOW_PATH_SHARED_READ(var)           FAST_PATH_SHARED_READ(var)
# define SLOW_PATH_SHARED_READ_P(var)         FAST_PATH_SHARED_READ_P(var)
# define SLOW_PATH_SHARED_READ_F(var)         FAST_PATH_SHARED_READ_D(var)
# define SLOW_PATH_SHARED_READ_D(var)         FAST_PATH_SHARED_READ_D(var)

# define SLOW_PATH_SHARED_WRITE(var, val)     FAST_PATH_SHARED_WRITE(var, val)
# define SLOW_PATH_SHARED_WRITE_P(var, val)   FAST_PATH_SHARED_WRITE_P(var, val)
# define SLOW_PATH_SHARED_WRITE_F(var, val)   FAST_PATH_SHARED_WRITE_D(var, val)
# define SLOW_PATH_SHARED_WRITE_D(var, val)   FAST_PATH_SHARED_WRITE_D(var, val)


#endif /* TM_H */

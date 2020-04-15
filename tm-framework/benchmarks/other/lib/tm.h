#ifndef TM_H
#define TM_H 1

#  include <stdio.h>
#  include <stdint.h>
#  include "timer.h"
#  include "nh.h"
#include <assert.h>

#ifndef REDUCED_TM_API

#  define MAIN(argc, argv)              int main (int argc, char** argv)
#  define MAIN_RETURN(val)              return val

// before start
#  define GOTO_SIM()                    NVHTM_start_stats()
// after end
#  define GOTO_REAL()                   NVHTM_end_stats()
#  define IS_IN_SIM()                   (0)

#  define SIM_GET_NUM_CPU(var)          /* nothing */

#  define TM_PRINTF                     printf
#  define TM_PRINT0                     printf
#  define TM_PRINT1                     printf
#  define TM_PRINT2                     printf
#  define TM_PRINT3                     printf

#    define TM_BEGIN_NO_LOG()
#    define TM_END_NO_LOG()

#  define P_MEMORY_STARTUP(numThread)   /* nothing */
#  define P_MEMORY_SHUTDOWN()           /* nothing */

#  include <assert.h>
#  include "memory.h"
#  include "thread.h"
#  include "types.h"
#  include "thread.h"
#  include <math.h>

#  define TM_ARG                        /* nothing */
#  define TM_ARG_ALONE                  /* nothing */
#  define TM_ARGDECL                    /* nothing */
#  define TM_ARGDECL_ALONE              /* nothing */
#  define TM_CALLABLE                   /* nothing */

#  define TM_BEGIN_WAIVER()
#  define TM_END_WAIVER()

// do not track private memory allocation
#ifdef CRAFTY
// Don't intercept private mallocs/free's, since we shouldn't log them.
#  define P_MALLOC(size)                malloc(size)
#  define P_FREE(ptr)                   free(ptr)
#else
#  define P_MALLOC(size)                NH_alloc(size) /* NVHTM_alloc("alloc.dat", size, 0) */
#  define P_FREE(ptr)                   NH_free(ptr)
#endif


// TODO: free(ptr)
// the patched benchmarks are broken -> intruder gives me double free or corruption

// NVHTM_alloc("alloc.dat", size, 0)
#  define TM_MALLOC(size)               NH_alloc(size) /* NVHTM_alloc("alloc.dat", size, 0) */
#  define TM_ALIGNED_MALLOC(alignment, size) NH_aligned_alloc(alignment, size)
#  define TM_FREE(ptr)           		    NH_free(ptr)
#  define FAST_PATH_FREE(ptr)           TM_FREE(ptr)
#  define SLOW_PATH_FREE(ptr)           FAST_PATH_FREE(ptr)

# define SETUP_NUMBER_TASKS(n)
# define SETUP_NUMBER_THREADS(n)
# define PRINT_STATS()
# define AL_LOCK(idx)

#define GLOBAL_instrument_write (1)

#endif /* REDUCED_TM_API */

#ifdef REDUCED_TM_API
#    define SPECIAL_THREAD_ID()         get_tid()
#else /* REDUCED_TM_API */
#    define SPECIAL_THREAD_ID()         thread_getId()
#endif /* REDUCED_TM_API */

#ifndef USE_P8
#  include <immintrin.h>
#  include <rtmintrin.h>
#else /* USE_P8 */
#  include <htmxlintrin.h>
#endif /* USE_P8 */

// TODO:

#  define TM_STARTUP(numThread)   NVHTM_init(numThread); printf("Budget=%i\n", HTM_SGL_INIT_BUDGET); HTM_set_budget(HTM_SGL_INIT_BUDGET)
#  define TM_SHUTDOWN()                NVHTM_shutdown()

#  define TM_THREAD_ENTER()            NVHTM_thr_init(); HTM_set_budget(HTM_SGL_INIT_BUDGET)
#  define TM_THREAD_EXIT()             NVHTM_thr_exit()

// leave local_exec_mode = 0 to use the FAST_PATH
# define TM_BEGIN(b)                   NH_begin()
// # define TM_BEGIN_spec(b)              NH_begin_spec()
# define TM_BEGIN_ID(b)                NH_begin()

# define TM_BEGIN_EXT(b,a)             TM_BEGIN(b)
// # define TM_BEGIN_EXT_spec(b,a)        TM_BEGIN_spec(b)

# define TM_END()                      NH_commit()

#    define TM_BEGIN_RO()              TM_BEGIN(0)
#    define TM_RESTART()               NVHTM_abort_tx()
#    define TM_EARLY_RELEASE(var)

// TODO: check incompatible types
# define TM_RESTART()          NVHTM_abort_tx()
# define TM_SHARED_READ(var)   ({ NH_read(&(var)); })
# define TM_SHARED_READ_P(var) ({ NH_read_P(&(var)); })
# define TM_SHARED_READ_F(var) ({ NH_read_D(&(var)); })
# define TM_SHARED_READ_D(var) ({ NH_read_D(&(var)); })

#define GRANULE_ALIGN_HIGH (~0ULL << 3) /* 1111...1000 */
#define GRANULE_ALIGN_LOW (~GRANULE_ALIGN_HIGH) /* 0000...0111 */
#define __ASSERT_ALIGNED(var) ({ txassert(!(((uintptr_t)&(var)) & GRANULE_ALIGN_LOW)); /* Ensure write is aligned */ })
#define __ASSERT_SIZE(var) ({ static_assert(sizeof(var) == sizeof(GRANULE_TYPE), "TM_SHARED_WRITE* can only be used on 64-bit writes. Use TM_SHR_WR for smaller writes."); })
#if GLOBAL_instrument_write
# define TM_SHARED_WRITE(var, val)   ({ __ASSERT_ALIGNED(var); __ASSERT_SIZE(var); NH_write(&(var), val); var;})
# define TM_SHARED_WRITE_P(var, val) ({ __ASSERT_ALIGNED(var); __ASSERT_SIZE(var); NH_write_P(&(var), val); var;})
# define TM_SHARED_WRITE_F(var, val) ({ __ASSERT_ALIGNED(var); __ASSERT_SIZE(var); NH_write_D(&(var), val); var;})
# define TM_SHARED_WRITE_D(var, val) ({ __ASSERT_ALIGNED(var); __ASSERT_SIZE(var); NH_write_D(&(var), val); var;})
#else
# define TM_SHARED_WRITE(var, val)   ({ TM_LOCAL_WRITE(var, val); var;})
# define TM_SHARED_WRITE_P(var, val) ({ TM_LOCAL_WRITE_P(var, val); var;})
# define TM_SHARED_WRITE_F(var, val) ({ TM_LOCAL_WRITE_F(var, val); var;})
# define TM_SHARED_WRITE_D(var, val) ({ TM_LOCAL_WRITE_D(var, val); var;})
#endif
#define TM_SHR_WR(var, val) ({ NH_write_small(&(var), val); var;})

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

// local is not tracked
#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_F(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_D(var, val)    ({var = val; var;})

static inline void* TM_MEMSET(void *s, int c, size_t n) {
    // Prepare a granule sized value
    GRANULE_TYPE val = 0;
    size_t i;
    for (i = 0; i < sizeof(GRANULE_TYPE); i++) {
        val <<= 8;
        val |= c;
    }
    // Do granule-sized writes
    GRANULE_TYPE *addr = (GRANULE_TYPE*)s;
    for (i = 0; i < n / sizeof(GRANULE_TYPE); i++) {
        TM_SHARED_WRITE(*(addr + i), val);
    }
    // Finish the remaining writes byte-sized
    char* tail_addr = (char *)(addr + i);
    for (i = 0; i < n % sizeof(GRANULE_TYPE); i++) {
        TM_SHR_WR(*(tail_addr + i), (char)c);
    }
    return s;
}


static inline void* TM_MEMCPY(void *dest, const void *src, size_t n) {
    size_t i;
    // Do granule-sized writes
    GRANULE_TYPE *dest_addr = (GRANULE_TYPE*)dest;
    GRANULE_TYPE *src_addr = (GRANULE_TYPE*)src;
    for (i = 0; i < n / sizeof(GRANULE_TYPE); i++) {
        TM_SHARED_WRITE(*(dest_addr + i), *(src_addr + i));
    }
    // Finish the remaining writes byte-sized
    char* dest_tail = (char *)(dest_addr + i);
    char* src_tail = (char *)(src_addr + i);
    for (i = 0; i < n % sizeof(GRANULE_TYPE); i++) {
        TM_SHR_WR(*(dest_tail + i), *(src_tail + i));
    }
    return dest;
}


static inline void* TM_REALLOC(void *ptr, size_t size) {
    void* new_ptr = TM_MALLOC(size);
    if (ptr == NULL) return new_ptr;
    TM_MEMCPY(new_ptr, ptr, size);
    TM_FREE(ptr);
    return new_ptr;
}


// Save and restore local state for programs where local state is modified within a transaction.
// Required only for Crafty, as it repeats transactions.
#ifdef IDEMPOTENT_TRANSACTIONS
#define TM_SAVE(var) {typeof(var) _TM__##var = var;
#define TM_RESTORE(var) var = _TM__##var;}
#else
#define TM_SAVE(var) /*noop*/
#define TM_RESTORE(var) /*noop*/
#endif

#endif

#ifndef HTM_SGL_RETRY_TEMPLATE_H_GUARD
#define HTM_SGL_RETRY_TEMPLATE_H_GUARD

#include "arch.h"
#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>
#include <syscall.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C"
{
#define __auto_type auto
#endif

#ifndef HTM_SGL_INIT_BUDGET
#define HTM_SGL_INIT_BUDGET 20
#endif /* HTM_SGL_INIT_BUDGET */


#ifndef HTM_RETRY_DELAY_MAX
#define HTM_RETRY_DELAY_MAX 65536
#endif
#ifndef HTM_RETRY_DELAY_MIN
#define HTM_RETRY_DELAY_MIN 0
#endif



typedef struct HTM_SGL_local_vars_ {
    int budget,
        tid,
        status,
        nesting;
    unsigned int rndstate;
} __attribute__((packed))  HTM_SGL_local_vars_s;

extern CL_ALIGN int HTM_SGL_var;
extern __thread CL_ALIGN HTM_SGL_local_vars_s HTM_SGL_vars;


#define thrprintf(...) /* nothing */
#ifdef VERBOSE
#undef thrprintf
static inline void _thrprintf(char const *format, ...) {
    if (HTM_test()) return;
    static int lock = 0;
    while (!__sync_bool_compare_and_swap(&lock, 0, 1)) PAUSE();
    va_list argp;
    va_start(argp, format);
    printf("%i: ", HTM_SGL_vars.tid);
    vprintf(format, argp);
    va_end(argp);
    __sync_bool_compare_and_swap(&lock, 1, 0);
}
#define thrprintf _thrprintf
#endif


#ifdef NDEBUG
#define txassert(c) /* nothing */
#define txassert_named(c,code) /* nothing */
#else
#define txassert(c) txassert_named(c, 1)
#define txassert_named(c,code) ({ \
        __auto_type __debug_t = c; \
        if (!(__debug_t)) { \
            if (HTM_test()) HTM_commit(); \
            thrprintf("Assertion failed!\n"); \
            assert(__debug_t); \
        } \
	})
#endif

#define START_TRANSACTION(status) (HTM_begin(status) != HTM_CODE_SUCCESS)
#define BEFORE_TRANSACTION(tid, budget) /* empty */
#define AFTER_TRANSACTION(tid, budget)  /* empty */

#define UPDATE_BUDGET(tid, budget, status) \
  HTM_inc_status_count(status); \
  HTM_INC(status); \
	budget = HTM_update_budget(budget, status)

/* The HTM_SGL_update_budget also handle statistics */

#define CHECK_SGL_NOTX() if (HTM_SGL_var) { HTM_block(); }
#define CHECK_SGL_HTM()  if (HTM_SGL_var) { HTM_abort(); }

#define AFTER_BEGIN(tid, budget, status)   /* empty */
#define BEFORE_COMMIT(tid, budget, status) /* empty */
#define COMMIT_TRANSACTION(tid, budget, status) \
	HTM_commit(); /* Commits and updates some statistics after */ \
	HTM_inc_status_count(status); \
  HTM_INC(status)

#define ENTER_SGL(tid) HTM_enter_fallback()
#define EXIT_SGL(tid)  HTM_exit_fallback()
#define AFTER_ABORT(tid, budget, status)   /* empty */

#define BEFORE_HTM_BEGIN(tid, budget)  /* empty */
#define AFTER_HTM_BEGIN(tid, budget)   /* empty */
#define BEFORE_SGL_BEGIN(tid)          /* empty */
#define AFTER_SGL_BEGIN(tid)           /* empty */

#define BEFORE_HTM_COMMIT(tid, budget) /* empty */
#define AFTER_HTM_COMMIT(tid, budget)  /* empty */
#define BEFORE_SGL_COMMIT(tid)         /* empty */
#define AFTER_SGL_COMMIT(tid)          /* empty */

#define BEFORE_CHECK_BUDGET(budget) /* empty */
// called within HTM_update_budget
#define HTM_UPDATE_BUDGET(budget, status) ({ \
    int res = budget - 1; \
    res; \
})




#define ENTER_HTM_COND(tid, budget) budget > 0
#define IN_TRANSACTION(tid, budget, status) \
	HTM_test()


#ifndef RETRY_NO_DELAY
#define AFTER_ABORT_INTERNAL(tid, budget, status) ({ \
    if (status & _XABORT_CONFLICT && !ENTER_HTM_COND(tid, budget)) { \
        int delay = (rand_r(&HTM_SGL_vars.rndstate) % HTM_RETRY_DELAY_MAX) | HTM_RETRY_DELAY_MIN; \
        int i = 0; \
        while (i < delay) { \
            PAUSE(); \
            i++; \
        } \
    } \
})
#else
#define AFTER_ABORT_INTERNAL(tid, budget, status) /* empty */
#endif

// #################################
// Called within the API
#define HTM_INIT()       /* empty */
#define HTM_EXIT()       /* empty */
#define HTM_THR_INIT()   /* empty */
#define HTM_THR_EXIT()   /* empty */
#define HTM_INC(status)  /* Use this to construct side statistics */
// #################################

#define HTM_SGL_budget HTM_SGL_vars.budget
#define HTM_SGL_status HTM_SGL_vars.status
#define HTM_SGL_tid    HTM_SGL_vars.tid
#define HTM_SGL_env    HTM_SGL_vars.env

#define HTM_SGL_begin() \
{ \
    assert(HTM_SGL_vars.nesting == 0 || HTM_test() || HTM_SGL_var /* Not in a transaction but nesting is non-0 */); \
    thrprintf("Beginning PTX %s:%d\n", __FILE__, __LINE__); \
    if (++HTM_SGL_vars.nesting == 1) { \
        HTM_SGL_budget = HTM_SGL_INIT_BUDGET; /*HTM_get_budget();*/ \
        BEFORE_TRANSACTION(HTM_SGL_tid, HTM_SGL_budget); \
        while (1) { /*setjmp(HTM_SGL_env);*/ \
            BEFORE_CHECK_BUDGET(HTM_SGL_budget); \
            if (ENTER_HTM_COND(HTM_SGL_tid, HTM_SGL_budget)) { \
                CHECK_SGL_NOTX(); \
                BEFORE_HTM_BEGIN(HTM_SGL_tid, HTM_SGL_budget); \
                if (START_TRANSACTION(HTM_SGL_status)) { \
                    UPDATE_BUDGET(HTM_SGL_tid, HTM_SGL_budget, HTM_SGL_status); \
                    thrprintf("Aborted\n"); \
                    AFTER_ABORT_INTERNAL(HTM_SGL_tid, HTM_SGL_budget, HTM_SGL_status); \
                    AFTER_ABORT(HTM_SGL_tid, HTM_SGL_budget, HTM_SGL_status); \
                    continue; /*longjmp(HTM_SGL_env, 1);*/ \
                } \
                CHECK_SGL_HTM(); \
                AFTER_HTM_BEGIN(HTM_SGL_tid, HTM_SGL_budget); \
            } \
            else { \
                BEFORE_SGL_BEGIN(HTM_SGL_tid); \
                thrprintf("Trying to acquire SGL\n"); \
                ENTER_SGL(HTM_SGL_tid); \
                AFTER_SGL_BEGIN(HTM_SGL_tid); \
                thrprintf("Acquired SGL\n"); \
            } \
            AFTER_BEGIN(HTM_SGL_tid, HTM_SGL_budget, HTM_SGL_status); \
            break; /* delete when using longjmp */ \
        } \
    } \
}
//
#define HTM_SGL_commit() \
{ \
    assert(HTM_SGL_vars.nesting >= 1); \
    if (--HTM_SGL_vars.nesting == 0) { \
        BEFORE_COMMIT(HTM_SGL_tid, HTM_SGL_budget, HTM_SGL_status); \
        if (IN_TRANSACTION(HTM_SGL_tid, HTM_SGL_budget, HTM_SGL_status)) { \
            BEFORE_HTM_COMMIT(HTM_SGL_tid, HTM_SGL_budget); \
            COMMIT_TRANSACTION(HTM_SGL_tid, HTM_SGL_budget, HTM_SGL_status); \
            AFTER_HTM_COMMIT(HTM_SGL_tid, HTM_SGL_budget); \
        } \
        else { \
            thrprintf("Finishing SGL\n"); \
            BEFORE_SGL_COMMIT(HTM_SGL_tid); \
            EXIT_SGL(HTM_SGL_tid); \
            AFTER_SGL_COMMIT(HTM_SGL_tid); \
        } \
        AFTER_TRANSACTION(HTM_SGL_tid, HTM_SGL_budget); \
        thrprintf("Finishing PTX\n"); \
    } \
} \


#define HTM_SGL_before_write(addr, val) /* empty */
#define HTM_SGL_after_write(addr, val)  /* empty */

#define HTM_SGL_write(addr, val) ({ \
    assert(HTM_test() || HTM_SGL_var || HTM_SGL_vars.nesting == 0 /* Not in a transaction but nesting is non-0 */); \
	HTM_SGL_before_write(addr, val); \
	*((GRANULE_TYPE*)addr) = val; \
	HTM_SGL_after_write(addr, val); \
	val; \
})

#define HTM_SGL_write_D(addr, val) ({ \
	GRANULE_TYPE g = CONVERT_GRANULE_D(val); \
	HTM_SGL_write((GRANULE_TYPE*)addr, g); \
	val; \
})

#define HTM_SGL_write_P(addr, val) ({ \
	GRANULE_TYPE g = (GRANULE_TYPE) val; /* works for pointers only */ \
	HTM_SGL_write((GRANULE_TYPE*)addr, g); \
	val; \
})

#define HTM_SGL_before_read(addr) /* empty */

#define HTM_SGL_read(addr) ({ \
	HTM_SGL_before_read(addr); \
	*((GRANULE_TYPE*)addr); \
})

#define HTM_SGL_read_P(addr) ({ \
	HTM_SGL_before_read(addr); \
	*((GRANULE_P_TYPE*)addr); \
})

#define HTM_SGL_read_D(addr) ({ \
	HTM_SGL_before_read(addr); \
	*((GRANULE_D_TYPE*)addr); \
})

/* TODO: persistency assumes an identifier */
#define HTM_SGL_alloc(size) malloc(size)
#define HTM_SGL_free(pool) free(pool)

// Exposed API
#define HTM_init(nb_threads) HTM_init_(HTM_SGL_INIT_BUDGET, nb_threads)
void HTM_init_(int init_budget, int nb_threads);
void HTM_exit();
void HTM_thr_init();
void HTM_thr_exit();
void HTM_block();

// int HTM_update_budget(int budget, HTM_STATUS_TYPE status);
#define HTM_update_budget(budget, status) HTM_UPDATE_BUDGET(budget, status)
void HTM_enter_fallback();
void HTM_exit_fallback();

void HTM_inc_status_count(int status_code);
int HTM_get_nb_threads();
int HTM_get_tid();

// Getter and Setter for the initial budget
int HTM_get_budget();
void HTM_set_budget(int budget);

void HTM_set_is_record(int is_rec);
int HTM_get_is_record();
/**
 * @accum : int[nb_threads][HTM_NB_ERRORS]
 */
int HTM_get_status_count(int status_code, int **accum);
void HTM_reset_status_count();


#ifdef __cplusplus
}
#endif

#endif /* HTM_SGL_RETRY_TEMPLATE_H_GUARD */

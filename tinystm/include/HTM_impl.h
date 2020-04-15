#ifndef HTM_IMPL_H_GUARD
#define HTM_IMPL_H_GUARD

#if defined(HTM_WRITE_THROUGH) || defined(MODULAR)

#include "HTM_arch.h"

#ifndef HTM_INIT_TX_SIZE
#define HTM_INIT_TX_SIZE 64 // change this depending on the ARCH
#endif /* HTM_SGL_INIT_BUDGET */

typedef struct HTM_local_vars_ {
  HTM_STATUS_TYPE status;
  int tid,
      isFallback,
      currTxSize,
      maxTxSize;
} __attribute__((packed)) HTM_local_vars_s;

// application must implement it
#define HTM_MAX_THREADS 2048
extern __thread HTM_CL_ALIGN HTM_local_vars_s HTM_local_vars;
extern HTM_CL_ALIGN unsigned int HTM_errors[HTM_MAX_THREADS][HTM_NB_ERRORS];

// easy to access state
#define HTM_tid        HTM_local_vars.tid
#define HTM_status     HTM_local_vars.status
#define HTM_isFallback HTM_local_vars.isFallback
#define HTM_currTxSize HTM_local_vars.currTxSize
#define HTM_maxTxSize  HTM_local_vars.currTxSize

#define HTM_START_TRANSACTION()   (HTM_begin(HTM_status) != HTM_CODE_SUCCESS)
#define HTM_BEFORE_TRANSACTION()  /* empty */
#define HTM_AFTER_TRANSACTION()   /* empty */

#define HTM_AFTER_BEGIN()   /* empty */
#define HTM_BEFORE_COMMIT() /* empty */
#define HTM_COMMIT_TRANSACTION() \
	HTM_commit(); /* Commits and updates some statistics after */ \
  HTM_INC(HTM_status)

#define HTM_AFTER_ABORT()   HTM_maxTxSize = HTM_maxTxSize / 2 + 1; HTM_currTxSize = 0
#define HTM_BEFORE_BEGIN()  /* empty */
#define HTM_BEFORE_COMMIT() /* empty */
#define HTM_AFTER_COMMIT()  /* empty */

#define HTM_ENTER_COND()      HTM_isFallback
#define HTM_IN_TRANSACTION()  HTM_test()

// TODO: add a flag to disable statistics
#define HTM_INC(status) HTM_errors[HTM_tid][HTM_ERROR_TO_INDEX(status)]++

#define HTM_BEGIN() \
{ \
  HTM_currTxSize = 0; \
  HTM_maxTxSize = 0; \
  HTM_isFallback = 0; \
  HTM_BEFORE_TRANSACTION(); \
  while (1) { \
    if (HTM_ENTER_COND()) { \
      HTM_BEFORE_BEGIN(); \
      if (HTM_START_TRANSACTION(HTM_status)) { \
        HTM_errors[HTM_tid][HTM_ABORT]++;
        HTM_INC(HTM_status); \
        HTM_AFTER_ABORT(); \
        continue; \
      } \
    } \
    HTM_AFTER_BEGIN(); \
    break;\
  } \
}
//
#define HTM_COMMIT() \
{ \
  HTM_BEFORE_COMMIT(); \
  if (HTM_IN_TRANSACTION()) { \
    HTM_BEFORE_COMMIT(); \
    HTM_COMMIT_TRANSACTION(); \
    HTM_AFTER_COMMIT(); \
    HTM_errors[HTM_tid][HTM_SUCCESS]++; \
  } else { \
    HTM_errors[HTM_tid][HTM_FALLBACK]++; \
  } \
  HTM_AFTER_TRANSACTION(); \
  HTM_isFallback = 1; \
  HTM_currTxSize = 0; \
} \

#endif /* defined(HTM_WRITE_THROUGH) || defined(MODULAR) */

#endif /* HTM_IMPL_H_GUARD */

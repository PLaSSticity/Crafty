#ifndef HTM_H
#define HTM_H 1

#include <sched.h>
#include <unistd.h>

#define BIND_THREAD(threadId) { \
    cpu_set_t my_set; \
    CPU_ZERO(&my_set); \
    if (CPU_LOCKS == 4 || CPU_LOCKS == 8) { /* Haswell8 and 16 cores */ \
        CPU_SET(threadId % (CPU_LOCKS * 2), &my_set); \
    } else if (CPU_LOCKS == 14) { /* Haswell56 cores */ \
        int cores = CPU_LOCKS * 2; \
        if (threadId >= 14) { \
            CPU_SET(threadId + 14, &my_set); \
        } else { \
            CPU_SET(threadId, &my_set); \
        } \
    } else if (CPU_LOCKS == 10) { /* Power8 80 cores */ \
        CPU_SET((threadId * 8 % 80) + ((int)threadId / 10), &my_set); \
    } \
    sched_setaffinity(0, sizeof(cpu_set_t), &my_set); \
}

#if defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)
  #include <htmxlintrin.h>

  #define CACHE_LINE_SIZE 128

  #define PAUSE_INSTR asm volatile("lwsync" : : )

  #define HTM_BEGIN(TM_buff) __TM_begin(&TM_buff)
  #define HTM_ABORT(code) __TM_named_abort(code)
  #define HTM_COMMIT __TM_end()

  #define STATUS_TYPE unsigned char
  #define ARG_TYPE TM_buff_type

  #define IS_STARTED _HTM_TBEGIN_STARTED
  #define PREPARE_CHECK_EXPLICIT(buff, status, code) unsigned char code_used;  long ret_val = __TM_is_named_user_abort(&buff, &code_used);
  #define IS_EXPLICIT_ABORT(code) ret_val > 0 && code_used  == code
  #define IS_EXPLICIT_ABORT_NO_CODE(buff) __TM_is_named_user_abort(&buff, &code_used) > 0
  #define SHOULD_RETRY(status, buff) __TM_is_failure_persistent(&buff) == 0
  #define IS_IN_TRANSACTION (_HTM_STATE(__builtin_ttest()) == _HTM_TRANSACTIONAL)
  #define IS_CAPACITY(status, buf) __TM_is_footprint_exceeded(&buf) != 0
  #define IS_CONFLICT(status, buf) __TM_is_conflict(&buf) != 0

  #define TICK(res) { \
    unsigned long long __tb; \
    __asm__ volatile ("mfspr %[tb], 268\n" \
      : [tb]"=r" (__tb) \
      : ); \
    res  = __tb; \
  }

#else
  #include <immintrin.h>
  #include <rtmintrin.h>

  #define CACHE_LINE_SIZE 64

  #define PAUSE_INSTR  __asm__ ( "pause;")

  #define HTM_BEGIN(TM_buff) _xbegin()
  #define HTM_ABORT(code) _xabort(code)
  #define HTM_COMMIT _xend()

  #define STATUS_TYPE int
  #define ARG_TYPE int

  #define IS_STARTED _XBEGIN_STARTED
  #define PREPARE_CHECK_EXPLICIT(buff, status, code) unsigned char code_used;  long ret_val = (status & _XABORT_EXPLICIT && _XABORT_CODE(status) == code);
  #define IS_EXPLICIT_ABORT(code) ret_val
  #define IS_EXPLICIT_ABORT_NO_CODE(buff) status & _XABORT_EXPLICIT
  #define SHOULD_RETRY(status, buff) status & _XABORT_RETRY && !(status & _XABORT_CONFLICT)
  #define IS_IN_TRANSACTION _xtest()
  #define IS_CAPACITY(status, buf) status & _XABORT_CAPACITY
  #define IS_CONFLICT(status, buf) status & _XABORT_CONFLICT

  # define TICK(res) { \
      unsigned hi, lo; \
      __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi)); \
      res = ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 ); \
  } \

#endif



#endif

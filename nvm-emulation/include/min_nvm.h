#ifndef MIN_NVM_GUARD_H
#define MIN_NVM_GUARD_H

#ifdef __cplusplus
extern "C"
{
  #endif

  #include "arch.h"

  #include <stdlib.h>
  #include <stdio.h>

  #ifndef CACHE_LINE_SIZE
  #if defined(__powerpc__)
  #define CACHE_LINE_SIZE 128
  #else /* USE_P8 */
  #define CACHE_LINE_SIZE 64
  #endif /* USE_P8 */
  #endif

  #ifndef CL_ALIGN
  #define CL_ALIGN __attribute__((align))
  #endif

  typedef struct __attribute__((packed)) NH_spin_info_ {
    long long spins_per_100;
    long long count_spins;
    long long count_writes;
    ts_s time_spins;
  } NH_spin_info_s ;

  extern long long MN_count_spins_total;
  extern unsigned long long MN_time_spins_total;
  extern long long MN_count_writes_to_PM_total;

  extern CL_ALIGN int SPINS_PER_100NS;
  extern __thread CL_ALIGN NH_spin_info_s MN_info;

  #define NH_spins_per_100 MN_info.spins_per_100
  #define MN_count_spins   MN_info.count_spins
  #define MN_count_writes  MN_info.count_writes
  #define MN_time_spins    MN_info.time_spins

  #define NOP_X10 asm volatile("nop\n\t\nnop\n\t\nnop\n\t\nnop\n\t\nnop" \
  "\n\t\nnop\n\t\nnop\n\t\nnop\n\t\nnop\n\t\nnop" ::: "memory");

  // nops cannot be optimized (how accurate is this?)
  #define SPIN_NS(ns) ({ \
    volatile unsigned long long i; \
    for (i = 0; i < ((ns * SPINS_PER_100NS) / 100); ++i) { \
      NOP_X10; \
    } \
    0; \
  })

  // simulation
  int SPIN_PER_WRITE(int nb_writes);
  int WRITE_TO_PM(void*, intptr_t, int); // args not used

  #define SPIN_10NOPS(nb_spins) ({ \
    volatile unsigned long long i; \
    for (i = 0; i < (unsigned long long) (nb_spins); ++i) NOP_X10; \
    i; \
  })

  #if defined(__powerpc__)
  // TODO
  #else
  #define mfence() ({ \
    asm volatile ( "mfence" :: : "memory" ); \
  })

  // volatile void *p
  #define clflush(p) ({ \
    asm volatile ( "clflush (%0)" :: "r"((p)) ); \
  })
  #endif

  void *MN_alloc(const char *file_name, size_t);
  void MN_free(void*);

  void MN_thr_enter(void);
  void MN_thr_exit(void);

  #define MN_read(addr) ({ \
    *addr; \
  })

  int MN_write(void *addr, void *buf, size_t size, int to_aux);

  void MN_flush(void *addr, size_t size, int);
  void MN_drain(void);
  void MN_learn_nb_nops(void);

  #ifdef __cplusplus
}
#endif

#endif /* MIN_NVM_GUARD_H */

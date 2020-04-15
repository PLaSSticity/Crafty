#ifndef LOG_AUX_H
#define LOG_AUX_H

#include "tm.h"
#include "nvhtm.h"
#include "nvhtm_helper.h"

#include <stdlib.h>
#include <assert.h>

#ifdef __cplusplus
extern "C"
{
  #endif

  #define distance_ptr(start_ptr, end_ptr) ({ \
    LOG_DISTANCE2(start_ptr, end_ptr, LOG_local_state.size_of_log); \
  })

  /*({ \
    int s = (int)(start_ptr), e = (int)(end_ptr), res; \
    assert(s >= 0); \
    assert(e >= 0); \
    assert(s < LOG_local_state.size_of_log); \
    assert(e < LOG_local_state.size_of_log); \
    res = ptr_mod(-s, e, LOG_local_state.size_of_log); \
    assert((s <= e && res == ((int)e - (int)s)) || \
    (s > e && res == (((int)e - (int)s) + \
    LOG_local_state.size_of_log))); \
    res; \
  })*/

  #define APPLY_BACKWARD_VAL (int)((double)LOG_FILTER_THRESHOLD*\
    (double)LOG_local_state.size_of_log) // TODO: parametrize this

  // NOTE: LOG_TS is the actual commit marker
  enum LOG_MARKS
  {
    LOG_EMPTY = 0, LOG_TS = 1, LOG_COMMIT = 2, LOG_MALLOC = 3,
  };

  int LOG_check_correct_ts(int tid);
  int LOG_try_lock();
  void LOG_unlock();

  int LOG_spin_per_write();
  int LOG_get_nb_spins();

  ts_s LOG_first_ts(int tid, int *entry_pos);
  ts_s LOG_last_ts(int tid);

  int LOG_last_commit_idx(int tid);

  ts_s LOG_next_tx_ts(int*, int *pos_base);
  void LOG_fix();
  int LOG_is_logged_tx();
  int LOG_is_logged_tx2();
  int LOG_is_valid(int tid);

  ts_s LOG_AUX_next_ts(NVLog_s*, ts_s, int*);

  int LOG_AUX_sort_logs();

  // TODO: what will the checkpoint be
  int LOG_redo_threads(); // is there threads to REDO
  double LOG_capacity_used(); // amount of used space
  long long int LOG_total_used();

  // TODO: move important static functions here
  // to_flush is set<intptr_t>*
  int LOG_AUX_apply_one_to_checkpoint(int update_start, int do_flush, void* to_flush);
  // to_flush is set<intptr_t>*
  int LOG_AUX_apply_one_to_checkpoint2(int update_start, int do_flush, void* to_flush);
  int LOG_AUX_apply_one_to_checkpoint3(int update_start, int do_flush, void* to_flush);
  // applies N transactions backward, and avoids repeated writes
  void *LOG_AUX_apply_to_checkpoint(void *addr, GRANULE_TYPE value, int do_flush);

  size_t LOG_base2_before(size_t size_of_log);
  NVLog_s* LOG_init_1thread(void *log_pool, size_t max_size);
  // arg can be a signed int

  #define entry_is_ts(entry) ({ \
    GRANULE_TYPE res = entry.addr == (GRANULE_TYPE*) LOG_TS ? entry.value : 0; \
    res; \
  })

  #define entry_is_commit(entry) ({ \
    bool res = entry.addr == (GRANULE_TYPE*) LOG_TS; \
    res; \
  })

  #define entry_is_update(entry) ({ \
    bool res = (entry.addr != (GRANULE_TYPE*) LOG_TS \
    && entry.addr != (GRANULE_TYPE*) LOG_MALLOC); \
    res; \
  })

  #ifdef __cplusplus
}
#endif

#endif /* LOG_AUX_H */

#include "utils.h"
#include "log_aux.h"
#include "nvhtm_helper.h"
#include "cp.h"

#include "nh_globals.h"

#include "rdtsc.h"

#include <cstdlib>
#include <cmath>
#include <climits>
#include <cstdio>
//#include <algorithm>
#include <vector>
#include <cassert>
#include <iostream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include <map>
#include <unordered_map>
#include <utility>
#include <set>
#include <unordered_set>
#include <list>
#include <thread>
#include <mutex>

// ################ defines

#define LOG_FILE "./shared"
#define LOGS_EXT ".log%i"

#define SET_START(log, start_v) ({ \
  log->start_tx = -1; \
  log->start = start_v; \
})

using namespace std;

// ################ types

/* empty */

// ################ variables
//
extern CL_ALIGN map<void*, NVMHTM_mem_s*> instances; // from nvmhtm.cpp
/* CL_ALIGN NVLog_s nvm_htm_log_insts[MAX_NB_THREADS]; */

// TODO: __attribute__((padding))
static double ns_per_10_nops = 100;
static int total_spins = 0;

//thread_local NVLog_s *nvm_htm_log = NULL;
CL_ALIGN map<ts_s, NVLog_s*> sorted_logs; // extern for log.cpp
CL_ALIGN map<int, NVLog_s*> empty_logs;
CL_ALIGN cp_s *cp_instance;

static CL_ALIGN mutex log_mtx;
static CL_ALIGN int log_lock;

// ################ variables (thread-local)

//__thread CL_ALIGN NVLog_s nvm_htm_local_log_inst; // DOESN'T WORK!!! why?
// __thread CL_ALIGN size_t nvm_htm_log_size;

// ################ functions

// TODO: repeated functions in log.cpp

static inline ts_s first_ts(int tid, int &entry_ptr);
static inline ts_s first_ts(NVLog_s *log, int &entry_ptr);

static int empty_to_sorted();
static int check_correct_ts(int tid);

// ################ implementation header

void* LOG_AUX_apply_to_checkpoint(void *addr,
  GRANULE_TYPE value, int do_flush
)
{
  // writes in some buffer
  MN_write(addr, &(value), sizeof(GRANULE_TYPE), 1);
  return NULL;
}

// use maps
int LOG_AUX_apply_one_to_checkpoint(int update_start,
  int do_flush, void *to_fl
)
{
  set<intptr_t> *to_flush = (set<intptr_t> *) to_fl;

  int i;

  empty_to_sorted();

  if (sorted_logs.empty()) {
    return 1;
  }

  // maps are sorted
  NVLog_s *smallest_log = sorted_logs.begin()->second;
  i = smallest_log->start;

  while (i != smallest_log->end) {

    NVLogEntry_s entry = smallest_log->ptr[i];
    ts_s ts_val = entry_is_ts(entry);
    if (ts_val) {

      i = ptr_mod_log(i, 1); // begin after TS
      smallest_log->start_tx = -1;
      if (update_start) {
        smallest_log->start = i; // TODO: check this
      }

      int entry_idx;
      ts_s new_ts = first_ts(smallest_log, entry_idx);

      // change the TS in sorted_logs (to quickly fetch the next TS)
      if (sorted_logs.size() == 1) {
        sorted_logs.clear();
      }
      else {
        sorted_logs.erase(sorted_logs.begin());
      }
      if (new_ts != 0) {
        sorted_logs.insert(make_pair(new_ts, smallest_log));
      }
      else {
        int tid = smallest_log->tid;
        empty_logs.insert(make_pair(tid, smallest_log));
      }

      break;
    }

    if (entry_is_update(entry)) {
      if (to_flush) {
        // apply the flush lazily
        intptr_t cl_addr = (intptr_t) entry.addr;
        cl_addr >>= 6; // remove the log_2(CL_SIZE) bits (offset)
        cl_addr <<= 6;
        to_flush->insert(cl_addr);
      }
      // writes
      LOG_AUX_apply_to_checkpoint(entry.addr, entry.value, do_flush);
    }

    i = ptr_mod_log(i, 1);
  }

  return 0;
}

// THIS DOES NOT WORK
//static int old_pos[MAX_NB_THREADS];
//
//static ts_s buffered_next_ts(int *pos, int *tid) {
//    int i;
//    ts_s next = 0;
//
//    for ( i = 0; i < TM_nb_threads; ++i ) {
//        NVLog_s *log = NH_global_logs[i];
//        ts_s new_ts = LOG_AUX_next_ts(log, 0, &(old_pos[i]));
//
//    }
//}

// Use arrays
int LOG_AUX_apply_one_to_checkpoint2(int update_start,
  int do_flush, void *to_fl
)
{
  set<intptr_t> *to_flush = (set<intptr_t> *) to_fl;
  int i;


  int pos = -1, tid = -1;
  ts_s next_ts;

  ts_s ts1 = 0, ts2 = 0;
  ts1 = rdtscp();

  next_ts = LOG_next_tx_ts(&pos, &tid);
  if (next_ts == 0) return 1;
  NVLog_s* smallest_log = NH_global_logs[tid];

  ts2 = rdtscp();
  NH_manager_order_logs += ts2 - ts1;

  i = smallest_log->start;
  while (1/* i != smallest_log->end_last_tx */) {
    NVLogEntry_s entry = smallest_log->ptr[i];
    ts_s ts_val = entry_is_ts(entry);
    if (ts_val) {

      i = ptr_mod_log(i, 1);
      smallest_log->start_tx = -1;
      smallest_log->start = i;
      break;
    }

    if (entry_is_update(entry)) {
      if (to_flush) {
        // apply the flush lazily
        intptr_t cl_addr = (intptr_t) entry.addr;
        cl_addr >>= 6; // remove the log_2(CL_SIZE) bits (offset)
        cl_addr <<= 6;
        to_flush->insert(cl_addr);
      }
      // writes
      LOG_AUX_apply_to_checkpoint(entry.addr, entry.value, do_flush);
    }

    i = ptr_mod_log(i, 1);
  }

  return 0;
}

// fast apply next
int LOG_AUX_apply_one_to_checkpoint3(int update_start,
  int do_flush, void *to_fl
)
{
  static int next_log = 0;
  set<intptr_t> *to_flush = (set<intptr_t> *) to_fl;
  int i;

  ts_s ts1 = 0, ts2 = 0;
  ts1 = rdtscp();

  NVLog_s* smallest_log;
  for (i = next_log; ; i = (i + 1) % TM_nb_threads) {
    NVLog_s* log = NH_global_logs[i];
    if (log->start != log->end) {
      smallest_log = log;
      break;
    }
  }

  ts2 = rdtscp();
  NH_manager_order_logs += ts2 - ts1;

  i = smallest_log->start;
  while (i != smallest_log->end) {
    NVLogEntry_s entry = smallest_log->ptr[i];
    ts_s ts_val = entry_is_ts(entry);
    if (ts_val) {

      i = ptr_mod_log(i, 1);
      smallest_log->start_tx = -1;
      smallest_log->start = i;
      break;
    }

    if (entry_is_update(entry)) {
      if (to_flush) {
        // apply the flush lazily
        intptr_t cl_addr = (intptr_t) entry.addr;
        cl_addr >>= 6; // remove the log_2(CL_SIZE) bits (offset)
        cl_addr <<= 6;
        to_flush->insert(cl_addr);
      }
      // writes
      LOG_AUX_apply_to_checkpoint(entry.addr, entry.value, do_flush);
    }

    i = ptr_mod_log(i, 1);
  }

  return 0;
}

int LOG_check_correct_ts(int tid)
{
  return check_correct_ts(tid);
}

int LOG_try_lock()
{
  return __sync_bool_compare_and_swap(&log_lock, 0, 1);
}

void LOG_unlock()
{
  log_lock = 0;
  __sync_synchronize();
}

int LOG_get_nb_spins()
{
  return total_spins;
}

ts_s LOG_first_ts(int tid, int *entry_pos)
{
  int i = -1;
  ts_s res;
  res = first_ts(tid, i);
  if (entry_pos != NULL) *entry_pos = i;
  return res;
}

ts_s LOG_last_ts(int tid)
{
  NVLog_s *log = NH_global_logs[tid];
  // log->end is not part of the log
  int i = ptr_mod_log(log->end, -1);

  while (i != log->start) {
    ts_s ts_val = entry_is_ts(log->ptr[i]);
    if (ts_val) {
      return ts_val;
    }
    i = ptr_mod_log(i, -1);
  }

  // log->start is still part of the log
  ts_s ts_val = entry_is_ts(log->ptr[i]);
  if (ts_val) {
    return ts_val;
  }

  return 0;
}

int LOG_last_commit_idx(int tid)
{
  NVLog_s *log = NH_global_logs[tid];
  int i = log->end;

  if (log->start_tx != -1) {
    return log->start_tx;
  }

  while (i != log->start) {
    i = ptr_mod_log(i, -1); // end is not part of the log
    if (entry_is_commit(log->ptr[i])) {
      return i;
    }
  }

  return log->start;
}

// returns the ts of the next transaction with a larger ts in the log
// pos is updated to that transaction (after the previous ts)
ts_s LOG_AUX_next_ts(NVLog_s *log, ts_s ts, int *pos)
{
  int i = log->start, log_end = log->end;
  ts_s res = 0;

  if (pos != NULL && *pos >= 0 && distance_ptr(i, log_end) >
  distance_ptr(*pos, log_end)) {
    // pos has a hint
    i = *pos;
  }
  else if (pos != NULL) {
    *pos = i;
  }

  while (i != log_end) {
    ts_s ts_val = entry_is_ts(log->ptr[i]);
    if (ts_val && ts_val > ts) {
      res = ts_val;
      return res;
    } else if (ts_val && ts_val <= ts && pos != NULL) {
      *pos = ptr_mod_log(i, 1); // lets find the next ts
    }

    i = ptr_mod_log(i, 1);
  }

  if (res == 0 && pos) *pos = -1;

  return res;
}

ts_s LOG_next_tx_ts(int *pos, int* tid) {
  ts_s res = ULLONG_MAX;
  int i, pos_i;

  for (i = 0; i < TM_nb_threads; ++i) {
    ts_s new_ts = first_ts(i, pos_i);

    // printf("[%i:0x%016llx]", i, new_ts);

    if (new_ts != 0 && new_ts < res) {
      res = new_ts;
      *pos = pos_i;
      *tid = i;
    }
  }
  // printf("\n");

  if (res == ULLONG_MAX) {
    res = 0;
  }

  return res;
}

int LOG_next_tx(int *pos, int* tid) {
  ts_s res = ULLONG_MAX;
  int i, pos_i;

  for (i = 0; i < TM_nb_threads; ++i) {
    ts_s new_ts = first_ts(i, pos_i);

    // printf("[%i:0x%016llx]", i, new_ts);

    if (new_ts != 0 && new_ts < res) {
      res = new_ts;
      *pos = pos_i;
      *tid = i;
    }
  }
  // printf("\n");

  if (res == ULLONG_MAX) {
    res = 0;
  }

  return res;
}

int LOG_spin_per_write()
{
  int nb_writes = LOG_count_writes(TM_tid_var);
  return SPIN_PER_WRITE(nb_writes);
}

int LOG_redo_threads()
{
  if(sorted_logs.size() == 0) {
    LOG_AUX_sort_logs();
  }
  return sorted_logs.size();
}

int LOG_is_logged_tx()
{
  empty_to_sorted();
  return sorted_logs.size() > 0;
}

int LOG_is_logged_tx2()
{
  int i;

  for (i = 0; i < TM_nb_threads; ++i) {
    NVLog_s *log = NH_global_logs[i];
    if (log->start != log->end) {
      return 1;
    }
  }
  return 0;
}

int LOG_is_valid(int tid) {
  NVLog_s *log = NH_global_logs[tid];
  int end = log->end;
  int start = log->start;
  ts_s last_ts = 0;

  while (start != end) {
    ts_s new_ts = entry_is_ts(log->ptr[start]);

    if (new_ts) {
      if (last_ts && last_ts > new_ts) {
        return 0;
      }
      last_ts = new_ts;
    }

    start = ptr_mod_log(start, 1);
  }

  return 1;
}

double LOG_capacity_used()
{
  double total = LOG_local_state.size_of_log * TM_nb_threads;
  double cap = 0;
  int i;

  for (i = 0; i < TM_nb_threads; ++i) {
    NVLog_s *log = NH_global_logs[i];
    cap += distance_ptr(log->start, log->end);
  }

  return cap / total;
}

long long int LOG_total_used()
{
  long long int cap = 0;
  int i;

  for (i = 0; i < TM_nb_threads; ++i) {
    NVLog_s *log = NH_global_logs[i];
    cap += distance_ptr(log->start, log->end);
  }

  return cap;
}

int LOG_AUX_sort_logs()
{
  int i;

  empty_logs.clear();
  sorted_logs.clear();

  //    cout << "sort from thread " << this_thread::get_id() << " \n";

  log_mtx.lock(); // TODO: should not exist any concurrency accessing this
  for (i = 0; i < TM_nb_threads; ++i) {
    NVLog_s *log = NH_global_logs[i];
    ts_s ts;
    int entry_ptr;
    ts = first_ts(log, entry_ptr);

    if (ts == 0) {
      empty_logs[i] = log;
    }
    else {
      sorted_logs[ts] = log;
    }
  }
  log_mtx.unlock();

  return 1;
}

size_t LOG_base2_before(size_t size_of_log)
{
  double new_exp = log((double)size_of_log) / log(2.0);
  size_t new_size = llround(pow(2.0, (size_t)new_exp));
  return new_size;
}

NVLog_s* LOG_init_1thread(void *log_pool, size_t max_size)
{
  char *aux_ptr;
  int i;
  NVLog_s *new_log;
  size_t size_of_struct = sizeof(NVLog_s);
  size_t size_of_log = max_size - size_of_struct;
  double max_nb_entries = (double)size_of_log / (double)sizeof(NVLogEntry_s);
  size_t new_size_log = LOG_base2_before(max_nb_entries);

  // printf("size of 1 log: %zu\n", new_size_log);

  aux_ptr = (char*) log_pool;
  new_log = (NVLog_s*) aux_ptr;
  aux_ptr += size_of_struct;
  new_log->ptr = (NVLogEntry_s*) aux_ptr;
  new_log->size_of_log = new_size_log;
  new_log->start = new_log->end = 0; // TODO: recovery
  aux_ptr += size_of_log;

  return new_log;
}

// ################ implementation local functions

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
static int empty_to_sorted()
{
  if (TM_nb_threads != (sorted_logs.size() + empty_logs.size())) {
    LOG_AUX_sort_logs();
  }

  map<int, NVLog_s*>::iterator it,
  begin = empty_logs.begin(),
  end = empty_logs.end();
  list<int>::iterator del_it, del_begin, del_end;
  list<int> toRemove;

  toRemove.clear();

  for (it = begin; it != end; ++it) {
    NVLog_s *log = it->second;
    ts_s ts;
    int entry_ptr;
    ts = first_ts(log, entry_ptr);

    if (ts != 0) {
      toRemove.push_back(it->first);
      sorted_logs[ts] = log;
    }
  }

  del_begin = toRemove.begin();
  del_end = toRemove.end();

  for (del_it = del_begin; del_it != del_end; ++del_it) {
    it = empty_logs.find(*del_it);
    if (it != empty_logs.end()) {
      empty_logs.erase(it);
    }
  }

  return 1; // TODO: what was to return?
}

static int check_correct_ts(int tid)
{
  int id = TM_tid_var;
  NVLog_s *log = tid == id ? nvm_htm_local_log : NH_global_logs[id];
  ts_s last_ts = 0;

  int i = log->start;

  // TODO: refactor
  if (log->end_last_tx < 0 || log->end_last_tx >
    LOG_local_state.size_of_log
  ) {
    log->end_last_tx = log->end;
  }

  while (i != log->end_last_tx) {
    NVLogEntry_s entry = log->ptr[i];
    ts_s ts_val = entry_is_ts(entry);
    if (ts_val && last_ts != 0 && ts_val < last_ts) {
      // error inverted ts
      return 0;
    }
    last_ts = ts_val;
    ++i;
  }
  return 1;
}

static inline ts_s first_ts(int tid, int &entry_ptr)
{
  NVLog_s *log = NH_global_logs[tid];
  ts_s res = first_ts(log, entry_ptr);

  return res;
}

static inline ts_s first_ts(NVLog_s *log, int &entry_ptr)
{

  int i = log->start; // start

  // return 0; // TODO: this causes conflicts

  // TODO: refactor
  while (i != log->end) {
    ts_s ts_val = entry_is_ts(log->ptr[i]);
    if (ts_val) {
      entry_ptr = i;
      return ts_val;
    }

    i = ptr_mod_log(i, 1);
  }

  entry_ptr = -1;
  return 0;
}

#include "utils.h"
#include "log.h"
#include "log_sorter.h"
#include "log_backward.h"
#include "log_forward.h"
#include "nvhtm_helper.h"
#include "cp.h"

#include "nh_globals.h"

#include "rdtsc.h"

#include <cstdlib>
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
#include <set>
#include <list>
#include <thread>
#include <mutex>

/**
* TODO: Set log size to power of 2, then use bitwise operations.
*/

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
extern CL_ALIGN map<ts_s, NVLog_s*> sorted_logs; // from log_aux
extern CL_ALIGN map<int, NVLog_s*> empty_logs;
extern CL_ALIGN cp_s *cp_instance;

static CL_ALIGN mutex log_mtx;
static CL_ALIGN int log_lock;

static CL_ALIGN int nb_applied_txs = 0; // DEBUG

// ################ variables (thread-local)

//__thread CL_ALIGN NVLog_s nvm_htm_local_log_inst; // DOESN'T WORK!!! why?
// __thread CL_ALIGN size_t nvm_htm_log_size;

// ################ functions

// TODO: repeated functions in log_aux.cpp

static inline void* apply_to_checkpoint(
  NVMHTM_mem_s *instance,
  void *addr,
  GRANULE_TYPE value,
  bool do_flush
);

static void init_log(NVLog_s *new_log, int tid, int fresh);
static int sort_logs();

// ################ implementation header

void LOG_init(int nb_threads, int fresh)
{
  int i;
  size_t size_of_logs;

  size_of_logs = (unsigned long)(NVMHTM_LOG_SIZE /* / TM_nb_threads */) * TM_nb_threads;

  #if defined(SORT_ALG) && SORT_ALG == 4
  // TODO: must be multiple of 2
  cp_instance = cp_init(0x10000, sizeof(NVLogLocation_s));
  #endif

  TM_nb_threads = nb_threads;
  // nvm_htm_log_size = NVMHTM_LOG_SIZE / nb_threads; // TODO

  printf("Number of threads: %i\n", TM_nb_threads);

  if (NH_global_logs == NULL) {
    ALLOC_FN(NH_global_logs, NVLog_s*, CACHE_LINE_SIZE * nb_threads);

    #if DO_CHECKPOINT == 1 || DO_CHECKPOINT == 5
    key_t key = KEY_LOGS;

    int shmid = shmget(key, size_of_logs, 0777 | IPC_CREAT);
    // first detach, reallocation may fail
    // shmctl(shmid, IPC_RMID, NULL);
    // shmid = shmget(key, NVMHTM_LOG_SIZE, 0777 | IPC_CREAT);

    LOG_global_ptr = shmat(shmid, (void *)0, 0);
    memset(LOG_global_ptr, 0, size_of_logs);

    if (shmid < 0) {
      perror("shmget");
    }

    LOG_global_ptr = shmat(shmid, (void *)0, 0);
    fresh = 1; // this is not init to 0

    if (LOG_global_ptr < 0) {
      perror("shmat");
    }
    #else
    LOG_global_ptr = ALLOC_MEM(LOG_FILE, size_of_logs);
    #endif
  }

  LOG_attach_shared_mem();
  for (i = 0; i < nb_threads; ++i) {
    NVLog_s *new_log = NH_global_logs[i];
    init_log(new_log, i, fresh);
  }

  sort_logs(); // TODO
  #if defined(SORT_ALG) && SORT_ALG == 4
  LOG_SOR_init(cp_instance, NH_global_logs, nb_threads);
  #endif
}

void LOG_attach_shared_mem() {
  char *aux_ptr;
  int i;
  size_t size_of_struct = sizeof(NVLog_s);
  size_t size_of_log = NVMHTM_LOG_SIZE /* / TM_nb_threads */;
  aux_ptr = (char*) LOG_global_ptr;
  for (i = 0; i < TM_nb_threads; ++i) {
    NVLog_s *new_log = LOG_init_1thread(aux_ptr, size_of_log);
    aux_ptr += size_of_log;
    NH_global_logs[i] = new_log;
  }
}

void LOG_get_ts_before_tx(int tid)
{
  ts_s ts = rdtscp();
  if (ts > NH_before_ts[tid]) {
    NH_before_ts[tid] = ts;
    asm volatile ("sfence" ::: "memory");
  }
}

void LOG_alloc(int tid, const char *pool_file, int fresh)
{
  NVLog_s *new_log;
  char logfile[512];
  int i;

  char *aux_ptr;
  size_t size_of_struct = sizeof(NVLog_s);
  size_t used_size = NVMHTM_LOG_SIZE /* / TM_nb_threads*/;

  sprintf(logfile, "%s" LOGS_EXT, pool_file, tid);
  new_log = (NVLog_s*) ALLOC_MEM(logfile, used_size);
  aux_ptr = (char*)new_log;
  LOG_init_1thread(aux_ptr, used_size);
  aux_ptr += used_size;
  NH_global_logs[i] = new_log;

  init_log(new_log, tid, fresh);
}

void LOG_thr_init(int tid)
{
  nvm_htm_local_log = NH_global_logs[tid];
  LOG_local_state.size_of_log = nvm_htm_local_log->size_of_log;
}

void LOG_clear(int tid)
{
  NVLog_s *log = NH_global_logs[tid];

  log_mtx.lock();
  SET_START(log, 0);
  log->end = 0;
  sorted_logs.clear();
  empty_logs.clear();
  log_mtx.unlock();
}

int LOG_has_new_writes(int tid)
{
  NVLog_s *log = NH_global_logs[tid];
  int last_entry = ptr_mod_log(log->end, -1);

  return !entry_is_commit(log->ptr[last_entry]);
}

NVLog_s *LOG_get(int tid)
{
  int id = TM_tid_var;
  NVLog_s *log = tid == id ? nvm_htm_local_log : NH_global_logs[tid];

  return log;
}

void LOG_push_malloc(int tid, GRANULE_TYPE *addr) {
  int id = TM_tid_var;
  NVLog_s *log = tid == id ? nvm_htm_local_log : NH_global_logs[tid];
  int end = log->end, new_end;
  NVLogEntry_s entry;
  entry.addr = (GRANULE_TYPE*) LOG_MALLOC;
  entry.value = (GRANULE_TYPE) addr;

  new_end = LOG_push_entry(log, entry);
}

int LOG_checkpoint_apply_one()
{
  return LOG_AUX_apply_one_to_checkpoint(true, true, NULL);
}

int LOG_checkpoint_apply_one2()
{
  return LOG_AUX_apply_one_to_checkpoint2(true, true, NULL);
}

int LOG_checkpoint_apply_one3()
{
  return LOG_AUX_apply_one_to_checkpoint3(true, true, NULL);
}

int LOG_checkpoint_backward()
{
  return LOG_checkpoint_backward_apply_one();
}

void LOG_fake_advance_ptrs(int n)
{
  int i;

  for (i = 0; i < TM_nb_threads; ++i) {
    // lazy flush
    NVLog_s* log = NH_global_logs[i];

    log->start = log->end_last_tx;
  }
}

void LOG_checkpoint_apply_N(int n)
{
  set<intptr_t> to_flush;
  int i = n;

  for (i = 0; i < n; ++i) {
    // lazy flush
    if (LOG_AUX_apply_one_to_checkpoint2(true, true, &to_flush)) {
      break;
    }
  }

  // flushes the checkpoint
  SPIN_PER_WRITE(to_flush.size()); // simulation
  // TODO: we do not need to spin all this time (pay the cost of a memfence only!)
}

void LOG_checkpoint_apply_N_update_after(int n)
{
  set<intptr_t> to_flush;
  int i = n;

  for (i = 0; i < n; ++i) {
    // lazy flush
    if (LOG_AUX_apply_one_to_checkpoint2(false, false, &to_flush)) {
      break;
    }
  }

  // flushes the checkpoint
  SPIN_PER_WRITE(to_flush.size()); // simulation

  for (i = 0; i < TM_nb_threads; ++i) {
    NH_global_logs[i]->start;
  }
}

void LOG_move_start_ptrs()
{
  int i;

  for (i = 0; i < TM_nb_threads; ++i) {
    NH_global_logs[i]->start = NH_global_logs[i]->start;
  }
  __sync_synchronize(); // must show the log ptr
}

void LOG_handle_checkpoint()
{
  // int applied_in_checkpoint = 0;
  while (sorted_logs.size() > 0) { // TODO: have in account the capacities
    LOG_checkpoint_apply_one();
    // applied_in_checkpoint++;
  }
  // printf("\n ------------------------------------------- \n"
  // " Recovery applied %i transactions            \n"
  // " ------------------------------------------- \n\n",
  // applied_in_checkpoint);
}

// ################ implementation local functions

static inline int find_minimum(int *ts, size_t size)
{
  int i, min = ts[0], min_i = 0;
  for (i = 1; i < size; ++i) {
    if (min == 0 || ts[i] < min) {
      min = ts[i];
      min_i = i;
    }
  }

  return min_i;
}

// DEPRECATED

static int sort_logs()
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
    ts = LOG_first_ts(i, &entry_ptr);

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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
static void init_log(NVLog_s *new_log, int tid, int fresh)
{
  new_log->tid = tid;
  //if (fresh || new_log->start >= new_log->size_of_log
  //        || new_log->end >= new_log->size_of_log) {
  new_log->start = 0;
  new_log->end = 0;
  new_log->end_last_tx = -1;
  //}
  new_log->start_tx = -1;

  // printf("Init log %i\n", new_log->end);
  // TODO: loads a bit of the log to cache

  NH_global_logs[tid] = new_log;
}

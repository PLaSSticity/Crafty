#include "nvhtm_helper.h"
#include "log.h"
#include "log_sorter.h"
#include "tm.h"
#include "nh.h"
#include "utils.h"

// TODO: count time flush
#include "rdtsc.h"
#include "arch.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <list>
#include <vector>
//#include <algorithm>
#include <climits>
#include <utility>
#include <bitset>

#include <sys/time.h> // POSIX only

#include <unistd.h>
#include <sys/types.h> // POSIX only
#include <sys/un.h>

#include <sys/shm.h>
#include <sys/mman.h>

#include <signal.h>


// ################ defines

#define ROOT_FILE "./libnvmhtm.root.state"
#define THR_LOG_FILE "./libnvmhtm.thr"
#define S_INSTANCE_EXT ".state"
#define CHECKPOINT_EXT ".ckp%i"

/**
* TODO: describe the file,
*
* Logs are created per thread, and not per pool.
*/

// TODO: this is extern
// ts_s NH_before_ts[1024];

/* A memory pool is composed as follows:
*
* | 4/8 bytes id | size_given pool |
*
* the id is used to manage different pools.
*/

// TODO: add optional volatile memory

#define ALLOC_WITH_INSTANCE(instance, ptr, file, size) ({ \
  ptr = ALLOC_MEM(file, size); /* TODO: is_vol == set to 0 */ \
  tmp_allocs->push_back(instance); \
  size; \
})

#define INSTANCE_FROM_POOL(pool) ({ \
  NVMHTM_mem_s *res = NULL; \
  map<void*, NVMHTM_mem_s*>::iterator it; \
  /* // mtx.lock(); */ \
  if ((it = instances.find(pool)) != instances.end()) { \
    res = it->second; \
  } \
  /* // mtx.unlock(); */ \
  res; \
})

using namespace std;

// ################ types

struct NVMHTM_root_
{
  int pool_counter;
};

enum FLAGS
{
  EMPTY = 0, IN_RECOVERY = 1
};

enum INTER_PROCESS_MESSAGES
{
  REQ_CLEAN_LOG = 1, RES_DONE_CLEANING = 2, REQ_KILL = 3
};

// ################ variables (extern)
CL_ALIGN map<void*, NVMHTM_mem_s*> instances; // extern

// ################ variables

// TODO: remove thread_local
//thread_local ID_TYPE nvm_htm_thr_id = 0;

// set_threads is set in init_thrs (before launching the threads)
// nb_thrs is a counter to give ids
static CL_ALIGN int nb_thrs;
static CL_ALIGN int set_threads;
// compute checkpoint addresses with this

static CL_ALIGN mutex malloc_mtx;
static CL_ALIGN ts_s count_val_wait[MAX_NB_THREADS];

static int manager_mutex = 0;
volatile int *NH_checkpointer_state; // extern
sem_t *NH_chkp_sem; // extern
static int exit_success = 0;

static int is_exit;
static thread *checkpoint_manager;
static int sockfd_client = -1;
static int sockfd_log_manager;

static int background_process_is_exit = 0;

static pid_t pid;

static ts_s time_chkp_1, time_chkp_2, time_chkp_total;
static ts_s time_o_chkp_1, time_o_chkp_2;

#ifdef DO_CHECKPOINT
NVMHTM_mem_s *mem_instance;
#endif /* DO_CHECKPOINT */

mutex mtx;

// ################ variables (thread-local)
static __thread CL_ALIGN vector<NVMHTM_mem_s*> *tmp_allocs; // transactional allocs go here
static __thread CL_ALIGN vector<void*> *tmp_frees;
static __thread CL_ALIGN char pad1[CACHE_LINE_SIZE];
static __thread CL_ALIGN char pad2[CACHE_LINE_SIZE];
static __thread CL_ALIGN void *malloc_ptr;
static __thread CL_ALIGN int retries_counter = 0;
// ################ functions
// vector<map<void*, NVMHTM_mem_s*>> allocs; // extern

static void dangerous_threads(int id, /*bitset<MAX_NB_THREADS> &*/ uintptr_t *threads, int nbThreads, ts_s my_global, int waitForTX);

static void NVMHTM_validate(int, /*bitset<MAX_NB_THREADS> &*/ uintptr_t *threads, int nbThreads, ts_s my_global);

// create a delete_thr, and handle logs
static void fork_manager(void);
static void* manage_checkpoint(void*);
static int loop_checkpoint_manager();
static void * server(void * args);
static void segfault_sigaction(int signal, siginfo_t *si, void *context);
static void segint_sigaction(int signal, siginfo_t *si, void *context);
static void aux_thread_stats_to_gnuplot_file(char *filename);

// ################ implementation header

void* NVMHTM_alloc(const char *file_name, size_t size, int vol_pool)
{
  int i;
  NVMHTM_mem_s *instance;
  void *pool;
  char chkp_file[512];
  char state_file[512];

  int in_htm = HTM_test(); // TODO:

  // printf("size of alloc=%i\n", size);

  if (tmp_allocs == NULL) {
    tmp_allocs = new vector<NVMHTM_mem_s*>();
    tmp_frees = new vector<void*>();
  }

  // instance
  // sprintf(state_file, "%s" S_INSTANCE_EXT, file_name);
  // instance = (NVMHTM_mem_s*) ALLOC_MEM(state_file, sizeof (NVMHTM_mem_s));
  instance = (NVMHTM_mem_s*) ALLOC_MEM(file_name, sizeof (NVMHTM_mem_s));

  // mem_pool also adds the extra id
  ALLOC_WITH_INSTANCE(instance, pool, file_name, size);

  instance->size = size;
  instance->ptr = pool;
  instance->last_alloc = pool;
  // strcpy(instance->file_name, file_name);

#ifdef DO_CHECKPOINT
  // sprintf(chkp_file, "%s" CHECKPOINT_EXT, file_name,
  // (int) (instance->chkp_counter));
  // instance->chkp.ptr = ALLOC_MEM(chkp_file, size);
  instance->chkp.ptr = ALLOC_MEM(file_name, size);
  instance->chkp.size = size;
  instance->chkp.delta_to_pool =
    CALC_DELTA(instance->chkp.ptr, instance->ptr);

  // printf("size of checkpoint=%i\n", size);

  if (vol_pool) { // TODO: i'm using volatile memory
    instance->ts = 0; // not used
  }
#endif /* DO_CHECKPOINT */

  if (!in_htm) {
    NVMHTM_apply_allocs();
    /*__sync_synchronize();*/
  }

  return pool;
}

// TODO: free and thread safe track of freed segments

void* NVMHTM_malloc(void *pool, size_t size)
{
  NVMHTM_mem_s *instance = INSTANCE_FROM_POOL(pool);
  char *ptr;
  void *res;
  intptr_t diff_ptrs, size_per_thrs, beg_seg;
  int tid = TM_tid_var;

  // TODO: threads are allocating in different areas!
  // may overlap

  int in_htm = HTM_test();

  if (!in_htm) {
    malloc_mtx.lock();
  }

  if (malloc_ptr != NULL) {
    ptr = (char*) malloc_ptr;
  }
  else {
    ptr = (char*) instance->last_alloc;
  }

  res = ptr;

  diff_ptrs = ptr - (char*) instance->ptr;
  size_per_thrs = instance->size / set_threads;
  beg_seg = (tid + 1) * size_per_thrs;

  // TODO: move this to asserts?
  if (diff_ptrs + size > instance->size) {
    // out of memory
    fprintf(stderr, "OUT OF MEMORY!\n");
    fflush(stderr);
    res = NULL;
  }
  else if (diff_ptrs > beg_seg) {
    fprintf(stderr, "Thread memory allocation overlap!\n");
    fflush(stderr);
    res = NULL;
  }

  ptr += size;

  // TODO: put a #ifdef or so
  if (malloc_ptr != NULL) {
    malloc_ptr = (void*) ptr;
  }
  else {
    instance->last_alloc = ptr;
  }

  if (!in_htm) {
    malloc_mtx.unlock();
  }

  return res;
}

void NVMHTM_apply_allocs()
{
  return; // TODO: I think we will use this no more

  vector<NVMHTM_mem_s*>::iterator it;
  vector<void*>::iterator it2;

  if (tmp_allocs == NULL) {
    tmp_allocs = new vector<NVMHTM_mem_s*>();
    tmp_frees = new vector<void*>();
    return; // no allocs
  }

  if (tmp_allocs->empty()) {
    return; // DO NOT LOCK!
  }

  mtx.lock();
  for (it = tmp_allocs->begin(); it != tmp_allocs->end(); ++it) {
    instances[(*it)->ptr] = *it;
  }
  mtx.unlock();

  for (it2 = tmp_frees->begin(); it2 != tmp_frees->end(); ++it2) {
    NVMHTM_free(*it2);
  }

  tmp_allocs->clear();
  tmp_frees->clear();

  __sync_synchronize();
}

NVMHTM_mem_s* NVMHTM_get_instance(void* pool)
{
  if (pool == NULL) {
    return NULL;
  }

  // mtx.lock();
  NVMHTM_mem_s* res = INSTANCE_FROM_POOL(pool);
  // mtx.unlock();

  NVMHTM_mem_s *instance;
  void *base;

  if (res == NULL && tmp_allocs != NULL) {
    // first check the tmp list of allocs
    vector<NVMHTM_mem_s*>::iterator it;
    for (it = tmp_allocs->begin(); it != tmp_allocs->end(); ++it) {
      instance = *it;
      base = instance->ptr;

      if (pool >= base && pool < ((char*) base + instance->size)) {
        res = instance;
        break;
      }
    }
  }

  if (res == NULL) {
    mtx.lock(); // TODO: this is really bad!!!
    // is not the base addr, check against the map
    map<void*, NVMHTM_mem_s*>::iterator it;

    // TODO: find the largest addr smaller than pool
    it = instances.lower_bound(pool);
    if (it != instances.begin()) {
      --it;
      instance = it->second;
      base = it->first;
      if (pool >= base && pool < ((char*) base + instance->size)) {
        res = instance;
      }
    }
    mtx.unlock();
  }

  return res;
}

void NVMHTM_thr_init(void *pool)
{
  int my_tid = TM_tid_var;

  // mtx.lock();
  // gather an id, from 0 to nb_thrs to the current thread

  LOG_thr_init(my_tid);

  if (tmp_allocs == NULL) {
    tmp_allocs = new vector<NVMHTM_mem_s*>();
    tmp_frees = new vector<void*>();
  }

  if (pool != NULL) {
    // use a huge pool from which allocate memory

    NVMHTM_mem_s *instance = INSTANCE_FROM_POOL(pool);
    int block_size = instance->size / set_threads;

    // if there are mallocs before this is called
    block_size -= (char*) instance->ptr - (char*) instance->last_alloc;
    malloc_ptr = (char*) instance->last_alloc + my_tid * block_size;
  }

  // mtx.unlock();
}

void NVMHTM_thr_exit()
{
  mtx.lock();
  NH_time_blocked_total += NH_time_blocked;
  NH_count_blocks_total += NH_count_blocks;
  mtx.unlock();
}

void NVMHTM_init_thrs(int nb_threads)
{
  static bool is_started = false;
  set_threads = nb_threads;
  nb_thrs = set_threads; // not used
  LOG_init(nb_threads, 0);

  is_exit = 0;

  NH_before_ts = (ts_s*)malloc(sizeof(ts_s)*1024);

  key_t key = KEY_SEMAPHORE;
  int shmid = shmget(key, sizeof (sem_t), 0777 | IPC_CREAT);
  shmctl(shmid, IPC_RMID, NULL);
  shmid = shmget(key, sizeof (sem_t), 0777 | IPC_CREAT);

  if (shmid < 0) {
    perror("shmget CHKP_STATE");
  }

  NH_chkp_sem = (sem_t*) shmat(shmid, (void *) 0, 0);
  sem_init(NH_chkp_sem, 1, 1);

  key = KEY_CHKP_STATE;
  shmid = shmget(key, sizeof (int), 0777 | IPC_CREAT);
  shmctl(shmid, IPC_RMID, NULL);
  shmid = shmget(key, sizeof (int), 0777 | IPC_CREAT);

  if (shmid < 0) {
    perror("shmget SEMAPHORE");
  }

  NH_checkpointer_state = (volatile int*) shmat(shmid, (void *) 0, 0);

  if (NH_checkpointer_state == NULL) {
    perror("shmat SEMAPHORE");
  }

  *NH_checkpointer_state = 0;

  if (!is_started) {
    is_started = true;
    #if DO_CHECKPOINT == 1
    launch_thread_at(MAX_PHYS_THRS - 1, manage_checkpoint);
    #if SORT_ALG == 4
    launch_thread_at(MAX_PHYS_THRS - 2, LOG_SOR_main_thread);
    #endif /* Sorting thread */
    #else
    exit_success = 1; // TODO: put this global
    #endif
  }
}

void NVMHTM_clear()
{
  int i;

  for (i = 0; i < nb_thrs; ++i) {
    LOG_clear(i);
  }
}

void NVMHTM_zero_pool(void *pool)
{
  // mtx.lock();
  NVMHTM_mem_s *instance = INSTANCE_FROM_POOL(pool);
  // mtx.unlock();
  memset(instance->ptr, 0, instance->size);
  #ifdef DO_CHECKPOINT
  memset(instance->chkp.ptr, 0, instance->chkp.size);
  #endif
}

int NVMHTM_has_writes(int tid)
{
  int id = tid;

  return LOG_has_new_writes(id);
}

void NVMHTM_shutdown()
{
  int i;

  is_exit = 1; // stops checkpoint manager

  #if DO_CHECKPOINT == 5
  int request = REQ_KILL;

  kill(pid, SIGINT);
  #endif

  __sync_synchronize();

  while (!exit_success) {
    sem_post(NH_chkp_sem);
    PAUSE(); // wait manager to exit
  }

  double time_taken = (double) NVHTM_get_total_time();

  printf("--- Percentage time blocked %f \n", ((double) NH_time_blocked_total
  / (double) CPU_MAX_FREQ / 1000.0D) / (double) TM_nb_threads / time_taken);
  printf("--- Nb. checkpoints %lli\n", NH_nb_checkpoints);
  printf("--- Time blocked %e ms!\n", (double) time_chkp_total / ((double) CPU_MAX_FREQ));
}

void NVMHTM_write_ts(int id, ts_s ts)
{
  LOG_push_ts(id, ts);
}

void NVMHTM_commit(int id, ts_s ts, int nb_writes)
{
  // printf("commit %llu\n", ts);
  /*if (nb_writes == 0) return; // done */ // this is check in the AFTER_TRANSACTION

  // bitset<MAX_NB_THREADS> threads_set;
  static thread_local uintptr_t threads_set;
  ts_s my_global = htm_tx_val_counters[id].global_counter;

  #if VALIDATION == 3
  int nb_threads = TM_get_nb_threads();
  if (nb_threads > 1) {
    threads_set = (1 << nb_threads) - 1;
    threads_set &= (~(1 << id));
    // threads_set << all_dang; // assume all dangerous initially
    dangerous_threads(id, &threads_set, nb_threads, my_global, 0);
  }
  #endif

  // TODO: disabling emulation
  int distance = distance_ptr(NH_global_logs[id]->start_tx, NH_global_logs[id]->end);
  int nbClsToFlush = distance / (64/sizeof(NVLogEntry_s));
  if (nbClsToFlush == 0 || nbClsToFlush > 10) nbClsToFlush = 1; // BUG: !!!
  //if (nbClsToFlush > 1) fprintf(stderr, "clflush %i lines, dist = %i start = %i end = %i\n", nbClsToFlush, distance, NH_global_logs[id]->start_tx, NH_global_logs[id]->end);
  uintptr_t addr_to_flush =(uintptr_t) &(NH_global_logs[id]->ptr[NH_global_logs[id]->start_tx]);
  addr_to_flush &= ~((uintptr_t)((1 << 6)-1));
  for (int i = 0; i < nbClsToFlush; i++) {
    asm(""); // TODO: kaan: Removed CLFLUSH from here
    addr_to_flush += 64;
  }
  // -------------------------

  // flush entries before write TS (does not need memory barrier)
  SPIN_PER_WRITE(MAX(nb_writes * sizeof(NVLogEntry_s) / CACHE_LINE_SIZE, 1));
  // int log_before = ptr_mod_log(NH_global_logs[id]->end, -nb_writes);
  // MN_flush(&(NH_global_logs[id]->ptr[log_before]),
  //   nb_writes * sizeof(NVLogEntry_s), 0
  // );

  #ifndef DISABLE_VALIDATION
  int nb_threads_ = TM_get_nb_threads();
  if (nb_threads_ > 1) {
    NVMHTM_validate(id, &threads_set, nb_threads_, my_global);
  }
  #endif

  // good place for a memory barrier

  NVMHTM_write_ts(id, ts); // Flush all together
  //clflush(&(NH_global_logs[id]->ptr[NH_global_logs[id]->end]));
  // TODO: kaan: Removed CLFLUSH from here
  SPIN_PER_WRITE(1);
  // log_before = ptr_mod_log(NH_global_logs[id]->end, -1);
  // MN_flush(&(NH_global_logs[id]->ptr[log_before]),
  //   sizeof(NVLogEntry_s), 0
  // );
  #if VALIDATION == 2 && !defined(DISABLE_VALIDATION)
  global_flushed_ts++;
  // __sync_synchronize(); // Is not working! need the fence in the while loop!
  #endif

  //asm volatile ("sfence" ::: "memory"); /*__sync_synchronize();*/
}

void NVMHTM_free(void *ptr)
{
  // TODO: unmap memory, deal with stuff stored
  // TODO: use global lock

  // first check the tmp list and remove from there if necessary
  NVMHTM_mem_s *instance = NULL;
  NVMHTM_mem_s *tmp;
  void *base;
  // TODO: refactor with NVMHTM_get_instance

  int in_htm = HTM_test();

  if (in_htm) {
    // postpone operation
    tmp_frees->push_back(ptr);
    return;
  }

  instance = NVMHTM_get_instance(ptr);
  if (instance != NULL) {

    // mtx.lock();
    instances.erase(instance->ptr);
    // mtx.unlock();

    FREE_MEM(instance->ptr, instance->size);
    FREE_MEM(instance->chkp.ptr, instance->size);
    FREE_MEM(instance, sizeof (NVMHTM_mem_s));
  }
}

void NVMHTM_copy_to_checkpoint(void *pool)
{
  static bool is_started = false;
  // TODO: This could be the FORK

  if (is_exit) {
    return;
  }

  if (!is_started) {
    is_started = true;
    #if DO_CHECKPOINT == 5
    fork_manager();
    #elif defined(DO_CHECKPOINT)
    NVMHTM_mem_s *instance = INSTANCE_FROM_POOL(pool);
    int i;

    if (instance == NULL) {
      // Error! The checkpoint was not found!
      return;
    }

    // mtx.lock();

    for (i = 0; i < nb_thrs; ++i) {
      LOG_clear(i);
    }

    memcpy(instance->chkp.ptr, instance->ptr, instance->size);
    NVM_PERSIST(instance->chkp.ptr, instance->size);

    // mtx.unlock();
    #endif /* DO_CHECKPOINT */
  }
}

// TODO: don't know where the memory pool actually is

void NVMHTM_reduce_logs()
{
  LOG_checkpoint_apply_one(); // TODO: just one?
}

// #if VALIDATION == 2
// 
// static void NVMHTM_validate(int id, bitset<MAX_NB_THREADS>&)
// {
//   int my_global = TM_get_global_counter(id); // TODO: Removing the MEMFENCE blocks the program
//   while (global_flushed_ts < my_global - 1) {
//     __sync_synchronize();
//   }
// }
// #elif VALIDATION == 3

static void NVMHTM_validate(int id, /*bitset<MAX_NB_THREADS>&*/ uintptr_t *threads_set, int nb_threads, ts_s my_global)
{
  int i;
  long nbTries = 0;
  ts_s ts1_wait_log_time = rdtscp();

  do {
    dangerous_threads(id, threads_set, nb_threads, my_global, 1);
    //if (nbTries++ % 10000000 == 9999999)
    //  fprintf(stderr, "[%2i] 0x%4lx NH_before_ts=%llu local=%llu global=%llu\n",
    //    id, *threads_set, NH_before_ts[id], htm_tx_val_counters[id].local_counter, htm_tx_val_counters[id].global_counter);
  }
  while (/*threads_set.any()*/ *threads_set != 0);
  NH_time_validate += rdtscp() - ts1_wait_log_time;
  //fprintf(stderr, "id=%2i finishes waiting for other threads\n", id);
}
//#else /* VALIDATION is not 2 and not 3 */
//
//// old implementation
//
//static void NVMHTM_validate(int id, bitset<MAX_NB_THREADS>&)
//{
//  int i;
//
//  printf("Error! Using old implementation!\n");
//
//  // wait for active uncommitted transactions
//  for (i = 0; i < TM_get_nb_threads(); ++i) { // TODO: replace with macros
//    int local_counter = TM_get_local_counter(i);
//    if (i != id && (local_counter & 1)) {
//      int other_global = TM_get_global_counter(i);
//      int my_global = TM_get_global_counter(id);
//      if (other_global < my_global) {
//        // the guy is running with a smaller global counter
//        // wait
//        // this_thread::yield();
//        --i; // try again for this thread
//        __sync_synchronize();
//        continue; // the spinning is not very energy efficient
//      }
//    }
//  }
//}
//#endif /* VALIDATION */

void NVMHTM_crash()
{
  // TODO
  abort();
}

// ################ implementation local functions

void NVHTM_req_clear_log(/* int block */)
{
  char request[MAXMSG];

  if (!(LOG_THRESHOLD > 0.0D)) return; // threshold is 0 means always run

  request[0] = REQ_CLEAN_LOG;
  request[1] = '\n';
  request[2] = '\0';

  // printf("Send REQ_CLEAN_LOG\n");

  if (NH_checkpointer_state == NULL) {
    // should have forked before
    while (__sync_bool_compare_and_swap(&manager_mutex, 0, 1)) PAUSE();
    //        if (NH_checkpointer_state == NULL) {
    //            fork_manager();
    //        }
    manager_mutex = 0;
    __sync_synchronize();
  }

  if (manager_mutex == 1 || *NH_checkpointer_state == 1) return; // somebody else got it

  while (!__sync_bool_compare_and_swap(&manager_mutex, 0, 1)) {
    if (manager_mutex != 0) // somebody else got it
    return;
  }
  retries_counter++;
  // if MANAGER is IDLE --> grab lock and make it work
  if (*NH_checkpointer_state == 0) {
    // cool, no one is requesting
    *NH_checkpointer_state = 1; // BUSY
    __sync_synchronize();
  }
  manager_mutex = 0;
  __sync_synchronize();
}

static void dangerous_threads(int id, /*bitset<MAX_NB_THREADS> &*/ uintptr_t *threads_set, int nbThreads, ts_s my_global, int waitForTX)
{
  int i;

  for (i = 0; i < nbThreads; ++i) {
    register uintptr_t bitTo1 = (1 << i);

    //if (!threads_set.test(i)) {
    if (!(*threads_set & bitTo1)) { // TODO: make sure i==id is 0
      continue; // not dangerous
    }

    do {
      ts_s local_counter = htm_tx_val_counters[i].local_counter;
      if (i != id && (local_counter & 1)) { // not me and the other is running a TX
        ts_s other_global = htm_tx_val_counters[i].global_counter;
        if (other_global <= my_global || NH_before_ts[i] > my_global) { 
          // the other has a smaller TS or started after
          //threads_set[i] = 1;
          *threads_set &= ~bitTo1; // remove the bit
          break;
        }
      } else {
        *threads_set &= ~bitTo1; // me or the other is not running a TX
        break;
      }
      if (waitForTX) {
        for (volatile int j = 0; j < 10; ++j) {
          asm volatile ("pause\nnop\nnop\nnop" ::: );
        }
        asm volatile ("lfence" ::: "memory");
      }
    } while (waitForTX);
  }
}

static void fork_manager()
{
  #if DO_CHECKPOINT == 5
  *NH_checkpointer_state = 1; // BUSY
  __sync_synchronize();

  pid = fork();

  if (pid == 0) {
    LOG_init(TM_nb_threads, 1); // reattach
    // printf("Maximum supported CPUs: %i\n", MAX_PHYS_THRS);

    if (MAX_PHYS_THRS == 56) {
      set_affinity_at(27);
    } else {
      set_affinity_at(MAX_PHYS_THRS - 1);
    }

    /*
    struct sigaction sa_sigsegv;
    memset(&sa_sigsegv, 0, sizeof(struct sigaction));
    sigemptyset(&sa_sigsegv.sa_mask);
    sa_sigsegv.sa_sigaction = segfault_sigaction;
    sa_.sa_flags     = 0;

    sigaction(SIGSEGV, &sa_sigsegv, NULL); // apply log does not SIGSEGV
    */

    // handles SIGINT from parent
    struct sigaction sa;

    memset(&sa, 0, sizeof (struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segint_sigaction;
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGINT, &sa, NULL); // The parent can SIGINT the child to shutdown

    LOG_attach_shared_mem();

    // sort the logs for faster checkpoint (TODO:)
    #if SORT_ALG == 4
    if (MAX_PHYS_THRS == 56) {
      launch_thread_at(26, LOG_SOR_main_thread);
    } else {
      launch_thread_at(MAX_PHYS_THRS - 2, LOG_SOR_main_thread);
    }
    #endif /* Sorting thread */
    server(NULL);
    exit(EXIT_SUCCESS);
  }

  // printf("PID: %i\n", pid);

  while (*NH_checkpointer_state == 1) PAUSE();
  #endif
}

static void * server(void * args)
{
  char req;

  MN_thr_enter();
  //set_affinity_at(15);
  LOG_local_state.size_of_log = NH_global_logs[0]->size_of_log; // TODO

  while (!background_process_is_exit) {

    req = 0;

    *NH_checkpointer_state = 0; // IDLE
    asm volatile ("sfence" ::: "memory");//__sync_synchronize();

    while (LOG_THRESHOLD > 0.0D && *NH_checkpointer_state == 0 && !background_process_is_exit) {
      PAUSE();
      // __sync_synchronize();
    }

    loop_checkpoint_manager();
  }

  MN_thr_exit();
  sem_post(NH_chkp_sem);

  fprintf(stderr, "outside the loop\n");

  return NULL;
}

static void *manage_checkpoint(void * args)
{
  #if DO_CHECKPOINT == 1
  LOG_local_state.size_of_log = NH_global_logs[0]->size_of_log;

  // long sleep_s = LOG_PERIOD / 1000000;
  // long sleep_ns = (LOG_PERIOD % 1000000) * 1000;

  // int nb_applied_txs = 0;
  while (!is_exit) {

    *NH_checkpointer_state = 0;
    __sync_synchronize();

    // printf("Received message!\n");

    // sem_wait(NH_chkp_sem); --> Set LOG_THRESHOLD to 0
    while (LOG_THRESHOLD > 0.0D && *NH_checkpointer_state == 0 /* IDLE */) {
      // waits for requests
      if (is_exit) {
        break;
      }
      PAUSE();
    }

    loop_checkpoint_manager();

    // TODO: add blocking time
    /* struct timespec sleep_time;
    sleep_time.tv_nsec = sleep_ns;
    sleep_time.tv_sec = sleep_s;

    nanosleep(&sleep_time, NULL); */
  }
  // printf("[--logger] NB_spins=%lli TIME_spins=%fms\n", MN_count_spins,
  //   (double)MN_time_spins / (double)CPU_MAX_FREQ);
  //
  // while(loop_checkpoint_manager()); // apply all the log
  //
  // printf("[  logger] NB_spins=%lli TIME_spins=%fms\n", MN_count_spins,
  //   (double)MN_time_spins / (double)CPU_MAX_FREQ);

  aux_thread_stats_to_gnuplot_file((char*) STATS_FILE ".aux_thr");
  #endif /* DO_CHECKPOINT */

  exit_success = 1;
  // MEMFENCE;
  __sync_synchronize();

  return NULL;
}

static int loop_checkpoint_manager()
{
  int res;
  int nb_txs = 0;
  int i;
  // printf("Are there entries? ");

  // NVHTM_snapshot_chkp();

  // time_o_chkp_2 = rdtscp();

  // double lat = (double)(time_o_chkp_2 - time_o_chkp_1) / (double)CPU_MAX_FREQ;

  /* if (lat > 100.0 && time_chkp_1 > 0) {
  // took more than 100ms: there is some problem
  printf("More than 100ms (%f)\n", lat);
}*/

time_chkp_1 = rdtscp();

//__sync_synchronize();

//    printf("APPLY_LOG: ");

#if SORT_ALG == 1
if (LOG_is_logged_tx()) {
  #else
  if (LOG_is_logged_tx2()) {
    #endif

    nb_txs = NH_global_logs[0]->size_of_log / 2;
    // printf("yes (nb_threads = %i) [before=%i TXs]", nb_threads, nb_txs);

    // LOG_checkpoint_apply_N_update_after(nb_txs); // super slow... does not work
    // #ifdef APPLY_BATCH_TX
    // LOG_checkpoint_apply_N(nb_txs);
    // #else
    // NH_nb_applied_txs++;
    // 2 -- in same thread
    // 3 -- no sorting
    // 4 -- different threads
    #if SORT_ALG == 1
    for (i = 0; i < nb_txs; ++i) {
      if (LOG_checkpoint_apply_one()) {
        break; // no more transactions
      }
    }
    #elif SORT_ALG == 2
    for (i = 0; i < nb_txs; ++i) {
      if (LOG_checkpoint_apply_one2()) {
        break; // no more transactions
      }
    }
    #elif SORT_ALG == 3
    for (i = 0; i < nb_txs; ++i) {
      if (LOG_checkpoint_apply_one3()) {
        break; // no more transactions
      }
    }
    #elif SORT_ALG == 4
    for (i = 0; i < nb_txs; ++i) {
      if (LOG_checkpoint_forward()) {
        break; // no more transactions
      }
    }
    #elif SORT_ALG == 5
    LOG_checkpoint_backward();
    #endif
    // LOG_move_start_ptrs();

    // LOG_fake_advance_ptrs(nb_txs);

    //__sync_synchronize();
    // #endif
    res = 1;
  } else {
    res = 0; // no more log
    // printf("NO TRANSACTIONS TO APPLY!\n");
  }

  time_chkp_2 = rdtscp();

  // time_o_chkp_1 = rdtscp();

  time_chkp_total += time_chkp_2 - time_chkp_1;

  // asm volatile ("sfence" ::: "memory");

  return res;
}

static void segfault_sigaction(int signal, siginfo_t *si, void *uap)
{
  static intptr_t old_addr = -1;
  intptr_t new_addr = (intptr_t) si->si_addr; // 6 is log_2(CACHE_LINE_SIZE)
  new_addr = ((new_addr >> 6) << 6);
  mmap(si->si_addr, 1024, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
  if (old_addr == (intptr_t) si->si_addr) {
    printf("SIGSEGV addr=%p\n", si->si_addr);
    // ucontext_t *context = (ucontext_t*)uap;
    // context->uc_mcontext.gregs[REG_RIP] += 0x04; // TODO: ignore load
    // exit(EXIT_FAILURE);
  }
  else {
    old_addr = (intptr_t) si->si_addr;
  }
  // is_sigsegv = 1;
  // __sync_synchronize();
  // TODO:
}

static void segint_sigaction(int signal, siginfo_t *si, void *context)
{
  char buffer[8192];
  char *ptr = buffer;

  sem_post(NH_chkp_sem);
  if (background_process_is_exit) exit(EXIT_SUCCESS);

  /* while(loop_checkpoint_manager()); */ // apply all the log

  ptr += sprintf(ptr, "[FORKED_MANAGER] Nb. checkpoints %lli\n",
  NH_nb_checkpoints);
  ptr += sprintf(ptr, "[FORKED_MANAGER] Time active %f ms!\n",
  (double) time_chkp_total / ((double) CPU_MAX_FREQ));
  ptr += sprintf(ptr, "[FORKED_MANAGER] Applied TXs %lli \n",
  NH_nb_applied_txs);
  ptr += sprintf(ptr, "[FORKED_MANAGER] Time extra sort logs %f ms \n",
  (double) NH_manager_order_logs / (double) CPU_MAX_FREQ);
  ptr += sprintf(ptr, "[logger] NB_spins=%lli NB_writes=%lli TIME_spins=%fms\n",
  MN_count_spins, MN_count_writes, (double)MN_time_spins / (double)CPU_MAX_FREQ);
  printf("%s", buffer);

  aux_thread_stats_to_gnuplot_file((char*) STATS_FILE ".aux_thr");

  fprintf(stderr, "Exit now!\n");
  fflush(stderr);
  background_process_is_exit = 1;
  //abort(); // exit is not safe
  //exit(EXIT_SUCCESS);
}

static void aux_thread_stats_to_gnuplot_file(char *filename) {
  FILE *gp_fp = fopen(filename, "a");
  if (ftell(gp_fp) < 8) {
    fprintf(gp_fp, "#"
    "NB_FLUSHES\t"         // [1]NB_FLUSHES
    "NB_WRITES\t"          // [2]NB_WRITES
    "REMAIN_LOG\t"         // [3]REMAIN_LOG
    "NB_CHKP\t"            // [4]NB_CHKP
    "TIME_FLUSHES(ms)\n"); // [5]TIME_FLUSHES
  }

  double time_after = (double)MN_count_spins / (double)CPU_MAX_FREQ;
  long long int remaining = LOG_total_used();

  fprintf(gp_fp, "%lli\t", MN_count_spins);           // [1]NB_FLUSHES
  fprintf(gp_fp, "%lli\t", MN_count_writes);          // [2]NB_WRITES
  fprintf(gp_fp, "%lli\t", remaining);                // [3]REMAIN_LOG
  fprintf(gp_fp, "%lli\t", NH_nb_checkpoints);        // [4]NB_CHKP
  fprintf(gp_fp, "%f\n", time_after);                 // [5]TIME_FLUSHES
  fclose(gp_fp);
}

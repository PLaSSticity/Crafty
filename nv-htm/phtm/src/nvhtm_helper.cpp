#include "nvhtm_helper.h"
#include "log.h"
#include "tm.h"
#include "nh.h"

// TODO: count time flush
#include "rdtsc.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <list>
#include <vector>
#include <algorithm>
#include <climits>
#include <utility>
#include <bitset>

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

// ################ variables

// TODO: remove thread_local
//thread_local ID_TYPE nvm_htm_thr_id = 0;

// set_threads is set in init_thrs (before launching the threads)
// nb_thrs is a counter to give ids
static CL_ALIGN int nb_thrs;
static CL_ALIGN int set_threads;
// compute checkpoint addresses with this

CL_ALIGN static mutex malloc_mtx;
static CL_ALIGN ts_s count_val_wait[MAX_NB_THREADS];

#ifdef DO_CHECKPOINT
NVMHTM_mem_s *mem_instance;
#endif /* DO_CHECKPOINT */

mutex mtx;

// ################ variables (thread-local)

__thread CL_ALIGN vector<NVMHTM_mem_s*> *tmp_allocs; // transactional allocs go here
__thread CL_ALIGN vector<void*> *tmp_frees;
__thread CL_ALIGN void *malloc_ptr;

// ################ functions
// vector<map<void*, NVMHTM_mem_s*>> allocs; // extern

static void handle_checkpoint(int tid, void *pool);

static void NVMHTM_validate(int, bitset<MAX_NB_THREADS> & threads);

// create a delete_thr, and handle logs
void static manage_checkpoint();

// ################ implementation header

void* NVMHTM_alloc(const char *file_name, size_t size, int vol_pool)
{
	// PHTM use normal malloc
    return NULL;
}

// TODO: free and thread safe track of freed segments

void* NVMHTM_malloc(void *pool, size_t size)
{
    return NULL; // TODO: PHTM
}

void NVMHTM_apply_allocs() {
	// TODO: no strange allocs in PHTM
}

NVMHTM_mem_s* NVMHTM_get_instance(void* pool)
{
	return NULL; // TODO: PHTM
}

void NVMHTM_thr_init(void *pool)
{
    int my_tid = TM_tid_var;

	PHTM_thr_init(my_tid);

	if (tmp_allocs == NULL) {
		// thread-local
		tmp_allocs = new vector<NVMHTM_mem_s*>();
		tmp_frees = new vector<void*>();
	}
}

void NVMHTM_thr_exit() { /* empty */ }

void NVMHTM_init_thrs(int nb_threads)
{
    set_threads = nb_threads;
	// instances.resize(nb_threads);

	PHTM_init(nb_threads);
}

void NVMHTM_clear() { /* empty */ }

void NVMHTM_zero_pool(void *pool) { /* empty */ }

int NVMHTM_has_writes(int tid)
{
    return LOG_nb_writes > 0; // TODO: use tid
}

void NVMHTM_shutdown()
{
    int i;
    ts_s res = 0;

    for (i = 0; i < MAX_NB_THREADS; ++i) {
        res += count_val_wait[i];
    }

    printf("Wait passed %llu times!\n", res);
}

void NVMHTM_write_ts(int id, ts_s ts) { }

void NVMHTM_commit(int id, ts_s ts, int nb_writes) { /* empty */ }

void NVMHTM_free(void *ptr) { /* empty */ }

void NVMHTM_copy_to_checkpoint(void *pool) { /* empty */ }

// TODO: don't know where the memory pool actually is

void NVMHTM_checkpoint(int tid, void *pool) { /* empty */ }

void NVMHTM_reduce_logs() { /* empty */ }

void NVMHTM_validate(int id, bitset<MAX_NB_THREADS>&) { /* empty */ }

void NVMHTM_crash()
{
    // TODO
}

// ################ implementation local functions

static void handle_checkpoint(int tid, void *pool)
{
    // TODO
}

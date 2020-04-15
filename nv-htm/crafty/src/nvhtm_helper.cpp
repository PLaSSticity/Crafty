#include "nvhtm_helper.h"
#include "log.h"
#include "tm.h"
#include "nh.h"
#include "extra_globals.h"

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

#ifdef CRAFTY_STATS
    crafty_aborts = {0};
#endif

	crafty_thr_init(my_tid);

	if (tmp_allocs == NULL) {
		// thread-local
		tmp_allocs = new vector<NVMHTM_mem_s*>();
		tmp_frees = new vector<void*>();
	}
}

// Only hold when accessing global/total stats, not when accessing thread local ones.
static CL_ALIGN mutex stats_count_mutex;

void NVMHTM_thr_exit() {
    int in_htm = HTM_test();
    if (!in_htm) stats_count_mutex.lock();
    crafty_global_redo_abort_capacity += crafty_redo_abort_capacity;
    crafty_global_redo_abort_conflict += crafty_redo_abort_conflict;
    crafty_global_redo_abort_explicit += crafty_redo_abort_explicit;
    crafty_global_redo_abort_zero += crafty_redo_abort_zero;
    crafty_global_validate_success += crafty_validate_success;
    crafty_global_replay_success += crafty_replay_success;
    crafty_global_readonly_success += crafty_readonly_success;
#ifdef CRAFTY_STATS
    crafty_global_aborts_in_redo += crafty_aborts_in_redo;
    crafty_global_aborts_in_SGL += crafty_aborts_in_SGL;
    crafty_total_aborts.capacity += crafty_aborts.capacity;
    crafty_total_aborts.conflict += crafty_aborts.conflict;
    crafty_total_aborts.explicit_ += crafty_aborts.explicit_;
    crafty_total_aborts.retry += crafty_aborts.retry;
    crafty_total_aborts.total += crafty_aborts.total;
    crafty_total_aborts.zero += crafty_aborts.zero;
    crafty_thread_count++;
    crafty_global_aborts_in_logging += crafty_aborts_in_logging;
    crafty_global_aborts_in_validating += crafty_aborts_in_validating;
    crafty_global_SGL_in_logging += crafty_SGL_in_logging;
    crafty_global_SGL_in_validating += crafty_SGL_in_validating;

    crafty_global_allocs_logged += crafty_allocs_logged;

    crafty_global_write_count += crafty_write_count;

    int i = 0;
    for (; i < 7; i++) {
        crafty_global_write_histogram[i] += crafty_write_histogram[i];
    }

    crafty_global_valfail += crafty_valfail;
    crafty_global_replayfail += crafty_replayfail;

    crafty_global_singlewrite_SGL += crafty_singlewrite_SGL;
    crafty_global_multiwrite_SGL += crafty_multiwrite_SGL;

    crafty_global_log_sections += crafty_log_sections;

    if (crafty_global_alloc_high_mark < crafty_alloc_high_mark) crafty_global_alloc_high_mark = crafty_alloc_high_mark;
    if (crafty_global_free_high_mark < crafty_free_high_mark) crafty_global_free_high_mark = crafty_free_high_mark;
#endif
    if (!in_htm) stats_count_mutex.unlock();
}

void NVMHTM_init_thrs(int nb_threads)
{
    set_threads = nb_threads;
	// instances.resize(nb_threads);
}

void NVMHTM_clear() { /* empty */ }

void NVMHTM_zero_pool(void *pool) { /* empty */ }

int NVMHTM_has_writes(int tid)
{
    return LOG_nb_writes > 0; // TODO: use tid
}

static inline int number_print_width(long number) {
    int i = 0;
    while (number > 0) {
        i++;
        number = number / 10;
    }
    return i;
}

void NVMHTM_shutdown()
{
    int i;
    ts_s res = 0;

    for (i = 0; i < MAX_NB_THREADS; ++i) {
        res += count_val_wait[i];
    }

    printf("Wait passed %llu times!\n", res);

    int successes = TM_get_error_count(HTM_SUCCESS);
    int fallbacks = TM_get_error_count(HTM_FALLBACK);
    long commits = (long int)successes + (long int)fallbacks;

    stats_count_mutex.lock();
    int w = 0;
#ifdef CRAFTY_STATS

    w = number_print_width(crafty_total_aborts.total);
    printf("Total aborts: %*i\n", w, crafty_total_aborts.total);
    printf("Capacity:     %*i\n", w, crafty_total_aborts.capacity);
    printf("Conflict:     %*i\n", w, crafty_total_aborts.conflict);
    printf("Explicit:     %*i\n", w, crafty_total_aborts.explicit_);
    printf("Retry:        %*i\n", w, crafty_total_aborts.retry);
    printf("Zero:        %*i\n", w, crafty_total_aborts.zero);

    w = number_print_width(crafty_global_SGL_in_validating + crafty_global_SGL_in_logging);
    printf("Switched to SGL from logging:    %*li\n", w, crafty_global_SGL_in_logging);
    printf("Switched to SGL from validating: %*li\n", w, crafty_global_SGL_in_validating);

    printf("Saw %i threads exit.\n", crafty_thread_count);
    printf("Alloc high mark: %lu\n", crafty_global_alloc_high_mark);
    printf("Free high mark: %lu\n", crafty_global_alloc_high_mark);
    printf("Allocs logged: %lu\n", crafty_global_allocs_logged);
    printf("Allocs per commit: %lf\n", (double)crafty_global_allocs_logged / (double)commits);
    printf("Number of log sections (logging transactions + SGL inner transactions): %li\n", crafty_global_log_sections);
    printf("Total writes: %li (includes logging, validation and SGL)\n", crafty_global_write_count);
    printf("Total commits: %li  (includes logging, validation and SGL)\n", commits);
    printf("Write per commit histogram:\n");

    w = number_print_width(commits);
    printf("0    : %*li\n", w, crafty_global_write_histogram[0]);
    printf("1    : %*li\n", w, crafty_global_write_histogram[1]);
    printf("2    : %*li\n", w, crafty_global_write_histogram[2]);
    printf("3    : %*li\n", w, crafty_global_write_histogram[3]);
    printf("4-5  : %*li\n", w, crafty_global_write_histogram[4]);
    printf("6-10 : %*li\n", w, crafty_global_write_histogram[5]);
    printf("11+  : %*li\n", w, crafty_global_write_histogram[6]);

    w = number_print_width(crafty_global_valfail + crafty_global_replayfail);
    printf("Crafty validation failures: %*lu\n", w, crafty_global_valfail);
    printf("Crafty replay failures:     %*lu\n", w, crafty_global_replayfail);

    w = number_print_width(crafty_global_singlewrite_SGL + crafty_global_multiwrite_SGL);
    printf("Crafty single write SGL sections: %*lu\n", w, crafty_global_singlewrite_SGL);
    printf("Crafty multi  write SGL sections: %*lu\n", w, crafty_global_multiwrite_SGL);

    w = number_print_width(
            crafty_global_aborts_in_validating + crafty_global_aborts_in_logging + crafty_global_aborts_in_redo +
            crafty_global_aborts_in_SGL);
    printf("Aborts in logging:    %*li\n", w, crafty_global_aborts_in_logging);
    printf("Aborts in validating: %*li\n", w, crafty_global_aborts_in_validating);
    printf("Aborts in redo:       %*li\n", w, crafty_global_aborts_in_redo);
    printf("Aborts in SGL:        %*li\n", w, crafty_global_aborts_in_SGL);
    stats_count_mutex.unlock();
#endif
    w = number_print_width(crafty_global_replay_success + crafty_global_validate_success + crafty_global_readonly_success);
    printf("Applying redo log succeeded: %*li\n", w, crafty_global_replay_success);
    printf("Validating log succeeded:    %*li\n", w, crafty_global_validate_success);
    printf("Readonly tx succeeded:       %*li\n", w, crafty_global_readonly_success);

    w = number_print_width(crafty_sgl_abort_zero + crafty_sgl_abort_conflict + crafty_sgl_abort_capacity + crafty_sgl_abort_explicit);
    printf("SGL inner aborts due to capacity: %*lu\n", w, crafty_sgl_abort_capacity);
    printf("SGL inner aborts due to conflict: %*lu\n", w, crafty_sgl_abort_conflict);
    printf("SGL inner aborts due to explicit: %*lu\n", w, crafty_sgl_abort_explicit);
    printf("SGL inner aborts due to zero:     %*lu\n", w, crafty_sgl_abort_zero);

    w = number_print_width(crafty_global_redo_abort_zero + crafty_global_redo_abort_conflict + crafty_global_redo_abort_capacity + crafty_global_redo_abort_explicit);
    printf("redo aborts due to capacity: %*lu\n", w, crafty_global_redo_abort_capacity);
    printf("redo aborts due to conflict: %*lu\n", w, crafty_global_redo_abort_conflict);
    printf("redo aborts due to explicit: %*lu\n", w, crafty_global_redo_abort_explicit);
    printf("redo aborts due to zero:     %*lu\n", w, crafty_global_redo_abort_zero);
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

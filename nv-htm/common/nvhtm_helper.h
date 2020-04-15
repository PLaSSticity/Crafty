#ifndef NVHTM_HELPER_H
#define NVHTM_HELPER_H 1

#include "tm.h"
#include "log.h"
#include "nvmlib_wrapper.h"

#ifdef __cplusplus
extern "C"
{
#endif

// DOES NOT WORK
#define PREPARE_MALLOC() ({ \
	int nb_tests = 5; \
	int i; \
	GRANULE_TYPE *test[nb_tests]; \
	for (i = 0; i < nb_tests; ++i) { \
		test[i] = (GRANULE_TYPE*) NH_alloc(CACHE_LINE_SIZE); \
		*((GRANULE_TYPE*)test[i]) = i; \
	} \
	for (i = 0; i < nb_tests; ++i) { \
		NH_free(test[i]); \
	} \
})

    // TODO: add deltas to the log to save space
#define ADDR_TO_DELTA(base, addr) ({ \
        ((GRANULE_TYPE*) addr - (GRANULE_TYPE*) base) + VALID_ADDRS; \
}) // Delta from the beginning of the pool

#define DELTA_TO_ADDR(base, delta) ({ \
        ((GRANULE_TYPE*) base + delta) - VALID_ADDRS; \
})

#define CALC_DELTA(base, addr) ({ \
        (((GRANULE_TYPE*) base) - ((GRANULE_TYPE*) addr)); \
})

#define ID_TYPE GRANULE_TYPE

    typedef struct NVCheckpoint_ NVCheckpoint_s;
    typedef struct NVMHTM_mem_ NVMHTM_mem_s;
    typedef struct NVMHTM_root_ NVMHTM_root_s;

    struct NVCheckpoint_
    {
        size_t size;
        long long int delta_to_pool;
        void *ptr;
    };

    struct NVMHTM_mem_
    {
        NVCheckpoint_s chkp;
        ts_s ts, chkp_counter;
        void *ptr, *last_alloc;
        size_t size;
        char file_name[256]; // TODO: check the size
        int flags;
    };

    /**
     *
     * @param file_name file where the shared pool will be stored
     * @param size number of granules (in sizeof (GRANULE_TYPE)) to allocate
     * @return
     */
    void* NVMHTM_alloc(const char* file_name, size_t, int vol_pool);
    void* NVMHTM_malloc(void *pool, size_t size); // allocs in a pool
    void NVMHTM_free(void *ptr);
    void NVMHTM_apply_allocs(); // call this after TX ends
    NVMHTM_mem_s* NVMHTM_get_instance(void *pool);
    void NVMHTM_thr_init(void *pool); // call this from within the thread
    void NVMHTM_init_thrs(int nb_threads);

    #define NVMHTM_get_thr_id() ({ TM_tid_var; })

    // sets to 0 all allocated memory, checkpoints and logs
    void NVMHTM_clear();
    void NVMHTM_zero_pool(void *pool);

    void NVMHTM_write_ts(int tid, ts_s ts);
    int NVMHTM_has_writes(int tid);
    void NVMHTM_commit(int tid, ts_s ts, int nb_writes);
    void NVMHTM_copy_to_checkpoint(void*);
    void NVMHTM_free(void*);
    void NVMHTM_shutdown();
	void NVMHTM_thr_exit();

	void NVMHTM_reduce_logs();

    // simulates a crash (SIGKILL)
    void NVMHTM_crash();

    // TODO: flush

#ifdef __cplusplus
}
#endif

#endif /* NVHTM_HELPER_H */

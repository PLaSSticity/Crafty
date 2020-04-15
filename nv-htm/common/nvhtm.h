#ifndef NVHTM_H
#define NVHTM_H

#include "tm.h"
// #include "simul.h"
#include "nvhtm_helper.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void* NVHTM_alloc(const char *file_name, size_t size, int use_vol);
    void* NVHTM_malloc(size_t size); // allocs in a pool
	void NVHTM_free(void *ptr);
#define NVHTM_init(nb_threads) HTM_init(nb_threads); NVHTM_init_(nb_threads)
    void NVHTM_init_(int nb_threads);

	void NVHTM_req_clear_log();

    void NVHTM_start_stats();
    void NVHTM_end_stats();
    double NVHTM_elapsed_time();
    int NVHTM_nb_thrs();
    void NVHTM_shutdown();
    void NVHTM_reduce_logs();
    void NVHTM_recover();
    void NVHTM_thr_init();
    void NVHTM_thr_exit();
    void NVHTM_abort_tx();

    void NVHTM_clear();
    void NVHTM_cpy_to_checkpoint(void *pool);

    int NVHTM_nb_transactions();

    int NVHTM_nb_htm_commits();
    int NVHTM_nb_sgl_commits();
    int NVHTM_nb_aborts_aborts();
    int NVHTM_nb_aborts_capacity();
    int NVHTM_nb_aborts_conflicts();
	double NVHTM_get_total_time();

    void NVHTM_thr_snapshot();
    void NVHTM_snapshot_chkp();

#if (!defined(DISABLE_LOG) && !defined(HTM_ONLY))
#define NVHTM_write(addr, value) ({ \
	GRANULE_TYPE val = (GRANULE_TYPE) (value); \
    NVMHTM_write(TM_tid_var, addr, val); \
})

#define NVHTM_write_P(addr, value) ({ \
	GRANULE_P_TYPE val = (GRANULE_P_TYPE) (value); \
    NVMHTM_write_P(TM_tid_var, addr, val); \
})

#define NVHTM_write_D(addr, value) ({ \
	GRANULE_D_TYPE val = (GRANULE_D_TYPE) (value); \
    NVMHTM_write_D(TM_tid_var, addr, val); \
})
#else /* (!defined(DISABLE_LOG) && !defined(HTM_ONLY)) */
#define NVHTM_write(addr, value) ({ \
    *((GRANULE_TYPE*) addr) = (GRANULE_TYPE) value; \
})

#define NVHTM_write_P(addr, value) ({ \
    *((GRANULE_P_TYPE*) addr) = (GRANULE_P_TYPE) value; \
})

#define NVHTM_write_D(addr, value) ({ \
    *((GRANULE_D_TYPE*) addr) = (GRANULE_D_TYPE) value; \
})
#endif

//#define NVHTM_write(addr, value) NVHTM_wr(addr, value)
//#define NVHTM_write_P(addr, value) NVHTM_wr_P(addr, value)
//#define NVHTM_write_D(addr, value) NVHTM_wr_D(addr, value)

    void NVHTM_stats_add_time(unsigned long long t_tx, unsigned long long t_after);
    double NVHTM_stats_get_avg_time_tx();
    double NVHTM_stats_get_avg_time_after();

#define NVHTM_write_T(addr, value, type) *((type*)addr) = value

#ifdef __cplusplus
}
#endif

#endif /* NVHTM_H */

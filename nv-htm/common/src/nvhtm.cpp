#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <thread>

#include "htm_retry_template.h"

using namespace std;

// ################ types

/* empty */

// ################ variables

static void* pool;



static TIMER_T ts_init, ts_shutdown;
static unsigned long long t_count[MAX_NB_THREADS],
	t_tx[MAX_NB_THREADS],
	t_after[MAX_NB_THREADS];

static int nb_thrs; // counter for the IDs
static int nb_threads;
static double used_cap = 0;
static int nb_cap_samples = 0;

static int mutex = 0;

// ################ variables (thread-local)
/* empty */
// ################ local functions
//static void load_sys_info();
static void stats_to_gnuplot_file(char *filename);
// ################ implementation header

void* NVHTM_alloc(const char *file_name, size_t size, int vol_pool)
{
    return NVMHTM_alloc(file_name, size, vol_pool);
}

void* NVHTM_malloc(size_t size)
{
    return NVMHTM_alloc("./some_file.txt", size, 1);
    // return NVMHTM_malloc(pool, size);
}

void NVHTM_free(void *ptr)
{
    return NVMHTM_free(ptr);
}

void NVHTM_init_(int _nb_threads)
{
	// load_sys_info(); // first thing

    NH_ts_init = rdtscp();

    TM_nb_threads = nb_threads = _nb_threads;
    TM_init_nb_threads(nb_threads);

    NVMHTM_init_thrs(nb_threads);

    // ---
		HTM_reset_status_count(); // TPCC calls this twice
    // ---

    // printf("Size of cache line: %i\n", CACHE_LINE_SIZE);

    MN_learn_nb_nops();
}

void NVHTM_start_stats()
{
	NVMHTM_copy_to_checkpoint(NULL); // forks on DO_CHECKPOINT == 5

    TIMER_READ(ts_init); // TODO: statistics optional
}

void NVHTM_end_stats()
{
    TIMER_READ(ts_shutdown); // TODO: statistics optional
}

double NVHTM_elapsed_time()
{
    return TIMER_DIFF_SECONDS(ts_init, ts_shutdown);
}

int NVHTM_nb_thrs()
{
    return nb_threads;
}

void NVHTM_thr_init()
{
	int my_tid;
	while(!__sync_bool_compare_and_swap(&mutex, 0, 1));

	my_tid = nb_thrs++;

	mutex = 0;
	__sync_synchronize(); // MEMFENCE

    TM_tid_var = my_tid;
    NVMHTM_thr_init(pool);
    TM_set_is_record(my_tid, 1);

    // ---
    HTM_thr_init();
    MN_thr_enter();
    // ---

    NVHTM_thr_snapshot();
}

void NVHTM_thr_exit()
{
	while(!__sync_bool_compare_and_swap(&mutex, 0, 1));

	nb_thrs--;

	NH_tx_time_total += (double) NH_tx_time;
	NH_time_after_commit_total += (double) NH_time_after_commit;

	NH_tx_time = 0;
	NH_time_after_commit = 0;

	NH_count_writes_total += NH_count_writes;
	NH_count_blocks_total += NH_count_blocks;
	NH_time_validate_total += NH_time_validate;

	mutex = 0;
	__sync_synchronize();

    // ---
    HTM_thr_exit();
    MN_thr_exit();
    // ---

    // printf("[%i] NB_spins=%lli TIME_spins=%fms\n", TM_tid_var, MN_count_spins,
    //       (double)MN_time_spins / (double)CPU_MAX_FREQ);

	NVMHTM_thr_exit();
}

void NVHTM_abort_tx()
{
    printf("Passou aqui!\n");
    HTM_abort();
}

void NVHTM_clear()
{
    NVMHTM_clear();
}

void NVHTM_zero()
{
    // NVMHTM_mem_s *instance = NVMHTM_get_instance(pool);
    // memset(pool, 0, instance->size);
    // NVMHTM_copy_to_checkpoint(pool);
}

void NVHTM_cpy_to_checkpoint(void *pool)
{
    NVMHTM_copy_to_checkpoint(pool);
}

int NVHTM_nb_transactions()
{
    return TM_get_error_count(HTM_SUCCESS) + TM_get_error_count(HTM_FALLBACK);
}

int NVHTM_nb_htm_commits()
{
    return TM_get_error_count(HTM_SUCCESS);
}

int NVHTM_nb_sgl_commits()
{
    return TM_get_error_count(HTM_FALLBACK);
}

int NVHTM_nb_aborts_aborts()
{
    return TM_get_error_count(HTM_ABORT);
}

int NVHTM_nb_aborts_capacity()
{
    return TM_get_error_count(HTM_CAPACITY);
}

int NVHTM_nb_aborts_conflicts()
{
    return TM_get_error_count(HTM_CONFLICT);
}

void NVHTM_stats_add_time(ts_s _t_tx, ts_s _t_after)
{
    int tid = NVMHTM_get_thr_id();
    t_count[tid]++;
    t_tx[tid] += _t_tx;
    t_after[tid] += _t_after;
}

double NVHTM_get_total_time()
{
    return TIMER_DIFF_SECONDS(ts_init, ts_shutdown);
}

double NVHTM_stats_get_avg_time_tx()
{
    int i;
    double res = 0;
    for (i = 0; i < MAX_NB_THREADS; ++i) {
        if (t_count[i] > 0) {
            res += (double) t_tx[i] / (double) t_count[i] / (double) CPU_MAX_FREQ;
        }
    }
    return res;
}

double NVHTM_stats_get_avg_time_after()
{
    int i;
    double res = 0;
    for (i = 0; i < MAX_NB_THREADS; ++i) {
        if (t_count[i] > 0) {
            res += (double) t_after[i] / (double) t_count[i] / (double) CPU_MAX_FREQ;
        }
    }
    return res;
}

static int s_aborts;
static int s_conflicts;
static int s_capacities;
static int s_explicit_a;
static int s_zero;

static inline int number_print_width(long number) {
    int i = 0;
    while (number > 0) {
        i++;
        number = number / 10;
    }
    return i;
}

void NVHTM_shutdown()
{
    // TODO: statistics optional
    double time_taken;

    time_taken = TIMER_DIFF_SECONDS(ts_init, ts_shutdown);

    // need memory fence here or in the end of the function
    __sync_synchronize();

    int successes = TM_get_error_count(HTM_SUCCESS);
    int fallbacks = TM_get_error_count(HTM_FALLBACK);
    s_aborts = TM_get_error_count(HTM_ABORT);
    s_conflicts = TM_get_error_count(HTM_CONFLICT);
    s_capacities = TM_get_error_count(HTM_CAPACITY);
    s_explicit_a = TM_get_error_count(HTM_EXPLICIT);
    s_zero = TM_get_error_count(HTM_ZERO);
    int commits = successes + fallbacks;
    double X = (double) commits / (double) time_taken;
    double P_A = (double) s_aborts / (double) (successes + s_aborts);

    double time_tx = NVHTM_stats_get_avg_time_tx();
    double time_after = NVHTM_stats_get_avg_time_after();

    NVMHTM_shutdown();

    int w = number_print_width(successes);
    printf("\n\n ########################################### \n");
    printf(" ########################################### \n");
    printf(" --- ABORTS\n");
    printf("COMMIT : %*i\n", w, successes);
    printf("ABORTS : %*i\n", w, s_aborts);
    printf("CONFLS : %*i\n", w, s_conflicts);
    printf("CAPACS : %*i\n", w, s_capacities);
    printf("EXPLIC : %*i\n", w, s_explicit_a);
    printf("  ZERO : %*i\n", w, s_zero);
    printf("FALLBK : %*i\n", w, fallbacks);
    printf("   P_A : %f\n", P_A);
    printf("     X : %f\n", X);
    printf(" ---   TIME\n");
    printf("Time %f s\n", time_taken);
    printf("TIME_TX %f ms\n", time_tx);
    printf("TIME_AFTER %f ms\n", time_after);
    printf("AVG_CAP %f\n", used_cap / (double) nb_cap_samples);
    printf("TOTAL_WRITES          %lli\n", MN_count_writes_to_PM_total);
    printf("TOTAL_SPINS (workers) %lli\n", MN_count_spins_total);
    printf("TOTAL_BLOCKS          %lli\n", NH_count_blocks_total);
    printf("TOTAL_TIME_B (clocks) %llu\n", NH_time_blocked_total);
    printf(" ---   ----\n");
    printf("CPU_MAX_FREQ          %llu\n", CPU_MAX_FREQ);
//    printf("CYCLES_TO_WAIT        %llu\n", CYCLES_TO_WAIT);
    printf("NH_spins_per_100      %llu\n", NH_spins_per_100);
    printf(" ########################################### \n");
    printf(" ########################################### \n\n");

    // ---
    HTM_exit();
    // ---

	// TODO: if gnuplot file
	stats_to_gnuplot_file((char*) STATS_FILE);
}

void NVHTM_reduce_logs()
{
	NVMHTM_reduce_logs();
}

// ################ implementation local functions

// TODO: remove or move to arch_dep
/* void load_sys_info() {
	// TODO: this goes better in a define and passed through the compiler!
	char result[128];
	char cmd_get_thrs[] =
		"cat /proc/cpuinfo | grep processor | wc -l";
	char cmd_get_freq[] =
		"cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";

	run_command(cmd_get_thrs, result, sizeof(result));
	MAX_PHYS_THRS = atoi(result);
	run_command(cmd_get_freq, result, sizeof(result));
	CPU_MAX_FREQ = atoll(result);
}*/

static void stats_to_gnuplot_file(char *filename) {
	FILE *gp_fp = fopen(filename, "a");
	if (ftell(gp_fp) < 8) {
		fprintf(gp_fp, "#"
		"THREADS\t"        // [1]THREADS
		"TIME\t"           // [2]TIME
		"NB_TXS\t"         // [3]NB_TXS
		"COMMITS_HTM\t"    // [4]COMMITS_HTM
		"COMMITS_SGL\t"    // [5]COMMITS_SGL
		"ABORTS\t"         // [6]ABORTS
		"CAPACITY\t"       // [7]CAPACITY
		"CONFLICTS\t"      // [8]CONFLICTS
    "EXPLICIT\t"       // [9]EXPLICIT
		"NB_BLOCKS\t"      // [10]NB_BLOCKS
		"TIME_WAIT_LOG\t"  // [11]TIME_WAIT_LOG
		"TIME_VALIDATE\t"  // [12]TIME_VALIDATE
		"TIME_TXS\t"       // [13]TIME_TXS
		"TIME_FLUSH\t"     // [14]TIME_FLUSH
		"NB_FLUSHES\t"     // [15]NB_FLUSH
		"NB_WRITES\t"      // [16]NB_FLUSH
		"PERC_BLOCKED\n"); // [17]PERC_BLOCKED
	}

	double time_taken = NVHTM_elapsed_time();
	double time_tx = NH_tx_time_total / (double) TM_nb_threads / (double) CPU_MAX_FREQ;
	double time_after = MN_time_spins_total / CPU_MAX_FREQ;
    // NH_time_after_commit_total / CPU_MAX_FREQ;

	double perc_blocked = ((double) NH_time_blocked_total / (double) CPU_MAX_FREQ
		/ 1000.0D) / (double) TM_nb_threads / time_taken;
	double time_validate = NH_time_validate_total / (double) CPU_MAX_FREQ \
		/ 1000.0D / (double) TM_nb_threads;

	fprintf(gp_fp, "%i\t", TM_nb_threads);              // [1]THREADS
	fprintf(gp_fp, "%f\t", time_taken);                 // [2]TIME
	fprintf(gp_fp, "%i\t", NVHTM_nb_transactions());    // [3]NB_TXS
	fprintf(gp_fp, "%i\t", NVHTM_nb_htm_commits());     // [4]COMMITS_HTM
	fprintf(gp_fp, "%i\t", NVHTM_nb_sgl_commits());     // [5]COMMITS_SGL
	fprintf(gp_fp, "%i\t", NVHTM_nb_aborts_aborts());   // [6]ABORTS
	fprintf(gp_fp, "%i\t", NVHTM_nb_aborts_capacity()); // [7]CAPACITY
	fprintf(gp_fp, "%i\t", NVHTM_nb_aborts_conflicts());// [8]CONFLICTS
	fprintf(gp_fp, "%i\t", s_explicit_a);               // [9]EXPLICIT
	fprintf(gp_fp, "%lli\t", NH_count_blocks_total);    // [10]NB_BLOCKS
	fprintf(gp_fp, "%llu\t", NH_time_blocked_total);    // [11]TIME_WAIT_LOG
	fprintf(gp_fp, "%e\t", time_validate);              // [12]TIME_VALIDATE
	fprintf(gp_fp, "%e\t", time_tx);                    // [13]TIME_TXS
	fprintf(gp_fp, "%e\t", time_after);                 // [14]TIME_FLUSH
	fprintf(gp_fp, "%lli\t", MN_count_spins_total);     // [15]NB_FLUSH
	fprintf(gp_fp, "%lli\t", MN_count_writes_to_PM_total); // [16]NB_WRITES
	fprintf(gp_fp, "%f\n", perc_blocked);               // [17]PERC_BLOCKED
	fclose(gp_fp);
}

void NVHTM_thr_snapshot()
{
    if (!NH_ts_last_snp) {
        NH_ts_last_snp = rdtscp();
        return;
    }

    ts_s now = rdtscp();
    // in ms
    double diff = (double)(now - NH_ts_last_snp)/(double)CPU_MAX_FREQ;
    diff /= 1000.0; // in seconds

    if (diff < 0.1) {
        // wait 0.1s before snapshoting some result
        return;
    }
    NH_ts_last_snp = now;
    double time = (double)(now - NH_ts_init)/(double)CPU_MAX_FREQ;

    char filename[256];

    sprintf(filename, "stats_file.thr%i", TM_tid_var);

    FILE *gp_fp = fopen(filename, "a");
	if (ftell(gp_fp) < 8) {
		fprintf(gp_fp, "#"
		"TIME\t"              // [1]TIME
		"NB_WRITES\t"         // [2]NB_WRITES
		"NB_FLUSHES\t"        // [3]NB_FLUSHES
		"NB_TXS\t"            // [4]NB_TXS
		"NB_ABORTS\t"         // [5]NB_ABORTS
		"NB_CONFLICT\t"       // [6]NB_CONFLICT
		"NB_CAPACITY\t"       // [7]NB_CAPACITY
		"NB_EXPLICIT\t"       // [8]NB_EXPLICIT
		"NB_COMMIT_HTM\t"     // [9]NB_COMMIT_HTM
		"NB_COMMIT_SGL\t"     // [10]NB_COMMIT_SGL
		"TIME_BLOCKED\n"      // [11]TIME_BLOCKED
		);
	}

	long aborts = HTM_get_status_count(HTM_ABORT, NULL);
	long conflt = HTM_get_status_count(HTM_CONFLICT, NULL);
	long capaci = HTM_get_status_count(HTM_CAPACITY, NULL);
	long explic = HTM_get_status_count(HTM_EXPLICIT, NULL);
	long htm_co = HTM_get_status_count(HTM_SUCCESS, NULL);
	long sgl_co = HTM_get_status_count(HTM_FALLBACK, NULL);
	double time_blocked = (double)NH_time_blocked / (double)CPU_MAX_FREQ;

	fprintf(gp_fp, "%f\t", time);               // [1]TIME
	fprintf(gp_fp, "%lli\t", NH_count_writes);  // [2]NB_WRITES
	fprintf(gp_fp, "%lli\t", MN_count_spins);   // [3]NB_FLUSHES
	fprintf(gp_fp, "%li\t", htm_co + sgl_co);   // [4]NB_TXS
	fprintf(gp_fp, "%li\t", aborts);            // [5]NB_ABORTS
	fprintf(gp_fp, "%li\t", conflt);            // [6]NB_CONFLICT
	fprintf(gp_fp, "%li\t", capaci);            // [7]NB_CAPACITY
	fprintf(gp_fp, "%li\t", explic);            // [8]NB_EXPLICIT
	fprintf(gp_fp, "%li\t", htm_co);            // [9]NB_COMMIT_HTM
	fprintf(gp_fp, "%li\t", sgl_co);            // [10]NB_COMMIT_SGL
	fprintf(gp_fp, "%f\n", time_blocked);       // [11]TIME_BLOCKED
	fclose(gp_fp);
}

void NVHTM_snapshot_chkp()
{
    if (!NH_ts_last_snp) {
        NH_ts_last_snp = rdtscp();
        return;
    }

    ts_s now = rdtscp();
    // in ms
    double diff = (double)(now - NH_ts_last_snp)/(double)CPU_MAX_FREQ;
    diff /= 1000.0; // in seconds

    if (diff < 0.1) {
        // wait 0.1s before snapshoting some result
        return;
    }
    NH_ts_last_snp = now;
    double time = (double)(now - NH_ts_init)/(double)CPU_MAX_FREQ;

    char filename[256];

    sprintf(filename, "stats_file.chkp");

    FILE *gp_fp = fopen(filename, "a");
	if (ftell(gp_fp) < 8) {
		fprintf(gp_fp, "#"
		"TIME\t"              // [1]TIME
		"NB_FLUSHES\t"        // [2]NB_FLUSHES
		"NB_CHECKPOINTS\n"    // [3]NB_CHECKPOINTS
		);
	}

	fprintf(gp_fp, "%f\t", time);                // [1]TIME
	fprintf(gp_fp, "%lli\t", MN_count_spins);    // [2]NB_FLUSHES
	fprintf(gp_fp, "%lli\n", NH_nb_checkpoints); // [3]NB_CHECKPOINTS
	fclose(gp_fp);
}

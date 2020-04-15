#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include "rdtsc.h"
#include "nvhtm.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//#include <sched.h>

GRANULE_TYPE **pool;
__thread GRANULE_TYPE *loc_pool;

//  * CACHE_LINE_SIZE / sizeof(GRANULE_TYPE)
#define GET_ADDR(pool, i) (&(pool[i * CACHE_LINE_SIZE / sizeof(GRANULE_TYPE)]))

// TODO: crashes
#ifndef PROB_CRASH
#define PROB_CRASH 0.001
#endif

#define THREAD_SPACING 1
#define THREAD_OFFSET 0

int nb_accounts = 128, threads = 4, bank_budget = 5000, transactions = 1,
        transfer_limit = 3, nb_transfers = 5000, nt_per_thread;
int no_conflicts = 0;
char *gnuplot_file;

TIMER_T c_ts1, c_ts2;

pthread_barrier_t barrier;

CL_ALIGN __thread int nb_write_txs_pt = 0;
CL_ALIGN int nb_write_txs = 0;

int bank_total(GRANULE_TYPE *pool);

void initialize_pool(GRANULE_TYPE *pool)
{
    int i;
	
	int inc_step = 1;

    NVHTM_clear();

    for (i = 0; i < nb_accounts; ++i) {
        *GET_ADDR(pool, i) = 0; // Replace with memset
    }

    for (i = 0; i < bank_budget; i += inc_step) {
        int account = rand() % nb_accounts;
        int new_val = *GET_ADDR(pool, account) + inc_step;

        *GET_ADDR(pool, account) = new_val;
    }

    NVHTM_cpy_to_checkpoint(pool);

    // check the total in the checkpoint
    // printf("Total in checkpoint: %i \n", bank_total(pool));
}

int bank_total(GRANULE_TYPE *pool)
{
    int i, res = 0;

    for (i = 0; i < nb_accounts; ++i) {
        res += *GET_ADDR(pool, i);
    }

    return res;
}

int bank_total_tx(GRANULE_TYPE *pool)
{
    int i, res = 0;

    NH_begin();
    for (i = 0; i < nb_accounts; ++i) {
        res += *GET_ADDR(pool, i);
    }
    NH_commit();

    return res;
}

void* waste_cpu(void *arg)
{
    int i = 0;
    double d = 0;
    do {
        ++i;
        d += 1.0;
    }
    while (i == i);
    return NULL;
}

void* random_transfer(void *arg)
{
    int i, tx, tid, transactions_loc = transactions;

    unsigned seed = clock();
	
    NVHTM_thr_init();
	
	NH_free(NH_alloc(64)); // TEST

    tid = TM_tid_var;

    if (tid >= threads) {
        return NULL; // error!
    }

    if (no_conflicts) {
        loc_pool = pool[tid];
    }
    else {
        loc_pool = *pool;
    }
	
    // bank_total(loc_pool);

    pthread_barrier_wait(&barrier);

    if (tid == 0) {
        NVHTM_start_stats();
    }

    for (tx = 0; tx < nt_per_thread; ++tx) {
		int sender_amount;
		int recipient_amount;
		int sender_new_amount;
		int recipient_new_amount;
		
		int transfer_amount;
		int recipient;
		int sender;
		
		// Not working
		void *some_ptr;
		
        NH_begin();
		
		some_ptr = NH_alloc(64);
		
			
		// -------------------------------------------
		// do some computation that is not erased by the compiler
		for (i = 0; i < transactions_loc ; ++i) {

			recipient = (rand_r(&seed) % nb_accounts);
			sender = (rand_r(&seed) % nb_accounts);
			
			if (sender == recipient) {
				--i;
				continue;
			}
			
			// TEST malloc inside transaction
			// TODO:
			
			// Read
			sender_amount = NH_read(GET_ADDR(loc_pool, sender));
			recipient_amount = NH_read(GET_ADDR(loc_pool, recipient));
			
			// Process
			transfer_amount = (rand_r(&seed) % transfer_limit) + 1; //
			sender_new_amount = sender_amount - transfer_amount;
			recipient_new_amount = recipient_amount + transfer_amount;
		
			if (sender_new_amount < 0) {
				sender_new_amount = sender_amount;
				recipient_new_amount = recipient_amount;
			}
			
			// Write
			NH_write(GET_ADDR(loc_pool, sender), sender_new_amount);
			NH_write(GET_ADDR(loc_pool, recipient), recipient_new_amount);
			
		}
		// -------------------------------------------

        NH_commit();
		
		NH_free(some_ptr);

        // LOG("TRANSFER from %i to %i amount=%i, bank total after %i\n", 
        //    sender, recipient, transfer_amount, bank_total(pool));
    }

    if (tid == 0) {
        NVHTM_end_stats();
    }

    NVHTM_thr_exit();

    // __sync_fetch_and_add(&nb_write_txs, nb_write_txs_pt);

    return NULL;
}

int main(int argc, char **argv)
{
    int i = 1;
    int reboot = 0;
    int args_threads[MAX_NB_THREADS];

    while (i < argc) {
        if (strcmp(argv[i], "REBOOT") == 0) {
            reboot = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "THREADS") == 0) {
            threads = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "NB_ACCOUNTS") == 0) {
            nb_accounts = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "NB_TRANSFERS") == 0) {
            nb_transfers = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "BANK_BUDGET") == 0) {
            bank_budget = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "NO_CONFL") == 0) {
            no_conflicts = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "TRANSFER_LIMIT") == 0) {
            threads = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "GNUPLOT_FILE") == 0) {
            gnuplot_file = strdup(argv[i + 1]);
        }
		else if (strcmp(argv[i], "TXS") == 0) {
            transactions = atoi(argv[i + 1]);
        }
        i += 2;
    }

    nt_per_thread = nb_transfers / threads;

    pthread_barrier_init(&barrier, NULL, threads);

    // thread example
    printf(" Start program ========== \n");
    printf("         REBOOT: %i\n", reboot);
    printf("       NO_CONFL: %i\n", no_conflicts);
    printf("        THREADS: %i\n", threads);
    printf("    NB_ACCOUNTS: %i\n", nb_accounts);
    printf("    BANK_BUDGET: %i\n", bank_budget);
    printf("   NB_TRANSFERS: %i\n", nb_transfers);
    printf(" TXs PER THREAD: %i\n", transactions);
    printf(" TRANSFER_LIMIT: %i\n", transfer_limit);
    printf(" ======================== \n");

	NH_free(NH_alloc(64)); // TEST
	
    NVHTM_init(threads);
	
	NH_free(NH_alloc(64)); // TEST

	if (no_conflicts) {
		pool = (GRANULE_TYPE**) malloc(threads * sizeof (GRANULE_TYPE*));
	
		for (i = 0; i < threads; ++i) {
			pool[i] = (GRANULE_TYPE*) NH_alloc(nb_accounts * CACHE_LINE_SIZE);
            initialize_pool(pool[i]);
		}
    }
    else {
		pool = (GRANULE_TYPE**) malloc(sizeof (GRANULE_TYPE*));
		*pool = (GRANULE_TYPE*) NH_alloc(nb_accounts * CACHE_LINE_SIZE);
    }
		
    srand(clock()); // TODO: seed

    int total = bank_total(pool[0]);
    if (reboot || total != bank_budget) {
        printf("Wrong bank amount: %i\n", total);
        initialize_pool(pool[0]);
    }
    total = bank_total(pool[0]);
    printf("Bank amount: %i\n", total);

    //    if (NVMHTM_is_crash(pool)) {
    //        printf("Is crash!\n");
    //        NVMHTM_recover(pool);
    //    }

    pthread_t pthrs[MAX_NB_THREADS];
    pthread_attr_t attr[MAX_NB_THREADS];
    cpu_set_t cpuset[MAX_NB_THREADS];
    void *res;

    TIMER_READ(c_ts1);

    args_threads[0] = 0;

    LOG_INIT("./log.txt");

    for (i = 1; i < threads; ++i) {
        int cpu_id;
		args_threads[i] = i;
        CPU_ZERO(&(cpuset[i]));
        if (i >= 14 && i < 28) {
            cpu_id = (i + 14) * THREAD_SPACING + THREAD_OFFSET;
        }
        else if (i >= 28 && i < 42) {
            cpu_id = (i - 14) * THREAD_SPACING + THREAD_OFFSET;
        }
        else {
            cpu_id = i * THREAD_SPACING + THREAD_OFFSET;
        }
		CPU_SET(cpu_id, &(cpuset[i]));
        pthread_attr_init(&(attr[i]));
        pthread_attr_setaffinity_np(&(attr[i]), sizeof (cpu_set_t), &(cpuset[i]));

        if (i > threads) {
			// printf("Waste cpu %i\n", cpu_id);
            pthread_create(&(pthrs[i]), &(attr[i]), waste_cpu, &(args_threads[i]));
        }
        else {
			// printf("Launch %i in CPU %i \n", i, cpu_id);
            pthread_create(&(pthrs[i]), &(attr[i]), random_transfer, &(args_threads[i]));
        }
    }

	// printf("Launch %i in CPU 0\n", 0);
    CPU_ZERO(cpuset);
    CPU_SET(0, cpuset);
    sched_setaffinity(0, sizeof (cpu_set_t), cpuset);
    random_transfer(&(args_threads[0]));

    // don't wait for the waste_cpu
    for (i = 1; i < threads; ++i) {
		// printf("Wait for %i\n", i);
        pthread_join(pthrs[i], &res);
    }

    TIMER_READ(c_ts2);

    double time_taken = TIMER_DIFF_SECONDS(c_ts1, c_ts2);
    int successes = TM_get_error_count(SUCCESS);
    int fallbacks = TM_get_error_count(FALLBACK);
    int aborts = TM_get_error_count(ABORT);
    int capacity = TM_get_error_count(CAPACITY);
    int conflicts = TM_get_error_count(CONFLICT);
    int commits = successes + fallbacks;
    double X = (double) commits / (double) time_taken;
    double P_A = (double) aborts / (double) (successes + aborts);

    NVHTM_shutdown();

    printf("     NB_UPDATES: %i\n", nb_write_txs);
    printf("     BANK_TOTAL: %i\n", bank_total(pool[0]));
    printf("     TIME_TAKEN: %f\n\n", time_taken);

    NVMHTM_mem_s *instance = NVMHTM_get_instance(pool[0]); // remove this
	#ifndef HTM_ONLY
    if (instance->chkp.ptr != NULL) {
        printf("Total in checkpoint: %i \n",
			bank_total((GRANULE_TYPE*)instance->chkp.ptr));
        TIMER_READ(c_ts1);
        NVHTM_recover();
        TIMER_READ(c_ts2);
        printf("bank total after recover: %i\n", bank_total(pool[0]));
        printf("Total in checkpoint: %i \n",
			bank_total((GRANULE_TYPE*)instance->chkp.ptr));
        printf("time to recover: %f s\n", TIMER_DIFF_SECONDS(c_ts2, c_ts2));
    }
	#endif

    if (gnuplot_file != NULL) {
        FILE *gp_fp = fopen(gnuplot_file, "a");

        if (ftell(gp_fp) < 8) {
            // not created yet

            fprintf(gp_fp, "#\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
                    "THREADS", "NB_ACCOUNTS", "BANK_BUDGET", "NB_TRANSFERS",
                    "TRANSFER_LIMIT", "TRANSFERS_IN_TX", "X", "P_A", "HTM_COMMITS",
                    "SGL_COMMITS", "ABORTS", "CAPACITY", "CONFLICTS");
        }
        fprintf(gp_fp, "\t%i\t%i\t%i\t%i\t%i\t%i\t%e\t%e\t%i\t%i\t%i\t%i\t%i\n",
                threads, nb_accounts, bank_budget, nb_transfers, transfer_limit,
				transactions, X, P_A, successes, fallbacks, aborts,
                capacity, conflicts);
        fclose(gp_fp);
    }
    // stdout
    printf("#\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
           "THREADS", "NB_ACCOUNTS", "BANK_BUDGET", "NB_TRANSFERS",
           "TRANSFER_LIMIT", "X", "P_A");
    printf("\t%i\t%i\t%i\t%i\t%i\t%e\t%e\n",
           threads, nb_accounts, bank_budget, nb_transfers,
           transfer_limit, X, P_A);

    int nb_spins = 0, clocks;

    LOG_CLOSE();

    return EXIT_SUCCESS;
}

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "thread.h"
#include "timer.h"

#include "tm.h"

#ifndef FILE_NAME
#define FILE_NAME "bank_stats"
#endif

// ########################## defines

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

#define ALIGNED __attribute__((aligned(CACHE_LINE_SIZE)))
#define GRANULE_T intptr_t

#define NOP_X10 asm volatile("nop\n\t\nnop\n\t\nnop\n\t\nnop\n\t\nnop" \
"\n\t\nnop\n\t\nnop\n\t\nnop\n\t\nnop\n\t\nnop" ::: "memory")

#define SPIN_TIME(nb) ({ \
	volatile int i; \
	for (i = 0; i < nb * 10; ++i) NOP_X10; \
	0; \
})

# define no_argument        0
# define required_argument  1
# define optional_argument  2

// TODO: cache-align vs non-cache-align
// TODO: bank accounts with different granularities
//  * CACHE_LINE_SIZE / sizeof(GRANULE_T)
#ifdef CACHE_ALIGN_POOL
#define GET_ACCOUNT(pool, i) (pool[i * CACHE_LINE_SIZE / sizeof(GRANULE_T)])
#else
#define GET_ACCOUNT(pool, i) (pool[i])
#endif

#define THREAD_SPACING 1
#define THREAD_OFFSET 0

#define READ_ONLY_NB_ACCOUNTS 200

#define INT_RATE 1.1D
#define NB_YEARS 5.5D
/*
#define RAND_R_FNC_aux(seed) \
({ \
unsigned seed_addr = seed; \
unsigned res = rand_r(&seed_addr); \
seed = seed_addr; \
res; \
})
// */
//*
#define RAND_R_FNC_aux(seed) rand_r(&seed)
/*({ \
	register unsigned long next = seed; \
	register unsigned long result; \
	next *= 1103515245; \
	next += 12345; \
	result = (unsigned long) (next / 65536) % 2048; \
	next *= 1103515245; \
	next += 12345; \
	result <<= 10; \
	result ^= (unsigned long) (next / 65536) % 1024; \
	next *= 1103515245; \
	next += 12345; \
	result <<= 10; \
	result ^= (unsigned long) (next / 65536) % 1024; \
	seed = next; \
	result; \
})*/
// */
// ########################## constants

// static const int  ---> not a constant in C
// static const int  ---> it is a constant in C++

#define DEFAULT_NB_ACCOUNTS     128
#define DEFAULT_NB_THREADS      1
#define DEFAULT_NB_NTX_THREADS  0
#define DEFAULT_BANK_BUDGET     5000
#define DEFAULT_TX_SIZE         1
#define DEFAULT_TRANSFER_LIM    3
#define DEFAULT_NB_TRANSFERS    5000
#define DEFAULT_SEQUENTIAL      0
#define DEFAULT_UPDATE_RATE     100
#define DEFAULT_NO_CONFL        0
#define DEFAULT_TX_TIME         0
#define DEFAULT_OUTSIDE_TIME    0
#define DEFAULT_READ_SIZE       200

// ########################## variables

static int nb_accounts    = DEFAULT_NB_ACCOUNTS,
nb_threads     = DEFAULT_NB_THREADS,
nb_ntx_threads = DEFAULT_NB_NTX_THREADS,
bank_budget    = DEFAULT_BANK_BUDGET,
tx_size        = DEFAULT_TX_SIZE,
transfer_lim   = DEFAULT_TRANSFER_LIM,
seq            = DEFAULT_SEQUENTIAL,
update_rate    = DEFAULT_UPDATE_RATE,
no_conflicts   = DEFAULT_NO_CONFL,
outside_time   = DEFAULT_OUTSIDE_TIME,
read_size      = DEFAULT_READ_SIZE,
tx_time        = DEFAULT_TX_TIME;

int nb_transfers = DEFAULT_NB_TRANSFERS;

static ALIGNED GRANULE_T **pool;
static __thread ALIGNED GRANULE_T *loc_pool;
static int nt_per_thread;
static TIMER_T c_ts1, c_ts2;
static pthread_barrier_t barrier;
static ALIGNED __thread int nb_write_txs_pt = 0;

static int nb_exit_threads = 0;
static unsigned long long count_time_tx;
static unsigned long long count_time_outside;
static double time_taken;
long nb_of_done_transactions = 0;

// ########################## functions
static int bank_total(GRANULE_T *pool);
static void initialize_pool(GRANULE_T *pool);
static int bank_total_tx(GRANULE_T *pool);
// static int bank_total_tx_spec(GRANULE_T *pool);
static void* random_transfer(void *arg); // runs the transactions
static void* random_transfer_thread13(void *arg); // runs the transactions
static void stats_to_gnuplot_file(char *filename);
// ########################## main

MAIN(argc, argv)
{
	int c, i;

	struct option long_options[] = {
		// These options don't set a flag
		{"help",                      no_argument,       NULL, 'h'},
		{"sequential",                no_argument,       NULL, 'q'},
		{"no-confl",                  no_argument,       NULL, 'c'},
		{"duration",                  required_argument, NULL, 'd'},
		{"num-accounts",              required_argument, NULL, 'a'},
		{"num-threads",               required_argument, NULL, 'n'},
		{"update-rate",               required_argument, NULL, 'u'},
		{"size-tx",                   required_argument, NULL, 's'},
		{"budget",                    required_argument, NULL, 'b'},
		{"wait",                      required_argument, NULL, 'w'},
		{"transfer-lim",              required_argument, NULL, 't'},
		{"outside",                   required_argument, NULL, 'o'},
		{"read-size",                 required_argument, NULL, 'r'},
		{NULL, 0, NULL, 0}
	};

	while(1) {
		i = 0;
		c = getopt_long(argc, argv, "hqcd:a:n:u:s:b:t:w:o:m:r:",
		long_options, &i);

		if(c == -1)
		break;

		if(c == 0 && long_options[i].flag == 0)
		c = long_options[i].val;

		switch(c) {
			case 0:
			/* Flag is automatically set */
			break;
			case 'h':
			printf("bank -- STM stress test "
			"\n"
			"Usage:\n"
			"  bank [options...]\n"
			"\n"
			"Options:\n"
			"  -h, --help\n"
			"        Print this message\n"
			"  -q, --sequential\n"
			"        Transfer from contiguos accounts\n"
			"  -c, --no-confl\n"
			"        Use different accounts per thread\n"
			"  -d, --duration <int>\n"
			"        Number of transfers (default=" XSTR(DEFAULT_NB_TRANSFERS) ")\n"
			"  -a, --nb-accounts <int>\n"
			"        Number of accounts (default=" XSTR(DEFAULT_NB_ACCOUNTS) ")\n"
			"  -n, --num-threads <int>\n"
			"        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
			"  -m, --num-ntx-threads <int>\n"
			"        Number of threads not running transactions (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
			"  -u, --update-rate <int>\n"
			"        Percentage of updates (default=" XSTR(DEFAULT_UPDATE_RATE) ")\n"
			"  -s, --size-tx <int>\n"
			"        Number transfers in transaction (default=" XSTR(DEFAULT_TX_SIZE) ")\n"
			"  -b, --budget <int>\n"
			"        Bank budget (default=" XSTR(DEFAULT_BANK_BUDGET) ")\n"
			"  -t, --transfer-lim <int>\n"
			"        Transfer limit (default=" XSTR(DEFAULT_TRANSFER_LIM) ")\n"
			"  -w, --wait <int>\n"
			"        Transaction time (default=" XSTR(DEFAULT_TX_TIME) ")\n"
			"  -o, --outside <int>\n"
			"        Outside transaction time (default=" XSTR(DEFAULT_OUTSIDE_TIME) ")\n"
			"  -r, --read-size <int>\n"
			"        Outside transaction time (default=" XSTR(DEFAULT_OUTSIDE_TIME) ")\n"
		);
		exit(EXIT_SUCCESS);
		case 'q':
		seq = 1;
		break;
		case 'c':
		no_conflicts = 1;
		break;
		case 'd':
		nb_transfers = atoi(optarg);
		break;
		case 'b':
		bank_budget = atoi(optarg);
		break;
		case 'a':
		nb_accounts = atoi(optarg);
		break;
		case 'n':
		nb_threads = atoi(optarg);
		break;
		case 'm':
		nb_ntx_threads = atoi(optarg);
		break;
		case 's':
		tx_size = atoi(optarg);
		break;
		case 't':
		transfer_lim = atoi(optarg);
		break;
		case 'u':
		update_rate = atoi(optarg);
		break;
		case 'w':
		tx_time = atoi(optarg);
		break;
		case 'o':
		outside_time = atoi(optarg);
		break;
		case 'r':
		read_size = atoi(optarg);
		break;
		case '?':
		printf("Use -h or --help for help\n");
		exit(EXIT_SUCCESS);
		default:
		exit(EXIT_FAILURE);
	}
}

#ifdef CACHE_ALIGN_POOL
printf(" ########## Bank is cache align! ########## \n");
#endif

nt_per_thread = nb_transfers / nb_threads;

pthread_barrier_init(&barrier, NULL, nb_threads);

// thread example
printf(" Start program ========== \n");
printf("   NO_CONFLICTS: %i\n", no_conflicts);
printf("     SEQUENTIAL: %i\n", seq);
printf("     NB_THREADS: %i\n", nb_threads);
printf("   NB_TRANSFERS: %i\n", nb_transfers);
printf("    NB_ACCOUNTS: %i\n", nb_accounts);
printf("    BANK_BUDGET: %i\n", bank_budget);
printf("        TX_SIZE: %i\n", tx_size);
printf("        TX_TIME: %i\n", tx_time);
printf("   OUTSIDE_TIME: %i\n", outside_time);
printf("    UPDATE_RATE: %i\n", update_rate);
printf(" TRANSFER_LIMIT: %i\n", transfer_lim);
printf(" -------------------- \n");
printf(" TXs PER THREAD: %i\n", nt_per_thread);
printf(" ======================== \n");

// TODO: these TM_MALLOCs should be probably normal mallocs
if (no_conflicts) {
	pool = (GRANULE_T**) malloc(nb_threads * sizeof (GRANULE_T*));

	for (i = 0; i < nb_threads; ++i) {
		pool[i] = (GRANULE_T*) malloc(nb_accounts * CACHE_LINE_SIZE);
		initialize_pool(pool[i]);
	}
}
else {
	pool = (GRANULE_T**) malloc(sizeof (GRANULE_T*));
	*pool = (GRANULE_T*) malloc(nb_accounts * CACHE_LINE_SIZE);
}

srand(clock()); // TODO: seed

// TODO: in NVM context test if there is money in the bank
int total = bank_total(pool[0]);
if (total != bank_budget) {
	printf("Wrong bank amount: %i\n", total);
	initialize_pool(pool[0]);
}
total = bank_total(pool[0]);
printf("Bank amount: %i\n", total);

SIM_GET_NUM_CPU(nb_threads);
TM_STARTUP(nb_threads);
P_MEMORY_STARTUP(nb_threads);
thread_startup(nb_threads);

TIMER_READ(c_ts1);
GOTO_SIM();
thread_start2(random_transfer, random_transfer_thread13, NULL);

GOTO_REAL();
TIMER_READ(c_ts2);

time_taken = TIMER_DIFF_SECONDS(c_ts1, c_ts2);
printf("\nTime = %0.6lf\n", time_taken);

stats_to_gnuplot_file(FILE_NAME);

TM_SHUTDOWN();
P_MEMORY_SHUTDOWN();
thread_shutdown();

MAIN_RETURN(EXIT_SUCCESS);
}

// ########################## function implementation

static void transaction(int tid, int update_rate_loc, int nb_accounts_loc,
	int seq_loc, unsigned seed, int tx_time_loc, int tx_size_loc, int transfer_lim_loc) {

	int sender_amount;
	int recipient_amount;
	int sender_new_amount;
	int recipient_new_amount;

	int transfer_amount;
	int recipient;
	int sender;

	int i;

	if ((RAND_R_FNC_aux(seed) % 100) < update_rate_loc) {
		TM_SAVE(seed);
		TM_BEGIN_EXT(0, 0);
		TM_RESTORE(seed);

		if (seq_loc) {
			sender = (RAND_R_FNC_aux(seed) % nb_accounts_loc);
		}

		// -------------------------------------------
		// do some computation that is not erased by the compiler
		for (i = 0; i < tx_size_loc ; ++i) {

			if (!seq_loc) {
				recipient = (RAND_R_FNC_aux(seed) % nb_accounts_loc);
				sender = (RAND_R_FNC_aux(seed) % nb_accounts_loc);
			} else {
				RAND_R_FNC_aux(seed);
				RAND_R_FNC_aux(seed);
				recipient = (sender + 10) % nb_accounts_loc;
				sender = (recipient + 10) % nb_accounts_loc;
			}

			if (sender == recipient) {
				--i;
				continue;
			}

			// Read
			sender_amount = TM_SHARED_READ(GET_ACCOUNT(loc_pool, sender));
			recipient_amount = TM_SHARED_READ(GET_ACCOUNT(loc_pool, recipient));

			// Process
			transfer_amount = (RAND_R_FNC_aux(seed) % transfer_lim_loc) + 1; //

			// TODO:
			transfer_amount = (int) ((double) transfer_amount *
				pow(INT_RATE, NB_YEARS));
			sender_new_amount = sender_amount - transfer_amount;
			recipient_new_amount = recipient_amount + transfer_amount;

			if (sender_new_amount < 0) {
				// the sender does not have the money! write back the same
				sender_new_amount = sender_amount;
				recipient_new_amount = recipient_amount;
			}

			// Write
			//if ((RAND_R_FNC_aux(seed) % 100) < update_rate_loc) {
			TM_SHARED_WRITE(GET_ACCOUNT(loc_pool, sender), sender_new_amount);
			TM_SHARED_WRITE(GET_ACCOUNT(loc_pool, recipient), recipient_new_amount);
			//}

			// spin here (probably the where to spin also affects prob abort)
			SPIN_TIME(tx_time_loc); // set to 0 to ignore spin
		}
		// -------------------------------------------

		TM_END();
	} else {
		bank_total_tx(loc_pool);
	}
}

static void* random_transfer(void *arg)
{
	TM_THREAD_ENTER();

	int i;
	int tx;
	int nb_accounts_loc  = nb_accounts,
	update_rate_loc  = update_rate,
	tx_size_loc      = tx_size,
	transfer_lim_loc = transfer_lim,
	tx_time_loc      = tx_time;
	int tid;
	int seq_loc = seq;

	intptr_t total;

	// TM_free(TM_alloc(64)); // TEST

	tid = thread_getId();

	unsigned seed = 123456 ^ (tid << 3);

	if (tid >= nb_threads) {
		return NULL; // error!
	}

	if (no_conflicts) {
		loc_pool = pool[tid];
	}
	else {
		loc_pool = *pool;
	}

	for (tx = 0; tx < nt_per_thread; ++tx) {
	// for (tx = 0; (tx < nt_per_thread || nb_exit_threads < nb_threads); ++tx) {
	// while (__sync_add_and_fetch(&nb_of_done_transactions, 1) < nb_transfers) {

	int sender_amount;
	int recipient_amount;
	int sender_new_amount;
	int recipient_new_amount;

	int transfer_amount;
	int recipient;
	int sender;

	if ((RAND_R_FNC_aux(seed) % 100) < update_rate_loc) {
		TM_SAVE(seed);
		TM_BEGIN_EXT(0, 0);
		TM_RESTORE(seed);

		if (seq_loc) {
			sender = (RAND_R_FNC_aux(seed) % nb_accounts_loc);
		}

		// -------------------------------------------
		// do some computation that is not erased by the compiler
		for (i = 0; i < tx_size_loc ; ++i) {

			if (!seq_loc) {
				recipient = (RAND_R_FNC_aux(seed) % nb_accounts_loc);
				sender = (RAND_R_FNC_aux(seed) % nb_accounts_loc);
			} else {
				recipient = (sender + 10) % nb_accounts_loc;
				sender = (recipient + 10) % nb_accounts_loc;
			}

			if (sender == recipient) {
				--i;
				continue;
			}

			// Read
			sender_amount = TM_SHARED_READ(GET_ACCOUNT(loc_pool, sender));
			recipient_amount = TM_SHARED_READ(GET_ACCOUNT(loc_pool, recipient));

			// Process
			transfer_amount = (RAND_R_FNC_aux(seed) % transfer_lim_loc) + 1; //

			// TODO:
			transfer_amount = (int) ((double) transfer_amount *
				pow(INT_RATE, NB_YEARS));
			sender_new_amount = sender_amount - transfer_amount;
			recipient_new_amount = recipient_amount + transfer_amount;

			if (sender_new_amount < 0) {
				// the sender does not have the money! write back the same
				sender_new_amount = sender_amount;
				recipient_new_amount = recipient_amount;
			}

			// Write
			//if ((RAND_R_FNC_aux(seed) % 100) < update_rate_loc) {
			TM_SHARED_WRITE(GET_ACCOUNT(loc_pool, sender), sender_new_amount);
			TM_SHARED_WRITE(GET_ACCOUNT(loc_pool, recipient), recipient_new_amount);
			//}

			// spin here (probably the where to spin also affects prob abort)
			SPIN_TIME(tx_time_loc); // set to 0 to ignore spin
		}
		// -------------------------------------------

		TM_END();
	} else {
		bank_total_tx(loc_pool);
	}

		unsigned o_time = outside_time > 0
		? RAND_R_FNC_aux(seed) % outside_time
		: 0;
		SPIN_TIME(o_time);

		if (tx == nt_per_thread) {
			__sync_add_and_fetch(&nb_exit_threads, 1); // TODO
			__sync_synchronize();
		}
	}

	// printf("thread %i exit!\n", tid);
	TM_THREAD_EXIT();
	return (void*) bank_total;
}

static void* random_transfer_thread13(void *arg)
{
	TM_THREAD_ENTER();

	int i;
	int tx;
	int nb_accounts_loc  = nb_accounts,
	update_rate_loc  = update_rate,
	tx_size_loc      = tx_size,
	transfer_lim_loc = transfer_lim,
	tx_time_loc      = tx_time;
	int tid;
	int seq_loc = seq;

	intptr_t total;

	// TM_free(TM_alloc(64)); // TEST

	tid = thread_getId();

	unsigned seed = 123456 ^ (tid << 3);

	if (tid >= nb_threads) {
		return NULL; // error!
	}

	if (no_conflicts) {
		loc_pool = pool[tid];
	}
	else {
		loc_pool = *pool;
	}

	/* total = bank_total(loc_pool); // loads array */

	// pthread_barrier_wait(&barrier);

	for (tx = 0; tx < nt_per_thread; ++tx) {
	// for (tx = 0; (tx < nt_per_thread || nb_exit_threads < nb_threads); ++tx) {
	// while (__sync_add_and_fetch(&nb_of_done_transactions, 1) < nb_transfers) {

		transaction(tid, update_rate_loc, nb_accounts_loc, seq_loc, seed, tx_time_loc, tx_size_loc, transfer_lim_loc);

		unsigned o_time = outside_time > 0
		? RAND_R_FNC_aux(seed) % outside_time
		: 0;
		SPIN_TIME(o_time);

		if (tx == nt_per_thread) {
			__sync_add_and_fetch(&nb_exit_threads, 1); // TODO
			__sync_synchronize();
		}
	}

	// printf("thread %i exit!\n", tid);
	TM_THREAD_EXIT();
	return (void*) bank_total;
}

static void initialize_pool(GRANULE_T *pool)
{
	int i;

	int inc_step = 10;

	for (i = 0; i < nb_accounts; ++i) {
		GET_ACCOUNT(pool, i) = 0; // Replace with memset
	}

	for (i = inc_step; i < bank_budget; i += inc_step) {
		int account = rand() % nb_accounts;
		int new_val = GET_ACCOUNT(pool, account) + inc_step;

		GET_ACCOUNT(pool, account) = new_val;
	}

	i -= inc_step;

	for (; i < bank_budget; ++i) {
		int account = rand() % nb_accounts;
		int new_val = GET_ACCOUNT(pool, account) + 1;

		GET_ACCOUNT(pool, account) = new_val;
	}

	// check the total in the checkpoint
	// printf("Total in checkpoint: %i \n", bank_total(pool));
}

static int bank_total(GRANULE_T *pool)
{
	int i, res = 0;

	for (i = 0; i < nb_accounts; ++i) {
		res += GET_ACCOUNT(pool, i);
	}

	return res;
}

static int bank_total_tx(GRANULE_T *pool)
{
	int i, read_size_loc = read_size;
	int nb_accounts_loc = nb_accounts;
	ALIGNED GRANULE_T **pool_loc = pool;
	int res = 0, rand;
	static __thread unsigned aux_seed = 1234;

	TM_SAVE(res);
	TM_SAVE(aux_seed);
	TM_BEGIN_EXT(0, 0);
	TM_RESTORE(aux_seed);
	TM_RESTORE(res);
	for (i = 0; i < read_size_loc; ++i) {
		int j;
		rand = (RAND_R_FNC_aux(aux_seed) % nb_accounts);
		// if seq -->
		j = (rand + i * 10) % nb_accounts_loc;
		res += (int) (pow(INT_RATE, NB_YEARS) * (double) ((intptr_t)TM_SHARED_READ(GET_ACCOUNT(pool_loc, j))));
	}
	TM_END();

	return res;
}

// static int bank_total_tx_spec(GRANULE_T *pool)
// {
// 	int i, read_size_loc = read_size;
// 	int nb_accounts_loc = nb_accounts;
// 	ALIGNED GRANULE_T **pool_loc = pool;
// 	int res = 0, rand;
// 	static __thread unsigned aux_seed = 1234;
//
//
// 	TM_BEGIN_EXT_spec(0, 0);
// 	for (i = 0; i < read_size_loc; ++i) {
// 		int j;
// 		rand = (RAND_R_FNC_aux(aux_seed) % nb_accounts);
// 		// if seq -->
// 		j = (rand + i * 10) % nb_accounts_loc;
// 		res += (int) (pow(INT_RATE, NB_YEARS) * (double) ((intptr_t)TM_SHARED_READ(GET_ACCOUNT(pool_loc, j))));
// 	}
// 	TM_END();
//
// 	return res;
// }

static void stats_to_gnuplot_file(char *filename) {
	FILE *gp_fp = fopen(filename, "a");
	if (ftell(gp_fp) < 8) {
		fprintf(gp_fp, "#"
		"TIME\t"              // [1]TIME
		"TIME_TX\t"           // [2]TIME_TX
		"TIME_NTX\n"          // [3]TIME_NTX
	);
}

double total_time_tx      = (double) count_time_tx / nb_threads / CPU_MAX_FREQ;
double total_time_outside = (double) count_time_outside / nb_threads / CPU_MAX_FREQ;

fprintf(gp_fp, "%f\t", time_taken);                   // [1]TIME
fprintf(gp_fp, "%f\t", total_time_tx / 1000.0);       // [2]TIME_TX
fprintf(gp_fp, "%f\n", total_time_outside / 1000.0);  // [3]TIME_NTX
fclose(gp_fp);

printf("printed stats\n");
}

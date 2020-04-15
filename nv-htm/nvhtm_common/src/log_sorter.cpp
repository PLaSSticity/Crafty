#include "nvhtm_helper.h"
#include "log_sorter.h"
#include "log_aux.h"
#include "log.h"
#include "cp.h"
#include <thread>

using namespace std;

// ############################ input variables
static cp_s *cp_instance;
static NVLog_s** logs;
static size_t nb_logs;

// ############################ local_variables
static ts_s last_timestamp = 0;
static int* last_log_ptr;

sorter_s *sorter;

// ############################ local functions
static void sort_one();

static bool in_between(int start, int b, int end);
static void next_tx(int sorter_id, NVLog_s *log);

// ############################ header implementation

// TODO: pass logs correctly
void LOG_SOR_init(cp_s* cp_instance_, NVLog_s** logs_, size_t nb_logs_)
{
	int i;

	cp_instance = cp_instance_;
	logs_ = logs;
	nb_logs = nb_logs_;

	sorter = (sorter_s*) malloc(nb_logs * sizeof(sorter_s));
	for (i = 0; i < nb_logs; ++i) {
		sorter[i].tid = i;
		sorter[i].ts = 0;
		sorter[i].pos_base = -1;
	}
}

void *LOG_SOR_main_thread(void*)
{
	// TODO: is_exit
	MN_thr_enter();
	LOG_local_state.size_of_log = NH_global_logs[0]->size_of_log; // TODO
	while (1) {
		sort_one();
	}
	MN_thr_exit();
	return NULL;
}

// ############################ local functions implementation

static void sort_one() {
	NVLog_s* log;
	NVLogLocation_s loc;
	int i;
	int log_start, log_end;
	static ts_s last_ts = 0;
	static int last_tid = 0;
	static int last_pos = 0;
	static int nb_txs = 0;
	static int did_produce = 0;

	// next transaction to apply (smallest TS)
	sorter_s *update = NULL;

	bool too_full = false;
	bool too_empty = false;

	// Test if we are too close to the end
	for (i = 0; i < TM_nb_threads; ++i) {
		log = NH_global_logs[i];
		log_start = sorter[i].pos_base;
		log_end = log->end;

		if (log_start == -1) continue;

		int dist = distance_ptr(log_start, log_end);

		if (dist > TOO_FULL) {
			too_full = true;
		}
	}

	for (i = 0; i < TM_nb_threads; ++i) {
		log = NH_global_logs[i];
		// TODO: log_end -> 8 == (2CL/16) (avoid conflicts)
		log_start = log->start, log_end = log->end;

		if (log_start == log_end) continue;

		if (sorter[i].pos_base == -1 || sorter[i].ts == 0) {
			// not initiallized
			next_tx(i, log);
		}

		// if update is not set or is set to zero
		if (update == NULL || update->ts == 0) {
			update = &(sorter[i]);
		} else if (update->ts > sorter[i].ts && sorter[i].ts != 0) {
			update = &(sorter[i]);
		}
	}

	if (update != NULL) {

		i = update->tid;
		log = NH_global_logs[i];

		// ------------------------------------------------------
		// Test if we are too close to the end
		if (!too_full) {
			// printf("[SORTER]: not too full\n");
			log_start = update->pos_base;
			log_end = log->end;

			if (log_start == -1) log_start = log->start;

			int dist = distance_ptr(log_start, log_end);

			if (dist < TOO_EMPTY) {
				return; // try again later
			}
		}
		// ------------------------------------------------------
		// printf("[SORTER]: ||| start=%5i (real=%5i) end=%5i\n",
		// update->pos_base, log->start, log->end);

		log_start = log->start;
		log_end = log->end;

		if (update->ts == 0 || update->pos_base == -1) {
			this_thread::yield();
			// printf("[SORTER]: ||| TOO_FAST\n");
			// sorter is going too fast
			// return; // BUG --> out of order txs
			// printf("error tid=%i\n",update->tid);
		}

		loc.tid = update->tid;
		loc.ptr = update->pos_base;
		loc.ts = update->ts;

		while(!(did_produce = cp_produce(NULL, &loc))) {
			this_thread::yield();
		}
		// printf("[SORTER]: ||| produce: %5i\n", update->pos_base);

		// printf("Produced: start = %5i end = %5i\n", log_start, log_end);

		// if buffer not full, move to the next transaction in the log

		// There is a bug where the sorter may miss to see a transaction
		// close to the log->end (inter-thread communication?)

		assert(update->ts == 0 || last_ts <= update->ts);

		last_ts = update->ts;
		last_tid = update->tid;
		last_pos = update->pos_base;
		log = NH_global_logs[i];
		next_tx(i, log); // this is causing lots of aborts!!! use with caution

		nb_txs++;
	}
}

static bool in_between(int start, int b, int end)
{
	int size_log = distance_ptr(start, end);
	int dist_end = distance_ptr(b, end);
	bool res = size_log >= dist_end;

	return res;
}

static ts_s next_ts(int idx, NVLog_s *log)
{
	int log_start, log_end;
	ts_s ts_val;
	log_end = log->end;
	log_start = log->start;

	while (idx != log_end) { // This is a TX nuke! abort lots of TXs
		ts_val = entry_is_ts(log->ptr[idx]);
		if (ts_val) {
			return ts_val;
		}
		idx = ptr_mod(idx, 1, LOG_local_state.size_of_log);
	}

	return 0;
}

static void next_tx(int sorter_id, NVLog_s *log)
{
	int i, pos = sorter[sorter_id].pos_base;
	int log_start, log_end;
	ts_s ts_val;
	log_end = log->end;
	log_start = log->start;

	if (pos == -1) {
		pos = log_start;
		ts_val = next_ts(pos, log);
		if (ts_val != 0) {
			sorter[sorter_id].ts = ts_val;
			sorter[sorter_id].pos_base = pos;
			return;
		}
	}

	i = pos;
	while (i != log_end) { // This is a TX nuke! abort lots of TXs
		ts_val = entry_is_ts(log->ptr[i]);
		if (ts_val) {
			i = ptr_mod_log(i, 1);
			break;
		}
		i = ptr_mod_log(i, 1);
	}

	ts_val = next_ts(i, log);
	if (ts_val != 0) {
		sorter[sorter_id].ts = ts_val;
		sorter[sorter_id].pos_base = i;
	} else {
		sorter[sorter_id].ts = 0;
		sorter[sorter_id].pos_base = pos;
	}
}

#include "log.h"
#include <string.h>
#include "extra_globals.h"

#include <map>



Crafty_log_s *crafty_create_log()
{
	Crafty_log_s *res;

	res = (Crafty_log_s*) malloc(sizeof (Crafty_log_s));

	memset(res, 0, sizeof (Crafty_log_s));

	return res;
}




void crafty_thr_init(int tid) {
    crafty_aborts_in_redo = 0;
    crafty_aborts_in_SGL = 0;

    crafty_validate_success = 0;
    crafty_replay_success = 0;
    crafty_readonly_success = 0;

    crafty_redo_abort_conflict = 0;
    crafty_redo_abort_capacity = 0;
    crafty_redo_abort_zero = 0;
    crafty_redo_abort_explicit = 0;

    if (!crafty_log) {
        crafty_log = crafty_create_log();
        crafty_log_curr_pos = crafty_log_tx_start = &crafty_log->entries[0];
        crafty_log_upper_bound = &crafty_log->entries[LOG_ENTRIES];
        // Insert a dummy commit entry
        crafty_log_entry(COMMIT_ENTRY_ADDR, 0);
        crafty_log_tx_start = crafty_log_curr_pos;
    }
    // TODO: Following lines are racy & ideally should be fixed
    last_tx_ts_arr[tid] = &last_tx_ts;
    log_curr_pos_arr[tid] = &crafty_log_curr_pos;
    log_tx_start_arr[tid] = &crafty_log_tx_start;
    log_arr[tid] = crafty_log;
#ifdef ROLLOVER_BITS
    rollover_bit_arr[tid] = &crafty_rollover_bit;
#endif
#ifdef CRAFTY_REDO
	last_log_time = 0;
#endif // CRAFTY_REDO

#ifdef CRAFTY_STATS
	crafty_alloc_high_mark = 0;
	crafty_free_high_mark = 0;
    crafty_allocs_logged = 0;
    crafty_write_count = 0;
    crafty_currenttx_write_count = 0;
    crafty_valfail = 0;
    crafty_validate_success = 0;
    crafty_replay_success = 0;
    crafty_replayfail = 0;
    crafty_aborts_in_logging = 0;
    crafty_aborts_in_validating = 0;
    crafty_aborts_in_redo = 0;
    crafty_aborts_in_SGL = 0;
    crafty_SGL_in_logging = 0;
    crafty_SGL_in_validating = 0;
    crafty_log_sections = 0;
    crafty_singlewrite_SGL = 0;
    memset(&crafty_aborts, 0, sizeof(crafty_aborts));
    memset(crafty_write_histogram, 0, sizeof(*crafty_write_histogram));
#endif
}

static std::map<void *, char const *> freed_memory;
void crafty_safe_free(void * ptr, char const * reason) {
    thrprintf("free %s %p\n", reason, ptr);
    static int lock = 0;
    while (!__sync_bool_compare_and_swap(&lock, 0, 1)) PAUSE();
    auto existing = freed_memory.insert(std::make_pair(ptr, reason));
    if(!existing.second) {
        printf("Double free %p, previously freed for %s, now freed for %s\n", ptr, existing.first->second, reason);
        txassert(0);
    }
    if (freed_memory.size() > 1000000) {
        // Otherwise can eat up all memory in a system
        for (auto p : freed_memory) free(p.first);
        freed_memory.clear();
    }
    __sync_bool_compare_and_swap(&lock, 1, 0);
}
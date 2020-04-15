#include "log.h"
#include "cp.h"

#include <map>
#include <unordered_map>
#include <utility>
#include <set>
#include <unordered_set>
#include <list>
#include <thread>
#include <mutex>

using namespace std;

int LOG_checkpoint_forward()
{
  return LOG_checkpoint_forward_apply_one(true, true, NULL);
}

// TODO: before advancing the start pointers they must be buffered
static int *snapshot_start_ptrs, *old_ptrs, nb_entries;
static unordered_set<uintptr_t> buffered_cls; // TODO: uses unordered_set

#define BUFFERING_THRESHOLD 0.5

static void move_ptrs()
{
  int i;

  // TODO: flush the old ptrs (on recovery UNDO ptrs)

  for (i = 0; i < TM_nb_threads; ++i) {
    auto log = NH_global_logs[i];
    log->start = snapshot_start_ptrs[i];
    // -----------
    // comment to old
    SPIN_PER_WRITE(1); // TODO: NM_flush
    // -----------
  }
  __sync_synchronize();
}

static void flush_buffered()
{
  unordered_set<uintptr_t>::iterator it;

  for (it = buffered_cls.begin(); it != buffered_cls.end(); ++it) {
    SPIN_PER_WRITE(1); // TODO: NM_flush
  }
  buffered_cls.clear();
}

// Use producer/consumer
int LOG_checkpoint_forward_apply_one(int, int do_flush,  void *to_fl)
{
  register int i;
  NVLogLocation_s loc;
  int log_start, log_end;
  int dist, threshold, entries_threshold;
  NVLog_s* log, *smallest_log;
  int tid = -1;
  ts_s next_ts;
  bool too_full = false;
  bool too_empty = false;
  bool someone_passed = false;
  int sum_dist = 0;
  static ts_s last_ts = 0;
  static int old_nb_entries = 0, count_rep = 0;

  // return 1;

  if (snapshot_start_ptrs == NULL) {
    // TODO: move to initialization
    snapshot_start_ptrs = (int*)malloc(sizeof(int) * TM_nb_threads);
    old_ptrs = (int*)malloc(sizeof(int) * TM_nb_threads);
    buffered_cls.reserve(LOG_local_state.size_of_log * TM_nb_threads);
    for (i = 0; i < TM_nb_threads; ++i) {
      snapshot_start_ptrs[i] = NH_global_logs[i]->start;
      old_ptrs[i] = 0;
    }
  }

  ts_s ts1 = 0, ts2 = 0;

  threshold = (int)((double)(LOG_local_state.size_of_log)
    * (double)BUFFERING_THRESHOLD);
  entries_threshold = (int)((double)(LOG_local_state.size_of_log)
    * TM_nb_threads * (double)BUFFERING_THRESHOLD);

  // ts1 = rdtscp();
  for (i = 0; i < TM_nb_threads; ++i) {
    log = NH_global_logs[i];
    log_start = log->start;
    log_end = log->end;
    dist = distance_ptr(log_start, log_end);
    sum_dist += dist;
    if (dist < TOO_EMPTY) {
      too_empty = true;
    }
    if (dist > TOO_FULL) {
      too_full = true;
      someone_passed = true;
    }

    if (dist > threshold) {
      someone_passed = true;
    }
  }

  // TODO: with 2 or more threads, the logs fill so fast that
  // this if triggers always (producing a write+flush), the
  // nb_entries > threshold ensures that at least nb_entries
  // are buffered (it should be NB_ACTIVE_THREADS * threshold,
  // but if only 1 thread produces logs that one blocks)
  if (nb_entries == old_nb_entries) {
    // are the workers blocked?
    count_rep++;
  }

  // -----------
  // comment to old
  if (someone_passed && (nb_entries > entries_threshold
      || nb_entries >= (sum_dist - WAIT_DISTANCE)
      || count_rep >= 128)) {
    count_rep = 0;
    // printf("entries: %i, dist: %i\n", nb_entries, sum_dist);
    // empty the buffered log
    NH_nb_checkpoints++;
    nb_entries = 0;
    flush_buffered();
    move_ptrs();
  }
  // -----------

  NH_nb_checkpoints++;
  old_nb_entries = nb_entries;

  if (!too_full && too_empty) {
    return 1;
  }

  // consume from buffer
  if (!cp_consume(NULL, &loc)) {
    return 1;
  }

  next_ts = loc.ts;
  tid = loc.tid;
  smallest_log = NH_global_logs[tid];

  log_start = smallest_log->start;
  log_end = smallest_log->end;

  i = snapshot_start_ptrs[tid]; // buffered new start pointers

  // ts2 = rdtscp();
  // NH_manager_order_logs += ts2 - ts1;
  //
  while (i != log_end) {
    NVLogEntry_s entry = smallest_log->ptr[i];
    ts_s ts_val = entry_is_ts(entry);
    nb_entries++;
    if (ts_val) {

      last_ts = ts_val;

      i = ptr_mod_log(i, 1);

      /* smallest_log->start_tx = i;
      smallest_log->start = i;
      SPIN_PER_WRITE(1);
      __sync_synchronize(); */
      // buffer the transaction
      snapshot_start_ptrs[tid] = i;
      // -----------
      // comment to new
      // move_ptrs(); // moves right way
      // SPIN_PER_WRITE(1);
      // MN_count_writes++;
      // -----------
      break;
    }

    if (entry_is_update(entry)) {
      uintptr_t addr = (((uintptr_t)entry.addr) >> 6);
      buffered_cls.insert(addr);
      MN_write(entry.addr, &(entry.value), sizeof(GRANULE_TYPE), 1);
      // -----------
      // comment to new
      // SPIN_PER_WRITE(1); // flushes right away
      // -----------
    }

    i = ptr_mod_log(i, 1);
  }

  return 0;
}

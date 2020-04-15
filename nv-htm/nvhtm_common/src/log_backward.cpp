#include "log.h"

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <set>
#include <list>
#include <thread>
#include <mutex>
#include <vector>
#include <unistd.h>

using namespace std;

// pos has an hint, return the next tx after the given ts
static ts_s max_tx_after(NVLog_s *log, int start, ts_s target, int *pos)
{
  int hint = *pos;
  for (; hint != start; hint = ptr_mod_log(hint, -1)) {
    ts_s ts = entry_is_ts(log->ptr[hint]);
    if (ts && ts <= target) {
      *pos = ptr_mod_log(hint, -1); // updates for the next write
      return ts;
    }
  }
  *pos = start; // did not find
  return 0;
}

// pos has an hint PER LOG (array), return the idx of the next log to apply
static int max_next_log(int *pos, int starts[], int ends[])
{
  int i;
  ts_s max_ts[TM_nb_threads];
  ts_s disc_max;
  int disc_max_pos;
  int count_ends = 0;
  for (i = 0; i < TM_nb_threads; ++i) {
    max_ts[i] = 0;
    NVLog_s *log = NH_global_logs[i];
    ts_s ts = entry_is_ts(log->ptr[pos[i]]);
    while (!ts && pos[i] != starts[i]) {
      pos[i] = ptr_mod_log(pos[i], -1);
      ts = entry_is_ts(log->ptr[pos[i]]);
    }
    if (pos[i] == starts[i]) {
      count_ends++;
    }
    if (starts[i] != ends[i]) {
      max_ts[i] = ts;
    }
  }
  if (count_ends == TM_nb_threads) {
    return -1; // no more transactions
  }
  disc_max = max_ts[0];
  disc_max_pos = 0;
  for (i = 1; i < TM_nb_threads; ++i) {
    if (disc_max < max_ts[i]) {
      // some ts may be 0 if log ended
      disc_max = max_ts[i];
      disc_max_pos = i;
    }
  }
  return disc_max_pos;
}

// Apply log backwards and avoid repeated writes
int LOG_checkpoint_backward_apply_one()
{
  // this function needs a huge refactoring!
  int i, j, targets[TM_nb_threads], size_hashmap = 0,
  starts[TM_nb_threads], ends[TM_nb_threads];
  int log_start, log_end;
  bool too_full = false;
  bool too_empty = false;
  bool someone_passed = false;
  int dist;
  NVLog_s *log;

  if (LOG_local_state.size_of_log == 0) {
    LOG_local_state.size_of_log = NH_global_logs[0]->size_of_log;
  }

  typedef struct _CL_BLOCK {
    char bit_map;
  } CL_BLOCK;

  // sem_wait(NH_chkp_sem);
  *NH_checkpointer_state = 1; // doing checkpoint
  asm volatile ("sfence" ::: "memory"); // __sync_synchronize();

  // stores the possible repeated writes
  unordered_map<GRANULE_TYPE*, CL_BLOCK> writes_map;
  vector<GRANULE_TYPE*> writes_list;

  writes_list.reserve(32000);
  writes_map.reserve(32000);

  // find target ts, and the idx in the log
  ts_s target_ts = 0;
  int pos[TM_nb_threads], pos_to_start[TM_nb_threads];

  // ---------------------------------------------------------------
  // First check if the logs are too empty
  for (i = 0; i < TM_nb_threads; ++i) {
    log = NH_global_logs[i];
    log_start = log->start;
    log_end = log->end;
    dist = distance_ptr(log_start, log_end);
    // sum_dist += dist;
    if (dist < TOO_EMPTY) {
      too_empty = true;
    }
    if (dist > TOO_FULL) {
      someone_passed = true;
      too_full = true;
    }
    if (dist > APPLY_BACKWARD_VAL) {
      j = ends[i];
      someone_passed = true;
    }
  }
  // Only apply log if someone passed the threshold mark
  if ((!too_full && too_empty) || !someone_passed) {
    *NH_checkpointer_state = 0; // doing checkpoint
    asm volatile ("sfence" ::: "memory");//__sync_synchronize();
    return 1; // try again later
  }
  // ---------------------------------------------------------------

  // first find target_ts, then the remaining TSs
  // TODO: keep the minimum anchor
  for (i = 0; i < TM_nb_threads; ++i) {
    log = NH_global_logs[i];

    starts[i] = log->start;
    ends[i] = log->end;

    // TODO: check the size of the log
    size_t log_size = ptr_mod_log(ends[i], -starts[i]);
    ts_s ts = 0;
    // find target ts in this log
    if (log_size <= APPLY_BACKWARD_VAL) {
      j = ends[i];
    } else {
      j = ptr_mod_log(starts[i], APPLY_BACKWARD_VAL);
    }

    // TODO: repeated code
    for (; j != starts[i]; j = ptr_mod_log(j, -1)) {
      NVLogEntry_s entry = log->ptr[j];
      ts = entry_is_ts(entry);
      if (ts && ((ts < target_ts) || !target_ts)) { // find minimum
        target_ts = ts;
        break;
      }
    }

    // if ts == 0 it means that we need to analyze more log
    if (ts == 0) {
      j = ends[i];
      for (; j != starts[i]; j = ptr_mod_log(j, -1)) {
        NVLogEntry_s entry = log->ptr[j];
        ts = entry_is_ts(entry);
        if (ts && ((ts < target_ts) || !target_ts)) { // find minimum
          target_ts = ts;
          break;
        }
      }
    }

    targets[i] = j;
    size_hashmap += distance_ptr(starts[i], targets[i]);

    // targets[] is between starts[] and ends[]
    assert((starts[i] <= ends[i] && targets[i] >= starts[i] &&
      targets[i] <= ends[i]) || (starts[i] > ends[i] &&
        ((targets[i] >= 0 && targets[i] < starts[i]) ||
        (targets[i] < LOG_local_state.size_of_log &&
          targets[i] > ends[i])
        )
      )
    );
    // printf("s:%i t:%i e:%i \n", starts[i], targets[i], ends[i]);
  }

  // other TSs smaller than target
  // TODO: sweep from the threshold backward only
  for (i = 0; i < TM_nb_threads; ++i) {
    log = NH_global_logs[i];
    ts_s ts = 0;
    // find the maximum TS of other TXs smaller than target_ts

    // sizes[] has the target idx
    pos[i] = targets[i]; // init with some default value
    // if j == start (none TX has smaller TS) then pos_to_start must not be changed
    pos_to_start[i] = starts[i];

    for (j = pos[i]; j != starts[i]; j = ptr_mod_log(j, -1)) {
      NVLogEntry_s entry = log->ptr[j];
      ts = entry_is_ts(entry);
      if (ts && ts <= target_ts) {
        // previous idx contains the write
        pos[i] = j; // ptr_mod_log(j, -1);
        if(j == ends[i]) {
          pos_to_start[i] = j; // THERE IS SOME BUG HERE!
        } else {
          pos_to_start[i] = ptr_mod_log(j, 1); // THERE IS SOME BUG HERE!
        }
        break;
      }
    }
  }

  if (!target_ts) {
    *NH_checkpointer_state = 0; // doing checkpoint
    asm volatile ("sfence" ::: "memory");//__sync_synchronize();
    return 1; // there isn't enough transactions
  }

  NH_nb_checkpoints++;

  // find the write-set to apply to the checkpoint (Cache_lines!)
  int next_log;
  unsigned long long proc_writes = 0;

  // ts_s time_ts1, time_ts2, time_ts3, time_ts4 = 0;
  // time_ts1 = rdtscp();
  writes_map.reserve(size_hashmap);
  do {
    // time_ts3 = rdtscp();
    next_log = max_next_log(pos, starts, ends);
    i = next_log;
    if (next_log == -1) {
      break;
    }

    log = NH_global_logs[next_log];

    target_ts = max_tx_after(log, starts[next_log], target_ts, &(pos[next_log])); // updates the ptr
    // time_ts4 += rdtscp() - time_ts3;
    // advances to the next write after the TS

    // pos[next_log] must be between start and end
    if (pos[next_log] != starts[next_log]) {
      pos[next_log] = ptr_mod_log(pos[next_log], -1);
    } else {
      // ended
      break;
    }

    // within the start/end boundary
    /*assert((starts[i] <= ends[i] && pos[i] >= starts[i] &&
      pos[i] <= ends[i]) || (starts[i] > ends[i] &&
        ((pos[i] >= 0 && pos[i] < starts[i]) ||
        (pos[i] < LOG_local_state.size_of_log && pos[i] > ends[i]))
      )
    );*/

    NVLogEntry_s entry = log->ptr[pos[next_log]];
    ts_s ts = entry_is_ts(entry);
    while (!ts && pos[next_log] != starts[next_log]) {

      // uses only the bits needed to identify the cache line
      intptr_t cl_addr = (((intptr_t)entry.addr >> 6) << 6);
      auto it = writes_map.find((GRANULE_TYPE*)cl_addr);
      int val_idx = ((intptr_t)entry.addr & 0x38) >> 3; // use bits 4,5,6
      char bit_map = 1 << val_idx;
      if (it == writes_map.end()) {
        // not found the write --> insert it
        CL_BLOCK block;
        block.bit_map = bit_map;
        /*block.block[val_idx] = entry.value;*/
        auto to_insert = make_pair((GRANULE_TYPE*)cl_addr, block);
        writes_map.insert(to_insert);
        writes_list.push_back((GRANULE_TYPE*)cl_addr);
        MN_write(entry.addr, &(entry.value), sizeof(GRANULE_TYPE), 1);
      } else {
        if ( !(it->second.bit_map & bit_map) ) {
          // Need to write this word
          MN_write(entry.addr, &(entry.value), sizeof(GRANULE_TYPE), 1);
          it->second.bit_map |= bit_map;
        }
      }

      pos[next_log] = ptr_mod_log(pos[next_log], -1);
      entry = log->ptr[pos[next_log]];
      ts = entry_is_ts(entry);
    }
    // NH_nb_applied_txs++;
  } while (next_log != -1);

  // flushes the changes
  auto cl_iterator = writes_list.begin();
  // auto cl_it_end = writes_list.end();
  for (; cl_iterator != writes_list.end(); ++cl_iterator) {
    // TODO: must write the cache line, now is just spinning
    GRANULE_TYPE *addr = *cl_iterator;
    //MN_flush(addr, CACHE_LINE_SIZE, 0);
    asm(""); // kaan: Instead of CLFLUSHing here, emulate CLWB. Empty assembly keeps compiler from optimizing the loop out.
  }

  // advance the pointers
  //    int freed_space = 0;
  for (i = 0; i < TM_nb_threads; ++i) {
    log = NH_global_logs[i];
    //        freed_space += distance_ptr(log->start, pos_to_start[i]);
    assert(starts[i] == log->start); // only this thread changes this
    // either in the boundary or just cleared the log
    assert((distance_ptr(starts[i], ends[i]) >=
    distance_ptr(pos_to_start[i], ends[i]))
    || (ptr_mod_log(ends[i], 1) == pos_to_start[i] ));
    //            printf("s:%i t:%i e:%i d1:%i d2:%i \n", starts[i], pos_to_start[i], ends[i],
    //                   distance_ptr(starts[i], ends[i]), distance_ptr(pos_to_start[i], ends[i]));

    MN_write(&(log->start), &(pos_to_start[i]), sizeof(int), 0);
    // log->start = pos_to_start[i];
    // TODO: snapshot the old ptrs before moving them
  }
  *NH_checkpointer_state = 0;
  asm volatile("sfence" ::: "memory"); //__sync_synchronize();
  SPIN_PER_WRITE(1); // kaan: Emulating the CLWB+SFENCE delay here. Keeping the actual sfence since it might be necessary for synchronization.
  return 0;
}

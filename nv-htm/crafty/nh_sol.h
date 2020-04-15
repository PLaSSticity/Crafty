#ifndef NH_SOL_H
#define NH_SOL_H

#include <setjmp.h>
#include <malloc.h>
#include <stdbool.h>
#include <assert.h>
#include "utils.h"
#include "extra_globals.h"

#ifdef __cplusplus
extern "C"
{
#endif



#ifndef CRAFTY_VALIDATE
#ifndef CRAFTY_REDO
#error Either CRAFTY_VALIDATE or CRAFTY_REDO must be enabled
#endif
#endif

#define CRAFTY_VALIDATION_FAIL 255u
#define CRAFTY_REDO_REPLAY_FAIL 254u
#define CRAFTY_NAMED_ABORT_RESERVED 128u

#define CRAFTY_JUMP_VALIDATION_SUCCESS 1
#define CRAFTY_JUMP_VALIDATION_FAIL 2

#define NS_IN_SECOND 1000000000

// The empty assembly prevents compiler optimizations from optimizing out code where CLWB appears.
// See https://godbolt.org/#g:!((g:!((g:!((h:codeEditor,i:(j:1,lang:c%2B%2B,source:'%23define+CLWB(e)+asm(%22%22)%0A%0Aint+tx_start%3B%0Aint+curr_pos%3B%0A%0Aint+incr(int+e)+%7B+return+e+%2B+1%3B+%7D%0A%0Avoid+crafty_after_htm_commit()+%7B%0A++++int+entry%3B%0A++++for+(entry+%3D+tx_start%3B+entry+!!%3D+curr_pos%3B+entry+%3D+incr(entry))+%7B%0A%09%09CLWB(undo_entry)%3B%0A%09%7D%0A%7D'),l:'5',n:'0',o:'C%2B%2B+source+%231',t:'0')),k:50,l:'4',n:'0',o:'',s:0,t:'0'),(g:!((h:compiler,i:(compiler:g82,filters:(b:'0',binary:'1',commentOnly:'0',demangle:'0',directives:'0',execute:'1',intel:'0',libraryCode:'1',trim:'1'),lang:c%2B%2B,libs:!(),options:'-O3',source:1),l:'5',n:'0',o:'x86-64+gcc+8.2+(Editor+%231,+Compiler+%231)+C%2B%2B',t:'0')),k:50,l:'4',n:'0',o:'',s:0,t:'0')),l:'2',n:'0',o:'',t:'0')),version:4
#define CLWB(addr) asm("")/* clflush(addr) */

// Do expensive debugging checks
#define DEBUG_EXPENSIVE 1
void crafty_safe_free(void * ptr, char const * reason);
#ifdef DEBUG_EXPENSIVE
#define _free(p, i) crafty_safe_free(p, i)
#else
#define _free(p, i) free(p)
#endif

#ifndef REPLAY_BUDGET
#define REPLAY_BUDGET 10
#endif

#ifndef CRAFTY_SGL_WRITE_START_SIZE
#define CRAFTY_SGL_WRITE_START_SIZE 64
#endif

#ifndef RECOVERY_MAX_LAG
#define RECOVERY_MAX_LAG 2294406258 /* Approximated rdtscp timing for 1 second on funes, may want to automate setting this */
#endif

enum {
    CRAFTY_VALIDATING = 0,
    CRAFTY_LOGGING = 1,
};


/* Denotes that the logs since the last commit entry belong in the same transaction. */
#define COMMIT_ENTRY_ADDR ((GRANULE_TYPE*)(1u<<2u))
/* Denotes the start of an SGL transaction in log. Any partial logs after this entry may have writes persisted. */
#define PRETRANSACTION_ENTRY_ADDR ((GRANULE_TYPE*)((1u<<2u)|(1u<<3u)))

/* If validation fails (i.e. value change), attempt logging and validating again this many additional times */
#define MAX_VALIDATION_REATTEMPTS 5


#ifndef thrprintf
#define thrprintf(...) /* nothing */
#endif



#ifndef CRAFTY_REDO_LOG_SIZE /* The number of writes that can be contained in a single transaction */
#define CRAFTY_REDO_LOG_SIZE 128
#endif

#undef  NH_alloc
#undef  NH_aligned_alloc
#undef  NH_free
#ifndef CRAFTY_ALLOC_LOG_SIZE /* The maximum number of malloc's/free's allowed within a transaction. */
#define CRAFTY_ALLOC_LOG_SIZE 256
#endif
#define NH_alloc(size) crafty_malloc(0, size)
#define NH_aligned_alloc(alignment, size) crafty_malloc(alignment, size)
#define NH_free(pool)  crafty_free(pool)

#undef NH_before_write
#define NH_before_write(addr, val) ({ \
	crafty_log_write((GRANULE_TYPE*)addr, val); \
})

#undef BEFORE_TRANSACTION_i
#define BEFORE_TRANSACTION_i(tid, budget) ({ \
	crafty_before_any_transaction(); \
})


#ifndef NDEBUG
#undef BEFORE_HTM_BEGIN
#ifdef CRAFTY_VALIDATE
#define BEFORE_HTM_BEGIN(tid, budget) ({ \
    if (crafty_is_logging == CRAFTY_LOGGING) { \
        txassert(crafty_alloc_index == 0); \
        txassert(crafty_alloc_last == 0); \
    } else { /* crafty_is_logging == CRAFTY_VALIDATING */ \
        txassert(crafty_alloc_index == 0); \
        txassert(crafty_alloc_last >= 0); \
    } \
})
#else
#define BEFORE_HTM_BEGIN(tid, budget) ({ \
    txassert(crafty_alloc_index == 0); \
    txassert(crafty_alloc_last == 0); \
})
#endif
#endif


#undef AFTER_ABORT
#define AFTER_ABORT(tid, budget, status) ({ \
    crafty_after_abort(status); \
})


#undef BEFORE_CHECK_BUDGET
#ifdef CRAFTY_VALIDATE
#define BEFORE_CHECK_BUDGET(budget) ({ \
	if (setjmp(setjmp_buf) == CRAFTY_JUMP_VALIDATION_SUCCESS) { \
		/* got here via longjmp after successful commit */ \
		txassert(!crafty_is_logging); \
		/* crafty_log_tx_start = crafty_log_curr_pos; */ \
		/* mdb: This code would, if enabled, cause committed transactions to count against the budget:
		budget = HTM_update_budget(budget, HTM_CODE_SUCCESS); */ \
		/* mdb: Added this to give the next transaction a fresh budget: */ \
		budget = HTM_SGL_INIT_BUDGET; \
		continue; \
	} \
	txassert(!crafty_is_logging || crafty_log_tx_start == crafty_log_curr_pos); \
})
#else
#define BEFORE_CHECK_BUDGET(budget) ({ \
    /* The only reason we'd jump back here is if applying redo log fails */ \
	setjmp(setjmp_buf); \
	txassert(crafty_log_tx_start == crafty_log_curr_pos); \
})
#endif


#undef AFTER_HTM_BEGIN
#define AFTER_HTM_BEGIN(tid, budget) ({ \
	crafty_after_htm_begin(); \
})

#undef BEFORE_HTM_COMMIT
#define BEFORE_HTM_COMMIT(tid, budget) ({ \
	crafty_before_htm_commit(tid); \
})


static inline Crafty_log_entry_s* incr(Crafty_log_entry_s* entry) {
    if (entry == crafty_log_upper_bound - 1) {
        return &crafty_log->entries[0];
    }
    return entry + 1;
}

static inline Crafty_log_entry_s* decr(Crafty_log_entry_s* entry) {
    if (entry == &crafty_log->entries[0]) {
        return crafty_log_upper_bound - 1;
    }
    return entry - 1;
}

#define PUREFN  __attribute__ ((pure))
#define BIT0_MASK ((uint64_t)(1u))
#define BIT1_MASK ((uint64_t)(1u << 1u))


#ifdef ROLLOVER_BITS
PUREFN static inline GRANULE_TYPE* rollover_addr_set(GRANULE_TYPE const * addr, GRANULE_TYPE val) {
    txassert(((uintptr_t) addr & BIT0_MASK) == 0 && ((uintptr_t) addr & BIT1_MASK) == 0); // Check low bits are unset
    // Use 0'th bit for rollover, and 1'st bit to save 1'st bit of value
    return (GRANULE_TYPE *) ((uintptr_t)addr | crafty_rollover_bit | (val & BIT1_MASK));
}

PUREFN static inline GRANULE_TYPE rollover_val_set(GRANULE_TYPE val) {
    // 1'st bit of value is the rollover bit
    return (val & ~BIT1_MASK) | (crafty_rollover_bit << 1);
}

PUREFN static inline GRANULE_TYPE* rollover_addr_get(GRANULE_TYPE const * addr) {
    return (GRANULE_TYPE*)((uintptr_t)addr & (~(BIT0_MASK | BIT1_MASK)));
}

PUREFN static inline GRANULE_TYPE rollover_val_get(GRANULE_TYPE const * addr, GRANULE_TYPE val) {
    return (val & ~BIT1_MASK) | ((uintptr_t)addr & BIT1_MASK);
}
#else
#define rollover_addr_set(addr, val) (addr)
#define rollover_val_set(val) (val)
#define rollover_addr_get(addr) (addr)
#define rollover_val_get(addr, val) (val)
#endif


static inline Crafty_log_entry_s* __crafty_log_entry(
        Crafty_log_entry_s** log_curr_pos, Crafty_log_entry_s* log_lower_bound,
        Crafty_log_entry_s* log_upper_bound,
#ifdef ROLLOVER_BITS
        uint_fast8_t* rollover_bit,
#endif
        GRANULE_TYPE* addr, GRANULE_TYPE oldVal
) {
    Crafty_log_entry_s* entry = *log_curr_pos;
    txassert(log_lower_bound <= entry && entry < log_upper_bound);
    entry->addr = rollover_addr_set(addr, oldVal);
    entry->oldValue = rollover_val_set(oldVal);
    txassert(rollover_addr_get(entry->addr) == addr);
    txassert(rollover_val_get(entry->addr, entry->oldValue) == oldVal);
    *log_curr_pos = incr(entry);
#ifdef ROLLOVER_BITS
    if (*log_curr_pos < entry) {
        // Next entry will roll over
        *rollover_bit = !(*rollover_bit);
    }
#endif
    return entry;
}


static inline Crafty_log_entry_s* crafty_log_entry(GRANULE_TYPE* addr, GRANULE_TYPE oldVal) {
    return __crafty_log_entry(&crafty_log_curr_pos, crafty_log->entries, crafty_log_upper_bound,
#ifdef ROLLOVER_BITS
    &crafty_rollover_bit,
#endif
        addr, oldVal);
}


static inline void crafty_before_sgl_begin() {
    crafty_in_SGL = 1;
    thrprintf("SGL starting\n");
#ifdef CRAFTY_VALIDATE
#ifdef CRAFTY_STATS
    if (crafty_is_logging) crafty_SGL_in_logging++;
    else                   crafty_SGL_in_validating++;
#endif
	crafty_is_logging = CRAFTY_LOGGING;
#endif
	crafty_log_entry(PRETRANSACTION_ENTRY_ADDR, rdtscp());
#ifdef ROLLOVER_BITS
	if (crafty_log_tx_start > crafty_log_curr_pos) crafty_rollover_bit = !crafty_rollover_bit;
#endif
    crafty_log_tx_start = crafty_log_curr_pos;
}


#undef BEFORE_SGL_BEGIN
#define BEFORE_SGL_BEGIN(tid) ({ \
crafty_before_sgl_begin(); \
})

#ifdef CRAFTY_REDO
#undef AFTER_SGL_BEGIN_i
#define AFTER_SGL_BEGIN_i(tid) \
    last_copy_time = rdtscp(); \
    __sync_synchronize(); /* Make sure this occurs before any access */
#endif // CRAFTY_REDO

#undef BEFORE_SGL_COMMIT
#define BEFORE_SGL_COMMIT(tid) ({ \
	crafty_before_sgl_commit(); \
})

#undef AFTER_HTM_COMMIT
#define AFTER_HTM_COMMIT(tid, budget) ({ \
	crafty_after_htm_commit(tid); \
})

#undef AFTER_TRANSACTION
#define AFTER_TRANSACTION(tid, budget) ({ \
	crafty_after_any_transaction(tid); \
})

/*
#undef AFTER_BEGIN
#define AFTER_BEGIN(tid, budget, status) ({ \
    crafty_after_begin_any_transaction(); \
})
 */


// Because we enter HTM inside the SGL, just checking HTM_test() isn't enough to determine whether we're in SGL or not.
#define CRAFTY_IN_SGL (crafty_in_SGL)
#undef IN_TRANSACTION
#define IN_TRANSACTION(tid, budget, status) \
    !CRAFTY_IN_SGL


/* Call this function wherever a persist operation should be. */
static inline void crafty_persist_start() {
#if NVM_LATENCY_NS
#ifdef NVM_LATENCY_RDTSCP
    clwb_end_time = rdtscp() + NVM_WAIT_CYCLES;
#else /* ! NVM_LATENCY_RDTSCP */
    timespec_get(&clwb_end_time, TIME_UTC);
    clwb_end_time.tv_nsec += NVM_LATENCY_NS;
    // timespec doesn't keep more than NS_IN_SECOND many nanoseconds in the nanosecond field
    clwb_end_time.tv_sec += clwb_end_time.tv_nsec / NS_IN_SECOND;
    clwb_end_time.tv_nsec = clwb_end_time.tv_nsec % NS_IN_SECOND;
#endif /* NVM_LATENCY_RDTSCP */
#else /* ! NVM_LATENCY_NS */
    // Do nothing if the latency is set to 0
#endif /* NVM_LATENCY_NS */
}


static inline uint64_t timespec_less(struct timespec const * left, struct timespec const * right) {
    return left->tv_sec < right->tv_sec || (left->tv_sec == right->tv_sec && left->tv_nsec < right->tv_nsec);
}


/* Call this function after any hardware transaction where a persist_start operation may have occured before it. */
static inline void crafty_persist_finish() {
#if NVM_LATENCY_NS
#ifdef NVM_LATENCY_RDTSCP
    unsigned long long cur_time = rdtscp();
    // Wait here for the latency of the CLWBs to end
    while (cur_time < clwb_end_time) {
        PAUSE();
        cur_time = rdtscp();
    }
#else /* ! NVM_LATENCY_RDTSCP */
    struct timespec cur_time;
    timespec_get(&cur_time, TIME_UTC);
    // Wait here for the latency of the CLWBs to end
    while (timespec_less(&cur_time, &clwb_end_time)) {
        PAUSE();
        timespec_get(&cur_time, TIME_UTC);
    }
#endif /* NVM_LATENCY_RDTSCP */
#else /* ! NVM_LATENCY_NS */
    // Do nothing if the latency is set to 0
#endif /* NVM_LATENCY_NS */
}


/* CLWB the log between tx_start and curr_pos. */
static inline void crafty_clwb_log_section() {
    // Execute CLWB a on each *address* written by the committing transaction.
    // However, we don't have to account for the latency because we don't need an SFENCE afterward!
    Crafty_log_entry_s* entry = crafty_log_tx_start;
    for (; entry != crafty_log_curr_pos; entry = incr(entry)) {
        txassert_named(crafty_log->entries <= entry && entry < crafty_log_upper_bound, 17);
        GRANULE_TYPE* addr = rollover_addr_get(entry->addr);
        CLWB(addr);
    }
    // No SFENCE needed here
#ifdef CRAFTY_STATS
    crafty_log_sections++;
#endif
}


/* Free logged malloc's that didn't get used during the last transaction to avoid leaking.
 * start is the number of malloc that WERE used in the last transaction.
 */
static inline void crafty_free_unused_mallocs(int start) {
    // If any logged malloc's didn't get used during the last transaction, free them to avoid leaking
    int i;
    for (i = start; i < crafty_alloc_last; i++) {
        txassert_named(i < CRAFTY_ALLOC_LOG_SIZE, 18);
        txassert(crafty_alloc_log[i] /* freeing a null pointer is correct, but the benchmarks never do it */);
        _free(crafty_alloc_log[i], "unused malloc");
#ifndef NDEBUG
        crafty_alloc_log[i] = NULL;
#endif
    }
    crafty_alloc_index = 0;
    crafty_alloc_last = 0;
}


#ifdef CRAFTY_REDO
static inline void crafty_perform_delayed_free() {
    int i;
    for (i = 0; i < crafty_free_last; i++) {
        txassert(crafty_free_log[i]);
        _free(crafty_free_log[i], "delayed free");
#ifndef NDEBUG
        crafty_free_log[i] = NULL;
#endif
    }
    crafty_free_last = 0;
}
static inline void crafty_reset_free_log() {
#ifndef NDEBUG
    int i;
    for (i = 0; i < crafty_free_last; i++) {
        txassert(crafty_free_log[i]);
        crafty_free_log[i] = NULL;
    }
#endif
    crafty_free_last = 0;
}

#define crafty_rollback_writes crafty_rollback_writes_with_redo
#else
#define crafty_rollback_writes _crafty_rollback_writes
#endif /* CRAFTY_REDO */

static inline void _crafty_rollback_writes() {
    Crafty_log_entry_s* entry = decr(crafty_log_curr_pos);
    Crafty_log_entry_s* term = decr(crafty_log_tx_start);
    for (; entry != term; entry = decr(entry)) {
        // Rollback the write by applying the undo log
        txassert_named(crafty_log->entries <= entry && entry < crafty_log_upper_bound, 17);
        *rollover_addr_get(entry->addr) = rollover_val_get(entry->addr, entry->oldValue);
    }
}

static inline void crafty_rollback_writes_with_redo() {
    Crafty_log_entry_s* entry = decr(crafty_log_curr_pos);
    Crafty_log_entry_s* term = decr(crafty_log_tx_start);
    redo_log_pos = redo_log;
    for (; entry != term; entry = decr(entry), redo_log_pos++) {
        // Save the new value, so that we can replay them later
        txassert_named(redo_log <= redo_log_pos && redo_log_pos < redo_log + CRAFTY_REDO_LOG_SIZE, 16);
        redo_log_pos->addr = rollover_addr_get(entry->addr);
        redo_log_pos->oldValue = *redo_log_pos->addr;
        // Rollback the write by applying the undo log
        txassert_named(crafty_log->entries <= entry && entry < crafty_log_upper_bound, 17);
        *redo_log_pos->addr = rollover_val_get(entry->addr, entry->oldValue);
    }
}


// Applies the redo log. This function is unsafe if any other thread is running, use crafty_apply_redo_log instead.
static inline void crafty_unsafe_apply_redo_log() {
    // Replay the log
    while (redo_log_pos > redo_log) {
#ifdef CRAFTY_STATS
        crafty_write_count++;
        crafty_currenttx_write_count++;
#endif
        Crafty_log_entry_s* pos = --redo_log_pos;
        txassert_named(redo_log <= pos && pos < redo_log + CRAFTY_REDO_LOG_SIZE, 15);
        *pos->addr = pos->oldValue;
    }
}


#ifdef CRAFTY_REDO
static inline bool crafty_apply_redo_log(int tid) {
    int budget = REPLAY_BUDGET;
    while (1) {
        if (HTM_SGL_var) return false; // SGL running, we can't enter redo. We will fail after SGL finishes, so just give up early.
        unsigned status = _xbegin();
        if (_XABORT_CODE(status) == CRAFTY_REDO_REPLAY_FAIL) {
#ifdef CRAFTY_STATS
            crafty_replayfail++;
#endif
            budget = 0;
        }
        if (status != _XBEGIN_STARTED && _XABORT_CODE(status) == CRAFTY_REDO_REPLAY_FAIL) {
            thrprintf("Replay failure: incorrect timestamp\n");
            crafty_redo_abort_explicit++;
            return false;
        }
        if (status != _XBEGIN_STARTED) {
            thrprintf("Replay failure: %d, budget was %i now %i\n", status, budget, budget - 1);
#ifdef CRAFTY_STATS
            crafty_aborts_in_redo++;
#endif
            budget--;
            if (status & _XABORT_CAPACITY) crafty_redo_abort_capacity++;
            if (status & _XABORT_CONFLICT) crafty_redo_abort_conflict++;
            if (status & _XABORT_EXPLICIT) crafty_redo_abort_explicit++;
            if (status == 0) crafty_redo_abort_zero++;
        }
        else break;
        if (budget <= 0) {
            thrprintf("Budget 0, replay failed\n");
            return false;
        }
    }

    // If another thread copied before us, we can't safely replay the log
    if (last_copy_time >= last_log_time) _xabort(CRAFTY_REDO_REPLAY_FAIL);
    if (HTM_SGL_var) _xabort(CRAFTY_REDO_REPLAY_FAIL);

    crafty_unsafe_apply_redo_log();
    __sync_synchronize();

    // Replay done. Mark the replay time.
    last_copy_time = rdtscp();
    _xend();
    crafty_persist_finish();
    crafty_replay_success++;
    return true;
}
#endif


static inline void crafty_SGL_enter_HTM() {
    // Attempt to execute a chunk of writes within the SGL inside a hardware transaction.
    // Do exponential backoff if it fails.
    crafty_SGL_writes_max = CRAFTY_SGL_WRITE_START_SIZE;
    while (crafty_SGL_writes_max > 1) { // With only 1 write, we don't need to enter htm
        crafty_SGL_writes_seen = 0;
        unsigned int crafty_SGL_status = _xbegin();
        crafty_log_curr_pos = crafty_log_tx_start;
        if (crafty_SGL_status == _XBEGIN_STARTED) break;
        thrprintf("SGL fallback abort, status: %i failed with %i writes, now trying %i\n", crafty_SGL_status, crafty_SGL_writes_max, crafty_SGL_writes_max / 2);
        if (crafty_SGL_status & _XABORT_CAPACITY) crafty_sgl_abort_capacity++;
        if (crafty_SGL_status & _XABORT_CONFLICT) crafty_sgl_abort_conflict++;
        if (crafty_SGL_status & _XABORT_EXPLICIT) crafty_sgl_abort_explicit++;
        if (crafty_SGL_status == 0) crafty_sgl_abort_zero++;
        crafty_SGL_writes_max = crafty_SGL_writes_max / 2;
#ifdef CRAFTY_STATS
        crafty_aborts_in_SGL++;
#endif
        txassert(crafty_SGL_writes_max > 0);
    }
    txassert(crafty_SGL_writes_max == 1 || HTM_test());
}


static inline void crafty_log_write_SGL(GRANULE_TYPE* addr, GRANULE_TYPE oldVal) {
    // SGL fallback.
    if (!HTM_test()) crafty_SGL_enter_HTM();
    crafty_log_entry(addr, oldVal);

    if (HTM_test()) {
        txassert(HTM_test());
        txassert(crafty_SGL_writes_max > 1);
        crafty_SGL_writes_seen++;
        if (crafty_SGL_writes_seen >= crafty_SGL_writes_max) {
            // Finish hardware transaction. Rollback writes, build redo log.
            crafty_rollback_writes_with_redo();
            _xend();
            // We commited, logging done. Just apply the writes now.
            SPIN_PER_WRITE(1);
            crafty_unsafe_apply_redo_log();
            crafty_clwb_log_section();
            crafty_log_tx_start = crafty_log_curr_pos;
#ifdef CRAFTY_STATS
            crafty_multiwrite_SGL++;
#endif
            thrprintf("SGL %i write commit\n", crafty_SGL_writes_seen);
        }
    } else {
        txassert(crafty_SGL_writes_max == 1);
        // We need a persist barrier between the log and the write.
        crafty_clwb_log_section();
        crafty_log_tx_start = crafty_log_curr_pos;
        SPIN_PER_WRITE(1);
#ifdef CRAFTY_STATS
        crafty_singlewrite_SGL++;
        crafty_write_count++;
        crafty_currenttx_write_count++;
#endif
        thrprintf("Single write SGL commit\n");
    }
}


static inline void crafty_log_write_HTM(GRANULE_TYPE* addr, GRANULE_TYPE oldVal) {
    txassert(HTM_test());
#ifdef CRAFTY_VALIDATE
    if (crafty_is_logging) {
#endif
        crafty_log_entry(addr, oldVal);
#ifdef CRAFTY_VALIDATE
    } else {
#ifdef CRAFTY_STATS
        crafty_write_count++;
        crafty_currenttx_write_count++;
#endif
        Crafty_log_entry_s* entry = crafty_log_curr_pos;
        txassert(crafty_log->entries <= entry && entry < crafty_log_upper_bound);
        if (addr != rollover_addr_get(entry->addr) || oldVal != rollover_val_get(entry->addr, entry->oldValue)) {
            // The log entry is invalid, or we are seeing more writes than we logged and we've just gone past the end.
            // In either case, we need to go back to logging.
            _xabort(CRAFTY_VALIDATION_FAIL);
        } else {
            // The entry is valid, continue verifying.
            crafty_log_curr_pos = incr(crafty_log_curr_pos);
        }
    }
#endif
}


static inline void crafty_log_write(GRANULE_TYPE* addr, GRANULE_TYPE newVal) {
    txassert(addr);
    GRANULE_TYPE oldVal = *addr;
    if (CRAFTY_IN_SGL)   crafty_log_write_SGL(addr, oldVal);
    else if (HTM_test()) crafty_log_write_HTM(addr, oldVal);
    else return; // Ignore any writes outside persistent regions
}


// Gets called once, before we enter the transaction. Won't get called when switching to validation or SGL.
static inline void crafty_before_any_transaction() {
    thrprintf("Transaction starting.\n");
#ifdef CRAFTY_VALIDATE
    crafty_is_logging = CRAFTY_LOGGING;
    crafty_validation_reattempts_left = MAX_VALIDATION_REATTEMPTS;
#endif
#ifdef CRAFTY_REDO
    crafty_perform_delayed_free();
#endif
}


static inline void crafty_after_htm_begin() {
#ifdef ROLLOVER_BITS
    if (crafty_log_tx_start > crafty_log_curr_pos) crafty_rollover_bit = !crafty_rollover_bit;
#endif
    crafty_log_curr_pos = crafty_log_tx_start;
}


static inline void crafty_before_htm_commit(int tid) {
    // Read only transactions don't need redo or validation
    if (crafty_log_tx_start == crafty_log_curr_pos) return;
#ifdef CRAFTY_VALIDATE
    if (crafty_is_logging) {
#endif
        // We're done logging, but we don't want the program writes to appear yet. Roll them back.
        crafty_rollback_writes();
        // Write a commit entry into the log to signal the end of the transaction.
        Crafty_log_entry_s* commit_entry = crafty_log_entry(COMMIT_ENTRY_ADDR, rdtscp());
        // Note the logging time, used later when replaying do rolled back writes.
        txassert_named(commit_entry == decr(crafty_log_curr_pos), 2);
#ifdef CRAFTY_REDO
        last_log_time = rdtscp();
#endif // CRAFTY_REDO
#ifdef CRAFTY_VALIDATE
    } else { // crafty is validating
        // We're at the end of the transaction, and all writes we have seen so far have been valid.

        // Did we validate all entries we logged?
        if (rollover_addr_get(crafty_log_curr_pos->addr) != COMMIT_ENTRY_ADDR) {
            // No, we saw fewer entries than we expected. We can't just continue because there are some writes
            // in the log that haven't been validated. We need to go back to logging.
            _xabort(CRAFTY_VALIDATION_FAIL);
        } else {
            // We saw the correct number of entries and everything is valid, we can finally finish.
            // Update the COMMIT log timestamp and move on.
            __sync_synchronize(); // Ensure that this timestamp is after any writes in the transaction
            crafty_log_entry(COMMIT_ENTRY_ADDR, rdtscp());
#ifdef CRAFTY_REDO
            // Invalidate any redo transactions, because they might conflict with the writes we just did
            last_copy_time = rdtscp();
#endif /* CRAFTY_REDO */
        }
    }
#endif
}

static inline void crafty_before_sgl_commit() {
    if (!HTM_test()) {
        // We need a commit entry even if the SGL was read-only because of the pre-transaction
        crafty_log_entry(COMMIT_ENTRY_ADDR, rdtscp());
        crafty_clwb_log_section();
        crafty_log_tx_start = crafty_log_curr_pos;
#ifdef CRAFTY_REDO
        crafty_reset_free_log();
#endif
        crafty_alloc_index = 0;
        crafty_alloc_last = 0;
        crafty_in_SGL = 0;
        return; // SGL transaction with no writes, or the last chunk finished all writes in SGL.
    }

    // Finish hardware transaction. Rollback writes, build redo log.
    crafty_rollback_writes_with_redo();
    crafty_log_entry(COMMIT_ENTRY_ADDR, rdtscp());
    _xend();
    thrprintf("SGL commit.\n");
#ifdef CRAFTY_STATS
    if (crafty_SGL_writes_seen > 1)
        crafty_multiwrite_SGL++;
    else
        crafty_singlewrite_SGL++;
#endif
    // We commited, logging done. For second phase, we just replay the writes without repeating the
    // second transaction. No validation needed since no other thread can conflict.
    SPIN_PER_WRITE(1);
    crafty_unsafe_apply_redo_log();
    crafty_clwb_log_section();
    crafty_alloc_index = 0;
    crafty_alloc_last = 0;
    crafty_log_tx_start = crafty_log_curr_pos;
#ifdef CRAFTY_REDO
    crafty_reset_free_log();
#endif
    crafty_in_SGL = 0;
}



static inline void crafty_after_htm_commit_logging(int tid) {
    thrprintf("HTM commit logging.\n");
    // End the log section, we're done logging
    crafty_clwb_log_section();
    // We may need to pause after the redo or validate phase is done for the logs to persist
    crafty_persist_start();

#ifdef CRAFTY_REDO
    if (crafty_apply_redo_log(tid)) {
        // Redo successful! We're done with this region. Just reset allocation logs.
        crafty_alloc_index = 0;
        crafty_alloc_last = 0;
        // We can perform the delayed free's now since the region is done.
        crafty_perform_delayed_free();
        crafty_log_tx_start = crafty_log_curr_pos;
        thrprintf("Replay successful!\n");
    } else {
        // Redo failed (i.e. some other thread replayed their transaction before us).
#ifdef CRAFTY_VALIDATE
        // Try validating in case the log is still valid.
        crafty_is_logging = CRAFTY_VALIDATING;
        // We will be using the allocation we logged.
        crafty_alloc_index = 0;
#else /* ! CRAFTY_VALIDATE */
        // Validation is not enabled, go back to logging. Use up a budget to avoid infinitely retrying logging.
        HTM_SGL_vars.budget--;
        // Don't reuse the malloc log, the new logging transaction can allocate them again.
        crafty_free_unused_mallocs(0 /* free all */);
        // Reset the log so we log everything again
        crafty_log_tx_start = crafty_log_curr_pos;
#endif /* CRAFTY_VALIDATE */
        // We don't need the free log, we'll perform them during validation/next logging.
        crafty_reset_free_log();
        // Execute transaction again, validating/retrying logging.
        thrprintf("Jumping back to validate, tx_start: %p, curr_pos: %p\n", crafty_log_tx_start, crafty_log_curr_pos);
        HTM_SGL_vars.nesting++; // We decrement nesting twice when we jump back, we need to restore it here
        longjmp(setjmp_buf, CRAFTY_JUMP_VALIDATION_FAIL);
    }
#else /* ! CRAFTY_REDO */
    // Logging done, switch to validation.
    crafty_is_logging = CRAFTY_VALIDATING;
    // We will be using the allocation we logged.
    crafty_alloc_index = 0;
    // Execute transaction again, validating.
    txassert(crafty_log_tx_start != crafty_log_curr_pos);
    HTM_SGL_vars.nesting++; // We decrement nesting twice when we jump back, we need to restore it here
    longjmp(setjmp_buf, CRAFTY_JUMP_VALIDATION_SUCCESS);
#endif /* CRAFTY_REDO */
}



#ifdef CRAFTY_VALIDATE
static inline void crafty_after_htm_commit_validating() {
    thrprintf("HTM commit validating.\n");
    // Validation was successful.
    crafty_log_tx_start = crafty_log_curr_pos;
    crafty_validate_success++;
    // If the validation wasn't long enough for the logs to persist, wait here
    crafty_persist_finish();
    // Any allocations logged but not used are dead (can't be visible anywhere). Free them to avoid leaking.
    crafty_free_unused_mallocs(crafty_alloc_index);
#ifdef CRAFTY_REDO
    // Validation doesn't log free's, it performs them directly.
    txassert(crafty_free_last == 0);
#endif
}
#endif /* CRAFTY_VALIDATE */


static inline void crafty_after_htm_commit_readonly() {
    // Read only TX, no need for redo/validation. Region is done.
    // Reset the malloc log, any allocations logged are in use.
    crafty_alloc_last = 0;
    crafty_alloc_index = 0;
    crafty_readonly_success++;
#ifdef CRAFTY_REDO
    // If we logged any free's, free them now since the region is done.
    crafty_perform_delayed_free();
#endif /* CRAFTY_REDO */
}


static inline void crafty_after_htm_commit(int tid) {
    if (crafty_log_curr_pos == crafty_log_tx_start) {
        crafty_after_htm_commit_readonly();
#ifdef CRAFTY_VALIDATE
    } else if (crafty_is_logging == CRAFTY_LOGGING) {
        crafty_after_htm_commit_logging(tid);
    } else {
        txassert(crafty_is_logging == CRAFTY_VALIDATING);
        crafty_after_htm_commit_validating();
    }
#else /* ! CRAFTY_VALIDATE */
    } else {
        crafty_after_htm_commit_logging(tid);
    }
#endif /* CRAFTY_VALIDATE */
}



// Ensures that all logs by all threads that were before right now have been persisted
static inline void crafty_ensure_recoverable_before(int tid, uint64_t before, uint64_t next_overwrite) {
    thrprintf("Log may become irrecoverable, checking\n");
    int i;
    for (i = 0; i < MAX_NB_THREADS; i++) {
        uint64_t last_tx;
        if (i != tid && last_tx_ts_arr[i] != NULL && (
             // If a thread hasn't made any commits since MAX_LAG
             (last_tx = __atomic_load_n(last_tx_ts_arr[i], __ATOMIC_RELAXED)) + RECOVERY_MAX_LAG < before
             // Or since the commit we are about to overwrite
             || next_overwrite >= last_tx)) {
            thrprintf("Found thread %d is is lagging behind, pulling them to now\n", tid);
            // We need to make a commit for them
            unsigned status = _XABORT_RETRY;
            unsigned retries = 10;
            while (retries > 0) {
                retries--;
                status = _xbegin();
                if (status == _XBEGIN_STARTED) break;
            }
            if (retries == 0) ENTER_SGL(tid);
            // Add a commit entry to the other thread's log
            __crafty_log_entry(
                    log_curr_pos_arr[i], log_arr[i]->entries, &log_arr[i]->entries[LOG_ENTRIES],
#ifdef ROLLOVER_BITS
                    rollover_bit_arr[i],
#endif
                    COMMIT_ENTRY_ADDR, before
            );
            *log_tx_start_arr[i] = *log_curr_pos_arr[i]; // Fix other thread's tx_start as well
            if (HTM_test()) _xend(); else EXIT_SGL(tid);
        }
        recovery_ts_lower_bound = before;
    }
}


static inline uint64_t crafty_next_rdtscp_to_overwrite() {
    Crafty_log_entry_s *next = incr(crafty_log_curr_pos);
    while (rollover_addr_get(next->addr) != COMMIT_ENTRY_ADDR) next = incr(next);
    return rollover_val_get(next->addr, next->oldValue);
}


// Runs after a validation commit, successful redo, or an SGL commit.
static inline void crafty_after_any_transaction(int tid) {
#ifdef CRAFTY_STATS
    switch (crafty_currenttx_write_count) {
        case 0:case 1:case 2:case 3:
            crafty_write_histogram[crafty_currenttx_write_count]++;
            break;
        case 4:case 5:
            crafty_write_histogram[4]++;
            break;
        case 6:case 7:case 8:case 9:case 10:
            crafty_write_histogram[5]++;
            break;
        default:
            crafty_write_histogram[6]++;
            break;
    }
    thrprintf("Last tx write count: %li\n", crafty_currenttx_write_count);
    crafty_currenttx_write_count = 0;
#endif
    uint64_t now = rdtscp();
    uint64_t next_to_overwrite;
    // We need to check if the undo log is recoverable
    if (recovery_ts_lower_bound + RECOVERY_MAX_LAG < now) {
        // If it's past the maximum recovery lag
        crafty_ensure_recoverable_before(tid, now, crafty_next_rdtscp_to_overwrite());
    } // Or if we are wrapping around the log and the next transaction we'll overwrite is not recoverable
    else if (crafty_log_curr_pos < crafty_log_tx_start
             && (next_to_overwrite = crafty_next_rdtscp_to_overwrite()) > recovery_ts_lower_bound) {
        crafty_ensure_recoverable_before(tid, now, next_to_overwrite);
    }
    last_commited_ts = now;
}


static inline void *__crafty_allocate(size_t alignment, size_t size) {
    if (alignment == 0) return malloc(size);
    else return aligned_alloc(alignment, size);
}


static inline void *crafty_malloc(size_t alignment, size_t size) {
    // Don't log mallocs during SGL's, or outside persistent regions
    if (CRAFTY_IN_SGL || !HTM_test()) {
        void *addr = __crafty_allocate(alignment, size);
        txassert(addr);
        return addr;
    }
#ifdef CRAFTY_VALIDATE
    if (crafty_is_logging == CRAFTY_LOGGING) {
#endif /* CRAFTY_VALIDATE */
        void * addr = __crafty_allocate(alignment, size);
        txassert(addr);
        txassert_named(crafty_alloc_index < CRAFTY_ALLOC_LOG_SIZE, 6);
        crafty_alloc_log[crafty_alloc_index++] = addr;
        crafty_alloc_last++;
#ifdef CRAFTY_STATS
        crafty_allocs_logged++;
        if (crafty_alloc_last > crafty_alloc_high_mark) crafty_alloc_high_mark = crafty_alloc_last;
#endif /* CRAFTY_STATS */
        return addr;
#ifdef CRAFTY_VALIDATE
    }
    txassert(crafty_is_logging == CRAFTY_VALIDATING);
    if (malloc_usable_size(crafty_alloc_log[crafty_alloc_index]) >= size) {
        // We know this malloc, just return it
        void *addr = crafty_alloc_log[crafty_alloc_index++];
        txassert(addr);
        return addr;
    } else {
        // This malloc doesn't match what we logged. Some values must have changed, we should abort transaction
        // and retry.
        _xabort(CRAFTY_VALIDATION_FAIL);
    }
#endif /* CRAFTY_VALIDATE */
}


static inline void crafty_free(void * addr) {
    // Don't log free's during SGL's, or outside persistent regions, or during validation
    if (CRAFTY_IN_SGL || !HTM_test()
#ifdef CRAFTY_VALIDATE
        || crafty_is_logging == CRAFTY_VALIDATING
#endif /* CRAFTY_VALIDATE */
            ) {
        _free(addr, CRAFTY_IN_SGL ? "SGL" : !HTM_test() ? "non-tx" : "validation");
        return;
    }
#ifdef CRAFTY_REDO
    // If we do replay, we need to log the remaining free's so we can do them after the replay.
    txassert_named(crafty_free_last < CRAFTY_ALLOC_LOG_SIZE, 4);
    crafty_free_log[crafty_free_last++] = addr;
#ifdef CRAFTY_STATS
    if (crafty_free_last > crafty_free_high_mark) crafty_free_high_mark = crafty_free_last;
#endif /* CRAFTY_STATS */
#else
#ifdef CRAFTY_VALIDATE
    txassert(crafty_is_logging == CRAFTY_LOGGING);
#endif
#endif /* CRAFTY_REDO */
}

static inline void crafty_after_abort(int status) {
#ifdef CRAFTY_VALIDATE
    txassert(!crafty_is_logging || crafty_log_tx_start == crafty_log_curr_pos);
#else /* ! CRAFTY_VALIDATE */
    txassert(crafty_log_tx_start == crafty_log_curr_pos);
#endif /* CRAFTY_VALIDATE */
#ifndef NDEBUG
    if (status & _XABORT_EXPLICIT && HTM_get_named(status) > 0) {
		thrprintf("Named abort: %x\n", HTM_get_named(status));
	}
#endif /* NDEBUG */
#ifdef CRAFTY_STATS
    crafty_aborts.total++;
    if (status & _XABORT_EXPLICIT) crafty_aborts.explicit_++;
    if (status & _XABORT_RETRY) crafty_aborts.retry++;
    if (status & _XABORT_CONFLICT) crafty_aborts.conflict++;
    if (status & _XABORT_CAPACITY) crafty_aborts.capacity++;
    if (status == 0) crafty_aborts.zero++;
#ifdef CRAFTY_VALIDATE
    if (crafty_is_logging == CRAFTY_LOGGING) crafty_aborts_in_logging++;
    else                                     crafty_aborts_in_validating++;
#endif /* CRAFTY_VALIDATE */
#endif /* CRAFTY_STATS */

#ifdef CRAFTY_VALIDATE
    if (CRAFTY_VALIDATION_FAIL == (unsigned char)_XABORT_CODE(status)) {
        thrprintf("Validation failure!\n");
        txassert(crafty_validation_reattempts_left > 0);
        crafty_validation_reattempts_left--;
        // Validation failed because the log is incorrect. Switch to logging.
        crafty_is_logging = CRAFTY_LOGGING;
        // Logging shouldn't reuse any logs, reset them
        crafty_log_tx_start = crafty_log_curr_pos;
#ifdef CRAFTY_STATS
        crafty_valfail++;
#endif /* CRAFTY_STATS */
        // Logging shouldn't reuse the allocation log.
        crafty_free_unused_mallocs(0 /* free all */);
#ifdef CRAFTY_REDO
        // Ignore logged free's, the next logging phase can log them again.
        crafty_reset_free_log();
#endif /* CRAFTY_REDO */
        // If we're out of reattempts, switch to SGL
        if (crafty_validation_reattempts_left <= 0) {
            HTM_SGL_vars.budget = 0;
        }
    }
#endif /* CRAFTY_VALIDATE */
}


#ifdef __cplusplus
}
#endif

#endif /* NH_SOL_H */

#include "log.h"
#include <string.h>

static void cpy_to_cl(int rec_idx, intptr_t *addr);

enum LOG_CODES_ {
	NOT_MARKED = -2, MARKED = -1
};

#define CL_ADDR(addr) ({ \
	intptr_t cl_addr; \
	intptr_t cl_bits_off = -1; \
	cl_bits_off = cl_bits_off << 6; \
	cl_addr = (intptr_t) addr; \
	cl_addr = cl_addr & cl_bits_off; \
	cl_addr; \
})\

#define MARKER_POINTER(pool, addr) ({ \
	((intptr_t)addr >> 6) % pool->nb_markers; \
})

PHTM_marker_pool_s *PHTM_create_marker_pool(size_t size)
{
	PHTM_marker_pool_s *res;
	int i, nb_cache_lines = size / CACHE_LINE_SIZE;

	ALLOC_FN(res, PHTM_marker_pool_s, sizeof (PHTM_marker_pool_s));
	ALLOC_FN(res->markers, PHTM_marker_s, sizeof (PHTM_marker_s) * nb_cache_lines);

	//    res->markers = (PHTM_marker_s*) aligned_alloc(CACHE_LINE_SIZE,
	//                                                  sizeof (PHTM_marker_s)
	//                                                  * nb_cache_lines);
	res->nb_markers = nb_cache_lines;

	for (i = 0; i < nb_cache_lines; ++i) {
		res->markers[i].tid = -1;
	}

	return res;
}

void PHTM_init(int) {
	phtm_markers = PHTM_create_marker_pool(PHTM_NB_MARKERS * CACHE_LINE_SIZE);
}

PHTM_log_s *PHTM_create_log()
{
	PHTM_log_s *res;

	// TODO: check, res is NULL
	// ALLOC_FN(res, PHTM_log_s, sizeof (PHTM_log_s));
	res = (PHTM_log_s*) malloc(sizeof (PHTM_log_s));

	memset(res, 0, sizeof (PHTM_log_s));
	res->size = 0;
	res->is_persistent = 0;

	return res;
}

void PHTM_thr_init(int tid) {
	phtm_log = PHTM_create_log();
}

int PHTM_mark_addr(void* addr, int tid, int log_rec)
{
	intptr_t ptr = MARKER_POINTER(phtm_markers, addr);
	PHTM_marker_s *marker = &phtm_markers->markers[ptr];

	// marked before, return address
	if (marker->tid == tid) {
		return marker->ptr_to_log;
	}
	// marked by other TX
	if (marker->tid != -1) {
		return MARKED;
	}

	marker->ptr_to_log = log_rec;
	marker->tid = tid;

	// still not in use
	return NOT_MARKED;
}

void PHTM_rem_mark(void* addr)
{
	intptr_t ptr = MARKER_POINTER(phtm_markers, addr);
	PHTM_marker_s *marker = &phtm_markers->markers[ptr];

	marker->tid = -1;
	__sync_synchronize();
}

int PHTM_log_cache_line(int tid, void* addr, GRANULE_TYPE val)
{
	size_t size = phtm_log->size;
	int rec_idx = size;
	int in_marker = PHTM_mark_addr(addr, tid, rec_idx);
	intptr_t cl_addr;
	int i;

	if (size >= PHTM_LOG_MAX_REC) {
		// write limit exceeded
		return 1;
	}

	if (in_marker == MARKED) {
		// abort
		if (HTM_test()) { // cache line is locked by other transaction
			HTM_abort();
		}
		// The SGL waits the others
		while ((in_marker = PHTM_mark_addr(addr, tid, rec_idx)) == -1)
		__sync_synchronize();
	}

	cl_addr = CL_ADDR(addr);

	if (in_marker == NOT_MARKED) {
		// new entry

		cpy_to_cl(rec_idx, (intptr_t*) cl_addr);
		phtm_log->addrs[rec_idx] = cl_addr;
		phtm_log->size += 1;
		return 1;
	} else {
		// same TX accessed the cache line
		rec_idx = in_marker;

		if (phtm_log->addrs[rec_idx] != cl_addr) {
			// aliasing in the locks ---> must search a new entry
			for (i = rec_idx + 1; i < size; ++i) {
				if (phtm_log->addrs[i] == cl_addr) {
					// found
					cpy_to_cl(i, (intptr_t*) cl_addr);
					phtm_log->addrs[i] = cl_addr;
					break;
				}
			}

			if (i == size) {
				// not found, add in the end
				cpy_to_cl(i, (intptr_t*) cl_addr);
				phtm_log->addrs[rec_idx] = cl_addr;
				// phtm_log->size += 1;
				// int buf = phtm_log->size + 1; // at least updating the pointer is PM
				phtm_log->size += 1;
				MN_count_writes++;
				// MN_write(&(phtm_log->size), &buf, sizeof(int), 0);
			}
		} else {
			MN_count_writes++; // simulate modify the cache line
		}
	}

	return 1;
}

/* int PHTM_log_size(int tid) {
return phtm_log->size;
} */

void PHTM_log_clear()
{
	int i;
	int buf = 0;

	for (i = 0; i < phtm_log->size; ++i) {
		PHTM_rem_mark((void*) phtm_log->addrs[i]);
	}

	MN_write(&(phtm_log->is_persistent), &buf, sizeof(int), 0);
	phtm_log->size = 0;
	// phtm_log->size = 0;
	// phtm_log->is_persistent = 0;
	__sync_synchronize();
}

static void cpy_to_cl(int rec_idx, intptr_t *addr) {
	int i;

	// TODO: only add intptr_t if cache line is present
	// copy cache line to phtm_log
	for (i = 0; i < CACHE_LINE_SIZE / sizeof(intptr_t); ++i) {
		MN_write(&(phtm_log->records[rec_idx].cache_line[i]), addr, sizeof(intptr_t), 0);
		// ((intptr_t*) phtm_log->records[rec_idx].cache_line)[i]
		// = ((intptr_t*)addr)[i];
	}

	// memcpy is bad in HTM --> syscall?
}

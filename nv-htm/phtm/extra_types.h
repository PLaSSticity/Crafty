#ifndef EXTRA_TYPES_H
#define EXTRA_TYPES_H

#include "nh_types.h"

typedef struct PHTM_marker_
{
	int tid; // use -1 to mark as empty
	int ptr_to_log;
} /* CL_ALIGN */ PHTM_marker_s;

typedef struct PHTM_marker_pool_
{
	PHTM_marker_s *markers;
	size_t nb_markers;
} CL_ALIGN PHTM_marker_pool_s;

// log

typedef struct phtm_log_rec_
{
	char cache_line[CACHE_LINE_SIZE]; // CACHE_LINE_SIZE
} /* CL_ALIGN */ PHTM_log_rec_s;

typedef struct phtm_log_
{
	PHTM_log_rec_s records[PHTM_LOG_MAX_REC];
	intptr_t addrs[PHTM_LOG_MAX_REC];
	int is_persistent;
	size_t size; /* size is pointer to next entry */
} CL_ALIGN PHTM_log_s;


#endif /* EXTRA_TYPES_H */

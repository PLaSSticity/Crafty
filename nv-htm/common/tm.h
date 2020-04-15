#ifndef TM_GUARD_H
#define TM_GUARD_H

#include "nh_globals.h"
#include "nvmlib_wrapper.h"

#ifdef __cplusplus
extern "C"
{
#endif

// don't call these
// --
void TM_set_is_record(int tid, int is_rec);
void TM_set_commit_counter(int tid);
// --

int TM_inc_global_counter(int tid);
void TM_inc_local_counter(int tid);

int TM_get_local_counter(int tid);
int TM_get_global_counter(int tid);
void TM_init_nb_threads(int threads);
int TM_get_nb_threads();

void TM_inc_fallback(int tid);
void TM_inc_error(int tid, HTM_STATUS_TYPE error);

// statistics
int TM_get_error_count(int);

#ifdef __cplusplus
}
#endif

#endif /* TM_GUARD_H */

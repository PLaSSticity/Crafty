#ifndef LOG_SORTER_GUARD_H
#define LOG_SORTER_GUARD_H

#include "cp.h"
#include "extra_types.h"

#ifdef __cplusplus
extern "C"
{
#endif
    
typedef struct sorter_ {
    int tid;
    ts_s ts;
    int pos_base;
} sorter_s;

void LOG_SOR_init(cp_s* cp, NVLog_s** logs, size_t nb_logs);
void *LOG_SOR_main_thread(void*);

#ifdef __cplusplus
}
#endif

#endif /* LOG_SORTER_GUARD_H */


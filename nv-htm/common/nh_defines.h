#ifndef NH_DEFINES_H
#define NH_DEFINES_H

#include "rdtsc.h"

#ifndef LOG_THRESHOLD
#define LOG_THRESHOLD 0.5
#endif /* LOG_THRESHOLD */

#ifndef LOG_FILTER_THRESHOLD
#define LOG_FILTER_THRESHOLD 0.1
#endif /* LOG_FILTER_THRESHOLD */

#ifndef LOG_PERIOD
#define LOG_PERIOD 100
#endif /* LOG_PERIOD */

#ifndef SORT_ALG
#define SORT_ALG 4
#endif /* LOG_PERIOD */

#define KEY_SEMAPHORE  0x00012345
#define KEY_CHKP_STATE 0x00012222
#define KEY_LOGS       0x00054321

#define LOG_MANAGER_FILE "/tmp/com_file.socket"
#define CLIENT_FILE "/tmp/cli_file.socket"
#define MAXMSG  512

#ifndef STATS_FILE
#define STATS_FILE "./stats_file"
#endif /* NVMHTM_LOG_SIZE */

#ifndef NVMHTM_LOG_SIZE
#define NVMHTM_LOG_SIZE 10000000
#endif /* NVMHTM_LOG_SIZE */

#ifndef TM_INIT_BUDGET
#define TM_INIT_BUDGET 5
#endif /* TM_INIT_BUDGET */

#ifndef MAX_NB_THREADS
#define MAX_NB_THREADS 56 // TODO: use MAX_PHYS_THRS
#endif /* MAX_NB_THREADS */

#define CODE_LOG_ABORT 1

// #################################
// ### PHTM ########################
// #################################
#ifndef PHTM_NB_MARKERS
#define PHTM_NB_MARKERS 0x2FFFF
#endif

#define PHTM_LOG_MAX_REC 1024
// #################################

// #################################
#include "arch.h" // architecture specific defines
// #################################

#endif /* NH_DEFINES_H */

#ifndef UTILS_H
#define UTILS_H

#include "nh.h"
#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LOG_MOD2(idx, mod) ({ \
  assert(__builtin_popcount((mod)) == 1); /* base 2 mod */ \
  (long long)(idx) & ((mod) - 1); \
})
//
#define LOG_DISTANCE2(start, end, mod) ({ \
  assert(start < mod); \
  assert(end < mod); \
  LOG_MOD2(((long long)(end) - (long long)(start)), (mod)); \
})
//

#define ptr_mod(ptr, inc, mod) ({ \
	LOG_MOD2((long long)ptr + (long long)inc, mod); \
})
/*({ \
	int res = (int)ptr + (int)inc; \
	res = (res % mod + mod) % mod; \
	res; \
})*/

#ifndef TIMER_T
#define TIMER_T                         struct timeval
#include <sys/time.h>
#define TIMER_READ(time)                gettimeofday(&(time), NULL)
#define TIMER_DIFF_SECONDS(start, stop) \
    (((double)(stop.tv_sec)  + (double)(stop.tv_usec / 1000000.0)) - \
     ((double)(start.tv_sec) + (double)(start.tv_usec / 1000000.0)))
#endif /* TIMER_T */

#ifndef MAX
#define MAX(a,b) ({ (a) > (b) ? (a) : (b); })
#define MIN(a,b) ({ (a) < (b) ? (a) : (b); })
#endif /* MAX */

// TODO: maybe pass to an arch.h like file

// TODO: create debug LOG system
#ifdef NDEBUG
#define LOG_INIT(file)
#define LOG(...)
#define LOG_CLOSE()
#else

    extern FILE* DBG_LOG_log_fp;
    extern ts_s DBG_LOG_log_init_ts;

#define LOG_INIT(file) ({ \
    DBG_LOG_log_fp = fopen(file, "w"); \
    DBG_LOG_log_init_ts = rdtscp(); \
})
#define LOG(...) ({ \
    fprintf(DBG_LOG_log_fp, "[%12f] FILE=%s FUNC=%s LINE=%s => ", \
		(double) (rdtscp() - DBG_LOG_log_init_ts) / (double) CLOCKS_PER_SEC, \
		__PRETTY_FUNCTION__, __FILE__, __LINE__); \
    fprintf(DBG_LOG_log_fp, __VA_ARGS__); \
    fprintf(DBG_LOG_log_fp, "\n"); \
		fflush(DBG_LOG_log_fp); \
})

#define LOG_CLOSE() ({ fclose(DBG_LOG_log_fp); })
#endif

int make_named_socket(const char *filename);
int send_to_named_socket(int sockfd, const char *filename, void*, size_t);
int recv_from_named_socket(int sockfd, char *name, size_t name_size, void*, size_t);
int run_command(char *command, char *buffer_stdout, size_t size_buf);
void launch_thread_at(int core, void*(*callback)(void*));
void set_affinity_at(int core);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */

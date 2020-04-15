/* =============================================================================
 *
 * thread.h
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 *
 * ------------------------------------------------------------------------
 *
 * For the license of ssca2, please see ssca2/COPYRIGHT
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 *
 * ------------------------------------------------------------------------
 *
 * Unless otherwise noted, the following license applies to STAMP files:
 *
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#ifndef THREAD_H
#define THREAD_H 1

#ifndef USE_P8
# define THR_CACHE_LINE_SIZE 64
#else /* USE_P8 */
# define THR_CACHE_LINE_SIZE 128
#endif /* USE_P8 */

#include <pthread.h>


typedef struct padded_scalar {
    volatile long value;
    char suffixPadding[THR_CACHE_LINE_SIZE];
} __attribute__((aligned(THR_CACHE_LINE_SIZE))) padded_scalar_t;

typedef struct spinlock {
    pthread_spinlock_t lock;
    char suffixPadding[THR_CACHE_LINE_SIZE];
} __attribute__((aligned(THR_CACHE_LINE_SIZE))) spinlock_t;

typedef struct padded_statistics {
    unsigned long tle_commits;
    unsigned long gl_commits;
    unsigned long conflicts;
    unsigned long self;
    unsigned long trans;
    unsigned long nontrans;
    unsigned long capacity;
    unsigned long other;
    unsigned long user;
    unsigned long persistent;
    char suffixPadding[THR_CACHE_LINE_SIZE];
} __attribute__((aligned(THR_CACHE_LINE_SIZE))) padded_statistics_t;

extern __attribute__((aligned(THR_CACHE_LINE_SIZE))) padded_statistics_t stats_array[];

extern __attribute__((aligned(THR_CACHE_LINE_SIZE))) pthread_spinlock_t single_global_lock;

extern long              global_numThread    ;

extern __thread unsigned long backoff;
extern __thread unsigned long cm_seed;

extern __thread unsigned int local_exec_mode;
extern __thread unsigned int local_thread_id;

extern __thread void* rot_readset[];
extern __thread unsigned long rs_counter;

extern __attribute__((aligned(THR_CACHE_LINE_SIZE))) padded_scalar_t counters[];

extern unsigned int htm_rot_enabled;
extern unsigned int allow_rots_ros;
extern unsigned int allow_htms;
extern unsigned int allow_stms;

# ifndef MIN_BACKOFF
#  define MIN_BACKOFF                   (1UL << 2)
# endif /* MIN_BACKOFF */
# ifndef MAX_BACKOFF
#  define MAX_BACKOFF                   (1UL << 31)
# endif /* MAX_BACKOFF */

#ifndef REDUCED_TM_API

#include <stdlib.h>
#include "types.h"
#include "nh.h"

#ifdef __cplusplus
extern "C" {
#endif


#define THREAD_T                            pthread_t
#define THREAD_ATTR_T                       pthread_attr_t

#define THREAD_ATTR_INIT(attr)              pthread_attr_init(&attr)
#define THREAD_JOIN(tid)                    pthread_join(tid, (void**)NULL)
#define THREAD_CREATE(tid, attr, fn, arg)   pthread_create(&(tid), \
                                                           &(attr), \
                                                           (void* (*)(void*))(fn), \
                                                           (void*)(arg))

#define THREAD_LOCAL_T                      pthread_key_t
#define THREAD_LOCAL_INIT(key)              pthread_key_create(&key, NULL)
#define THREAD_LOCAL_SET(key, val)          pthread_setspecific(key, (void*)(val))
#define THREAD_LOCAL_GET(key)               pthread_getspecific(key)

#define THREAD_MUTEX_T                      pthread_mutex_t
#define THREAD_MUTEX_INIT(lock)             pthread_mutex_init(&(lock), NULL)
#define THREAD_MUTEX_LOCK(lock)             pthread_mutex_lock(&(lock))
#define THREAD_MUTEX_UNLOCK(lock)           pthread_mutex_unlock(&(lock))

#define THREAD_COND_T                       pthread_cond_t
#define THREAD_COND_INIT(cond)              pthread_cond_init(&(cond), NULL)
#define THREAD_COND_SIGNAL(cond)            pthread_cond_signal(&(cond))
#define THREAD_COND_BROADCAST(cond)         pthread_cond_broadcast(&(cond))
#define THREAD_COND_WAIT(cond, lock)        pthread_cond_wait(&(cond), &(lock))

#  define THREAD_BARRIER_T                  pthread_barrier_t
#  define THREAD_BARRIER_ALLOC(N)           (pthread_barrier_t*)malloc(sizeof(pthread_barrier_t))
#  define THREAD_BARRIER_INIT(bar, N)       pthread_barrier_init(bar, NULL, N)
#  define THREAD_BARRIER(bar, tid)          pthread_barrier_wait(bar)
#  define THREAD_BARRIER_FREE(bar)          pthread_barrier_destroy(bar)

/*
typedef struct barrier {
    pthread_cond_t complete;
    pthread_mutex_t mutex;
    int count;
    int crossing;
} barrier_t;

barrier_t *barrier_alloc();

void barrier_free(barrier_t *b);

void barrier_init(barrier_t *b, int n);

void barrier_cross(barrier_t *b);

*/

void thread_startup (long numThread);

void thread_start (void (*funcPtr)(void*), void* argPtr);

void thread_shutdown ();

void thread_barrier_wait();

long thread_getId();

long thread_getNumThread();

void bindThread(long threadId);


#ifdef __cplusplus
}
#endif

#endif

#endif /* THREAD_H */


/* =============================================================================
 *
 * End of thread.h
 *
 * =============================================================================
 */

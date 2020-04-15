/* =============================================================================
 *
 * thread.c
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


#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include "thread.h"

#ifndef REDUCED_TM_API

#include <assert.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include "types.h"
#include "random.h"
//#include "rapl.h"

THREAD_LOCAL_T           global_threadId;
__thread int             global_threadId_;
long                     global_numThread  = 1; // is called extern in other file
static THREAD_BARRIER_T* global_barrierPtr      = NULL;
static long*             global_threadIds       = NULL;
static THREAD_ATTR_T     global_threadAttr;
static THREAD_T*         global_threads         = NULL;
static void            (*global_funcPtr)(void*) = NULL;
static void            (*global_funcPtr2)(void*) = NULL;
static void*             global_argPtr          = NULL;
static volatile bool_t   global_doShutdown      = FALSE;

// random.h defines this
void
bindThread(long threadId) {
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    int offset = threadId;
    int id = threadId;

	// TODO:
/*#if defined(DO_CHECKPOINT) && DO_CHECKPOINT == 5
  if (threadId == 13) {
    id = 26; // does not pin on the checkpointer
  }
  if (threadId == 26) {
    id = 13; // does not pin on the checkpointer
  }
  if (threadId == 27) {
    id = 1; // does not pin on the checkpointer
  }
  // printf("[bind] convert thread %i to %i \n", threadId, id);
#endif*/
#if defined(__powerpc__)
	// TODO: it is easy to get the number of cores
	/* power8 */
	offset = (id * 8) % 80;
#else /* intel14 */
	if (id >= 14 && id < 28)
		offset += 14;
	if (id >= 28 && id < 42)
		offset -= 14;
	offset = id % 56;
#endif
//	printf("pinned thread to %i\n", offset);
    CPU_SET(offset, &my_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
}

static void
threadWait (void* argPtr)
{
    long threadId = *(long*)argPtr;
	static int init_stats = 0;

    THREAD_LOCAL_SET(global_threadId, (long)threadId);
    global_threadId_ = threadId;

    bindThread(threadId);

	// think twice before changing anything!!!

	do {
		THREAD_BARRIER(global_barrierPtr, threadId); /* wait for start parallel */
		if (global_doShutdown) {
			break; // after shutdown thread 0 call this
		}
		while (global_funcPtr == NULL) {
			pthread_yield(); // to block or to execute NULL ptr function
		}

		THREAD_BARRIER(global_barrierPtr, threadId); /* wait for start parallel */
		if(threadId == 0 && !init_stats) {
			init_stats = 1;
			// NVHTM_start_stats(); // GOTO_SIM()
		}
    #if DO_CHECK == 1
    if (threadId == 13) global_funcPtr2(global_argPtr);
    else
    #endif
    global_funcPtr(global_argPtr);
		THREAD_BARRIER(global_barrierPtr, threadId); /* wait for end parallel */
		if (threadId == 0) {
			global_funcPtr = NULL;
			MEMFENCE;
			break; // main thread exits loop, other keep looping
		}
	} while (1);
}

/* =============================================================================
 * thread_startup
 * -- Create pool of secondary threads
 * -- numThread is total number of threads (primary + secondaries)
 * =============================================================================
 */
void
thread_startup (long numThread)
{
    int i;

    global_numThread = numThread;
    global_doShutdown = FALSE;

    /* Set up barrier */
    assert(global_barrierPtr == NULL);
    global_barrierPtr = THREAD_BARRIER_ALLOC(numThread);
    assert(global_barrierPtr);
    THREAD_BARRIER_INIT(global_barrierPtr, numThread);

    /* Set up ids */
    THREAD_LOCAL_INIT(global_threadId);
    assert(global_threadIds == NULL);
    global_threadIds = (long*)malloc(numThread * sizeof(long));
    assert(global_threadIds);
    for (i = 0; i < numThread; i++) {
        global_threadIds[i] = i;
    }

    /* Set up thread list */
    assert(global_threads == NULL);
    global_threads = (THREAD_T*)malloc(numThread * sizeof(THREAD_T));
    assert(global_threads);

    //single_global_lock.counter = 0;

    /* Set up pool */
    THREAD_ATTR_INIT(global_threadAttr);
    for (i = 1; i < numThread; i++) { // main thread also does work
        THREAD_CREATE(global_threads[i],
                      global_threadAttr,
                      &threadWait,
                      &global_threadIds[i]);
    }
}

void
thread_start (void (*funcPtr)(void*), void* argPtr)
{

    global_funcPtr = funcPtr;
    global_argPtr = argPtr;
    __sync_synchronize();

    long threadId = 0; /* primary */

    threadWait((void*)&threadId);
}

void
thread_start2 (void (*funcPtr)(void*), void (*funcPtr2)(void*), void* argPtr)
{

    global_funcPtr = funcPtr;
    global_funcPtr2 = funcPtr2;
    global_argPtr = argPtr;
    __sync_synchronize();

    long threadId = 0; /* primary */

    threadWait((void*)&threadId);
}

void
thread_barrier_wait()
{
	THREAD_BARRIER(global_barrierPtr, 0);
}

void
thread_shutdown ()
{
    /* Make secondary threads exit wait() */

	long i, numThread = global_numThread;

    global_doShutdown = TRUE;
	MEMFENCE;

	long threadId = 0; /* primary */

    // threadWait((void*)&threadId); // goes there to unblock the threads
	THREAD_BARRIER(global_barrierPtr, threadId);

    for (i = 1; i < numThread; i++) {
        THREAD_JOIN(global_threads[i]);
    }

    THREAD_BARRIER_FREE(global_barrierPtr);
    global_barrierPtr = NULL;

    free(global_threadIds);
    global_threadIds = NULL;

    free(global_threads);
    global_threads = NULL;

    global_numThread = 1;
}

long
thread_getId()
{
    return NVMHTM_get_thr_id();
}

long
thread_getNumThread()
{
    return global_numThread;
}

#endif

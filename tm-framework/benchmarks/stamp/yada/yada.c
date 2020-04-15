/* =============================================================================
 *
 * yada.c
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

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "region.h"
#include "list.h"
#include "mesh.h"
#include "heap.h"
#include "thread.h"
#include "timer.h"
#include "tm.h"


#define PARAM_DEFAULT_INPUTPREFIX ("")
#define PARAM_DEFAULT_NUMTHREAD   (1L)
#define PARAM_DEFAULT_ANGLE       (20.0)
#define PARAM_DEFAULT_REPEATS (1)

unsigned int htm_rot_enabled = 1;
unsigned int allow_rots_ros = 1;
unsigned int allow_htms = 1;

__attribute__ ((aligned (CACHE_LINE_SIZE)))
     padded_scalar_t fallback_in_use;

__attribute__ ((aligned (CACHE_LINE_SIZE)))
     padded_scalar_t exists_sw;

     __thread unsigned long backoff = MIN_BACKOFF;
     __thread unsigned long cm_seed = 123456789UL;

__attribute__ ((aligned (CACHE_LINE_SIZE)))
     padded_statistics_t stats_array[80];

__attribute__ ((aligned (CACHE_LINE_SIZE)))
     pthread_spinlock_t single_global_lock = 0;

__attribute__ ((aligned (CACHE_LINE_SIZE)))
     padded_scalar_t counters[80];

__attribute__ ((aligned (CACHE_LINE_SIZE)))
     pthread_spinlock_t writers_lock = 0;

__attribute__ ((aligned (CACHE_LINE_SIZE)))
     pthread_spinlock_t reader_locks[80];

__attribute__ ((aligned (CACHE_LINE_SIZE)))
     padded_statistics_t statistics_array[80];

     __thread unsigned int local_exec_mode = 0;

     __thread unsigned int local_thread_id;

     __thread void *rot_readset[100000];
     __thread unsigned long rs_counter = 0;


     unsigned int ucb_levers = 3;
     unsigned long ucb_trials[3];


     char *global_inputPrefix = PARAM_DEFAULT_INPUTPREFIX;
     // long global_numThread = PARAM_DEFAULT_NUMTHREAD;
     double global_angleConstraint = PARAM_DEFAULT_ANGLE;
     int global_repeats = PARAM_DEFAULT_REPEATS;
     mesh_t *global_meshPtr;
     heap_t *global_workHeapPtr;
     long global_totalNumAdded = 0;
     long global_numProcess = 0;


/* =============================================================================
 * displayUsage
 * =============================================================================
 */
     static void displayUsage (const char *appName)
{
  printf ("Usage: %s [options]\n", appName);
  puts ("\nOptions:                              (defaults)\n");
  printf ("    a <FLT>   Min [a]ngle constraint  (%lf)\n",
          PARAM_DEFAULT_ANGLE);
  printf ("    i <STR>   [i]nput name prefix     (%s)\n",
          PARAM_DEFAULT_INPUTPREFIX);
  printf ("    t <UINT>  Number of [t]hreads     (%li)\n",
          PARAM_DEFAULT_NUMTHREAD);
  exit (1);
}


/* =============================================================================
 * parseArgs
 * =============================================================================
 */
static void
parseArgs (long argc, char *const argv[])
{
  long i;
  long opt;

  opterr = 0;

  while ((opt = getopt (argc, argv, "a:i:t:r:")) != -1) {
    switch (opt) {
    case 'a':
      global_angleConstraint = atof (optarg);
      break;
    case 'i':
      global_inputPrefix = optarg;
      break;
    case 'r':
      global_repeats = atoi (optarg);
    case 't':
      global_numThread = atol (optarg);
      break;
    case '?':
    default:
      opterr++;
      break;
    }
  }

  for (i = optind; i < argc; i++) {
    fprintf (stderr, "Non-option argument: %s\n", argv[i]);
    opterr++;
  }

  if (opterr) {
    displayUsage (argv[0]);
  }
}


/* =============================================================================
 * initializeWork
 * =============================================================================
 */
static long
initializeWork (heap_t * workHeapPtr, mesh_t * meshPtr)
{
  random_t *randomPtr = random_alloc ();

  random_seed (randomPtr, 0);
  mesh_shuffleBad (meshPtr, randomPtr);
  random_free (randomPtr);

  long numBad = 0;

  while (1) {
    element_t *elementPtr = mesh_getBad (meshPtr);

    if (!elementPtr) {
      break;
    }
    numBad++;
    bool_t status = heap_insert (workHeapPtr, (void *) elementPtr);

    assert (status);
    element_setIsReferenced (elementPtr, TRUE);
  }

  return numBad;
}


/* =============================================================================
 * process
 * =============================================================================
 */
void
process ()
{
  TM_THREAD_ENTER ();

  heap_t *workHeapPtr = global_workHeapPtr;
  mesh_t *meshPtr = global_meshPtr;
  region_t *regionPtr;
  long totalNumAdded = 0;
  long numProcess = 0;

  regionPtr = PREGION_ALLOC ();
  assert (regionPtr);

  while (1) {

    element_t *elementPtr;

    AL_LOCK (0);
    int ro = 0;

    local_exec_mode = 0;
    TM_BEGIN();
    elementPtr = TMHEAP_REMOVE (workHeapPtr);
    TM_END ();
    if (elementPtr == NULL) {
      break;
    }

    bool_t isGarbage;

    AL_LOCK (0);
    local_exec_mode = 0;
    TM_BEGIN();
    isGarbage = TMELEMENT_ISGARBAGE (elementPtr);
    TM_END ();
    if (isGarbage) {
      /*
       * Handle delayed deallocation
       */
      PELEMENT_FREE (elementPtr);
      continue;
    }

    long numAdded;

    AL_LOCK (0);
    local_exec_mode = 0;
    TM_BEGIN();
    PREGION_CLEARBAD (regionPtr);
    numAdded = TMREGION_REFINE (regionPtr, elementPtr, meshPtr);
    TM_END ();

    AL_LOCK (0);
    local_exec_mode = 0;
    TM_BEGIN();
    TMELEMENT_SETISREFERENCED (elementPtr, FALSE);
    isGarbage = TMELEMENT_ISGARBAGE (elementPtr);
    TM_END ();
    if (isGarbage) {
      /*
       * Handle delayed deallocation
       */
      PELEMENT_FREE (elementPtr);
    }

    totalNumAdded += numAdded;

    AL_LOCK (0);
    local_exec_mode = 0;
    TM_BEGIN();
    TMREGION_TRANSFERBAD (regionPtr, workHeapPtr);
    TM_END ();

    numProcess++;

  }

  AL_LOCK (0);
  int ro = 0;

  local_exec_mode = 0;
  TM_BEGIN();
  if (local_exec_mode == 1 || local_exec_mode == 3) {
    long temp_l = SLOW_PATH_SHARED_READ (global_totalNumAdded);

    SLOW_PATH_SHARED_WRITE (global_totalNumAdded, temp_l + totalNumAdded);
    long temp_l2 = SLOW_PATH_SHARED_READ (global_numProcess);

    SLOW_PATH_SHARED_WRITE (global_numProcess, temp_l2 + numProcess);
  }
  else {
    FAST_PATH_SHARED_WRITE (global_totalNumAdded,
                            FAST_PATH_SHARED_READ (global_totalNumAdded) +
                            totalNumAdded);
    FAST_PATH_SHARED_WRITE (global_numProcess,
                            FAST_PATH_SHARED_READ (global_numProcess) +
                            numProcess);
  }
  TM_END ();

  PREGION_FREE (regionPtr);

  TM_THREAD_EXIT ();
}


/* =============================================================================
 * main
 * =============================================================================
 */
MAIN (argc, argv)
{
  GOTO_REAL ();

  SETUP_NUMBER_TASKS (6);

  /*
   * Initialization
   */

  parseArgs (argc, (char **const) argv);
  TM_STARTUP (global_numThread);
  P_MEMORY_STARTUP (global_numThread);

  SETUP_NUMBER_THREADS (global_numThread);

  thread_startup (global_numThread);


  int repeat = global_repeats;
  double time_total = 0.0;

  for (; repeat > 0; --repeat) {

    global_meshPtr = mesh_alloc ();
    assert (global_meshPtr);
    long initNumElement = mesh_read (global_meshPtr, global_inputPrefix);

    global_workHeapPtr = heap_alloc (1, &element_heapCompare);
    assert (global_workHeapPtr);
    long initNumBadElement =
      initializeWork (global_workHeapPtr, global_meshPtr);

    TIMER_T start;

    TIMER_READ (start);
    GOTO_SIM ();
    thread_start (process, NULL);
    GOTO_REAL ();
    TIMER_T stop;

    TIMER_READ (stop);
    double time_tmp = TIMER_DIFF_SECONDS (start, stop);

    printf ("%lf\n", time_tmp);
    PRINT_STATS ();
    time_total += time_tmp;

  }
  printf ("Elapsed time                    = %0.3lf\n", time_total);
  fflush (stdout);

  TM_SHUTDOWN ();
  P_MEMORY_SHUTDOWN ();

  GOTO_SIM ();

  thread_shutdown ();

  MAIN_RETURN (0);
}


/* =============================================================================
 *
 * End of ruppert.c
 *
 * =============================================================================
 */

/* =============================================================================
 *
 * intruder.c
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
#include "decoder.h"
#include "detector.h"
#include "dictionary.h"
#include "packet.h"
#include "stream.h"
#include "thread.h"
#include "timer.h"
#include "tm.h"

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

     enum param_types
     {
       PARAM_ATTACK = (unsigned char) 'a',
       PARAM_LENGTH = (unsigned char) 'l',
       PARAM_NUM = (unsigned char) 'n',
       PARAM_SEED = (unsigned char) 's',
       PARAM_THREAD = (unsigned char) 't',
       PARAM_REPEAT = (unsigned char) 'r',
     };

     enum param_defaults
     {
       PARAM_DEFAULT_ATTACK = 10,
       PARAM_DEFAULT_LENGTH = 16,
       PARAM_DEFAULT_NUM = 1 << 20,
       PARAM_DEFAULT_SEED = 1,
       PARAM_DEFAULT_THREAD = 1,
       PARAM_DEFAULT_REPEAT = 1,
     };

     long global_params[256];   /* 256 = ascii limit */

     static void setDefaultParams (void)
{
  global_params[PARAM_ATTACK] = PARAM_DEFAULT_ATTACK;
  global_params[PARAM_LENGTH] = PARAM_DEFAULT_LENGTH;
  global_params[PARAM_NUM] = PARAM_DEFAULT_NUM;
  global_params[PARAM_SEED] = PARAM_DEFAULT_SEED;
  global_params[PARAM_THREAD] = PARAM_DEFAULT_THREAD;
  global_params[PARAM_REPEAT] = PARAM_DEFAULT_REPEAT;
}

typedef struct arg
{
  /* input: */
  stream_t *streamPtr;
  decoder_t *decoderPtr;
  /* output: */
  vector_t **errorVectors;
} arg_t;


/* =============================================================================
 * displayUsage
 * =============================================================================
 */
static void
displayUsage (const char *appName)
{
  printf ("Usage: %s [options]\n", appName);
  puts ("\nOptions:                            (defaults)\n");
  printf ("    a <UINT>   Percent [a]ttack     (%i)\n", PARAM_DEFAULT_ATTACK);
  printf ("    l <UINT>   Max data [l]ength    (%i)\n", PARAM_DEFAULT_LENGTH);
  printf ("    n <UINT>   [n]umber of flows    (%i)\n", PARAM_DEFAULT_NUM);
  printf ("    s <UINT>   Random [s]eed        (%i)\n", PARAM_DEFAULT_SEED);
  printf ("    t <UINT>   Number of [t]hreads  (%i)\n", PARAM_DEFAULT_THREAD);
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

  setDefaultParams ();

  while ((opt = getopt (argc, argv, "a:l:n:s:t:f:p:r:")) != -1) {
    switch (opt) {
    case 'a':
    case 'l':
    case 'n':
    case 's':
    case 't':
    case 'f':
    case 'p':
    case 'r':
      global_params[(unsigned char) opt] = atol (optarg);
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
 * processPackets
 * =============================================================================
 */
void
processPackets (void *argPtr)
{
  TIMER_T startTime;

  TIMER_READ (startTime);

  TM_THREAD_ENTER ();

  long threadId = thread_getId ();

  stream_t *streamPtr = ((arg_t *) argPtr)->streamPtr;
  decoder_t *decoderPtr = ((arg_t *) argPtr)->decoderPtr;
  vector_t **errorVectors = ((arg_t *) argPtr)->errorVectors;

  detector_t *detectorPtr = PDETECTOR_ALLOC ();

  assert (detectorPtr);
  PDETECTOR_ADDPREPROCESSOR (detectorPtr, &preprocessor_toLower);

  vector_t *errorVectorPtr = errorVectors[threadId];

  while (1) {

    char *bytes;
    int i;
    int lim = 10000;

    AL_LOCK (0);
    int ro = 0;

    local_exec_mode = 0;
    thrprintf(">>>> GetPacket\n"); // kaan
    TM_BEGIN();
    bytes = TMSTREAM_GETPACKET (streamPtr);
    TM_END ();
    thrprintf("<<<< GetPacket: %p\n", bytes); // kaan
    if (!bytes) {
      break;
    }

    packet_t *packetPtr = (packet_t *) bytes;
    long flowId = packetPtr->flowId;

    error_t error;

    AL_LOCK (0);
    local_exec_mode = 1;
    thrprintf("Decoder: %p\n", bytes); // kaan
    TM_BEGIN();
    error = TMDECODER_PROCESS (decoderPtr,
                               bytes,
                               (PACKET_HEADER_LENGTH + packetPtr->length));
    TM_END ();
    if (error) {
      /*
       * Currently, stream_generate() does not create these errors.
       */
      assert (0);
      bool_t status = PVECTOR_PUSHBACK (errorVectorPtr, (void *) flowId);

      assert (status);
    }

    char *data;
    long decodedFlowId;

    AL_LOCK (0);
    local_exec_mode = 0;
    TM_BEGIN();
    data = TMDECODER_GETCOMPLETE (decoderPtr, &decodedFlowId);
    TM_END ();
    if (data) {
      error_t error = PDETECTOR_PROCESS (detectorPtr, data);

      P_FREE (data);
      if (error) {
        bool_t status = PVECTOR_PUSHBACK (errorVectorPtr,
                                          (void *) decodedFlowId);

        assert (status);
      }
    }

  }

  PDETECTOR_FREE (detectorPtr);

  TM_THREAD_EXIT ();

}


/* =============================================================================
 * main
 * =============================================================================
 */
MAIN (argc, argv)
{
  GOTO_REAL ();

  /*
   * Initialization
   */
  SETUP_NUMBER_TASKS (3);

  parseArgs (argc, (char **const) argv);
  long numThread = global_params[PARAM_THREAD];

  SETUP_NUMBER_THREADS (numThread);
  TM_STARTUP(numThread);
  P_MEMORY_STARTUP (numThread);
  thread_startup (numThread);

  long percentAttack = global_params[PARAM_ATTACK];
  long maxDataLength = global_params[PARAM_LENGTH];
  long numFlow = global_params[PARAM_NUM];
  long randomSeed = global_params[PARAM_SEED];

/*    printf("Percent attack  = %li\n", percentAttack);
    printf("Max data length = %li\n", maxDataLength);
    printf("Num flow        = %li\n", numFlow);
    printf("Random seed     = %li\n", randomSeed); */

  double time_total = 0.0;
  int repeats = global_params[PARAM_REPEAT];

  for (; repeats > 0; --repeats) {

    dictionary_t *dictionaryPtr = dictionary_alloc ();

    assert (dictionaryPtr);
    stream_t *streamPtr = stream_alloc (percentAttack);

    assert (streamPtr);
    long numAttack = stream_generate (streamPtr,
                                      dictionaryPtr,
                                      numFlow,
                                      randomSeed,
                                      maxDataLength);

//    printf("Num attack      = %li\n", numAttack);

    decoder_t *decoderPtr = decoder_alloc ();

    assert (decoderPtr);

    vector_t **errorVectors =
      (vector_t **) malloc (numThread * sizeof (vector_t *));
    assert (errorVectors);
    long i;

    for (i = 0; i < numThread; i++) {
      vector_t *errorVectorPtr = vector_alloc (numFlow);

      assert (errorVectorPtr);
      errorVectors[i] = errorVectorPtr;
    }

    arg_t arg;

    arg.streamPtr = streamPtr;
    arg.decoderPtr = decoderPtr;
    arg.errorVectors = errorVectors;

    /*
     * Run transactions
     */

    TIMER_T startTime;

    TIMER_READ (startTime);
    GOTO_SIM ();
    thread_start (processPackets, (void *) &arg);
    GOTO_REAL ();
    TIMER_T stopTime;

    TIMER_READ (stopTime);
    double time_tmp = TIMER_DIFF_SECONDS (startTime, stopTime);

    time_total += time_tmp;
    printf ("%lf\n", time_tmp);
    PRINT_STATS ();

    /*
     * Check solution
     */

    long numFound = 0;

    for (i = 0; i < numThread; i++) {
      vector_t *errorVectorPtr = errorVectors[i];
      long e;
      long numError = vector_getSize (errorVectorPtr);

      numFound += numError;
      for (e = 0; e < numError; e++) {
        long flowId = (long) vector_at (errorVectorPtr, e);
        bool_t status = stream_isAttack (streamPtr, flowId);

        assert (status);
      }
    }
//    printf("Num found       = %li\n", numFound);
    assert (numFound == numAttack);

    /*
     * Clean up
     */

    for (i = 0; i < numThread; i++) {
      vector_free (errorVectors[i]);
    }
    free (errorVectors);
    decoder_free (decoderPtr);
    stream_free (streamPtr);
    dictionary_free (dictionaryPtr);

  }

  printf ("Elapsed time    = %f seconds\n", time_total);

  TM_SHUTDOWN ();
  P_MEMORY_SHUTDOWN ();

  GOTO_SIM ();

  thread_shutdown ();

  MAIN_RETURN (0);
}


/* =============================================================================
 *
 * End of intruder.c
 *
 * =============================================================================
 */

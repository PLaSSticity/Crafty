#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "computeGraph.h"
#include "cutClusters.h"
#include "defs.h"
#include "findSubGraphs.h"
#include "genScalData.h"
#include "getStartLists.h"
#include "getUserParameters.h"
#include "globals.h"
#include "timer.h"
#include "thread.h"
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


MAIN (argc, argv)
{
  GOTO_REAL ();

  SETUP_NUMBER_TASKS (10);

  /*
   * Tuple for Scalable Data Generation
   * stores startVertex, endVertex, long weight and other info
   */
  graphSDG *SDGdata;

  /*
   * The graph data structure for this benchmark - see defs.h
   */
  graph *G;

#ifdef ENABLE_KERNEL2
  /*
   * Kernel 2
   */
  edge *maxIntWtList;
  edge *soughtStrWtList;
  long maxIntWtListSize;
  long soughtStrWtListSize;

#endif /* ENABLE_KERNEL2 */

#ifdef ENABLE_KERNEL3

#  ifndef ENABLE_KERNEL2
#    error KERNEL3 requires KERNEL2
#  endif

  /*
   * Kernel 3
   */
  V *intWtVList = NULL;
  V *strWtVList = NULL;
  Vl **intWtVLList = NULL;
  Vl **strWtVLList = NULL;
  Vd *intWtVDList = NULL;
  Vd *strWtVDList = NULL;

#endif /* ENABLE_KERNEL3 */

  double totalTime = 0.0;

  /* -------------------------------------------------------------------------
   * Preamble
   * -------------------------------------------------------------------------
   */

  /*
   * User Interface: Configurable parameters, and global program control
   */

  getUserParameters (argc, (char **const) argv);

  TM_STARTUP (THREADS);
  P_MEMORY_STARTUP (THREADS);
  SETUP_NUMBER_THREADS (THREADS);
  thread_startup (THREADS);

  double time_total = 0.0;
  int repeat = REPEATS;

  for (; repeat > 0; --repeat) {

    SDGdata = (graphSDG *) malloc (sizeof (graphSDG));
    assert (SDGdata);

    genScalData_seq (SDGdata);

    G = (graph *) malloc (sizeof (graph));
    assert (G);

    computeGraph_arg_t computeGraphArgs;

    computeGraphArgs.GPtr = G;
    computeGraphArgs.SDGdataPtr = SDGdata;

    TIMER_T start;

    TIMER_READ (start);

    GOTO_SIM ();
    thread_start (computeGraph, (void *) &computeGraphArgs);
    GOTO_REAL ();
    TIMER_T stop;

    TIMER_READ (stop);
    double time_tmp = TIMER_DIFF_SECONDS (start, stop);

    printf ("%lf\n", time_tmp);
    PRINT_STATS ();
    time_total += time_tmp;
  }

  totalTime += time_total;


#ifdef ENABLE_KERNEL2

  /* -------------------------------------------------------------------------
   * Kernel 2 - Find Max weight and sought string
   * -------------------------------------------------------------------------
   */

  printf ("\nKernel 2 - getStartLists() beginning execution...\n");

  maxIntWtListSize = 0;
  soughtStrWtListSize = 0;
  maxIntWtList = (edge *) malloc (sizeof (edge));
  assert (maxIntWtList);
  soughtStrWtList = (edge *) malloc (sizeof (edge));
  assert (soughtStrWtList);

  getStartLists_arg_t getStartListsArg;

  getStartListsArg.GPtr = G;
  getStartListsArg.maxIntWtListPtr = &maxIntWtList;
  getStartListsArg.maxIntWtListSize = &maxIntWtListSize;
  getStartListsArg.soughtStrWtListPtr = &soughtStrWtList;
  getStartListsArg.soughtStrWtListSize = &soughtStrWtListSize;

  TIMER_READ (start);

  GOTO_SIM ();
#  ifdef OTM
#    pragma omp parallel
  {
    getStartLists ((void *) &getStartListsArg);
  }
#  else
  thread_start (getStartLists, (void *) &getStartListsArg);
#  endif
  GOTO_REAL ();

  TIMER_T stop;

  TIMER_READ (stop);

  time = TIMER_DIFF_SECONDS (start, stop);
  totalTime += time;

  printf ("\n\tgetStartLists() completed execution.\n");
  printf ("\nTime taken for kernel 2 is %9.6f sec.\n\n", time);

#endif /* ENABLE_KERNEL2 */

#ifdef ENABLE_KERNEL3

  /* -------------------------------------------------------------------------
   * Kernel 3 - Graph Extraction
   * -------------------------------------------------------------------------
   */

  printf ("\nKernel 3 - findSubGraphs() beginning execution...\n");

  if (K3_DS == 0) {

    intWtVList =
      (V *) malloc (G->numVertices * maxIntWtListSize * sizeof (V));
    assert (intWtVList);
    strWtVList =
      (V *) malloc (G->numVertices * soughtStrWtListSize * sizeof (V));
    assert (strWtVList);

    findSubGraphs0_arg_t findSubGraphs0Arg;

    findSubGraphs0Arg.GPtr = G;
    findSubGraphs0Arg.intWtVList = intWtVList;
    findSubGraphs0Arg.strWtVList = strWtVList;
    findSubGraphs0Arg.maxIntWtList = maxIntWtList;
    findSubGraphs0Arg.maxIntWtListSize = maxIntWtListSize;
    findSubGraphs0Arg.soughtStrWtList = soughtStrWtList;
    findSubGraphs0Arg.soughtStrWtListSize = soughtStrWtListSize;

    TIMER_READ (start);

    GOTO_SIM ();
#  ifdef OTM
#    pragma omp parallel
    {
      findSubGraphs0 ((void *) &findSubGraphs0Arg);
    }
#  else
    thread_start (findSubGraphs0, (void *) &findSubGraphs0Arg);
#  endif
    GOTO_REAL ();

    TIMER_READ (stop);

  }
  else if (K3_DS == 1) {

    intWtVLList = (Vl **) malloc (maxIntWtListSize * sizeof (Vl *));
    assert (intWtVLList);
    strWtVLList = (Vl **) malloc (soughtStrWtListSize * sizeof (Vl *));
    assert (strWtVLList);

    findSubGraphs1_arg_t findSubGraphs1Arg;

    findSubGraphs1Arg.GPtr = G;
    findSubGraphs1Arg.intWtVLList = intWtVLList;
    findSubGraphs1Arg.strWtVLList = strWtVLList;
    findSubGraphs1Arg.maxIntWtList = maxIntWtList;
    findSubGraphs1Arg.maxIntWtListSize = maxIntWtListSize;
    findSubGraphs1Arg.soughtStrWtList = soughtStrWtList;
    findSubGraphs1Arg.soughtStrWtListSize = soughtStrWtListSize;

    TIMER_READ (start);

    GOTO_SIM ();
#  ifdef OTM
#    pragma omp parallel
    {
      findSubGraphs1 ((void *) &findSubGraphs1Arg);
    }
#  else
    thread_start (findSubGraphs1, (void *) &findSubGraphs1Arg);
#  endif
    GOTO_REAL ();

    TIMER_READ (stop);

    /*  Verification
       on_one_thread {
       for (i=0; i<maxIntWtListSize; i++) {
       printf("%ld -- ", i);
       currV = intWtVLList[i];
       while (currV != NULL) {
       printf("[%ld %ld] ", currV->num, currV->depth);
       currV = currV->next;
       }
       printf("\n");
       }

       for (i=0; i<soughtStrWtListSize; i++) {
       printf("%ld -- ", i);
       currV = strWtVLList[i];
       while (currV != NULL) {
       printf("[%ld %ld] ", currV->num, currV->depth);
       currV = currV->next;
       }
       printf("\n");
       }

       }
     */

  }
  else if (K3_DS == 2) {

    intWtVDList = (Vd *) malloc (maxIntWtListSize * sizeof (Vd));
    assert (intWtVDList);
    strWtVDList = (Vd *) malloc (soughtStrWtListSize * sizeof (Vd));
    assert (strWtVDList);

    findSubGraphs2_arg_t findSubGraphs2Arg;

    findSubGraphs2Arg.GPtr = G;
    findSubGraphs2Arg.intWtVDList = intWtVDList;
    findSubGraphs2Arg.strWtVDList = strWtVDList;
    findSubGraphs2Arg.maxIntWtList = maxIntWtList;
    findSubGraphs2Arg.maxIntWtListSize = maxIntWtListSize;
    findSubGraphs2Arg.soughtStrWtList = soughtStrWtList;
    findSubGraphs2Arg.soughtStrWtListSize = soughtStrWtListSize;

    TIMER_READ (start);

    GOTO_SIM ();
#  ifdef OTM
#    pragma omp parallel
    {
      findSubGraphs2 ((void *) &findSubGraphs2Arg);
    }
#  else
    thread_start (findSubGraphs2, (void *) &findSubGraphs2Arg);
#  endif
    GOTO_REAL ();

    TIMER_READ (stop);

    /* Verification */
    /*
       on_one_thread {
       printf("\nInt weight sub-graphs \n");
       for (i=0; i<maxIntWtListSize; i++) {
       printf("%ld -- ", i);
       for (j=0; j<intWtVDList[i].numArrays; j++) {
       printf("\n [Array %ld] - \n", j);
       for (k=0; k<intWtVDList[i].arraySize[j]; k++) {
       printf("[%ld %ld] ", intWtVDList[i].vList[j][k].num, intWtVDList[i].vList[j][k].depth);
       }

       }
       printf("\n");
       }

       printf("\nStr weight sub-graphs \n");
       for (i=0; i<soughtStrWtListSize; i++) {
       printf("%ld -- ", i);
       for (j=0; j<strWtVDList[i].numArrays; j++) {
       printf("\n [Array %ld] - \n", j);
       for (k=0; k<strWtVDList[i].arraySize[j]; k++) {
       printf("[%ld %ld] ", strWtVDList[i].vList[j][k].num, strWtVDList[i].vList[j][k].depth);
       }

       }
       printf("\n");
       }

       }
     */

  }
  else {

    assert (0);

  }

  time = TIMER_DIFF_SECONDS (start, stop);
  totalTime += time;

  printf ("\n\tfindSubGraphs() completed execution.\n");
  printf ("\nTime taken for kernel 3 is %9.6f sec.\n\n", time);

#endif /* ENABLE_KERNEL3 */

#ifdef ENABLE_KERNEL4

  /* -------------------------------------------------------------------------
   * Kernel 4 - Graph Clustering
   * -------------------------------------------------------------------------
   */

  printf ("\nKernel 4 - cutClusters() beginning execution...\n");

  TIMER_READ (start);

  GOTO_SIM ();
#  ifdef OTM
#    pragma omp parallel
  {
    cutClusters ((void *) G);
  }
#  else
  thread_start (cutClusters, (void *) G);
#  endif
  GOTO_REAL ();

  TIMER_READ (stop);

  time = TIMER_DIFF_SECONDS (start, stop);
  totalTime += time;

  printf ("\n\tcutClusters() completed execution.\n");
  printf ("\nTime taken for Kernel 4 is %9.6f sec.\n\n", time);

#endif /* ENABLE_KERNEL4 */

  printf ("\nTime taken for all is %9.6f sec.\n\n", totalTime);

  /* -------------------------------------------------------------------------
   * Cleanup
   * -------------------------------------------------------------------------
   */

  P_FREE (G->outDegree);
  P_FREE (G->outVertexIndex);
  P_FREE (G->outVertexList);
  P_FREE (G->paralEdgeIndex);
  P_FREE (G->inDegree);
  P_FREE (G->inVertexIndex);
  P_FREE (G->inVertexList);
  P_FREE (G->intWeight);
  P_FREE (G->strWeight);

#ifdef ENABLE_KERNEL3

  LONGINT_T i;
  LONGINT_T j;
  Vl *currV;
  Vl *tempV;

  if (K3_DS == 0) {
    P_FREE (strWtVList);
    P_FREE (intWtVList);
  }

  if (K3_DS == 1) {
    for (i = 0; i < maxIntWtListSize; i++) {
      currV = intWtVLList[i];
      while (currV != NULL) {
        tempV = currV->next;
        P_FREE (currV);
        currV = tempV;
      }
    }
    for (i = 0; i < soughtStrWtListSize; i++) {
      currV = strWtVLList[i];
      while (currV != NULL) {
        tempV = currV->next;
        P_FREE (currV);
        currV = tempV;
      }
    }
    P_FREE (strWtVLList);
    P_FREE (intWtVLList);
  }

  if (K3_DS == 2) {
    for (i = 0; i < maxIntWtListSize; i++) {
      for (j = 0; j < intWtVDList[i].numArrays; j++) {
        P_FREE (intWtVDList[i].vList[j]);
      }
      P_FREE (intWtVDList[i].vList);
      P_FREE (intWtVDList[i].arraySize);
    }
    for (i = 0; i < soughtStrWtListSize; i++) {
      for (j = 0; j < strWtVDList[i].numArrays; j++) {
        P_FREE (strWtVDList[i].vList[j]);
      }
      P_FREE (strWtVDList[i].vList);
      P_FREE (strWtVDList[i].arraySize);
    }
    P_FREE (strWtVDList);
    P_FREE (intWtVDList);
  }

  P_FREE (soughtStrWtList);
  P_FREE (maxIntWtList);

#endif /* ENABLE_KERNEL2 */

  P_FREE (SOUGHT_STRING);
  P_FREE (G);
  P_FREE (SDGdata);

  TM_SHUTDOWN ();
  P_MEMORY_SHUTDOWN ();

  GOTO_SIM ();

  thread_shutdown ();

  MAIN_RETURN (0);
}


/* =============================================================================
 *
 * End of ssca2.c
 *
 * =============================================================================
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "thread.h"
#include "timer.h"

#include "tm.h"

#ifndef FILE_NAME
#define FILE_NAME "memory_stats"
#endif

// ########################## defines

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

#define ALIGNED __attribute__((aligned(CACHE_LINE_SIZE)))
#define GRANULE_T intptr_t

#ifdef CRAFTY
#define NH_alloc(size) crafty_malloc(0, size)
#define NH_free(pool)  crafty_free(pool)
#else
#define NH_alloc(size) malloc(size)
#define NH_free(pool)  free(pool)
#endif


// pointers to persistent memory
static int *x;
static int *y;
static int *z;

#define WRITE(var, val) TM_SHR_WR(var[0], val)
#define EXPECT(var, val) assert(TM_SHARED_READ(var[0]) == val)

static void memory() {
    TM_THREAD_ENTER();


    // Empty transaction
    TM_BEGIN_EXT(0, 0);
    TM_END();

// Allocate and immediately deallocate
    TM_BEGIN();
    x = NH_alloc(10 * sizeof(int));
    WRITE(x, 1);
    NH_free(x);
    TM_END();

// Allocate in first transaction, deallocate in the next
    TM_BEGIN();
    x = NH_alloc(10 * sizeof(int));
    WRITE(x, 1);
    TM_END();
    TM_BEGIN();
    EXPECT(x, 1);
    WRITE(x, 2);
    NH_free(x);
    TM_END();

// Mix allocations and deallocations over 3 transactions
    TM_BEGIN();
    x = NH_alloc(10 * sizeof(int));
    y = NH_alloc(10 * sizeof(int));
    WRITE(x, 1);
    WRITE(y, 2);
    TM_END();
    TM_BEGIN();
    EXPECT(x, 1);
    EXPECT(y, 2);
    WRITE(x, 11);
    NH_free(x);
    z = NH_alloc(30 * sizeof(int));
    WRITE(z, 3);
    TM_END();
    TM_BEGIN();
    EXPECT(y, 2);
    EXPECT(z, 3);
    WRITE(y, 1);
    WRITE(z, 1);
    NH_free(z);
    NH_free(y);
    TM_END();

    // Allocate and free memory inside an SGL transaction
    TM_BEGIN();
    x = NH_alloc(10 * sizeof(int));
    WRITE(x, 1);
    TM_END();
    long *ptr = malloc(sizeof(*ptr) * 1024);
    TM_BEGIN();
    EXPECT(x, 1);
    TM_SHARED_WRITE(ptr[0], 0);
        int i = 0;
        for (; i < 1024; i++) {
            TM_SHARED_WRITE(ptr[i], i);
            if (i == 10) y = NH_alloc(10 * sizeof(int));
            if (i == 15) NH_free(x);
            if (i == 1000) z = NH_alloc(20 * sizeof(int));
        }
    WRITE(y, 2);
    WRITE(z, 3);
    TM_END();
    TM_BEGIN();
    EXPECT(y, 2);
    EXPECT(z, 3);
    WRITE(y, 22);
    WRITE(z, 33);
    NH_free(y);
    NH_free(z);
    TM_END();

    TM_THREAD_EXIT();
}


MAIN(argc, argv)
{
int nb_threads = 1;
SIM_GET_NUM_CPU(nb_threads);
TM_STARTUP(nb_threads);
P_MEMORY_STARTUP(nb_threads);
thread_startup(nb_threads);

GOTO_SIM();

thread_start(memory, NULL);

GOTO_REAL();
printf("Success!\n");

TM_SHUTDOWN();
P_MEMORY_SHUTDOWN();
thread_shutdown();

MAIN_RETURN(EXIT_SUCCESS);
}

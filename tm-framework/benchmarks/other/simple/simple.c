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
#define FILE_NAME "simple_stats"
#endif

#define BIG_TRANSACTION_SIZE 1024000

#define EXPECT(var, val) assert(TM_SHARED_READ(var) == val)

// ########################## defines

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

#define ALIGNED __attribute__((aligned(CACHE_LINE_SIZE)))
#define GRANULE_T intptr_t


// persistent memory
static int x;
static int y;


static void simple() {
    TM_THREAD_ENTER();


    // Empty transaction
    TM_BEGIN_EXT(0, 0);
    TM_END();

// TX with 1 write
    TM_BEGIN();
    TM_SHR_WR(x, 10);
    TM_END();

// TX with 2 writes
    TM_BEGIN();
    TM_SHR_WR(x, 20);
    TM_SHR_WR(x, 30);
    TM_END();

// TX with 3 writes
    TM_BEGIN();
    TM_SHR_WR(x, 40);
    TM_SHR_WR(x, 50);
    TM_SHR_WR(x, 60);
    TM_END();

// TX with 4 writes
    TM_BEGIN();
    TM_SHR_WR(x, 70);
    TM_SHR_WR(x, 80);
    TM_SHR_WR(x, 90);
    TM_SHR_WR(x, 100);
    TM_END();

// TX with 5 writes
    TM_BEGIN();
    TM_SHR_WR(x, 110);
    TM_SHR_WR(x, 120);
    TM_SHR_WR(x, 130);
    TM_SHR_WR(x, 140);
    TM_SHR_WR(x, 150);
    TM_END();

    // Value checks
    TM_BEGIN();
    TM_SHR_WR(x, 100);
    TM_SHR_WR(y, 200);
    TM_END();
    EXPECT(x, 100);
    EXPECT(y, 200);
    TM_BEGIN();
    TM_SHR_WR(y, 2000);
    TM_SHR_WR(x, 1000);
    TM_END();
    EXPECT(x, 1000);
    EXPECT(y, 2000);


    // Nested transactions
    TM_BEGIN();
    TM_SHR_WR(x, 1);
    TM_BEGIN();
    TM_SHR_WR(y, 2);
    TM_END();
    TM_END();
    EXPECT(x, 1);
    EXPECT(y, 2);
    //
    TM_BEGIN();
    TM_BEGIN();
    TM_SHR_WR(x, 11);
    TM_SHR_WR(y, 22);
    TM_END();
    TM_END();
    EXPECT(x, 11);
    EXPECT(y, 22);
    //
    TM_BEGIN();
    TM_BEGIN();
    TM_SHR_WR(y, 32);
    TM_END();
    TM_SHR_WR(x, 31);
    TM_END();
    EXPECT(x, 31);
    EXPECT(y, 32);

    long *ptr = malloc(sizeof(*ptr) * BIG_TRANSACTION_SIZE);
    // Fails due to cache failures?
    TM_BEGIN();
    TM_SHR_WR(ptr[0], 0);
        int i = 0;
        for (; i < BIG_TRANSACTION_SIZE; i++) {
            TM_SHR_WR(ptr[i], i);
        }
    TM_END();

    // Fails due to capacity?
    TM_BEGIN();
    TM_SHR_WR(ptr[0], 0);
    i = 0;
    for (; i < BIG_TRANSACTION_SIZE; i++) {
        TM_SHR_WR(ptr[i], i);
    }
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

thread_start(simple, NULL);

GOTO_REAL();
printf("Success!\n");

TM_SHUTDOWN();
P_MEMORY_SHUTDOWN();
thread_shutdown();

MAIN_RETURN(EXIT_SUCCESS);
}

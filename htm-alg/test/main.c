#include "htm_retry_template.h"

#include <stdio.h>
#include <stdlib.h>

#define PRINT_STATS() \
    printf("Nb. aborts = %i, Nb. commits = %i, Nb. fall-backs = %i\n", \
           HTM_get_status_count(ABORT, NULL), \
           HTM_get_status_count(SUCCESS, NULL), \
           HTM_get_status_count(FALLBACK, NULL)); HTM_reset_status_count()

static void TM_begin() {
    HTM_SGL_begin();
}

static void TM_commit() {
    HTM_SGL_commit();
}

/*
 * 
 */
int main(int argc, char** argv)
{
    int i = 0;
    int *pool;
    
    HTM_init(1);
    HTM_thr_init();
    
    HTM_set_is_record(1);
    
    HTM_SGL_begin();
    // always abort
    pool = (int*)malloc(512 * CACHE_LINE_SIZE);
    for (i = 0; i < 512 * (CACHE_LINE_SIZE/sizeof(int)); ++i) {
        *pool = i;
    }
    free(pool);
    HTM_SGL_commit();
    
    PRINT_STATS();
    
    i = 0;
    
    HTM_SGL_begin();
    i = 1; // always commit
    HTM_SGL_commit();
    
    PRINT_STATS();
    
    TM_begin();
    i = 1; // always commit
    TM_commit();
    
    PRINT_STATS();
    
    TM_begin();
    // always abort
    fprintf(stdout, "Hello world!\n");
    fflush(stdout);
    TM_commit();
    
    PRINT_STATS();
    
    HTM_thr_exit();
    HTM_exit();
    
    return (EXIT_SUCCESS);
}


#include <assert.h>
#include <stdlib.h>
#include "tm.h"
#include "list.h"
#include "queue.h"
#include "heap.h"
#include "hashtable.h"
#include "rbtree.h"

#define NB_THREADS 8

#define NB_ELEMENTS 1024
#define NB_ATTEMPTS 10000

static list_t* listPtr;
static queue_t* queuePtr;
static heap_t* heapPtr;
static hashtable_t* hashmapPtr;
static rbtree_t* rbtreePtr;
static __thread seed;

static int
elem_cmp(void *a, void *b)
{
    int a_ = (int)a;
    int b_ = (int)b;

    return a_ - b_;
}

static int
elem_hash(void *a)
{
    int a_ = (int)a;

    return a_;
}

static void*
test_list (void *arg)
{
    TM_THREAD_ENTER();

    seed = (int)arg;

    int i;
    for (i = 0; i < NB_ATTEMPTS; ++i) {
        int elem = rand_r(&seed) % NB_ELEMENTS, is_ins = rand_r(&seed) % 2;
        int find;

        TM_BEGIN();
        find = TMLIST_FIND(listPtr, (void*)elem);
        if (is_ins || !find) {
            TMLIST_INSERT(listPtr, (void*)elem);
        } else {
            TMLIST_REMOVE(listPtr, (void*)elem);
        }
        TM_END();
    }

    TM_THREAD_EXIT();
    return NULL;
}

static void*
test_queue (void *arg)
{
    TM_THREAD_ENTER();

    seed = (int)arg;

    int i;
    for (i = 0; i < NB_ATTEMPTS; ++i) {
        int elem = rand_r(&seed) % NB_ELEMENTS, is_ins = rand_r(&seed) % 2;
        TM_BEGIN();
        if (is_ins || TMQUEUE_ISEMPTY(queuePtr)) {
            TMQUEUE_PUSH(queuePtr, (void*)elem);
        } else {
            TMQUEUE_POP(queuePtr);
        }
        TM_END();
    }

    TM_THREAD_EXIT();
    return NULL;
}

static void*
test_hashmap (void *arg)
{
    TM_THREAD_ENTER();
    int contains;

    seed = (int)arg;

    int i;
    for (i = 0; i < NB_ATTEMPTS; ++i) {
        int elem = rand_r(&seed) % NB_ELEMENTS, is_ins = rand_r(&seed) % 2;
        TM_BEGIN();
        contains = TMHASHTABLE_FIND(hashmapPtr, elem);
        if (!contains || (!contains && !is_ins)) {
            TMHASHTABLE_INSERT(hashmapPtr, (void*)elem, (void*)elem);
        } else if (contains) {
            hashtable_remove(hashmapPtr, (void*)elem);
        }
        TM_END();
    }

    TM_THREAD_EXIT();
    return NULL;
}

static void*
test_rbtree (void *arg)
{
    TM_THREAD_ENTER();
    int contains;

    seed = (int)arg;

    int i;
    for (i = 0; i < NB_ATTEMPTS; ++i) {
        int elem = rand_r(&seed) % NB_ELEMENTS, is_ins = rand_r(&seed) % 2;
        TM_BEGIN();
        contains = TMRBTREE_CONTAINS(rbtreePtr, elem);
        if (!contains || (!contains && !is_ins)) {
            TMRBTREE_INSERT(rbtreePtr, (void*)elem, (void*)elem);
        } else if (contains) {
            TMRBTREE_DELETE(rbtreePtr, (void*)elem);
        }
        TM_END();
    }

    TM_THREAD_EXIT();
    return NULL;
}

static void*
test_heap (void *arg)
{
    TM_THREAD_ENTER();

    seed = (int)arg;

    int i;
    for (i = 0; i < NB_ATTEMPTS; ++i) {
        int elem = rand_r(&seed) % NB_ELEMENTS, is_ins = rand_r(&seed) % 2;
        TM_BEGIN();
        if (is_ins) {
            TMHEAP_INSERT(heapPtr, (void*)elem);
        } else {
            TMHEAP_REMOVE(heapPtr);
        }
        TM_END();
    }

    TM_THREAD_EXIT();
    return NULL;
}

int
main ()
{
    int nb_tests = 5;
    void* (*tests[5])(void*) = {
        test_queue,
        test_heap,
        test_list,
        test_hashmap,
        test_rbtree,
    };
    const char *tests_str[5] = {
        "queue",
        "heap",
        "list",
        "hashtable",
        "rbtree",
    };
    int i;

    GOTO_REAL();

    TM_STARTUP(NB_THREADS);
    P_MEMORY_STARTUP(NB_THREADS);
    thread_startup(NB_THREADS);

    hashmapPtr = hashtable_alloc(1, elem_hash, elem_cmp, 2, 2);
    rbtreePtr  = rbtree_alloc(elem_cmp);
    heapPtr    = heap_alloc(NB_ELEMENTS, elem_cmp);
    queuePtr   = queue_alloc(NB_ELEMENTS);
    listPtr    = list_alloc(elem_cmp);

    for (i = 0; i < NB_ELEMENTS; ++i) {
        list_insert(listPtr, (void*)i);
        heap_insert(heapPtr, (void*)i);
        rbtree_insert(rbtreePtr, (void*)i, (void*)i);
        hashtable_insert(hashmapPtr, (void*)i, (void*)i);
        queue_push(queuePtr, (void*)i);
    }

    GOTO_SIM();
    for (i = 0; i < nb_tests; ++i) {
#ifdef OTM
#pragma omp parallel
        {
            tests[i]((void*)(1234 << i));
        }
#else
        thread_start(tests[i], (void*)(1234 << i));
#endif
        printf("Test %s complete!\n", tests_str[i]);
    }
    GOTO_REAL();

    TM_SHUTDOWN();
    P_MEMORY_SHUTDOWN();

    GOTO_SIM();

    thread_shutdown();

    return EXIT_SUCCESS;
}

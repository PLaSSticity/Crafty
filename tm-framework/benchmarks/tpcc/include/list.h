#ifndef TPCC_LIST_H
#define TPCC_LIST_H 1

#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct list_node {
    void* dataPtr;
    struct list_node* nextPtr;
} list_node_t;

typedef list_node_t* list_iter_t;

typedef struct list {
    list_node_t head;
    long (*compare)(const void*, const void*);   /* returns {-1,0,1}, 0 -> equal */
    long size;
} list_t;


void list_iter_reset (list_iter_t* itPtr, list_t* listPtr);

bool_t list_iter_hasNext (list_iter_t* itPtr, list_t* listPtr);

void* list_iter_next (list_iter_t* itPtr, list_t* listPtr);

list_t* list_alloc (long (*compare)(const void*, const void*));

void list_free (list_t* listPtr);

bool_t list_isEmpty (list_t* listPtr);

long list_getSize (list_t* listPtr);

void* list_find (list_t* listPtr, void* dataPtr);

bool_t list_insert (list_t* listPtr, void* dataPtr);

bool_t list_remove (list_t* listPtr, void* dataPtr);

void list_clear (list_t* listPtr);


#define PLIST_ALLOC(cmp)                Plist_alloc(cmp)
#define PLIST_FREE(list)                Plist_free(list)
#define PLIST_GETSIZE(list)             list_getSize(list)
#define PLIST_INSERT(list, data)        Plist_insert(list, data)
#define PLIST_REMOVE(list, data)        Plist_remove(list, data)
#define PLIST_CLEAR(list)               Plist_clear(list)

#ifdef __cplusplus
}
#endif


#endif

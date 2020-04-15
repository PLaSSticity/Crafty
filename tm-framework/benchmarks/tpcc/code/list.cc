#include <stdlib.h>
#include "list.h"
#include "types.h"

static long compareDataPtrAddresses (const void* a, const void* b) {
    return ((long)a - (long)b);
}

void list_iter_reset (list_iter_t* itPtr, list_t* listPtr) {
    *itPtr = &(listPtr->head);
}

bool_t list_iter_hasNext (list_iter_t* itPtr, list_t* listPtr) {
    return (((*itPtr)->nextPtr != NULL) ? TRUE : FALSE);
}

void* list_iter_next (list_iter_t* itPtr, list_t* listPtr) {
    *itPtr = (*itPtr)->nextPtr;

    return (*itPtr)->dataPtr;
}

static list_node_t* allocNode (void* dataPtr) {
    list_node_t* nodePtr = (list_node_t*)malloc(sizeof(list_node_t));
    if (nodePtr == NULL) {
        return NULL;
    }

    nodePtr->dataPtr = dataPtr;
    nodePtr->nextPtr = NULL;

    return nodePtr;
}

list_t* list_alloc (long (*compare)(const void*, const void*)) {
    list_t* listPtr = (list_t*)malloc(sizeof(list_t));
    if (listPtr == NULL) {
        return NULL;
    }

    listPtr->head.dataPtr = NULL;
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;

    if (compare == NULL) {
        listPtr->compare = &compareDataPtrAddresses; /* default */
    } else {
        listPtr->compare = compare;
    }

    return listPtr;
}

static void freeNode (list_node_t* nodePtr) {
    free(nodePtr);
}

static void freeList (list_node_t* nodePtr) {
    if (nodePtr != NULL) {
        freeList(nodePtr->nextPtr);
        freeNode(nodePtr);
    }
}

void list_free (list_t* listPtr) {
    freeList(listPtr->head.nextPtr);
    free(listPtr);
}

bool_t list_isEmpty (list_t* listPtr) {
    return (listPtr->head.nextPtr == NULL);
}

long list_getSize (list_t* listPtr) {
    return listPtr->size;
}

static list_node_t* findPrevious (list_t* listPtr, void* dataPtr) {
    list_node_t* prevPtr = &(listPtr->head);
    list_node_t* nodePtr = prevPtr->nextPtr;

    for (; nodePtr != NULL; nodePtr = nodePtr->nextPtr) {
        if (listPtr->compare(nodePtr->dataPtr, dataPtr) >= 0) {
            return prevPtr;
        }
        prevPtr = nodePtr;
    }

    return prevPtr;
}

void* list_find (list_t* listPtr, void* dataPtr)
{
    list_node_t* nodePtr;
    list_node_t* prevPtr = findPrevious(listPtr, dataPtr);

    nodePtr = prevPtr->nextPtr;

    if ((nodePtr == NULL) ||
        (listPtr->compare(nodePtr->dataPtr, dataPtr) != 0)) {
        return NULL;
    }

    return (nodePtr->dataPtr);
}

bool_t list_insert (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr;
    list_node_t* nodePtr;
    list_node_t* currPtr;

    prevPtr = findPrevious(listPtr, dataPtr);
    currPtr = prevPtr->nextPtr;

#ifdef LIST_NO_DUPLICATES
    if ((currPtr != NULL) &&
        listPtr->compare(currPtr->dataPtr, dataPtr) == 0) {
        return FALSE;
    }
#endif

    nodePtr = allocNode(dataPtr);
    if (nodePtr == NULL) {
        return FALSE;
    }

    nodePtr->nextPtr = currPtr;
    prevPtr->nextPtr = nodePtr;
    listPtr->size++;

    return TRUE;
}

bool_t list_remove (list_t* listPtr, void* dataPtr) {
    list_node_t* prevPtr;
    list_node_t* nodePtr;

    prevPtr = findPrevious(listPtr, dataPtr);

    nodePtr = prevPtr->nextPtr;
    if ((nodePtr != NULL) &&
        (listPtr->compare(nodePtr->dataPtr, dataPtr) == 0))
    {
        prevPtr->nextPtr = nodePtr->nextPtr;
        nodePtr->nextPtr = NULL;
        freeNode(nodePtr);
        listPtr->size--;
        return TRUE;
    }

    return FALSE;
}

void list_clear (list_t* listPtr) {
    freeList(listPtr->head.nextPtr);
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;
}

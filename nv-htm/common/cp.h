#ifndef CP_H_GUARD
#define CP_H_GUARD

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef struct cp_ cp_s;

cp_s* cp_init(size_t nb_item, size_t item_size);

/**
 * Writes the item in the buffer.
 */
int cp_produce(cp_s*, void* item);

/**
 * Writes in item from the buffer.
 */
int cp_consume(cp_s*, void* item);

int cp_count_items(cp_s*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CP_H_GUARD */


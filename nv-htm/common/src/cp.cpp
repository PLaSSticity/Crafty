#include "cp.h"
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define BUFFER_ADDR(ptr) \
((void*) &(((char*) buffer)[ptr * item_size]) )

#define IS_EMPTY (c_ptr == p_ptr)
#define IS_FULL (ptr_mod(c_ptr, -p_ptr, nb_items) == 1)

#define CPY_AND_INC(type, dst, src) ({ \
	type block = *((type*) src); \
	type *src2 = (type*)src, *dst2 = (type*)dst;\
	*dst2 = block; \
	src = (void*) ++src2; \
	dst = (void*) ++dst2; \
})

#define CPY_PTR(dst, src) memcpy(dst, src, item_size)

// TODO: allow more than one buffer
struct cp_ {
	int dummy;
};

static void* buffer;
static int c_ptr;
static int p_ptr;
static size_t nb_items;
static size_t item_size;

cp_s* cp_init(size_t nb_items_, size_t item_size_)
{
	nb_items = nb_items_;
	item_size = item_size_;
	buffer = malloc(nb_items * item_size);
	return NULL;
}

int cp_produce(cp_s*, void* i)
{
	void* item;
	void* place;

	if (IS_FULL) return 0;

	item = i;
	place = BUFFER_ADDR(p_ptr);

	CPY_PTR(place, item);

	p_ptr = p_ptr < nb_items ? p_ptr+1 : 0;
	__sync_synchronize(); // TODO: need this?

	return 1;
}

int cp_consume(cp_s*, void* i)
{
	void* item;
	void* place;

	if (IS_EMPTY) return 0;

	item = i;
	place = BUFFER_ADDR(c_ptr);

	CPY_PTR(item, place);

	c_ptr = ptr_mod(c_ptr, 1, nb_items);
	__sync_synchronize(); // TODO: need this?

	return 1;
}

int cp_count_items(cp_s*)
{
	return ptr_mod(p_ptr, -c_ptr, nb_items);
}

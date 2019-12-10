/**
 * The dummy implementation of tmmh.
 * To be used in transitional code, e.g. for debugging.
 * This passes all basic calls directly on to malloc etc.
 * There is no (actual) GC'ing or typing, just basic malloc support.
 */

#include <stdlib.h>
#include "tmmh.h"

void * tmmh_init(size_t memsize, pif pfs[])
{
	return NULL;
}

void * allocate(void * memory, uint32_t size, bool preserve)
{
	return malloc(size);
}

void * reallocate (void * memory, void * data, uint32_t size, bool update_references)
{
	return realloc(data, size);
}

void * release(void * memory, void * data, bool clear_references)
{
	free(data);
	return NULL;
}

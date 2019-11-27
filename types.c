#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// printf debugging only
//#include <stdio.h>

#include "tmmh_internal.h"
#include "tmmh.h"

/**
 * Standard pifs.
 */
bool pif_none(void * data, int n, void *** result)
{
	return false;
}

bool pif_ptr(void * data, int n, void *** result)
{
	if (n != 0) return false;
	void ** d = (void **) data;
	*result = d;
	return true;
}

/**
 * Types.
 */
void set_type(void * data, int type)
{
	header * h = find_header(data);
	h->type = type;
}

int get_type(void * data)
{
	header * h = find_header(data);
	return h->type;
}

uint32_t get_size(void * data) {
	header * h = find_header(data);
	return ((h->size-1) * header_size) - h->bytes_unused;
}

bool in_use(void * data) {
	header * h = find_header(data);
	return h->in_use;
}

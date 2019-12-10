#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// printf debugging only
//#include <stdio.h>

#include "tmmh_internal.h"
#include "tmmh.h"

/**
 * Visualize.
 */
void tmmh_visualize(void * memory, char * buffer)
{
	int i=0;
	header * h = first_header(memory);
	while (!is_end(memory, h))
	{
		if (h->in_use) buffer[i++] = '0' + h->type;
		else buffer[i++] = 'v';
		for (int j=1; j < h->size; j++)
			if(h->preserve)
				buffer[i++]='*';
			else
				buffer[i++]='.';

		h = next(h);
	}
	buffer[i] = 0;
}

uint64_t tmmh_memsize(void * memory) {
#ifdef TMMH_USE_END_MARKER
	header * end_marker = ((mem_header *) memory)->end_marker;
#else
	header * end_marker = first_header(memory);
	while(!is_end(memory, end_marker)) {
		end_marker = next(end_marker);
	}
#endif
	return end_marker - first_header(memory);
}

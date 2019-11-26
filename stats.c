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
void tmmh_visualize(char * buffer)
{
	int i=0;
	header * h = memory;
	while (!is_end(h))
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

uint64_t tmmh_memsize() {
	return end_marker - memory;
}

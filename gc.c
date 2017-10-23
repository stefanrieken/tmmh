#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// printf debugging only
//#include <stdio.h>

#include "tmmh_internal.h"
#include "tmmh.h"

/**
 * Garbage collection.
 */
static void clear()
{
	header * h = memory;
	while (h->size != 0)
	{
		h->gc_mark = h->preserve; // keep 'preserved' entries
		h = next(h);
	}
}

static void mark_and_follow(void * data)
{
	if (data == NULL) return;

	header * h = find_header(data);
	if (h->gc_mark) return; // do not visit twice
	h->gc_mark = true;

	pif p = pifs[h->type];
	void ** ptr = NULL;

	int i = 0;
	while (p(data, i, &ptr))
	{
		mark_and_follow(*ptr);
		i++;
	}
}

static void mark(void * roots[], int num_roots)
{
	for (int i=0; i<num_roots; i++)
		mark_and_follow(roots[i]);
}

static void sweep()
{
	header * h = memory;
	while (h->size != 0)
	{
		if (!h->gc_mark)
			release(&h[1], false); // shouldn't need to clear references

		h = next(h);
	}
}

void tmmh_gc(void * roots[], int num_roots)
{
	clear();
	mark(roots, num_roots);
	sweep();
}

void tmmh_compact(void ** stack_ptrs[], int num_ptrs)
{
	header * h = memory;
	while (h->size != 0)
	{
		if (!h->in_use)
		{
			header * next_h = next(h);

			if (!next_h->in_use && next_h->size != 0)
			{
				// merge the two
				h->size += next_h->size;
				next_h = next(h);
			}

			if (next_h->size == 0)
			{
				// empty slot at end of heap; shorten heap size
				h->size = 0;
				return;
			}

			uint32_t gap_size = h->size;
			bool swapping_with_next = true;

			// if (next_h->in_use) ==> this is always true after the above tests

			if (next_h->preserve)
			{
				swapping_with_next = false;
				// Skip any 'preserved' slot and look for the next used slot
				// that we can swap with. Has to be of the right size then!
				// (swapping two adjacent places has no size requirements)
				do {
					next_h = next(next_h);
				} while (next_h->size != 0 && (!next_h->in_use || next_h->preserve || h->size < next_h->size));
				gap_size -= next_h->size;
			}

			if (next_h->size != 0)
			{
				// swap places
				// using memmove because it guarantees to work on overlap
				// (memcpy could work if it copied left-to-right, 1-by-1)
				memmove(h, next_h, next_h->size * header_size);
				if (gap_size > 0)
					mark_available(next(h), gap_size);
				if (!swapping_with_next) // we moved something from further away; free it
					mark_available(next_h, next_h->size);
				// update pointers
				void * old_value = &next_h[1];
				void * new_value = &h[1];
				update_pointers(old_value, new_value);
	
				for (int i=0;i<num_ptrs;i++)
					if (*stack_ptrs[i] == old_value)
						*stack_ptrs[i] = new_value;
			}
			
		}

		h = next(h);
	}
}

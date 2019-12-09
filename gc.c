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
/* void check_memory()
 {
 	header * h = memory;
 	while (!is_end(h))
 	{
		uint32_t * foo = (uint32_t *) h;
		while (foo < next(h)) {
			printf("%08x|", *foo);
			foo++;
		}
		printf("\n");
		printf("%d\n", h->type);
 		h = next(h);
 	}
	printf("Safe and sound here\n");
}*/

static void clear()
{
	header * h = memory;
	while (!is_end(h))
	{
		h->gc_mark = false;
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

/**
 * Since these are the objects accessed when calling GC,
 * they should likely be preserved!
 */
/*static void preserve_roots(void * roots[], int num_roots)
{
	for (int i=0; i<num_roots; i++) {
		if (roots[i] == NULL) continue;
		header * h = find_header(roots[i]);
		h->preserve = true;
	}
}*/

static void mark_preserved() {
	header * h = memory;
	while (!is_end(h))
	{
		if (h->in_use && h->preserve) {
			mark_and_follow(&h[1]);
		}
		h = next(h);
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
	while (!is_end(h))
	{
		if (!h->gc_mark)
			release(&h[1], false); // shouldn't need to clear references

		h = next(h);
	}
}

void tmmh_gc(void * roots[], int num_roots)
{
//	check_memory();
	clear();
//	preserve_roots(roots, num_roots);
	mark_preserved();
	mark(roots, num_roots);
	sweep();
//	check_memory();
}

void tmmh_compact(void ** stack_ptrs[], int num_ptrs)
{
	header * h = memory;
	while (!is_end(h))
	{
		if (!h->in_use)
		{
			header * next_h = next(h);

			while(!is_end(next_h) && !next_h->in_use && h->size + next_h->size <= TMMH_MAX_SIZE)
			{
				// merge the two
				h->size += next_h->size;
				next_h = next(h);
			}

			if (is_end(next_h))
			{
				// empty slot at end of heap; shorten heap size
				mark_end(h);
				return;
			}

			// if swapping two consecutive places, the 'gap' at the end
			// will be equal to the size of the open space that we swap into.
			tmmh_size_t gap_size = h->size;

			// if (next_h->in_use) ==> this is always true after the above tests

			if (next_h->preserve)
			{
				// Skip any 'preserved' slot and look for the next used slot
				// that we can swap with. Has to be of the right size then!
				// (swapping two adjacent places has no size requirements)
				do {
					next_h = next(next_h);
				} while (!is_end(next_h) && (!next_h->in_use || next_h->preserve || h->size < next_h->size));

				// now the gap at the given point is simply the diff
				// between the available space and portion we will use after the move.
				gap_size -= next_h->size;
			}

			if (!is_end(next_h))
			{
				// swap places
				// using memmove because it guarantees to work on overlap
				// (memcpy could work if it copied left-to-right, 1-by-1)
				memmove(h, next_h, next_h->size * header_size);
				//memcpy(h, next_h, next_h->size * header_size);
				if (gap_size > 0)
					mark_available(next(h), gap_size);
				if (next(h) + gap_size < next_h) // we swapped with something from further away; free it
					mark_available(next_h, next_h->size);

				// update pointers
				void * old_value = &next_h[1];
				void * new_value = &h[1];
				update_pointers(old_value, new_value);

				for (int i=0;i<num_ptrs;i++)
					if (*stack_ptrs[i] == old_value)
						*stack_ptrs[i] = new_value;
			}

/*		} else {
			if (is_end(next(h)) && h->preserve) {
				printf("Further compacting memory limited by object of type %d\n", h->type);
			}*/
		}

		h = next(h);
	}
}

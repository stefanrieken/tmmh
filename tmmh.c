#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// printf debugging only
//#include <stdio.h>

#include "tmmh_internal.h"
#include "tmmh.h"

pif * pifs;

header * memory;
#ifdef TMMH_USE_END_MARKER
header * end_marker;
#endif

/**
 * Init.
 */
void tmmh_init(uint32_t memsize, pif pfs[])
{
	pifs = pfs;
	memory = malloc(memsize);
	mark_end(memory);
}

/**
 * Main allocation functionality.
 */

/** Calculates full size in 'words' (that is, sizesof(header)), including 1 for the header */
static uint32_t calc_full_size(uint32_t size)
{
	uint32_t full_size_in_words = (size / header_size);
	if ((size % header_size) != 0) full_size_in_words++;
	return full_size_in_words+1;
}

static header * allocate_internal (uint32_t full_size_in_words, uint32_t size)
{
#if defined(TMMH_OPTIMIZE_SIZE) || !defined(TMMH_USE_END_MARKER)
	header * h = memory;
#else
	// The cost of walking is incredibly high.
	// Just appending new items by default gives a huge speedup.
	header * h = end_marker;
#endif
	while (!is_end(h))
	{
		header * next_h = next(h);
		if (!h->in_use) {
			// Glue vacant neighbours together
			while(!is_end(next_h) && !next_h->in_use) {
				h->size += next_h->size;
				next_h = next(h);
			}

			// Allocate if it fits
			if (h->size >= full_size_in_words)
			{
				uint32_t old_size = h->size;
				mark_allocated(h, full_size_in_words, size);

				// mark any unused part as new slot
				uint32_t gap = old_size - h->size;
				if (gap > 0)
					mark_available(next(h), gap);

				return h;
			}
		}

		h = next_h;
	}

	// nothing found; at end; allocate as new
	mark_allocated(h, full_size_in_words, size);

	// and mark new end
	mark_end(next(h));
	return h;
}

void * allocate(uint32_t size, bool preserve)
{
	uint32_t full_size_in_words = calc_full_size(size);
	header * h = allocate_internal(full_size_in_words, size);
	h->preserve = preserve;
	return &h[1]; // location of value
}

static void release_internal(header * h)
{
	uint32_t total_size_in_words = h->size;

	// merge with any next space
	// (incidentally, this also works well when next is the end marker)
	// We don't merge with any previous space, because the lookup takes
	// too much effort; we fix this by merging forward once more on allocate,
	// splitting the load of the effort between these two functions.
	header * next_h = next(h);
	if (is_end(next_h)) {
		//printf("Release at end of heap\n");
		mark_end(h);
	} else {
		if (!next_h->in_use && total_size_in_words + next_h->size <= TMMH_MAX_SIZE)
			total_size_in_words += next_h->size;

		mark_available(h, total_size_in_words);
	}
}

library_local void update_pointers(void * old_value, void * new_value)
{
	header * h = memory;
	while (!is_end(h))
	{
		if (h->in_use && h->size > 1) // no need if only a header
		{
			pif p = pifs[h->type];
			void ** ptr = NULL;

			int i = 0;
			while (p(&h[1], i++, &ptr)) {
				//printf("Old: %p, new: %p\n", old_value, new_value);
				if (*ptr == old_value) *ptr = new_value;
			}
		}
		h = next(h);
	}
}

// always returns null, as a service: bla = release(bla); // bla is null
void * release(void * data, bool clear_references)
{
	// Allow blindly releasing something that was never actually allocated
	// but was properly marked as NULL.
	if (data == NULL) return NULL;

	header * h = find_header(data);
	release_internal(h);
	if (clear_references) update_pointers(data, NULL);
	return NULL;
}

void * reallocate_internal (void * data, uint32_t size)
{
	uint32_t full_size_in_words = calc_full_size(size);

	header * h = find_header(data);

	if (h->size == full_size_in_words) return data; // no change
	else if (h->size > full_size_in_words)
	{
#ifdef TMMH_OPTIMIZE_SIZE
		// shrink
		uint32_t gap = h->size - full_size_in_words;
		h->size = full_size_in_words;
		mark_available(next(h), gap);
#endif
		return data;
	}
	else
	{
		header * next_h = next(h);
		if(is_end(next_h))
		{
			h->size = full_size_in_words;
			h->bytes_unused = (header_size - (size % header_size)) % header_size;
			next_h = next(h);
			mark_end(next_h);
			return data;
		}
		else if (!next_h->in_use && h->size + next_h->size >= full_size_in_words)
		{
			// grow into next
			uint32_t gap = (h->size + next_h->size) - full_size_in_words;
			h->size = full_size_in_words;
			if(gap > 0) mark_available(next(h), gap);
			return data;
		}
		else
		{
			// move
			header * new_h = allocate_internal(full_size_in_words, size);
			memcpy (new_h, h, h->size * header_size);
			new_h->size = full_size_in_words;
			new_h->bytes_unused = (header_size - (size % header_size)) % header_size;
			release_internal(h);
			return &new_h[1];
		}
	}

}

void * reallocate (void * data, uint32_t size, bool update_references)
{
	void * new_data = reallocate_internal(data, size);
	if (update_references) update_pointers(data, new_data);
	return new_data;
}

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// printf debugging only
//#include <stdio.h>

#include "tmmh.h"

#define packed __attribute__((__packed__))

#define MAX_MEM 400

typedef struct packed header {
	// flags + bytes_unused
	bool in_use : 1;
	bool gc_mark : 1;
	bool direct_value : 1;
	uint8_t user_defined_flags : 3;
	uint8_t bytes_unused : 2;

	uint8_t type : 8; // up to 256 user-definable datatypes
	union {
		uint16_t size : 16; // 4-byte aligned, so multiply by 4 #ifdef MAXIMIZE_SIZE (max=256k)
		uint16_t value : 16;  // if flagged as 'direct_value'
	};
} header;

pif * pifs = NULL;

header * memory = NULL;
uint32_t memory_size_in_words = 0; // in sizeof(header) granularity!

uint32_t header_size = sizeof(header);
uint32_t granularity = sizeof(header);

/**
 * Helper functions.
 */
static inline void mark_end(header * end_marker)
{
	end_marker->in_use = false;
	end_marker->size = 0;	
}

static inline void mark_available(header * h, uint32_t full_size_in_words)
{
	h->in_use = false;
	h->size = full_size_in_words;
}

static inline void mark_allocated(header * h, uint32_t full_size_in_words, uint32_t size)
{
	h->in_use = true;
	h->bytes_unused = full_size_in_words - header_size - size;
	h->size = full_size_in_words;
	h->type = 0;
}

/**
 * Init.
 */
void tmmh_init(pif pfs[])
{
	pifs = pfs;
	memory = malloc(MAX_MEM);
	mark_end(memory);
}

/**
 * Standard pifs.
 */
bool pif_none(void * data, int n, void ** result)
{
	return false;
}

bool pif_ptr(void * data, int n, void ** result)
{
	if (n != 0) return false;
	void ** d = (void **) data;
	*result = *d;
	return true;
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

static inline header * next(header * h)
{
	return &h[h->size];
}

static header * allocate_internal(uint32_t full_size_in_words, uint32_t size)
{
	header * h = memory;

	while (h < &memory[memory_size_in_words])
	{
		if (!h->in_use && h->size > full_size_in_words)
		{
			uint32_t old_size = h->size;
			mark_allocated(h, full_size_in_words, size);

			// mark any unused part as new slot
			uint32_t gap = old_size - h->size;
			if (gap > 0)
				mark_available(next(h), gap);

			return h; 
		}
		h = next(h);
	}

	// nothing found; at end; allocate as new
	h = next(h);
	mark_allocated(h, full_size_in_words, size);
	memory_size_in_words += full_size_in_words;
	// and mark new end
	mark_end(&memory[memory_size_in_words]);

	return h;
}

void * allocate(uint32_t size)
{
	uint32_t full_size_in_words = calc_full_size(size);
	header * h = allocate_internal(full_size_in_words, size);
	return &h[1]; // location of value
}

static header * prev(header * next_h)
{
	header * h = memory;
	if (h == next_h) return NULL; // no previous

	while (h < &memory[memory_size_in_words])
	{
		if (next(h) == next_h)
			return h;
		h = next(h);
	}

	return NULL; // should not get here
}

static inline header * find_header(void * data)
{
	return &((header *) data)[-1];
}

static void release_internal(header * h)
{
	// merge with any previous space
	header * prev_h = prev(h);
	if (prev_h != NULL && !prev_h->in_use)
	{
		mark_available(prev_h, prev_h->size+h->size);
		h = prev_h;
	}

	uint32_t total_size_in_words = h->size;

	// merge with any next space
	// (incidentally, this also works well when next is the end marker)
	header * next_h = next(h);
	if (!next_h->in_use)
		total_size_in_words += next_h->size;

	mark_available(h, total_size_in_words);
}

// always returns null, as a service: bla = release(bla); // bla is null
void * release(void * data)
{
	header * h = find_header(data);
	release_internal(h);
	return NULL;
}

void * reallocate (void * data, uint32_t size)
{
	uint32_t full_size_in_words = calc_full_size(size);

	header * h = find_header(data);

	if (h->size == full_size_in_words) return data; // no change
	else if (h->size > full_size_in_words)
	{
		// shrink
		uint32_t gap = h->size - full_size_in_words;
		h->size = full_size_in_words;
		mark_available(next(h), gap);
		return data;
	}
	else
	{
		header * next_h = next(h);
		if (!next_h->in_use && h->size + next_h->size >= full_size_in_words)
		{
			// grow into next
			uint32_t gap = (h->size + next_h->size) - full_size_in_words;
			h->size = full_size_in_words;
			mark_available(next(h), gap);
			return data;
		}
		else
		{
			// move
			header * new_h = allocate_internal(full_size_in_words, size);
			memcpy (new_h, h, h->size);
			release_internal(h);
			return &new_h[1];
		}
	}
	
}

/**
 * Visualize.
 */
void visualize(char * buffer)
{
	int i=0;
	header * h = memory;
	while (h->size != 0)
	{
		if (h->in_use) buffer[i++] = '0' + h->type;
		else buffer[i++] = 'v';
		for (int j=1; j < h->size; j++)
			buffer[i++]='.';

		h = next(h);
	}
	buffer[i] = 0;
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

/**
 * Garbage collection.
 */
static void clear()
{
	header * h = memory;
	while (h->size != 0)
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
	void * ptr = NULL;

	int i = 0;
	while (p(data, i, &ptr))
	{
		mark_and_follow(ptr);
		i++;
	}
}

static void mark(void * roots[], int num_roots)
{
	for (int i=0; i<num_roots; i++)
	{
		mark_and_follow(roots[i]);
		i++;
	}
}

static void sweep()
{
	header * h = memory;
	while (h->size != 0)
	{
		if (!h->gc_mark)
			release(&h[1]);

		h = next(h);
	}
}

void tmmh_gc(void * roots[], int num_roots)
{
	clear();
	mark(roots, num_roots);
	sweep();
}

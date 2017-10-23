#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#include "tmmh.h"

#define packed __attribute__((__packed__))
#define library_local __attribute__((visibility ("hidden")))

#define MAX_MEM 400

#define header_size sizeof(header)

#define next(h) &h[h->size]


typedef struct packed header {
	// flags + bytes_unused
	bool in_use : 1;
	bool gc_mark : 1;
	bool direct_value : 1;
	bool preserve : 1; // don't GC or move when heap compressing
	uint8_t user_defined_flags : 2;
	uint8_t bytes_unused : 2;

	uint8_t type : 8; // up to 256 user-definable datatypes
	union {
		uint16_t size : 16; // 4-byte aligned, so multiply by 4 #ifdef MAXIMIZE_SIZE (max=256k)
		uint16_t value : 16;  // if flagged as 'direct_value'
	};
} header;

extern pif * pifs;

extern header * memory;
extern uint32_t memory_size_in_words; // in sizeof(header) granularity!


/**
 * Inline helper functions.
 */
static inline void mark_end(header * end_marker)
{
	end_marker->in_use = false;
	end_marker->size = 0;
	end_marker->preserve = false;
}

static inline void mark_available(header * h, uint32_t full_size_in_words)
{
	h->in_use = false;
	h->size = full_size_in_words;
	h->preserve = false;
}

static inline void mark_allocated
(header * h, uint32_t full_size_in_words, uint32_t size)
{
	h->in_use = true;
	h->bytes_unused = (header_size - (size % header_size)) % header_size;
	h->size = full_size_in_words;
	h->type = 0;
}

static inline header * find_header(void * data)
{
	return &((header *) data)[-1];
}

/**
 * Library-local functions.
 */
extern void update_pointers(void * old_value, void * new_value);

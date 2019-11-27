#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <signal.h>

#include "tmmh.h"

#define packed __attribute__((__packed__))
#define library_local __attribute__((visibility ("hidden")))

#define header_size sizeof(header)

#define next(h) &h[h->size]

#define TMMH_HEADER_32

#ifdef TMMH_HEADER_32
typedef struct header {
	union {
		uint16_t size : 16; // 4-byte aligned, so multiply by 4 #ifdef MAXIMIZE_SIZE (max=256k)
		uint16_t value : 16;  // if flagged as 'direct_value'
	};
	// flags + bytes_unused
	bool in_use : 1;
	bool gc_mark : 1;
	bool direct_value : 1;
	bool preserve : 1; // don't GC or move when heap compressing
	uint8_t bytes_unused : 2; // 0-3 bytes
	uint8_t user_defined: 2; // user defined flags

	uint8_t type : 8; // up to 256 user-definable datatypes
} header;
#endif

#ifdef TMMH_HEADER_64
typedef struct header {
	union {
		uint32_t size : 32; // 4-byte aligned, so multiply by 4 #ifdef MAXIMIZE_SIZE (max=256k)
		uint32_t value : 32;  // if flagged as 'direct_value'
	};
	// flags + bytes_unused
	bool in_use : 1;
	bool gc_mark : 1;
	bool direct_value : 1;
	bool preserve : 1; // don't GC or move when heap compressing
	uint8_t bytes_unused : 3; // 0-7 bytes
	bool user_defined: 1; // user defined flags
	uint8_t unused; // more unused stuff to fill in the 64 bits.

	uint16_t type : 16; // up to 65536 user-definable datatypes
} header;
#endif

extern pif * pifs;

extern header * memory;
extern header * end_marker;

/**
 * Inline helper functions.
 */
static inline void mark_end(header * h)
{
/*	h->in_use = false;
	h->size = 0;
	h->preserve = false;
	h->bytes_unused = 3;*/
	end_marker = h;
}

static inline bool is_end(header * h)
{
	return h == end_marker;

/*	return h->in_use == false &&
	end_marker->size == 0 &&
	h->preserve == false &&
	h->bytes_unused == 3;*/
}

static inline void mark_available(header * h, uint32_t full_size_in_words)
{
	h->in_use = false;
	h->size = full_size_in_words;
	h->preserve = false;
	h->bytes_unused = 0;
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

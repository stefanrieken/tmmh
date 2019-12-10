#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <signal.h>

#include "tmmh.h"

#define packed __attribute__((__packed__))
#define library_local __attribute__((visibility ("hidden")))

#define header_size sizeof(header)

#define next(h) &h[h->size]

#ifdef TMMH_HEADER_16
/* 16-bit aligned header with max 8-bit inline value */
typedef struct packed header {
	union {
		uint16_t size : 9; // 2-byte aligned, so multiply by 2 (so max=1024)
		uint16_t value : 9; // if flagged as 'direct_value'
	};
	union { // note GC_MARKER == IN_USE flag to save space
		bool in_use : 1;
		bool gc_mark : 1;
		bool preserve : 1;
	};
	bool bytes_unused : 1;  // value size = node_size - bytes_unused (max 1)
	uint8_t type : 4; // up to 16 user-definable datatypes
} header;

#define tmmh_size_t uint8_t
#define TMMH_MAX_SIZE UINT8_MAX
#endif

#ifdef TMMH_HEADER_32
typedef struct packed header {
	union {
		uint16_t size : 16; // 4-byte aligned, so multiply by 4 (so max=256k)
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

#define tmmh_size_t uint16_t
#define TMMH_MAX_SIZE UINT16_MAX
#endif

#ifdef TMMH_HEADER_64
typedef struct packed header {
	union {
		uint32_t size : 32; // 8-byte aligned, so multiply by 8 (so max=32GB)
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

#define tmmh_size_t uint32_t
#define TMMH_MAX_SIZE UINT32_MAX
#endif

typedef struct mem_header {
	size_t size;
	header * end_marker;
} mem_header;

extern pif * pifs;

/**
 * Inline helper functions.
 */
static inline header * first_header(void * memory) {
	return (header*) &((mem_header *) memory)[1];
}

static inline void mark_end(mem_header * m, header * h)
{
#ifdef TMMH_USE_END_MARKER
	m->end_marker = h;
	h->size = 0; // this is needed in loops where the last item becomes the new end marker
#else
	h->in_use = false;
	h->size = 0;
	h->preserve = false;
	h->bytes_unused = 1;
#endif
}

static inline bool is_end(mem_header * m, header * h)
{
#ifdef TMMH_USE_END_MARKER
	return m->end_marker == h;
#else
	return h->in_use == false &&
	h->size == 0 &&
	h->preserve == false &&
	h->bytes_unused == 1;
#endif
}

static inline void mark_available(header * h, tmmh_size_t full_size_in_words)
{
	h->in_use = false;
	h->size = full_size_in_words;
	h->preserve = false;
	h->bytes_unused = 0;
}

static inline void mark_allocated
(header * h, tmmh_size_t full_size_in_words, tmmh_size_t size)
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
extern void update_pointers(void * memory, void * old_value, void * new_value);

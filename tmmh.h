#include <stdbool.h>
#include <stdint.h>

/* That's a pointer to a void pointer, passed by reference. */
typedef bool (* pif) (void * data, int n, void *** result);

/**
 * Call tmmh_init first.
 * memsize is the total size of memory used in the malloc-based implementation.
 * pifs may be NULL.
 * If not, it should contain a pointer identifying function (pif) for each type number in use.
 */
void tmmh_init(uint32_t memsize, pif pifs[]);

/**
 * Standard pifs for data containing no pointers at all, resp. data of type 'pointer'.
 */
extern bool pif_none(void * data, int n, void *** result);
extern bool pif_ptr(void * data, int n, void *** result);

/**
 * The core allocation functions
 */
// preserve == don't GC or move when heap compressing
// use whenever anything on stack requires survival of GC or heap compaction
// but don't expect the heap compactor to work optimal around permenanent values
extern void * allocate(uint32_t size, bool preserve);
extern void * release(void * data, bool clear_references);
extern void * reallocate (void * data, uint32_t size, bool update_references);

/** Visualize memory into buffer. Buffer should be at least memory size in 'words'+1 */
extern void tmmh_visualize(char * buffer);

/**
 * Types.
 */
extern void set_type(void * data, int type);
extern int get_type(void * data);

/**
 * Garbage collection.
 *
 * Everything that can be reached from the roots will be 'saved' from GC.
 */
void tmmh_gc(void * roots[], int num_roots);

/**
 * Heap compaction.
 *
 * Updates pointers in heap; updates any pointers in stack that are given as arguments.
 */
void tmmh_compact(void ** stack_ptrs[], int num_prts);

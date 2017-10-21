#include <stdbool.h>

/* That's a pointer to a void pointer, passed by reference. */
typedef bool (* pif) (void * data, int n, void *** result);

/**
 * Call tmmh_init first.
 * fips may be NULL.
 * If not, it should contain a pointer identifying function (pif) for each type number in use.
 */
void tmmh_init(pif pifs[]);

/**
 * Standard pifs for data containing no pointers at all, resp. data of type 'pointer'.
 */
extern bool pif_none(void * data, int n, void *** result);
extern bool pif_ptr(void * data, int n, void *** result);

/**
 * The core allocation functions
 */
extern void * allocate(uint32_t size, bool preserve_when_gc);
extern void * release(void * data, bool clear_references);
extern void * reallocate (void * data, uint32_t size, bool update_references);

/** Visualize memory into buffer. Buffer should be at least memory size in 'words'+1 */
extern void visualize(char * buffer);

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

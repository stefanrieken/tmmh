#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tmmh.h"

void * memory;

static void assert(char * expected, char * explain)
{
	char buffer[255];
	tmmh_visualize(memory, buffer);
	printf("%-30s: %s", explain, buffer);
	if (strcmp(expected,buffer) != 0) printf("E!");
	printf("\n");
}

int main (int argc, char ** argv)
{
	pif pifs[] = {
		pif_none,
		pif_ptr
	};

	memory = tmmh_init(400, pifs);

	printf("Running test scenarios.\nLegend:\n");
	printf("- numbers are type numbers\n");
	printf("- 'v' is a vacant slot\n");
	printf("- '.' is one word of relocatable data\n");
	printf("- '*' is one word of preserved data\n");
	printf("- 'E!' is a test assert failure\n\n");

	void * val1 = allocate(memory, 7, false);
	void * val2 = allocate(memory, 6, false);
	assert("0..0..", "allocate two untyped slots");
	val2 = reallocate(memory, val2, 12, false);
	assert("0..0...", "resize slot 2");
	void ** val3 = (void **) allocate(memory, 8, false); // allows for a max 64-bit pointer to be stored
	// (which keeps this test stable)

	set_type(val3, 1); // pointer

	assert("0..0...1..", "allocate third and set type");
	val2 = release(memory, val2, false);
	assert("0..v...1..", "release second");
	val1 = reallocate(memory, val1, 9, false); // should grow into next
	assert("0...v..1..", "grow into next vacant");
	void * val4 = allocate(memory, 4, false); // should take space from old val2
	assert("0...0.v1..", "allocate in vacant slot");

	*val3 = val4; // point to val4;

	void * val4_2 = reallocate(memory, val4, 4, false); // should be unchanged
	if (val4_2 != val4) printf("E!\n");
	assert("0...0.v1..", "reallocate with same lenth");
	val4_2 = reallocate(memory, val4_2, 13, true); // should move
	assert("0...v..1..0....", "force a move by growing");

	if (*val3 != val4_2) printf("E! Pointer did not relocate\n");

	tmmh_gc(memory, (void *) &val3, 1); // should preserve val3 and the pointed-to val4_2
	assert("v......1..0....", "GC preserving from val3");

	void * val5 = allocate(memory, 25, false);
	assert("v......1..0....0.......", "allocate another large one");

	release(memory, val4_2, true);
	if (*val3 != NULL) printf("E! Pointer did not invalidate\n");
	assert("v......1..v....0.......", "release pointer to same object");

	*val3 = val5;
	void * old_val5 = val5;
	void * val6 = NULL;
	void * val7 = NULL;
	void * val8 = NULL;
	void * val9 = NULL;

	void ** stack_ptrs[] = {&val1, &val2, (void **) &val3, &val4, &val4_2, &val5, &val6, &val7, &val8, &val9};
	tmmh_compact(memory, stack_ptrs, 9);
	assert("1..0.......", "compact");
	if (*val3 != val5) printf("E! Pointer did not relocate\n");
	if (val5 == old_val5) printf("E! Pointer did not move\n");

	val6 = allocate(memory, 2, true); // permanent
	assert("1..0.......0*", "allocate permanent");
	val7 = allocate(memory, 4, false); // not permanent
	assert("1..0.......0*0.", "allocate not permanent");
	val8 = allocate(memory, 12, false); // not permanent
	assert("1..0.......0*0.0...", "and another");
	val9 = allocate(memory, 13, false); // not permanent
	assert("1..0.......0*0.0...0....", "and another");

	void * roots[] = {val5, val7, val9};
	tmmh_gc(memory, roots, 3);
	assert("v..0.......0*0.v...0....", "gc using 3 roots");
	tmmh_compact(memory, stack_ptrs, 7);
	assert("0.......0.v0*0....", "compact retaining permanent"); // notice val7 jumped over permanent val6
}

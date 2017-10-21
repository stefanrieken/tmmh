#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tmmh.h"

static void assert(char * expected)
{
	char buffer[255];
	visualize(buffer);
	printf(buffer);
	if (strcmp(expected,buffer) != 0) printf("E!");
	printf("\n");
}

int main (int argc, char ** argv)
{
	pif pifs[] = {
		pif_none,
		pif_ptr
	};

	tmmh_init(pifs);

	printf("Running test scenarios.\nLegend:\n");
	printf("- numbers are type numbers\n");
	printf("- 'v' is a vacant slot\n");
	printf("- '.' is one word of data\n");
	printf("- 'E!' is a test assert failure\n\n");

	void * val1 = allocate(7, false);
	void * val2 = allocate(12, false);
	void ** val3 = (void **) allocate(8, false); // allows for a max 64-bit pointer to be stored 
	// (which keeps this test stable)

	set_type(val3, 1); // pointer

	assert("0..0...1..");
	val2 = release(val2, false);
	assert("0..v...1..");
	val1 = reallocate(val1, 9, false); // should grow into next
	assert("0...v..1..");
	void * val4 = allocate(4, false); // should take space from old val2
	assert("0...0.v1..");

	*val3 = val4; // point to val4;

	void * val4_2 = reallocate(val4, 4, false); // should be unchanged
	if (val4_2 != val4) printf("E!\n");
	assert("0...0.v1..");
	val4_2 = reallocate(val4_2, 13, true); // should move
	assert("0...v..1..0....");

	if (*val3 != val4_2) printf("E! Pointer did not relocate\n");

	tmmh_gc((void *) &val3, 1); // should preserve val3 and the pointed-to val4_2
	assert("v......1..0....");
}

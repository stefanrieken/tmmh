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

	void * val1 = allocate(7);
	void * val2 = allocate(12);
	void ** val3 = (void **) allocate(8); // allows for a max 64-bit pointer to be stored 
	// (which keeps this test stable)

	set_type(val3, 1);
	*val3 = val1; // point to val1;
	// TODO replace with val2 as soon as reallocate supports pointer moving

	assert("0..0...1..");
	val2 = release(val2);
	assert("0..v...1..");
	val1 = reallocate(val1, 9); // should grow into next
	assert("0...v..1..");
	void * val4 = allocate(4); // should take space from old val2
	assert("0...0.v1..");
	void * val4_2 = reallocate(val4, 4); // should be unchanged
	if (val4_2 != val4) printf("E!\n");
	assert("0...0.v1..");
	val4_2 = reallocate(val4_2, 13); // should move
	assert("0...v..1..0....");

	tmmh_gc((void *) &val3, 1);
	assert("0...v..1..v....");
}

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
	void * val1 = allocate(7);
	void * val2 = allocate(12);
	/*void * val3 =*/ allocate(19);
	assert("H..H...H.....");
	val2 = release(val2);
	assert("H..h...H.....");
	val1 = reallocate(val1, 9); // should grow into next
	assert("H...h..H.....");
	void * val4 = allocate(4); // should take space from old val2
	assert("H...H.hH.....");
	void * val4_2 = reallocate(val4, 4); // should be unchanged
	if (val4_2 != val4) printf("E!\n");
	assert("H...H.hH.....");
	val4_2 = reallocate(val4_2, 13); // should move
	assert("H...h..H.....H....");
}

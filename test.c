#include <stdio.h>
#include <stdint.h>

#include "tmmh.h"

int main (int argc, char ** argv)
{
	char buffer[255];

	allocate(7);
	void * val = allocate(12);
	allocate(19);
	release(val);

	visualize(buffer);

	printf(buffer);
	printf("\n");
}

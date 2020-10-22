#include <stdio.h>

#include "heap2.h"

int main()
{
	struct heap_t *test = heap_make(100);

	void *a1 = heap_malloc(test, 50);
	for (int i = 0; i < 50; i++)
	{
		*((char *)a1 + i) = 0xFF;
	}
	heap_debug(test);

	printf("\n...\n");

	void *a2 = heap_malloc(test, 20);
	heap_debug(test);

	printf("\n...\n");

	heap_mfree(test, a2, 10);
	heap_debug(test);

	system("pause");

	return 0;
}
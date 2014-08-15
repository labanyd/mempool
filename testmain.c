#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mempool.h"

#define COUNT  1000000
#define SIZE  234
int main(int argc, char **argv)
{
	struct timeval start, end;
	int i;
	void *p;

	MemPool *pool = InitMemPool();

	gettimeofday(&start, 0);
	for (i = 0; i < COUNT; i++)
	{
		p = malloc(SIZE);
		free(p);
	}
	gettimeofday(&end, 0);
	
	printf("%d times malloc %d cost %ld mill seconds\n", COUNT , SIZE,
		(end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec));


	
	gettimeofday(&start, 0);
	//for (i = 0; i < COUNT; i++)
	while (1)
	{
		p = MemAlloc(pool, SIZE);
		MemFree(pool, p);	
	}
	gettimeofday(&end, 0);

	printf("%d times MemAlloc %d cost %ld mill seconds\n", COUNT, SIZE,
                (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec));

	/* the print list is long, so you can use ./testmain | grep "is 1" to test */
	//PrintMem(pool);
	DestroyMemPool(pool);
}

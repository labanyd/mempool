#include <stdio.h>
#include <stdlib.h>
#include "mempool.h"

void InsertBlk(Block *blk, Node *node);
Node * GetBlk(Block *blk);
void FreeNode(Block *blk, Node *node);

void
InitBlock(Block *blk)
{
	blk->m_pHead  = NULL;
	blk->m_pTail  = NULL;
	blk->m_nCount = 0;
#ifdef USELOCK
	pthread_mutex_init(&blk->m_mutex, NULL);
#endif
}


MemPool * 
InitMemPool(void)
{
	MemPool *pool = (MemPool *) malloc (sizeof(MemPool));
	if (NULL == pool) 
	{
		return NULL;
	}

	/* init used, free block */
	Block *blk;
	int i;
	for (i = 0; i < NBLOCK; i++) 
	{
		blk = pool->m_pUsed + i;
		InitBlock(blk);

		blk = pool->m_pFree + i;
		InitBlock(blk);
	}

	/* init big block */
	InitBlock(&pool->m_sBigBlk);


	return pool;	
}

Node *
AllocNode(int nSize)
{
	/* alloc nsize + sizeof(Node )  + sizeof(int)*/
	void * ptr = (void *) malloc (nSize + sizeof(Node) + sizeof(int)); 
	if (NULL == ptr)
	{
		return NULL;
	}

	Node *node = (Node *) ptr;	
	node->m_pData = ptr + sizeof(Node) + sizeof(int);
	node->m_pNext = NULL;
	node->m_pPrev = NULL;

	*(int *)(ptr + sizeof(Node)) = nSize;
	
	return node;
}

void *
MemAlloc(MemPool *pool, int allocSize)
{
	Node *node = NULL;
	int nSize = AlignSize(allocSize, ALIGNMIN);	
	
	/* more than MAX_MEM_POOL_SIZE */
	if (nSize > MAX_MEM_POOL_SIZE) 
	{
		node = AllocNode(nSize);
		InsertBlk(&pool->m_sBigBlk, node);
		memset(node->m_pData, 0, nSize);
		return node->m_pData ;
	}

	
	Block *used, *free;
	int nbyte = 0;
	nbyte = nSize >> MINBIT;

	nbyte = nbyte - 1;

	used  = pool->m_pUsed + nbyte;
	free  = pool->m_pFree + nbyte;

	node = GetBlk(free);	

	if (NULL == node) 
	{
		node = AllocNode(nSize);
		if (NULL == node)
		{
			return NULL;
		}
	}

	InsertBlk(used, node);

	memset(node->m_pData, 0, nSize);
	return node->m_pData;
}

void 
FreeNode(Block *blk, Node *node)
{
	/* lock */
	LOCK(&blk->m_mutex);
	
	if (node == blk->m_pHead) /* the head node */
	{
		blk->m_pHead = blk->m_pHead->m_pNext;
		blk->m_nCount--;
		node->m_pNext = NULL;

		if (blk->m_pHead == NULL) 
		{
			UNLOCK(&blk->m_mutex);
			return;
		}
		blk->m_pHead->m_pPrev = NULL;
	}
	else if (node == blk->m_pTail) /* the tail node */
	{
		blk->m_pTail = blk->m_pTail->m_pPrev;
		blk->m_nCount--;
		node->m_pPrev = NULL;
		
		if (blk->m_pTail == NULL)
		{
			UNLOCK(&blk->m_mutex);
			return;
		}
		blk->m_pTail->m_pNext = NULL;

	}
	else /* the mid node */
	{
		node->m_pPrev->m_pNext = node->m_pNext;
		node->m_pNext->m_pPrev = node->m_pPrev;	
		blk->m_nCount--;

		node->m_pNext = NULL;
		node->m_pPrev = NULL;
	}

	/* unlock */	
	UNLOCK(&blk->m_mutex);
}


void
MemFree(MemPool *pool, void *ptr)
{
	Node *node =(Node *)(ptr - sizeof(int) - sizeof(Node));
	int nSize  = *(int*)(ptr - sizeof(int));
	int nbyte  = 0;

	if (nSize > MAX_MEM_POOL_SIZE)
	{
		/* delelte from big block */
		FreeNode(&pool->m_sBigBlk, node);	
		/* free */
		free(node);
		return;
	}

	nbyte = nSize >> MINBIT;
		
	nbyte       = nbyte - 1;
	Block *free = pool->m_pFree + nbyte;
	Block *used = pool->m_pUsed + nbyte;

	/* delete from used block */
	FreeNode(used, node);

	/* insert to free block */
	InsertBlk(free, node);
	
	return;
}

Node *
GetBlk(Block *blk)
{
	/* lock */
	LOCK(&blk->m_mutex);	


	if (NULL == blk->m_pHead)
	{
		UNLOCK(&blk->m_mutex);
		return NULL;
	}

	Node *node    = blk->m_pHead;
	
	/* free list head = head->next */
	blk->m_pHead  = blk->m_pHead->m_pNext;
	if (NULL != blk->m_pHead)
	{
		blk->m_pHead->m_pPrev = NULL;
	}
	blk->m_nCount--; 

	node->m_pNext = NULL;
	node->m_pPrev = NULL;

	/* unlock */
	UNLOCK(&blk->m_mutex);

	return node;
}

void 
InsertBlk(Block *blk, Node *node)
{
	/* lock */
	LOCK(&blk->m_mutex);

	if (NULL == blk->m_pHead) 
	{
		blk->m_pHead  = node;
		blk->m_pTail  = node;
		blk->m_nCount++;
		/* unlock */
		UNLOCK(&blk->m_mutex);
		return;
	}

	blk->m_pTail->m_pNext = node;
	node->m_pPrev         = blk->m_pTail;
	blk->m_pTail          = node;
	blk->m_nCount++;

	/* unlock */
	UNLOCK(&blk->m_mutex);
}


void DestroyNode(Node *node)
{
	Node *tmp;

	tmp = node;
	while (node) 
	{
		node = node->m_pNext;
		free(tmp);
		tmp = node;
	}

}

void  DestroyMemPool(MemPool *pool)
{
	Node *node;
	Block *blk;
	int i;

	for (i = 0; i < NBLOCK; i++)
	{
		blk = pool->m_pUsed + i;
		node = blk->m_pHead;
		DestroyNode(node);
		blk->m_nCount = 0;

		blk = pool->m_pFree + i;
		node = blk->m_pHead;
		DestroyNode(node);
		blk->m_nCount = 0;
	}
	
	blk = &pool->m_sBigBlk;
	node = blk->m_pHead;
	DestroyNode(node);
	blk->m_nCount = 0;
	
	free(pool);
}

void PrintMem(MemPool *pool)
{
	
	Block *blk;
	int i, size;	

	for (i = 0; i < NBLOCK; i++)
	{
		blk = pool->m_pUsed + i;	
		size = (i + 1) <<  MINBIT;
		
		printf("the counter of %d size, used: is %d\t", size, blk->m_nCount);

		blk = pool->m_pFree + i;
		printf("free: is %d\n", blk->m_nCount);
	}

	blk = &pool->m_sBigBlk;
	
	printf("the counter of the big mem is %d\n", blk->m_nCount);

}

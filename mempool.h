#ifndef __MEMPOOL_H__IOUJJLJLK
#define __MEMPOOL_H__IOUJJLJLK

#include <pthread.h>

#define AlignSize(a, b) ( ((a) + ((b) - 1)) & (~((b) - 1)) )

#if (SYSBYTE == 4)
	#define   ALIGNMIN   4
	#define   MINBIT     2   /* 1 << 2 == 4 */
	#define   NBLOCK     1024  /* alloc 4 8 12 16  ... 4096 block */
#else
	#define   ALIGNMIN   8
	#define   MINBIT     3  /* 1 << 3 == 8 */
	#define   NBLOCK     512 /* alloc 8 16 24 32 ... 4096 block */
#endif

#define MAX_MEM_POOL_SIZE   4096

#ifdef USELOCK
	#define    LOCK(a)    pthread_mutex_lock(a)	
	#define    UNLOCK(a)  pthread_mutex_unlock(a)
#else
	#define    LOCK(a)     
	#define    UNLOCK(a)  	
#endif

struct Node
{
	void *m_pData; /* sizeof(struct Node *) + void *realData */
	struct Node *m_pNext;
	struct Node *m_pPrev;
} __attribute__((__packed__));

typedef struct Node Node;


typedef struct Block
{
	Node *m_pHead;
	Node *m_pTail;
	
	int   m_nCount;
#ifdef   USELOCK
	pthread_mutex_t m_mutex; /* need thread lock, as for thread safe */
#endif
}Block;



typedef struct
{
	Block  m_pUsed[NBLOCK]; 
	Block  m_pFree[NBLOCK];
	
	Block  m_sBigBlk; /* alloc more than 4096 block */

}MemPool;


MemPool * InitMemPool(void);
void *MemAlloc(MemPool *pool, int allocSize);
void  MemFree(MemPool *pool, void *ptr);
void  DestroyMemPool(MemPool *pool); 
void  PrintMem(MemPool *pool);

#endif

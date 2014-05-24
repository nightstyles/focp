
#include "VmmDevice.hpp"

#ifndef _VMM_PAGE_HPP_
#define _VMM_PAGE_HPP_

FOCP_BEGIN();

#define VMM_CACHE_GRANULARITY 64
#define VMM_CACHE_BLOCKSIZE 1024

class CVirtualCachePage;

struct CLruVirtualCachePageList
{
	CVirtualCachePage* pHead;
	CVirtualCachePage* pTail;
	CVirtualCachePage* pDirty;
};

class CVirtualCachePage
{
public:
	uint8 m_sPage[VMM_PAGE_SIZE];
	uint64 m_nPageAddr;
	CVirtualDevice* m_pDevice;
	CVirtualCachePage *m_pPrev, *m_pNext, *m_pDirty;
	CLruVirtualCachePageList* m_pLruList;
	uint32 m_nPageId, m_nCounter;
	uint32 m_nDirty;

public:
	CVirtualCachePage(uint32 nPageId, CLruVirtualCachePageList* pList);
	~CVirtualCachePage();

	uint32 MountPage(CVirtualDevice* pDevice, uint64 nPageAddr);

	void AddRef();
	uint32 Release();

	void Commit();

private:
	void DetachFromLru(CLruVirtualCachePageList* pList);
	void AttachIntoLru(CLruVirtualCachePageList* pList);
};

struct CVirtualPageHead
{
	uint16 nBlockSize;
	uint16 nTotalBlock;
	uint16 nFreeBlock;
	uint16 nAllocator;
	uint8 pTable[1];
};

class CVirtualPage
{
private:
	CVirtualPageHead * m_pPage;
	CVirtualCachePage* m_pCachePage;
	uint64 m_nPageAddr;
	uint32 m_nHeadSize;
	uint32 m_nTableSize;

public:
	CVirtualPage(uint64 nPageAddr, CVirtualCachePage* pCachePage, uint32 nBlockSize=0, uint32 nTotalBlock=0);
	CVirtualPage(uint64 nPageAddr, CVirtualPageHead* pPageHead, uint32 nBlockSize=0, uint32 nTotalBlock=0);
	~CVirtualPage();

	uint64 Allocate();
	bool Deallocate(uint64 nAddr, uint32 &nBlockSize);

	static uint32 GetTotalBlock(uint32 nBlockSize);
};

FOCP_END();

#endif


#include "VmmPage.hpp"

FOCP_BEGIN();

CVirtualCachePage::CVirtualCachePage(uint32 nPageId, CLruVirtualCachePageList* pList)
{
	m_pDevice = NULL;
	m_pDirty = m_pPrev = m_pNext = NULL;
	m_nPageAddr = 0;
	m_nPageId = nPageId;
	m_nCounter = 0;
	m_nDirty = 0;
	CBinary::MemorySet(m_sPage, 0, VMM_PAGE_SIZE);
	m_pLruList = pList;
	AttachIntoLru(pList);
}

CVirtualCachePage::~CVirtualCachePage()
{
}

uint32 CVirtualCachePage::MountPage(CVirtualDevice* pDevice, uint64 nPageAddr)
{
	m_nPageAddr = nPageAddr;
	m_pDevice = pDevice;
	pDevice->MountPage(((uint32)m_nPageAddr)>>16, (char*)m_sPage);
	return 0;
}

void CVirtualCachePage::AddRef()
{
	DetachFromLru(m_pLruList);
	AttachIntoLru(m_pLruList);
	++m_nCounter;
}

uint32 CVirtualCachePage::Release()
{
	if(m_nCounter)
		--m_nCounter;
	else
		Abort();
	return m_nCounter;
}

void CVirtualCachePage::Commit()
{
	if(!m_nDirty)
	{
		m_nDirty = 1;
		m_pDirty = m_pLruList->pDirty;
		m_pLruList->pDirty = this;
		SetVmmControlStatus(VMM_CONTROL_DIRTY, 1);
	}
}

void CVirtualCachePage::DetachFromLru(CLruVirtualCachePageList* pList)
{
	if(m_pPrev)
		m_pPrev->m_pNext = m_pNext;
	else
		pList->pHead = m_pNext;
	if(m_pNext)
		m_pNext->m_pPrev = m_pPrev;
	else
		pList->pTail = m_pPrev;
}

void CVirtualCachePage::AttachIntoLru(CLruVirtualCachePageList* pList)
{
	m_pPrev = NULL;
	m_pNext = pList->pHead;
	if(pList->pHead)
		pList->pHead->m_pPrev = this;
	else
		pList->pTail = this;
	pList->pHead = this;
}

CVirtualPage::CVirtualPage(uint64 nPageAddr, CVirtualCachePage* pCachePage, uint32 nBlockSize, uint32 nTotalBlock)
{
	m_nPageAddr = nPageAddr;
	m_pCachePage = pCachePage;
	m_pPage = (CVirtualPageHead*)pCachePage->m_sPage;
	if(nTotalBlock)
	{
		m_pPage->nAllocator = 0;
		m_pPage->nBlockSize = (uint16)nBlockSize;
		m_pPage->nFreeBlock = m_pPage->nTotalBlock = (uint16)nTotalBlock;
	}
	m_nHeadSize = VMM_PAGE_SIZE - m_pPage->nBlockSize * m_pPage->nTotalBlock;
	m_nTableSize = ((m_pPage->nTotalBlock-1)>>3) + 1;
	if(nTotalBlock)
	{
		CBinary::MemorySet(m_pPage->pTable, 0, m_nTableSize);
		m_pCachePage->Commit();
	}
}

CVirtualPage::CVirtualPage(uint64 nPageAddr, CVirtualPageHead* pPageHead, uint32 nBlockSize, uint32 nTotalBlock)
{
	m_nPageAddr = nPageAddr;
	m_pCachePage = NULL;
	m_pPage = pPageHead;
	if(nTotalBlock)
	{
		m_pPage->nAllocator = 0;
		m_pPage->nBlockSize = (uint16)nBlockSize;
		m_pPage->nFreeBlock = m_pPage->nTotalBlock = (uint16)nTotalBlock;
	}
	m_nHeadSize = VMM_PAGE_SIZE - m_pPage->nBlockSize * m_pPage->nTotalBlock;
	m_nTableSize = ((m_pPage->nTotalBlock-1)>>3) + 1;
	if(nTotalBlock)
		CBinary::MemorySet(m_pPage->pTable, 0, m_nTableSize);
}

CVirtualPage::~CVirtualPage()
{
}

uint64 CVirtualPage::Allocate()
{
	if(!m_pPage->nFreeBlock)
		return 0;
	register uint8 nBit = 1, iBit;
	register uint8* pTable = m_pPage->pTable;
	register uint16& nAllocator = m_pPage->nAllocator;
	while(true)
	{
		if(pTable[nAllocator] != 0xFF)
		{
			for(iBit=0; iBit<8; ++iBit)
			{
				if(!(nBit & pTable[nAllocator]))
				{
					pTable[nAllocator] |= nBit;
					--m_pPage->nFreeBlock;
					if(m_pCachePage)
						m_pCachePage->Commit();
					break;
				}
				nBit <<= 1;
			}
			break;
		}
		++nAllocator;
		if(nAllocator == m_nTableSize)
			nAllocator = 0;
	}
	register int32 nBlock = nAllocator*8 + iBit;
	return m_nPageAddr + m_nHeadSize + nBlock*m_pPage->nBlockSize;
}

bool CVirtualPage::Deallocate(uint64 nAddr, uint32 &nBlockSize)
{
#if VMM_SAFE_ENABLED
	if(m_pPage->nFreeBlock == m_pPage->nTotalBlock)
		Abort();
	if(nAddr < m_nPageAddr + m_nHeadSize)
		Abort();
#endif
	nAddr -= m_nPageAddr + m_nHeadSize;
	register uint32 nOffset = (uint32)nAddr;
#if VMM_SAFE_ENABLED
	if((nOffset % m_pPage->nBlockSize) || nOffset >= m_pPage->nBlockSize*m_pPage->nTotalBlock)
		Abort();
#endif
	register uint32 nBlock = nOffset / m_pPage->nBlockSize;
	register uint8 nIdx = (nBlock>>3);
	register uint8 nBit = (0x01<<(nBlock&7));
	if(m_pPage->pTable[nIdx] & nBit)
	{
		m_pPage->pTable[nIdx] &= ~nBit;
		++m_pPage->nFreeBlock;
		if(m_pCachePage)
			m_pCachePage->Commit();
	}
#if VMM_SAFE_ENABLED
	else
		Abort();
#endif
	nBlockSize = m_pPage->nBlockSize;
	return (m_pPage->nFreeBlock==0);
}

uint32 CVirtualPage::GetTotalBlock(uint32 nBlockSize)
{
	register uint32 nHeadSize;
	register uint32 nPageSize = VMM_PAGE_SIZE;
	register uint32 nNewTotalBlock, nTotalBlock = 0xFFFFFFFF;
	while(true)
	{
		nNewTotalBlock = nPageSize / nBlockSize;
		if(!nNewTotalBlock)
		{
			nTotalBlock = 0;
			break;
		}
		if(nNewTotalBlock >= nTotalBlock)
			break;
		nHeadSize = 8 + ((nNewTotalBlock-1)>>3) + 1;
		nPageSize = VMM_PAGE_SIZE - MakeAlign(nHeadSize, 8);
		nTotalBlock = nNewTotalBlock;
	}
	return nTotalBlock;
}

FOCP_END();

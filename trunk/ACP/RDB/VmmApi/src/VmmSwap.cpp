
#include "VmmStorage.hpp"

#ifdef MSVC
#define ComputePageAddr(nAddr) ((nAddr) & 0xFFFFFFFFFFFF0000i64)
#else
#define ComputePageAddr(nAddr) ((nAddr) & 0xFFFFFFFFFFFF0000ULL)
#endif

FOCP_BEGIN();

CVmmSwapper::CVmmSwapper(CVirtualStorage* pStorage)
:m_hFlushThread(this),m_hFlushEvent(true)
{
	m_pStorage = pStorage;
	m_nPageCount = 0;
	m_nFreePage = 0;
	m_pSwapPages = NULL;
	m_oLru.pHead = m_oLru.pTail = m_oLru.pDirty = NULL;
	m_nMountCount = 0;
	m_nFlushPageCount = 0;
	m_nFlushCount = 0;
	m_nApplyPageCount = 0;
}

CVmmSwapper::~CVmmSwapper()
{
}

uint32 CVmmSwapper::Startup(uint32 nSwapAreaSize)
{
	if(nSwapAreaSize >= 65535)
	{
		FocpError(("CVmmSwapper::Startup(%u Page) failure", nSwapAreaSize));
		return 1;
	}
	m_nPageCount = nSwapAreaSize;
	m_pSwapPages = (CVirtualCachePage*)CMalloc::Malloc(m_nPageCount*sizeof(CVirtualCachePage));
	if(!m_pSwapPages)
	{
		FocpError(("CVmmSwapper::Startup(%u Page) failure", nSwapAreaSize));
		return 1;
	}
	for(uint32 i=0; i<m_nPageCount; ++i)
		new(m_pSwapPages+i) CVirtualCachePage(i, &m_oLru);
	m_nFreePage = m_nPageCount;
	m_hEvent.Set();
	m_hFlushThread.Start();
	return 0;
}

void CVmmSwapper::CleanSwapper()
{
	m_hFlushThread.Stop(false);
	m_hFlushEvent.Set();
	m_hFlushThread.Stop(true);
	if(m_pSwapPages)
	{
		for(uint32 i=0; i<m_nPageCount; ++i)
			m_pSwapPages[i].~CVirtualCachePage();
		CMalloc::Free(m_pSwapPages);
		m_pSwapPages = NULL;
	}
}

void CVmmSwapper::CommitAllPages(bool bFlushAll)
{
	CVirtualCachePage* pPrev = NULL, *pPage, *pNext;
	CRbMap<uint64, uint32> oPages;
	m_oMutex.Enter();
	pPage = m_oLru.pDirty;
	while(pPage)
	{
		pNext = pPage->m_pDirty;
		if(!pPage->m_nCounter || bFlushAll)
		{
			oPages[pPage->m_nPageAddr] = pPage->m_nPageId;
			pPage->m_nDirty = 0;
			pPage->m_pDirty = NULL;
			if(pPrev)
				pPrev->m_pDirty = pNext;
			else
				m_oLru.pDirty = pNext;
			if(!m_nFreePage)
				m_hEvent.Set();
			++m_nFreePage;
			++m_nFlushPageCount;
		}
		else pPrev = pPage;
		pPage = pNext;
	}
	if(oPages.GetSize())
	{
		CRbTreeNode* idx = oPages.First();
		CRbTreeNode* end = oPages.End();
		for(; idx!=end; idx=oPages.GetNext(idx))
		{
			pPage = m_pSwapPages + oPages.GetItem(idx);
			uint32 nPageId = ((uint32)oPages.GetKey(idx))>>VMM_PAGE_BIT;
			pPage->m_pDevice->CommitPage(nPageId, (char*)pPage->m_sPage);
		}
		++m_nFlushCount;
	}
	m_oMutex.Leave();
}

void CVmmSwapper::MainProc(CCooperator* pThread, bool &bRunning)
{
	while(bRunning)
	{
		m_hFlushEvent.Wait();
		CommitAllPages();
	}
}

void* CVmmSwapper::Map(uint64 nAddr)
{
	uint64 nPageAddr = ComputePageAddr(nAddr);
	uint32 nOffset = (uint32)(nAddr - nPageAddr);
	return m_pSwapPages[ApplyPage(nPageAddr)].m_sPage + nOffset;
}

void CVmmSwapper::UnMap(void* pMem)
{
	uint32 nPageId = ((char*)pMem-(char*)m_pSwapPages)/sizeof(CVirtualCachePage);
#if VMM_SAFE_ENABLED
	if(nPageId >= m_nPageCount)
		Abort();
#endif
	m_oMutex.Enter();
	RelasePage(nPageId);
	m_oMutex.Leave();
}

void CVmmSwapper::CommitMap(void* pMem)
{
	uint32 nPageId = ((char*)pMem-(char*)m_pSwapPages)/sizeof(CVirtualCachePage);
#if VMM_SAFE_ENABLED
	if(nPageId >= m_nPageCount)
		Abort();
#endif
	m_oMutex.Enter();
	m_pSwapPages[nPageId].Commit();
	m_oMutex.Leave();
}

CVirtualCachePage* CVmmSwapper::MapPage(uint64 nPageAddr)
{
	return m_pSwapPages + ApplyPage(nPageAddr);
}

void CVmmSwapper::UnMapPage(CVirtualCachePage* pPage)
{
	m_oMutex.Enter();
	pPage->Release();
	if(!pPage->m_nCounter && !pPage->m_nDirty)
	{
		if(!m_nFreePage)
			m_hEvent.Set();
		++m_nFreePage;
	}
	m_oMutex.Leave();
}

uint32 CVmmSwapper::ApplyPage(uint64 nPageAddr)
{
	uint32 nRet;
	CVirtualCachePage* pPage;

	uint32 nDeviceNo = (uint32)(nPageAddr>>32);
	uint32 nPageIdx = ((uint32)nPageAddr)>>16;
	CVirtualDevice* pDevice = m_pStorage->m_pDevices[nDeviceNo];
#if VMM_SAFE_ENABLED
	if(!pDevice)
		Abort();
#endif

	m_oMutex.Enter();
	++m_nApplyPageCount;
	nRet = pDevice->m_pSwappTable[nPageIdx];
	if(nRet < m_nPageCount)
	{
		pPage = m_pSwapPages + nRet;
		if(!pPage->m_nCounter && !pPage->m_nDirty)
		{
			--m_nFreePage;
			if(!m_nFreePage)
				m_hEvent.Reset();
		}
		pPage->AddRef();
		m_oMutex.Leave();
		return nRet;
	}
	if(!m_nFreePage)
		m_hFlushEvent.Set();
	m_oMutex.Leave();

loop:
	m_hEvent.Wait(1000);

	m_oMutex.Enter();
	nRet = pDevice->m_pSwappTable[nPageIdx];
	if(nRet < m_nPageCount)
	{
		pPage = m_pSwapPages + nRet;
		if(!pPage->m_nCounter && !pPage->m_nDirty)
		{
			--m_nFreePage;
			if(!m_nFreePage)
				m_hEvent.Reset();
		}
	}
	else if(m_nFreePage)
	{
		pPage = m_oLru.pTail;
		while(pPage->m_nCounter || pPage->m_nDirty)
			pPage = pPage->m_pPrev;
		CVirtualDevice* pOldDevice = pPage->m_pDevice;
		if(pOldDevice)
			pOldDevice->m_pSwappTable[((uint32)pPage->m_nPageAddr)>>16] = 0xFFFF;
		--m_nFreePage;
		if(!m_nFreePage)
			m_hEvent.Reset();
		pPage->MountPage(pDevice, nPageAddr);
		nRet = pPage->m_nPageId;
		pDevice->m_pSwappTable[nPageIdx] = (uint16)nRet;
		++m_nMountCount;
	}
	else
	{
		m_hFlushEvent.Set();
		m_oMutex.Leave();
		if(!m_hFlushThread.GetCooperatorData())
			return 0;
		goto loop;
	}

	pPage->AddRef();
	m_oMutex.Leave();
	return nRet;
}

void CVmmSwapper::RelasePage(uint32 nPageId)
{
	CVirtualCachePage* pPage = m_pSwapPages + nPageId;
	pPage->Release();
	if(!pPage->m_nCounter && !pPage->m_nDirty)
	{
		if(!m_nFreePage)
			m_hEvent.Set();
		++m_nFreePage;
	}
}

FOCP_END();

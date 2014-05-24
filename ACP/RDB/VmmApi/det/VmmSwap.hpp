
#include "VmmPage.hpp"

#ifndef _VMM_SWAP_HPP_
#define _VMM_SWAP_HPP_

FOCP_BEGIN();

class CVirtualStorage;

class CVmmSwapper: public CCooperateFunction
{
private:
	CVirtualStorage* m_pStorage;
	CMutex m_oMutex;
	CCooperator m_hFlushThread;
	CEvent m_hEvent, m_hFlushEvent;
	uint32 m_nPageCount, m_nFreePage;
	CVirtualCachePage* m_pSwapPages;
	CLruVirtualCachePageList m_oLru;
	uint32 m_nMountCount, m_nFlushPageCount, m_nFlushCount, m_nApplyPageCount;

public:
	CVmmSwapper(CVirtualStorage* pStorage);
	virtual ~CVmmSwapper();

	uint32 Startup(uint32 nSwapAreaSize);
	void CleanSwapper();

	void* Map(uint64 nAddr);
	void UnMap(void* pMem);
	void CommitMap(void* pMem);

	void CommitAllPages(bool bFlushAll=false);

	CVirtualCachePage* MapPage(uint64 nPageAddr);
	void UnMapPage(CVirtualCachePage* pPage);

private:
	uint32 ApplyPage(uint64 nPageAddr);
	void RelasePage(uint32 nPageId);

protected:
	virtual void MainProc(CCooperator* pThread, bool &bRunning);
};

FOCP_END();

#endif


#include "Timer.hpp"

#include <time.h>

#ifdef UNIX
#include <sys/time.h>
#endif

#ifdef WINDOWS
#include <windows.h>
#endif

FOCP_BEGIN();

FOCP_DETAIL_BEGIN();

static CTimerItem* GetTimerItem(CRbTreeNode* pNode)
{
	return (CTimerItem*)((uint8*)(pNode) - FocpFieldOffset(CTimerItem, oNode));
}

CTimerItem::CTimerItem()
{
	Function = NULL;
}

const uint32* CQueryTimerKey::QueryKey(const CRbTreeNode* pNode)
{
	return &GetTimerItem((CRbTreeNode*)pNode)->nTimeOut;
}

FOCP_DETAIL_END();

CTimer::CTimer()
	:m_oTimerTable(false)
{
}

CTimer::~CTimer()
{
}

CTimer* CTimer::GetInstance()
{
	return CSingleInstance<CTimer>::GetInstance();
}

bool CTimer::Initialize(uint32 nCapacity)
{
	uint32 nMod = nCapacity%32;
	if(nMod)
		nCapacity += 32 - nMod;
	if(nCapacity == 0)
		nCapacity = 2048*32;
	CAllocatePolicy oPolicy = {nCapacity, nCapacity, nCapacity};
	return m_oAllocator.SetAllocatePolicy(oPolicy);
}

void CTimer::Cleanup()
{
	m_oTimerTable.Clear();
	m_oAllocator.Clear();
}

uint32 CTimer::SetTimer(uint32 nElapse, FTimeOutFunction Function, uint8* pData, uint32 nDataLen)
{
	if(nDataLen > FOCP_TIMER_PRIVATE_DATA_LEN || !Function)
		return 0xFFFFFFFF;

	if(nElapse == (uint32)(-1))
		--nElapse;

	FOCP_DETAIL_NAME::CTimerItem * pItem = (FOCP_DETAIL_NAME::CTimerItem*)m_oAllocator.CreateObject();
	if(pItem == NULL)
		return 0xFFFFFFFF;

	// return timer id;
	uint32 nTimerId = m_oAllocator.GetObjectIndex(pItem);

	// init timer item
	uint32 nTime = GetTickCount() + nElapse;

	pItem->nTimeOut = nTime;
	pItem->nDataLen = nDataLen;
	if(pData && nDataLen>0)
		CBinary::MemoryCopy(pItem->pData, pData, nDataLen);
	else
	{
		pItem->nDataLen = 0;
		CBinary::MemorySet(pItem->pData, 0, FOCP_TIMER_PRIVATE_DATA_LEN);
	}

	// put in timer table;
	m_oMutex.Enter();
	pItem->Function = Function;
	m_oTimerTable.Insert(&pItem->oNode);
	m_oMutex.Leave();

	return nTimerId;
}

bool CTimer::KillTimer(uint32 nTimerId)
{
	FOCP_DETAIL_NAME::CTimerItem * pItem = (FOCP_DETAIL_NAME::CTimerItem*)m_oAllocator.QueryObject(nTimerId);
	if(pItem == NULL || pItem->Function == NULL)
		return false;

	bool bFind = false;
	m_oMutex.Enter();
	if(pItem->Function == NULL)
	{
		m_oMutex.Leave();
		return false;
	}
	CRbTreeNode* idx = m_oTimerTable.Find(pItem->nTimeOut);
	CRbTreeNode* end = m_oTimerTable.End();
	for(; idx!=end; idx=m_oTimerTable.GetNext(idx))
	{
		FOCP_DETAIL_NAME::CTimerItem* pItem2 = FOCP_DETAIL_NAME::GetTimerItem(idx);
		if(pItem == pItem2)
		{
			m_oTimerTable.Remove(idx);
			pItem->Function = NULL;
			bFind = true;
			break;
		}
		if(pItem->nTimeOut != pItem2->nTimeOut)
			break;
	}
	m_oMutex.Leave();

	if(bFind == false)
		return false;

	m_oAllocator.DestroyObject(pItem);
	return true;
}

void CTimer::ProcessOnce(CCooperator* pThread, bool &bRunning)
{
	CBaseSingleList<CRbTreeNode> oNewSet(0);
	uint32 nTime1 = GetTickCount();
	while(bRunning)
	{
		CCooperator::Sleep(5);
		uint32 nTime2 = GetTickCount();
		m_oMutex.Enter();
		if(m_oTimerTable.GetSize() == 0)
		{
			m_oMutex.Leave();
			nTime1 = nTime2;
			continue;
		}
		CRbTreeNode* pFirst = m_oTimerTable.LowerBound(nTime1);
		if(nTime1 < nTime2)
		{
			CRbTreeNode* pIdx, *pLast = m_oTimerTable.UpperBound(nTime2);
			if(pFirst != pLast)
			{
				for(pIdx=pFirst; pIdx!=pLast; )
				{
					CRbTreeNode* pNext = m_oTimerTable.Remove(pIdx);
					oNewSet.Push(pIdx);
					pIdx = pNext;
				}
			}
		}
		else
		{
			CRbTreeNode* pIdx, *pEnd = m_oTimerTable.End();
			if(pFirst != pEnd)
			{
				for(pIdx=pFirst; pIdx!=pEnd; )
				{
					CRbTreeNode* pNext = m_oTimerTable.Remove(pIdx);
					oNewSet.Push(pIdx);
					pIdx = pNext;
				}
			}
			CRbTreeNode* pLast = m_oTimerTable.UpperBound(nTime2);
			CRbTreeNode* pBeg = m_oTimerTable.First();
			if(pBeg != pLast)
			{
				for(pIdx=pBeg; pIdx!=pLast; )
				{
					CRbTreeNode* pNext = m_oTimerTable.Remove(pIdx);
					oNewSet.Push(pIdx);
					pIdx = pNext;
				}
			}
		}
		m_oMutex.Leave();
		if(oNewSet.GetSize())
		{
			CRbTreeNode* pIdx;
			while((pIdx=oNewSet.Pop()))
			{
				FOCP_DETAIL_NAME::CTimerItem * pItem = FOCP_DETAIL_NAME::GetTimerItem(pIdx);
				pItem->Function(pItem->pData, pItem->nDataLen, m_oAllocator.GetObjectIndex(pItem));
				m_oAllocator.DestroyObject(pItem);
			}
		}
		nTime1 = nTime2;
	}

	m_oMutex.Enter();
	CRbTreeNode* pIdx = m_oTimerTable.First();
	CRbTreeNode* pEnd = m_oTimerTable.End();
	while(pIdx!=pEnd)
	{
		FOCP_DETAIL_NAME::CTimerItem * pItem = FOCP_DETAIL_NAME::GetTimerItem(pIdx);
		pIdx=m_oTimerTable.GetNext(pIdx);
		m_oAllocator.DestroyObject(pItem);
	}
	m_oTimerTable.Clear();
	m_oMutex.Leave();
}

uint32 CTimer::GetTickCount()
{
#ifdef WINDOWS
	return ::GetTickCount();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000+tv.tv_usec/1000;
#endif
}

uint32 CTimer::GetTime()
{
	return (uint32)time(NULL);
}

FOCP_END();
